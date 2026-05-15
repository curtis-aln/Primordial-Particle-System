#include "PPS_renderer.h"


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




PPS_Renderer::PPS_Renderer(sf::RenderWindow* window) : window_(window)
{
	constexpr int vertex_count = PPS_Settings::particle_count * 6;
	vertex_array.resize(vertex_count);

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

float PPS_Renderer::zoom_to_alpha(const float zoom,
	const float zoom_min, const float zoom_max,
	const float alpha_min, const float alpha_max)
{
	const float t = std::clamp((zoom - zoom_min) / (zoom_max - zoom_min), 0.f, 1.f);
	return alpha_min + t * (alpha_max - alpha_min);
}

void PPS_Renderer::render(const SimSnapshot& snapshot, Camera& camera, bool rend_map, bool rend_particles)
{
	// the heat map is great for rendering millions of particles from afar but it has a lot of visual noise up close
	// the particles are more expensive to render but they look better up close
	// the solution is to discretely transition from rendering the heat map to rendering the particles

	// when the visible world width surpasses a certain number of particle radiuii we begin transition
	constexpr float transition_thresh_begin = 800.f * PPS_Settings::particle_radius; // begin transition
	constexpr float transition_thresh_end = 1400.f * PPS_Settings::particle_radius; // end transition
	constexpr float diff = transition_thresh_end - transition_thresh_begin;

	const float left = camera.mapPixelToCoords({ 0, 0 }).x;
	const float right = camera.mapPixelToCoords({ SimulationSettings::screen_width, 0 }).x;

	const float visible_world_width = right - left;
	const float alpha_heat_map = std::clamp((visible_world_width - transition_thresh_begin) / diff * 255.f, 0.f, 255.f);
	const float alpha_particles = 255 - alpha_heat_map;

	rend_particles = visible_world_width < transition_thresh_end;
	rend_map = visible_world_width > transition_thresh_begin;

	if (rend_map)
		render_heat_map(snapshot, camera, alpha_heat_map);
	if (rend_particles)
		render_particles(snapshot, camera, alpha_particles);
}

void PPS_Renderer::render_heat_map(const SimSnapshot& snapshot, const Camera& camera, const float alpha)
{
	heatmap.clear();
	heatmap.scatter(snapshot.render.positions_x, snapshot.render.positions_y, PPS_Settings::particle_count, camera.m_view_);
	heatmap.upload();          
	heatmap.draw(*window_, static_cast<uint8_t>(alpha));
}

void PPS_Renderer::render_particles(const SimSnapshot& snapshot, const Camera& camera, const float alpha)
{
	// Fetching the data generated by the update loop
	auto& positions_x = snapshot.render.positions_x;
	auto& positions_y = snapshot.render.positions_y;
	auto& neighbourhood_count = snapshot.render.neighbourhood_count_;


	// when we reassign the vertex array values we want to preserve the texture coordinates
	const auto tex_size = static_cast<float>(texture.getSize().x);
	const float u1 = tex_size, v1 = tex_size;

	// The renderer needs to figure out what needs to be drawn and what doesnt need to be drawn, if any positions
	// are outside the viewing area we dont have to draw them
	const sf::Vector2f top_left = camera.mapPixelToCoords({ 0, 0 });
	const sf::Vector2f bottom_right = camera.mapPixelToCoords({ SimulationSettings::screen_width, SimulationSettings::screen_height });

	vertex_array.clear();

	for (size_t i = 0; i < PPS_Settings::particle_count; ++i)
	{
		// Fetching particle information
		const size_t base = i * 6;
		const float pos_x = positions_x[i];
		const float pos_y = positions_y[i];
		constexpr float r = PPS_Settings::particle_radius;

		if (pos_x < top_left.x || pos_y < top_left.y || pos_x > bottom_right.x || pos_y > bottom_right.y)
			continue;

		// The color of the particle is determined by its neighbourhood density
		sf::Color col = get_color(neighbourhood_count[i]);
		col.a = alpha;

		// First Triangle
		
		vertex_array.append({ .position = { pos_x - r, pos_y - r }, .color = col, .texCoords = {0.f, 0.f} });
		vertex_array.append({ .position = { pos_x + r, pos_y - r }, .color = col, .texCoords = {u1, 0.f} });
		vertex_array.append({ .position = { pos_x + r, pos_y + r }, .color = col, .texCoords = {u1, v1} });
		
		// Second Triangle
		vertex_array.append({ .position = { pos_x - r, pos_y - r }, .color = col, .texCoords = {0.f, 0.f} });
		vertex_array.append({ .position = { pos_x + r, pos_y + r }, .color = col, .texCoords = {u1, v1} });
		vertex_array.append({ .position = { pos_x - r, pos_y + r }, .color = col, .texCoords = {0.f, v1} });
	}

	window_->draw(vertex_array, states);
}

void PPS_Renderer::render_debug(const SimSnapshot& snapshot, const sf::Vector2f mouse_pos, const float mouse_radius)
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
	}

	window_->draw(debug_lines_);
}

