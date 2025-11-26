module;

#include <stdafx.h>

export module x_basicmodesmfc;

import ComVars;

export void InitX_BasicModesMFC()
{
    //screenshots aspect ratio
    auto pattern = hook::module_pattern(GetModuleHandle(L"X_BasicModesMFC"), "A1 ? ? ? ? 8B 0D ? ? ? ? 89 8E ? ? ? ? 89 86");
    static auto dword_100451A8 = *pattern.get_first<float*>(1);
    struct SaveScrHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)&regs.eax = *dword_100451A8 * ((4.0f / 3.0f) / Screen.fAspectRatio);
        }
    }; injector::MakeInline<SaveScrHook>(pattern.get_first(0)); //10007684
}