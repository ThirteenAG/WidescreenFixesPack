module;

#include <stdafx.h>
#include <atomic>
#include "FixSettings.hpp"

export module Graphics;

import ComVars;

injector::hook_back<void(__cdecl*)(float a1, float a2, int a3)> hb_8330DB;
void __cdecl sub_8330DB(float a1, float a2, int a3)
{
    gVisibility.store(a1, std::memory_order_relaxed);

    if (!bDisableBlackAndWhiteFilter)
        return hb_8330DB.fun(a1, a2, a3);
}

export injector::hook_back<void(__cdecl*)(void* a1, int a2, int a3)> hb_100177B7;
export void __cdecl sub_100177B7(void* a1, int a2, int a3)
{
    if (gBlacklistIndicators.load(std::memory_order_relaxed)) //enables bloom in dark areas
    {
        if (gVisibility.load(std::memory_order_relaxed) == 1.0f)
            return hb_100177B7.fun(a1, a2, a3);
        else
            return;
    }
    return hb_100177B7.fun(a1, a2, a3);
}

// Distortion during movement (sonar)
export injector::hook_back<void(__fastcall*)(void* self, int edx)> hb_1002581C;
export void __fastcall sub_1002581C(void* self, int edx)
{
    //return hb_1002581C.fun(self, edx);
}

namespace Detail
{
    // l3d::Options: shadowed lights need a screen coverage of at least +1D0h times their own and are drawn up to +1D4h times their
    // shadow distance (Shadow Quality sets both, also the sun's cascade range). Coverage falls with the square of the distance.
    uint8_t* options = nullptr;
    float coverage = 0.0f;
    float distance = 0.0f;

    float Factor(float value)
    {
        return std::isfinite(value) ? std::clamp(value, 1.0f, 4.0f) : 1.0f;
    }

    void ApplyShadowDistance()
    {
        if (!options)
            return;
        const float factor = Factor(FixSettings().Float("GRAPHICS.ShadowDistance"));
        *reinterpret_cast<float*>(options + 0x1D0) = coverage / (factor * factor);
        *reinterpret_cast<float*>(options + 0x1D4) = distance * factor;
    }

    SafetyHookInline shSetLKPVisible = {};
    int __fastcall SetLKPVisible(uintptr_t pawn, void* edx, int32_t visible)
    {
        static const int& show = FixSettings().Int("GAMEPLAY.LastKnownPosition");
        if (!show)
            visible = 0;
        return shSetLKPVisible.fastcall<int>(pawn, edx, visible);
    }
}

// Geometry LOD (LeadD3DRender): a node uses LOD 1 or 2 when its projected size falls below l3d::Options +118h or +11Ch of the screen,
// the size falls with the square of the distance.
export void InitLODDistance()
{
    static const float& lodDistance = FixSettings().Float("GRAPHICS.LODDistance");
    auto pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "F3 0F 10 80 18 01 00 00 F3 0F 11 45 08 F3 0F 10 80 1C 01 00 00 F3 0F 11 45 FC");
    static auto LODSizeHook = safetyhook::create_mid(pattern.get_first(21), [](SafetyHookContext& regs)
    {
        const float factor = Detail::Factor(lodDistance);
        const float scale = 1.0f / (factor * factor);
        regs.xmm0.f32[0] *= scale;
        *reinterpret_cast<float*>(regs.ebp + 8) *= scale;
    });
}

export void InitGraphics()
{
    // Particle systems: emitters marked for near distances were shown only at Low Environment Detail (bullet casings, impact debris),
    // higher settings dropped them. Every setting shows them, as Low does.
    {
        auto pattern = hook::pattern("E8 ? ? ? ? 8A 80 00 01 00 00 88 45 FF E8");
        injector::WriteMemory<uint16_t>(pattern.get_first(5), 0xC032, true); // xor al, al
        injector::MakeNOP(pattern.get_first(7), 4, true);
    }

    // Video options apply (sub_78D5F1, edi = l3d::Options) after the shadow quality values are set
    {
        auto pattern = hook::pattern("F3 0F 11 87 CC 01 00 00 C6 87 68 01 00 00 01");
        static auto ShadowQualityHook = safetyhook::create_mid(pattern.get_first(8), [](SafetyHookContext& regs)
        {
            Detail::options = reinterpret_cast<uint8_t*>(regs.edi);
            Detail::coverage = *reinterpret_cast<float*>(regs.edi + 0x1D0);
            Detail::distance = *reinterpret_cast<float*>(regs.edi + 0x1D4);
            Detail::ApplyShadowDistance();
        });
        FixSettings().OnChange("GRAPHICS.ShadowDistance", Detail::ApplyShadowDistance);
    }

    // Last known position (sub_570ECA(visible), the pawn's display flag, LKP_VISIBLE / LKP_HIDDEN sequencer events): kept hidden when
    // turned off, as the game's own ForceDisable does.
    {
        auto pattern = hook::pattern("53 56 8B F1 F6 86 ? ? ? ? 10 57 74 18");
        Detail::shSetLKPVisible = safetyhook::create_inline(pattern.get_first(), Detail::SetLKPVisible);
    }

    //if (bDisableBlackAndWhiteFilter) //light and shadow
    {
        auto pattern = hook::pattern("E8 ? ? ? ? D9 05 ? ? ? ? 83 C4 0C 33 F6 56 51");
        hb_8330DB.fun = injector::MakeCALL(pattern.get_first(), sub_8330DB, true).get();
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 0C 83 3D ? ? ? ? ? 0F 8C ? ? ? ? 7F 0D 83 3D ? ? ? ? ? 0F 86");
        hb_8330DB.fun = injector::MakeCALL(pattern.get_first(), sub_8330DB, true).get();
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 0C 83 3D ? ? ? ? ? 7C 77 7F 09 83 3D ? ? ? ? ? 76 6C");
        hb_8330DB.fun = injector::MakeCALL(pattern.get_first(), sub_8330DB, true).get();
        pattern = hook::pattern("D9 5C 24 04 D9 40 08 D9 1C 24 E8 ? ? ? ? 83 C4 0C C3");
        hb_8330DB.fun = injector::MakeCALL(pattern.get_first(10), sub_8330DB, true).get();
    }
}
