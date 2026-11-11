module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"

export module Focus;

import ComVars;
import DeveloperOverlay;

namespace
{
    SafetyHookInline PauseHook;
    void* PausedEngine = nullptr;

    int __fastcall Pause(void* engine, void* edx, int paused)
    {
        if (paused && nViewportWndProcMsg == WM_ACTIVATEAPP && !FixSettings().Bool("MAIN.PauseOnFocusLoss") &&
            hGameWindow && !IsIconic(hGameWindow))
            return 0;
        return PauseHook.unsafe_fastcall<int>(engine, edx, paused);
    }

    SafetyHookInline MovieSkipHook;

    // Movies play in a loop of their own that never reads the window's messages, so Windows reports the window as
    // not responding, and the loop takes its skip keys from the keyboard whichever program they are pressed in.
    // This runs once per movie frame, after the frame is shown.
    int __fastcall MovieSkip(void* engine, void* edx)
    {
        const bool unicode = hGameWindow && IsWindowUnicode(hGameWindow);
        MSG msg;
        while (unicode ? PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE) : PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                // Left for the main loop, which ends the game
                PostQuitMessage(int(msg.wParam));
                return 1;
            }
            TranslateMessage(&msg);
            unicode ? DispatchMessageW(&msg) : DispatchMessageA(&msg);
        }

        // Mouse movement during a movie doesn't turn the camera afterwards
        UWindowsViewport::deferredCauseInputEvent = {};
        UWindowsViewport::deferredCauseInputEventForRawInput = nullptr;
        RawInputHandler<>::RawMouseDeltaX.exchange(0);
        RawInputHandler<>::RawMouseDeltaY.exchange(0);

        if (hGameWindow && GetForegroundWindow() != hGameWindow)
            return 0;
        return MovieSkipHook.unsafe_fastcall<int>(engine, edx);
    }
}

export namespace Focus
{
    void Update(void* engine)
    {
        CursorConfinement::Suspend(DeveloperOverlay::BlocksInput());
        CursorConfinement::Frame(hGameWindow);
        if (!PauseHook || !hGameWindow)
            return;
        const bool inactive = GetForegroundWindow() != hGameWindow;
        const bool wanted = IsIconic(hGameWindow) || (inactive && FixSettings().Bool("MAIN.PauseOnFocusLoss"));
        if (wanted)
        {
            // The native focus pause remembers an existing player pause and pauses sound.
            // Retry while loading, before the player controller exists.
            if (!(*reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(engine) + 0x1CC) & 0x40))
                PauseHook.unsafe_fastcall<int>(engine, nullptr, 1);
            PausedEngine = engine;
        }
        else if (PausedEngine == engine)
        {
            PauseHook.unsafe_fastcall<int>(engine, nullptr, 0);
            PausedEngine = nullptr;
        }
    }

    void Init()
    {
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        FixSettings().OnChange("MAIN.ConfineCursor", []()
        {
            CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        });
        auto pattern = hook::pattern("81 EC B4 00 00 00 A1 ? ? ? ? 56 8B F1 89 84 24 B4 00 00 00 8B 86 A8 00 00 00 85 C0 74 30");
        PauseHook = safetyhook::create_inline(pattern.get_first(), Pause);

        pattern = hook::pattern("56 57 8B F9 8B 07 33 F6 FF 50 48 85 C0 0F 84 E1 00 00 00");
        MovieSkipHook = safetyhook::create_inline(pattern.get_first(), MovieSkip);
    }
}
