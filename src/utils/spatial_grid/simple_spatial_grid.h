#pragma once

#include <cstdint>
#include "fixed_span.h"
#include <Imgui.h>

using cell_idx = uint32_t;
using obj_idx = uint32_t; 

struct SimpleSpatialGrid
{
    uint32_t CellsX = 0;
    uint32_t CellsY = 0;
    uint32_t cell_max_capacity = 0;

    float cell_width = 0;
    float cell_height = 0;
    float world_width = 0;
    float world_height = 0;

    alignas(64) std::vector<obj_idx>  grid{};             // flat 1D: [cell * capacity + slot]
    alignas(64) std::vector<uint8_t>  cell_capacities{};  // one counter per cell

public:
    explicit SimpleSpatialGrid(uint32_t cells_x, uint32_t cells_y, uint32_t cell_capacity,
        float world_width, float world_height)
        : CellsX(cells_x), CellsY(cells_y), cell_max_capacity(cell_capacity)
        , world_width(world_width), world_height(world_height)
    {
        const uint32_t total = CellsX * CellsY;
        grid.resize(total * cell_max_capacity, 0);
        cell_capacities.resize(total, 0);
        update_cell_dimensions();
    }

    ~SimpleSpatialGrid() = default;

    void update_cell_dimensions()
    {
        cell_width = world_width / static_cast<float>(CellsX);
        cell_height = world_height / static_cast<float>(CellsY);
    }

    void change_cell_dimensions(uint32_t new_cells_x, uint32_t new_cells_y)
    {
        CellsX = new_cells_x;
        CellsY = new_cells_y;
        update_cell_dimensions();

        const uint32_t total = CellsX * CellsY;
        grid.assign(total * cell_max_capacity, 0);
        cell_capacities.assign(total, 0);
    }

    cell_idx inline hash(const float x, const float y) const
    {
        const auto cell_x = static_cast<cell_idx>(x / cell_width);
        const auto cell_y = static_cast<cell_idx>(y / cell_height);
        return cell_y * CellsX + cell_x;
    }

    cell_idx inline add_object(const float x, const float y, const size_t obj_id)
    {
		const float clamped_x = std::fmod(std::fmod(x, world_width) + world_width, world_width);
		const float clamped_y = std::fmod(std::fmod(y, world_height) + world_height, world_height);
        const cell_idx index = hash(clamped_x, clamped_y);
        uint8_t& cap = cell_capacities[index];

        if (cap < cell_max_capacity)
            grid[index * cell_max_capacity + cap++] = static_cast<obj_idx>(obj_id);

        return index;
    }

    void clear()
    {
        std::memset(cell_capacities.data(), 0, cell_capacities.size());
    }

    size_t get_total_cells() const { return CellsX * CellsY; }

    void find(const float x, const float y, FixedSpan<obj_idx>* container) const
    {
        find_from_index(hash(x, y), container);
    }

    void find_from_index(const cell_idx index, FixedSpan<obj_idx>* container) const
    {
        container->count = 0;

        const uint32_t cell_x = index % CellsX;
        const uint32_t cell_y = index / CellsX;

        for (uint32_t nx = cell_x - 1; nx <= cell_x + 1; ++nx)
        {
            if (nx >= CellsX) continue;
            for (uint32_t ny = cell_y - 1; ny <= cell_y + 1; ++ny)
            {
                if (ny >= CellsY) continue;

                const cell_idx  neighbour = ny * CellsX + nx;
                const uint8_t   count = cell_capacities[neighbour];
                const obj_idx* data = &grid[neighbour * cell_max_capacity];

                for (uint8_t i = 0; i < count; ++i)
                    container->add(data[i]);
            }
        }
    }

    // This converts a world-space rect into a clamped grid cell range with zero allocation
    void get_cell_range(float wx0, float wy0, float wx1, float wy1,
        uint32_t& cx0, uint32_t& cy0,
        uint32_t& cx1, uint32_t& cy1) const
    {
        cx0 = static_cast<uint32_t>(std::clamp(wx0 / cell_width, 0.f, static_cast<float>(CellsX - 1)));
        cy0 = static_cast<uint32_t>(std::clamp(wy0 / cell_height, 0.f, static_cast<float>(CellsY - 1)));
        cx1 = static_cast<uint32_t>(std::clamp(wx1 / cell_width, 0.f, static_cast<float>(CellsX - 1)));
        cy1 = static_cast<uint32_t>(std::clamp(wy1 / cell_height, 0.f, static_cast<float>(CellsY - 1)));
    }


    void track_stats(int& total, int& max_in, int& full, int& empty, int& inv)
    {
        for (size_t i = 0; i < get_total_cells(); ++i)
        {
            const int c = static_cast<int>(cell_capacities[i]);
            total += c;
            if (c == 0)                                    ++empty;
            if (c >= static_cast<int>(cell_max_capacity))  ++full;
            if (c > max_in)                                 max_in = c;
        }

        const auto tc = static_cast<float>(get_total_cells());
        inv = tc > 0.f ? 1.f / tc : 0.f;

        ImGui::Spacing();
        ImGui::Text("Objects  %d", total);
        ImGui::Text("Avg/cell %.2f", tc > 0 ? static_cast<float>(total) * inv : 0.f);
        ImGui::Text("Max cell %d  (%.0f%%)", max_in,
            cell_max_capacity > 0 ? max_in * 100.f / static_cast<float>(cell_max_capacity) : 0.f);
        ImGui::Text("Full     %d  (%.1f%%)", full, full * 100.f * inv);
        ImGui::Text("Empty    %d  (%.1f%%)", empty, empty * 100.f * inv);

        if (full > 0)
            ImGui::TextColored({ 1.f, 0.4f, 0.4f, 1.f },
                "[!] %d at cap — objects may drop", full);
    }
};