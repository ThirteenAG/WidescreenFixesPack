module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module LightingFixMirror;

import ComVars;

// The rear-view mirror. The game draws its road with the low detail technique and no reflection texture. With the
// console lighting it uses the main view's road technique; the reflection stays off there.
namespace RearViewRoad
{
    constexpr int MirrorView = 3;
    constexpr int ReflectionTextureParam = 89;      // MISCMAP1_TEXTURE
    constexpr int SurfaceReflectionParam = 128;     // cfSurfaceReflection

    void* (__thiscall* SetTexture)(void* shader, int param, void* texture) = nullptr;
    void* (__thiscall* SetValue)(void* shader, int param, uint32_t value) = nullptr;
    void** ReflectionTexture = nullptr;
}

class LightingFixMirror
{
public:
    LightingFixMirror()
    {
        WFP::onInitEventAsync() += []()
        {
            if (!FixSettings().Bool("GRAPHICS.ConsoleLighting"))
                return;

            // The lighting values of the Xbox 360 version
            {
                uint32_t* dword_7497B9 = hook::pattern("8B 0D ? ? ? ? 83 F8 06 89 0D ? ? ? ? C7 05 ? ? ? ? CD CC CC 3E").count(2).get(1).get<uint32_t>(2);
                static float f20 = 2.0f;
                injector::WriteMemory((uint32_t)dword_7497B9, &f20, true);
                static float f19 = 1.9f;
                uint32_t* dword_748A70 = hook::pattern("A1 ? ? ? ? A3 ? ? ? ? 83 3D ? ? ? ? 06 C7 05 ? ? ? ? CD CC CC 3E").get(1).get<uint32_t>(1); // the second of three
                injector::WriteMemory(dword_748A70, &f19, true);
                uint32_t* dword_73E7CB = *hook::pattern("C7 05 ? ? ? ? CD CC CC 3E B8 ? ? ? ? 74 05 B8 ? ? ? ? C3").count(1).get(0).get<uint32_t*>(11);
                injector::WriteMemory<float>(dword_73E7CB, 1.35f, true);
            }

            // The mirror pass sets the low detail flag for its draw; it's left off
            auto mirrorPass = hook::pattern("6A 00 6A 01 68 ? ? ? ? C7 05 ? ? ? ? 01 00 00 00 C7 05 ? ? ? ? 00 00 80 3F E8");
            injector::WriteMemory<uint32_t>(mirrorPass.get_first(15), 0, true);

            // The road shader's per-view parameters: in the mirror, the main view's reflection texture with no reflection
            auto roadParams = hook::pattern("83 7F 08 03 8B CE 74 08 A1 ? ? ? ? 50 EB 02 6A 00 6A 59 E8 ? ? ? ? 8B 86 ? ? ? ? 85 C0 74 ? 8B 0D ? ? ? ? 51 6A 78 8B CE E8");
            RearViewRoad::ReflectionTexture = *roadParams.get_first<void**>(9);
            RearViewRoad::SetTexture = reinterpret_cast<decltype(RearViewRoad::SetTexture)>(injector::GetBranchDestination(roadParams.get_first(20)).as_int());
            RearViewRoad::SetValue = reinterpret_cast<decltype(RearViewRoad::SetValue)>(injector::GetBranchDestination(roadParams.get_first(46)).as_int());
            static auto RoadParamsHook = safetyhook::create_mid(roadParams.get_first(51), [](SafetyHookContext& regs)
            {
                auto view = reinterpret_cast<const int*>(regs.edi);
                if (view[2] != RearViewRoad::MirrorView)
                    return;
                auto shader = reinterpret_cast<void*>(regs.esi);
                RearViewRoad::SetTexture(shader, RearViewRoad::ReflectionTextureParam, *RearViewRoad::ReflectionTexture);
                RearViewRoad::SetValue(shader, RearViewRoad::SurfaceReflectionParam, 0);
            });
        };
    }
} LightingFixMirror;
