#pragma once

namespace PunisherWindow
{
    inline SafetyHookInline CreateHook;
    inline HWND Window{};
    inline WNDPROC OriginalProc{};

    inline bool Framed(HWND window)
    {
        return (GetWindowLongPtrW(window, GWL_STYLE) & WS_CAPTION) == WS_CAPTION
            && !IsIconic(window) && !IsZoomed(window);
    }

    inline void Centre(HWND window, WINDOWPOS& position)
    {
        RECT bounds{};
        MONITORINFO monitor{ sizeof(monitor) };
        if (!Framed(window) || !GetWindowRect(window, &bounds))
            return;
        const POINT centre{ bounds.left + (bounds.right - bounds.left) / 2, bounds.top + (bounds.bottom - bounds.top) / 2 };
        if (!GetMonitorInfoW(MonitorFromPoint(centre, MONITOR_DEFAULTTONEAREST), &monitor))
            return;
        position.x = monitor.rcWork.left + (monitor.rcWork.right - monitor.rcWork.left - position.cx) / 2;
        position.y = monitor.rcWork.top + (monitor.rcWork.bottom - monitor.rcWork.top - position.cy) / 2;
        position.flags &= ~SWP_NOMOVE;
    }

    inline LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_WINDOWPOSCHANGING)
        {
            auto& position = *reinterpret_cast<WINDOWPOS*>(lParam);
            if (!(position.flags & SWP_NOSIZE))
                Centre(window, position);
        }
        const auto result = CallWindowProcW(OriginalProc, window, message, wParam, lParam);
        if (message == WM_NCDESTROY)
        {
            Window = nullptr;
            OriginalProc = nullptr;
        }
        return result;
    }

    inline BOOL CALLBACK Attach(HWND window, LPARAM)
    {
        DWORD process{};
        GetWindowThreadProcessId(window, &process);
        if (Window || process != GetCurrentProcessId() || GetWindow(window, GW_OWNER)
            || (GetWindowLongPtrW(window, GWL_STYLE) & WS_CHILD) || !Framed(window))
            return TRUE;
        const auto previous = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)));
        if (!previous)
            return TRUE;
        Window = window;
        OriginalProc = previous;
        RECT bounds{};
        if (GetWindowRect(window, &bounds))
        {
            WINDOWPOS position{ window, nullptr, bounds.left, bounds.top, bounds.right - bounds.left,
                bounds.bottom - bounds.top, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE };
            Centre(window, position);
            SetWindowPos(window, nullptr, position.x, position.y, 0, 0, position.flags);
        }
        return FALSE;
    }

    inline int __cdecl Create(int width, int height, char fullscreen)
    {
        const auto result = CreateHook.ccall<int>(width, height, fullscreen);
        EnumThreadWindows(GetCurrentThreadId(), Attach, 0);
        return result;
    }

    inline void Init()
    {
        if (!PunMenu::SupportedImage())
            return;
        auto create = hook::pattern("55 8B EC 83 E4 F8 81 EC 34 08 00 00 A1 ? ? ? ? 53 56 57");
        if (create.size() == 1)
            CreateHook = safetyhook::create_inline(create.get_first(), Create);
    }
}
