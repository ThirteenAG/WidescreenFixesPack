module;

#include <stdafx.h>

export module AftermarketTires;

import ComVars;

// Wheel packs whose tires are textured with the placeholders TIRE_STYLE01 and TIRE_STYLE01_N need those textures
// in a texture pack. The game has neither and replaces neither, so such tires got the default texture, which is
// transparent: rims without tires. Without the textures, each car's texture replacement table now gives the
// placeholders the car's own tire textures.

// The game's hash of texture names, continued from the hash of a prefix
constexpr uint32_t NameHash(std::string_view name, uint32_t hash = 0xFFFFFFFF)
{
    for (char c : name)
        hash = hash * 33 + static_cast<uint8_t>(c);
    return hash;
}

struct ReplacementTextureEntry
{
    uint32_t dwOldNameHash;
    uint32_t dwNewNameHash; // 0 = entry not in use
    intptr_t pTextureInfo;  // -1 = looked up when the table is next applied
};

// CarRenderInfo's table of 73 entries, used by the models of the car's parts
constexpr ptrdiff_t ReplacementTextureTable = 0x23C;
constexpr size_t SpareEntry = 10; // the game never gives it a texture name
constexpr size_t TireEntry = 12;  // <CAR>_TIRE, replaced by TRAFFIC_TIRE on traffic cars only

class AftermarketTires
{
public:
    AftermarketTires()
    {
        WFP::onInitEventAsync() += []()
        {
            // GetTextureInfo, the lookup of a texture by its name hash in the loaded texture packs
            auto pattern = hook::pattern("53 55 56 57 8B 7C 24 14 85 FF 0F 84 ? ? ? ? 8B C7 25 FF 00 00 00 8B 04 85");
            static auto GetTextureInfo = reinterpret_cast<void* (__cdecl*)(uint32_t nameHash, int returnDefault, int includeUnloaded)>(pattern.get_first());

            // CarRenderInfo's constructor, after it has set up the tire entry
            pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? 89 86 D8 02 00 00");
            static auto CarRenderInfoHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                constexpr uint32_t tireStyle = NameHash("TIRE_STYLE01");
                constexpr uint32_t tireStyleNormal = NameHash("_N", tireStyle);

                // A texture pack that brings the placeholder textures keeps them
                if (GetTextureInfo(tireStyle, 0, 0))
                    return;

                auto table = reinterpret_cast<ReplacementTextureEntry*>(regs.esi + ReplacementTextureTable);
                auto& spare = table[SpareEntry];
                auto& tire = table[TireEntry];
                uint32_t carTire = tire.dwOldNameHash;

                spare = { tireStyle, tire.dwNewNameHash ? tire.dwNewNameHash : carTire, -1 };
                // Cars that keep their own tire texture have no use for its entry
                if (!tire.dwNewNameHash)
                    tire = { tireStyleNormal, NameHash("_N", carTire), -1 };
            });
        };
    }
} AftermarketTires;
