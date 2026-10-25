#pragma once
#include "NativeMenuContext.hpp"
#include "../../external/FusionFixUtils/include/WindowStyle.hpp"

namespace DeerWindow
{
    struct VideoMode
    {
        int width, height, depth, flags;
    };

    inline SafetyHookMid selectionHook, showHook;
    inline SafetyHookInline procedureHook;
    inline uintptr_t* contextSlot = nullptr;
    inline VideoMode** selectedSlot = nullptr;
    inline VideoMode** modesSlot = nullptr;
    inline int* modeCount = nullptr;
    inline int* width = nullptr;
    inline int* height = nullptr;
    inline int mode = 0;
    inline bool ready = false, windowed = false;

    inline HWND Window()
    {
        auto context = contextSlot ? DeerMenu::Read<uintptr_t>(reinterpret_cast<uintptr_t>(contextSlot)) : 0;
        return DeerMenu::Read<HWND>(context);
    }

    inline bool ValidSize(int w, int h)
    {
        return w > 0 && h > 0 && w <= 16384 && h <= 16384;
    }

    inline bool Prepare()
    {
        if (!ready || mode == 0 || !contextSlot || !selectedSlot || !modesSlot || !modeCount || !width || !height)
            return false;
        auto context = DeerMenu::Read<uintptr_t>(reinterpret_cast<uintptr_t>(contextSlot));
        auto selected = DeerMenu::Read<VideoMode*>(reinterpret_cast<uintptr_t>(selectedSlot));
        auto modes = DeerMenu::Read<VideoMode*>(reinterpret_cast<uintptr_t>(modesSlot));
        const auto count = DeerMenu::Read<int>(reinterpret_cast<uintptr_t>(modeCount));
        if (!DeerMenu::Readable(reinterpret_cast<void*>(context), 12) || count <= 0 || count > 65536 ||
            !DeerMenu::Readable(modes, size_t(count) * sizeof(VideoMode)))
            return false;
        const auto first = reinterpret_cast<uintptr_t>(modes);
        const auto chosen = reinterpret_cast<uintptr_t>(selected);
        if (chosen < first || chosen >= first + size_t(count) * sizeof(VideoMode) ||
            (chosen - first) % sizeof(VideoMode) || !ValidSize(selected->width, selected->height))
            return false;
        // RenderWare only enumerates a windowed mode for a compatible adapter.
        bool supported = false;
        for (int i = 0; i < count; ++i)
            if (!(modes[i].flags & 1) && ValidSize(modes[i].width, modes[i].height))
                supported = true;
        if (!supported)
            return false;
        int w = selected->width, h = selected->height;
        if (mode == 2)
        {
            const auto monitor = WindowStyle::MonitorRect(Window());
            w = monitor.right - monitor.left;
            h = monitor.bottom - monitor.top;
        }
        else
        {
            const auto window = Window();
            MONITORINFO monitor{ sizeof(monitor) };
            RECT work{};
            if (GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor))
                work = monitor.rcWork;
            else if (!SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0))
                return false;
            const DWORD decorations = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
            auto style = DWORD(GetWindowLongW(window, GWL_STYLE)) & ~(decorations | WS_POPUP);
            style |= WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
            const auto frame = WindowStyle::FrameOf(style, DWORD(GetWindowLongW(window, GWL_EXSTYLE)), WindowStyle::DpiOf(window));
            const auto availableWidth = work.right - work.left - frame.Width();
            const auto availableHeight = work.bottom - work.top - frame.Height();
            if (availableWidth < 320 || availableHeight < 200)
                return false;
            const auto scale = std::min({ 1.0, double(availableWidth) / w, double(availableHeight) / h });
            w = std::max(2, int(w * scale) & ~1);
            h = std::max(2, int(h * scale) & ~1);
        }
        if (!ValidSize(w, h))
            return false;
        // The device and camera consume the same fitted windowed dimensions.
        ProtectedGameRef<int>{ &selected->width } = w;
        ProtectedGameRef<int>{ &selected->height } = h;
        // The device and camera both consume this native mode flag.
        ProtectedGameRef<int>{ &selected->flags } = selected->flags & ~1;
        ProtectedGameRef<int>{ reinterpret_cast<int*>(context + 8) } = 0;
        ProtectedGameRef<int>{ width } = w;
        ProtectedGameRef<int>{ height } = h;
        windowed = true;
        return true;
    }

    inline void Place()
    {
        const auto window = Window();
        if (!window)
            return;
        WindowStyle::SquareCorners(window);
        if (!ready || !windowed || mode == 0 || !width || !height || !ValidSize(*width, *height))
            return;
        const DWORD frame = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
        DWORD style = DWORD(GetWindowLongW(window, GWL_STYLE)) & ~(frame | WS_POPUP);
        style |= mode == 2 ? WS_POPUP : WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
        SetWindowLongW(window, GWL_STYLE, LONG(style));
        SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        if (mode == 1)
        {
            MONITORINFO monitor{ sizeof(monitor) };
            RECT work{};
            if (GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor))
                work = monitor.rcWork;
            else if (!SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0))
                return;
            const auto frameSize = WindowStyle::FrameOf(style, DWORD(GetWindowLongW(window, GWL_EXSTYLE)), WindowStyle::DpiOf(window));
            const auto outerWidth = *width + frameSize.Width();
            const auto outerHeight = *height + frameSize.Height();
            const auto left = work.left + (work.right - work.left - outerWidth) / 2;
            const auto top = work.top + (work.bottom - work.top - outerHeight) / 2;
            SetWindowPos(window, nullptr, left, top, outerWidth, outerHeight, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        }
        else
            WindowStyle::Place(window, *width, *height);
    }

    inline LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (ready && windowed && mode != 0 && window == Window() && message == WM_GETMINMAXINFO && lParam)
        {
            const auto style = DWORD(GetWindowLongW(window, GWL_STYLE));
            const auto exStyle = DWORD(GetWindowLongW(window, GWL_EXSTYLE));
            const auto frame = WindowStyle::FrameOf(style, exStyle, WindowStyle::DpiOf(window));
            auto info = reinterpret_cast<MINMAXINFO*>(lParam);
            info->ptMinTrackSize = info->ptMaxTrackSize = { *width + frame.Width(), *height + frame.Height() };
            info->ptMaxSize = info->ptMaxTrackSize;
            return 0;
        }
        return procedureHook.stdcall<LRESULT>(window, message, wParam, lParam);
    }

    inline bool Init(int requested, HMODULE game = GetModuleHandleW(nullptr))
    {
        mode = std::clamp(requested, 0, 2);
        auto selection = hook::pattern(game, "8A 45 E8 BB 01 00 00 00 83 C4 08 84 C3 74 1A 8B 45 DC 8B 4D E0 8B 15 ? ? ? ? A3 ? ? ? ? 89 0D ? ? ? ? 89 5A 08 8D 45 EC 8D 4D F8");
        auto selected = hook::pattern(game, "A1 ? ? ? ? 85 C0 74 03 33 C0 C3 85 C9 7C F9 3B 0D ? ? ? ? 7D F1 A1 ? ? ? ? 89 0D ? ? ? ? C1 E1 04 03 C8 B8 01 00 00 00 89 0D ? ? ? ? C3");
        auto show = hook::pattern(game, "A1 ? ? ? ? 8B 94 24 98 03 00 00 83 C4 04 8B 08 52 51 FF 15 ? ? ? ? 8B 15 ? ? ? ? 8B 02 50 FF 15");
        auto procedure = hook::pattern(game, "8B 44 24 08 83 EC 18 3D 04 01 00 00 56 57 0F 87 C4 02 00 00 0F 84 76 02 00 00 83 F8 24");
        if (selection.size() != 1 || selected.size() != 1 || show.size() != 1 || procedure.size() != 1)
            return false;
        contextSlot = *selection.get_first<uintptr_t*>(23);
        width = *selection.get_first<int*>(28);
        height = *selection.get_first<int*>(34);
        modeCount = *selected.get_first<int*>(18);
        modesSlot = *selected.get_first<VideoMode**>(25);
        selectedSlot = *selected.get_first<VideoMode**>(47);
        selectionHook = safetyhook::create_mid(selection.get_first(41), [](SafetyHookContext&)
        {
            if (Prepare()) Place();
        });
        showHook = safetyhook::create_mid(show.get_first(), [](SafetyHookContext&) { Place(); });
        procedureHook = safetyhook::create_inline(procedure.get_first(), Procedure);
        if (!selectionHook || !showHook || !procedureHook)
        {
            selectionHook.reset(); showHook.reset(); procedureHook.reset();
            return false;
        }
        ready = true;
        return true;
    }
}
