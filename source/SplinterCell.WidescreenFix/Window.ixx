module;

#include <stdafx.h>

export module Window;

import ComVars;

export void InitWindow()
{
    auto pattern = find_module_pattern(GetModuleHandle(L"Window"), "85 C0 75 ? 50 FF 15 ? ? ? ? 83 C4 04 8D A5 D0 FB FF FF");
    static auto CreateWindowWHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        hGameWindow = (HWND)regs.eax;
        if (Screen.fRawInputMouseForMenu > 0.0f)
            RawInputHandler<int32_t>::RegisterRawInput(hGameWindow, Screen.Width, Screen.Height, Screen.fRawInputMouseForMenu);
    });
}
