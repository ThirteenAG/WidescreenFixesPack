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
        };
    }
} Fixes;
