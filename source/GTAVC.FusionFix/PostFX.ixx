module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"
#include <d3d9.h>
#include "automation/asi_automation_frames.h"

export module PostFX;

import Skeleton;
import PostFXCore;

injector::hook_back<void(__fastcall*)(void*, void*)> hbRenderMotionBlur;
void __fastcall RenderMotionBlur(void* camera, void* edx);

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
    if (!pD3D8Device.is_initialized() || !*pD3D8Device) return nullptr;

    IDirect3DDevice9* dev9 = nullptr;
    if (FAILED((*pD3D8Device)->QueryInterface(__uuidof(IDirect3DDevice9), (void**)&dev9)))
        return nullptr;

    RefreshPostFXSize(dev9);
    dev9->Release();
    return dev9;
}

static std::atomic_bool consoleGammaEnabled = false;
static std::atomic_bool smaaEnabled = true;

static void InstallConsoleGamma()
{
    static bool installed = false;
    if (installed || !consoleGammaEnabled.load())
        return;
    installed = true;

    auto pattern = hook::pattern("A1 ? ? ? ? 50 E8 ? ? ? ? A1 ? ? ? ? 59 50 E8 ? ? ? ? 80 3D");
    static auto ConsoleGammaHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        CPostFX::bConsoleGammaEnabled = consoleGammaEnabled.load();
        CPostFX::RenderGamma(GetDevice9());
    });

    // SwitchToNewScreen presents two frames through
    // DoRWStuffEndOfFrame instead of Idle. Apply gamma after its
    // final draws too, or the transparent backdrop flashes brighter
    // during every page switch. Keep the native text crossfade.
    pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? A1 ? ? ? ? 59 50 E8 ? ? ? ? 59 C3");

    {
        static auto TransitionGammaHook = safetyhook::create_mid(pattern.get_first(10), [](SafetyHookContext&)
        {
            CPostFX::bConsoleGammaEnabled = consoleGammaEnabled.load();
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
            consoleGammaEnabled = ReadFixSetting("GRAPHICS.ConsoleGamma", 0) != 0;
            const auto updateSmaa = []() { smaaEnabled = ReadFixSetting("GRAPHICS.AntiAliasing", 1) == 1; };
            updateSmaa();
            CPostFX::bConsoleGammaEnabled = consoleGammaEnabled.load();
            CPostFX::bSmaaEnabled = smaaEnabled.load();
            OnSettingChange("GRAPHICS.AntiAliasing", updateSmaa);

            {
                auto pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E9 ? ? ? ? E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? DF E0");
                hbRenderMotionBlur.fun = injector::MakeCALL(pattern.get_first(), RenderMotionBlur).get();
            }

            // The gamma hooks sit on code that other mods (Project2DFX) look for: install them only once Console
            // Gamma is turned on
            InstallConsoleGamma();
            OnSettingChange("GRAPHICS.ConsoleGamma", []()
            {
                consoleGammaEnabled = ReadFixSetting("GRAPHICS.ConsoleGamma", 0) != 0;
                InstallConsoleGamma();
            });

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
    const bool enableSmaa = smaaEnabled.load();
    if (CPostFX::bSmaaEnabled != enableSmaa)
    {
        // The SMAA textures are made when it's on: start over
        CPostFX::Shutdown();
        CPostFX::bSmaaEnabled = enableSmaa;
    }
    auto device = GetDevice9();
    CPostFX::RenderSMAA(device);
    hbRenderMotionBlur.fun(camera, edx);
    if (device && asi_automation_frames::Wanted(ASI_AUTOMATION_STAGE_BEFORE_UI))
        asi_automation_frames::Fire(ASI_AUTOMATION_STAGE_BEFORE_UI, device);
}
