#include "imgui-SFML.h"
#include "simulation.h"

void Simulation::handle_events()
{
	const sf::Vector2f cam_pos = camera.get_world_mouse_pos();

	while (const std::optional event = window_.pollEvent())
	{
		ImGui::SFML::ProcessEvent(window_, *event);
		dispatch_event(*event, cam_pos);
	}

	camera.update(clock_.get_delta_time());
}



void Simulation::dispatch_event(const sf::Event& event, const sf::Vector2f& cam_pos)
{
	if (event.is<sf::Event::Closed>())
		running = false;

	else if (const auto* key = event.getIf<sf::Event::KeyPressed>())
	{
		handle_keyboard_events(key->code);
	}
	else if (const auto* scroll = event.getIf<sf::Event::MouseWheelScrolled>())
	{
		if (!ImGui::GetIO().WantCaptureMouse)  // don't zoom sim if imgui is using scroll
			camera.zoom(scroll->delta);
	}
	else if (event.is<sf::Event::MouseButtonPressed>())
	{
		if (!ImGui::GetIO().WantCaptureMouse)  // don't interact with sim if imgui is focused
			handle_mouse_press(cam_pos);
	}
	else if (event.is<sf::Event::MouseButtonReleased>())
		handle_mouse_release();
}



void Simulation::handle_keyboard_events(const sf::Keyboard::Key& event_key_code)
{
	switch (event_key_code)
	{
	case sf::Keyboard::Key::Escape: running = false;              break;
	case sf::Keyboard::Key::Space:  handle_pause_toggle();           break;
	case sf::Keyboard::Key::R:      particle_system_.toggles.m_rendering_ = !particle_system_.toggles.m_rendering_; break;
	case sf::Keyboard::Key::Q:      particle_system_.toggles.hide_panels = !particle_system_.toggles.hide_panels; break;
	case sf::Keyboard::Key::O:
		particle_system_.toggles.m_tick_frame_time = true;
		particle_system_.toggles.paused = true;
		break;

	case sf::Keyboard::Key::D:
		debug_ = !debug_;
		break;

	case sf::Keyboard::Key::G:
		render_hash_grid_ = !render_hash_grid_;
		break;

	default: break;
	}
}


void Simulation::handle_pause_toggle()
{
	WorldToggles& toggles = particle_system_.toggles;

	bool& paused = toggles.paused;
	paused = !paused;
}

void Simulation::handle_mouse_press(const sf::Vector2f& cam_pos)
{
	camera.begin_pan();  // start pan only if we didn't click an organism
}

void Simulation::handle_mouse_release()
{
	camera.end_pan();
}