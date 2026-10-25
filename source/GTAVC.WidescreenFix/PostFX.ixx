module;

#include <stdafx.h>
#include "../Shared/CheckedPattern.hpp"
#include "common.h"
#include "FixSettings.hpp"
#include <d3d9.h>

export module PostFX;

import Skeleton;
import PostFXCore;
import AntiAliasing;

injector::hook_back<void(__fastcall*)(void*, void*)> hbRenderMotionBlur;
void __fastcall RenderMotionBlur(void* camera, void* edx);

static IDirect3DDevice9* GetDevice9()
{
    if (!pD3D8Device || !*pD3D8Device) return nullptr;

    IDirect3DDevice9* dev9 = nullptr;
    if (FAILED((*pD3D8Device)->QueryInterface(__uuidof(IDirect3DDevice9), (void**)&dev9)))
        return nullptr;

    dev9->Release();
    return dev9;
}

static const int* pConsoleGamma = &FixSettings().Int("GRAPHICS.ConsoleGamma");
static const int* pAntiAliasing = &FixSettings().Int("GRAPHICS.AntiAliasing");

static void InstallConsoleGamma()
{
    static bool installed = false;
    if (installed || !*pConsoleGamma)
        return;
    installed = true;

    auto pattern = hook::pattern("A1 ? ? ? ? 50 E8 ? ? ? ? A1 ? ? ? ? 59 50 E8 ? ? ? ? 80 3D");
    if (pattern.empty())
        return;
    static auto ConsoleGammaHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        CPostFX::bConsoleGammaEnabled = *pConsoleGamma != 0;
        CPostFX::RenderGamma(GetDevice9());
    });

    // SwitchToNewScreen presents two frames through
    // DoRWStuffEndOfFrame instead of Idle. Apply gamma after its
    // final draws too, or the transparent backdrop flashes brighter
    // during every page switch. Keep the native text crossfade.
    pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? A1 ? ? ? ? 59 50 E8 ? ? ? ? 59 C3");
    if (pattern.size() == 1)
    {
        static auto TransitionGammaHook = safetyhook::create_mid(pattern.get_first(10), [](SafetyHookContext&)
        {
            CPostFX::RenderGamma(GetDevice9());
        });
    }
}

class PostFX
{
public:
    PostFX()
    {
        WFP::onInitEvent() += []()
        {
            // Both can be switched in the game's menu: SMAA is the Anti-Aliasing setting's SMAA choice
            CPostFX::bConsoleGammaEnabled = *pConsoleGamma != 0;
            CPostFX::bSmaaEnabled = *pAntiAliasing == 1;

            {
                auto pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E9 ? ? ? ? E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? DF E0");
                if (pattern.size() < 1)
                {
                    wfp::patterns::Report("GTAVC.WidescreenFix: feature signature unavailable.\n");
                    return;
                }
                hbRenderMotionBlur.fun = injector::MakeCALL(pattern.get_first(), RenderMotionBlur).get();
            }

            // The gamma hooks sit on code that other mods (Project2DFX) look for: install them only once Console
            // Gamma is turned on
            InstallConsoleGamma();
            OnSettingChange("GRAPHICS.ConsoleGamma", []() { InstallConsoleGamma(); });

            WFP::onBeforeReset() += []()
            {
                CPostFX::Shutdown();
                CPostFX::OnDeviceReset();
            };
        };
    }
} PostFX;

// After the 3D scene and its effects, before the HUD: SMAA, or temporal anti-aliasing
void __fastcall RenderMotionBlur(void* camera, void* edx)
{
    if (CPostFX::bSmaaEnabled != (*pAntiAliasing == 1))
    {
        // The SMAA textures are made when it's on: start over
        CPostFX::Shutdown();
        CPostFX::bSmaaEnabled = *pAntiAliasing == 1;
    }
    auto device = GetDevice9();
    CPostFX::RenderSMAA(device);
    AntiAliasing::Render(device);
    hbRenderMotionBlur.fun(camera, edx);
}
