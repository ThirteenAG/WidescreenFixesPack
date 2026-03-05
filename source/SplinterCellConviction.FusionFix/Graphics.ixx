module;

#include <stdafx.h>

export module Graphics;

import ComVars;

injector::hook_back<void(__cdecl*)(float a1, float a2, int a3)> hb_8330DB;
void __cdecl sub_8330DB(float a1, float a2, int a3)
{
    gVisibility = a1;

    if (!bDisableBlackAndWhiteFilter)
        return hb_8330DB.fun(a1, a2, a3);
}

export injector::hook_back<void(__cdecl*)(void* a1, int a2, int a3)> hb_100177B7;
export void __cdecl sub_100177B7(void* a1, int a2, int a3)
{
    if (gBlacklistIndicators) //enables bloom in dark areas
    {
        if (gVisibility == 1.0f)
            return hb_100177B7.fun(a1, a2, a3);
        else
            return;
    }
    return hb_100177B7.fun(a1, a2, a3);
}

// Distortion during movement (sonar)
export injector::hook_back<void(__fastcall*)(void* self, int edx)> hb_1002581C;
export void __fastcall sub_1002581C(void* self, int edx)
{
    //return hb_1002581C.fun(self, edx);
}

export void InitGraphics()
{
    //if (bDisableBlackAndWhiteFilter) //light and shadow
    {
        auto pattern = hook::pattern("E8 ? ? ? ? D9 05 ? ? ? ? 83 C4 0C 33 F6 56 51");
        hb_8330DB.fun = injector::MakeCALL(pattern.get_first(), sub_8330DB, true).get();
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 0C 83 3D ? ? ? ? ? 0F 8C ? ? ? ? 7F 0D 83 3D ? ? ? ? ? 0F 86");
        hb_8330DB.fun = injector::MakeCALL(pattern.get_first(), sub_8330DB, true).get();
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 0C 83 3D ? ? ? ? ? 7C 77 7F 09 83 3D ? ? ? ? ? 76 6C");
        hb_8330DB.fun = injector::MakeCALL(pattern.get_first(), sub_8330DB, true).get();
        pattern = hook::pattern("D9 5C 24 04 D9 40 08 D9 1C 24 E8 ? ? ? ? 83 C4 0C C3");
        hb_8330DB.fun = injector::MakeCALL(pattern.get_first(10), sub_8330DB, true).get();
    }
}
