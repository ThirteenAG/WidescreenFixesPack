module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>

export module PostFX;

import ComVars;
import PostFXCore;
import AntiAliasing;

class PostFX
{
public:
    PostFX()
    {
        WFP::onInitEventAsync() += []()
        {
            // Both can be switched in the game's video options
            static const int* pConsoleGamma = &FixSettings().Int("GRAPHICS.ConsoleGamma");
            CPostFX::bConsoleGammaEnabled = *pConsoleGamma != 0;
            CPostFX::bSmaaEnabled = AntiAliasing::IsSMAA();
            CPostFX::bRenderToBackBuffer = true;

            auto pattern = hook::pattern("89 3D ? ? ? ? 74 ? 6A");
            // Before the front end (the HUD): SMAA, or temporal anti-aliasing, which runs every frame (it also starts
            // FusionFixHub's bridge)
            bool uniqueResolve = pattern.size() == 1;
            static auto SMAAHook = uniqueResolve ? safetyhook::create_mid(pattern.get_first(), +[](SafetyHookContext& regs)
            {
                CPostFX::RenderSMAA(Direct3DDevice);
                AntiAliasing::Render(Direct3DDevice);
            }) : SafetyHookMid{};
            AntiAliasing::SetResolveAvailable(uniqueResolve && bool(SMAAHook));

            WFP::onEndScene() += []()
            {
                CPostFX::bConsoleGammaEnabled = *pConsoleGamma != 0;
                if (CPostFX::bSmaaEnabled != AntiAliasing::IsSMAA())
                {
                    // The SMAA textures are made when it's on: start over
                    CPostFX::Shutdown();
                    CPostFX::bSmaaEnabled = AntiAliasing::IsSMAA();
                }
                CPostFX::RenderGamma(Direct3DDevice);
            };

            WFP::onBeforeReset() += []()
            {
                CPostFX::Shutdown();
                CPostFX::OnDeviceReset();
            };
        };
    }
} PostFX;
