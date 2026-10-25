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
            auto& settings = FixSettings();
            CPostFX::bConsoleGammaEnabled = settings.Int("GRAPHICS.ConsoleGamma") != 0;
            CPostFX::bSmaaEnabled = settings.Int("GRAPHICS.AntiAliasing") != 0;

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
