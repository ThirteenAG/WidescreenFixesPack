module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Focus;

import Window;

import ComVars;

export namespace GodfatherFocus
{
    int(__cdecl* pause)(int) = nullptr;
    int(__cdecl* resume)(int) = nullptr;
    bool owned = false;
    constexpr uint32_t reason = 8;

    void Service(bool focused, bool enabled)
    {
        if (!pause || !resume || !GodfatherWantedMode.is_initialized() || !GodfatherPauseReasons.is_initialized())
            return;
        const auto mode = GodfatherWantedMode.get();
        const auto flags = GodfatherPauseReasons.get();
        if (enabled && !focused && (mode == 1 || mode == 2))
        {
            if (!(flags & reason))
            {
                pause(reason);
                owned = true;
            }
        }
        else if (owned)
        {
            if (flags & reason)
            {
                if (mode == 3)
                    GodfatherPauseReasons = flags & ~reason;
                else
                    resume(reason);
            }
            owned = false;
        }
    }

    bool Message(HWND target, UINT id, WPARAM wParam, LPARAM, LRESULT& result)
    {
        if (target != GodfatherWindow::window || id != WM_ACTIVATEAPP)
            return false;
        // Keep pumping the main loop; the engine's pause reasons own simulation pause.
        GodfatherIdle = 0;
        result = 0;
        return true;
    }

    void Init()
    {
        auto add = hook::pattern("8B 0D ? ? ? ? 8B 44 24 04 0B C8 C7 05 ? ? ? ? 01 00 00 00 89 0D ? ? ? ? C3");
        auto remove = hook::pattern("8B 44 24 04 8B 0D ? ? ? ? F7 D0 23 C8 89 0D ? ? ? ? 75 0A C7 05 ? ? ? ? 02 00 00 00 C3");
        auto inactive = hook::pattern("84 C0 A0 ? ? ? ? 74 29 84 C0 75 47 C6 05 ? ? ? ? 01 E8 ? ? ? ? C6 05");
        const bool available = GodfatherWindow::createWindowHook && GodfatherWindow::createD3DHook;
        FixSettings().Availability("MAIN.PauseOnFocusLoss", [available](std::string_view value)
        {
            return value == "0" || available;
        });
        if (!available)
        {
            OutputDebugStringA("The Godfather focus: native pause contracts unavailable; feature disabled.\n");
            return;
        }
        GodfatherPauseReasons.SetAddress(*add.get_first<uint32_t*>(2));
        GodfatherWantedMode.SetAddress(*add.get_first<int*>(14));
        GodfatherIdle.SetAddress(*inactive.get_first<uint8_t*>(3));
        pause = reinterpret_cast<decltype(pause)>(add.get_first());
        resume = reinterpret_cast<decltype(resume)>(remove.get_first());
        const auto message = GodfatherWindow::message;
        GodfatherWindow::message = [message](HWND target, UINT id, WPARAM wParam, LPARAM lParam, LRESULT& result)
        {
            return Message(target, id, wParam, lParam, result) || (message && message(target, id, wParam, lParam, result));
        };
        const auto frame = GodfatherWindow::frame;
        GodfatherWindow::frame = [frame](IDirect3DDevice9* device)
        {
            Service(GodfatherWindow::window && GetForegroundWindow() == GodfatherWindow::window,
                FixSettings().Bool("MAIN.PauseOnFocusLoss"));
            if (frame)
                frame(device);
        };
    }
}
