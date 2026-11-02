module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Misc;

import ComVars;

int32_t nLanguage;
int32_t __cdecl SetLanguage(LPCSTR lpValueName)
{
    return nLanguage;
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            nLanguage = settings.Int("MAIN.Language");
            bool bHighResolutionShadows = settings.Bool("MISC.HighResolutionShadows");

            auto pattern = hook::pattern("89 88 ? ? ? ? 0F B6 8A ? ? ? ? 88 88 ? ? ? ? 0F B6 8A ? ? ? ? 88 88 ? ? ? ? 0F B6 8A ? ? ? ? 88 88 ? ? ? ? 0F B6 8A ? ? ? ? 88 88 ? ? ? ? 0F B6 8A");
            static auto CacheMissionIDs = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
            {
                CurrentCaseName = *(int*)(regs.eax + 0xD9C);
                CurrentMissionName = *(int*)(regs.eax + 0xDA0);
            });

            // One request to fusionfix.io per start when MAIN.CheckForUpdates is on; FusionFixHub opens on its update
            // page when the game closes.
            RWGTA::UpdateCheck::Config config;
            config.pluginId = "TrueCrimeNewYorkCity.FusionFix";
            config.fileVersion = rsc_FileVersion;
            config.settings = &FixSettings();
            config.playable = []() { return true; };
            config.show = [](const std::wstring&) {};
            RWGTA::UpdateCheck::Init(std::move(config));

            // Load the data from the folder next to the exe instead of the registry install path
            pattern = hook::pattern("B9 20 00 00 00 8D 7C 24 18 F3 AB 8D 44 24 0C"); //0x496F27
            struct RegHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.ecx = 0x20;
                    regs.edi = (regs.esp + 0x18);

                    GetModuleFileNameA(NULL, (char*)regs.edi, MAX_PATH);
                    *strrchr((char*)regs.edi, '\\') = '\0';
                    strcat_s((char*)regs.edi, MAX_PATH, "\\data");
                }
            }; injector::MakeInline<RegHook>(pattern.get_first(0), pattern.get_first(11));
            injector::MakeJMP(pattern.get_first(11), hook::pattern("8D 44 24 18 8D 50 01").count(2).get(1).get<uintptr_t>(0), true); //0x496FD8

            if (nLanguage >= 0)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? 8B 04 85 ? ? ? ? 83 C4 04 C3"); //0x495E95
                injector::MakeCALL(pattern.count(1).get(0).get<uintptr_t>(0), SetLanguage, true);
                pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 83 C4 04 C3"); //0x495EB5
                injector::MakeCALL(pattern.count(2).get(1).get<uintptr_t>(5), SetLanguage, true);
            }

            if (bHighResolutionShadows)
            {
                auto pattern = hook::pattern("68 00 01 00 00 68 00 01 00 00 6A 06 B9 ? ? ? ? E8 ? ? ? ? 83 3D ? ? ? ? ? 75 ? 83 3D ? ? ? ? ? 75 ? B9 ? ? ? ? E8 ? ? ? ? 6A 01");
                injector::WriteMemory(pattern.get_first(1), 256 * 3, true);
                injector::WriteMemory(pattern.get_first(6), 256 * 3, true);

                pattern = hook::pattern("68 00 01 00 00 68 00 01 00 00 6A 06 8D 8C 24");
                injector::WriteMemory(pattern.get_first(1), 256 * 3, true);
                injector::WriteMemory(pattern.get_first(6), 256 * 3, true);

                pattern = hook::pattern("68 00 01 00 00 68 00 01 00 00 6A 06 B9 ? ? ? ? E8 ? ? ? ? 83 3D ? ? ? ? ? 75 ? 83 3D ? ? ? ? ? 75 ? B9 ? ? ? ? E8 ? ? ? ? 51");
                injector::WriteMemory(pattern.get_first(1), 256 * 3, true);
                injector::WriteMemory(pattern.get_first(6), 256 * 3, true);
            }

            // Shadows distance fix
            {
                pattern = hook::pattern("68 ? ? ? ? 0F 59 E1");
                injector::WriteMemory(pattern.get_first(1), 10.0f * 10.0f, true);
            }
        };

        WFP::onGameProcessEvent() += []()
        {
            RWGTA::UpdateCheck::Tick();
        };
    }
} Misc;
