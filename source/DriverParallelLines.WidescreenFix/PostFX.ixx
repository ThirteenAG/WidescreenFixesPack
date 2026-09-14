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
            CPostFX::bConsoleGammaEnabled = iniReader.ReadInteger("GRAPHICS", "ConsoleGamma", 0) != 0;
            CPostFX::bSmaaEnabled = iniReader.ReadInteger("GRAPHICS", "SMAA", 0) != 0;

            if (!CPostFX::bConsoleGammaEnabled && !CPostFX::bSmaaEnabled)
                return;

            auto pattern = hook::pattern("8B 88 ? ? ? ? ? ? ? ? ? ? 8B 45");
            static auto SMAAHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (!Direct3DDevice || *(uint32_t*)(regs.ebp - 4) != 3)
                    return;

                CPostFX::RenderSMAA(Direct3DDevice);
                CPostFX::RenderGamma(Direct3DDevice);
            });

            WFP::onBeforeReset() += []()
            {
                CPostFX::Shutdown();
                CPostFX::OnDeviceReset();
            };
        };
    }
} PostFX;
