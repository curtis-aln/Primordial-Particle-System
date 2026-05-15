#include "simulation.h"

#include "imgui-SFML.h"
#include "imgui.h"
#include "implot.h"

Simulation::Simulation() : window_(sf::VideoMode({ screen_width, screen_height }), simulation_title)//, sf::Style::None)
{
	window_.setFramerateLimit(max_frame_rate);
	window_.setVerticalSyncEnabled(Vsync);

	if (set_Random_seed)
	{
		Random::set_seed(0);
	}

	init_imGUI();
}

void Simulation::init_imGUI()
{
	if (!ImGui::SFML::Init(window_))
		std::cerr << "[ERROR]: Failed to initialize ImGui-SFML\n";

	const int ui_scale_percent = 100.f;
	ImGui::GetIO().FontGlobalScale = ui_scale_percent / 100.f;

	ImPlot::CreateContext();
}


void Simulation::run()
{
	m_sim_thread_ = std::thread([this]
		{
			while (running)
				update();
		});

	while (running)
	{
		handle_events();
		manage_frame_rate();
		render();
	}

	m_sim_thread_.join();
	ImGui::SFML::Shutdown();
	ImPlot::DestroyContext();
}

void Simulation::quit()
{
	running_ = false;
}

void Simulation::update()
{
	resolve_modifications();

	for (size_t i = 0; i < sub_iterations; ++i)
	{
		if (iterations_ % add_to_grid_freq == 0)
		{
			particle_system_.add_particles_to_grid();
		}
		particle_system_.update_particles();
	}

	iterations_ += sub_iterations;


	// Package results into the triple buffer
	SimSnapshot& snap = m_sim_buffer_.get_write_buffer();

	// Filling the snapshot with information
	particle_system_.fill_snapshot(snap);
	snap.stats.fps = fps_;
	snap.stats.m_total_time_elapsed_ = m_total_time_elapsed_;

	m_sim_buffer_.publish();
}

void Simulation::render()
{
	if (!m_sim_buffer_.has_new_frame())
	{
		return;
	}

	// Always grab the freshest completed simulation frame
	const SimSnapshot& snap = m_sim_buffer_.begin_read();
	float dt = static_cast<float>(m_delta_time_.get_delta());
	m_total_time_elapsed_ += dt;

	const sf::Vector2f mouse_pos = camera.get_world_mouse_pos();

	window_.clear(screen_color);

	pps_renderer_.render(snap, camera.get_current_zoom(), camera);

	if (debug_)
	{
		pps_renderer_.render_debug(snap, mouse_pos, debug_radius);
		particle_system_.beacons.render(window_);
	}

	handle_imGUI(snap, dt);

	m_sim_buffer_.end_read();

	ImGui::SFML::Render(window_);
	window_.display();
}


void Simulation::manage_frame_rate()
{
	fps_ = static_cast<float>(clock_.get_average_frame_rate());
	clock_.update_frame_rate();

	std::ostringstream title;
	title << "Primordial Particle System"
		<< " | FPS: " << std::fixed << std::setprecision(1) << fps_;

	window_.setTitle(title.str());
}


void Simulation::handle_imGUI(const SimSnapshot& snap, float dt)
{

	sf::Time delta_time = sf::seconds(static_cast<float>(dt));
	ImGui::SFML::Update(window_, delta_time);

    draw_tab(snap, dt);
}


void Simulation::draw_tab(const SimSnapshot& snap, float dt)
{
    const auto& stats = snap.stats;
    const auto& toggles = particle_system_.toggles;

    constexpr float PANEL_WIDTH = 300.f;

    ImGui::SetNextWindowPos({ 10.f, 10.f }, ImGuiCond_Once);
    ImGui::SetNextWindowSize({ PANEL_WIDTH, 0.f }, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.85f);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 8.f, 6.f });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 14.f, 12.f });

    ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.08f, 0.08f, 0.10f, 0.90f });
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, { 0.13f, 0.13f, 0.18f, 1.00f });
    ImGui::PushStyleColor(ImGuiCol_Separator, { 0.30f, 0.30f, 0.35f, 0.80f });

    if (ImGui::Begin("  Primordial Particle System", nullptr, flags))
    {
        // ── Description ───────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.55f, 0.55f, 0.65f, 1.f });
        ImGui::TextWrapped("An emergent self-organising particle simulation.");
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ── Performance ───────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.85f, 0.75f, 0.40f, 1.f });
        ImGui::Text("  Performance");
        ImGui::PopStyleColor();

        ImGui::Spacing();

        auto stat_row = [](const char* label, const char* fmt, ...)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, { 0.55f, 0.55f, 0.65f, 1.f });
                ImGui::Text("%-18s", label);
                ImGui::PopStyleColor();
                ImGui::SameLine();

                char buf[64];
                va_list args;
                va_start(args, fmt);
                vsnprintf(buf, sizeof(buf), fmt, args);
                va_end(args);

                ImGui::PushStyleColor(ImGuiCol_Text, { 0.95f, 0.95f, 0.95f, 1.f });
                ImGui::Text("%s", buf);
                ImGui::PopStyleColor();
            };

        const float render_fps = stats.fps;
        const float update_fps = stats.updating_fps;

        // colour-code FPS: green > 55, yellow > 30, red otherwise
        auto fps_color = [](float f) -> ImVec4 {
            if (f >= 55.f) return { 0.35f, 0.90f, 0.45f, 1.f };
            if (f >= 30.f) return { 0.95f, 0.80f, 0.20f, 1.f };
            return               { 0.95f, 0.30f, 0.25f, 1.f };
            };

        ImGui::PushStyleColor(ImGuiCol_Text, { 0.55f, 0.55f, 0.65f, 1.f });
        ImGui::Text("%-18s", "Render FPS");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, fps_color(render_fps));
        ImGui::Text("%.1f", render_fps);
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, { 0.55f, 0.55f, 0.65f, 1.f });
        ImGui::Text("%-18s", "Update FPS");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, fps_color(update_fps));
        ImGui::Text("%.1f", update_fps);
        ImGui::PopStyleColor();

        stat_row("Particles", "%d", stats.cell_particle_count);
        stat_row("Iterations", "%d", stats.iterations_);
        stat_row("Time Elapsed", "%.2f s", stats.m_total_time_elapsed_);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ── Toggles ───────────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.85f, 0.75f, 0.40f, 1.f });
        ImGui::Text("  Toggles");
        ImGui::PopStyleColor();

        ImGui::Spacing();

        struct Toggle {
            const char* label;
            const char* key;
            bool        state;
        };

        const Toggle toggles_list[] = {
            { "Paused",       "[Space]", toggles.paused           },
            { "Debug",        "[D]",     toggles.debug_mode       },
            { "Rendering",    "[R]",     toggles.m_rendering_     },
            { "Hide Panels",  "[Q]",     toggles.hide_panels      },
            { "Draw Grid",    "[ ]",     toggles.draw_grid        },
            { "Statistics",   "[ ]",     toggles.track_statistics },
            { "Tick Frame",   "[O]",     toggles.m_tick_frame_time},
        };

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 8.f, 5.f });

        for (const auto& t : toggles_list)
        {
            // Indicator dot
            ImVec4 dot_col = t.state
                ? ImVec4{ 0.35f, 0.90f, 0.45f, 1.f }
            : ImVec4{ 0.45f, 0.45f, 0.50f, 1.f };

            ImGui::PushStyleColor(ImGuiCol_Text, dot_col);
            ImGui::Text(t.state ? "  " : "  ");  // spacing placeholder
            ImGui::SameLine(0.f, 0.f);
            ImGui::Text("%s", t.state ? "\xe2\x97\x8f" : "\xe2\x97\x8b"); // ● / ○
            ImGui::PopStyleColor();

            ImGui::SameLine(30.f);
            ImGui::PushStyleColor(ImGuiCol_Text, { 0.90f, 0.90f, 0.90f, 1.f });
            ImGui::Text("%-16s", t.label);
            ImGui::PopStyleColor();

            ImGui::SameLine(PANEL_WIDTH - 60.f);
            ImGui::PushStyleColor(ImGuiCol_Text, { 0.45f, 0.55f, 0.75f, 1.f });
            ImGui::Text("%s", t.key);
            ImGui::PopStyleColor();
        }

        ImGui::PopStyleVar(); // ItemSpacing (toggles)

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Text, { 0.35f, 0.35f, 0.40f, 1.f });
        ImGui::Text("[Esc] Quit   [O] Step frame   [Scroll] Zoom");
        ImGui::PopStyleColor();
    }
    ImGui::End();

    ImGui::PopStyleColor(3); // WindowBg, TitleBgActive, Separator
    ImGui::PopStyleVar(4);   // WindowRounding, FrameRounding, ItemSpacing, WindowPadding
}