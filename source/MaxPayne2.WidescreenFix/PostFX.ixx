module;

#include <stdafx.h>
#include <d3d9.h>

export module PostFX;

import PostFXCore;

// ConsoleGamma and SMAA, on the game view before the HUD draws over it. The game renders with
// Direct3D 8, which d3d8to9 turns into Direct3D 9, and the device it hands the game gives out the
// Direct3D 9 one it forwards to. Without d3d8to9 there's none and nothing gets drawn.
namespace P_D3D
{
    enum
    {
        DEVICE = 0x100, // IDirect3DDevice8*
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
            CPostFX::ReleaseDevice();
            pDevice = pDevice9;
        }

        if (pDevice && pDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
        {
            CPostFX::Shutdown();
            CPostFX::OnDeviceReset();
        }

        shBeginScene.unsafe_fastcall(_this, edx);
    }

    // Video mode changes and quitting release the device
    SafetyHookInline shFreeMode = {};
    void __fastcall DRV_freeMode(uint8_t* _this, void* edx)
    {
        CPostFX::ReleaseDevice();
        pDevice = nullptr;
        shFreeMode.unsafe_fastcall(_this, edx);
    }
}

bool IsPostFXEnabled()
{
    static bool bEnabled = []
    {
        CIniReader iniReader("");
        CPostFX::bConsoleGammaEnabled = iniReader.ReadInteger("GRAPHICS", "ConsoleGamma", 0) != 0;
        CPostFX::bSmaaEnabled = iniReader.ReadInteger("GRAPHICS", "SMAA", 0) != 0;
        return CPostFX::bConsoleGammaEnabled || CPostFX::bSmaaEnabled;
    }();
    return bEnabled;
}

// At the end of MP_GameMode::renderMode: before the HUD, and before XboxRainDroplets draws its drops
// once it returns
void RenderPostFX()
{
    auto pDevice = P_D3D::pDevice;
    if (!pDevice)
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

// The post-processing of pain and bullet time ends the game view, drawing it to the screen when it's on
injector::hook_back<void(__fastcall*)(void*, void*)> hbEndPostProcessing;
void __fastcall EndPostProcessing(void* _this, void* edx)
{
    hbEndPostProcessing.fun(_this, edx);
    RenderPostFX();
}

export void InitPostFX()
{
    if (!IsPostFXEnabled())
        return;

    auto pattern = hook::pattern("8B C8 FF 52 10 8B 8E ? ? ? ? E8 ? ? ? ? 5D"); // MP_GameMode::renderMode
    hbEndPostProcessing.fun = injector::MakeCALL(pattern.get_first(11), EndPostProcessing, true).get(); //0x451EB6
}

// e2_d3d8_driver_mfc gets loaded again for video mode changes
export void InitPostFXDriver()
{
    if (!IsPostFXEnabled())
        return;

    auto e2_d3d8_driver_mfc = GetModuleHandle(L"e2_d3d8_driver_mfc");
    P_D3D::shBeginScene = safetyhook::create_inline(GetProcAddress(e2_d3d8_driver_mfc, "?DRV_beginScene@P_D3D@@UAEXXZ"), P_D3D::DRV_beginScene);
    P_D3D::shFreeMode = safetyhook::create_inline(GetProcAddress(e2_d3d8_driver_mfc, "?DRV_freeMode@P_D3D@@UAEXXZ"), P_D3D::DRV_freeMode);
}

export void ShutdownPostFXDriver()
{
    CPostFX::ReleaseDevice();
    P_D3D::pDevice = nullptr;
    P_D3D::shBeginScene.reset();
    P_D3D::shFreeMode.reset();
}
