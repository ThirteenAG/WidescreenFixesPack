module;

#include <stdafx.h>

export module x_helpersmfc;

import ComVars;

export void InitX_HelpersMFC()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"X_HelpersMFC"), "8B 41 08 8B 49 0C 50");
    struct X_QuadRendererRenderHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.ecx + 0x08);
            regs.ecx = *(uint32_t*)(regs.ecx + 0x0C);
            Screen.bIsX_QuadRenderer = true;
        }
    }; injector::MakeInline<X_QuadRendererRenderHook>(pattern.get_first(0), pattern.get_first(6)); //10004820

    pattern = hook::module_pattern(GetModuleHandle(L"X_HelpersMFC"), "05 58 01 00 00 33 D2");
    struct X_ProgressBarUpdateProgressBarHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax += 0x158;
            Screen.bDrawBorders = true;
        }
    }; injector::MakeInline<X_ProgressBarUpdateProgressBarHook>(pattern.get_first(0)); //10002FF0
}