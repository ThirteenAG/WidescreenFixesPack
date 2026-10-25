module;

#include <stdafx.h>
#include "../Shared/CheckedPattern.hpp"
#include "common.h"

export module CutsceneMgr;

export class CCutsceneMgr
{
public:
    static inline GameRef<bool> ms_running;
    static inline GameRef<bool> ms_useLodMultiplier;
    static bool IsRunning() { return ms_running; }

    CCutsceneMgr()
    {
        WFP::onGameInitEvent() += []()
        {
            auto pattern = hook::pattern("80 3D ? ? ? ? ? 0F 84 ? ? ? ? D9 05 ? ? ? ? D8 0D");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTA3.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            ms_running.SetAddress(*pattern.get_first<bool*>(2));

            pattern = hook::pattern("C6 05 ? ? ? ? ? E8 ? ? ? ? A1");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("GTA3.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            ms_useLodMultiplier.SetAddress(*pattern.get_first<bool*>(2));
        };
    }
} CutsceneMgr;