// density_grid_renderer.h
// ─────────────────────────────────────────────────────────────────────────────
//  Thread-safe renderer for the DensityGrid.
//  The UPDATE thread calls push_snapshot() after DensityGrid::build().
//  The RENDER thread calls render() and imgui_controls() each frame.
//
//  Thread model: triple-buffer via two atomics.
//    slot[0]  ─ one of three GridSnapshot slots
//    slot[1]  ─ ditto
//    slot[2]  ─ ditto
//    write_idx_ = slot the update thread is currently writing TO
//    read_idx_  = slot the render thread is currently reading FROM
//    The third slot is always free, so push_snapshot() never blocks.
// ─────────────────────────────────────────────────────────────────────────────

#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <atomic>
#include <array>
#include <cmath>
#include <algorithm>

#include "density_grid.h"

class DensityGridRenderer
{
public:
    // ── Rendering modes ───────────────────────────────────────────────────────
    enum class VisualMode : uint8_t
    {
        Density,           // heatmap of particle counts
        GradientMagnitude, // magnitude of ∇D (how sharp the density cliff is)
        GradientDirection, // hue-encoded direction of ∇D
        RightMinusLeft,    // signed (Gx·sin_a − Gy·cos_a) "turning pressure"
    };

    // ── Settings (safe to set from any thread; plain POD) ─────────────────────
    std::atomic<bool>       enabled{ true };
    std::atomic<VisualMode> mode{ VisualMode::Density };
    std::atomic<float>      brightness{ 0.15f };    // density → pixel scale
    std::atomic<uint8_t>    overlay_alpha{ 160 };   // 0–255 transparency

private:
    // ── Per-slot snapshot ─────────────────────────────────────────────────────
    struct GridSnapshot
    {
        std::vector<float> density;
        std::vector<float> grad_x;
        std::vector<float> grad_y;
        int   W = 0, H = 0;
        float cell_size = 1.f;
        bool  valid = false;
    };

    // Triple buffer — no mutex needed for push/pull themselves.
    // We only need a single CAS to swap read/write indices.
    static constexpr int NUM_SLOTS = 3;
    std::array<GridSnapshot, NUM_SLOTS> slots_;
    std::atomic<int> read_idx_{ 0 };
    std::atomic<int> write_idx_{ 1 };
    // slot 2 is the "free" slot not currently owned by either thread

    // ── SFML resources (render-thread only) ───────────────────────────────────
    sf::Texture texture_;
    sf::Sprite  sprite_{texture_};
    std::vector<std::uint8_t> pixels_;
    int tex_W_ = 0, tex_H_ = 0;   // track when we need to recreate the texture

public:
    DensityGridRenderer()
    {

    }

    // ── UPDATE THREAD ─────────────────────────────────────────────────────────
    //  Call after DensityGrid::build() every simulation tick.
    void push_snapshot(const DensityGrid& grid)
    {
        // Grab the free slot (the one that is neither read_idx nor write_idx)
        const int r = read_idx_.load(std::memory_order_acquire);
        const int w = write_idx_.load(std::memory_order_relaxed);
        int free_slot = -1;
        for (int i = 0; i < NUM_SLOTS; ++i)
            if (i != r && i != w) { free_slot = i; break; }

        GridSnapshot& snap = slots_[free_slot];
        snap.W = grid.width();
        snap.H = grid.height();
        snap.cell_size = grid.cell_size();
        snap.density = grid.density();   // std::vector copy — one allocation
        snap.grad_x = grid.gradient_x();
        snap.grad_y = grid.gradient_y();
        snap.valid = true;

        // Publish: new write slot becomes readable; old read slot becomes writable.
        const int old_read = read_idx_.exchange(free_slot, std::memory_order_acq_rel);
        write_idx_.store(old_read, std::memory_order_release);
    }

    // ── RENDER THREAD ─────────────────────────────────────────────────────────
    //  Call once per render frame.  world_scale = pixels_per_world_unit.
    void render(sf::RenderWindow& window, float world_scale)
    {
        if (!enabled.load(std::memory_order_relaxed))
            return;

        const int r = read_idx_.load(std::memory_order_acquire);
        const GridSnapshot& snap = slots_[r];
        if (!snap.valid || snap.W <= 0 || snap.H <= 0)
            return;

        const int W = snap.W, H = snap.H;

        // Recreate texture only when grid dimensions change
        if (W != tex_W_ || H != tex_H_)
        {
            texture_.resize({ static_cast<unsigned>(W), static_cast<unsigned>(H) });
            pixels_.resize(static_cast<size_t>(W) * H * 4);
            tex_W_ = W;
            tex_H_ = H;
        }

        fill_pixels(snap);
        texture_.update(pixels_.data());

        sprite_.setTexture(texture_, /* resetRect= */ true);
        const float s = world_scale * snap.cell_size;
        sprite_.setScale({ s, s });
        sprite_.setPosition({ 0.f, 0.f });

        window.draw(sprite_);
    }

    // ImGui controls panel — call from the render/GUI thread inside an ImGui window.
    // Requires imgui.h to be included at the call site.
    // Declared here for completeness; define inline or in a .cpp that has ImGui.
    // void imgui_controls();

private:
    // ── Pixel fill ────────────────────────────────────────────────────────────
    void fill_pixels(const GridSnapshot& snap)
    {
        const int    W = snap.W, H = snap.H;
        const float  br = brightness.load(std::memory_order_relaxed);
        const uint8_t al = overlay_alpha.load(std::memory_order_relaxed);
        const VisualMode m = mode.load(std::memory_order_relaxed);

        // Find the max value once so we can normalise
        float max_val = 1e-6f;
        if (m == VisualMode::Density)
        {
            for (float v : snap.density)  max_val = std::max(max_val, v);
        }
        else if (m == VisualMode::GradientMagnitude || m == VisualMode::GradientDirection)
        {
            for (int i = 0; i < W * H; ++i)
                max_val = std::max(max_val,
                    std::hypot(snap.grad_x[i], snap.grad_y[i]));
        }
        else // RightMinusLeft — range is signed, normalise by max |value|
        {
            // sign proxy: we don't have angle here so just show grad_x for debug
            for (float v : snap.grad_x)
                max_val = std::max(max_val, std::abs(v));
        }

        for (int y = 0; y < H; ++y)
        {
            for (int x = 0; x < W; ++x)
            {
                const int    i = y * W + x;
                const size_t p = static_cast<size_t>(i) * 4;

                uint8_t r = 0, g = 0, b = 0;

                switch (m)
                {
                case VisualMode::Density:
                {
                    const float t = std::min(snap.density[i] * br / max_val, 1.f);
                    heatmap_rgb(t, r, g, b);
                    break;
                }
                case VisualMode::GradientMagnitude:
                {
                    const float mag = std::hypot(snap.grad_x[i], snap.grad_y[i]);
                    const float t = std::min(mag / max_val, 1.f);
                    heatmap_rgb(t, r, g, b);
                    break;
                }
                case VisualMode::GradientDirection:
                {
                    const float angle = std::atan2(snap.grad_y[i], snap.grad_x[i]);
                    const float hue = (angle + 3.14159265f) / (2.f * 3.14159265f);
                    const float mag = std::hypot(snap.grad_x[i], snap.grad_y[i]);
                    const float sat = std::min(mag / max_val, 1.f);
                    hsv_to_rgb(hue, sat, 1.f, r, g, b);
                    break;
                }
                case VisualMode::RightMinusLeft:
                {
                    // Signed divergence: red = gradient points +x, blue = -x
                    const float v = snap.grad_x[i] / max_val;  // [-1, 1]
                    if (v >= 0.f) { r = static_cast<uint8_t>(v * 255); }
                    else { b = static_cast<uint8_t>(-v * 255); }
                    break;
                }
                }

                pixels_[p + 0] = r;
                pixels_[p + 1] = g;
                pixels_[p + 2] = b;
                pixels_[p + 3] = al;
            }
        }
    }

    // ── Colour helpers ────────────────────────────────────────────────────────

    // 5-stop perceptual heatmap: black → blue → cyan → green → yellow → red
    static void heatmap_rgb(float t, uint8_t& r, uint8_t& g, uint8_t& b)
    {
        t = std::clamp(t, 0.f, 1.f);
        float rf = 0.f, gf = 0.f, bf = 0.f;

        if (t < 0.25f) { bf = t * 4.f; }
        else if (t < 0.50f) { bf = 1.f; gf = (t - 0.25f) * 4.f; }
        else if (t < 0.75f)
        {
            gf = 1.f;
            bf = 1.f - (t - 0.5f) * 4.f;
            rf = (t - 0.5f) * 4.f;
        }
        else { rf = 1.f; gf = 1.f - (t - 0.75f) * 4.f; }

        r = static_cast<uint8_t>(rf * 255.f);
        g = static_cast<uint8_t>(gf * 255.f);
        b = static_cast<uint8_t>(bf * 255.f);
    }

    static void hsv_to_rgb(float h, float s, float v,
        uint8_t& r, uint8_t& g, uint8_t& b)
    {
        const int   sector = static_cast<int>(h * 6.f) % 6;
        const float f = h * 6.f - std::floor(h * 6.f);
        const float p = v * (1.f - s);
        const float q = v * (1.f - f * s);
        const float tv = v * (1.f - (1.f - f) * s);

        float rf = 0, gf = 0, bf = 0;
        switch (sector)
        {
        case 0: rf = v;  gf = tv; bf = p;  break;
        case 1: rf = q;  gf = v;  bf = p;  break;
        case 2: rf = p;  gf = v;  bf = tv; break;
        case 3: rf = p;  gf = q;  bf = v;  break;
        case 4: rf = tv; gf = p;  bf = v;  break;
        default:rf = v;  gf = p;  bf = q;  break;
        }
        r = static_cast<uint8_t>(rf * 255.f);
        g = static_cast<uint8_t>(gf * 255.f);
        b = static_cast<uint8_t>(bf * 255.f);
    }
};