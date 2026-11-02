module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include "DeveloperOverlay.hpp"
#include "automation/asi_automation_frames.h"

export module PostFX;

import PostFXCore;

// ConsoleGamma and SMAA, on the game view before the HUD draws over it. The game renders with
// Direct3D 8, which d3d8to9 turns into Direct3D 9, and the device it hands the game gives out the
// Direct3D 9 one it forwards to. Without d3d8to9 there's none and nothing gets drawn.
namespace P_D3D
{
    enum
    {
        DEVICE = 0xD0, // IDirect3DDevice8*
    };

    // Of the current video mode, refreshed whenever the driver begins a scene
    IDirect3DDevice9* pDevice = nullptr;

    IDirect3DDevice9* GetDevice9(uint8_t* _this)
    {
        auto pDevice8 = *(IUnknown**)(_this + DEVICE);
        IDirect3DDevice9* pDevice9 = nullptr;
        if (!pDevice8 || FAILED(pDevice8->QueryInterface(__uuidof(IDirect3DDevice9), (void**)&pDevice9)) || !pDevice9)
            return nullptr;

        pDevice9->Release();
        return pDevice9;
    }

    // A lost device gets reset at the start of the next scene, which fails while default pool
    // resources are left
    SafetyHookInline shBeginScene = {};
    void __fastcall DRV_beginScene(uint8_t* _this, void* edx)
    {
        auto pDevice9 = GetDevice9(_this);
        if (pDevice9 != pDevice)
        {
            MaxPayneOverlay::Release();
            CPostFX::ReleaseDevice();
            pDevice = pDevice9;
        }

        bool bReset = pDevice && pDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET;
        if (bReset)
        {
            MaxPayneOverlay::BeforeReset();
            CPostFX::Shutdown();
            CPostFX::OnDeviceReset();
        }

        shBeginScene.unsafe_fastcall(_this, edx);
    }

    // Video mode changes and quitting release the device
    SafetyHookInline shFreeMode = {};
    void __fastcall DRV_freeMode(uint8_t* _this, void* edx)
    {
        MaxPayneOverlay::Release();
        CPostFX::ReleaseDevice();
        pDevice = nullptr;
        shFreeMode.unsafe_fastcall(_this, edx);
    }
}

export void PrepareVideoModeChange()
{
    // The native mode switch can release or reset the device without calling the exported freeMode wrapper.
    MaxPayneOverlay::Release();
    CPostFX::ReleaseDevice();
    P_D3D::pDevice = nullptr;
}

export int RegisterAutomationFrameCallback(int stage, AsiAutomationFrameCallback callback, void* context)
{
    return asi_automation_frames::Set(stage, callback, context);
}

export void RenderDeveloperOverlay()
{
    MaxPayneOverlay::Draw(P_D3D::pDevice);
}

export HWND GetRenderWindow()
{
    D3DDEVICE_CREATION_PARAMETERS parameters{};
    if (P_D3D::pDevice && SUCCEEDED(P_D3D::pDevice->GetCreationParameters(&parameters)))
        return parameters.hFocusWindow;
    return nullptr;
}

export void RenderAutomationFrame(int stage)
{
    if (asi_automation_frames::Wanted(stage) && P_D3D::pDevice)
        asi_automation_frames::Fire(stage, P_D3D::pDevice);
}

// Both can be switched in the menu: the hooks are always in place, and the settings are read where the effects draw
const int* pConsoleGamma = nullptr;
const int* pAntiAliasing = nullptr;

void ReadPostFXSettings()
{
    CPostFX::bConsoleGammaEnabled = *pConsoleGamma != 0;
    bool bSmaa = *pAntiAliasing == 1;
    if (CPostFX::bSmaaEnabled != bSmaa)
    {
        // The SMAA textures are made while it's on: start over
        CPostFX::Shutdown();
        CPostFX::bSmaaEnabled = bSmaa;
    }
}

// At the end of MaxPayne_GameMode::renderMode: before the HUD, and before XboxRainDroplets draws its
// flakes once it returns
void RenderPostFX()
{
    auto pDevice = P_D3D::pDevice;
    if (!pDevice)
        return;


    ReadPostFXSettings();
    if (!CPostFX::bConsoleGammaEnabled && !CPostFX::bSmaaEnabled)
        return;

    // the passes cover the viewport, the one of the last camera may not be the whole screen
    D3DVIEWPORT9 viewport = {};
    bool bViewport = SUCCEEDED(pDevice->GetViewport(&viewport));
    IDirect3DSurface9* pRenderTarget = nullptr;
    if (SUCCEEDED(pDevice->GetRenderTarget(0, &pRenderTarget)) && pRenderTarget)
    {
        D3DSURFACE_DESC desc = {};
        if (SUCCEEDED(pRenderTarget->GetDesc(&desc)))
        {
            D3DVIEWPORT9 screen = { 0, 0, desc.Width, desc.Height, 0.0f, 1.0f };
            pDevice->SetViewport(&screen);
        }
        pRenderTarget->Release();
    }

    // the game view is done with its scene by now
    bool bScene = SUCCEEDED(pDevice->BeginScene());
    CPostFX::RenderSMAA(pDevice);
    CPostFX::RenderGamma(pDevice);
    if (bScene)
        pDevice->EndScene();

    if (bViewport)
        pDevice->SetViewport(&viewport);
}

// X_SceneDump::endFrame, last after the scenes of the game view are rendered
injector::hook_back<void(__fastcall*)(void*, void*)> hbEndSceneDumpFrame;
void __fastcall EndSceneDumpFrame(void* _this, void* edx)
{
    hbEndSceneDumpFrame.fun(_this, edx);
    RenderPostFX();
    RenderAutomationFrame(ASI_AUTOMATION_STAGE_BEFORE_UI);
}

void InitPostFX()
{
    pConsoleGamma = &FixSettings().Int("GRAPHICS.ConsoleGamma");
    pAntiAliasing = &FixSettings().Int("GRAPHICS.AntiAliasing");
    ReadPostFXSettings();

    auto pattern = hook::pattern("8B 8E 74 10 00 00 E8 ? ? ? ? 8B 4C 24 ? E8 ? ? ? ? 5F 5D"); // MaxPayne_GameMode::renderMode
    hbEndSceneDumpFrame.fun = injector::MakeCALL(pattern.get_first(15), EndSceneDumpFrame, true).get(); //0x44EDEE
}

// e2_d3d8_driver_mfc gets loaded again for video mode changes
export void InitPostFXDriver()
{
    auto e2_d3d8_driver_mfc = GetModuleHandle(L"e2_d3d8_driver_mfc");
    auto beginScene = hook::module_pattern(e2_d3d8_driver_mfc, "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 1C 53 56 8B F1 8B 86 D0 00 00 00 8B 08 57");
    auto freeMode = hook::module_pattern(e2_d3d8_driver_mfc, "56 8B F1 E8 ? ? ? ? 8B 86 D0 00 00 00 85 C0 74 2A 8B 08 50 FF 51 08 85 C0 74 16 50 68 ? ? ? ? FF 15");
    P_D3D::shBeginScene = safetyhook::create_inline(beginScene.get_first(), P_D3D::DRV_beginScene);
    P_D3D::shFreeMode = safetyhook::create_inline(freeMode.get_first(), P_D3D::DRV_freeMode);
}

export void ShutdownPostFXDriver()
{
    MaxPayneOverlay::Release();
    CPostFX::ReleaseDevice();
    P_D3D::pDevice = nullptr;
    P_D3D::shBeginScene.reset();
    P_D3D::shFreeMode.reset();
}

class PostFX
{
public:
    PostFX()
    {
        WFP::onInitEvent() += []()
        {
            InitPostFX();
        };
    }
} PostFX;
