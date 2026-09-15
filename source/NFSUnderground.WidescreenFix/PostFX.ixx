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
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            CPostFX::bConsoleGammaEnabled = iniReader.ReadInteger("GRAPHICS", "ConsoleGamma", 1) != 0;
            CPostFX::bSmaaEnabled = iniReader.ReadInteger("GRAPHICS", "SMAA", 0) != 0;
            CPostFX::bRenderToBackBuffer = true;

            if (!CPostFX::bConsoleGammaEnabled && !CPostFX::bSmaaEnabled)
                return;

            //auto pattern = hook::pattern("85 C0 74 ? 8D 70 ? 8B 40");
            //static auto SMAAHook = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
            //{
            //    CPostFX::RenderSMAA(Direct3DDevice);
            //});

            WFP::onEndScene() += []()
            {
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
