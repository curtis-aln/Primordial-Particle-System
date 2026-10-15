#include "density_grid.h"


// ── Bilinear sampler ──────────────────────────────────────────────────────
// Samples any of the three flat grids at world position (wx, wy).
// Matches the bilinear pattern in heatmap.h's scatter().
float DensityGrid::bilerp(const std::vector<float>& grid, float wx, float wy) const
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


float DensityGrid::bicubic(const std::vector<float>& grid, float wx, float wy) const
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


float DensityGrid::sampleRadius(
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


float DensityGrid::sample8(const std::vector<float>& grid, float wx, float wy) const
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



float DensityGrid::cubicInterpolate(float p0, float p1, float p2, float p3, float t) const
{
    const float a0 = -0.5f * p0 + 1.5f * p1 - 1.5f * p2 + 0.5f * p3;
    const float a1 = p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3;
    const float a2 = -0.5f * p0 + 0.5f * p2;
    const float a3 = p1;

    return ((a0 * t + a1) * t + a2) * t + a3;
}
