module;

#include "stdafx.h"
#include <d3d9.h>

export module PostFX;

import PostFXCore;

IDirect3DDevice9* pDevice = nullptr;

injector::hook_back<void(*)()> hb_sub_654B20;
void sub_654B20()
{
    CPostFX::Shutdown();
    CPostFX::OnDeviceReset();
    return hb_sub_654B20.fun();
}

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

            auto pattern = hook::pattern("88 87 ? ? ? ? 8B 47");
            static auto PresentHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                pDevice = *(IDirect3DDevice9**)(regs.edi + 0x10);
            });

            pattern = hook::pattern("E8 ? ? ? ? 8B 4C 24 ? 8B 54 24 ? ? ? 83 C4");
            static auto DrawHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&)
            {
                if (!pDevice)
                    return;

                CPostFX::RenderSMAA(pDevice);
                CPostFX::RenderGamma(pDevice);
            });

            pattern = hook::pattern("8B 46 ? ? ? 50 FF 51 ? 3D");
            static auto ResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&)
            {
                CPostFX::Shutdown();
                CPostFX::OnDeviceReset();
            });

            pattern = hook::pattern("75 26 E8 ? ? ? ? 8B 86 ? ? ? ?");
            hb_sub_654B20.fun = injector::MakeCALL(pattern.get_first(2), sub_654B20, true).get();
            pattern = hook::pattern("E8 ? ? ? ? 8B 06 8B 08 53");
            hb_sub_654B20.fun = injector::MakeCALL(pattern.get_first(0), sub_654B20, true).get();
        };

        WFP::onShutdownEvent() += []()
        {
            CPostFX::Shutdown();
            pDevice = nullptr;
        };
    }
} PostFX;
