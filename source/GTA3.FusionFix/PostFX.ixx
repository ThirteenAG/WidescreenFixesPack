module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"
#include <d3d9.h>
#include "automation/asi_automation_frames.h"

export module PostFX;

import Skeleton;
import Camera;
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

static int(__cdecl* OpenMotionBlur)(RwCamera*) = nullptr;
static RwCamera* BlurCamera = nullptr;
static int BlurWidth = 0;
static int BlurHeight = 0;
static GameRef<uint8_t> pBlurOn;
static GameRef<void*> pBlurFrontBuffer;

static void RefreshMotionBlur(void* owner)
{
    auto camera = static_cast<RwCamera*>(static_cast<CCamera*>(owner)->m_pRwCamera);
    if (!camera || !camera->frameBuffer)
        return;
    auto raster = camera->frameBuffer;
    if (raster->width <= 0 || raster->height <= 0)
        return;
    // Restore Defaults on Display Setup turns trails on without making their buffer (a crash when they were off)
    if (BlurCamera != camera || BlurWidth != raster->width || BlurHeight != raster->height || (*pBlurOn && !*pBlurFrontBuffer))
    {
        // The native resize path can rebuild the quad before resizing the raster.
        // Recreate it with the current raster, including the optional trails texture.
        OpenMotionBlur(camera);
        BlurCamera = camera;
        BlurWidth = raster->width;
        BlurHeight = raster->height;
    }
}

static const int* pConsoleGamma = nullptr;
static const int* pAntiAliasing = nullptr;

static void InstallConsoleGamma()
{
    static bool installed = false;
    if (installed || !*pConsoleGamma)
        return;
    installed = true;

    auto pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? 80 3D ? ? ? ? ? 74 ? E8 ? ? ? ? 83 C4");
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
            // Both can be switched in the game's menu: SMAA is the Anti-Aliasing setting's SMAA choice
            CPostFX::bConsoleGammaEnabled = *pConsoleGamma != 0;
            CPostFX::bSmaaEnabled = *pAntiAliasing == 1;

            auto pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E9 ? ? ? ? 8D 44 20");
            hbRenderMotionBlur.fun = injector::MakeCALL(pattern.get_first(), RenderMotionBlur).get();

            pattern = hook::pattern("8B 83 A0 07 00 00 50 E8 ? ? ? ? 59 5B C2 04 00");
            OpenMotionBlur = reinterpret_cast<decltype(OpenMotionBlur)>(injector::GetBranchDestination(pattern.get_first(7)).as_int());

            pattern = hook::pattern("80 3D ? ? ? ? 00 8B 6C 24 ? 8B 5C 24 ? 8B 7C 24 ? 8B 54 24 ? 8B 44 24 ? 8B 4C 24 ? 74 ? 83 3D ? ? ? ? 00");
            pBlurOn.SetAddress(*pattern.get_first<uint8_t*>(2));
            pBlurFrontBuffer.SetAddress(*pattern.get_first<void**>(35));

            // The gamma hook sits on code that other mods (Project2DFX) look for: install it only once Console Gamma is
            // turned on
            InstallConsoleGamma();
            OnSettingChange("GRAPHICS.ConsoleGamma", []() { InstallConsoleGamma(); });

            WFP::onBeforeReset() += []()
            {
                CPostFX::Shutdown();
                CPostFX::OnDeviceReset();
                BlurCamera = nullptr;
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
    RefreshMotionBlur(camera);
    auto device = GetDevice9();
    CPostFX::RenderSMAA(device);
    hbRenderMotionBlur.fun(camera, edx);
    if (device && asi_automation_frames::Wanted(ASI_AUTOMATION_STAGE_BEFORE_UI))
        asi_automation_frames::Fire(ASI_AUTOMATION_STAGE_BEFORE_UI, device);
}