module;

#include <stdafx.h>

export module GUI;

import ComVars;

export void InitGUI()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"GUI"), "85 C0 79 ? 33 DB EB 0A BB 80 02 00 00 3B C3 0F 4C D8");
    if (!pattern.empty())
    {
        struct UGUIControllerNativePostRenderHook
        {
            void operator()(injector::reg_pack& regs)
            {
                int32_t delta = int32_t(((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f);
                *(int32_t*)&regs.ebx = std::clamp(int32_t(regs.eax), 0 - delta + 4, 640 + delta + 4);
            }
        }; injector::MakeInline<UGUIControllerNativePostRenderHook>(pattern.get_first(0), pattern.get_first(18));
    }
}
