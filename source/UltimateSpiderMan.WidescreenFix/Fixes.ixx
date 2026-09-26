module;

#include "stdafx.h"

export module Fixes;

void* (__thiscall* pCreateFrameTimer)(void*, float, float) = nullptr;

SafetyHookInline shSwitchTimeOfDay;
void __cdecl SwitchTimeOfDay(int)
{
    // Native preset 2 selects the rainy sky, lighting and water environment.
    // Let the engine create/update its resources at the usual setup points.
    shSwitchTimeOfDay.ccall<void>(2);
}

float fFPSLimit = 60.0f;
void* __fastcall CreateFrameTimer(void* timer, void*, float, float)
{
    // nglSetFrameLock normally constructs a timer at 60 / frameLock FPS.
    // Supply the configured rate directly, for both frame-lock modes.
    return pCreateFrameTimer(timer, fFPSLimit, fFPSLimit);
}

class Fixes
{
public:
    Fixes()
    {
        WFP::onInitEvent() += []
        {
            CIniReader iniReader("");
            // Developer-only weather override; omitted from the shipped INI.
            // Rain particles and mission thunder effects may be script-driven.
            if (iniReader.ReadInteger("DEBUG", "AlwaysRain", 0) != 0)
            {
                auto pattern = hook::pattern("64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 8B 44 24 10 64 89 25 00 00 00 00 83 EC 30 53 55 33 DB 3B C3 56 57"); //0x408790
                shSwitchTimeOfDay = safetyhook::create_inline(pattern.get_first(), SwitchTimeOfDay);
            }

            if (iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0)
            {
                // Retain the frontend state advance after the four startup logos.
                auto pattern = hook::pattern("39 6E ? 0F 85"); //0x635DAE
                auto resume = pattern.get_first();
                pattern = hook::pattern("6A ? 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 ? 84 C0 75 ? 6A ? 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 ? 84 C0 75 ? 6A ? 68"); //0x635D55
                injector::MakeJMP(pattern.get_first(), resume, true);

                // Select the frontend's existing start-screen path after menu
                // initialization, bypassing activation of the copyright screen.
                pattern = hook::pattern("38 1D ? ? ? ? 8B 46 0C 89 46 34 74 51 83 F8 FF"); // 0x6488F3 + 12
                injector::MakeNOP(pattern.get_first(12), 2, true);
            }

            auto nFPSLimit = iniReader.ReadInteger("MAIN", "FPSLimit", 0);
            if (nFPSLimit > 0)
            {
                fFPSLimit = static_cast<float>(nFPSLimit);
                auto pattern = hook::pattern("E8 ? ? ? ? EB ? 33 C0 A3 ? ? ? ? E8 ? ? ? ? 6A"); //0x5AC399
                pCreateFrameTimer = reinterpret_cast<decltype(pCreateFrameTimer)>(injector::MakeCALL(pattern.get_first(), CreateFrameTimer, true).get<void>());

                pattern = hook::pattern("E8 ? ? ? ? EB ? 33 C0 8B C8 A3"); //0x76E7DF
                injector::MakeCALL(pattern.get_first(), CreateFrameTimer, true);
            }
        };
    }
} Fixes;
