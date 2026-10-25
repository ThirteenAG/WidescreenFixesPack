#pragma once
#include <cstring>
#include <d3d9.h>
#include "../Shared/AsiSettings/asi_settings.hpp"
#include "../Shared/AA/AntiAliasing.hpp"

// Temporal anti-aliasing of Cold Fear through the shared library (source/Shared/AA): TAA, DLAA and FSR native AA, the
// last two run by FusionFixHub.exe on DXVK (Graphics API Vulkan). The game is RenderWare on Direct3D 9.
//
// - Jitter: RenderWare's camera begin update (sub_6377A0) builds the view (D3DTS_VIEW) and the projection
//   (D3DTS_PROJECTION, a Direct3D projection, row vectors, w = z, standard [0, 1] depth) every time a camera starts
//   rendering, and sets them through its transform cache, which the game's vertex shaders read too. For the main camera
//   (the one the game shows, dword_820B90) a mid hook on that call moves the projection by the jitter; the other
//   cameras (shadows, reflections) don't get it. The 2D of the HUD and the menus is in screen space.
// - Depth: the driver keeps the device's depth-stencil as a plain pointer (it releases its reference at once) and sets
//   it for every camera drawing to the screen; an INTZ texture of the same size takes its place, so the scene's depth
//   can be read. The device's own comes back before every reset (IDirect3DDevice9::Reset of the game's device).
// - Resolve: before the first 2D of the frame (the HUD, the fades, the cutscene bars), else before the frame is shown;
//   LDR.
namespace ColdFearAA
{
    enum Setting
    {
        Off = 0, TAA = 1, DLAA = 2, FSR = 3
    };

    inline asi_settings::Plugin* pSettings = nullptr;
    inline const int* pAntiAliasing = nullptr;
    inline const float* pSharpness = nullptr;
    inline bool bDepthUnusable = false;         // no INTZ depth: temporal modes aren't offered

    inline IDirect3DDevice9** ppDevice = nullptr;           // dword_89C438
    inline IDirect3DSurface9** ppDepthStencil = nullptr;    // dword_89C43C, the device's depth-stencil
    inline IDirect3DSurface9** ppCurrentDepth = nullptr;    // dword_89C89C, the one set last
    inline void** ppMainCamera = nullptr;                   // dword_820B90
    inline float* pView = nullptr;                          // flt_8A2260, D3DTS_VIEW of the camera being begun
    inline float* pProjection = nullptr;                    // dword_78EC78, D3DTS_PROJECTION of the camera being begun
    inline std::function<std::pair<int32_t, int32_t>()> renderSize;

    inline IDirect3DSurface9* pOriginalDepth = nullptr;     // while the INTZ surface stands in for it
    inline IDirect3DSurface9* pIntzSurface = nullptr;
    inline bool bIntzTried = false;                         // for this device (since its last reset)
    inline bool bJittered = false;                          // the main camera got the jitter of this frame
    inline bool bRendered = false;                          // Render ran this frame
    inline bool bResetPending = false;                      // the game's last reset failed
    inline WFPAA::Jitter CurrentJitter;

    inline WFPAA::Mode ToMode(int value)
    {
        switch (value)
        {
        case TAA: return WFPAA::Mode::TAA;
        case DLAA: return WFPAA::Mode::DLAA;
        case FSR: return WFPAA::Mode::FSR;
        default: return WFPAA::Mode::Off;
        }
    }

    inline bool IsTemporal()
    {
        return pAntiAliasing && *pAntiAliasing >= TAA;
    }

    inline void SetDepthUnusable(bool bUnusable)
    {
        if (bUnusable != bDepthUnusable)
        {
            bDepthUnusable = bUnusable;
            pSettings->Refresh("GRAPHICS.AntiAliasing");
        }
    }

    // The device's own depth-stencil back, before a reset; the library lets its resources go
    inline void UninstallIntz()
    {
        auto device = ppDevice ? *ppDevice : nullptr;
        if (pOriginalDepth)
        {
            if (*ppDepthStencil == pIntzSurface)
                *ppDepthStencil = pOriginalDepth;
            *ppCurrentDepth = pOriginalDepth;
            if (device)
            {
                device->SetDepthStencilSurface(pOriginalDepth);
                // Nothing of the INTZ texture may stay bound for the reset
                for (DWORD stage = 0; stage < 16; ++stage)
                    device->SetTexture(stage, nullptr);
            }
        }
        pOriginalDepth = nullptr;
        pIntzSurface = nullptr;
        bIntzTried = false;
        WFPAA::TemporalAA::OnLostDevice();
    }

    // IDirect3DDevice9::Reset of the game's device
    inline HRESULT(__stdcall* ResetOriginal)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*) = nullptr;
    inline HRESULT __stdcall Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* parameters)
    {
        UninstallIntz();
        auto hr = ResetOriginal(device, parameters);
        // A failed reset (Alt+Tab in full screen, the window not back yet): the game goes on without another, and
        // putting the INTZ texture back in then makes it crash. The anti-aliasing stays off until a reset succeeds.
        if (FAILED(hr) && !bResetPending && IsTemporal())
            WFPAA::TemporalAA::Log("Device reset failed: anti-aliasing off until the next reset");
        bResetPending = FAILED(hr);
        return hr;
    }

    inline void HookReset(IDirect3DDevice9* device)
    {
        static void** pHooked = nullptr;
        auto vtable = *(void***)device;
        if (vtable == pHooked)
            return;
        pHooked = vtable;
        DWORD protect;
        VirtualProtect(&vtable[16], sizeof(void*), PAGE_EXECUTE_READWRITE, &protect);
        if (vtable[16] != (void*)&Reset)
        {
            ResetOriginal = (decltype(ResetOriginal))vtable[16];
            vtable[16] = (void*)&Reset;
        }
        VirtualProtect(&vtable[16], sizeof(void*), protect, &protect);
    }

    // The game created a device (at start, and again after a failed reset when the window wasn't shown yet):
    // everything of the library belonged to the old one
    inline void OnDeviceCreated()
    {
        if (ppDevice && *ppDevice && IsTemporal())
            HookReset(*ppDevice);
        static bool bHadDevice = false;
        if (!bHadDevice)
        {
            bHadDevice = true;
            return;
        }
        WFPAA::TemporalAA::OnDeviceReleased();
        pOriginalDepth = nullptr;
        pIntzSurface = nullptr;
        bIntzTried = false;
        bResetPending = false;
    }

    // The driver's depth-stencil becomes an INTZ texture of the same size, once per device and reset, while a temporal
    // mode is chosen. The driver sets it for the camera that's beginning.
    inline void InstallIntz(IDirect3DDevice9* device)
    {
        auto depth = *ppDepthStencil;
        if (!device || !depth || bResetPending || bIntzTried || depth == pIntzSurface)
            return;
        // Not while the device is lost (Alt+Tab in full screen): it's tried again after the reset
        HookReset(device);
        if (FAILED(device->TestCooperativeLevel()))
            return;
        bIntzTried = true;
        D3DSURFACE_DESC desc{};
        if (FAILED(depth->GetDesc(&desc)))
            return;
        auto surface = WFPAA::TemporalAA::ReplaceDepthStencil(device, depth);
        if (!surface)
        {
            // Multisampled or no INTZ: for good
            SetDepthUnusable(desc.MultiSampleType != D3DMULTISAMPLE_NONE || !WFPAA::TemporalAA::IntzSupported(device));
            return;
        }
        // The texture keeps its surface; the driver holds no reference, as with the original
        surface->Release();
        pOriginalDepth = depth;
        pIntzSurface = surface;
        *ppDepthStencil = surface;
        SetDepthUnusable(false);
    }

    // The camera begin update's D3DTS_PROJECTION, about to be set (ebx: the camera)
    inline void OnProjection(void* pCamera)
    {
        auto device = *ppDevice;
        if (!device || !ppMainCamera || pCamera != *ppMainCamera || !IsTemporal())
            return;
        InstallIntz(device);
        if (bDepthUnusable || !pIntzSurface)
            return;

        auto [width, height] = renderSize();
        if (width <= 0 || height <= 0)
            return;
        if (!bJittered)
        {
            WFPAA::Camera camera;
            camera.View = WFPAA::Matrix::From(pView);
            camera.Projection = WFPAA::Matrix::From(pProjection);
            camera.Width = width;
            camera.Height = height;
            CurrentJitter = WFPAA::TemporalAA::OnCamera(camera);
            bJittered = true;
        }
        // Begun again in the same frame: the same jitter
        WFPAA::JitterProjection(pProjection, CurrentJitter, float(width), float(height));
    }

    // Once per frame, before its first 2D or before it's shown. Off does nothing here.
    inline void Render()
    {
        if (bRendered || !IsTemporal() || !ppDevice || !*ppDevice || bResetPending)
            return;
        bRendered = true;
        auto device = *ppDevice;

        static uint32_t generation = 0;
        if (generation != WFPAA::TemporalAA::Generation())
        {
            generation = WFPAA::TemporalAA::Generation();
            pSettings->Refresh("GRAPHICS.AntiAliasing");
        }

        WFPAA::TemporalAA::SetMode(ToMode(*pAntiAliasing), *pSharpness / 100.0f);
        IDirect3DSurface9* scene = nullptr;
        device->GetRenderTarget(0, &scene);
        auto depth = IsTemporal() && bJittered ? WFPAA::TemporalAA::DepthTexture() : nullptr;
        WFPAA::TemporalAA::Render(device, scene, depth, scene);
        if (scene)
            scene->Release();
    }

    // Before the frame is shown (RwCameraShowRaster of the main camera)
    inline void EndFrame()
    {
        // Switched off: the device's own depth-stencil back
        if (!IsTemporal() && pOriginalDepth)
            UninstallIntz();
        Render();
        bRendered = false;
        bJittered = false;
    }

    // From the plugin's init: before the game creates its Direct3D object
    inline void Init(asi_settings::Plugin& settings, const char* clientName)
    {
        pSettings = &settings;
        pAntiAliasing = &settings.Int("GRAPHICS.AntiAliasing");
        pSharpness = &settings.Float("GRAPHICS.Sharpness");

        auto name = GetThisModuleName<std::wstring>();
        name = name.substr(0, name.find_last_of(L'.'));
        WFPAA::Options options;
        options.BridgeName = L"Local\\" + name + L".Upscaler";
        options.ClientName = clientName;
        options.LowDynamicRange = true;
        WFPAA::TemporalAA::Configure(options);

        // Graphics API: Vulkan is offered once the hub installed DXVK (vulkan.dll in the game folder), D3D9on12 only
        // with GRAPHICS.D3D9on12 = 1 in the file
        static bool allowD3D9On12 = settings.Bool("GRAPHICS.D3D9on12");
        settings.Availability("MAIN.GraphicsAPI", [](std::string_view value)
        {
            if (value == "1")
                return WFPAA::VulkanAvailable();
            if (value == "2")
                return allowD3D9On12;
            return true;
        });
        auto api = WFPAA::InstallGraphicsApi(settings.Int("MAIN.GraphicsAPI"), allowD3D9On12);
        if (IsTemporal())
            WFPAA::TemporalAA::Log(api == WFPAA::GraphicsApi::Vulkan ? "Graphics API: Vulkan (DXVK)" : api == WFPAA::GraphicsApi::D3D9On12 ? "Graphics API: D3D9on12" : "Graphics API: Direct3D 9");

        // DLAA and FSR when they can run (the hub starts with the first DLAA or FSR frame), TAA unless the depth can't be
        // read
        settings.Availability("GRAPHICS.AntiAliasing", [](std::string_view value)
        {
            if (value == "1")
                return !bDepthUnusable;
            if (value == "2")
                return !bDepthUnusable && WFPAA::TemporalAA::IsAvailable(WFPAA::Backend::DLSS);
            if (value == "3")
                return !bDepthUnusable && WFPAA::TemporalAA::IsAvailable(WFPAA::Backend::FSR);
            return true;
        });
    }
}
