module;

#include <stdafx.h>

export module Misc;

import ComVars;

// Crash fixes: the sound sample manager's wait, the intro, and resolutions above 2048 in Direct3D
export namespace GTA2Misc
{
    void InitD3DDim()
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"d3dim"), "B8 00 08 00 00 39");
        injector::WriteMemory<int>(pattern.count(2).get(0).get<int>(1), -1, true);
    }

    void InitD3DDim700()
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"d3dim700"), "B8 00 08 00 00 39");
        injector::WriteMemory<int>(pattern.count(2).get(0).get<int>(1), -1, true);
    }

    void Install()
    {
        // Sound sample manager wait
        auto pattern = hook::pattern("FF D5 6A 01");
        injector::MakeNOP(pattern.get_first(2), 8, true); //0x4B6821+2

        // Intro crash fix
        pattern = hook::pattern("74 66 E8 ? ? ? ?");
        injector::MakeNOP(pattern.get_first(0), 2, true);
    }
}
