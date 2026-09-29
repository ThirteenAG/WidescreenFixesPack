module;

#include <stdafx.h>

export module x_basicmodesmfc;

import ComVars;

export void InitX_BasicModesMFC()
{
    // Menu cursor bounds, see UpdateCursorBounds
    auto pattern = hook::module_pattern(GetModuleHandle(L"X_BasicModesMFC"), "51 68 00 00 20 44 E8 ? ? ? ? 51 D9 1C 24 6A 00 E8 ? ? ? ? D9 5C 24 ? 8B 54 24 ? 52 68 00 00 F0 43 E8 ? ? ? ? 51 D9 1C 24 6A 00 E8"); // X_MenuModeBase::update
    injector::MakeCALL(pattern.get_first(6), ClampCursorRight, true); //0x100060AA
    injector::MakeCALL(pattern.get_first(17), ClampCursorLeft, true);
    injector::MakeCALL(pattern.get_first(36), ClampCursorBottom, true);
    injector::MakeCALL(pattern.get_first(47), ClampCursorTop, true);

    //screenshots aspect ratio
    pattern = hook::module_pattern(GetModuleHandle(L"X_BasicModesMFC"), "A1 ? ? ? ? 8B 0D ? ? ? ? 89 8E ? ? ? ? 89 86");
    static auto dword_100451A8 = *pattern.get_first<float*>(1);
    struct SaveScrHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)&regs.eax = *dword_100451A8 * ((4.0f / 3.0f) / Screen.fAspectRatio);
        }
    }; injector::MakeInline<SaveScrHook>(pattern.get_first(0)); //10007684
}