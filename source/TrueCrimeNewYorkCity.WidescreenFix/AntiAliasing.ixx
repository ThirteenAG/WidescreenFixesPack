module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include "../Shared/AA/AntiAliasing.hpp"

export module AntiAliasing;

import ComVars;

// Anti-aliasing uses the Direct3D 9 device supplied by the ASI loader's d3d8to9.
//
// - Jitter: the main camera (flt_7A0390 in the exe without disc check) gets its projection (+0x340, left-handed, w = z,
//   no far plane) and then its view (+0xD0) every frame in sub_49B8C0, before the scene. After the view, the projection
//   is set again with the jitter, and the view again, which builds the view * projection products from it. Once the
//   scene is resolved both are set back without the jitter, for what the render list draws after it.
// - Depth: the depth-stencil is an INTZ texture already (DepthStencil), the depth for the camera motion and the resolve.
// - Resolve: PostFX's pass after the render list node sub_52F7D0, where the opaque scene is complete and the HUD isn't
//   drawn yet, before the other effects; LDR.

namespace
{
    enum Setting
    {
        Off = 0, SMAA = 1, TAA = 2, DLAA = 3, FSR = 4
    };

    const int* pAntiAliasing = nullptr;
    const float* pSharpness = nullptr;
    bool bDepthUnusable = false;        // no INTZ depth texture: TAA isn't offered
    bool bTemporalStarted = false;      // a temporal mode ran once: the hub was asked for DLAA and FSR

    IUnknown** ppDevice = nullptr;      // the game's IDirect3DDevice8; d3d8to9 answers IDirect3DDevice9

    // The main camera: its view at +0xD0, its projection at +0x340 (16-byte aligned, SSE)
    uint8_t* pCamera = nullptr;
    void(__fastcall* SetProjection)(void* camera, void* edx, const float* projection) = nullptr;     // thiscall
    void(__fastcall* SetView)(void* camera, void* edx, const float* cameraMatrix) = nullptr;         // thiscall, the camera's world matrix

    alignas(16) float Unjittered[16]{};
    alignas(16) float CameraMatrix[16]{};
    bool bJittered = false;

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

    bool BackBufferSize(uint32_t& width, uint32_t& height)
    {
        IDirect3DDevice9* device = nullptr;
        if (!ppDevice || !*ppDevice || FAILED((*ppDevice)->QueryInterface(__uuidof(IDirect3DDevice9), (void**)&device)) || !device)
            return false;
        IDirect3DSurface9* backBuffer = nullptr;
        D3DSURFACE_DESC desc{};
        bool ok = SUCCEEDED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer)) && backBuffer && SUCCEEDED(backBuffer->GetDesc(&desc));
        if (backBuffer)
            backBuffer->Release();
        device->Release();
        width = desc.Width;
        height = desc.Height;
        return ok && width && height;
    }

    void __fastcall SetViewHook(uint8_t* camera, void* edx, const float* cameraMatrix)
    {
        SetView(camera, edx, cameraMatrix);
        bJittered = false;

        // The main camera, while a temporal mode is chosen
        if (!pAntiAliasing || *pAntiAliasing < TAA || camera != pCamera)
            return;

        uint32_t width = 0, height = 0;
        if (!BackBufferSize(width, height))
            return;

        auto projection = reinterpret_cast<float*>(camera + 0x340);
        WFPAA::Camera c;
        c.View = WFPAA::Matrix::From(reinterpret_cast<float*>(camera + 0xD0));
        c.Projection = WFPAA::Matrix::From(projection);
        c.Width = width;
        c.Height = height;

        auto jitter = WFPAA::TemporalAA::OnCamera(c);
        if (jitter.X == 0.0f && jitter.Y == 0.0f)
            return;

        std::memcpy(Unjittered, projection, sizeof(Unjittered));
        std::memcpy(CameraMatrix, cameraMatrix, sizeof(CameraMatrix));
        alignas(16) float jittered[16];
        std::memcpy(jittered, projection, sizeof(jittered));
        WFPAA::JitterProjection(jittered, jitter, static_cast<float>(width), static_cast<float>(height));
        SetProjection(camera, nullptr, jittered);
        SetView(camera, nullptr, CameraMatrix);
        bJittered = true;
    }
}

export namespace AntiAliasing
{
    bool IsSMAA()
    {
        return pAntiAliasing && *pAntiAliasing == SMAA;
    }

    // Whether Render has work this frame: a temporal mode, or a jittered camera to set back. Off and SMAA cost nothing.
    bool IsActive()
    {
        return bJittered || (pAntiAliasing && *pAntiAliasing >= TAA && SetView);
    }

    // In PostFX's pass, before the other effects, while IsActive (the first call also starts the hub). scene is the
    // render target, depth the INTZ depth-stencil's texture (null without it).
    void Render(IDirect3DDevice9* device, IDirect3DSurface9* scene, IDirect3DBaseTexture9* depth)
    {
        if (!device || !pAntiAliasing)
            return;

        static uint32_t generation = 0;
        bool unusable = !depth;
        if (unusable != bDepthUnusable || generation != WFPAA::TemporalAA::Generation())
        {
            bDepthUnusable = unusable;
            generation = WFPAA::TemporalAA::Generation();
            FixSettings().Refresh("GRAPHICS.AntiAliasing");
        }

        auto mode = ToMode(*pAntiAliasing);
        WFPAA::TemporalAA::SetMode(mode, *pSharpness / 100.0f);
        if (mode != WFPAA::Mode::Off)
        {
            bTemporalStarted = true;
            WFPAA::TemporalAA::Render(device, scene, depth, scene);
        }

        // What the render list draws after the scene (transparent objects, particles) isn't resolved: without the jitter
        if (bJittered)
        {
            bJittered = false;
            SetProjection(pCamera, nullptr, Unjittered);
            SetView(pCamera, nullptr, CameraMatrix);
        }
    }

    // From the plugin's init: before the game creates its Direct3D object
    void Init()
    {
        auto& settings = FixSettings();
        pAntiAliasing = &settings.Int("GRAPHICS.AntiAliasing");
        pSharpness = &settings.Float("GRAPHICS.Sharpness");

        auto name = GetThisModuleName<std::wstring>();
        name = name.substr(0, name.find_last_of(L'.'));
        WFPAA::Options options;
        options.BridgeName = L"Local\\" + name + L".Upscaler";
        options.ClientName = "TrueCrimeNewYorkCity.WidescreenFix";
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
        WFPAA::InstallGraphicsApi(settings.Int("MAIN.GraphicsAPI"), allowD3D9On12);

        // TAA unless the depth can't be read or the camera wasn't found. DLAA and FSR on Vulkan: the hub starts with the
        // first temporal frame and then reports what it can run.
        settings.Availability("GRAPHICS.AntiAliasing", [](std::string_view value)
        {
            bool temporal = !bDepthUnusable && SetView;
            if (value == "2")
                return temporal;
            if (value == "3" || value == "4")
            {
                if (!temporal)
                    return false;
                if (!bTemporalStarted)
                    return WFPAA::RunningApi() == WFPAA::GraphicsApi::Vulkan;
                return WFPAA::TemporalAA::IsAvailable(value == "3" ? WFPAA::Backend::DLSS : WFPAA::Backend::FSR);
            }
            return true;
        });

        auto pattern = hook::pattern("A1 ? ? ? ? 8B 10 8D 4C 24 ? 51 50");
        if (pattern.empty())
        {
            OutputDebugStringA("TCNYC: device pointer not found, temporal anti-aliasing stays off.\n");
            return;
        }
        ppDevice = *pattern.get_first<IUnknown**>(1);

        // sub_49B8C0: the main camera's projection (times the identity), then its view, before the scene
        pattern = hook::pattern("68 ? ? ? ? 51 8D 54 24 ? 52 E8 ? ? ? ? 83 C4 0C 8D 44 24 ? 50 B9 ? ? ? ? E8 ? ? ? ? 8D 4C 24 ? 51 8B CE E8 ? ? ? ? 50 B9 ? ? ? ? E8");
        if (!pattern.empty())
        {
            pCamera = *pattern.get_first<uint8_t*>(25);
            SetProjection = reinterpret_cast<decltype(SetProjection)>(injector::GetBranchDestination(pattern.get_first(29)).as_int());
            SetView = reinterpret_cast<decltype(SetView)>(injector::GetBranchDestination(pattern.get_first(52)).as_int());
            injector::MakeCALL(pattern.get_first(52), SetViewHook, true);
        }
        else
        {
            OutputDebugStringA("TCNYC: main camera not found, temporal anti-aliasing stays off.\n");
        }
    }
}
