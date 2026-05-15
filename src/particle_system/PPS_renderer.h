#pragma once
#include "heatmap.h"
#include "../settings.h"
#include "../utils/utils.h"
#include "../utils/font.h"
#include "simulation/context/sim_snapshot.h"
#include "utils/Camera.hpp"

// This renders all the particles on the screen
// A circle texture is first generated then it is replicated in a vertex array


struct SimSnapshot;

inline sf::Texture generate_circle_texture(float radius)
{
	const auto r = static_cast<unsigned>(radius * 2.f);

	sf::RenderTexture renderTexture({ r, r });
	sf::CircleShape circle(radius);

	circle.setFillColor(sf::Color::White);
	circle.setOrigin({ radius, radius });
	circle.setPosition({ radius, radius });
	renderTexture.clear(sf::Color::Transparent);
	renderTexture.draw(circle);
	renderTexture.display();

	sf::Texture tex = renderTexture.getTexture();
	return tex;
}


// colors
inline static constexpr std::uint8_t alpha_col = 200;
inline static constexpr sf::Color Red = { 255, 0, 0, alpha_col };
inline static constexpr sf::Color Green = { 50, 255, 0, alpha_col };
inline static constexpr sf::Color Blue = { 0, 0, 255, alpha_col };
inline static constexpr sf::Color Magenta = { 255, 0, 255, alpha_col };
inline static constexpr sf::Color Yellow = { 255, 255, 0, alpha_col };
inline static constexpr sf::Color Pink = { 255, 192, 203, alpha_col };

// transition thresholds - determined by nearby particles
inline static constexpr float range1 = 22;
inline static constexpr float range2 = 34;
inline static constexpr float range3 = 40;

// colors to be mapped to the ranges
inline static constexpr sf::Color first_color = Green;
inline static constexpr sf::Color second_color = Blue;
inline static constexpr sf::Color third_color = Pink;
inline static constexpr sf::Color fourth_color = Red;

// turning a nearby count into a color which is smoothed
inline sf::Color get_color(const float nearby_count)
{
	if (nearby_count <= 0.0f)
	{
		return first_color;
	}
	if (nearby_count <= range1)
	{
		float factor = nearby_count / range1;
		return {
			static_cast<std::uint8_t>(first_color.r + factor * (second_color.r - first_color.r)),
			static_cast<std::uint8_t>(first_color.g + factor * (second_color.g - first_color.g)),
			static_cast<std::uint8_t>(first_color.b + factor * (second_color.b - first_color.b))
		};
	}
	if (nearby_count <= range2)
	{
		float factor = (nearby_count - range1) / (range2 - range1);
		return {
			static_cast<std::uint8_t>(second_color.r + factor * (third_color.r - second_color.r)),
			static_cast<std::uint8_t>(second_color.g + factor * (third_color.g - second_color.g)),
			static_cast<std::uint8_t>(second_color.b + factor * (third_color.b - second_color.b))
		};
	}
	if (nearby_count < range3)
	{
		float factor = (nearby_count - range2) / (range3 - range2);
		return {
			static_cast<std::uint8_t>(third_color.r + factor * (fourth_color.r - third_color.r)),
			static_cast<std::uint8_t>(third_color.g + factor * (fourth_color.g - third_color.g)),
			static_cast<std::uint8_t>(third_color.b + factor * (fourth_color.b - third_color.b))
		};
	}
	return fourth_color;
}




class PPS_Renderer
{
	sf::Texture texture;
	sf::VertexArray vertex_array{ sf::PrimitiveType::Triangles };

	sf::RenderStates states{};

	// Debug rendering
	sf::CircleShape visual_radius_shape_;
	sf::VertexArray debug_lines_;

	sf::RenderWindow* window_ = nullptr;
	Font debug_font_;


	DensityHeatmap heatmap{ PPS_Settings::world_width, PPS_Settings::world_height, SimulationSettings::screen_width, SimulationSettings::screen_height };


public:
	PPS_Renderer(sf::RenderWindow* window)
		: window_(window), debug_font_(window, 80, "fonts/Calibri.ttf")
	{
		visual_radius_shape_.setFillColor(sf::Color::Transparent);
		visual_radius_shape_.setOutlineThickness(5);
		visual_radius_shape_.setOutlineColor(sf::Color(255, 255, 255, 100));
		visual_radius_shape_.setRadius(PPS_Settings::visual_radius);

		debug_lines_.setPrimitiveType(sf::PrimitiveType::Lines);

		texture = generate_circle_texture(PPS_Settings::particle_radius);
		texture.setSmooth(true);

		states.blendMode = sf::BlendAdd;
		
		std::cout << "Renderer Initialized\n";
	}

	static float zoom_to_alpha(const float zoom,
		const float zoom_min, const float zoom_max,
		const float alpha_min, const float alpha_max)
	{
		const float t = std::clamp((zoom - zoom_min) / (zoom_max - zoom_min), 0.f, 1.f);
		return alpha_min + t * (alpha_max - alpha_min);
	}

	void render(const SimSnapshot& snapshot, const float zoom, Camera& camera)
	{
		heatmap.clear();
		heatmap.scatter(snapshot.render.positions_x, snapshot.render.positions_y, PPS_Settings::particle_count, camera.m_view_);
		heatmap.upload();          // auto-normalises to peak density each frame
		heatmap.draw(*window_);
		return;

		const float alpha = zoom_to_alpha(zoom, 0.001f, 0.01f, 45.f, 225.f);
		std::cout << alpha << "\n";

		auto* positions_x = &snapshot.render.positions_x;
		auto* positions_y = &snapshot.render.positions_y;
		auto* neighbourhood_count = &snapshot.render.neighbourhood_count_;
		const int circle_count_ = static_cast<int>(positions_x->size());
		const size_t vertex_count = circle_count_ * 6;
		vertex_array.resize(vertex_count);

		auto tex_size = static_cast<float>(texture.getSize().x);
		constexpr float u0 = 0.f, v0 = 0.f;
		const float u1 = tex_size, v1 = tex_size;

		for (size_t i = 0; i < circle_count_; ++i)
		{
			const size_t base = i * 6;
			const float pos_x = (*positions_x)[i];
			const float pos_y = (*positions_y)[i];
			const float r = PPS_Settings::particle_radius;
			sf::Color col = get_color((*neighbourhood_count)[i]);

			col.a = alpha;

			vertex_array[base + 0] = { .position = { pos_x - r, pos_y - r }, .color = col, .texCoords = {u0, v0} };
			vertex_array[base + 1] = { .position = { pos_x + r, pos_y - r }, .color = col, .texCoords = {u1, v0} };
			vertex_array[base + 2] = { .position = { pos_x + r, pos_y + r }, .color = col, .texCoords = {u1, v1} };
			vertex_array[base + 3] = { .position = { pos_x - r, pos_y - r }, .color = col, .texCoords = {u0, v0} };
			vertex_array[base + 4] = { .position = { pos_x + r, pos_y + r }, .color = col, .texCoords = {u1, v1} };
			vertex_array[base + 5] = { .position = { pos_x - r, pos_y + r }, .color = col, .texCoords = {u0, v1} };
		}

		window_->draw(vertex_array, states);
	}
	
	void render_debug(const SimSnapshot& snapshot, const sf::Vector2f mouse_pos, const float mouse_radius)
	{
		auto* positions_x = &snapshot.render.positions_x;
		auto* positions_y = &snapshot.render.positions_y;
		auto* neighbourhood_count = &snapshot.render.neighbourhood_count_;
		auto* angles = &snapshot.render.angles_;

		constexpr float visual_radius = PPS_Settings::visual_radius;

		debug_lines_.clear();
		debug_lines_.resize(positions_x->size() * 2);

		for (size_t i = 0; i < positions_x->size(); ++i) 
		{
			const sf::Vector2f& position = { (*positions_x)[i], (*positions_y)[i] };

			if (dist_squared(position, mouse_pos) > mouse_radius * mouse_radius)
			{
				continue;
			}

			const float angle = (*angles)[i];

			// Draw direction line
			sf::Vector2f direction = sf::Vector2f(std::sin(angle), std::cos(angle)) * PPS_Settings::particle_radius;
			debug_lines_[i * 2].position = position;
			debug_lines_[i * 2].color = sf::Color::White;
			debug_lines_[i * 2 + 1].position = position + direction;
			debug_lines_[i * 2 + 1].color = sf::Color::White;

			// Draw visual radius
			visual_radius_shape_.setPosition(position - sf::Vector2f(visual_radius, visual_radius));
			window_->draw(visual_radius_shape_, sf::BlendAdd);

			// Draw debug text
			sf::Vector2f text_pos = position + sf::Vector2f(0, PPS_Settings::particle_radius);
			debug_font_.draw(text_pos, "nearby: " + std::to_string((*neighbourhood_count)[i]), true);
			debug_font_.draw(text_pos + sf::Vector2f(0, 30), "angle: " + std::to_string((*angles)[i]), true);
		}

		window_->draw(debug_lines_);
	}
};
