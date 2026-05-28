#include "density_grid.h"

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
void DensityGrid::smooth_box()
{
    const int W = m_W, H = m_H;
    const int bh = m_box_half;

    // Three cascade passes in each dimension approximate a Gaussian well
    // enough for PPS purposes (~5 % error on neighbour count vs exact disc).
    for (int pass = 0; pass < box_filter_cascade_passes; ++pass)
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

    const float box_area = static_cast<float>((2 * bh + 1) * (2 * bh + 1));
    const float disc_area = 3.14159f * static_cast<float>(m_box_half * m_box_half);
    const float scale = disc_area / box_area;
    for (float& v : m_D) v *= scale;
}



// ── Step 2b — Gaussian smoothing (alternative to smooth_box) ─────────────
// Separable 1-D Gaussian, σ ≈ visual_radius / 2.  More accurate, ~5× more
// multiply-adds than the box filter but still O(W·H·kernel_w).
// Swap smooth_box() for smooth_gaussian() in build() when training neural
// models that need a smooth, differentiable density signal.
void DensityGrid::smooth_gaussian()
{
    const int W = m_W, H = m_H;

    const float scaled_sigma = PPS_Settings::visual_radius * m_inv_s * gaussian_sigma;
    const int   krad = static_cast<int>(std::ceil(2.5f * scaled_sigma));

    // Build 1-D kernel
    std::vector<float> kern(2 * krad + 1);
    float kern_sum = 0.f;
    for (int k = -krad; k <= krad; ++k)
    {
        const float v = std::exp(-0.5f * (k * k) / (scaled_sigma * scaled_sigma));
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

