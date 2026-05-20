#pragma once
#include "i_tab.h"

class SimulationTab final : public ITab
{
public:
    const char* label() const override { return "Simulation"; }
    void draw(const SimSnapshot& snap, SimCtx& ctx) override;

private:
    int   m_thread_count_ = static_cast<int>(PPS_Settings::threads);
    int   m_grid_freq_ = PPS_Settings::add_to_grid_freq;
    bool  m_first_draw_ = true;
};