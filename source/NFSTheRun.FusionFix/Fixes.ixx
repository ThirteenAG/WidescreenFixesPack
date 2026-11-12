module;

#include <stdafx.h>

export module Fixes;

import ComVars;

// The render job skips entities whose mesh is gone, using the game's existing skip path.
namespace
{
    // Streaming can free a mesh while an entity still refers to it; the links then read as null or small values
    bool HasMeshData(uintptr_t entity)
    {
        auto mesh = *(uintptr_t*)(entity + 0x50);
        if (mesh < 0x10000)
            return false;
        auto data = *(uintptr_t*)mesh;
        if (data < 0x10000)
            return false;
        auto info = *(uintptr_t*)(data + 0x10);
        return info >= 0x10000;
    }
}

// Effects spawned at a transform are tracked by their owner in a fixed table of 32 entries until they end or are
// stopped. When one more spawns while the table is full (Stage 8 Event 5, first explosion), the game gets no
// entry and writes through null. A spawn that finds the table full is dropped instead, like a spawn without an
// effect asset.
namespace
{
    bool IsEffectTableFull(uintptr_t owner)
    {
        auto table = owner + 4;
        auto freeList = *(uintptr_t*)(table + 0x1C);
        auto next = *(uintptr_t*)(table + 0x20);
        auto end = *(uintptr_t*)(table + 0x24);
        return !freeList && next == end;
    }

    SafetyHookInline shSpawnEffect = {};
    uint32_t __fastcall SpawnEffect(uintptr_t owner, void* edx, void* effect, void* local, void* transform, int arg)
    {
        if (IsEffectTableFull(owner))
            return 0;
        return shSpawnEffect.unsafe_thiscall<uint32_t>(owner, effect, local, transform, arg);
    }

    SafetyHookInline shSpawnEffectEx = {};
    uint32_t __fastcall SpawnEffectEx(uintptr_t owner, void* edx, void* effect, void* local, void* transform, int arg, int arg2)
    {
        if (IsEffectTableFull(owner))
            return 0;
        return shSpawnEffectEx.unsafe_thiscall<uint32_t>(owner, effect, local, transform, arg, arg2);
    }
}

class Fixes
{
public:
    Fixes()
    {
        WFP::onInitEvent() += []()
        {
            auto pattern = hook::pattern("0F 84 ? ? ? ? 8B 57 50 8B 0A 8B 51 10 8B 4A 04 D1 E9");
            static auto skipEntity = injector::GetBranchDestination(pattern.get_first()).as_int();
            static auto BuildEntityPrimitivesHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
            {
                if (!HasMeshData(regs.edi))
                    regs.eip = skipEntity;
            });

            // Effect spawns at a transform, found by the code after the spawn that adds the effect to the table
            pattern = hook::pattern("8B F8 85 FF 74 11 8B 4D 08 6A FF 56 57 51 8B CB E8");
            shSpawnEffect = safetyhook::create_inline(pattern.get_first(-0x10F), SpawnEffect);

            pattern = hook::pattern("8B F8 85 FF 74 13 8B 4D 18 8B 55 08 51 56 57 52 8B CB E8");
            shSpawnEffectEx = safetyhook::create_inline(pattern.get_first(-0x10F), SpawnEffectEx);

            // The race HUD's periodic heading check reads the player vehicle through a weak reference without a null
            // check. When the vehicle is already gone (leaving a stage from the post-race screen) the check is skipped,
            // as the HUD update that calls it already does.
            pattern = hook::pattern("0F 86 ? ? ? ? 83 BE E8 00 00 00 00 0F 57 C0 F3 0F 11 86 BC 00 00 00");
            static auto headingCheckEnd = injector::GetBranchDestination(pattern.get_first()).as_int();
            pattern = hook::pattern("2B D1 2B C1 3B D0 76 02 33 FF 0F 57 C0 F3 0F 10 47 58");
            static auto HeadingCheckHook = safetyhook::create_mid(pattern.get_first(10), [](SafetyHookContext& regs)
            {
                if (!regs.edi)
                    regs.eip = headingCheckEnd;
            });
        };
    }
} Fixes;
