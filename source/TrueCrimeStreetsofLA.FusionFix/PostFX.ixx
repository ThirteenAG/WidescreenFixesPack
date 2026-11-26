module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>

export module PostFX;

import Device;
import Hud;
import PostFXCore;

// SMAA and the Xbox 360 gamma curve on the finished 3D picture, before the HUD and the story-scene bars
export namespace TrueCrimePostFX
{
    inline constexpr uint8_t Gamma = 1, Smaa = 2;
    inline std::atomic<uint8_t> Effects{ 0 };

    inline void UpdateEffects()
    {
        static std::mutex mutex;
        const std::lock_guard lock(mutex);
        auto& settings = FixSettings();
        Effects.store((settings.Get("GRAPHICS.ConsoleGamma") == "1" ? Gamma : 0) |
            (settings.Get("GRAPHICS.AntiAliasing") == "1" ? Smaa : 0), std::memory_order_relaxed);
    }

    inline void Render()
    {
        const auto effects = Effects.load(std::memory_order_relaxed);
        if (!effects)
            return;
        const auto device = TrueCrimeDevice::RenderDevice();
        if (!device)
            return;
        CPostFX::bConsoleGammaEnabled = (effects & Gamma) != 0;
        CPostFX::bSmaaEnabled = (effects & Smaa) != 0;
        IDirect3DStateBlock9* state = nullptr;
        if (FAILED(device->CreateStateBlock(D3DSBT_ALL, &state)))
            return;
        IDirect3DSurface9* target = nullptr;
        IDirect3DSurface9* depth = nullptr;
        D3DVIEWPORT9 viewport{};
        if (SUCCEEDED(device->GetRenderTarget(0, &target)) && SUCCEEDED(device->GetViewport(&viewport)))
        {
            device->GetDepthStencilSurface(&depth);
            device->SetDepthStencilSurface(nullptr);
            CPostFX::RenderSMAA(device.Get());
            CPostFX::RenderGamma(device.Get());
            device->SetRenderTarget(0, target);
            device->SetDepthStencilSurface(depth);
            device->SetViewport(&viewport);
        }
        state->Apply();
        if (depth)
            depth->Release();
        if (target)
            target->Release();
        state->Release();
    }

    inline void Init()
    {
        FixSettings().OnChange("GRAPHICS.ConsoleGamma", UpdateEffects);
        FixSettings().OnChange("GRAPHICS.AntiAliasing", UpdateEffects);
        UpdateEffects();
        CPostFX::bRenderToBackBuffer = false;
        TrueCrimeHud::BeforeDraw = Render;
        WFP::onBeforeReset() += []()
        {
            CPostFX::Shutdown();
            CPostFX::OnDeviceReset();
        };
        WFP::onShutdownEvent() += []() { CPostFX::Shutdown(); };
        TrueCrimeDevice::Releasing.push_back([]() { CPostFX::ReleaseDevice(); });
    }
}
