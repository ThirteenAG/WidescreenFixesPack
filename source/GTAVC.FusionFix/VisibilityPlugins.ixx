module;

#include <stdafx.h>

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
            injector::WriteMemory(pattern.get_first(1), 200 * 3, true);
        };
    }
} VisibilityPlugins;