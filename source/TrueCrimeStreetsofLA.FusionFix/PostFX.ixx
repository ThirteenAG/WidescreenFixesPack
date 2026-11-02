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
    inline void Render()
    {
        IDirect3DDevice9* device = TrueCrimeDevice::Device;
        if (!device)
            return;
        CPostFX::bConsoleGammaEnabled = FixSettings().Bool("GRAPHICS.ConsoleGamma");
        CPostFX::bSmaaEnabled = FixSettings().Int("GRAPHICS.AntiAliasing") == 1;
        if (!CPostFX::bConsoleGammaEnabled && !CPostFX::bSmaaEnabled)
            return;
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
            CPostFX::RenderSMAA(device);
            CPostFX::RenderGamma(device);
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
