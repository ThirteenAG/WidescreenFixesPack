module;

#include "stdafx.h"

export module World;

import Settings;

namespace
{
    SafetyHookInline compositionHook;
    float** compositionDistance = nullptr;

    void UseClassicLODs()
    {
        if (compositionDistance && *compositionDistance)
        {
            // gta.worldcomp.distmult expands both section and submap streaming
            // distances. Keep real map actors and their original object LODs
            // resident instead of hiding proxies before their maps are ready.
            // Native streaming retains its loading/fade fallback and headlight
            // ribbons; object draw distances still belong to the GTA renderer.
            // The radius covers the main maps even with native FOV scaling.
            // bNeverLoadMaps chunks have no regular-map replacement: retain
            // their proxies, street decals and native interior-area switching.
            // TConsoleVariableData<float> has game/render-thread value copies.
            auto values = *compositionDistance;
            values[0] = values[1] = 100.0f;
        }
    }
    void UpdateComposition(void* world, float timeStep, const void* position, bool force)
    {
        UseClassicLODs();
        compositionHook.ccall<void>(world, timeStep, position, force);
    }
}

class WorldModule
{
public:
    WorldModule()
    {
        WFP::onInitEvent() += []()
        {
            if (!Settings.classicLODsOnly)
                return;
            auto address = hook::pattern("48 8B 05 ? ? ? ? F3 41 0F 10 84 24 30 01 00 00 F3 44 0F 10 15 ? ? ? ? F3 0F 59 C0 4C 89 74 24 48 F3 44 0F 10 18 48 8B 05 ? ? ? ? 44 0F 29 AC 24 10 04 00 00").get_first<uint8_t>(43);
            auto update = hook::pattern("40 55 41 54 41 55 48 8D AC 24 50 FC FF FF 48 81 EC B0 04 00 00").get_first();
            if (address && update)
            {
                compositionDistance = reinterpret_cast<float**>(injector::ReadRelativeOffset(address).as_int());
                // Apply before native streaming, including initial level loads.
                // DE can reapply its graphics CVars after plugin initialization.
                compositionHook = safetyhook::create_inline(update, UpdateComposition);
            }
        };
    }
} WorldModuleInstance;
