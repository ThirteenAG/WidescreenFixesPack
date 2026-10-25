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
        WFP::onInitEventAsync() += []()
        {
            // Both can be switched in the game's options
            static const int* pConsoleGamma = &FixSettings().Int("GRAPHICS.ConsoleGamma");
            static const int* pSmaa = &FixSettings().Int("GRAPHICS.AntiAliasing");
            CPostFX::bConsoleGammaEnabled = *pConsoleGamma != 0;
            CPostFX::bSmaaEnabled = *pSmaa != 0;
            CPostFX::bRenderToBackBuffer = true;

            WFP::onEndScene() += []()
            {
                CPostFX::bConsoleGammaEnabled = *pConsoleGamma != 0;
                if (CPostFX::bSmaaEnabled != (*pSmaa != 0))
                {
                    // The SMAA textures are made when it's on: start over
                    CPostFX::Shutdown();
                    CPostFX::bSmaaEnabled = *pSmaa != 0;
                }
                CPostFX::RenderSMAA(*Direct3DDevice);
                CPostFX::RenderGamma(*Direct3DDevice);
            };

            WFP::onBeforeReset() += []()
            {
                CPostFX::Shutdown();
                CPostFX::OnDeviceReset();
            };
        };
    }
} PostFX;
