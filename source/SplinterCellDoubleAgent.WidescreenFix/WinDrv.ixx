module;

#include <stdafx.h>

export module WinDrv;

import ComVars;
import Xidi;

namespace UWindowsViewport
{
    bool bCalledFromWndProc = false;

    SafetyHookInline shCauseInputEvent = {};
    SafetyHookInline shViewportWndProc = {};
    int32_t __fastcall ViewportWndProc(void* UWindowsViewport, void* edx, UINT Msg, WPARAM wParam, LPARAM lParam)
    {
        hGameWindow = *(HWND*)(*(uintptr_t*)((uintptr_t)UWindowsViewport + 0x230) + 4);

        if (Screen.fRawInputMouse > 0.0f)
        {
            static bool bOnce = false;
            if (!bOnce)
            {
                RawInputHandler<>::RegisterRawInput(hGameWindow, Screen.Width, Screen.Height, Screen.fRawInputMouse);
                bOnce = true;
            }

            if (Msg == WM_MOUSEMOVE)
                return 0;
            else if (Msg == WM_RAWINPUTMOUSE)
            {
                deferredCauseInputEventForRawInput = [UWindowsViewport](int inputID, int a3, float value)
                {
                    shCauseInputEvent.unsafe_fastcall(UWindowsViewport, 0, inputID, a3, value);
                };
            }
        }

        bCalledFromWndProc = true;
        auto ret = shViewportWndProc.unsafe_fastcall<int32_t>(UWindowsViewport, edx, Msg, wParam, lParam);
        bCalledFromWndProc = false;
        return ret;
    }

    void __fastcall CauseInputEvent(void* UWindowsViewport, void* edx, int inputID, int a3, float value)
    {
        if (Screen.bDeferredInput && bCalledFromWndProc)
        {
            deferredCauseInputEvent.emplace([UWindowsViewport, edx, inputID, a3, value]()
            {
                shCauseInputEvent.unsafe_fastcall(UWindowsViewport, edx, inputID, a3, value);
            });
            return;
        }

        return shCauseInputEvent.unsafe_fastcall(UWindowsViewport, edx, inputID, a3, value);
    }

    SafetyHookInline shUpdateRumble = {};
    void __fastcall UpdateRumble(void* UWindowsViewport, void* edx, float vibrateStrength, float shakeStrength, int a4)
    {
        if (!XidiSendVibration)
            return shUpdateRumble.unsafe_fastcall(UWindowsViewport, edx, vibrateStrength, shakeStrength, a4);

        constexpr float RUMBLE_MIN_STRENGTH = 0.0f;
        constexpr float RUMBLE_MAX_STRENGTH = 1.0f;
        constexpr WORD RUMBLE_MAX_VALUE = 0xFFFF;

        const WORD left = static_cast<WORD>(std::clamp(vibrateStrength, RUMBLE_MIN_STRENGTH, RUMBLE_MAX_STRENGTH) * RUMBLE_MAX_VALUE);
        const WORD right = static_cast<WORD>(std::clamp(shakeStrength, RUMBLE_MIN_STRENGTH, RUMBLE_MAX_STRENGTH) * RUMBLE_MAX_VALUE);

        XidiSendVibration(-1, left, right);
    }
}

export void InitWinDrv()
{
    UWindowsViewport::shUpdateRumble = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"WinDrv"), "?UpdateRumble@UWindowsViewport@@UAEXMMH@Z"), UWindowsViewport::UpdateRumble);
    UWindowsViewport::shCauseInputEvent = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"WinDrv"), "?CauseInputEvent@UWindowsViewport@@UAEHHW4EInputAction@@M@Z"), UWindowsViewport::CauseInputEvent);
    UWindowsViewport::shViewportWndProc = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"WinDrv"), "?ViewportWndProc@UWindowsViewport@@QAEJIIJ@Z"), UWindowsViewport::ViewportWndProc);
}
