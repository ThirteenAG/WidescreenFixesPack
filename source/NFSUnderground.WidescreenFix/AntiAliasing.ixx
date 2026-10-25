module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include "../Shared/AA/AntiAliasing.hpp"

export module AntiAliasing;

import ComVars;

// eViewPlatInfo::CalculateViewMatricies (US 1.4: 0x40DD00) builds the view at +0, projection at +64 and their
// product at +128. Capture after the main-loop call, for eView 1 only; headlights, mirrors and reflections keep
// their own projections. The existing PostFX boundary follows the world's post processing and precedes the HUD.
// The default depth is acquired by the target setup (0x40A890), then shared by target descriptors 0, 1 and 3.
// Bind the INTZ replacement before those descriptors acquire it, so the renderer and the resolve use the same depth.

namespace
{
    const int* pAntiAliasing = nullptr;
    const float* pSharpness = nullptr;
    std::atomic<bool> bHooksPresent = false;
    std::atomic<bool> bResolveHook = false;
    std::atomic<bool> bDepthUnusable = false;
    float* jitteredMatrices = nullptr;
    std::array<float, 32> cleanProjectionProduct{};

    void RestoreCamera()
    {
        if (jitteredMatrices)
        {
            std::memcpy(jitteredMatrices + 16, cleanProjectionProduct.data(), sizeof(cleanProjectionProduct));
            jitteredMatrices = nullptr;
        }
    }

    WFPAA::Mode ToMode(int value)
    {
        if (value == 2) return WFPAA::Mode::TAA;
        if (value == 3) return WFPAA::Mode::DLAA;
        if (value == 4) return WFPAA::Mode::FSR;
        return WFPAA::Mode::Off;
    }

    void Multiply(float* out, const float* a, const float* b)
    {
        float result[16];
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                result[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j] + a[i * 4 + 3] * b[12 + j];
        std::memcpy(out, result, sizeof(result));
    }

    bool DepthReady()
    {
        return bHooksPresent && bResolveHook && !bDepthUnusable && WFPAA::TemporalAA::DepthTexture();
    }

    bool Temporal()
    {
        return pAntiAliasing && *pAntiAliasing >= 2;
    }

    // DLAA and FSR: what the hub reported once it runs; before that, whether it can run at all
    bool bBridgeStarted = false;
    bool bBridgePossible = false;
    bool BridgeOffers(WFPAA::Backend backend)
    {
        return WFPAA::TemporalAA::IsAvailable(backend) || (!bBridgeStarted && bBridgePossible);
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
        return pAntiAliasing && *pAntiAliasing == 1;
    }

    // Called at the same pre-HUD boundary as SMAA; nothing to do while anti-aliasing is off or SMAA
    void Render(IDirect3DDevice9* device)
    {
        if (!device || !Temporal())
            return;
        bBridgeStarted = true;
        WFPAA::TemporalAA::SetMode(DepthReady() ? ToMode(*pAntiAliasing) : WFPAA::Mode::Off, *pSharpness / 100.0f);
        IDirect3DSurface9* scene = nullptr;
        IDirect3DSurface9* output = nullptr;
        device->GetRenderTarget(0, &scene);
        device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &output);
        WFPAA::TemporalAA::Render(device, scene, WFPAA::TemporalAA::DepthTexture(), output);
        // World submissions have consumed the jitter. The native UI uses the same view cache.
        RestoreCamera();
        if (scene) scene->Release();
        if (output) output->Release();
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
            options.ClientName = "NFSUnderground.WidescreenFix";
            options.LowDynamicRange = true;
            WFPAA::TemporalAA::Configure(options);

            static bool allowD3D9On12 = settings.Bool("GRAPHICS.D3D9on12");
            settings.Availability("MAIN.GraphicsAPI", [](std::string_view value)
            {
                if (value == "1") return WFPAA::VulkanAvailable();
                if (value == "2") return allowD3D9On12;
                return true;
            });
            auto api = WFPAA::InstallGraphicsApi(settings.Int("MAIN.GraphicsAPI"), allowD3D9On12);
            bBridgePossible = api == WFPAA::GraphicsApi::Vulkan && WFPAA::TemporalAA::BridgePossible();

            auto camera = hook::pattern("57 E8 ? ? ? ? 43 83 FB 0F 7E ? A1 ? ? ? ? 8B 08 50 FF 51 0C");
            auto depth = hook::pattern("68 ? ? ? ? 50 FF 91 A0 00 00 00 A1 ? ? ? ? 8B 10 68");
            bHooksPresent = camera.size() == 1 && depth.size() == 1;

            settings.Availability("GRAPHICS.AntiAliasing", [](std::string_view value)
            {
                if (value == "2") return bHooksPresent && bResolveHook && !bDepthUnusable;
                if (value == "3") return bHooksPresent && bResolveHook && !bDepthUnusable && BridgeOffers(WFPAA::Backend::DLSS);
                if (value == "4") return bHooksPresent && bResolveHook && !bDepthUnusable && BridgeOffers(WFPAA::Backend::FSR);
                return true;
            });
            if (bHooksPresent)
            {
                static auto CameraHook = safetyhook::create_mid(camera.get_first(6), [](SafetyHookContext& regs)
                {
                    auto view = reinterpret_cast<uint8_t*>(regs.edi);
                    if (*reinterpret_cast<int32_t*>(view + 4) != 1)
                        return;
                    // The native calculation just rebuilt this frame's clean projection and product.
                    jitteredMatrices = nullptr;
                    if (!Temporal() || !*reinterpret_cast<void**>(view + 64) || !DepthReady())
                        return;
                    auto target = *reinterpret_cast<uint8_t**>(view + 88);
                    if (!target) return;
                    WFPAA::Camera camera;
                    camera.View = WFPAA::Matrix::From(reinterpret_cast<float*>(regs.esi));
                    camera.Projection = WFPAA::Matrix::From(reinterpret_cast<float*>(regs.esi + 64));
                    camera.Width = *reinterpret_cast<uint32_t*>(target + 20);
                    camera.Height = *reinterpret_cast<uint32_t*>(target + 24);
                    WFPAA::TemporalAA::SetMode(ToMode(*pAntiAliasing), *pSharpness / 100.0f);
                    auto jitter = WFPAA::TemporalAA::OnCamera(camera);
                    if (jitter.X == 0.0f && jitter.Y == 0.0f) return;
                    auto matrix = reinterpret_cast<float*>(regs.esi);
                    jitteredMatrices = matrix;
                    std::memcpy(cleanProjectionProduct.data(), matrix + 16, sizeof(cleanProjectionProduct));
                    WFPAA::JitterProjection(matrix + 16, jitter, static_cast<float>(camera.Width), static_cast<float>(camera.Height));
                    Multiply(matrix + 32, matrix, matrix + 16);
                });

                static auto ppDepth = *depth.get_first<IDirect3DSurface9**>(1);
                // The game takes its depth-stencil here once (and after a reset): INTZ only while a temporal mode is chosen
                static auto DepthHook = safetyhook::create_mid(depth.get_first(12), [](SafetyHookContext& regs)
                {
                    if (!Temporal())
                        return;
                    auto original = *ppDepth;
                    auto replacement = WFPAA::TemporalAA::ReplaceDepthStencil(Direct3DDevice, original);
                    bool unusable = !replacement;
                    if (replacement)
                    {
                        if (SUCCEEDED(Direct3DDevice->SetDepthStencilSurface(replacement)))
                        {
                            *ppDepth = replacement;
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
                bHooksPresent = static_cast<bool>(CameraHook) && static_cast<bool>(DepthHook);
                settings.Refresh("GRAPHICS.AntiAliasing");
            }

            WFP::onGameProcessEvent() += []()
            {
                static uint32_t generation = 0;
                if (generation != WFPAA::TemporalAA::Generation())
                {
                    generation = WFPAA::TemporalAA::Generation();
                    FixSettings().Refresh("GRAPHICS.AntiAliasing");
                }
            };
            WFP::onBeforeReset() += []() { RestoreCamera(); WFPAA::TemporalAA::OnLostDevice(); };
            WFP::onShutdownEvent() += []() { WFPAA::TemporalAA::Shutdown(); };
        };
    }
} AntiAliasingInit;
