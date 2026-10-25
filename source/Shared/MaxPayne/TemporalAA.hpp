#pragma once
#include <cstring>
#include <d3d9.h>
#include "../AsiSettings/asi_settings.hpp"
#include "../AA/AntiAliasing.hpp"

// Temporal anti-aliasing of the Max Payne games through the shared library (source/Shared/AA): TAA, DLAA and FSR native
// AA, the last two run by FusionFixHub.exe on DXVK (Graphics API Vulkan). The games are Direct3D 8; all of it needs
// d3d8to9 (dxwrapper with D3d8to9 = 1), the effects draw with its Direct3D 9 device.
//
// - Graphics API: dxwrapper's d3d8to9 takes Direct3DCreate9 from the system's d3d9.dll before the fix runs; for Vulkan,
//   graphics_api.h makes the system's IDirect3D9 create the device on DXVK under it.
// - Jitter: P_Camera keeps its projection (a Direct3D projection, row vectors, w = z, standard [0, 1] depth) and its
//   view (4x3), which P_Camera::prepare computes when the camera is dirty, and the driver reads for every primitive.
//   The level camera (the one X_LevelRuntimeCamera sets up) gets the jitter added to its projection in prepare while
//   the game view renders, and its projection back once the scene is resolved: the HUD and the other cameras after it
//   don't have it.
// - Depth: the driver keeps the device's depth-stencil in P_D3D and sets it with the back buffer; an INTZ texture of
//   the same size takes its place (created through d3d8to9, which passes the format through), so the scene's depth
//   can be read. It's put back before the game resets or frees the device.
// - Resolve: at the end of the game view, before the HUD and the other effects; LDR.

namespace MaxPayneAA
{
    enum Setting
    {
        Off = 0, SMAA = 1, TAA = 2, DLAA = 3, FSR = 4
    };

    // Where the game keeps what the anti-aliasing needs
    struct Layout
    {
        ptrdiff_t cameraView;           // P_Camera: M_Matrix4x3, world -> view
        ptrdiff_t cameraProjection;     // P_Camera: M_Matrix4, row vectors
        ptrdiff_t d3dDevice;            // P_D3D: IDirect3DDevice8*
        ptrdiff_t d3dDepthStencil;      // P_D3D: IDirect3DSurface8*, set with the back buffer
    };

    inline Layout layout{};
    inline asi_settings::Plugin* pSettings = nullptr;
    inline const int* pAntiAliasing = nullptr;
    inline const float* pSharpness = nullptr;
    inline bool bDepthUnusable = false;         // no INTZ depth: temporal modes aren't offered

    inline uint8_t* pMainCamera = nullptr;      // X_LevelRuntimeCamera's
    inline bool bInGameView = false;            // between the start of the game view and its resolve
    inline bool bJittered = false;              // the main camera's projection has the jitter of this frame
    inline bool bNoJitter = false;              // the game view isn't resolved this frame (the game's own post-processing)
    inline WFPAA::Jitter CurrentJitter;
    alignas(16) inline float Unjittered[16]{};

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

    // ---------------------------------------------------------------------------------------------------------------
    // The depth-stencil, through the Direct3D 8 interfaces of d3d8to9 (by their vtable slots, d3d8.h and d3d9.h can't
    // be in one module)

    namespace D3D8
    {
        // IDirect3DDevice8
        inline HRESULT CreateTexture(IUnknown* device, UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IUnknown** texture)
        {
            using Fn = HRESULT(__stdcall*)(IUnknown*, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IUnknown**);
            return (*reinterpret_cast<Fn**>(device))[20](device, width, height, levels, usage, format, pool, texture);
        }

        inline HRESULT SetRenderTarget(IUnknown* device, IUnknown* renderTarget, IUnknown* depthStencil)
        {
            using Fn = HRESULT(__stdcall*)(IUnknown*, IUnknown*, IUnknown*);
            return (*reinterpret_cast<Fn**>(device))[31](device, renderTarget, depthStencil);
        }

        // IDirect3DTexture8
        inline HRESULT GetSurfaceLevel(IUnknown* texture, UINT level, IUnknown** surface)
        {
            using Fn = HRESULT(__stdcall*)(IUnknown*, UINT, IUnknown**);
            return (*reinterpret_cast<Fn**>(texture))[15](texture, level, surface);
        }

        // IDirect3DSurface8::GetDesc, D3DSURFACE_DESC of Direct3D 8: Format, Type, Usage, Pool, Size, MultiSampleType,
        // Width, Height
        struct SurfaceDesc
        {
            D3DFORMAT Format;
            D3DRESOURCETYPE Type;
            DWORD Usage;
            D3DPOOL Pool;
            UINT Size;
            D3DMULTISAMPLE_TYPE MultiSampleType;
            UINT Width;
            UINT Height;
        };

        inline HRESULT GetDesc(IUnknown* surface, SurfaceDesc* desc)
        {
            using Fn = HRESULT(__stdcall*)(IUnknown*, SurfaceDesc*);
            return (*reinterpret_cast<Fn**>(surface))[8](surface, desc);
        }
    }

    inline IUnknown* pIntzTexture = nullptr;   // IDirect3DTexture8 of d3d8to9
    inline IUnknown* pIntzSurface = nullptr;   // its level 0, which P_D3D holds a reference to as well while installed
    inline IUnknown* pOriginalDepth = nullptr; // the device's depth-stencil, P_D3D's reference to it kept while replaced
    inline bool bIntzTried = false;            // for this device

    inline void ReleaseIntz()
    {
        if (pIntzSurface)
            pIntzSurface->Release();
        if (pIntzTexture)
            pIntzTexture->Release();
        pIntzSurface = nullptr;
        pIntzTexture = nullptr;
        bIntzTried = false;
    }

    // Puts the device's own depth-stencil back, before the game resets or frees the device, which then finds what it
    // created
    inline void UninstallIntz(uint8_t* pD3D)
    {
        auto device = *(IUnknown**)(pD3D + layout.d3dDevice);
        auto& depth = *(IUnknown**)(pD3D + layout.d3dDepthStencil);
        if (pOriginalDepth && depth == pIntzSurface)
        {
            depth->Release();
            depth = pOriginalDepth;
            if (device)
                D3D8::SetRenderTarget(device, nullptr, depth);
        }
        pOriginalDepth = nullptr;
        ReleaseIntz();
    }

    inline void SetDepthUnusable(bool bUnusable)
    {
        if (bUnusable != bDepthUnusable)
        {
            bDepthUnusable = bUnusable;
            pSettings->Refresh("GRAPHICS.AntiAliasing");
        }
    }

    // The driver's depth-stencil becomes an INTZ texture of the same size, once per device, while a temporal mode is
    // chosen. The game releases what P_D3D holds when it resets or frees the device.
    inline void InstallIntz(uint8_t* pD3D, IDirect3DDevice9* device9)
    {
        auto device = *(IUnknown**)(pD3D + layout.d3dDevice);
        auto& depth = *(IUnknown**)(pD3D + layout.d3dDepthStencil);
        if (!device || !depth || !device9 || depth == pIntzSurface || bIntzTried)
            return;
        bIntzTried = true;

        D3D8::SurfaceDesc desc{};
        if (FAILED(D3D8::GetDesc(depth, &desc)) || desc.MultiSampleType != D3DMULTISAMPLE_NONE || !WFPAA::TemporalAA::IntzSupported(device9))
        {
            WFPAA::TemporalAA::Log("Depth: the depth-stencil is multisampled or INTZ isn't supported, temporal anti-aliasing is off");
            SetDepthUnusable(true);
            return;
        }

        ReleaseIntz();
        bIntzTried = true;
        if (FAILED(D3D8::CreateTexture(device, desc.Width, desc.Height, 1, D3DUSAGE_DEPTHSTENCIL, static_cast<D3DFORMAT>(MAKEFOURCC('I', 'N', 'T', 'Z')), D3DPOOL_DEFAULT, &pIntzTexture)) ||
            !pIntzTexture || FAILED(D3D8::GetSurfaceLevel(pIntzTexture, 0, &pIntzSurface)) || !pIntzSurface)
        {
            WFPAA::TemporalAA::Log("Depth: the INTZ texture couldn't be created");
            ReleaseIntz();
            bIntzTried = true;
            SetDepthUnusable(true);
            return;
        }

        // P_D3D's reference to the original is kept here, it gets one to the INTZ surface instead
        pOriginalDepth = depth;
        depth = pIntzSurface;
        depth->AddRef();
        D3D8::SetRenderTarget(device, nullptr, depth);
        SetDepthUnusable(false);
    }

    // The scene's depth as a Direct3D 9 texture: the INTZ texture behind the device's depth-stencil
    inline IDirect3DTexture9* GetDepthTexture(IDirect3DDevice9* device)
    {
        IDirect3DSurface9* surface = nullptr;
        IDirect3DTexture9* texture = nullptr;
        if (SUCCEEDED(device->GetDepthStencilSurface(&surface)) && surface)
        {
            D3DSURFACE_DESC desc{};
            if (SUCCEEDED(surface->GetDesc(&desc)) && desc.Format == static_cast<D3DFORMAT>(MAKEFOURCC('I', 'N', 'T', 'Z')))
                surface->GetContainer(__uuidof(IDirect3DTexture9), (void**)&texture);
            surface->Release();
        }
        return texture;
    }

    inline WFPAA::Matrix ViewMatrix(const float* m)
    {
        // M_Matrix4x3: three axes and the position, as rows of a row vector transform
        WFPAA::Matrix r = WFPAA::Matrix::Identity();
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 3; ++j)
                r.m[i][j] = m[i * 3 + j];
        return r;
    }

    // -------------------------------------------------------------------------------------------------------------------
    // What the game calls

    inline bool IsSMAA()
    {
        return pAntiAliasing && *pAntiAliasing == SMAA;
    }

    // X_LevelRuntimeCamera's camera, the main one of the game view
    inline void SetMainCamera(void* pCamera)
    {
        pMainCamera = (uint8_t*)pCamera;
    }

    // MaxPayne_GameMode::renderMode: the game view starts
    inline void BeginGameView()
    {
        bInGameView = true;
        bJittered = false;
    }

    // The game view of this frame isn't resolved where Render is called (it goes through the game's own
    // post-processing first): no jitter
    inline void SkipJitter(bool bSkip)
    {
        bNoJitter = bSkip;
    }

    // After P_Camera::prepare; bDirty: it computed the projection again. width, height: the render size.
    inline void OnCameraPrepared(uint8_t* pCamera, bool bDirty, int32_t width, int32_t height)
    {
        if (!bInGameView || pCamera != pMainCamera || !IsTemporal() || bDepthUnusable)
            return;

        auto projection = (float*)(pCamera + layout.cameraProjection);
        if (bJittered)
        {
            // Prepared again in the same frame: a projection computed again gets the same jitter
            if (bDirty)
            {
                std::memcpy(Unjittered, projection, sizeof(Unjittered));
                WFPAA::JitterProjection(projection, CurrentJitter, float(width), float(height));
            }
            return;
        }

        if (width <= 0 || height <= 0)
            return;

        WFPAA::Camera camera;
        camera.View = ViewMatrix((const float*)(pCamera + layout.cameraView));
        camera.Projection = WFPAA::Matrix::From(projection);
        camera.Width = width;
        camera.Height = height;
        CurrentJitter = WFPAA::TemporalAA::OnCamera(camera);
        if (bNoJitter || (CurrentJitter.X == 0.0f && CurrentJitter.Y == 0.0f))
            return;

        std::memcpy(Unjittered, projection, sizeof(Unjittered));
        WFPAA::JitterProjection(projection, CurrentJitter, float(width), float(height));
        bJittered = true;
    }

    // At the end of the game view, before the HUD and the other effects. Off and SMAA do nothing here; DLAA and FSR
    // also start the hub. bResolve: the scene is there to be resolved (not after the game's own post-processing).
    inline void Render(IDirect3DDevice9* device, bool bResolve = true)
    {
        bInGameView = false;
        if (!device || !IsTemporal())
        {
            if (bJittered && pMainCamera)
            {
                std::memcpy(pMainCamera + layout.cameraProjection, Unjittered, sizeof(Unjittered));
                bJittered = false;
            }
            return;
        }

        static uint32_t generation = 0;
        if (generation != WFPAA::TemporalAA::Generation())
        {
            generation = WFPAA::TemporalAA::Generation();
            pSettings->Refresh("GRAPHICS.AntiAliasing");
        }

        WFPAA::TemporalAA::SetMode(ToMode(*pAntiAliasing), *pSharpness / 100.0f);
        IDirect3DSurface9* scene = nullptr;
        device->GetRenderTarget(0, &scene);
        auto depth = IsTemporal() && bResolve ? GetDepthTexture(device) : nullptr;
        WFPAA::TemporalAA::Render(device, scene, depth, scene);
        if (depth)
            depth->Release();
        if (scene)
            scene->Release();

        // The HUD and the cameras after the scene don't have the jitter
        if (bJittered && pMainCamera)
        {
            std::memcpy(pMainCamera + layout.cameraProjection, Unjittered, sizeof(Unjittered));
            bJittered = false;
        }
    }

    // DRV_beginScene, before the driver's own: a device that is about to be reset gets its depth-stencil back and loses
    // the library's resources
    inline void BeforeBeginScene(uint8_t* pD3D, bool bReset)
    {
        if (bReset)
        {
            UninstallIntz(pD3D);
            WFPAA::TemporalAA::OnLostDevice();
        }
    }

    // DRV_beginScene, after the driver's own: the INTZ depth only while a temporal mode is chosen
    inline void AfterBeginScene(uint8_t* pD3D, IDirect3DDevice9* device9)
    {
        if (IsTemporal())
            InstallIntz(pD3D, device9);
        else if (pOriginalDepth || pIntzTexture)
            UninstallIntz(pD3D);
    }

    // DRV_freeMode, before the driver's own: the device goes
    inline void OnFreeMode(uint8_t* pD3D)
    {
        UninstallIntz(pD3D);
        WFPAA::TemporalAA::OnDeviceReleased();
    }

    // From the plugin's init: before the game creates its Direct3D object. clientName: the plugin's name for the hub.
    inline void Init(asi_settings::Plugin& settings, const Layout& gameLayout, const char* clientName, const char* sharpnessId = "GRAPHICS.AntiAliasingSharpness", const char* apiId = "GRAPHICS.API")
    {
        layout = gameLayout;
        pSettings = &settings;
        pAntiAliasing = &settings.Int("GRAPHICS.AntiAliasing");
        pSharpness = &settings.Float(sharpnessId);

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
        settings.Availability(apiId, [](std::string_view value)
        {
            if (value == "1")
                return WFPAA::VulkanAvailable();
            if (value == "2")
                return allowD3D9On12;
            return true;
        });
        WFPAA::InstallGraphicsApi(settings.Int(apiId), allowD3D9On12);

        // The hub starts with the first DLAA or FSR frame. Until it has answered, DLAA and FSR are offered when they can
        // run at all (TAA stands in while it starts); after that, as it reported them. TAA unless the depth can't be read.
        settings.Availability("GRAPHICS.AntiAliasing", [](std::string_view value)
        {
            if (value == "2")
                return !bDepthUnusable;
            if (value == "3")
                return !bDepthUnusable && WFPAA::TemporalAA::IsAvailable(WFPAA::Backend::DLSS);
            if (value == "4")
                return !bDepthUnusable && WFPAA::TemporalAA::IsAvailable(WFPAA::Backend::FSR);
            return true;
        });
    }
}
