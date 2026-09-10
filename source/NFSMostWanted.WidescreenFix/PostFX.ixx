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
            static bool bUseMWGammaPreset = nConsoleGammaEnabled == 2;
            CPostFX::bSmaaEnabled = iniReader.ReadInteger("GRAPHICS", "SMAA", 0) != 0;
            CPostFX::bRenderToBackBuffer = true;

            if (!CPostFX::bConsoleGammaEnabled && !CPostFX::bSmaaEnabled)
                return;

            auto pattern = hook::pattern("A1 ? ? ? ? 50 50");
            static auto SMAAHook = safetyhook::create_mid(pattern.get_first(), +[](SafetyHookContext& regs)
            {
                CPostFX::RenderSMAA(Direct3DDevice);
            });

            WFP::onEndScene() += []()
            {
                // MW's own fx defines the MWGamma technique (X360 preset, ini mode 2)
                CPostFX::RenderGamma(Direct3DDevice, bUseMWGammaPreset ? "MWGamma" : nullptr);
            };

            WFP::onBeforeReset() += []()
            {
                CPostFX::Shutdown();
                CPostFX::OnDeviceReset();
            };
        };
    }
} PostFX;
