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

            static auto dw8AFA60 = *hook::get_pattern<uintptr_t*>("A1 ? ? ? ? 8B 08 6A 00 6A 00 50 FF 91", 1);

            auto pattern = hook::pattern("75 ? A1 ? ? ? ? ? ? 6A ? 6A ? 50 FF 91 ? ? ? ? A1");
            static auto SMAAHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (!Direct3DDevice || regs.ebp != (5 - 1))
                    return;

                (*(void(__stdcall**)(int, int, int))(*(int*)*(int*)dw8AFA60 + 260))(*(int*)dw8AFA60, 0, 0);
                (*(void(__stdcall**)(int, int, int))(*(int*)*(int*)dw8AFA60 + 260))(*(int*)dw8AFA60, 1, 0);
                (*(void(__stdcall**)(int, int, int))(*(int*)*(int*)dw8AFA60 + 260))(*(int*)dw8AFA60, 2, 0);
                (*(void(__stdcall**)(int, int, int))(*(int*)*(int*)dw8AFA60 + 260))(*(int*)dw8AFA60, 3, 0);

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
