#pragma once
#include "../../external/FusionFixUtils/include/WindowStyle.hpp"

namespace CoCWindow
{
    inline SafetyHookInline styleHook, selectionHook;
    inline void* (__cdecl* getContext)() = nullptr;
    inline D3DPRESENT_PARAMETERS* presentation = nullptr;
    inline int mode = 0;
    inline bool ready = false;

    inline HWND Window()
    {
        auto context = getContext ? static_cast<uint8_t*>(getContext()) : nullptr;
        return context ? *reinterpret_cast<HWND*>(context + 4) : nullptr;
    }

    inline void Resolution(int& width, int& height)
    {
        if (!ready || mode != 2)
            return;
        const auto monitor = WindowStyle::MonitorRect(Window());
        const int monitorWidth = monitor.right - monitor.left;
        const int monitorHeight = monitor.bottom - monitor.top;
        if (monitorWidth > 0 && monitorHeight > 0 && monitorWidth <= 16384 && monitorHeight <= 16384)
        {
            width = monitorWidth;
            height = monitorHeight;
        }
    }

    inline void Place()
    {
        const auto window = Window();
        if (!window || !presentation)
            return;
        WindowStyle::SquareCorners(window);
        if (mode == 0)
            return;
        const DWORD frameBits = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
        DWORD style = DWORD(GetWindowLongW(window, GWL_STYLE)) & ~(frameBits | WS_POPUP);
        style |= mode == 2 ? WS_POPUP : WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
        SetWindowLongW(window, GWL_STYLE, LONG(style));
        SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        WindowStyle::Place(window, LONG(presentation->BackBufferWidth), LONG(presentation->BackBufferHeight));
        // The native app also caches these rectangles when creating its window.
        auto context = static_cast<uint8_t*>(getContext());
        GetWindowRect(window, reinterpret_cast<RECT*>(context + 20));
        GetClientRect(window, reinterpret_cast<RECT*>(context + 36));
    }

    inline LONG __cdecl Style()
    {
        if (ready && mode != 0 && presentation)
        {
            presentation->Windowed = TRUE;
            presentation->FullScreen_RefreshRateInHz = 0;
        }
        const auto result = styleHook.ccall<LONG>();
        if (ready)
            Place();
        return result;
    }

    inline void* __cdecl Selection(uint8_t windowed, int format, int depth)
    {
        return selectionHook.ccall<void*>(ready && mode != 0 ? uint8_t(1) : windowed, format, depth);
    }

    inline void Init()
    {
        mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
        auto style = hook::pattern("83 EC 2C 56 E8 ? ? ? ? 83 3D ? ? ? ? 00 8B 70 04 74 17 E8 ? ? ? ? 8B 40 10 50 6A F0 56");
        auto selection = hook::pattern("83 EC 34 A1 ? ? ? ? 8B 40 0C 53 56 33 F6 33 DB 3B C6 89 74 24 30 89 5C 24 34 89 74 24 38 89 44 24 28");
        if (style.size() != 1 || selection.size() != 1)
        {
            OutputDebugStringA("Call of Cthulhu: window mode signatures unavailable; native mode retained.\n");
            return;
        }
        getContext = reinterpret_cast<decltype(getContext)>(injector::GetBranchDestination(style.get_first(4)).as_int());
        auto windowed = *style.get_first<uint8_t*>(11);
        presentation = reinterpret_cast<D3DPRESENT_PARAMETERS*>(windowed - offsetof(D3DPRESENT_PARAMETERS, Windowed));
        styleHook = safetyhook::create_inline(style.get_first(), Style);
        selectionHook = safetyhook::create_inline(selection.get_first(), Selection);
        if (!styleHook || !selectionHook)
        {
            styleHook.reset();
            selectionHook.reset();
            presentation = nullptr;
            getContext = nullptr;
            OutputDebugStringA("Call of Cthulhu: window mode hooks failed; native mode retained.\n");
            return;
        }
        ready = true;
    }
}
