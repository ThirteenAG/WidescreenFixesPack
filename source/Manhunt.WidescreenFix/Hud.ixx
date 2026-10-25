module;

#include <stdafx.h>
#include "../Shared/CheckedPattern.hpp"
#include "common.h"

export module Hud;

import Renderer;

// Which 2D layer each part of the game draws in (see Renderer). Anything not listed here is in the Box layer: menus,
// loading and legal screens, the frontend.
namespace
{
    SafetyHookInline shSceneRender = {};
    void __cdecl SceneRender(void* camera, int32_t flag)
    {
        // The world and its screen-space sprites; the HUD inside it sets its own layer
        Scope2D scope(Layer2D::Projected);
        shSceneRender.unsafe_ccall(camera, flag);
    }

    SafetyHookInline shRenderFrontend = {};
    void __cdecl RenderFrontend(void* camera, int32_t flag)
    {
        Scope2D scope(Layer2D::Box);
        shRenderFrontend.unsafe_ccall(camera, flag);
    }

    SafetyHookInline shRenderTapeInfo = {};
    void __fastcall RenderTapeInfo(void* player, void*, void* camera)
    {
        // The camcorder's REC and time code, at the corners of the picture
        Scope2D scope(Layer2D::Box, Anchor::Auto);
        shRenderTapeInfo.unsafe_thiscall(player, camera);
    }

    SafetyHookInline shTextOverlayRender = {};
    void __fastcall TextOverlayRender(void* overlay, void*)
    {
        // A text box keeps its side of the screen as a whole
        auto base = static_cast<uint8_t*>(overlay);
        const float left = *reinterpret_cast<float*>(base + 60);
        const float width = *reinterpret_cast<float*>(base + 36);
        const float center = left + width * 0.5f;
        const Anchor anchor = center < 1.0f / 3.0f ? Anchor::Left : center > 2.0f / 3.0f ? Anchor::Right : Anchor::Center;
        Scope2D scope(Layer2D::Box, anchor);
        shTextOverlayRender.unsafe_thiscall(overlay);
    }

    SafetyHookInline shRenderScreenFX = {};
    void __cdecl RenderScreenFX(void* camera)
    {
        // Night vision, drug and hit effects
        Scope2D scope(Layer2D::Stretch);
        shRenderScreenFX.unsafe_ccall(camera);
    }

    SafetyHookInline shRenderVideoFX = {};
    int32_t __cdecl RenderVideoFX()
    {
        // Camcorder noise across the picture
        Scope2D scope(Layer2D::Stretch);
        return shRenderVideoFX.unsafe_ccall<int32_t>();
    }

    SafetyHookInline shRenderFuzz = {};
    uint8_t __cdecl RenderFuzz()
    {
        Scope2D scope(Layer2D::Stretch);
        return shRenderFuzz.unsafe_ccall<uint8_t>();
    }

    SafetyHookInline shLightFXLevelStartInit = {};
    void __cdecl LightFXLevelStartInit()
    {
        // Light glows keep the sizes made here for the whole level
        Scope2D scope(Layer2D::Projected);
        shLightFXLevelStartInit.unsafe_ccall();
    }

    SafetyHookInline shRadarDraw = {};
    void __cdecl RadarDraw()
    {
        Scope2D scope(Layer2D::Box, Anchor::Left);
        shRadarDraw.unsafe_ccall();
    }

    SafetyHookInline shTimerDraw = {};
    void __cdecl TimerDraw()
    {
        Scope2D scope(Layer2D::Box, Anchor::Right);
        shTimerDraw.unsafe_ccall();
    }

    SafetyHookInline shInventoryControl = {};
    void __cdecl InventoryControl()
    {
        Scope2D scope(Layer2D::Box, Anchor::Right);
        shInventoryControl.unsafe_ccall();
    }

    template<typename T>
    void HookFunction(SafetyHookInline& hook, std::string_view signature, T* destination)
    {
        auto pattern = hook::pattern(signature);
        if (!pattern.empty())
            hook = safetyhook::create_inline(pattern.get_first(), destination);
    }
}

class Hud
{
public:
    Hud()
    {
        WFP::onInitEvent() += []()
        {
            HookFunction(shSceneRender, "53 56 57 55 83 EC 04 83 3D ? ? ? ? 00 8B 6C 24 18", SceneRender);
            HookFunction(shRenderFrontend, "53 56 55 8B 6C 24 14 85 ED 8B 74 24 10", RenderFrontend);
            HookFunction(shRenderTapeInfo, "53 56 57 55 81 EC 48 01 00 00 80 3D", RenderTapeInfo);
            HookFunction(shTextOverlayRender, "53 56 57 55 89 CD 83 EC 2C 83 BD 90 19 00 00 00", TextOverlayRender);
            HookFunction(shRenderScreenFX, "53 56 57 55 81 EC F4 00 00 00 83 3D", RenderScreenFX);
            HookFunction(shRenderVideoFX, "53 83 EC 30 E8 ? ? ? ? 85 C0", RenderVideoFX);
            HookFunction(shRenderFuzz, "53 55 83 EC 20 8B 0D", RenderFuzz);
            HookFunction(shLightFXLevelStartInit, "53 55 83 EC 28 31 D2 31 C9", LightFXLevelStartInit);
            HookFunction(shRadarDraw, "53 83 EC 60 83 3D ? ? ? ? 15", RadarDraw);
            HookFunction(shTimerDraw, "57 B8 4C 12 00 00 E8", TimerDraw);
            HookFunction(shInventoryControl, "53 56 57 55 81 EC B8 00 00 00 83 3D", InventoryControl);

            // CFrontend::Render2d: the player's status icon, health and stamina bars sit at the right
            auto pattern = hook::pattern("D9 05 ? ? ? ? DD D1 D8 0D ? ? ? ? 8B 2D");
            if (pattern.size() < 1)
            {
                wfp::patterns::Report("Manhunt.WidescreenFix: feature signature unavailable.\n");
                return;
            }
            auto end = hook::pattern("E8 ? ? ? ? 83 3D ? ? ? ? 00 0F 84 ? ? ? ? 83 3D ? ? ? ? 00");
            if (!pattern.empty() && !end.empty())
            {
                static auto PlayerStatusBegin = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    CRenderer::SetAnchor(Anchor::Right);
                });

                static auto PlayerStatusEnd = safetyhook::create_mid(end.get_first(), [](SafetyHookContext& regs)
                {
                    CRenderer::SetAnchor(Anchor::Center);
                });
            }
        };
    }
} Hud;
