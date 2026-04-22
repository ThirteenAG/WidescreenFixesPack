module;

#include <stdafx.h>
#include <d3d9.h>

export module Misc;

import ComVars;

namespace EAXSound
{
    SafetyHookInline shUpdate = {};
    void __fastcall Update(void* EAXSound, void* edx, float timeStep)
    {
        return shUpdate.unsafe_fastcall(EAXSound, edx, std::clamp(timeStep, 1.0f / 60.0f, std::numeric_limits<float>::max()));
    }
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEventAsync() += []()
        {
            CIniReader iniReader("");
            bool bSkipIntro = iniReader.ReadInteger("MISC", "SkipIntro", 0) != 0;
            static auto nFPSLimit = iniReader.ReadInteger("MISC", "FPSLimit", -1);
            bool bHighFPSCutscenes = iniReader.ReadInteger("MISC", "HighFPSCutscenes", 1) != 0;
            bool bSingleCoreAffinity = iniReader.ReadInteger("MISC", "SingleCoreAffinity", 0) != 0;
            bool bNoOpticalDriveFix = iniReader.ReadInteger("MISC", "NoOpticalDriveFix", 1) != 0;

            if (bSkipIntro)
            {
                static constexpr auto SkipThis = "SkipThis";
                auto pattern = hook::pattern("8B F1 50 89 74 24 08 E8 ? ? ? ? 8B 46 04 68 ? ? ? ? 68 ? ? ? ? 50");
                uint32_t* dword_4A888B = pattern.count(3).get(0).get<uint32_t>(16);
                uint32_t* dword_4A895B = pattern.count(3).get(1).get<uint32_t>(16);
                injector::WriteMemory(dword_4A888B, &SkipThis, true); // FMV Opening
                injector::WriteMemory(dword_4A895B, &SkipThis, true); // EA Bumper

                // THX
                uint32_t* dword_4A8CEB = hook::pattern("56 57 8B F1 50 89 74 24 0C E8 ? ? ? ? 68 ? ? ? ? 33 C0").count(1).get(0).get<uint32_t>(15);
                injector::WriteMemory(dword_4A8CEB, &SkipThis, true);

                // PSA
                uint32_t* dword_4A8B74 = hook::pattern("C7 06 ? ? ? ? E8 ? ? ? ? 8B 46 04 68 ? ? ? ? 68 ? ? ? ? 50").count(1).get(0).get<uint32_t>(15);
                injector::WriteMemory(dword_4A8B74, &SkipThis, true);
            }

            if (nFPSLimit)
            {
                if (nFPSLimit < 0)
                {
                    if (nFPSLimit == -1)
                        nFPSLimit = GetDesktopRefreshRate();
                    else if (nFPSLimit == -2)
                        nFPSLimit = GetDesktopRefreshRate() * 2;
                    else
                        nFPSLimit = 60;
                }

                static float FrameTime = 1.0f / nFPSLimit;
                uint32_t* dword_865558 = *hook::pattern("D9 05 ? ? ? ? B9 64 00 00 00 D8 64").count(1).get(0).get<uint32_t*>(2);
                injector::WriteMemory(dword_865558, FrameTime, true);
                uint32_t* dword_7FB710 = *hook::pattern("D9 05 ? ? ? ? D8 74 ? ? D9 1D ? ? ? ? C3").count(1).get(0).get<uint32_t*>(33);
                injector::WriteMemory(dword_7FB710, FrameTime, true);
                // Video mode frametime
                float* flt_7875BC = *hook::pattern("8B 44 24 14 D9 05 ? ? ? ? 8B 0D ? ? ? ? D8 44 24 14 8B 15 ? ? ? ? D9 05 ? ? ? ?").count(1).get(0).get<float*>(0x55); //0x57EB8B anchor, 0x57EBE0 dereference
                injector::WriteMemory(flt_7875BC, FrameTime, true);
                // a function in eDisplayFrame (particle effects?) frametime
                uint32_t* dword_40A744 = hook::pattern("68 89 88 88 3C").count(1).get(0).get<uint32_t>(1);
                injector::WriteMemory(dword_40A744, FrameTime, true);
                // Smokeable frametime
                float* flt_7FC858 = *hook::pattern("8B F1 E8 ? ? ? ? 8A 86 06 05 00 00 84 C0 0F 84 ? ? ? ?").count(1).get(0).get<float*>(0x2F); //0x5B1DE0 anchor, 0x005B1E0F dereference
                injector::WriteMemory(flt_7FC858, FrameTime, true);

                // GAME BUGFIX: disable player steering autocentering to prevent sticky input
                // same thing as NFSU fix, check its dllmain.cpp for more info
                uint32_t* dword_416D82 = hook::pattern("DD D8 D9 44 24 18 D9 54 24 1C D9 E1 D8 1D ? ? ? ? DF E0 F6 C4 05").count(1).get(0).get<uint32_t>(0x17); //0x416D6B anchor
                injector::WriteMemory<uint8_t>(dword_416D82, 0xEB, true);
            }

            if (bHighFPSCutscenes)
            {
                static int AnimSceneFPS = 60;
                static float fAnimSceneFPS = 60.0f;

                if (nFPSLimit > 0)
                    AnimSceneFPS = nFPSLimit;

                if (AnimSceneFPS % 30)
                    AnimSceneFPS = AnimSceneFPS - (AnimSceneFPS % 30);

                // car physics are buggy above 60 FPS
                // TODO: bugfix this if possible
                if (AnimSceneFPS > 60)
                    AnimSceneFPS = 60;

                if (AnimSceneFPS < 30)
                    AnimSceneFPS = 30;

                fAnimSceneFPS = (float)AnimSceneFPS;

                uint32_t* dword_435FA4 = hook::pattern("7C ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 ? 5F 5E").count(1).get(0).get<uint32_t>(3);
                injector::WriteMemory(dword_435FA4, fAnimSceneFPS, true);
                uint32_t* dword_7A572C = *hook::pattern("8B 15 ? ? ? ? 52 A3 ? ? ? ? E8 ? ? ? ? 83 C4 14").count(1).get(0).get<uint32_t*>(2);
                injector::WriteMemory(dword_7A572C, fAnimSceneFPS, true);
            }

            if (bSingleCoreAffinity)
            {
                if (AffinityChanges::Init())
                {
                    IATHook::Replace(GetModuleHandleA(NULL), "KERNEL32.DLL",
                        std::forward_as_tuple("CreateThread", AffinityChanges::CreateThread_GameThread)
                    );
                }
            }

            if (bNoOpticalDriveFix)
            {
                auto pattern = hook::pattern("56 8B 35 ? ? ? ? A1 ? ? ? ? 85 C0 ? ? E8");
                uint32_t* dword_5C0D3F = pattern.count(1).get(0).get<uint32_t>(15);
                injector::WriteMemory<uint8_t>(dword_5C0D3F, 0x78, true);
                uint32_t* dword_5C0D54 = pattern.count(1).get(0).get<uint32_t>(36);
                injector::WriteMemory<uint8_t>(dword_5C0D54, 0x00, true);
            }

            {
                auto pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 89 1D");
                EAXSound::shUpdate = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), EAXSound::Update);
            }
        };
    }
} Misc;