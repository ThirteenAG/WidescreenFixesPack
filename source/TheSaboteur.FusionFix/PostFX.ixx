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

            auto pattern = hook::pattern("83 05 ? ? ? ? 01 F7 47 08 00 00 00 80 8B CE");
            static auto BeforeRenderPassHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                static constexpr std::string_view UIPasses[] = { "ClamberingHUDDrawList", "ClamberingHUDBucket", "Flash Bucket", "Debug" };
                auto szPassName = (const char*)(regs.edi + 0x0C);
                if (std::find(std::begin(UIPasses), std::end(UIPasses), szPassName) == std::end(UIPasses))
                    return;

                static constexpr auto nDeviceOffset = 24424;
                Direct3DDevice = *(IDirect3DDevice9**)(regs.esi + nDeviceOffset);
                if (Direct3DDevice)
                    CPostFX::RenderSMAA(Direct3DDevice);
            });

            WFP::onEndScene() += []()
            {
                if (Direct3DDevice)
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
