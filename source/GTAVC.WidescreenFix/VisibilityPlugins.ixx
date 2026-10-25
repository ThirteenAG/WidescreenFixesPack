module;

#include <stdafx.h>
#include "../Shared/CheckedPattern.hpp"

export module VisibilityPlugins;

class VisibilityPlugins
{
public:
    VisibilityPlugins()
    {
        WFP::onGameInitEvent() += []()
        {
            // m_alphaList: default 150 is not enough for bigger FOVs
            auto pattern = hook::pattern("68 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? B9");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTAVC.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            injector::WriteMemory(pattern.get_first(1), 200 * 3, true);
        };
    }
} VisibilityPlugins;