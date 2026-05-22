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

    // cell_size: world units per grid cell.
    //   s = 1.0          — natural, grid == world integer coords
    //   s = visual_radius/2  — better accuracy, 3×3 block covers sensing disc
    //   s = visual_radius/4  — highest accuracy, 5×5 block, more convolution cost
    explicit DensityGrid(float world_w, float world_h, float cell_size)
        : m_world_w(world_w)
        , m_world_h(world_h)
        , m_s(cell_size)
        , m_inv_s(1.f / cell_size)
        , m_W(static_cast<int>(std::ceil(world_w / cell_size)))
        , m_H(static_cast<int>(std::ceil(world_h / cell_size)))
    {
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
    void build(const float* px, const float* py, int n)
    {
        bin(px, py, n);
        smooth_gaussian();

        // Rescale: normalized Gaussian gives density per cell.
        // Multiply by disc area in grid units to recover particle counts.
        const float r_grid = PPS_Settings::visual_radius * m_inv_s;
        const float disc_area = 3.14159f * r_grid * r_grid;
        for (float& v : m_D) v *= disc_area;

        gradient();
    }

    // Sample the density field at world position (wx, wy) — bilinear.
    // Returns approximate neighbour count within visual_radius.
    float sample_density(float wx, float wy) const
    {
        return bilerp(m_D, wx, wy);
    }

    // Sample the density gradient at world position (wx, wy) — bilinear.
    void sample_gradient(float wx, float wy, float& out_gx, float& out_gy) const
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
    inline void update_particle_density(
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

        // ── Sample density field (bilinear) ───────────────────────────────────
        const float N = bicubic(m_D, x, y);
        const float gx = bicubic(m_Gx, x, y);
        const float gy = bicubic(m_Gy, x, y);

        neighbourhood_count_out = static_cast<uint16_t>(static_cast<int>(N + 0.5f));

        // ── sign(R - L) via perpendicular gradient component ──────────────────
        // heading_perp = (-sin_a, cos_a)  — the rightward normal
        const float perp = gx * (sin_a) + gy * (-cos_a);

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

    // ── Accessors (useful for debug / visualisation) ──────────────────────────

    int   width()     const { return m_W; }
    int   height()    const { return m_H; }
    float cell_size() const { return m_s; }

    // Direct read access to the raw grids (row-major, row = y).
    const std::vector<float>& density()    const { return m_D; }
    const std::vector<float>& gradient_x() const { return m_Gx; }
    const std::vector<float>& gradient_y() const { return m_Gy; }

private:
    // ── Step 1 — Binning ──────────────────────────────────────────────────────
    // Each particle votes its count into one cell via bilinear splat (same
    // pattern as DensityHeatmap::scatter in heatmap.h).  Bilinear splatting
    // gives smoother counts than nearest-cell and eliminates hard cell-boundary
    // artefacts for free.
    void bin(const float* px, const float* py, int n)
    {
        std::fill(m_D.begin(), m_D.end(), 0.f);

        const int W = m_W, H = m_H;

        for (int i = 0; i < n; ++i)
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

    // ── Step 2a — Separable box-filter smoothing ──────────────────────────────
    // Two passes of a sliding-sum box filter (horizontal then vertical) with
    // half-width m_box_half.  Three cascade iterations approximate a Gaussian.
    // This is O(W·H) regardless of kernel size — one increment + one decrement
    // per cell per pass.
    //
    // The box half-width is set so that the full kernel diameter covers the
    // sensing disc: box_half = ceil(visual_radius / s).
    // After smoothing, D[y][x] ≈ number of particles within visual_radius of
    // cell (x, y).
    void smooth_box()
    {
        const int W = m_W, H = m_H;
        const int bh = m_box_half;

        // Three cascade passes in each dimension approximate a Gaussian well
        // enough for PPS purposes (~5 % error on neighbour count vs exact disc).
        for (int pass = 0; pass < 3; ++pass)
        {
            // Horizontal pass  (operates on m_D, writes to m_tmp)
            for (int y = 0; y < H; ++y)
            {
                float* row_D = m_D.data() + y * W;
                float* row_tmp = m_tmp.data() + y * W;

                // Initialise sliding sum for x = 0
                float sum = 0.f;
                for (int k = -bh; k <= bh; ++k)
                    sum += row_D[((k % W) + W) % W];

                for (int x = 0; x < W; ++x)
                {
                    row_tmp[x] = sum;
                    // Slide: remove leftmost sample, add next rightmost (toroidal)
                    const int remove_x = ((x - bh) % W + W) % W;
                    const int add_x = ((x + bh + 1) % W + W) % W;
                    sum -= row_D[remove_x];
                    sum += row_D[add_x];
                }
            }

            // Vertical pass  (operates on m_tmp, writes back to m_D)
            for (int x = 0; x < W; ++x)
            {
                // Initialise sliding sum for y = 0
                float sum = 0.f;
                for (int k = -bh; k <= bh; ++k)
                    sum += m_tmp[(((k % H) + H) % H) * W + x];

                for (int y = 0; y < H; ++y)
                {
                    m_D[y * W + x] = sum;
                    const int remove_y = (((y - bh) % H) + H) % H;
                    const int add_y = (((y + bh + 1) % H) + H) % H;
                    sum -= m_tmp[remove_y * W + x];
                    sum += m_tmp[add_y * W + x];
                }
            }
        }

        // Normalise so that a uniform density field returns the expected
        // neighbour count (the box filter inflates counts by (2·bh+1)^2 × 3
        // cascade passes accumulate area).  We divide by the kernel area once.
        const float kernel_area = static_cast<float>((2 * bh + 1) * (2 * bh + 1));
        const float norm = 1.f / (kernel_area * kernel_area * kernel_area); // 3 cascade passes
        // NOTE: this normalisation brings D back to units of "particles per
        // visual_radius disc area".  If you want raw counts, omit this step.
        for (float& v : m_D) v *= norm;
    }

    // ── Step 2b — Gaussian smoothing (alternative to smooth_box) ─────────────
    // Separable 1-D Gaussian, σ ≈ visual_radius / 2.  More accurate, ~5× more
    // multiply-adds than the box filter but still O(W·H·kernel_w).
    // Swap smooth_box() for smooth_gaussian() in build() when training neural
    // models that need a smooth, differentiable density signal.
    void smooth_gaussian()
    {
        const int W = m_W, H = m_H;

        const float sigma = PPS_Settings::visual_radius * m_inv_s * 0.5f;
        const int   krad = static_cast<int>(std::ceil(2.5f * sigma));

        // Build 1-D kernel
        std::vector<float> kern(2 * krad + 1);
        float kern_sum = 0.f;
        for (int k = -krad; k <= krad; ++k)
        {
            const float v = std::exp(-0.5f * (k * k) / (sigma * sigma));
            kern[k + krad] = v;
            kern_sum += v;
        }
        for (float& v : kern) v /= kern_sum;   // normalise

        // Horizontal pass
        for (int y = 0; y < H; ++y)
        {
            const float* src = m_D.data() + y * W;
            float* dst = m_tmp.data() + y * W;

            for (int x = 0; x < W; ++x)
            {
                float acc = 0.f;
                for (int k = -krad; k <= krad; ++k)
                    acc += src[(((x + k) % W) + W) % W] * kern[k + krad];
                dst[x] = acc;
            }
        }

        // Vertical pass
        for (int x = 0; x < W; ++x)
        {
            for (int y = 0; y < H; ++y)
            {
                float acc = 0.f;
                for (int k = -krad; k <= krad; ++k)
                    acc += m_tmp[(((y + k) % H) + H) % H * W + x] * kern[k + krad];
                m_D[y * W + x] = acc;
            }
        }
    }

    // ── Step 3 — Central-difference gradient ──────────────────────────────────
    // ∂D/∂x and ∂D/∂y via central differences.  Toroidal wrap at all edges so
    // particles near the world boundary see a consistent gradient (no edge
    // artefacts, matching the toroidal simulation topology).
    void gradient()
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

    // ── Bilinear sampler ──────────────────────────────────────────────────────
    // Samples any of the three flat grids at world position (wx, wy).
    // Matches the bilinear pattern in heatmap.h's scatter().
    float bilerp(const std::vector<float>& grid, float wx, float wy) const
    {
        const int W = m_W, H = m_H;

        const float fx = wx * m_inv_s - 0.5f;
        const float fy = wy * m_inv_s - 0.5f;

        const int x0 = static_cast<int>(std::floor(fx));
        const int y0 = static_cast<int>(std::floor(fy));

        const float tx = fx - static_cast<float>(x0);
        const float ty = fy - static_cast<float>(y0);

        // Toroidal wrap on all four sample points
        const int x1 = (x0 + 1);
        const int y1 = (y0 + 1);

        auto idx = [&](int cx, int cy) -> int {
            cx = ((cx % W) + W) % W;
            cy = ((cy % H) + H) % H;
            return cy * W + cx;
            };

        const float v00 = grid[idx(x0, y0)];
        const float v10 = grid[idx(x1, y0)];
        const float v01 = grid[idx(x0, y1)];
        const float v11 = grid[idx(x1, y1)];

        return v00 * (1.f - tx) * (1.f - ty)
            + v10 * tx * (1.f - ty)
            + v01 * (1.f - tx) * ty
            + v11 * tx * ty;
    }

    float sample8(const std::vector<float>& grid, float wx, float wy) const
    {
        const int W = m_W, H = m_H;

        const float fx = wx * m_inv_s - 0.5f;
        const float fy = wy * m_inv_s - 0.5f;

        const int x = static_cast<int>(std::floor(fx));
        const int y = static_cast<int>(std::floor(fy));

        auto idx = [&](int cx, int cy) -> int {
            cx = ((cx % W) + W) % W;
            cy = ((cy % H) + H) % H;
            return cy * W + cx;
            };

        float sum = 0.0f;
        float weightSum = 0.0f;

        for (int oy = -1; oy <= 1; oy++)
        {
            for (int ox = -1; ox <= 1; ox++)
            {
                if (ox == 0 && oy == 0)
                    continue;

                const float dx = fx - (x + ox);
                const float dy = fy - (y + oy);

                const float dist2 = dx * dx + dy * dy;

                // inverse distance weighting
                const float w = 1.0f / (dist2 + 0.0001f);

                sum += grid[idx(x + ox, y + oy)] * w;
                weightSum += w;
            }
        }

        return sum / weightSum;
    }

    float cubicInterpolate(float p0, float p1, float p2, float p3, float t) const
    {
        const float a0 = -0.5f * p0 + 1.5f * p1 - 1.5f * p2 + 0.5f * p3;
        const float a1 = p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3;
        const float a2 = -0.5f * p0 + 0.5f * p2;
        const float a3 = p1;

        return ((a0 * t + a1) * t + a2) * t + a3;
    }

    float bicubic(const std::vector<float>& grid, float wx, float wy) const
    {
        const int W = m_W;
        const int H = m_H;

        // convert world -> grid coordinates
        const float fx = wx * m_inv_s - 0.5f;
        const float fy = wy * m_inv_s - 0.5f;

        const int x = static_cast<int>(std::floor(fx));
        const int y = static_cast<int>(std::floor(fy));

        const float tx = fx - static_cast<float>(x);
        const float ty = fy - static_cast<float>(y);

        auto wrap = [](int v, int max) -> int
            {
                return ((v % max) + max) % max;
            };

        auto sample = [&](int sx, int sy) -> float
            {
                sx = wrap(sx, W);
                sy = wrap(sy, H);
                return grid[sy * W + sx];
            };

        float col[4];

        // interpolate along x for 4 rows
        for (int j = -1; j <= 2; ++j)
        {
            const float p0 = sample(x - 1, y + j);
            const float p1 = sample(x + 0, y + j);
            const float p2 = sample(x + 1, y + j);
            const float p3 = sample(x + 2, y + j);

            col[j + 1] = cubicInterpolate(p0, p1, p2, p3, tx);
        }

        // interpolate those results along y
        return cubicInterpolate(
            col[0],
            col[1],
            col[2],
            col[3],
            ty
        );
    }

    float sampleRadius(
        const std::vector<float>& grid,
        float wx,
        float wy,
        float radiusWorld
    ) const
    {
        const int W = m_W;
        const int H = m_H;

        // world -> grid space
        const float gx = wx * m_inv_s - 0.5f;
        const float gy = wy * m_inv_s - 0.5f;

        const int cx = static_cast<int>(std::floor(gx));
        const int cy = static_cast<int>(std::floor(gy));

        const float radiusGrid = radiusWorld * m_inv_s;

        const int r = static_cast<int>(std::ceil(radiusGrid));

        auto wrap = [](int v, int max) -> int
            {
                return ((v % max) + max) % max;
            };

        auto sample = [&](int x, int y) -> float
            {
                x = wrap(x, W);
                y = wrap(y, H);
                return grid[y * W + x];
            };

        float sum = 0.0f;
        float weightSum = 0.0f;

        for (int oy = -r; oy <= r; ++oy)
        {
            for (int ox = -r; ox <= r; ++ox)
            {
                const float dx = (cx + ox) - gx;
                const float dy = (cy + oy) - gy;

                const float dist2 = dx * dx + dy * dy;

                if (dist2 > radiusGrid * radiusGrid)
                    continue;

                // Gaussian kernel
                const float w =
                    std::exp(-dist2 / (2.0f * radiusGrid * radiusGrid));

                sum += sample(cx + ox, cy + oy) * w;
                weightSum += w;
            }
        }

        return (weightSum > 0.0f)
            ? sum / weightSum
            : 0.0f;
    }

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
