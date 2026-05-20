#pragma once
#include "i_tab.h"

class HelpTab final : public ITab
{
public:
    const char* label() const override { return "Help"; }
    void draw(const SimSnapshot& snap, SimCtx& ctx) override;
};