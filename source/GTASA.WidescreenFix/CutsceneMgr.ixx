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
            auto pattern = hook::pattern("88 1D ? ? ? ? 88 1D ? ? ? ? 88 1D ? ? ? ? 88 1D ? ? ? ? 88 1D ? ? ? ? 88 1D ? ? ? ? E8 ? ? ? ? 83 C4");
            ms_running.SetAddress(*pattern.get_first<bool*>(2));

            pattern = hook::pattern("88 1D ? ? ? ? 88 1D ? ? ? ? 88 1D ? ? ? ? E8 ? ? ? ? 83 C4");
            ms_useLodMultiplier.SetAddress(*pattern.get_first<bool*>(2));
        };
    }
} CutsceneMgr;