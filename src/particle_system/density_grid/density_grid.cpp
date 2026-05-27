#include "density_grid.h"


// cell_size: world units per grid cell.
//   s = 1.0          — natural, grid == world integer coords
//   s = visual_radius/2  — better accuracy, 3×3 block covers sensing disc
//   s = visual_radius/4  — highest accuracy, 5×5 block, more convolution cost
DensityGrid::DensityGrid(float world_w, float world_h, float cell_size) : m_world_w(world_w) , m_world_h(world_h)
{
    set_cell_size(cell_size);
}

void DensityGrid::set_cell_size(float cell_size)
{
    m_s = cell_size;
    m_inv_s = 1.f / cell_size;
    m_W = static_cast<int>(std::ceil(m_world_w / cell_size));
    m_H = static_cast<int>(std::ceil(m_world_h / cell_size));

    const size_t n = static_cast<size_t>(m_W) * m_H;
    m_D.resize(n, 0.f);
    m_Gx.resize(n, 0.f);
    m_Gy.resize(n, 0.f);
    m_tmp.resize(n, 0.f);   // scratch for separable box passes

    // Precompute the half-width (in cells) of the sensing disc for the
    // box-filter approximation.  We use radius r = visual_radius.
    m_box_half = static_cast<int>(std::ceil(PPS_Settings::visual_radius * m_inv_s));
}

// ── Main API ──────────────────────────────────────────────────────────────

// build() — call once per simulation tick, before the particle update loop.
// Performs binning → smoothing → gradient in O(n + W·H).
void DensityGrid::build(const float* positions_x, const float* positions_y, int particle_count)
{
    // Bottleneck
    bin(positions_x, positions_y, particle_count);

    if (box_smoothing_)
        smooth_box();

    if (gaussian_smoothing_)
    {
        smooth_gaussian();

        // Rescale: normalized Gaussian gives density per cell.
        // Multiply by disc area in grid units to recover particle counts.
        const float r_grid = PPS_Settings::visual_radius * m_inv_s;
        const float disc_area = 3.14159f * r_grid * r_grid;
        for (float& v : m_D) v *= disc_area;
    }

    gradient();
}

// Sample the density field at world position (wx, wy) — bilinear.
// Returns approximate neighbour count within visual_radius.
float DensityGrid::sample_density(float wx, float wy) const
{
    return bilerp(m_D, wx, wy);
}

// Sample the density gradient at world position (wx, wy) — bilinear.
void DensityGrid::sample_gradient(float wx, float wy, float& out_gx, float& out_gy) const
{
    out_gx = bilerp(m_Gx, wx, wy);
    out_gy = bilerp(m_Gy, wx, wy);
}

// ── Per-particle update helper ────────────────────────────────────────────
//
// Drop-in replacement for the neighbour-loop body inside update_particle().
// Call after build().  Reads D/Gx/Gy, computes sign(R-L) from the gradient
// perpendicular to the heading, and advances angle + position.
//
// Parameters mirror particle_system.cpp's update_particle() signature so
// the call site can be swapped with minimal changes.
//
//   angle  — current heading in radians (modified in-place)
//   x, y   — world position (modified in-place by gamma step)
//   neighbourhood_count_out — filled with the rounded neighbour estimate
//
// Math:
//   heading = (cos_a, sin_a)
//   heading_perp (rightward normal) = (-sin_a, cos_a)
//   perp = ∇D · heading_perp  →  positive = more density on the right
//   Δφ = alpha + beta * N * sign(perp)          (same formula as original)
//
void DensityGrid::update_particle_density(
    float& angle,
    float& x, float& y,
    uint16_t& neighbourhood_count_out,
    const float* sin_table, const float* cos_table,
    int angle_table_size) const
{
    static constexpr float two_pi = 2.f * 3.14159265358979323846f;
    static constexpr float pi_div_180 = 3.14159265358979323846f / 180.f;

    // ── Angle → trig via LUT (identical to particle_system.cpp) ──────────
    const int angle_index = static_cast<int>((angle / two_pi) * angle_table_size)
        & (angle_table_size - 1);
    const float cos_a = cos_table[angle_index];
    const float sin_a = sin_table[angle_index];

    // ── Sample density field ───────────────────────────────────
    auto sampleField = [&](const std::vector<float>& field) -> float {
        switch (sampling_mode_) {
        case SamplingMode::Bilinear: return bilerp(field, x, y);
        case SamplingMode::Bicubic:  return bicubic(field, x, y);
        case SamplingMode::Sample8:  return sample8(field, x, y);
        }
        };

    const float N = sampleField(m_D);
    const float gx = sampleField(m_Gx);
    const float gy = sampleField(m_Gy);

    // the + 0.5 is a manual round-to-nearest trick
    neighbourhood_count_out = static_cast<uint16_t>(static_cast<int>(N + 0.5f));

    // ── sign(R - L) via perpendicular gradient component ──────────────────
    // heading_perp = (-sin_a, cos_a)  — the rightward normal
    const float perp = gx * (sin_a * sin_sign)+gy * (cos_a * cos_sign);

    // sign convention matches particle_system.cpp:
    //   sign = +1 when right >= left,  -1 when left > right
    const float sign = (perp >= 0.f) ? 1.f : -1.f;

    // ── Angle update (identical formula to update_particle()) ─────────────
    angle += (PPS_Settings::alpha + PPS_Settings::beta * N * sign) * pi_div_180;

    // Keep angle in [0, 2π)
    angle = std::fmod(angle, two_pi);
    angle += two_pi * (angle < 0.f);

    // ── Position step ─────────────────────────────────────────────────────
    x += PPS_Settings::gamma * cos_a;
    y += PPS_Settings::gamma * sin_a;
}


// ── Step 1 — Binning ──────────────────────────────────────────────────────
// Each particle votes its count into one cell via bilinear splat (same
// pattern as DensityHeatmap::scatter in heatmap.h).  Bilinear splatting
// gives smoother counts than nearest-cell and eliminates hard cell-boundary
// artefacts for free.
void DensityGrid::bin(const float* px, const float* py, int particle_count)
{
    std::fill(m_D.begin(), m_D.end(), 0.f);

    const int W = m_W, H = m_H;

    for (int i = 0; i < particle_count; ++i)
    {
        // Continuous cell-space position
        const float fx = px[i] * m_inv_s - 0.5f;
        const float fy = py[i] * m_inv_s - 0.5f;

        const int x0 = static_cast<int>(std::floor(fx));
        const int y0 = static_cast<int>(std::floor(fy));

        const float sx = fx - static_cast<float>(x0);
        const float sy = fy - static_cast<float>(y0);

        // Splat to 4 neighbours with toroidal wrap
        auto splat = [&](int cx, int cy, float w) {
            // Toroidal wrap — mirrors particle_system.cpp's wrap logic
            cx = ((cx % W) + W) % W;
            cy = ((cy % H) + H) % H;
            m_D[cy * W + cx] += w;
            };

        splat(x0, y0, (1.f - sx) * (1.f - sy));
        splat(x0 + 1, y0, sx * (1.f - sy));
        splat(x0, y0 + 1, (1.f - sx) * sy);
        splat(x0 + 1, y0 + 1, sx * sy);
    }
}


// ── Step 3 — Central-difference gradient ──────────────────────────────────
// ∂D/∂x and ∂D/∂y via central differences.  Toroidal wrap at all edges so
// particles near the world boundary see a consistent gradient (no edge
// artefacts, matching the toroidal simulation topology).
void DensityGrid::gradient()
{
    const int W = m_W, H = m_H;

    for (int y = 0; y < H; ++y)
    {
        const int yp = (y + 1) % H;
        const int yn = (y - 1 + H) % H;

        for (int x = 0; x < W; ++x)
        {
            const int xp = (x + 1) % W;
            const int xn = (x - 1 + W) % W;

            m_Gx[y * W + x] = (m_D[y * W + xp] - m_D[y * W + xn]) * 0.5f;
            m_Gy[y * W + x] = (m_D[yp * W + x] - m_D[yn * W + x]) * 0.5f;
        }
    }
}