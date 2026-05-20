#pragma once
#include "tabs/i_tab.h"
#include <vector>
#include <memory>
#include "../context/sim_snapshot.h"
#include "../context/sim_command.h"


class ControlPanel
{
public:
    ControlPanel();

    void draw(const SimSnapshot& snap, SimCtx& ctx, float dt);

private:
    std::vector<std::unique_ptr<ITab>> m_tabs_;
};