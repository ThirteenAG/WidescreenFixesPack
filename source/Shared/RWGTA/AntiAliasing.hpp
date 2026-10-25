#pragma once
#include <cstring>
#include <functional>
#include <string>
#include "../AA/AntiAliasing.hpp"

// Anti-aliasing of the classic GTA fixes: SMAA (PostFX), and temporal anti-aliasing through the shared library
// (source/Shared/AA): TAA, DLAA and FSR native AA, the last two run by FusionFixHub.exe on DXVK (Graphics API Vulkan).
// III and Vice City are Direct3D 8: everything needs d3d8to9 (the bundled Ultimate ASI Loader with UseD3D8to9=1), the
// effects draw with its Direct3D 9 device.
//
// - Jitter: RenderWare's _rwD3D8CameraBeginUpdate sets the camera's view and then its projection (RwD3D8SetTransform,
//   D3DTS_VIEW then D3DTS_PROJECTION) every frame. For the scene's camera a mid hook at the projection's call moves
//   the projection by the jitter before RenderWare sets it. The 2D frontend and HUD use transformed vertices and don't
//   see it. After the resolve the device gets the projection without the jitter back. GTA III's Steam exe was built
//   with another compiler: same places, other registers.
// - Depth: while a temporal mode is chosen, the scene camera's Z raster gets an INTZ texture's surface instead of the
//   device's depth-stencil (Depth8.hpp), the depth for the camera motion and the resolve; the original comes back
//   before every reset and when the mode is left. Not with MSAA: INTZ can't be multisampled.
// - Off and SMAA cost nothing here: the camera hooks are installed the first time a temporal mode is chosen.
// - Resolve: PostFX's hook before CCamera::RenderMotionBlur, after the 3D scene and its effects, before the HUD; LDR.
namespace RWGTA::AntiAliasing
{
    enum Setting
    {
        Off = 0, SMAA = 1, TAA = 2, DLAA = 3, FSR = 4
    };

    // What the game's module provides: its settings and the functions of Depth8.hpp (in a file of their own)
    struct Game
    {
        asi_settings::Plugin* settings = nullptr;
        const char* clientName = nullptr;
        std::function<void*()> sceneCamera;             // Scene.m_pRwCamera
        std::function<IUnknown*()> device8;             // the game's IDirect3DDevice8 (d3d8to9's)
        std::function<bool(void* device8, void* camera, int32_t extOffset, bool& created)> replaceDepth;
        std::function<void(void* device8, void* camera, int32_t extOffset)> restoreDepth;
        std::function<std::pair<uint32_t, uint32_t>()> depthSize;
    };

    namespace detail
    {
        inline Game game;
        inline const int* pAntiAliasing = nullptr;
        inline const float* pSharpness = nullptr;
        inline bool bDepthUnusable = false;     // MSAA or no INTZ: TAA isn't offered
        inline bool bCameraFound = false;
        inline bool bSteamCamera = false;       // III Steam's _rwD3D8CameraBeginUpdate: other registers
        inline uint8_t* pViewCall = nullptr;
        inline uint8_t* pProjectionCall = nullptr;
        inline int32_t* pRasterExtOffset = nullptr;
        inline bool bDepthReplaced = false;
        inline float View[16]{};
        inline float Unjittered[16]{};
        inline bool bJittered = false;
        // When the scene camera's update begins more than once a frame, the first one is the frame's camera for the
        // temporal AA and the others get the same jitter
        inline bool bFrameCamera = false;
        inline WFPAA::Jitter FrameJitter{};

        inline IDirect3DDevice9* Device9()
        {
            auto device = game.device8 ? game.device8() : nullptr;
            IDirect3DDevice9* device9 = nullptr;
            if (!device || FAILED(device->QueryInterface(__uuidof(IDirect3DDevice9), (void**)&device9)))
                return nullptr;
            device9->Release();
            return device9;
        }

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

        inline void SetDepthUnusable(bool unusable)
        {
            if (unusable != bDepthUnusable)
            {
                bDepthUnusable = unusable;
                game.settings->Refresh("GRAPHICS.AntiAliasing");
            }
        }

        // At _rwD3D8CameraBeginUpdate's projection: the scene camera's view and projection, before RenderWare sets them
        inline void OnProjection(void* camera, float* projection)
        {
            bJittered = false;
            if (!pAntiAliasing || !pRasterExtOffset || !camera || camera != game.sceneCamera())
                return;
            if (*pAntiAliasing < TAA)
            {
                // Left the temporal modes: the camera gets its own depth-stencil back
                if (bDepthReplaced)
                {
                    game.restoreDepth(game.device8(), camera, *pRasterExtOffset);
                    bDepthReplaced = false;
                }
                return;
            }
            // Only its frames on the screen (a camera raster, RwRaster::cType 2), not into a texture
            auto raster = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(camera) + 0x60);
            auto parent = raster ? *reinterpret_cast<uint8_t**>(raster) : nullptr;
            if (!parent || parent[0x20] != 2)
                return;

            bool created = false;
            bDepthReplaced = game.replaceDepth(game.device8(), camera, *pRasterExtOffset, created);
            SetDepthUnusable(!bDepthReplaced);
            if (bDepthUnusable)
                return;

            auto device = Device9();
            IDirect3DSurface9* backBuffer = nullptr;
            D3DSURFACE_DESC desc{};
            if (!device || FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer)) || !backBuffer)
                return;
            backBuffer->GetDesc(&desc);
            backBuffer->Release();

            WFPAA::Camera c;
            c.View = WFPAA::Matrix::From(View);
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

        // _rwD3D8CameraBeginUpdate's view and projection: hooked the first time a temporal mode is chosen
        inline void InstallCameraHooks()
        {
            static bool installed = false;
            if (installed || !bCameraFound)
                return;
            installed = true;
            if (!bSteamCamera)
            {
                static auto ViewHook = safetyhook::create_mid(pViewCall, [](SafetyHookContext& regs)
                {
                    std::memcpy(View, *reinterpret_cast<float**>(regs.esp + 4), sizeof(View));
                });
                static auto ProjectionHook = safetyhook::create_mid(pProjectionCall, [](SafetyHookContext& regs)
                {
                    OnProjection(reinterpret_cast<void*>(regs.edi), *reinterpret_cast<float**>(regs.esp + 4));
                });
            }
            else
            {
                static auto ViewHook = safetyhook::create_mid(pViewCall, [](SafetyHookContext& regs)
                {
                    std::memcpy(View, reinterpret_cast<float*>(regs.edx), sizeof(View));
                });
                static auto ProjectionHook = safetyhook::create_mid(pProjectionCall, [](SafetyHookContext& regs)
                {
                    OnProjection(reinterpret_cast<void*>(regs.ebx), reinterpret_cast<float*>(regs.edx));
                });
            }
        }
    }

    inline bool IsSMAA()
    {
        return detail::pAntiAliasing && *detail::pAntiAliasing == SMAA;
    }

    // Before CCamera::RenderMotionBlur: every frame of the 3D scene, whatever the mode
    inline void Render(IDirect3DDevice9* device)
    {
        using namespace detail;
        bFrameCamera = false;
        if (!device || !pAntiAliasing)
            return;

        static uint32_t generation = 0;
        if (generation != WFPAA::TemporalAA::Generation())
        {
            generation = WFPAA::TemporalAA::Generation();
            game.settings->Refresh("GRAPHICS.AntiAliasing");
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
        IDirect3DSurface9* depthSurface = nullptr;
        IDirect3DTexture9* depth = nullptr;
        device->GetRenderTarget(0, &scene);
        if (SUCCEEDED(device->GetDepthStencilSurface(&depthSurface)) && depthSurface)
            depthSurface->GetContainer(__uuidof(IDirect3DTexture9), (void**)&depth);
        WFPAA::TemporalAA::Render(device, scene, depth, scene);
        if (depth)
            depth->Release();
        if (depthSurface)
            depthSurface->Release();
        if (scene)
            scene->Release();

        // What is drawn after the scene isn't resolved: without the jitter. RenderWare keeps the jittered one as its
        // last projection, so the next frame's (a new jitter) still reaches the device.
        if (bJittered)
        {
            bJittered = false;
            device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(Unjittered));
        }
    }

    // From the plugin's init, before the game creates its Direct3D object
    inline void Init(const Game& g)
    {
        using namespace detail;
        game = g;
        auto& settings = *game.settings;
        pAntiAliasing = &settings.Int("GRAPHICS.AntiAliasing");
        pSharpness = &settings.Float("GRAPHICS.AntiAliasingSharpness");

        auto name = GetThisModuleName<std::wstring>();
        name = name.substr(0, name.find_last_of(L'.'));
        WFPAA::Options options;
        options.BridgeName = L"Local\\" + name + L".Upscaler";
        options.ClientName = game.clientName;
        options.LowDynamicRange = true;
        WFPAA::TemporalAA::Configure(options);

        // Graphics API: before the game creates its Direct3D object. Vulkan is offered once the hub installed DXVK
        // (vulkan.dll in the game folder), D3D9on12 only with GRAPHICS.D3D9on12 = 1 in the file.
        static bool allowD3D9On12 = settings.Bool("GRAPHICS.D3D9on12");
        settings.Availability("GRAPHICS.API", [](std::string_view value)
        {
            if (value == "1")
                return WFPAA::VulkanAvailable();
            if (value == "2")
                return allowD3D9On12;
            return true;
        });
        if (settings.Int("GRAPHICS.API") != 0)
            WFPAA::InstallGraphicsApi(settings.Int("GRAPHICS.API"), allowD3D9On12);

        // DLAA and FSR once the hub reported them (it starts with the device), TAA unless the depth can't be read or
        // the camera wasn't found
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

        // _rwD3D8CameraBeginUpdate: the view's and the projection's RwD3D8SetTransform, and the raster extension's
        // offset just after them. III 1.0/1.1 and Vice City: cdecl (state, matrix), the camera in edi; III Steam:
        // eax = state, edx = matrix, the camera in ebx. Found here, hooked once a temporal mode is chosen.
        auto view = hook::pattern("6A 02 C7 44 24 ? 00 00 00 00 C7 44 24 ? 00 00 00 00 C7 84 24 ? ? ? ? 00 00 00 00 89 94 24 ? ? ? ? 89 84 24 ? ? ? ? C7 84 24 ? ? ? ? 00 00 80 3F E8");
        auto projection = hook::pattern("51 6A 03 D9 E0 D9 9C 24 ? ? ? ? E8 ? ? ? ? 8B 6F 60 8B 15");
        if (view.size() == 1 && projection.size() == 1)
        {
            pRasterExtOffset = *projection.get_first<int32_t*>(22);
            pViewCall = view.get_first<uint8_t>(0x36);
            pProjectionCall = projection.get_first<uint8_t>(12);
            bCameraFound = true;
        }
        else
        {
            view = hook::pattern("B8 02 00 00 00 89 95 ? ? ? ? 8D 95 ? ? ? ? E8");
            projection = hook::pattern("B8 03 00 00 00 D9 9D ? ? ? ? 8D 95 ? ? ? ? E8 ? ? ? ? 8B 73 60 8B 3D");
            if (view.size() == 1 && projection.size() == 1)
            {
                pRasterExtOffset = *projection.get_first<int32_t*>(27);
                pViewCall = view.get_first<uint8_t>(17);
                pProjectionCall = projection.get_first<uint8_t>(17);
                bSteamCamera = true;
                bCameraFound = true;
            }
        }
        settings.Refresh("GRAPHICS.AntiAliasing");

        WFP::onBeforeReset() += []()
        {
            if (pRasterExtOffset)
                game.restoreDepth(game.device8(), game.sceneCamera(), *pRasterExtOffset);
            bDepthReplaced = false;
            bJittered = false;
            WFPAA::TemporalAA::OnLostDevice();
        };

        WFP::onShutdownEvent() += []()
        {
            WFPAA::TemporalAA::Shutdown();
        };
    }
}
