#pragma once
#include "../settings.h"
#include "../utils/utils.h"
#include "../utils/font.h"
#include "../utils/spatial_grid.h"
#include "../utils/Camera.hpp"

class PPS_Renderer : ColorSettings
{
private:
    sf::VertexArray circle_vertex_array{};  // Template for a single circle
    sf::VertexArray vertex_array_;          // Main vertex array to hold all the circles

    // Debug rendering
    sf::CircleShape visual_radius_shape_;
    sf::VertexArray debug_lines_;

    sf::RenderWindow& window_;
    Font debug_font_;

    // references
    std::vector<float>& positions_x_;
    std::vector<float>& positions_y_;
    std::vector<float>& angles_;
    std::vector<uint16_t>& neighbourhood_count_;

    size_t CIRCLE_SIDES = 16;  // Number of sides for the circle
    unsigned verticies_per_particle = CIRCLE_SIDES * 3;  // Triangles for each circle (center + two adjacent points)



public:
    PPS_Renderer(sf::RenderWindow& window,
        std::vector<float>& positions_x, std::vector<float>& positions_y,
        std::vector<float>& angles,
        std::vector<uint16_t>& neighbourhood_count)
        : window_(window), debug_font_(&window, FontSettings::font_size_debug, FontSettings::font_path),
        positions_x_(positions_x), positions_y_(positions_y), angles_(angles),
        neighbourhood_count_(neighbourhood_count)
    {
        std::cout << "Renderer Initialized\n";
    }

    void init_vertex_array()
    {
        // Initialize a single circle template
        circle_vertex_array.setPrimitiveType(sf::Triangles);
        circle_vertex_array.resize(CIRCLE_SIDES * 3);  // Each circle will be made of CIRCLE_SIDES triangles

        const float radius = PPS_Settings::particle_radius;
        const sf::Color default_color = sf::Color::White;  // Default color for initialization

        // Set up the center vertex
        for (unsigned i = 0; i < CIRCLE_SIDES; ++i)
        {
            circle_vertex_array[i * 3 + 0].position = { 0.f, 0.f };  // center of the circle
            circle_vertex_array[i * 3 + 0].color = default_color;

            // Generate surrounding vertices for the circle
            const float angle = static_cast<float>(i) * 2.f * 3.14159265f / CIRCLE_SIDES;
            const sf::Vector2f point = { std::cos(angle) * radius, std::sin(angle) * radius };

            circle_vertex_array[i * 3 + 1].position = point;  // vertex 1
            circle_vertex_array[i * 3 + 1].color = default_color;

            // Next point for the triangle
            const float next_angle = static_cast<float>((i + 1) % CIRCLE_SIDES) * 2.f * 3.14159265f / CIRCLE_SIDES;
            const sf::Vector2f next_point = { std::cos(next_angle) * radius, std::sin(next_angle) * radius };

            circle_vertex_array[i * 3 + 2].position = next_point;  // vertex 2
            circle_vertex_array[i * 3 + 2].color = default_color;
        }

        // The vertex_array_ will hold all the translated circles
        size_t total_vertices = positions_x_.size() * verticies_per_particle;
        vertex_array_.setPrimitiveType(sf::Triangles);
        vertex_array_.resize(total_vertices);
    }

    void render_particles()
    {
        // For each particle, update the vertex array in place
        for (size_t i = 0; i < positions_x_.size(); ++i)
        {
            add_circle_to_vertex_array(i);  // Update the translated circle for this particle
        }

        // Draw all circles at once
        window_.draw(vertex_array_);
    }

    void add_circle_to_vertex_array(size_t index)
    {
        const sf::Vector2f center = { positions_x_[index], positions_y_[index] };  // Particle position
        const sf::Color color = get_color(neighbourhood_count_[index]);  // Particle color based on nearby count

        // Compute the base index for the current particle's vertices in the vertex array
        size_t base_index = index * verticies_per_particle;

        // Add the circle's vertices to the main vertex array with the translation
        for (unsigned i = 0; i < CIRCLE_SIDES; ++i)
        {
            // Translate each vertex to the particle's position and set its color
            sf::Vertex v1 = circle_vertex_array[i * 3 + 0];
            v1.position += center;
            v1.color = color;
            vertex_array_[base_index + i * 3 + 0] = v1;  // Set vertex 1

            sf::Vertex v2 = circle_vertex_array[i * 3 + 1];
            v2.position += center;
            v2.color = color;
            vertex_array_[base_index + i * 3 + 1] = v2;  // Set vertex 2

            sf::Vertex v3 = circle_vertex_array[i * 3 + 2];
            v3.position += center;
            v3.color = color;
            vertex_array_[base_index + i * 3 + 2] = v3;  // Set vertex 3
        }
    }

    // turning a nearby count into a color which is smoothed
    sf::Color interpolate(const sf::Color& a, const sf::Color& b, float factor)
    {
        return sf::Color(
            static_cast<sf::Uint8>(a.r + factor * (b.r - a.r)),
            static_cast<sf::Uint8>(a.g + factor * (b.g - a.g)),
            static_cast<sf::Uint8>(a.b + factor * (b.b - a.b))
        );
    }

    sf::Color get_color(const float nearby_count)
    {
        if (nearby_count <= 0.0f) return first_color;
        if (nearby_count <= range1) return interpolate(first_color, second_color, nearby_count / range1);
        if (nearby_count <= range2) return interpolate(second_color, third_color, (nearby_count - range1) / (range2 - range1));
        if (nearby_count < range3) return interpolate(third_color, fourth_color, (nearby_count - range2) / (range3 - range2));
        return fourth_color;
    }
};
