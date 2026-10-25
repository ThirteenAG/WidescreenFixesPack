module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module WinDrv;

import ComVars;
import Xidi;
import WindowedMode;

namespace UWindowsViewport
{
    bool bCalledFromWndProc = false;

    SafetyHookInline shCauseInputEvent = {};
    SafetyHookInline shViewportWndProc = {};
    int32_t __fastcall ViewportWndProc(void* UWindowsViewport, void* edx, UINT Msg, WPARAM wParam, LPARAM lParam)
    {
        hGameWindow = *(HWND*)(*(uintptr_t*)((uintptr_t)UWindowsViewport + 0x1F0) + 4);

        if ((Msg == WM_KEYDOWN || Msg == WM_KEYUP) && wParam == VK_F1 && NativeWindowsMenuHelp &&
            NativeWindowsMenuHelp(Msg == WM_KEYDOWN && !(lParam & (1u << 30))))
            return 0;

        if (Screen.bRawInputMouse)
        {
            static bool bOnce = false;
            if (!bOnce)
            {
                RawInputHandler<>::RegisterRawInput(hGameWindow, FixSettings().Float("MAIN.MouseSensitivity"), Screen.bRawInputMouseRawData);
                bOnce = true;
            }

            auto menuMouse = (Msg == WM_MOUSEMOVE || Msg == WM_RAWINPUTMOUSE) &&
                NativeWindowsMenuMouse && NativeWindowsMenuMouse(lParam, Msg == WM_MOUSEMOVE);
            if (Msg == WM_MOUSEMOVE)
            {
                return 0;
            }
            else if (Msg == WM_RAWINPUTMOUSE)
            {
                if (hGameWindow != GetForegroundWindow())
                    return 0;

                if (menuMouse)
                {
                    // Absolute Windows input now follows the native Magma
                    // hover/picking path. Keep raw deltas out of menu motion
                    // and out of gameplay after the native page is dismissed.
                    RawInputHandler<>::RawMouseDeltaX.exchange(0);
                    RawInputHandler<>::RawMouseDeltaY.exchange(0);
                    deferredCauseInputEventForRawInput = nullptr;
                    return 0;
                }

                deferredCauseInputEventForRawInput = [UWindowsViewport](int inputID, int a3, float value)
                {
                    shCauseInputEvent.unsafe_fastcall(UWindowsViewport, 0, inputID, a3, value);
                };

                auto pViewport = reinterpret_cast<uint8_t*>(UWindowsViewport);
                bool bCheck1 = (pViewport[129] & 1) != 0;
                bool bCheck2 = *reinterpret_cast<int*>(pViewport + 134) != -1;

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

        // Alt+Enter: the game only goes from the window to fullscreen, back to the window here; held keys don't switch again
        if (Msg == WM_SYSCHAR && wParam == VK_RETURN)
            return 0; // DefWindowProc would restore the window to where it was before fullscreen
        if ((Msg == WM_SYSKEYDOWN || Msg == WM_KEYDOWN) && wParam == VK_RETURN && (GetKeyState(VK_MENU) & 0x8000))
        {
            if (lParam & (1 << 30))
                return 0;
            if (*reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(UWindowsViewport) + 0x204) & 1)
            {
                LeaveFullscreen(reinterpret_cast<uintptr_t>(UWindowsViewport));
                return 0;
            }
        }

        auto lastMsg = nViewportWndProcMsg;
        nViewportWndProcMsg = Msg;
        bCalledFromWndProc = true;
        auto ret = shViewportWndProc.unsafe_fastcall<int32_t>(UWindowsViewport, edx, Msg, wParam, lParam);
        bCalledFromWndProc = false;
        nViewportWndProcMsg = lastMsg;

        // A window at the desktop resolution is bigger than the screen with its frame
        if (Msg == WM_GETMINMAXINFO)
            reinterpret_cast<MINMAXINFO*>(lParam)->ptMaxTrackSize = { GetSystemMetrics(SM_CXVIRTUALSCREEN) * 2, GetSystemMetrics(SM_CYVIRTUALSCREEN) * 2 };
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

namespace AEPlayerController
{
    SafetyHookInline shbIsInRumbleMode = {};
    bool __fastcall bIsInRumbleMode(void* AEPlayerController, void* edx)
    {
        return true;
    }
}

export void InitWinDrv()
{
    auto pattern = hook::pattern("C7 06 ? ? ? ? C7 46 ? ? ? ? ? C7 46 ? ? ? ? ? 89 9E F4 01 00 00");
    auto UWindowsViewportVTtable = *pattern.get_first<uint32_t*>(2);
    UWindowsViewport::shUpdateRumble = safetyhook::create_inline(UWindowsViewportVTtable[25], UWindowsViewport::UpdateRumble); //-0x64

    pattern = hook::pattern("C7 06 ? ? ? ? E8 ? ? ? ? 8B 86 1C 08 00 00");
    auto APlayerControllerVTable = *pattern.get_first<uint32_t*>(2);
    AEPlayerController::shbIsInRumbleMode = safetyhook::create_inline(APlayerControllerVTable[178], AEPlayerController::bIsInRumbleMode); //0x2c8

    pattern = hook::pattern("56 8B F1 8B 46 18 8B 48 28 8B 81 A8 00 00 00 85 C0 74");
    UWindowsViewport::shCauseInputEvent = safetyhook::create_inline(pattern.get_first(), UWindowsViewport::CauseInputEvent);

    pattern = hook::pattern("81 EC 8C 03 00 00");
    UWindowsViewport::shViewportWndProc = safetyhook::create_inline(pattern.get_first(), UWindowsViewport::ViewportWndProc);
}
