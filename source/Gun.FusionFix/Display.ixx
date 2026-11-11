module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include "Shared/WindowCursor.hpp"
#include <commctrl.h>
#include <atomic>
#include <functional>

export module Display;

import Imports;
import ComVars;

#pragma comment(lib, "comctl32.lib")

export namespace GunDisplay
{
    inline SafetyHookInline createHook, parametersHook, resetHook;
    inline HWND window = nullptr;
    inline int mode = 0, lastWindowMode = 1;
    inline int requestedWidth = 0, requestedHeight = 0;
    inline std::atomic<int> pending{ -1 };
    inline std::function<void(int, int)> resolution;
    inline std::atomic_bool changing{ false };
    inline bool styled = false;
    inline constexpr UINT ChangeMessage = WM_APP + 0x475;
    inline decltype(&ShowWindow) nativeShow = nullptr;
    inline decltype(&SetWindowPos) nativePosition = nullptr;
    inline void(__cdecl* pauseGame)() = nullptr;
    inline void(__cdecl* resumeGame)() = nullptr;
    inline bool (*messageHook)(HWND, UINT, WPARAM, LPARAM, LRESULT&) = nullptr;

    inline void Request(int wanted)
    {
        pending.store(wanted, std::memory_order_release);
        if (ResetRequested.is_initialized()) ResetRequested = 1;
    }

    inline void Place()
    {
        if (!window) return;
        WindowedModeWrapper::GameHWND = window;
        WindowedModeWrapper::bEnableWindowResize = false;
        WindowedModeWrapper::bBorderlessWindowed = mode != 1;
        WindowedModeWrapper::bScaleWindow = false;
        WindowedModeWrapper::bStretchWindow = false;
        auto width = requestedWidth, height = requestedHeight;
        const auto monitor = WindowedModeWrapper::WindowMonitor();
        if (mode == 2)
        {
            width = monitor.rcMonitor.right - monitor.rcMonitor.left;
            height = monitor.rcMonitor.bottom - monitor.rcMonitor.top;
        }
        RECT client{}, outer{};
        GetClientRect(window, &client);
        GetWindowRect(window, &outer);
        const auto style = GetWindowLongW(window, GWL_STYLE);
        const bool framed = (style & WS_CAPTION) == WS_CAPTION;
        const bool correctPosition = mode != 2 || EqualRect(&outer, &monitor.rcMonitor);
        if (styled && framed == (mode == 1) && client.right == width && client.bottom == height && correctPosition)
            return;
        WindowedModeWrapper::desiredClientWidth = width;
        WindowedModeWrapper::desiredClientHeight = height;
        auto [x, y, clientWidth, clientHeight] = WindowedModeWrapper::beforeCreateWindow(width, height);
        if (mode == 2)
            SetWindowPos(window, HWND_NOTOPMOST, x, y, clientWidth, clientHeight, SWP_NOACTIVATE);
        else if (!mode)
            SetWindowPos(window, nullptr, monitor.rcMonitor.left, monitor.rcMonitor.top, width, height,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        WindowedModeWrapper::afterCreateWindow();
        styled = true;
    }

    inline LRESULT CALLBACK Messages(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        LRESULT handled{};
        if (messageHook && messageHook(hwnd, message, wParam, lParam, handled))
            return handled;
        // In window modes the native handler minimizes the window on deactivation and maximizes it on activation.
        if (message == WM_ACTIVATE && mode && pauseGame && resumeGame && GameActive.is_initialized())
        {
            if (LOWORD(wParam) != WA_INACTIVE)
                resumeGame();
            else
            {
                pauseGame();
                if (!FixSettings().Bool("MAIN.PauseOnFocusLoss"))
                    GameActive = 1;
            }
            return 0;
        }
        if (message == WM_SYSKEYDOWN && wParam == VK_RETURN && !(lParam & (1L << 30)))
        {
            Request(mode ? 0 : lastWindowMode);
            return 0;
        }
        if (message == ChangeMessage)
        {
            Request(std::clamp(static_cast<int>(wParam), 0, 2));
            return 0;
        }
        if (message == WM_SYSCHAR && wParam == VK_RETURN) return 0;
        if (message == WM_NCDESTROY)
        {
            RemoveWindowSubclass(hwnd, Messages, 1);
            window = nullptr;
            styled = false;
        }
        return DefSubclassProc(hwnd, message, wParam, lParam);
    }

    inline BOOL WINAPI Show(HWND hwnd, int command)
    {
        if (hwnd && hwnd == window && mode && (command == SW_MAXIMIZE || command == SW_MINIMIZE))
            return TRUE;
        return nativeShow(hwnd, command);
    }

    inline BOOL WINAPI Position(HWND hwnd, HWND after, int x, int y, int width, int height, UINT flags)
    {
        // The game sizes the outer window to the back buffer; window modes keep the exact client placement.
        if (hwnd && hwnd == window && mode && !(flags & SWP_NOSIZE))
        {
            Place();
            return TRUE;
        }
        return nativePosition(hwnd, after, x, y, width, height, flags);
    }

    inline HWND __cdecl Create(LPCSTR className, LPCSTR title, HINSTANCE instance, int x, int y, int width, int height, DWORD fullscreen)
    {
        auto result = createHook.ccall<HWND>(className, title, instance, mode ? -1 : x, mode ? -1 : y,
            width, height, mode ? 0u : fullscreen);
        if (result)
        {
            window = result;
            SetWindowSubclass(window, Messages, 1, 0);
            WindowCursor::Attach(window);
            Place();
        }
        return result;
    }

    inline void __cdecl Parameters()
    {
        int width = requestedWidth, height = requestedHeight;
        if (mode == 2)
        {
            WindowedModeWrapper::GameHWND = window;
            const auto monitor = WindowedModeWrapper::WindowMonitor();
            width = monitor.rcMonitor.right - monitor.rcMonitor.left;
            height = monitor.rcMonitor.bottom - monitor.rcMonitor.top;
        }
        resolution(width, height);
        parametersHook.ccall<void>();
        if (mode)
        {
            PresentationParameters->Windowed = TRUE;
            PresentationParameters->FullScreen_RefreshRateInHz = 0;
            PresentationParameters->BackBufferFormat = D3DFMT_UNKNOWN;
            D3DDISPLAYMODE display{};
            DWORD colorQuality = 0, depthQuality = 0;
            const auto sample = PresentationParameters->MultiSampleType;
            if (sample != D3DMULTISAMPLE_NONE && Direct3D.is_initialized() && Direct3D.get()
                && SUCCEEDED(Direct3D->GetAdapterDisplayMode(0, &display))
                && SUCCEEDED(Direct3D->CheckDeviceMultiSampleType(0, D3DDEVTYPE_HAL, display.Format, TRUE, sample, &colorQuality))
                && SUCCEEDED(Direct3D->CheckDeviceMultiSampleType(0, D3DDEVTYPE_HAL, PresentationParameters->AutoDepthStencilFormat, TRUE, sample, &depthQuality))
                && colorQuality && depthQuality)
                PresentationParameters->MultiSampleQuality = std::min(PresentationParameters->MultiSampleQuality, std::min(colorQuality, depthQuality) - 1);
            else
            {
                PresentationParameters->MultiSampleType = D3DMULTISAMPLE_NONE;
                PresentationParameters->MultiSampleQuality = 0;
            }
        }
        if (mode) Place();
    }

    inline char __cdecl Reset()
    {
        const auto wanted = pending.exchange(-1, std::memory_order_acquire);
        const auto previous = mode;
        const bool transition = wanted >= 0 && wanted <= 2 && wanted != mode && !changing;
        if (transition)
        {
            changing = true;
            mode = wanted;
            Place();
            ResetRequested = 1;
        }
        auto result = resetHook.ccall<char>();
        if (transition)
        {
            if (ResetRequested != 0)
            {
                // The native reset keeps its lost-resource state on failure;
                // retry the former display mode through the same recovery path.
                mode = previous;
                Place();
                result = resetHook.ccall<char>();
            }
            else if (mode)
                lastWindowMode = mode;
            FixSettings().Set("MAIN.WindowMode", std::to_string(mode));
            Place();
            changing = false;
        }
        return result;
    }

    inline void Init(int width, int height, std::function<void(int, int)> update)
    {
        auto create = hook::pattern("83 EC 10 8A 4C 24 30 53 8B 5C 24 28 55 8B 6C 24 28 56 33 C0 84 C9 57 89 44 24 30 50");
        auto build = hook::pattern("A1 ? ? ? ? 8B 08 83 EC 14 53 8D 54 24 08 52 33 DB 53 50 FF 51 20 85 C0 0F 8C ? ? ? ? 33 C0 56 57 B9 0E 00 00 00");
        auto reset = hook::pattern("51 A0 ? ? ? ? 84 C0 56 0F 84 ? ? ? ? 8B 35 ? ? ? ? 8B 4E 08 6A 01 8D 44 24 08 50 C7 44 24 0C 18 5D 18 13");
        auto buffer = hook::pattern("B9 0E 00 00 00 BF ? ? ? ? F3 AB A1 ? ? ? ? 8B 0D");
        requestedWidth = width;
        requestedHeight = height;
        resolution = std::move(update);
        PresentationParameters.SetAddress(*buffer.get_first<D3DPRESENT_PARAMETERS*>(6));
        Direct3D.SetAddress(*build.get_first<IDirect3D9**>(1));
        ResetRequested.SetAddress(*reset.get_first<uint8_t*>(2));
        mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
        if (mode) lastWindowMode = mode;
        createHook = safetyhook::create_inline(create.get_first(), Create);
        parametersHook = safetyhook::create_inline(build.get_first(), Parameters);
        resetHook = safetyhook::create_inline(reset.get_first(), Reset);
        auto pause = hook::pattern("6A 00 68 ? ? ? ? 6A 08 6A 3B C6 05 ? ? ? ? 00 FF 15 ? ? ? ? C3");
        auto resume = hook::pattern("6A 00 68 ? ? ? ? 6A 08 6A 3B FF 15 ? ? ? ? C6 05 ? ? ? ? 01 C3");
        pauseGame = reinterpret_cast<void(__cdecl*)()>(pause.get_first());
        resumeGame = reinterpret_cast<void(__cdecl*)()>(resume.get_first());
        GameActive.SetAddress(*pause.get_first<uint8_t*>(13));
        nativeShow = GunImports::Replace("USER32.dll", "ShowWindow", &Show);
        nativePosition = GunImports::Replace("USER32.dll", "SetWindowPos", &Position);
        if (!nativeShow) nativeShow = ShowWindow;
        if (!nativePosition) nativePosition = SetWindowPos;
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        FixSettings().OnChange("MAIN.ConfineCursor", []()
        {
            CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        });
        FixSettings().OnChange("MAIN.WindowMode", []()
        {
            if (!changing)
                PostMessageW(window, ChangeMessage, std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2), 0);
        });
    }
}
