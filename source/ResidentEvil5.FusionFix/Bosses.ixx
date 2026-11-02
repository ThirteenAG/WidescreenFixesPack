module;

#include <stdafx.h>

export module Bosses;

import System;

export namespace Bosses
{
    void Register()
    {
        WFP::onInitEvent() += []()
        {
            auto pattern = hook::pattern("80 BE B6 20 00 00 00 F3 0F 10 8E 48 21 00 00 F3 0F 58 C8");
            static SafetyHookMid FireChargeHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                const float delta = System::FrameDelta();
                if (delta > 0.0f)
                    regs.xmm0.f32[0] /= delta;
            });
        };
    }
}
