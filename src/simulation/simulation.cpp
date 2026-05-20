#include "simulation.h"

#include "imgui-SFML.h"
#include "imgui.h"
#include "implot.h"

// ── Constructor ───────────────────────────────────────────────────────────────
Simulation::Simulation()
    : window_(sf::VideoMode({ screen_width, screen_height }), simulation_title)
{
    window_.setFramerateLimit(144);
    window_.setVerticalSyncEnabled(Vsync);

    if (set_Random_seed)
        Random::set_seed(0);

    init_imGUI();

    camera.m_view_.move({ world_width / 2.f, world_height / 2.f });
}

// ── ImGui init ────────────────────────────────────────────────────────────────
void Simulation::init_imGUI()
{
    if (!ImGui::SFML::Init(window_))
        std::cerr << "[ERROR]: Failed to initialize ImGui-SFML\n";

    ImGui::GetIO().FontGlobalScale = 1.0f;
    ImPlot::CreateContext();
}

// ── Main loop ─────────────────────────────────────────────────────────────────
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
    running = false;
}

// ── Update (sim thread) ───────────────────────────────────────────────────────
void Simulation::update()
{
    resolve_modifications();

    if (particle_system_.iterations_ % add_to_grid_freq == 0)
        particle_system_.add_particles_to_grid();

    particle_system_.update_particles();

    SimSnapshot& snap = m_sim_buffer_.get_write_buffer();
    particle_system_.fill_snapshot(snap);
    snap.stats.fps = fps_;
    snap.stats.m_total_time_elapsed_ = m_total_time_elapsed_;

    // Copy current toggles into snapshot so the render thread can read them
    snap.toggles = particle_system_.toggles;

    m_sim_buffer_.publish();
}

// ── Command dispatch ──────────────────────────────────────────────────────────
void Simulation::resolve_modifications()
{
    std::queue<SimCommand> local;
    {
        std::lock_guard<std::mutex> lock(m_cmd_mutex);
        std::swap(local, m_commands);
    }

    while (!local.empty())
    {
        SimCommand cmd = std::move(local.front());
        local.pop();

        switch (cmd.type)
        {
            // ── Toggles ───────────────────────────────────────────────────────────
        case CommandType::SetToggles:
            particle_system_.toggles = cmd.toggles;
            break;

            // ── Reset ─────────────────────────────────────────────────────────────
        case CommandType::ResetSimulation:
            particle_system_.init_grid_positioning();
            break;

            // ── Physics ───────────────────────────────────────────────────────────
        case CommandType::SetAlpha:
            PPS_Settings::alpha = cmd.float_val;
            break;

        case CommandType::SetBeta:
            PPS_Settings::beta = cmd.float_val;
            break;

        case CommandType::SetGamma:
            // IMGUI_TODO: gamma was constexpr — promoted to inline static float
            //   in settings.h.  Once that change is made, this line is live.
            PPS_Settings::gamma = cmd.float_val;
            break;

            // ── World ─────────────────────────────────────────────────────────────
        case CommandType::RandomizeSimulation:
            // IMGUI_TODO: expose randomize_angles() + init_grid_positioning()
            //   as public on ParticlePopulation (currently they are private).
            //   Then uncomment:
            //   particle_system_.randomize_angles();
            //   particle_system_.init_grid_positioning();
            break;

        case CommandType::ClearBeacons:
            // IMGUI_TODO: add a clear() method to the Beacons class:
            //   particle_system_.beacons.clear();
            break;

        case CommandType::SetThreadCount:
            // IMGUI_TODO: resizing the thread pool at runtime requires:
            //   particle_system_.thread_pool.wait_all();   // drain in-flight tasks
            //   particle_system_.thread_pool = tp::ThreadPool(cmd.int_val);
            //   particle_system_.precompute_thread_ranges();
            //   thread_pool and precompute_thread_ranges() must be made public first.
            break;

        default:
            break;
        }
    }
}

// ── Render (main thread) ──────────────────────────────────────────────────────
void Simulation::render()
{
    if (!m_sim_buffer_.has_published()) return;

    const bool         is_new_frame = m_sim_buffer_.has_new_frame();
    const SimSnapshot& snap = m_sim_buffer_.begin_read();

    if (is_new_frame)
        pps_renderer_.notify_new_snapshot(snap);

    const float dt = static_cast<float>(m_delta_time_.get_delta());
    m_total_time_elapsed_ += dt;

    const sf::Vector2f mouse_pos = camera.get_world_mouse_pos();

    window_.clear(screen_color);

    pps_renderer_.render(snap, camera);

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

// ── Frame rate ────────────────────────────────────────────────────────────────
void Simulation::manage_frame_rate()
{
    fps_ = static_cast<float>(clock_.get_average_frame_rate());
    clock_.update_frame_rate();

    std::ostringstream title;
    title << "Primordial Particle System"
        << " | FPS: " << std::fixed << std::setprecision(1) << fps_;
    window_.setTitle(title.str());
}