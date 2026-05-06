module;

#include <stdafx.h>
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
            auto pattern = hook::pattern("80 3D ? ? ? ? ? 0F 84 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 31 DB");
            ms_running.SetAddress(*pattern.get_first<bool*>(2));

            pattern = hook::pattern("C6 05 ? ? ? ? ? 72 ? 89 C0");
            ms_useLodMultiplier.SetAddress(*pattern.get_first<bool*>(2));
        };
    }
} CutsceneMgr;