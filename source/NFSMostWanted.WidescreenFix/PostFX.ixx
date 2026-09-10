module;

#include <stdafx.h>
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
            CIniReader iniReader("");
            auto nConsoleGammaEnabled = std::clamp(iniReader.ReadInteger("GRAPHICS", "ConsoleGamma", 1), 0, 2);
            CPostFX::bConsoleGammaEnabled = nConsoleGammaEnabled != 0;
            CPostFX::bUseMWGammaTechnique = nConsoleGammaEnabled == 2;
            CPostFX::bSmaaEnabled = iniReader.ReadInteger("GRAPHICS", "SMAA", 0) != 0;
            CPostFX::bRenderToBackBuffer = true;

            if (!CPostFX::bConsoleGammaEnabled && !CPostFX::bSmaaEnabled)
                return;

            WFP::onEndScene() += []()
            {
                CPostFX::RenderSMAA(Direct3DDevice);
                CPostFX::RenderGamma(Direct3DDevice);
            };

            WFP::onBeforeReset() += []()
            {
                CPostFX::Shutdown();
                CPostFX::OnDeviceReset();
            };

            WFP::onShutdownEvent() += []()
            {
                CPostFX::ShutdownProcess();
            };
        };
    }
} PostFX;
