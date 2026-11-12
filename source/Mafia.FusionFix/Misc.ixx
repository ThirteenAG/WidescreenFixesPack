module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <intrin.h>
#include <unordered_map>

export module Misc;

import ComVars;

// Skip intro: the logos at start-up
char __fastcall SkipLogo(const char*, void*, float, float)
{
    return 1;
}

void InitIntro()
{
    if (!FixSettings().Bool("MAIN.SkipIntro")) return;
    auto logos = hook::pattern("68 00 00 16 44 68 00 00 48 44 B9 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? 68 00 00 16 44 68 00 00 48 44 E8 ? ? ? ? B9 ? ? ? ? 68 00 00 C8 43 68 00 00 30 44 E8 ? ? ? ?");
    if (!logos.empty()) // 1.1 and 1.2
    {
        for (int offset : { 15, 35, 55 })
            injector::MakeCALL(logos.get_first(offset), SkipLogo, true);
    }
    else // 1.0
    {
        auto first = hook::pattern("84 C0 74 36 A1 ? ? ? ? 50 8B 08 FF 11 8B F0 8D 44 24 60");
        auto second = hook::pattern("84 C0 0F 84 8D 00 00 00 A1 ? ? ? ? 50 8B 10 FF 52 70 89 44 24 08");
        injector::WriteMemory<uint8_t>(first.get_first(2), 0xEB, true);
        injector::WriteMemory<uint16_t>(second.get_first(2), 0xE990, true);
    }
}

void RestoreGameOverSound()
{
    static constexpr char sound[] = "sounds\\music\\13 - Game Over.ogg";
    auto site = hook::pattern("85 FF 0F 84 ? ? ? ? 8B 0D ? ? ? ? 56 68 ? ? ? ? E8 ? ? ? ? 8B C8 E8");
    injector::WriteMemory<const char*>(site.get_first(16), sound, true);
}

void PreserveSniperBinding()
{
    auto update = hook::pattern("6A 3C 8D 86 50 03 00 00 53 89 8E F0 01 00 00 50 6A 1D 8B CE 89 96 9C 00 00 00 88 9E 91 00 00 00 E8 ? ? ? ? 85 C0 74 45 6A FF 8D 4C 24 14 53 51 6A 1D B9 ? ? ? ? 66 89 7C 24 24 66 C7 44 24 26 13 00 89 5C 24 28 89 5C 24 20 E8 ? ? ? ? 53 8B CE 85 C0 53 74 04 53 53 EB 03 6A 13 57 6A 1D E8 ? ? ? ? 8B CE E8 ? ? ? ? 8D 4C 24 20");
    if (!update.empty()) // 1.1 and 1.2
    {
        // Keep the saved binding and publish the profile without rewriting the sniper action.
        injector::WriteMemory<uint16_t>(update.get_first(39), 0x3EEB, true);
    }
    else // 1.0 already preserves the binding
    {
        auto native = hook::pattern("8B 46 70 8B 4E 64 8B 56 68 89 86 94 00 00 00 89 8E F0 01 00 00 89 96 9C 00 00 00 C6 86 91 00 00 00 00");
        native.get_first();
    }
}

// Mission programs run one command per frame and drop what a "wait" overshoots, so their loops speed up with the
// frame rate (Free Ride Extreme monster and bombs). Above 60 FPS, run them on frames 16 ms apart and hand each
// program the time of the frames it sat out. Program starts and script events run unchanged.
namespace ScriptTiming
{
    constexpr double step = 0.016;
    SafetyHookInline runHook;
    uintptr_t startReturn = 0, eventReturn = 0;
    uint32_t presented = 0, decided = ~0u;
    void* first = nullptr;
    bool runFrame = true;
    double credit = 0.0;
    LARGE_INTEGER frequency{}, last{};
    std::unordered_map<void*, uint32_t> skipped;

    // Once per frame: time left over carries to the next frame, so 60 FPS and below run every frame
    void Decide(uint32_t elapsed)
    {
        if (runFrame)
            skipped.clear();
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        const double real = last.QuadPart ? static_cast<double>(now.QuadPart - last.QuadPart) / frequency.QuadPart : step;
        last = now;
        credit = std::min(credit + std::max(real, elapsed / 1000.0), step * 2.0);
        runFrame = credit >= step;
        if (runFrame)
            credit -= step;
    }

    char __fastcall Run(void* program, void*, uint32_t elapsed)
    {
        const auto caller = reinterpret_cast<uintptr_t>(_ReturnAddress());
        if (caller == startReturn || caller == eventReturn)
            return runHook.unsafe_thiscall<char>(program, elapsed);
        if (decided != presented || program == first)
        {
            decided = presented;
            first = program;
            Decide(elapsed);
        }
        if (!runFrame)
        {
            skipped[program] += elapsed;
            return 0;
        }
        if (!skipped.empty())
        {
            if (auto it = skipped.find(program); it != skipped.end())
                elapsed += it->second;
        }
        return runHook.unsafe_thiscall<char>(program, elapsed);
    }

    void Init()
    {
        auto run = hook::pattern("56 57 8B 7C 24 0C 8B F1 8B 4E 10 85 C9 74 2B 8B 46 14 2B C1 8B 4E 1C C1 F8 03 3B C8 73 1C 8B 46 10 83 3C C8 06 75 04 C6 46 64 00");
        startReturn = reinterpret_cast<uintptr_t>(hook::get_pattern("6A 01 C7 41 44 00 00 00 00 C6 41 64 01 E8", 18));
        eventReturn = reinterpret_cast<uintptr_t>(hook::get_pattern("8B 06 6A 01 8B CE FF 50 1C 6A 01 8B CE E8", 18));
        QueryPerformanceFrequency(&frequency);
        runHook = safetyhook::create_inline(run.get_first(), Run);
        WFP::onGameProcessEvent() += []()
        {
            ++presented;
        };
    }
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            InitIntro();
            PreserveSniperBinding();
            RestoreGameOverSound();
            ScriptTiming::Init();

            // Portable settings: the game's registry settings in savegame\settings.bin
            bool bWriteSettingsToFile = FixSettings().Bool("MAIN.PortableSettings") != 0;

            if (bWriteSettingsToFile)
            {
                auto szSettingsSavePath = GetExeModulePath<std::string>() + "savegame\\settings.bin";
                RegistryWrapper("Illusion Softworks", szSettingsSavePath);
                RegistryWrapper::OverrideTypeREG_NONE = REG_BINARY;

                IATHook::Replace(GetModuleHandleA(NULL), "ADVAPI32.DLL",
                    std::forward_as_tuple("RegCloseKey", RegistryWrapper::RegCloseKey),
                    std::forward_as_tuple("RegCreateKeyA", RegistryWrapper::RegCreateKeyA),
                    std::forward_as_tuple("RegOpenKeyA", RegistryWrapper::RegOpenKeyA),
                    std::forward_as_tuple("RegOpenKeyExA", RegistryWrapper::RegOpenKeyExA),
                    std::forward_as_tuple("RegCreateKeyExA", RegistryWrapper::RegCreateKeyExA),
                    std::forward_as_tuple("RegQueryValueExA", RegistryWrapper::RegQueryValueExA),
                    std::forward_as_tuple("RegSetValueExA", RegistryWrapper::RegSetValueExA),
                    std::forward_as_tuple("RegQueryValueA", RegistryWrapper::RegQueryValueA),
                    std::forward_as_tuple("RegDeleteKeyA", RegistryWrapper::RegDeleteKeyA),
                    std::forward_as_tuple("RegEnumKeyA", RegistryWrapper::RegEnumKeyA)
                );
            }
        };
    }
} Misc;
