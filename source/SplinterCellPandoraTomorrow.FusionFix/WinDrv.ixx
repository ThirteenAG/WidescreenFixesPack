module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/SplinterCell/ViewportMouse.hpp"
#include <functional>
#include <unordered_set>

export module WinDrv;

import ComVars;
import Xidi;

namespace UWindowsViewport
{
    bool bCalledFromWndProc = false;
    HWND feedWindow = nullptr;
    void* feedViewport = nullptr;

    constexpr UINT IST_Press = 1;
    constexpr UINT IST_Release = 3;
    constexpr UINT IK_MOUSE4 = 193; // IK_UnknownC0
    constexpr UINT IK_MOUSE5 = 194; // IK_UnknownC1

    SafetyHookInline shCauseInputEvent = {};
    SafetyHookInline shViewportWndProc = {};
    int32_t __fastcall ViewportWndProc(void* UWindowsViewport, void* edx, UINT Msg, WPARAM wParam, LPARAM lParam)
    {
        const HWND window = *(HWND*)(*(uintptr_t*)((uintptr_t)UWindowsViewport + 0x18C) + 4);
        if (!hGameWindow || window == GetForegroundWindow())
            hGameWindow = window;

        // The game turns the camera by the cursor's distance from the window centre. Minimizing and restoring send mouse
        // moves that don't match the cursor (the minimized window's far-off position, or the client corner); skip them.
        // While minimized the cursor is recentred on the empty client area, so after restoring it sits at the client
        // corner: it's put back at the centre instead of turning the camera.
        static bool wasMinimized = false;
        static ULONGLONG restoredAt = 0;
        if (Msg == WM_SIZE)
        {
            if (wParam == SIZE_MINIMIZED)
                wasMinimized = true;
            else if (wasMinimized)
            {
                wasMinimized = false;
                restoredAt = GetTickCount64();
            }
        }
        if (Msg == WM_MOUSEMOVE)
        {
            const POINT reported{ static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam)) };
            RECT client{};
            POINT cursor{};
            if (IsIconic(window) || !GetClientRect(window, &client) || !PtInRect(&client, reported))
                return 0;
            if (reported.x == 0 && reported.y == 0 && GetCursorPos(&cursor) && ScreenToClient(window, &cursor) &&
                (std::abs(cursor.x) > 2 || std::abs(cursor.y) > 2))
                return 0;
            if (reported.x == 0 && reported.y == 0 && restoredAt && GetTickCount64() - restoredAt < 2000)
            {
                restoredAt = 0;
                POINT centre{ (client.left + client.right) / 2, (client.top + client.bottom) / 2 };
                if (window == GetForegroundWindow() && ClientToScreen(window, &centre))
                    SetCursorPos(centre.x, centre.y);
                return 0;
            }
        }

        // Handle Mouse 4 / Mouse 5 (XButtons)
        if (Msg == WM_XBUTTONDOWN || Msg == WM_XBUTTONUP)
        {
            const UINT button = GET_XBUTTON_WPARAM(wParam);
            UINT iKey = 0;
            if (button == XBUTTON1)
                iKey = IK_MOUSE4;
            else if (button == XBUTTON2)
                iKey = IK_MOUSE5;

            if (iKey != 0)
            {
                const UINT inputState = (Msg == WM_XBUTTONDOWN) ? IST_Press : IST_Release;
                shCauseInputEvent.unsafe_fastcall(UWindowsViewport, 0, (int)iKey, (int)inputState, 1.0f);
                return 0;
            }
        }

        if (Screen.bRawInputMouse)
        {
            const bool inactive = Msg == WM_KILLFOCUS || (Msg == WM_ACTIVATEAPP && !wParam) ||
                Msg == WM_DESTROY || Msg == WM_NCDESTROY;
            if (inactive && window == feedWindow)
            {
                RawMouseFeed = nullptr;
                feedViewport = nullptr;
                feedWindow = nullptr;
                deferredCauseInputEvent = {};
                ViewportMouse::Bind(nullptr);
            }
            else if (!inactive && window && window == GetForegroundWindow())
            {
                if (feedWindow != window || feedViewport != UWindowsViewport)
                {
                    deferredCauseInputEvent = {};
                    ViewportMouse::Bind(nullptr);
                    feedWindow = window;
                    feedViewport = UWindowsViewport;
                    RawMouseFeed = [UWindowsViewport](int dx, int dy)
                    {
                        if (hGameWindow != GetForegroundWindow() || IsIconic(hGameWindow))
                            return;
                        if (dx)
                            shCauseInputEvent.unsafe_fastcall(UWindowsViewport, 0, 228, 4, static_cast<float>(dx));
                        if (dy)
                            shCauseInputEvent.unsafe_fastcall(UWindowsViewport, 0, 229, 4, static_cast<float>(dy));

                        auto pViewport = reinterpret_cast<uint8_t*>(UWindowsViewport);
                        bool bCheck1 = (pViewport[416] & 1) != 0;
                        bool bCheck2 = *reinterpret_cast<int*>(pViewport + 109) != -1;
                        if (bCheck1 || bCheck2)
                        {
                            RECT rc{};
                            GetClientRect(hGameWindow, &rc);
                            POINT screenPt{ (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 };
                            ClientToScreen(hGameWindow, &screenPt);
                            SetCursorPos(screenPt.x, screenPt.y);
                        }
                    };
                }
                ViewportMouse::Bind(window);
                if (Msg == WM_MOUSEMOVE)
                {
                    if (ViewportMouse::target.load() == window && ViewportMouse::movement.HasRelative() &&
                        !ViewportMouse::nativeFrame)
                        return 0;
                }
            }
        }

        // Messages can nest: each call restores the outer message's state
        const bool outerCalled = std::exchange(bCalledFromWndProc, true);
        auto ret = shViewportWndProc.unsafe_fastcall<int32_t>(UWindowsViewport, edx, Msg, wParam, lParam);
        bCalledFromWndProc = outerCalled;
        return ret;
    }

    int __fastcall CauseInputEvent(void* UWindowsViewport, void* edx, int inputID, int a3, float value)
    {
        // Ignore release events the game triggers automatically, GetKeyState doesn't work for XButtons
        if (inputID == IK_MOUSE4 || inputID == IK_MOUSE5)
            return 1;

        if (bCalledFromWndProc && (inputID == 228 || inputID == 229))
        {
            if (Screen.bRawInputMouse)
                ViewportMouse::nativeFrame = true;
            deferredCauseInputEvent.emplace([UWindowsViewport, edx, inputID, a3, value]()
            {
                shCauseInputEvent.unsafe_fastcall(UWindowsViewport, edx, inputID, a3, value);
            });
            return 1;
        }

        return shCauseInputEvent.unsafe_fastcall<int>(UWindowsViewport, edx, inputID, a3, value);
    }

    std::mutex rumbleMutex;
    bool rumbleActive = false;
    ULONGLONG nextRumbleStop = 0;

    void StopRumbleLocked()
    {
        if (!rumbleActive || GetTickCount64() < nextRumbleStop)
            return;

        const auto sendVibration = XidiSendVibration.load(std::memory_order_acquire);
        if (sendVibration && sendVibration(-1, 0, 0))
        {
            rumbleActive = false;
            nextRumbleStop = 0;
        }
        else
            nextRumbleStop = GetTickCount64() + 100;
    }

    void UpdateRumbleStop()
    {
        std::lock_guard lock(rumbleMutex);
        if (rumbleActive && (nextRumbleStop || !FixSettings().Bool("MAIN.Vibration")))
            StopRumbleLocked();
    }

    SafetyHookInline shUpdateRumble = {};
    void __fastcall UpdateRumble(void* UWindowsViewport, void* edx, float vibrateStrength, float shakeStrength)
    {
        constexpr float RUMBLE_MIN_STRENGTH = 0.0f;
        constexpr float RUMBLE_MAX_STRENGTH = 1.0f;
        constexpr WORD RUMBLE_MAX_VALUE = 0xFFFF;

        const WORD left = static_cast<WORD>(std::clamp(vibrateStrength, RUMBLE_MIN_STRENGTH, RUMBLE_MAX_STRENGTH) * RUMBLE_MAX_VALUE);
        const WORD right = static_cast<WORD>(std::clamp(shakeStrength, RUMBLE_MIN_STRENGTH, RUMBLE_MAX_STRENGTH) * RUMBLE_MAX_VALUE);

        std::lock_guard lock(rumbleMutex);
        if ((!left && !right) || !FixSettings().Bool("MAIN.Vibration"))
        {
            StopRumbleLocked();
            return;
        }

        if (const auto sendVibration = XidiSendVibration.load(std::memory_order_acquire))
        {
            rumbleActive = true;
            if (sendVibration(-1, left, right))
                nextRumbleStop = 0;
        }
    }
}

export void InitWinDrv()
{
    FixSettings().OnChange("MAIN.Vibration", UWindowsViewport::UpdateRumbleStop);
    WFP::onGameProcessEvent() += UWindowsViewport::UpdateRumbleStop;
    UWindowsViewport::shUpdateRumble = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"WinDrv"), "?UpdateRumble@UWindowsViewport@@UAEXMM@Z"), UWindowsViewport::UpdateRumble);
    UWindowsViewport::shCauseInputEvent = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"WinDrv"), "?CauseInputEvent@UWindowsViewport@@QAEHHW4EInputAction@@M@Z"), UWindowsViewport::CauseInputEvent);
    // The main loop empties the message queue each frame; a high polling rate mouse must not keep it going
    MessagePumpGuard::Install(GetModuleHandle(nullptr));

    UWindowsViewport::shViewportWndProc = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"WinDrv"), "?ViewportWndProc@UWindowsViewport@@QAEJIIJ@Z"), UWindowsViewport::ViewportWndProc);

    // UseDIMouse=True: a DirectInput mouse takes the mouse input away from the raw input reader,
    // so with raw input on the viewport keeps the window messages route
    static auto UseDIMouseOff = [](SafetyHookContext& regs)
    {
        if (FixSettings().Int("MAIN.RawInputMouse") != 0)
            *reinterpret_cast<int32_t*>(regs.edi) = 0;
    };
    auto pattern = find_module_pattern(GetModuleHandle(L"WinDrv"), "8D BE B0 01 00 00 C7 07 00 00 00 00 A1 ? ? ? ? 6A 00 57 68 ? ? ? ? 8B 08 68 ? ? ? ? 8B 01 FF 10");
    if (!pattern.empty()) // digital
    {
        static auto UseDIMouseHook = safetyhook::create_mid(pattern.get_first(36), UseDIMouseOff);
    }
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"WinDrv"), "8D BE B0 01 00 00 89 1F 8B 0D ? ? ? ? 8B 09 8B 11 57 68 ? ? ? ? 68 ? ? ? ? FF 12");
        static auto UseDIMouseHook = safetyhook::create_mid(pattern.get_first(31), UseDIMouseOff);
    }
}
