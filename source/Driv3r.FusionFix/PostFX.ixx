module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>

export module PostFX;

import ComVars;
import PostFXCore;

class PostFX
{
public:
    PostFX()
    {
        WFP::onInitEvent() += []()
        {

            CPostFX::bConsoleGammaEnabled = FixSettings().Bool("GRAPHICS.ConsoleGamma") != 0;
            CPostFX::bSmaaEnabled = FixSettings().Int("GRAPHICS.AntiAliasing") != 0;

            auto pattern = hook::pattern("75 ? A1 ? ? ? ? ? ? 6A ? 6A ? 50 FF 91 ? ? ? ? A1");
            static auto SMAAHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (!Direct3DDevice || regs.ebp != (5 - 1))
                    return;

                if (!CPostFX::bConsoleGammaEnabled && !CPostFX::bSmaaEnabled)
                    return;

                auto device = Direct3DDevice.get();
                IDirect3DStateBlock9* state = nullptr;
                if (FAILED(device->CreateStateBlock(D3DSBT_ALL, &state))) return;
                if (FAILED(state->Capture())) { state->Release(); return; }
                D3DCAPS9 capabilities{};
                device->GetDeviceCaps(&capabilities);
                const DWORD count = std::clamp<DWORD>(capabilities.NumSimultaneousRTs, 1, 4);
                std::array<IDirect3DSurface9*, 4> targets{};
                IDirect3DSurface9* depth = nullptr;
                if (FAILED(device->GetRenderTarget(0, &targets[0]))) { state->Release(); return; }
                for (DWORD i = 1; i < count; ++i) device->GetRenderTarget(i, &targets[i]);
                device->GetDepthStencilSurface(&depth);
                // Change device state without invalidating the engine's cached texture bindings.
                for (DWORD i = 1; i < count; ++i) device->SetRenderTarget(i, nullptr);
                device->SetDepthStencilSurface(nullptr);
                for (DWORD i = 0; i < 4; ++i) device->SetTexture(i, nullptr);
                CPostFX::RenderSMAA(device);
                CPostFX::RenderGamma(device);
                for (DWORD i = 0; i < count; ++i) device->SetRenderTarget(i, targets[i]);
                device->SetDepthStencilSurface(depth);
                state->Apply(); state->Release();
                for (auto target : targets) if (target) target->Release();
                if (depth) depth->Release();
            });

            WFP::onBeforeReset() += []()
            {
                CPostFX::Shutdown();
                CPostFX::OnDeviceReset();
            };
        };
    }
} PostFX;
