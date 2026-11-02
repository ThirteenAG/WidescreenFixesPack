module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module SkipStart;

import EngineText;

export namespace RE0Start
{
    inline SafetyHookMid prompt;
    inline uintptr_t* titleVtable = nullptr;
    inline bool (*blocked)() = nullptr;
    inline bool ready = false, passed = false;

    inline bool AtPrompt(const void* gui)
    {
        if (!titleVtable || !gui || RE0Text::Read<uintptr_t*>(gui, 0) != titleVtable ||
            !RE0Text::Read<uint8_t>(gui, 596) || RE0Text::Read<uint32_t>(gui, 636) != 7)
            return false;
        const auto node = RE0Text::Read<void*>(gui, 768);
        const auto timeline = node ? RE0Text::Read<void*>(node, 236) : nullptr;
        const auto clip = timeline ? RE0Text::Read<void*>(timeline, 148) : nullptr;
        return clip && RE0Text::Read<uint32_t>(clip, 0) == 1000005;
    }

    inline void Confirm(SafetyHookContext& context)
    {
        if (passed || !blocked || blocked() || !AtPrompt(reinterpret_cast<void*>(context.edi))) return;
        // Follow the native keyboard confirmation path for the initial Start selection.
        context.eflags &= ~0x40u;
        passed = true;
    }

    inline void Install(bool (*isBlocked)())
    {
        blocked = isBlocked;
        ready = passed = false;
        titleVtable = nullptr;
        auto constructor = hook::pattern("53 56 57 8B F1 E8 ? ? ? ? 8D 8E 8C 02 00 00 C7 06 ? ? ? ? C7 86 60 02 00 00 00 00 00 00 C7 86 64 02 00 00 00 00 00 00");
        auto branch = hook::pattern("A1 ? ? ? ? 85 98 6C 0B 00 00 0F 85 ? ? ? ? F7 C6 50 00 05 00 74 ? 8B 8F 00 03 00 00 E8 ? ? ? ? 8B 8F 00 03 00 00 3D 45 42 0F 00");
        const auto entry = constructor.get_first<uint8_t>();
        const auto site = branch.get_first<uint8_t>();
        const auto table = RE0Text::Read<uintptr_t*>(entry, 18);
        hook::range_pattern(reinterpret_cast<uintptr_t>(entry), reinterpret_cast<uintptr_t>(entry) + 256,
            "C7 86 7C 02 00 00 04 00 00 00").get_first();
        const auto frame = table[8];
        hook::range_pattern(frame, frame + 16, "56 8B F1 E8 ? ? ? ? 80 BE 54 02 00 00 00").get_first();
        const auto getter = reinterpret_cast<uintptr_t>(site + 36 + RE0Text::Read<int32_t>(site, 32));
        hook::range_pattern(getter, getter + 33,
            "8B 81 EC 00 00 00 85 C0 74 11 8B 80 94 00 00 00 85 C0 74 03 8B 00 C3").get_first();
        titleVtable = table;
        ready = true;
        if (FixSettings().Bool("MAIN.SkipPressStart"))
        {
            prompt = safetyhook::create_mid(site + 11, Confirm);
            ready = bool(prompt);
        }
        FixSettings().Availability("MAIN.SkipPressStart", [](std::string_view value) { return value == "0" || ready; });
        if (!ready) OutputDebugStringA("Resident Evil 0: title prompt skip unavailable; native confirmation retained.\n");
    }
}
