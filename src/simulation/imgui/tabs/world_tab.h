#pragma once
#include "i_tab.h"

class WorldTab final : public ITab
{
public:
    const char* label() const override { return "World"; }
    void draw(const SimSnapshot& snap, SimCtx& ctx) override;
};