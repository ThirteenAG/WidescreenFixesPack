module;

#include <stdafx.h>

export module IconBindings;

import EngineText;

export namespace REIcons
{
    inline void* fontLoad = nullptr;
    inline void* fontRelease = nullptr;

    inline bool Bind()
    {
        const auto name = REText::DataPattern("72 47 55 49 46 6F 6E 74 00 00 00 00").get_first<uint8_t>();
        // The current compiler places the vtable after the type name; the original registers it separately.
        auto current = hook::pattern("83 EC 20 53 8B D9 55 56 57 8B 03 FF 50 38");
        auto older = hook::pattern("83 EC 3C 53 8B D9 56 8B 03 57 FF 50 38");
        const bool original = current.empty();
        (original ? older : current).get_first();
        auto table = reinterpret_cast<uintptr_t*>(name + 12);
        if (original)
        {
            const auto pointer = reinterpret_cast<uintptr_t>(name);
            char signature[512]{};
            sprintf_s(signature, "6A 00 6A 00 6A 00 68 D0 00 00 00 68 ? ? ? ? 68 %02X %02X %02X %02X B9 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? E8",
                pointer & 255, (pointer >> 8) & 255, (pointer >> 16) & 255, (pointer >> 24) & 255);
            const auto registration = hook::pattern(signature).get_first();
            table = reinterpret_cast<uintptr_t*>(REText::Read<uint8_t*>(registration, 42) + 12);
        }
        const auto load = table[10];
        const auto release = table[14];
        fontLoad = hook::range_pattern(load, load + 48, original ?
            "83 EC 3C 53 8B D9 56 8B 03 57 FF 50 38" :
            "83 EC 20 53 8B D9 55 56 57 8B 03 FF 50 38").get_first();
        fontRelease = hook::range_pattern(release, release + 32,
            "56 8B F1 E8 ? ? ? ? 8B 8E AC 00 00 00 85 C9 74 12 8B 06 51 8B CE FF 50 44 C7 86 AC 00 00 00").get_first();
        hook::range_pattern(load, load + 512, "3D 47 46 44 00").get_first();
        hook::range_pattern(load, load + 512, "3D 06 0C 01 00").get_first();
        return true;
    }
}
