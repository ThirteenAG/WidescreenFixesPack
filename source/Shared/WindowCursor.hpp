#pragma once
#include "CursorConfinement.hpp"

namespace WindowCursor
{
    inline HWND Window = nullptr;
    inline UINT_PTR Timer = 0;

    inline LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        if (window == Window && message == WM_TIMER && Timer && wParam == Timer)
        {
            CursorConfinement::Frame(window);
            return 0;
        }
        if (message == WM_NCDESTROY)
        {
            if (window == Window)
            {
                if (Timer)
                    KillTimer(window, Timer);
                Timer = 0;
                Window = nullptr;
                CursorConfinement::Shutdown();
            }
            RemoveWindowSubclass(window, Procedure, 0);
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }

    inline void Shutdown()
    {
        if (Window && IsWindow(Window))
        {
            if (Timer)
                KillTimer(Window, Timer);
            RemoveWindowSubclass(Window, Procedure, 0);
        }
        Window = nullptr;
        Timer = 0;
        CursorConfinement::Shutdown();
    }

    inline void Attach(HWND window)
    {
        if (!window || (Window && IsWindow(Window)))
            return;
        if (!CursorConfinement::Attach(window))
            return;
        if (!SetWindowSubclass(window, Procedure, 0, 0))
        {
            CursorConfinement::Shutdown();
            OutputDebugStringA("WFP: cursor window service unavailable.\n");
            return;
        }
        Timer = SetTimer(window, reinterpret_cast<UINT_PTR>(&Timer), 16, nullptr);
        if (!Timer)
        {
            RemoveWindowSubclass(window, Procedure, 0);
            CursorConfinement::Shutdown();
            OutputDebugStringA("WFP: cursor window timer unavailable.\n");
            return;
        }
        Window = window;
        CursorConfinement::Frame(window);
    }

    template <class Settings>
    void Initialize(Settings& settings)
    {
        CursorConfinement::SetEnabled(settings.Bool("MAIN.ConfineCursor"));
        settings.OnChange("MAIN.ConfineCursor", [&settings]()
        {
            CursorConfinement::SetEnabled(settings.Bool("MAIN.ConfineCursor"));
        });
    }
}
