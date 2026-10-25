module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include "../Shared/AA/AntiAliasing.hpp"

export module AntiAliasing;

import ComVars;

// Anti-aliasing: SMAA (PostFX), and temporal anti-aliasing through the shared library (source/Shared/AA): TAA, DLAA and
// FSR native AA, the last two run by FusionFixHub.exe on DXVK (Graphics API Vulkan).
//
// - Jitter: eViewPlatInfo::CalculateViewMatricies (0x71B780) builds every view's matrices each frame. For the player's
//   view (eView id 1: the race and the front end's garage) the projection at +64 (left-handed, w = z) and the second
//   one at +192 get the jitter afterwards, and +128 (view * projection, the shaders' cmWorldViewProj) and +256 are
//   built again. Mirrors, reflections, shadow maps and the environment maps are other views and stay as they are.
// - Depth: the scene and the front end draw with the device's depth-stencil, which the game gets once in its render
//   target setup (0x71AAE0, also after a reset). It becomes an INTZ texture of the same size, the depth for the camera
//   motion and the resolve. Not with the game's MSAA (Anti-Aliasing in its video options): INTZ can't be multisampled.
// - Resolve: PostFX's hook before the front end (the HUD): the scene is complete there, after the visual treatment,
//   so LDR. The game's motion blur is off while a temporal mode is on (Rendering).

namespace
{
    enum Setting
    {
        Off = 0, SMAA = 1, TAA = 2, DLAA = 3, FSR = 4
    };

    const int* pAntiAliasing = nullptr;
    const float* pSharpness = nullptr;
    std::atomic<bool> bDepthUnusable = false;
    std::atomic<bool> bHooksPresent = false;
    std::atomic<bool> bResolveHook = false;

    bool DepthReady()
    {
        return bHooksPresent && bResolveHook && !bDepthUnusable && WFPAA::TemporalAA::DepthTexture();
    }

    bool Temporal()
    {
        return pAntiAliasing && *pAntiAliasing >= TAA;
    }

    // DLAA and FSR: what the hub reported once it runs; before that, whether it can run at all
    bool bBridgeStarted = false;
    bool bBridgePossible = false;
    bool BridgeOffers(WFPAA::Backend backend)
    {
        return WFPAA::TemporalAA::IsAvailable(backend) || (!bBridgeStarted && bBridgePossible);
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

    void Multiply(float* out, const float* a, const float* b)
    {
        float r[16];
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                r[i * 4 + j] = a[i * 4 + 0] * b[0 * 4 + j] + a[i * 4 + 1] * b[1 * 4 + j] + a[i * 4 + 2] * b[2 * 4 + j] + a[i * 4 + 3] * b[3 * 4 + j];
        std::memcpy(out, r, sizeof(r));
    }

    bool BackBufferSize(IDirect3DDevice9* device, uint32_t& width, uint32_t& height)
    {
        IDirect3DSurface9* backBuffer = nullptr;
        if (!device || FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer)) || !backBuffer)
            return false;
        D3DSURFACE_DESC desc{};
        bool ok = SUCCEEDED(backBuffer->GetDesc(&desc));
        backBuffer->Release();
        width = desc.Width;
        height = desc.Height;
        return ok && width && height;
    }

    SafetyHookInline shCalculateViewMatricies = {};
    int __fastcall CalculateViewMatricies(uint8_t* info, void* edx, uint8_t* view, float nearClip, float farClip, int a5, char planes)
    {
        auto result = shCalculateViewMatricies.unsafe_thiscall<int>(info, view, nearClip, farClip, a5, planes);

        // The player's view with a camera, while a temporal mode is chosen
        if (!pAntiAliasing || *pAntiAliasing < TAA || !DepthReady() || *reinterpret_cast<int32_t*>(view + 8) != 1 || !*reinterpret_cast<void**>(view + 64))
            return result;

        uint32_t width = 0, height = 0;
        if (!BackBufferSize(Direct3DDevice, width, height))
            return result;

        auto viewMatrix = reinterpret_cast<float*>(info);
        auto projection = reinterpret_cast<float*>(info + 64);
        WFPAA::Camera camera;
        camera.View = WFPAA::Matrix::From(viewMatrix);
        camera.Projection = WFPAA::Matrix::From(projection);
        camera.Width = width;
        camera.Height = height;

        WFPAA::TemporalAA::SetMode(ToMode(*pAntiAliasing), *pSharpness / 100.0f);
        auto jitter = WFPAA::TemporalAA::OnCamera(camera);
        if (jitter.X == 0.0f && jitter.Y == 0.0f)
            return result;

        auto w = static_cast<float>(width);
        auto h = static_cast<float>(height);
        WFPAA::JitterProjection(projection, jitter, w, h);
        Multiply(reinterpret_cast<float*>(info + 128), viewMatrix, projection);
        WFPAA::JitterProjection(reinterpret_cast<float*>(info + 192), jitter, w, h);
        if (planes)
            Multiply(reinterpret_cast<float*>(info + 256), viewMatrix, reinterpret_cast<float*>(info + 192));
        return result;
    }
}

export namespace AntiAliasing
{
    void SetResolveAvailable(bool available)
    {
        bResolveHook = available;
        if (pAntiAliasing) FixSettings().Refresh("GRAPHICS.AntiAliasing");
    }

    bool IsSMAA()
    {
        return pAntiAliasing && *pAntiAliasing == SMAA;
    }

    bool IsTemporal()
    {
        return pAntiAliasing && *pAntiAliasing >= TAA && DepthReady();
    }

    // Before the front end; nothing to do while anti-aliasing is off or SMAA
    void Render(IDirect3DDevice9* device)
    {
        if (!device || !Temporal())
            return;
        bBridgeStarted = true;
        WFPAA::TemporalAA::SetMode(DepthReady() ? ToMode(*pAntiAliasing) : WFPAA::Mode::Off, *pSharpness / 100.0f);

        IDirect3DSurface9* scene = nullptr;
        IDirect3DSurface9* backBuffer = nullptr;
        device->GetRenderTarget(0, &scene);
        device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
        WFPAA::TemporalAA::Render(device, scene, WFPAA::TemporalAA::DepthTexture(), backBuffer);
        if (scene)
            scene->Release();
        if (backBuffer)
            backBuffer->Release();
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
            options.ClientName = "NFSCarbon.WidescreenFix";
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
            auto api = WFPAA::InstallGraphicsApi(settings.Int("MAIN.GraphicsAPI"), allowD3D9On12);
            bBridgePossible = api == WFPAA::GraphicsApi::Vulkan && WFPAA::TemporalAA::BridgePossible();

            // DLAA and FSR while the hub can offer them, TAA unless the depth can't be read
            settings.Availability("GRAPHICS.AntiAliasing", [](std::string_view value)
            {
                if (value == "2")
                    return bHooksPresent && bResolveHook && !bDepthUnusable;
                if (value == "3")
                    return bHooksPresent && bResolveHook && !bDepthUnusable && BridgeOffers(WFPAA::Backend::DLSS);
                if (value == "4")
                    return bHooksPresent && bResolveHook && !bDepthUnusable && BridgeOffers(WFPAA::Backend::FSR);
                return true;
            });

            WFP::onGameProcessEvent() += []()
            {
                static uint32_t generation = 0;
                if (generation != WFPAA::TemporalAA::Generation())
                {
                    generation = WFPAA::TemporalAA::Generation();
                    FixSettings().Refresh("GRAPHICS.AntiAliasing");
                }
            };

            // eViewPlatInfo::CalculateViewMatricies
            auto pattern = hook::pattern("83 EC 0C 53 55 8B 6C 24 18 56 57 8B 7D 40 33 DB 3B FB 8B F1");
            if (pattern.size() == 1)
                shCalculateViewMatricies = safetyhook::create_inline(pattern.get_first(), CalculateViewMatricies);

            // The render target setup: the device's depth-stencil, just taken with GetDepthStencilSurface. INTZ only while
            // a temporal mode is chosen; a change from off or SMAA takes effect with the next setup (a reset).
            pattern = hook::pattern("68 ? ? ? ? 50 FF 92 A0 00 00 00 E8");
            if (pattern.size() == 1 && shCalculateViewMatricies)
            {
                static auto ppDepthStencil = *pattern.get_first<IDirect3DSurface9**>(1);
                static auto DepthStencilHook = safetyhook::create_mid(pattern.get_first(12), [](SafetyHookContext& regs)
                {
                    if (!Temporal())
                        return;
                    auto original = *ppDepthStencil;
                    auto replacement = WFPAA::TemporalAA::ReplaceDepthStencil(Direct3DDevice, original);
                    bool unusable = !replacement;
                    if (replacement)
                    {
                        if (SUCCEEDED(Direct3DDevice->SetDepthStencilSurface(replacement)))
                        {
                            *ppDepthStencil = replacement;
                            original->Release();
                        }
                        else
                        {
                            replacement->Release();
                            unusable = true;
                        }
                    }
                    if (unusable != bDepthUnusable)
                    {
                        bDepthUnusable = unusable;
                        FixSettings().Refresh("GRAPHICS.AntiAliasing");
                    }
                });
                bHooksPresent = static_cast<bool>(DepthStencilHook);
            }
            settings.Refresh("GRAPHICS.AntiAliasing");

            WFP::onBeforeReset() += []()
            {
                WFPAA::TemporalAA::OnLostDevice();
            };

            WFP::onShutdownEvent() += []()
            {
                WFPAA::TemporalAA::Shutdown();
            };
        };
    }
} AntiAliasingInit;
