#pragma once

namespace CondemnedWindow
{
    inline int Mode = 0;
    inline decltype(&SetWindowPos) OriginalPosition = SetWindowPos;
    inline SafetyHookMid SelectionHook;

    inline BOOL WINAPI Position(HWND window, HWND after, int x, int y, int width, int height, UINT flags)
    {
        if (Mode && window)
        {
            MONITORINFO monitor{ sizeof(monitor) };
            if (GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor))
            {
                const auto& bounds = monitor.rcMonitor;
                if (Mode == 2)
                {
                    x = bounds.left;
                    y = bounds.top;
                    width = bounds.right - bounds.left;
                    height = bounds.bottom - bounds.top;
                    flags &= ~(SWP_NOMOVE | SWP_NOSIZE);
                }
                else
                {
                    RECT frame{};
                    AdjustWindowRectEx(&frame, GetWindowLongW(window, GWL_STYLE), FALSE, GetWindowLongW(window, GWL_EXSTYLE));
                    const int clientWidth = width - (frame.right - frame.left);
                    const int clientHeight = height - (frame.bottom - frame.top);
                    if (clientWidth >= bounds.right - bounds.left && clientHeight >= bounds.bottom - bounds.top)
                    {
                        x = bounds.left + frame.left;
                        y = bounds.top + frame.top;
                        flags &= ~SWP_NOMOVE;
                    }
                }
            }
            using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
            static const auto setAttribute = []()
            {
                auto dwm = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
                return dwm ? reinterpret_cast<SetAttribute>(GetProcAddress(dwm, "DwmSetWindowAttribute")) : nullptr;
            }();
            if (setAttribute)
            {
                const DWORD square = 1;
                setAttribute(window, 33, &square, sizeof(square));
            }
        }
        return OriginalPosition(window, after, x, y, width, height, flags);
    }

    inline void Install(int mode)
    {
        auto selection = hook::pattern("8B 0D ? ? ? ? 85 C9 A3 ? ? ? ? A1 ? ? ? ? 0F 95 C3 85 C0 88 5C 24 10");
        auto sizing = hook::pattern("68 00 02 00 00 2B CA 51 2B F0 56 52 8B 15 ? ? ? ? 50 6A 00 52 FF 15");
        auto rectangleStyle = hook::pattern("68 ? ? ? ? 8D 4C 24 0C 03 C7 51");
        auto windowStyle = hook::pattern("68 ? ? ? ? 52 68 ? ? ? ? 53");
        if (selection.size() != 1 || sizing.size() != 1 || rectangleStyle.size() != 1 || windowStyle.size() != 1)
        {
            OutputDebugStringA("Condemned: native window mode unavailable.\n");
            return;
        }
        auto positionSlot = *sizing.get_first<decltype(&SetWindowPos)*>(24);
        if (!positionSlot || !*positionSlot)
        {
            OutputDebugStringA("Condemned: native window sizing unavailable.\n");
            return;
        }
        Mode = std::clamp(mode, 0, 2);
        // Override the renderer's native Windowed decision after configuration is read.
        SelectionHook = safetyhook::create_mid(selection.get_first(6), [](SafetyHookContext& context)
        {
            context.ecx = Mode != 0;
        });
        if (!SelectionHook)
        {
            OutputDebugStringA("Condemned: native window mode hook unavailable.\n");
            return;
        }
        OriginalPosition = *positionSlot;
        ProtectedGameRef<decltype(&SetWindowPos)>{ positionSlot } = Position;
        if (Mode == 2)
        {
            ProtectedGameRef<uint32_t>{ rectangleStyle.get_first<uint32_t>(1) } = WS_POPUP;
            ProtectedGameRef<uint32_t>{ windowStyle.get_first<uint32_t>(1) } = WS_POPUP;
        }
    }
}
