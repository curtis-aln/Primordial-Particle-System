#pragma once
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/VertexBuffer.hpp>
#include <SFML/System/Vector2.hpp>

// Forward declarations — avoids pulling in all of SFML/Graphics.hpp
namespace sf { class RenderWindow; }
struct SimpleSpatialGrid;

// ─── Settings ────────────────────────────────────────────────────────────────
namespace GridRendererSettings
{
    inline constexpr int        char_size = 12;
    inline constexpr sf::Color  grid_line_color = { 75, 75, 75 };
    inline constexpr sf::Color  text_color_active = sf::Color::White;
    inline constexpr sf::Color  text_color_dim = { 80, 80, 80 };
    inline constexpr const char font_path[] = "media/fonts/Calibri.ttf";
}
// ─────────────────────────────────────────────────────────────────────────────

class SpatialGridRenderer
{
public:
    SpatialGridRenderer(const SimpleSpatialGrid* grid) : m_grid(grid)
    {
        sync_dimensions();
        load_font();
        build_line_buffer();
    }

    // Call if the grid dimensions change at runtime.
    void rebuild();

    // Renders grid lines and, for cells within query_radius of query_pos,
    // annotates them with index/position/occupancy info.
    // Pass query_radius = 0 to annotate every cell.
    void render(sf::RenderWindow& window,
        sf::Vector2f      query_pos,
        float             query_radius = 0.f) const;

private:
    const SimpleSpatialGrid* m_grid;

    sf::VertexBuffer m_line_buffer;
    sf::Font         m_font;
    sf::Text         m_text{ m_font };

    // Cached grid dimensions — avoids pointer chasing every frame.
    sf::Vector2f m_cell_dim{};
    std::size_t  m_cells_x = 0;
    std::size_t  m_cells_y = 0;

    void sync_dimensions();
    void build_line_buffer();
    void load_font();

    // Returns a one-decimal-place string for a float without <format>.
    static std::string fmt_float(float v);
};