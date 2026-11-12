module;

#include <stdafx.h>

export module Shadows;

// Shadow generators ask for 1024 px maps, also the ones stored with the lights of a scene. The engine caps 2D shadow
// maps at 2048 px and counts their memory against a 64 MB budget; a map that doesn't fit is not created.
// Point light cube maps keep their size: six faces each would not fit.
export namespace BullyShadows
{
    int Resolution = 1024;
    SafetyHookMid MapSize;

    void Install(int resolution)
    {
        Resolution = std::clamp(resolution, 1024, 4096);

        // The size a generator asks for when its 2D map is created, scaled with the setting
        auto mapSize = hook::pattern("0F B7 45 64 53 8A 5D 08 56 8B 75 0C 57 0F B7 F8");
        MapSize = safetyhook::create_mid(mapSize.get_first(4), [](SafetyHookContext& regs)
        {
            regs.eax = std::min<uint32_t>(regs.eax * Resolution / 1024, Resolution);
        });

        auto maxSize = hook::pattern("B8 00 08 00 00 39 44 24 04 7C 04 89 44 24 04 55 8B 6C 24 0C");
        injector::WriteMemory<int32_t>(maxSize.get_first(1), std::max(Resolution, 2048), true);

        // At most three generators are in use, with a 32-bit map each, and a spare map for the moment one is
        // replaced; only 4096 px needs more than 64 MB
        const uint32_t needed = 4u * 4u * uint32_t(Resolution) * uint32_t(Resolution);
        auto budget = hook::pattern("C7 86 B8 00 00 00 00 00 00 04 C6 86 BC 00 00 00 01");
        injector::WriteMemory<uint32_t>(budget.get_first(6), std::max(needed, 0x4000000u), true);
    }
}
