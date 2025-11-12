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
        hGameWindow = *(HWND*)(*(uintptr_t*)((uintptr_t)UWindowsViewport + 0x1F0) + 4);

        if (Screen.fRawInputMouse > 0.0f)
        {
            static bool bOnce = false;
            if (!bOnce)
            {
                RawInputHandler<>::RegisterRawInput(hGameWindow, Screen.Width, Screen.Height, Screen.fRawInputMouse);
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
