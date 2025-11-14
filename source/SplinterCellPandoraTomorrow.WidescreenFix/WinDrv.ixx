module;

#include <stdafx.h>
#include <functional>
#include <unordered_set>

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
        hGameWindow = *(HWND*)(*(uintptr_t*)((uintptr_t)UWindowsViewport + 0x18C) + 4);

        if (Screen.fRawInputMouse > 0.0f)
        {
            static bool bOnce = false;
            if (!bOnce)
            {
                RawInputHandler<int32_t>::RegisterRawInput(hGameWindow, Screen.fRawInputMouse, Screen.bRawInputMouseRawData);
                bOnce = true;
            }

            if (Msg == WM_MOUSEMOVE)
            {
                return 0;
            }
            else if (Msg == WM_RAWINPUTMOUSE)
            {
                if (hGameWindow != GetForegroundWindow())
                    return 0;

                deferredCauseInputEventForRawInput = [UWindowsViewport](int inputID, int a3, float value)
                {
                    shCauseInputEvent.unsafe_fastcall(UWindowsViewport, 0, inputID, a3, value);
                };

                auto pViewport = reinterpret_cast<uint8_t*>(UWindowsViewport);
                bool bCheck1 = (pViewport[416] & 1) != 0;
                bool bCheck2 = *reinterpret_cast<int*>(pViewport + 109) != -1;

                if ((bCheck1 || bCheck2))
                {
                    RECT rc{};
                    GetClientRect(hGameWindow, &rc);
                    POINT center{ (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 };
                    POINT screenPt = center;
                    ClientToScreen(hGameWindow, &screenPt);
                    SetCursorPos(screenPt.x, screenPt.y);
                }
            }
        }

        bCalledFromWndProc = true;
        auto ret = shViewportWndProc.unsafe_fastcall<int32_t>(UWindowsViewport, edx, Msg, wParam, lParam);
        bCalledFromWndProc = false;
        return ret;
    }

    void __fastcall CauseInputEvent(void* UWindowsViewport, void* edx, int inputID, int a3, float value)
    {
        if (Screen.bDeferredInput && bCalledFromWndProc && (inputID == 228 || inputID == 229))
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
    void __fastcall UpdateRumble(void* UWindowsViewport, void* edx, float vibrateStrength, float shakeStrength)
    {
        if (!XidiSendVibration)
            return;

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
    UWindowsViewport::shUpdateRumble = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"WinDrv"), "?UpdateRumble@UWindowsViewport@@UAEXMM@Z"), UWindowsViewport::UpdateRumble);
    UWindowsViewport::shCauseInputEvent = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"WinDrv"), "?CauseInputEvent@UWindowsViewport@@QAEHHW4EInputAction@@M@Z"), UWindowsViewport::CauseInputEvent);
    UWindowsViewport::shViewportWndProc = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"WinDrv"), "?ViewportWndProc@UWindowsViewport@@QAEJIIJ@Z"), UWindowsViewport::ViewportWndProc);
}
