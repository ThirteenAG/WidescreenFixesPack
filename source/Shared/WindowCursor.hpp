#pragma once
#include "CursorConfinement.hpp"

namespace WindowCursor
{
    inline std::atomic<HWND> Window{ nullptr };
    inline std::mutex ServiceMutex;
    inline std::atomic<HWND> PendingWindow{ nullptr };
    inline HHOOK PendingHook = nullptr;

    inline UINT ServiceMessage()
    {
        static const UINT message = RegisterWindowMessageW(L"WFP.WindowCursor.Service");
        return message;
    }

    inline UINT_PTR TimerId()
    {
        return reinterpret_cast<UINT_PTR>(&Window);
    }

    inline void Tick(HWND window)
    {
        CursorConfinement::Update(window);
        CursorConfinement::Bind();
    }

    inline LRESULT CALLBACK Procedure(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);

    inline void Retire(HWND window)
    {
        KillTimer(window, TimerId());
        RemoveWindowSubclass(window, Procedure, 0);
    }

    inline void RetireOnOwner(HWND window)
    {
        if (GetWindowThreadProcessId(window, nullptr) == GetCurrentThreadId())
            Retire(window);
        else if (const auto message = ServiceMessage(); message && window)
            PostMessageW(window, message, 0, 0);
    }

    inline LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        const auto service = ServiceMessage();
        if (message == WM_TIMER || message == WM_NCDESTROY || (service && message == service) ||
            message == WM_ENTERSIZEMOVE || message == WM_EXITSIZEMOVE || message == WM_ENTERMENULOOP ||
            message == WM_EXITMENULOOP || message == WM_KILLFOCUS || message == WM_ACTIVATEAPP)
        {
            const std::lock_guard lock(ServiceMutex);
            const bool current = window == Window.load();
            if (message == WM_TIMER && wParam == TimerId())
            {
                if (current)
                    Tick(window);
                return 0;
            }
            if (message == WM_NCDESTROY)
            {
                Retire(window);
                if (current)
                {
                    Window.store(nullptr);
                    CursorConfinement::Shutdown();
                }
            }
            else if (message == service && wParam == 0)
            {
                if (!current)
                    Retire(window);
                return 0;
            }
            else if (current)
                CursorConfinement::Message(message, wParam);
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }

    inline void ClearPending()
    {
        if (PendingHook)
            UnhookWindowsHookEx(PendingHook);
        PendingHook = nullptr;
        PendingWindow.store(nullptr);
    }

    inline void AttachOnOwner(HWND window)
    {
        if (!window || window == Window.load() || GetWindowThreadProcessId(window, nullptr) != GetCurrentThreadId())
            return;
        if (!SetWindowSubclass(window, Procedure, 0, 0))
        {
            OutputDebugStringA("WFP: cursor window service unavailable.\n");
            return;
        }
        if (!SetTimer(window, TimerId(), 16, nullptr))
        {
            RemoveWindowSubclass(window, Procedure, 0);
            OutputDebugStringA("WFP: cursor window timer unavailable.\n");
            return;
        }
        const auto previous = Window.exchange(window);
        RetireOnOwner(previous);
        CursorConfinement::Release();
        Tick(window);
    }

    inline LRESULT CALLBACK InstallFromQueue(int code, WPARAM remove, LPARAM data)
    {
        const auto message = reinterpret_cast<const MSG*>(data);
        if (code == HC_ACTION && remove == PM_REMOVE && message->message == ServiceMessage() && message->wParam == 1)
        {
            const std::lock_guard lock(ServiceMutex);
            if (message->hwnd == PendingWindow.load())
            {
                ClearPending();
                AttachOnOwner(message->hwnd);
            }
        }
        return CallNextHookEx(nullptr, code, remove, data);
    }

    inline void Follow(HWND window)
    {
        if (!window || (window == Window.load() && !PendingWindow.load()))
            return;
        const std::lock_guard lock(ServiceMutex);
        if (window == PendingWindow.load())
            return;
        if (window == Window.load())
        {
            ClearPending();
            return;
        }
        DWORD process = 0;
        const auto owner = GetWindowThreadProcessId(window, &process);
        if (!owner || process != GetCurrentProcessId())
            return;
        ClearPending();
        if (owner == GetCurrentThreadId())
        {
            AttachOnOwner(window);
            return;
        }
        if (const auto message = ServiceMessage())
        {
            PendingHook = SetWindowsHookExW(WH_GETMESSAGE, InstallFromQueue, nullptr, owner);
            if (PendingHook && PostMessageW(window, message, 1, 0))
                PendingWindow.store(window);
            else
                ClearPending();
        }
    }

    inline void Shutdown()
    {
        const std::lock_guard lock(ServiceMutex);
        ClearPending();
        RetireOnOwner(Window.exchange(nullptr));
        CursorConfinement::Shutdown();
    }

    inline void Attach(HWND window)
    {
        const std::lock_guard lock(ServiceMutex);
        if (!Window.load() && !PendingWindow.load())
            AttachOnOwner(window);
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
