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
            // Both can be switched in the game's options
            static const int* pConsoleGamma = &FixSettings().Int("GRAPHICS.ConsoleGamma");
            CPostFX::bConsoleGammaEnabled = *pConsoleGamma != 0;
            CPostFX::bSmaaEnabled = AntiAliasing::IsSMAA();
            CPostFX::bRenderToBackBuffer = true;

            auto pattern = hook::pattern("A1 ? ? ? ? 85 C0 0F 84 ? ? ? ? 6A 03");
            bool uniqueResolve = pattern.size() == 1;
            static auto SMAAHook = uniqueResolve ? safetyhook::create_mid(pattern.get_first(), +[](SafetyHookContext& regs)
            {
                CPostFX::RenderSMAA(Direct3DDevice);
                AntiAliasing::Render(Direct3DDevice);
            }) : SafetyHookMid{};
            AntiAliasing::SetResolveAvailable(uniqueResolve && static_cast<bool>(SMAAHook));

            WFP::onEndScene() += []()
            {
                CPostFX::bConsoleGammaEnabled = *pConsoleGamma != 0;
                if (CPostFX::bSmaaEnabled != (AntiAliasing::IsSMAA()))
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
