module;

#include <stdafx.h>
#include "common.h"
#include "callargs.h"
#include <d3d9.h>

export module TransparentMenu;

import Skeleton;
import Menu;
import TransparentMenuDX9;

export bool gTransparentMenuCanRender = false;
export bool gTransparentMenuWaitForGameplay = false;

injector::hook_back<void(__cdecl*)(CRect*, uint8_t*)> hbDrawRect2;
void __cdecl CSprite2d__DrawRect2(CRect* a1, uint8_t* a2)
{
    if (gTransparentMenuCanRender)
    {
        if ((*pD3D9Device))
            TransparentMenuDX9::RenderBlur();
    }
    else
        return hbDrawRect2.fun(a1, a2);
}

class TransparentMenu
{
public:
    TransparentMenu()
    {
        WFP::onGameInitEvent() += []()
        {
            CIniReader iniReader("");
            static int bTransparentMenu = iniReader.ReadInteger("GRAPHICS", "TransparentMenu", 0);

            if (bTransparentMenu <= 0)
                return;

            auto pattern = hook::pattern("E8 ? ? ? ? 0F B6 15 ? ? ? ? 50");
            static auto CRGBA = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                using tCRGBA = void(__thiscall*)(void*, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
                auto [crgba, r, g, b, a] = deduce_args<tCRGBA>(regs);

                if (gTransparentMenuCanRender)
                    a = 0;
            });

            pattern = hook::pattern("E8 ? ? ? ? A0 ? ? ? ? 83 C4 ? 84 C0 74 ? 68");
            hbDrawRect2.fun = injector::MakeCALL(pattern.get_first(), CSprite2d__DrawRect2, true).get();

            pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? 53 53 53 53 53 53 E8 ? ? ? ? E8");
            static auto captureHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&)
            {
                if (*(int*)0xC8D4C0 < 9)
                    return;

                if ((*pD3D9Device))
                    gTransparentMenuCanRender = TransparentMenuDX9::CaptureFrame();
            });

            WFP::onBeforeReset() += []()
            {
                gTransparentMenuCanRender = false;
                gTransparentMenuWaitForGameplay = true;

                TransparentMenuDX9::OnDeviceReset();
            };
        };
    }
} TransparentMenu;