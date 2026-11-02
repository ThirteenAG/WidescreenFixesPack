module;

#include <stdafx.h>

export module IconBindings;

import EngineText;

export namespace RE0Icons
{
    inline void* fontLoad = nullptr;
    inline void* fontRelease = nullptr;

    inline bool Bind()
    {
        const auto entry = hook::pattern("56 8B F1 E8 ? ? ? ? 8B 46 68 25 FF FF FF 00 0D 00 00 00 41 89 46 68 C6 86 BC 00 00 00 00").get_first<uint8_t>();
        const auto table = RE0Text::Read<uintptr_t*>(entry, 0x37);
        const auto load = table[10];
        fontLoad = hook::range_pattern(load, load + 32,
            "83 EC 3C 53 8B D9 56 8B 03 57 FF 50 38").get_first();
        hook::range_pattern(load, load + 96, "3D 47 46 44 00").get_first();
        hook::range_pattern(load, load + 96, "3D 06 0C 01 00").get_first();
        const auto release = table[14];
        fontRelease = hook::range_pattern(release, release + 32,
            "56 8B F1 E8 ? ? ? ? 8B 8E AC 00 00 00 85 C9 74 12 8B 06 51 8B CE FF 50 44 C7 86 AC 00 00 00").get_first();
        return true;
    }
}
