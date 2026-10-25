module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"
#include <d3d9.h>
#include "../Shared/AA/AntiAliasing.hpp"

export module AntiAliasing;

import Skeleton;
import Main;

// Anti-aliasing: SMAA (PostFX), and temporal anti-aliasing through the shared library (source/Shared/AA): TAA, DLAA and
// FSR native AA, the last two run by FusionFixHub.exe on DXVK (Graphics API Vulkan). San Andreas is Direct3D 9.
//
// - Jitter: RenderWare's _rwD3D9CameraBeginUpdate builds the camera's view and projection in its globals
//   (_RwD3D9D3D9ViewTransform, _RwD3D9D3D9ProjTransform) and sets them (RwD3D9SetTransform) every frame. For the
//   scene's camera a mid hook at the projection's call moves the projection by the jitter before RenderWare sets it;
//   the next frame builds it from the camera again. After the resolve the device gets it without the jitter back.
// - Depth: the scene's camera draws with RenderWare's _RwD3D9DepthStencilSurface, which it gets from the device after
//   creating or resetting it, and which every render to texture (mirrors, shadows) sets back afterwards. It becomes
//   an INTZ texture of the same size (ReplaceDepthStencil), the depth for the camera motion and the resolve; the
//   original comes back before every reset and when the temporal modes are left. Not with the game's MSAA: INTZ
//   can't be multisampled.
// - Resolve: PostFX's hook before CCamera::RenderMotionBlur, after the 3D scene and its effects, before the HUD; LDR.
// - Off and SMAA cost nothing here: the camera hooks are installed the first time a temporal mode is chosen.

namespace
{
    enum Setting
    {
        Off = 0, SMAA = 1, TAA = 2, DLAA = 3, FSR = 4
    };

    const int* pAntiAliasing = nullptr;
    const float* pSharpness = nullptr;
    bool bDepthUnusable = false;
    bool bCameraFound = false;
    uint8_t* pViewCall = nullptr;
    uint8_t* pProjectionCall = nullptr;

    IDirect3DSurface9** ppDepthStencil = nullptr;   // _RwD3D9DepthStencilSurface
    IDirect3DSurface9* OriginalDepth = nullptr;
    IDirect3DSurface9* IntzSurface = nullptr;       // with our reference
    float* pView = nullptr;
    float Unjittered[16]{};
    bool bJittered = false;
    // San Andreas begins the scene camera's update several times a frame (6 in a street): the first one is the frame's
    // camera for the temporal AA, the others get the same jitter
    bool bFrameCamera = false;
    WFPAA::Jitter FrameJitter{};

    IDirect3DDevice9* Device()
    {
        return pD3D9Device ? *pD3D9Device : nullptr;
    }

    RwCamera* SceneCamera()
    {
        return Scene.is_initialized() ? Scene.get_ptr()->m_pRwCamera : nullptr;
    }

    // The scene camera also draws the mirrors (CMirrors::RenderMirrorBuffer) into a texture: only its frames on the
    // screen (a camera raster) are jittered
    bool RendersToScreen(RwCamera* camera)
    {
        auto raster = camera->frameBuffer;
        if (!raster)
            return false;
        auto parent = *reinterpret_cast<uint8_t**>(raster);
        return parent && parent[0x20] == 2;     // RwRaster::cType: rwRASTERTYPECAMERA
    }

    WFPAA::Mode ToMode(int value)
    {
        switch (value)
        {
        case TAA: return WFPAA::Mode::TAA;
        case DLAA: return WFPAA::Mode::DLAA;
        case FSR: return WFPAA::Mode::FSR;
        default: return WFPAA::Mode::Off;
        }
    }

    void SetDepthUnusable(bool unusable)
    {
        if (unusable != bDepthUnusable)
        {
            bDepthUnusable = unusable;
            FixSettings().Refresh("GRAPHICS.AntiAliasing");
        }
    }

    IDirect3DSurface9* RefusedDepth = nullptr;  // not tried again each frame (multisampled, no INTZ) until a reset

    // RenderWare's depth-stencil becomes the INTZ surface; false when it can't be
    bool ReplaceDepth()
    {
        auto device = Device();
        if (!device || !ppDepthStencil || !*ppDepthStencil)
            return false;
        if (*ppDepthStencil == IntzSurface)
            return true;
        if (IntzSurface)
        {
            IntzSurface->Release();
            IntzSurface = nullptr;
        }
        if (*ppDepthStencil == RefusedDepth)
            return false;
        auto replacement = WFPAA::TemporalAA::ReplaceDepthStencil(device, *ppDepthStencil);
        if (!replacement)
        {
            RefusedDepth = *ppDepthStencil;
            return false;
        }
        RefusedDepth = nullptr;
        OriginalDepth = *ppDepthStencil;
        IntzSurface = replacement;
        *ppDepthStencil = IntzSurface;
        return true;
    }

    void RestoreDepth()
    {
        if (ppDepthStencil && IntzSurface && *ppDepthStencil == IntzSurface)
        {
            *ppDepthStencil = OriginalDepth;
            if (auto device = Device())
                device->SetDepthStencilSurface(OriginalDepth);
        }
        if (IntzSurface)
            IntzSurface->Release();
        IntzSurface = nullptr;
        OriginalDepth = nullptr;
    }

    void OnProjection(RwCamera* camera, float* projection)
    {
        bJittered = false;
        if (!pAntiAliasing || !camera || camera != SceneCamera() || !RendersToScreen(camera))
            return;
        if (*pAntiAliasing < TAA)
        {
            // Left the temporal modes: RenderWare gets its own depth-stencil back
            if (IntzSurface)
                RestoreDepth();
            return;
        }
        SetDepthUnusable(!ReplaceDepth());
        if (bDepthUnusable)
            return;

        auto device = Device();
        IDirect3DSurface9* backBuffer = nullptr;
        D3DSURFACE_DESC desc{};
        if (!device || FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer)) || !backBuffer)
            return;
        backBuffer->GetDesc(&desc);
        backBuffer->Release();

        WFPAA::Camera c;
        c.View = WFPAA::Matrix::From(pView);
        c.Projection = WFPAA::Matrix::From(projection);
        c.Width = desc.Width;
        c.Height = desc.Height;
        auto jitter = bFrameCamera ? FrameJitter : WFPAA::TemporalAA::OnCamera(c);
        bFrameCamera = true;
        FrameJitter = jitter;
        if (jitter.X == 0.0f && jitter.Y == 0.0f)
            return;
        std::memcpy(Unjittered, projection, sizeof(Unjittered));
        WFPAA::JitterProjection(projection, jitter, static_cast<float>(desc.Width), static_cast<float>(desc.Height));
        bJittered = true;
    }

    // _rwD3D9CameraBeginUpdate's view and projection: hooked the first time a temporal mode is chosen
    void InstallCameraHooks()
    {
        static bool installed = false;
        if (installed || !bCameraFound)
            return;
        installed = true;
        static auto ViewHook = safetyhook::create_mid(pViewCall, [](SafetyHookContext& regs)
        {
            pView = *reinterpret_cast<float**>(regs.esp + 4);
        });
        static auto ProjectionHook = safetyhook::create_mid(pProjectionCall, [](SafetyHookContext& regs)
        {
            if (pView)
                OnProjection(reinterpret_cast<RwCamera*>(regs.ebx), *reinterpret_cast<float**>(regs.esp + 4));
        });
    }
}

export namespace AntiAliasing
{
    bool IsSMAA()
    {
        return pAntiAliasing && *pAntiAliasing == SMAA;
    }

    // Before CCamera::RenderMotionBlur: every frame of the 3D scene, whatever the mode
    void Render(IDirect3DDevice9* device)
    {
        bFrameCamera = false;
        if (!device || !pAntiAliasing)
            return;

        static uint32_t generation = 0;
        if (generation != WFPAA::TemporalAA::Generation())
        {
            generation = WFPAA::TemporalAA::Generation();
            FixSettings().Refresh("GRAPHICS.AntiAliasing");
        }

        WFPAA::TemporalAA::SetMode(ToMode(*pAntiAliasing), *pSharpness / 100.0f);
        if (*pAntiAliasing < TAA && !bJittered)
        {
            // Off and SMAA: nothing to resolve. On DXVK (chosen for DLAA and FSR) the hub still starts, so that the
            // menu can offer them.
            static const bool vulkan = WFPAA::RunningApi() == WFPAA::GraphicsApi::Vulkan;
            if (vulkan)
                WFPAA::TemporalAA::Render(device, nullptr, nullptr, nullptr);
            return;
        }
        InstallCameraHooks();

        IDirect3DSurface9* scene = nullptr;
        device->GetRenderTarget(0, &scene);
        WFPAA::TemporalAA::Render(device, scene, WFPAA::TemporalAA::DepthTexture(), scene);
        if (scene)
            scene->Release();

        if (bJittered)
        {
            bJittered = false;
            device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(Unjittered));
        }
    }
}

class AntiAliasingInit
{
public:
    AntiAliasingInit()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            pAntiAliasing = &settings.Int("GRAPHICS.AntiAliasing");
            pSharpness = &settings.Float("GRAPHICS.Sharpness");

            auto name = GetThisModuleName<std::wstring>();
            name = name.substr(0, name.find_last_of(L'.'));
            WFPAA::Options options;
            options.BridgeName = L"Local\\" + name + L".Upscaler";
            options.ClientName = "GTASA.WidescreenFix";
            options.LowDynamicRange = true;
            WFPAA::TemporalAA::Configure(options);

            // Graphics API: before the game creates its Direct3D object. Vulkan is offered once the hub installed DXVK
            // (vulkan.dll in the game folder), D3D9on12 only with GRAPHICS.D3D9on12 = 1 in the file.
            static bool allowD3D9On12 = settings.Bool("GRAPHICS.D3D9on12");
            settings.Availability("MAIN.GraphicsAPI", [](std::string_view value)
            {
                if (value == "1")
                    return WFPAA::VulkanAvailable();
                if (value == "2")
                    return allowD3D9On12;
                return true;
            });
            if (settings.Int("MAIN.GraphicsAPI") != 0)
                WFPAA::InstallGraphicsApi(settings.Int("MAIN.GraphicsAPI"), allowD3D9On12);

            settings.Availability("GRAPHICS.AntiAliasing", [](std::string_view value)
            {
                bool temporal = !bDepthUnusable && bCameraFound;
                if (value == "2")
                    return temporal;
                if (value == "3")
                    return temporal && WFPAA::TemporalAA::IsAvailable(WFPAA::Backend::DLSS);
                if (value == "4")
                    return temporal && WFPAA::TemporalAA::IsAvailable(WFPAA::Backend::FSR);
                return true;
            });

            // _rwD3D9CameraBeginUpdate: the view's and the projection's RwD3D9SetTransform (cdecl state, matrix), the
            // camera in ebx; RenderWare's depth-stencil, which every GetDepthStencilSurface of it fills. Found here,
            // hooked once a temporal mode is chosen.
            auto view = hook::pattern("C7 05 ? ? ? ? 00 00 80 3F E8 ? ? ? ? 8B 4B 70");
            auto projection = hook::pattern("68 ? ? ? ? 6A 03 D9 E0 D9 1D ? ? ? ? E8");
            auto depth = hook::pattern("68 ? ? ? ? 50 8B 08 FF 91 A0 00 00 00");
            bool sameDepth = !depth.empty();
            for (size_t i = 0; i < depth.size(); ++i)
                sameDepth = sameDepth && *depth.get(i).get<void*>(1) == *depth.get_first<void*>(1);
            if (view.size() == 1 && projection.size() == 1 && sameDepth)
            {
                ppDepthStencil = *depth.get_first<IDirect3DSurface9**>(1);
                pViewCall = view.get_first<uint8_t>(10);
                pProjectionCall = projection.get_first<uint8_t>(15);
                bCameraFound = true;
            }
            settings.Refresh("GRAPHICS.AntiAliasing");

            WFP::onBeforeReset() += []()
            {
                RestoreDepth();
                RefusedDepth = nullptr;
                bJittered = false;
                WFPAA::TemporalAA::OnLostDevice();
            };

            WFP::onShutdownEvent() += []()
            {
                WFPAA::TemporalAA::Shutdown();
            };
        };
    }
} AntiAliasingInit;
