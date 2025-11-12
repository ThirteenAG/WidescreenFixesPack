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
        hGameWindow = *(HWND*)(*(uintptr_t*)((uintptr_t)UWindowsViewport + 0x180) + 4);

        // Handle Alt+F4 to close the window
        if (Msg == WM_SYSKEYDOWN && wParam == VK_F4)
        {
            PostMessage(hGameWindow, WM_CLOSE, 0, 0);
            return 0;
        }

        if (Screen.fRawInputMouse > 0.0f)
        {
            static bool bOnce = false;
            if (!bOnce)
            {
                RawInputHandler<int32_t>::RegisterRawInput(hGameWindow, Screen.Width, Screen.Height, Screen.fRawInputMouse, Screen.bRawInputMouseRawData);
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
                bool bCheck1 = (pViewport[404] & 1) != 0;
                bool bCheck2 = *reinterpret_cast<int*>(pViewport + 108) != -1;

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
        if (!XidiSendVibration || bIsEnhanced)
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
    auto pattern = hook::module_pattern(GetModuleHandle(L"WinDrv"), "8B 5D 10 8B 46 18 83 FB FF");
    struct OpenWindowHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto [DesktopResW, DesktopResH] = GetDesktopRes();
            tagRECT rect;
            rect.left = (LONG)(((float)DesktopResW / 2.0f) - ((float)Screen.Width / 2.0f));
            rect.top = (LONG)(((float)DesktopResH / 2.0f) - ((float)Screen.Height / 2.0f));
            rect.right = (LONG)Screen.Width;
            rect.bottom = (LONG)Screen.Height;

            *(int32_t*)(regs.esp + 0x474) = rect.right;
            *(int32_t*)(regs.esp + 0x478) = rect.bottom;
            *(int32_t*)(regs.esp + 0x47C) = rect.left;
            *(int32_t*)(regs.esp + 0x480) = rect.top;

            regs.ebx = *(uint32_t*)(regs.ebp + 0x10);
            regs.eax = *(uint32_t*)(regs.esi + 0x18);
        }
    }; injector::MakeInline<OpenWindowHook>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::module_pattern(GetModuleHandle(L"WinDrv"), "8D 4D B8 68 ? ? ? ? C7 45 ? ? ? ? ? 51");
    injector::WriteMemory(pattern.get_first(4), *pattern.get_first<LONG>(4) & ~WS_OVERLAPPEDWINDOW, true);
    injector::WriteMemory(pattern.get_first(11), *pattern.get_first<LONG>(11) & ~WS_OVERLAPPEDWINDOW, true);

    pattern = hook::module_pattern(GetModuleHandle(L"WinDrv"), "6A 00 8D 55 B8 68 ? ? ? ? C7 45");
    injector::WriteMemory(pattern.get_first(6), *pattern.get_first<LONG>(6) & ~WS_OVERLAPPEDWINDOW, true);
    injector::WriteMemory(pattern.get_first(13), *pattern.get_first<LONG>(13) & ~WS_OVERLAPPEDWINDOW, true);

    pattern = hook::module_pattern(GetModuleHandle(L"WinDrv"), "68 ? ? ? ? 6A F0 83 38 00 74 1D 8B 8E");
    injector::WriteMemory(pattern.get_first(1), *pattern.get_first<LONG>(1) & ~WS_OVERLAPPEDWINDOW, true);

    pattern = hook::module_pattern(GetModuleHandle(L"WinDrv"), "8B 8E ? ? ? ? 8B 51 04 52 FF 15 ? ? ? ? 8D A5");
    struct OpenWindowHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            auto [DesktopResW, DesktopResH] = GetDesktopRes();
            tagRECT rect;
            rect.left = (LONG)(((float)DesktopResW / 2.0f) - ((float)Screen.Width / 2.0f));
            rect.top = (LONG)(((float)DesktopResH / 2.0f) - ((float)Screen.Height / 2.0f));
            rect.right = (LONG)Screen.Width;
            rect.bottom = (LONG)Screen.Height;

            regs.ecx = *(uint32_t*)(regs.esi + 0x180);
            regs.edx = *(uint32_t*)(regs.ecx + 0x04);

            SetWindowLong((HWND)regs.edx, GWL_STYLE, GetWindowLong((HWND)regs.edx, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos((HWND)regs.edx, NULL, rect.left, rect.top, rect.right, rect.bottom, SWP_NOACTIVATE | SWP_NOZORDER);
            SetForegroundWindow((HWND)regs.edx);
            SetCursor(NULL);
        }
    }; injector::MakeInline<OpenWindowHook2>(pattern.get_first(0), pattern.get_first(9));

    UWindowsViewport::shUpdateRumble = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"WinDrv"), "?UpdateRumble@UWindowsViewport@@UAEXMM@Z"), UWindowsViewport::UpdateRumble);
    UWindowsViewport::shCauseInputEvent = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"WinDrv"), "?CauseInputEvent@UWindowsViewport@@UAEHHW4EInputAction@@M@Z"), UWindowsViewport::CauseInputEvent);
    UWindowsViewport::shViewportWndProc = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"WinDrv"), "?ViewportWndProc@UWindowsViewport@@QAEJIIJ@Z"), UWindowsViewport::ViewportWndProc);
}
