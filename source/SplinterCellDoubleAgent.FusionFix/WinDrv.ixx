module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/SplinterCell/ViewportMouse.hpp"

export module WinDrv;

import ComVars;
import Xidi;
import DeveloperOverlay;

namespace UWindowsViewport
{
    std::atomic<bool> VibrationEnabled{ true };
    bool bCalledFromWndProc = false;
    HWND feedWindow = nullptr;
    void* feedViewport = nullptr;

    SafetyHookInline shCauseInputEvent = {};
    SafetyHookInline shViewportWndProc = {};
    int32_t __fastcall ViewportWndProc(void* UWindowsViewport, void* edx, UINT Msg, WPARAM wParam, LPARAM lParam)
    {
        const HWND window = *(HWND*)(*(uintptr_t*)((uintptr_t)UWindowsViewport + 0x230) + 4);
        if (!hGameWindow || window == GetForegroundWindow())
            hGameWindow = window;

        LRESULT overlayResult = 0;
        if (DeveloperOverlay::Message(hGameWindow, Msg, wParam, lParam, overlayResult))
            return int32_t(overlayResult);

        if ((Msg == WM_KEYDOWN || Msg == WM_KEYUP) && wParam == VK_F1 && hGameWindow == GetForegroundWindow() &&
            NativeWindowsMenuHelp && NativeWindowsMenuHelp(Msg == WM_KEYDOWN && !(lParam & (1u << 30))))
            return 0;

        // Handle Alt+F4 to close the window
        if (Msg == WM_SYSKEYDOWN && wParam == VK_F4)
        {
            PostMessage(hGameWindow, WM_CLOSE, 0, 0);
            return 0;
        }
        else if (Msg == WM_SETFOCUS)
        {
            PostMessage(hGameWindow, WM_KEYDOWN, VK_LWIN, 0);
            PostMessage(hGameWindow, WM_KEYUP, VK_LWIN, 0);
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
                        if (hGameWindow != GetForegroundWindow() || DeveloperOverlay::BlocksInput())
                            return;
                        if (dx)
                            shCauseInputEvent.unsafe_fastcall(UWindowsViewport, 0, 228, 4, static_cast<float>(dx));
                        if (dy)
                            shCauseInputEvent.unsafe_fastcall(UWindowsViewport, 0, 229, 4, static_cast<float>(dy));

                        auto pViewport = reinterpret_cast<uint8_t*>(UWindowsViewport);
                        bool bCheck1 = (pViewport[580] & 1) != 0;
                        bool bCheck2 = *reinterpret_cast<int*>(pViewport + 0x360) != -1;
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

        bCalledFromWndProc = true;
        auto ret = shViewportWndProc.unsafe_fastcall<int32_t>(UWindowsViewport, edx, Msg, wParam, lParam);
        bCalledFromWndProc = false;
        return ret;
    }

    int __fastcall CauseInputEvent(void* UWindowsViewport, void* edx, int inputID, int a3, float value)
    {
        if (DeveloperOverlay::BlocksInput() && a3 != 3)
            return 1;
        if (Screen.bRawInputMouse && bCalledFromWndProc && (inputID == 228 || inputID == 229))
            ViewportMouse::nativeFrame = true;
        if (Screen.bDeferredInput && bCalledFromWndProc && (inputID == 228 || inputID == 229))
        {
            deferredCauseInputEvent.emplace([UWindowsViewport, edx, inputID, a3, value]()
            {
                shCauseInputEvent.unsafe_fastcall(UWindowsViewport, edx, inputID, a3, value);
            });
            return 1;
        }

        return shCauseInputEvent.unsafe_fastcall<int>(UWindowsViewport, edx, inputID, a3, value);
    }

    SafetyHookInline shUpdateRumble = {};
    void __fastcall UpdateRumble(void* UWindowsViewport, void* edx, float vibrateStrength, float shakeStrength, int a4)
    {
        const auto sendVibration = XidiSendVibration.load(std::memory_order_acquire);
        if (!sendVibration)
            return shUpdateRumble.unsafe_fastcall(UWindowsViewport, edx, vibrateStrength, shakeStrength, a4);

        if (!VibrationEnabled.load())
            vibrateStrength = shakeStrength = 0.0f;

        constexpr float RUMBLE_MIN_STRENGTH = 0.0f;
        constexpr float RUMBLE_MAX_STRENGTH = 1.0f;
        constexpr WORD RUMBLE_MAX_VALUE = 0xFFFF;

        const WORD left = static_cast<WORD>(std::clamp(vibrateStrength, RUMBLE_MIN_STRENGTH, RUMBLE_MAX_STRENGTH) * RUMBLE_MAX_VALUE);
        const WORD right = static_cast<WORD>(std::clamp(shakeStrength, RUMBLE_MIN_STRENGTH, RUMBLE_MAX_STRENGTH) * RUMBLE_MAX_VALUE);

        sendVibration(-1, left, right);
    }
}

export void InitWinDrv()
{
    // The resize handler recreates impact graphics only after SetRes succeeds.
    auto resize = hook::module_pattern(GetModuleHandleW(L"WinDrv.dll"),
        "8B 4B 64 8B 11 6A 00 57 56 53 FF 52 7C 8B 0D ? ? ? ? 8B 43 64 8B 11 8B 4A 5C 50 FF 15 ? ? ? ? EB ?");
    static const auto afterInitGraphic = reinterpret_cast<uintptr_t>(resize.get_first(34));
    static SafetyHookMid resizeResult = safetyhook::create_mid(resize.get_first(13), [](SafetyHookContext& regs)
    {
        if (!regs.eax)
            regs.eip = afterInitGraphic;
    });

    // Override the two native StartupFullscreen reads without requiring an INI write.
    static std::vector<SafetyHookMid> startupModeHooks;
    const char* startupReads[] =
    {
        "8B 88 B0 00 00 00 8B 16 51 53 57 68",
        "8B 88 B0 00 00 00 85 C9 74 ? 8B 06 6A 01 53 57 68",
    };
    for (const auto signature : startupReads)
    {
        auto pattern = hook::module_pattern(GetModuleHandleW(L"WinDrv.dll"), signature);
        startupModeHooks.push_back(safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
        {
            regs.ecx = FixSettings().Int("MAIN.WindowMode") == 0 ? 1 : 0;
        }));
    }
    ObserveFixSetting("MAIN.Vibration", asi_settings::detail::ParseBool, [](int value)
    {
        const auto sendVibration = XidiSendVibration.load(std::memory_order_acquire);
        UWindowsViewport::VibrationEnabled.store(value != 0);
        if (!value && sendVibration)
            sendVibration(-1, 0, 0);
    });
    UWindowsViewport::shUpdateRumble = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"WinDrv"), "?UpdateRumble@UWindowsViewport@@UAEXMMH@Z"), UWindowsViewport::UpdateRumble);
    UWindowsViewport::shCauseInputEvent = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"WinDrv"), "?CauseInputEvent@UWindowsViewport@@UAEHHW4EInputAction@@M@Z"), UWindowsViewport::CauseInputEvent);
    // The main loop empties the message queue each frame; a high polling rate mouse must not keep it going
    MessagePumpGuard::Install(GetModuleHandle(nullptr));

    UWindowsViewport::shViewportWndProc = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"WinDrv"), "?ViewportWndProc@UWindowsViewport@@QAEJIIJ@Z"), UWindowsViewport::ViewportWndProc);
}
