#include "spatial_grid_renderer.h"
#include "simple_spatial_grid.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <iostream>

// ─────────────────────────────────────────────────────────────────────────────
//  Construction
// ─────────────────────────────────────────────────────────────────────────────


void SpatialGridRenderer::rebuild()
{
    sync_dimensions();
    build_line_buffer();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Render
// ─────────────────────────────────────────────────────────────────────────────

void SpatialGridRenderer::render(sf::RenderWindow& window,
    sf::Vector2f      query_pos,
    float             query_radius) const
{
    // Draw the grid lines first (single GPU draw call via VertexBuffer).
    window.draw(m_line_buffer);

    const float radius_sq = query_radius * query_radius;

    for (std::size_t x = 0; x < m_cells_x; ++x)
    {
        for (std::size_t y = 0; y < m_cells_y; ++y)
        {
            const sf::Vector2f top_left = { x * m_cell_dim.x, y * m_cell_dim.y };
            const sf::Vector2f centre = top_left + m_cell_dim * 0.5f;

            // Skip cells outside the query radius.
            if (query_radius > 0.f)
            {
                const float dx = centre.x - query_pos.x;
                const float dy = centre.y - query_pos.y;
                if (dx * dx + dy * dy > radius_sq)
                    continue;
            }

            const auto idx_1d = static_cast<cell_idx>(y * m_cells_x + x);
            const int  obj_count = static_cast<int>(m_grid->cell_capacities[idx_1d]);

            const std::string info =
                "1D: " + std::to_string(idx_1d) + "\n"
                "2D: (" + std::to_string(x) + ", " + std::to_string(y) + ")\n"
                "TL: (" + fmt_float(top_left.x) + ", " + fmt_float(top_left.y) + ")\n"
                "objs: " + std::to_string(obj_count);

            // const_cast is safe here — Text::setString/setFillColor/setPosition
            // only modify the Text object's own state, not the grid.
            auto& text = const_cast<sf::Text&>(m_text);
            text.setFillColor(obj_count > 0
                ? GridRendererSettings::text_color_active
                : GridRendererSettings::text_color_dim);
            text.setString(info);
            text.setPosition(top_left + sf::Vector2f{ 4.f, 4.f });
            window.draw(text);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void SpatialGridRenderer::sync_dimensions()
{
    m_cells_x = m_grid->CellsX;
    m_cells_y = m_grid->CellsY;
    m_cell_dim = { m_grid->cell_width, m_grid->cell_height };
}

void SpatialGridRenderer::build_line_buffer()
{
    // One line per column boundary + one per row boundary, two vertices each.
    const std::size_t vertex_count = (m_cells_x + m_cells_y) * 2;
    std::vector<sf::Vertex> vertices(vertex_count);

    const float world_w = m_grid->world_width;
    const float world_h = m_grid->world_height;

    std::size_t vi = 0;

    for (std::size_t x = 0; x < m_cells_x; ++x)
    {
        const float px = static_cast<float>(x) * m_cell_dim.x;
        vertices[vi].position = { px, 0.f };
        vertices[vi + 1].position = { px, world_h };
        vi += 2;
    }

    for (std::size_t y = 0; y < m_cells_y; ++y)
    {
        const float py = static_cast<float>(y) * m_cell_dim.y;
        vertices[vi].position = { 0.f,    py };
        vertices[vi + 1].position = { world_w, py };
        vi += 2;
    }

    for (std::size_t i = 0; i < vi; ++i)
        vertices[i].color = GridRendererSettings::grid_line_color;

    m_line_buffer = sf::VertexBuffer(sf::PrimitiveType::Lines,
        sf::VertexBuffer::Usage::Static);

    m_line_buffer.create(vertex_count);
    m_line_buffer.update(vertices.data(), vertex_count, 0);
}

void SpatialGridRenderer::load_font()
{
    m_text.setCharacterSize(GridRendererSettings::char_size);

    if (!m_font.openFromFile(GridRendererSettings::font_path))
    {
        std::cerr << "[SpatialGridRenderer] Failed to load font from: "
            << GridRendererSettings::font_path << '\n';
    }
}

std::string SpatialGridRenderer::fmt_float(float v)
{
    const int whole = static_cast<int>(v);
    const int frac = static_cast<int>(std::abs(v - static_cast<float>(whole)) * 10.f);
    return std::to_string(whole) + "." + std::to_string(frac);
}