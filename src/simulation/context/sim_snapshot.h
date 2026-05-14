#pragma once

#include "../../particle_system/state.h"


struct SimSnapshot
{
    WorldToggles toggles;
    WorldStatistics stats;
    RenderData render;

	SimSnapshot() = default;

    SimSnapshot(int cell_render_reserve)
    {
	    render.colors.reserve(cell_render_reserve);
	    render.positions_x.reserve(cell_render_reserve);
	    render.positions_y.reserve(cell_render_reserve);
    }
};
