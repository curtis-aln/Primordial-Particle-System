#pragma once
#include "../particle_system/density_grid/density_grid_renderer.h"
#include "../settings.h"
#include "heatmap.h"
#include "utils/Camera.hpp"
#include <cstdint>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>
#include <vector>

#include "../utils/css_spatial_grid/css_spatial_grid.h"
#include "../utils/css_spatial_grid/css_spatial_grid_renderer.h"

struct SimSnapshot;

class PPS_Renderer
{
	sf::Texture texture;
	sf::VertexArray vertex_array{ sf::PrimitiveType::Triangles };

	sf::RenderStates states{};

	// Debug rendering
	sf::CircleShape visual_radius_shape_;
	sf::VertexArray debug_lines_;

	sf::RenderWindow* window_ = nullptr;

	CssSpatialGridRenderer grid_renderer;

	DensityHeatmap heatmap{ PPS_Settings::world_width, PPS_Settings::world_height,
							 SimulationSettings::screen_width, SimulationSettings::screen_height };

	std::vector<sf::Color> m_cached_colors_;
	bool                   m_colors_dirty_ = true;

	std::vector<float> radii_;

	DensityGridRenderer density_grid_renderer{};

public:
	PPS_Renderer(sf::RenderWindow* window, CSRSpatialGrid* spatial_grid);

	static float zoom_to_alpha(const float zoom,
		const float zoom_min, const float zoom_max,
		const float alpha_min, const float alpha_max);

	void render(const SimSnapshot& snapshot, Camera& camera);
	void render_heat_map(const SimSnapshot& snapshot, const Camera& camera, float alpha);
	void render_particles(const SimSnapshot& snapshot, const Camera& camera, float alpha);
	void check_rebuild_color_cashe(const std::vector<uint16_t>& neighbourhood_count);
	void render_debug(const SimSnapshot& snapshot, const sf::Vector2f mouse_pos, const float mouse_radius);

};