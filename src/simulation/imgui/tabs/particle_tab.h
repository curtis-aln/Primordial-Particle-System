#pragma once
#include "i_tab.h"

class ParticleTab final : public ITab
{
public:
    const char* label() const override { return "Particle"; }
    void draw(const SimSnapshot& snap, SimCtx& ctx) override;

private:
    // Local copies of physics params so sliders feel responsive
    // before the sim thread applies the command.
    float m_alpha_ = 180.f;
    float m_beta_ = 17.f;
    float m_gamma_ = 0.f;    // initialised lazily from PPS_Settings::gamma on first draw

    bool m_first_draw_ = true;

    void draw_physics(SimCtx& ctx);
    void draw_color_scheme();
};