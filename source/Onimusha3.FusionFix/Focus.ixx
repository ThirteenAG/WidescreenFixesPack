module;
#include <stdafx.h>
#include "Shared/CursorConfinement.hpp"
#include "FixSettings.hpp"

export module Focus;
import Window;


export namespace OnimushaFocus
{
    inline std::atomic<bool> Pause{ true }, Confine{ true };
    inline std::atomic<HWND> Bound{ nullptr };
    inline bool Paused = false;
    inline void(__fastcall* NativePause)(bool, bool) = nullptr;
    inline std::function<bool()> BlocksInput;
    constexpr UINT Refresh = WM_APP + 0x3D7;

    inline void Update()
    {
        const auto window = OnimushaWindow::Window;
        Bound.store(window, std::memory_order_relaxed);
        const bool pause = window && OnimushaWindow::Windowed && Pause.load(std::memory_order_relaxed) &&
            (GetForegroundWindow() != window || IsIconic(window));
        if (pause != Paused)
        {
            NativePause(pause, pause);
            Paused = pause;
        }
        CursorConfinement::SetEnabled(Confine.load(std::memory_order_relaxed) && !(BlocksInput && BlocksInput()));
        CursorConfinement::Frame(window);
    }

    inline void Message(HWND window, UINT message, WPARAM wParam)
    {
        if (message == WM_NCDESTROY)
        {
            if (Paused)
                NativePause(false, false);
            Paused = false;
            Bound.store(nullptr, std::memory_order_relaxed);
            CursorConfinement::Shutdown();
        }
        else if (message == Refresh || message == WM_ACTIVATEAPP || message == WM_SETFOCUS ||
            message == WM_KILLFOCUS || message == WM_SIZE)
        {
            Update();
        }
    }

    inline void Init()
    {
        NativePause = reinterpret_cast<decltype(NativePause)>(hook::pattern("51 53 55 56 88 54 24 0C 8A D9 E8 ? ? ? ? 80 3D").get_first());
        const auto refresh = []()
        {
            Pause = FixSettings().Bool("MAIN.PauseOnFocusLoss");
            Confine = FixSettings().Bool("MAIN.ConfineCursor");
            CursorConfinement::SetEnabled(Confine.load());
            if (const auto window = Bound.load(std::memory_order_relaxed))
                PostMessageW(window, Refresh, 0, 0);
        };
        refresh();
        FixSettings().OnChange("MAIN.PauseOnFocusLoss", refresh);
        FixSettings().OnChange("MAIN.ConfineCursor", refresh);
        OnimushaWindow::Tick = Update;
        OnimushaWindow::Message = Message;
    }
}
