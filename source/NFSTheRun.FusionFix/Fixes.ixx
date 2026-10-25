module;

#include <stdafx.h>

export module Fixes;

import ComVars;

// The render job that builds the primitives of the world's entities sometimes meets an entity whose mesh was already
// freed by streaming, and crashes (GitHub issue #2099, the crashes during races). Such an entity is skipped like the
// ones the job skips on its own.
namespace
{
    bool HasMeshData(uintptr_t entity)
    {
        __try
        {
            auto mesh = *(uintptr_t*)(entity + 0x50);
            if (mesh < 0x10000)
                return false;
            auto data = *(uintptr_t*)mesh;
            if (data < 0x10000)
                return false;
            auto info = *(uintptr_t*)(data + 0x10);
            if (info < 0x10000)
                return false;
            volatile auto flags = *(uint32_t*)(info + 4);
            (void)flags;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
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
            if (!pattern.empty())
            {
                static auto skipEntity = injector::GetBranchDestination(pattern.get_first()).as_int();
                static auto BuildEntityPrimitivesHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
                {
                    if (!HasMeshData(regs.edi))
                        regs.eip = skipEntity;
                });
            }
        };
    }
} Fixes;
