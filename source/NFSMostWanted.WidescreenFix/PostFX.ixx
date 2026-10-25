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

            auto pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? 57 E8 ? ? ? ? A1 ? ? ? ? 8B 10 83 C4 04 50 FF 92 A8 00 00 00");
            bool uniqueResolve = pattern.size() == 1;
            static auto SMAAHook = uniqueResolve ? safetyhook::create_mid(pattern.get_first(10), +[](SafetyHookContext& regs)
            {
                CPostFX::RenderSMAA(Direct3DDevice);
                AntiAliasing::Render(Direct3DDevice);
            }) : SafetyHookMid{};
            AntiAliasing::SetResolveAvailable(uniqueResolve && bool(SMAAHook));

            WFP::onEndScene() += []()
            {
                CPostFX::bConsoleGammaEnabled = *pConsoleGamma != 0;
                if (CPostFX::bSmaaEnabled != (AntiAliasing::IsSMAA()))
                {
                    // The SMAA textures are made when it's on: start over
                    CPostFX::Shutdown();
                    CPostFX::bSmaaEnabled = AntiAliasing::IsSMAA();
                }

                // MW's own fx defines the MWGamma technique (X360 preset, cfg mode 2)
                CPostFX::RenderGamma(Direct3DDevice, *pConsoleGamma == 2 ? "MWGamma" : nullptr);
            };

            WFP::onBeforeReset() += []()
            {
                CPostFX::Shutdown();
                CPostFX::OnDeviceReset();
            };
        };
    }
} PostFX;
