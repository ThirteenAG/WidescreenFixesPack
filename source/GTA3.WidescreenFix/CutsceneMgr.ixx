module;

#include <stdafx.h>
#include "common.h"

export module CutsceneMgr;

export class CCutsceneMgr
{
public:
    static inline GameRef<bool> ms_running;
    static bool IsRunning() { return ms_running; }

    CCutsceneMgr()
    {
        WFP::onGameInitEvent() += []()
        {
            auto pattern = hook::pattern("80 3D ? ? ? ? ? 0F 84 ? ? ? ? D9 05 ? ? ? ? D8 0D");
            ms_running.SetAddress(*pattern.get_first<bool*>(2));
        };
    }
} CutsceneMgr;