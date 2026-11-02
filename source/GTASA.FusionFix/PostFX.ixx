module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"
#include <d3d9.h>
#include "automation/asi_automation_frames.h"

export module PostFX;

import Skeleton;
import Draw;
import PostFXCore;

injector::hook_back<void(__fastcall*)(void*, void*)> hbRenderMotionBlur;
void __fastcall RenderMotionBlur(void* camera, void* edx);

static SafetyHookInline SetupBackBuffer;
static void __cdecl SetupBackBufferVertex()
{
    // SilentPatch can rebuild the buffer after our video-mode hook returns,
    // while the frontend still has its constrained canvas active.
    MenuCanvas::Suspend physicalViewport;
    SetupBackBuffer.unsafe_ccall();
}

// CPostEffects::SpeedFX, the speed overlay: skipped while Speed Blur is off
injector::hook_back<void(__cdecl*)(float)> hbSpeedFX;
static void __cdecl SpeedFX(float speed)
{
    static auto& bDisableSpeedBlur = FixSettings().Int("GRAPHICS.DisableSpeedBlur");
    if (!bDisableSpeedBlur)
        hbSpeedFX.fun(speed);
}

// Window resizing can replace the surface outside the native video-mode hooks.
static void RefreshPostFXSize(IDirect3DDevice9* device)
{
    static IDirect3DDevice9* previousDevice = nullptr;
    static D3DSURFACE_DESC previous{};
    IDirect3DSurface9* surface = nullptr;
    if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface)))
        return;
    D3DSURFACE_DESC current{};
    auto result = surface->GetDesc(&current);
    surface->Release();
    if (FAILED(result))
        return;
    if (previousDevice != device || previous.Width != current.Width || previous.Height != current.Height ||
        previous.Format != current.Format || previous.MultiSampleType != current.MultiSampleType ||
        previous.MultiSampleQuality != current.MultiSampleQuality)
    {
        if (previousDevice != device)
            CPostFX::ReleaseDevice();
        else
            CPostFX::Shutdown();
        CPostFX::OnDeviceReset();
        previousDevice = device;
        previous = current;
    }
}

static IDirect3DDevice9* GetDevice9()
{
    if (!pD3D9Device.is_initialized() || !*pD3D9Device) return nullptr;
    auto device = *pD3D9Device;
    RefreshPostFXSize(device);
    return device;
}

static const int* pConsoleGamma = nullptr;
static const int* pAntiAliasing = nullptr;

static void InstallConsoleGamma()
{
    static bool installed = false;
    if (installed || !*pConsoleGamma)
        return;
    installed = true;

    auto pattern = hook::pattern("E8 ? ? ? ? 8B 0D ? ? ? ? 51 E8 ? ? ? ? 8B 15 ? ? ? ? 52 E8 ? ? ? ? 83 C4 ? 83 C4");
    static auto ConsoleGammaHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        CPostFX::bConsoleGammaEnabled = *pConsoleGamma != 0;
        CPostFX::RenderGamma(GetDevice9());
    });
}

class PostFX
{
public:
    PostFX()
    {
        WFP::onInitEvent() += []()
        {
            pConsoleGamma = &FixSettings().Int("GRAPHICS.ConsoleGamma");
            pAntiAliasing = &FixSettings().Int("GRAPHICS.AntiAliasing");
            // All of them can be switched in the game's menu: SMAA is the Anti-Aliasing setting's SMAA choice
            CPostFX::bConsoleGammaEnabled = *pConsoleGamma != 0;
            CPostFX::bSmaaEnabled = *pAntiAliasing == 1;

            // Resolve the buffer initializer from CPostEffects::Update; the
            // initial setup call itself can be redirected by SilentPatch.
            auto setup = hook::pattern("A1 ? ? ? ? 85 C0 75 05 E9 ? ? ? ? C3");

                SetupBackBuffer = safetyhook::create_inline(injector::GetBranchDestination(setup.get_first(9)).as_int(), SetupBackBufferVertex);

            {
                // Skip only the speed overlay when it's off. Keep the caller's state reset
                // and all other post-processing, including SMAA, intact.
                auto speed = hook::pattern("A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 04 8A 0D ? ? ? ? 32 C0 84 C9");

                    hbSpeedFX.fun = injector::MakeCALL(speed.get_first(6), SpeedFX, true).get();
            }

            {
                auto pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? EB ? E8");
                hbRenderMotionBlur.fun = injector::MakeCALL(pattern.get_first(), RenderMotionBlur).get();
            }

            // The gamma hook sits on code that other mods (Project2DFX) look for: install it only once Console Gamma is
            // turned on
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

// After the 3D scene and its effects, before the HUD: SMAA
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
    hbRenderMotionBlur.fun(camera, edx);
    if (device && asi_automation_frames::Wanted(ASI_AUTOMATION_STAGE_BEFORE_UI))
        asi_automation_frames::Fire(ASI_AUTOMATION_STAGE_BEFORE_UI, device);
}
