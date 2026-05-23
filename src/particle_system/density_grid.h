// density_grid.h
// ─────────────────────────────────────────────────────────────────────────────
// Replaces the O(n·k) spatial-hash neighbour loop with a precomputed density
// field + gradient that every particle samples in O(1).
//
// USAGE — drop into particle_system.cpp  (replaces solveCollisions + process_cell):
//
//   // Once per frame, before update_particle_positions():
//   density_grid.build(render_data.positions_x.data(),
//                      render_data.positions_y.data(),
//                      particle_count);
//
//   // Then dispatch per-particle update with density sampling instead of
//   // the neighbour loop — see update_particle_density() below.
//
// INTEGRATION NOTES:
//   • Cell size `s` defaults to 1.0 (one grid cell per world unit).
//     Set s = visual_radius / 2.f for better accuracy at the cost of
//     a larger convolution kernel.
//   • The grid dimensions are fixed at construction to
//     ceil(world_w / s) × ceil(world_h / s).
//   • All grids (D, Gx, Gy) are kept in a flat float[] — same layout as
//     heatmap.h's m_counts so the memory access pattern is identical.
//   • Wrapping (toroidal world) is handled during binning and gradient
//     computation, matching particle_system.cpp's wrap logic.
// ─────────────────────────────────────────────────────────────────────────────

#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>

#include "../settings.h"   // PPS_Settings — visual_radius, alpha, beta, gamma, world_w/h

class DensityGrid
{
public:
    // ── Construction ──────────────────────────────────────────────────────────

  
    explicit DensityGrid(float world_w, float world_h, float cell_size);
    // ── Main API ──────────────────────────────────────────────────────────────

    void build(const float* px, const float* py, int n);

    float sample_density(float wx, float wy) const;
    void sample_gradient(float wx, float wy, float& out_gx, float& out_gy) const;

    void update_particle_density(
        float& angle,
        float& x, float& y,
        uint16_t& neighbourhood_count_out,
        const float* sin_table, const float* cos_table,
        int angle_table_size) const;

    // ── Accessors (useful for debug / visualisation) ──────────────────────────

    int   width()     const { return m_W; }
    int   height()    const { return m_H; }
    float cell_size() const { return m_s; }

    // Direct read access to the raw grids (row-major, row = y).
    const std::vector<float>& density()    const { return m_D; }
    const std::vector<float>& gradient_x() const { return m_Gx; }
    const std::vector<float>& gradient_y() const { return m_Gy; }

private:
    void bin(const float* px, const float* py, int n);

    void smooth_box();
    void smooth_gaussian();

    void gradient();
    

    // ── Bilinear sampler ──────────────────────────────────────────────────────
    float bilerp(const std::vector<float>& grid, float wx, float wy) const;

    float sample8(const std::vector<float>& grid, float wx, float wy) const;

    float cubicInterpolate(float p0, float p1, float p2, float p3, float t) const;

    float bicubic(const std::vector<float>& grid, float wx, float wy) const;

    float sampleRadius(
        const std::vector<float>& grid,
        float wx,
        float wy,
        float radiusWorld
    ) const;

    // ── Members ───────────────────────────────────────────────────────────────

    float m_world_w, m_world_h;
    float m_s;          // cell size in world units
    float m_inv_s;      // 1 / m_s
    int   m_W, m_H;     // grid dimensions
    int   m_box_half;   // box-filter half-width in cells

    std::vector<float> m_D;    // density field  D[y*W+x]
    std::vector<float> m_Gx;   // x-gradient     ∂D/∂x
    std::vector<float> m_Gy;   // y-gradient     ∂D/∂y
    std::vector<float> m_tmp;  // scratch for separable passes
};
