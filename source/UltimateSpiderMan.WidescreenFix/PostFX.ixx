module;

#include "stdafx.h"
#include <d3d9.h>

export module PostFX;

import ComVars;
import PostFXCore;

namespace
{
    void (__cdecl* pBeginScene)(int) = nullptr;
    void (__cdecl* pEndScene)() = nullptr;
    void (__cdecl* pSetClearFlags)(int) = nullptr;
    void (__cdecl* pAddCustomNode)(void(__cdecl*)(void*), void*, const uint32_t*) = nullptr;

    void __cdecl RenderPostFX(void*)
    {
        IDirect3DDevice9* device = Direct3DDevice;
        if (!device)
            return;

        // NGL executes this pass after the world, before the UI scenes.
        // Preserve D3D state because NGL caches state across frames.
        IDirect3DStateBlock9* state = nullptr;
        if (FAILED(device->CreateStateBlock(D3DSBT_ALL, &state)))
            return;

        IDirect3DSurface9* target = nullptr;
        IDirect3DSurface9* depth = nullptr;
        D3DVIEWPORT9 viewport{};
        if (SUCCEEDED(device->GetRenderTarget(0, &target)) &&
            SUCCEEDED(device->GetViewport(&viewport)))
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
        if (depth) depth->Release();
        if (target) target->Release();
        state->Release();
    }
}

class PostFX
{
public:
    PostFX()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            CPostFX::bConsoleGammaEnabled = iniReader.ReadInteger("GRAPHICS", "ConsoleGamma", 0) != 0;
            CPostFX::bSmaaEnabled = iniReader.ReadInteger("GRAPHICS", "SMAA", 0) != 0;
            if (!CPostFX::bConsoleGammaEnabled && !CPostFX::bSmaaEnabled)
                return;

            CPostFX::bRenderToBackBuffer = false;
            auto pattern = hook::pattern("6A 01 E8 ? ? ? ? 8A 15 ? ? ? ? 33 C0"); //0x52B284 + 2
            pBeginScene = reinterpret_cast<decltype(pBeginScene)>(injector::GetBranchDestination(pattern.get_first(2)).as_int());
            pattern = hook::pattern("E8 ? ? ? ? A0 ? ? ? ? 84 C0 75 2A"); //0x52B2A5
            pEndScene = reinterpret_cast<decltype(pEndScene)>(injector::GetBranchDestination(pattern.get_first()).as_int());
            pattern = hook::pattern("50 E8 ? ? ? ? 83 C4 08 E8 ? ? ? ? A0"); //0x52B29C + 1
            pSetClearFlags = reinterpret_cast<decltype(pSetClearFlags)>(injector::GetBranchDestination(pattern.get_first(1)).as_int());
            pattern = hook::pattern("6A 00 E8 ? ? ? ? A1 ? ? ? ? 83 C0 0F 83 C4 04 83 E0 F0 8D 48 20"); //0x76C3A0
            pAddCustomNode = reinterpret_cast<decltype(pAddCustomNode)>(pattern.get_first());
            // Separate pass before game::render_ui creates its first scene.
            pattern = hook::pattern("81 EC 88 01 00 00 56 6A 01 8B F1"); //0x52B265
            static auto BeforeUIHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&)
            {
                pBeginScene(1);
                pSetClearFlags(0);
                const uint32_t sortInfo[2] = { 0, 0 };
                pAddCustomNode(RenderPostFX, nullptr, sortInfo);
                pEndScene();
            });

            WFP::onBeforeReset() += []()
            {
                CPostFX::Shutdown();
                CPostFX::OnDeviceReset();
            };

            WFP::onShutdownEvent() += []()
            {
                CPostFX::Shutdown();
            };
        };
    }
} PostFX;
