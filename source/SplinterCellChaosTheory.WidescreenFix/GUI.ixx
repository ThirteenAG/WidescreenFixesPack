module;

#include <stdafx.h>

export module GUI;

import ComVars;

export void InitGUI()
{
    auto pattern = hook::pattern("3B C5 89 44 24 44 7D ? 66 89 6C 24 40");
    if (!pattern.empty())
    {
        struct UGUIControllerNativePostRenderHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uint32_t*)(regs.esp + 0x44) = regs.eax;

                int16_t delta = int16_t(((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f);
                *(int16_t*)(regs.esp + 0x40) = std::clamp(*(int16_t*)&regs.eax, int16_t(0 - delta + 4), int16_t(640 + delta + 4));
            }
        }; injector::MakeInline<UGUIControllerNativePostRenderHook>(pattern.get_first(0), pattern.get_first(33));
    }
}
