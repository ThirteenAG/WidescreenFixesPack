module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/TonyHawk/Camera.hpp"

export module Camera;

import ComVars;
import Cinema;

export void InstallCameraFix()
{
    hook::pattern pattern("");
    TonyCamera::Install(
        {
            "THAW",
            "F3 0F 11 24 24 8B 44 24 14 8B 48 18 E8 ? ? ? ? 8B 44 24 2C 83 C0 FB 83 F8 07 77 ? FF 24 85",
            12, 0x0,
            "C7 06 ? ? ? ? F3 0F 11 86 D4 00 00 00 E8 ? ? ? ? D9 96 C0 00 00 00",
            "8B F1 8B 86 84 01 00 00 85 C0 0F 84 ? ? ? ? 8B 86 8C 01 00 00 85 C0 74 ? 8B 40 2C",
            "E8 ? ? ? ? D9 44 24 04 8B CE D9 F2 DD D8 D9 54 24 04 D8 B6 D4 00 00 00 DE C9",
            0xc0, 0xd4, 0x184, 0xd8, 0x18c, 0x130
        }, &FixSettings().Float("MAIN.FOVFactor"));
    pattern = hook::pattern("D9 9E C4 00 00 00 E8 ? ? ? ? D9 86 C4 00 00 00 5E 59 C3");
    struct FovHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float fov = 0.0f;
            _asm { fstp dword ptr[fov] }
            *reinterpret_cast<float*>(regs.esi + 0xC4) = TonyCamera::Correct(regs.esi, fov, THAWCinema::FovAspect(Screen.fAspectRatio));
        }
    };
    injector::MakeInline<FovHook>(pattern.get_first(), pattern.get_first(6));
}
