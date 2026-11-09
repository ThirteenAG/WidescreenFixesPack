module;

#include <stdafx.h>
#include <d3d9.h>
#include "RawMouse.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Window;

import Overlay;
import ComVars;

export namespace BullyWindow
{
    SafetyHookInline OptionsHook, ResetHook, FrameHook;
    SafetyHookMid ValidationHook;
    int Mode = 0, LastWindowMode = 1;
    HWND Window = nullptr;
    WNDPROC OriginalProc = nullptr;
    bool Changing = false;
    LONG Pending = 0;
    RECT FramedRect{};
    bool HaveFramedRect = false;
    int FramedWidth = 0, FramedHeight = 0;
    std::function<void()> OnResolutionChanged;

    void SquareCorners(HWND window)
    {
        using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        static auto setAttribute = []() -> SetAttribute
        {
            auto module = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            return module ? reinterpret_cast<SetAttribute>(GetProcAddress(module, "DwmSetWindowAttribute")) : nullptr;
        }();
        const int preference = 1;
        if (setAttribute)
            setAttribute(window, 33, &preference, sizeof(preference));
    }

    MONITORINFOEXA Monitor()
    {
        MONITORINFOEXA monitor{ sizeof(monitor) };
        GetMonitorInfoA(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &monitor);
        return monitor;
    }

    void BorderlessResolution()
    {
        if (Mode != 2)
            return;
        auto monitor = Monitor();
        const int width = monitor.rcMonitor.right - monitor.rcMonitor.left;
        const int height = monitor.rcMonitor.bottom - monitor.rcMonitor.top;
        if (width > 0 && height > 0)
        {
            DisplayWidth = width;
            DisplayHeight = height;
        }
    }

    // Activation messages that arrive while the window is being placed are kept from the game,
    // so tell it the real state once placing is done.
    void SyncActivation()
    {
        if (!Window || !FocusActive.is_initialized())
            return;
        const bool active = GetForegroundWindow() == Window;
        if (active != (FocusActive != 0))
            PostMessageA(Window, WM_ACTIVATE, active ? WA_ACTIVE : WA_INACTIVE, 0);
    }

    void Place()
    {
        if (!Window || !DisplayWidth.is_initialized() || !DisplayHeight.is_initialized())
            return;
        const bool wasChanging = std::exchange(Changing, true);
        WindowedModeWrapper::GameHWND = Window;
        WindowedModeWrapper::bBorderlessWindowed = Mode == 2;
        WindowedModeWrapper::bEnableWindowResize = false;
        WindowedModeWrapper::desiredClientWidth = DisplayWidth;
        WindowedModeWrapper::desiredClientHeight = DisplayHeight;
        const auto style = GetWindowLongA(Window, GWL_STYLE);
        SetWindowLongA(Window, GWL_STYLE, (style & ~WS_OVERLAPPEDWINDOW) | WS_POPUP);
        if (Mode == 1)
        {
            WindowedModeWrapper::afterCreateWindow();
            if (HaveFramedRect && FramedWidth == DisplayWidth && FramedHeight == DisplayHeight)
                SetWindowPos(Window, HWND_NOTOPMOST, FramedRect.left, FramedRect.top,
                    FramedRect.right - FramedRect.left, FramedRect.bottom - FramedRect.top,
                    SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
        }
        else
        {
            auto monitor = Monitor();
            SetWindowPos(Window, Mode ? HWND_NOTOPMOST : HWND_TOP, monitor.rcMonitor.left, monitor.rcMonitor.top,
                Mode == 2 ? monitor.rcMonitor.right - monitor.rcMonitor.left : DisplayWidth,
                Mode == 2 ? monitor.rcMonitor.bottom - monitor.rcMonitor.top : DisplayHeight,
                SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
        }
        SquareCorners(Window);
        Changing = wasChanging;
        if (!Changing)
            SyncActivation();
    }

    LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        BullyRawMouse::Message(window, message, lParam, BullyOverlay::BlocksInput());
        LRESULT result = 0;
        if (BullyOverlay::Message(window, message, wParam, lParam, result))
            return result;
        if (window == Window)
            CursorConfinement::Message(message, wParam);
        if ((message == WM_SYSKEYDOWN || message == WM_SYSKEYUP) && wParam == VK_RETURN)
        {
            if (message == WM_SYSKEYDOWN && (lParam & (1u << 29)) && !(lParam & (1u << 30)))
                InterlockedExchange(&Pending, 1);
            return 0;
        }
        if (Changing && (message == WM_ACTIVATE || message == WM_ACTIVATEAPP || message == WM_KILLFOCUS || message == WM_SETFOCUS || message == WM_SIZE))
            return DefWindowProcA(window, message, wParam, lParam);
        auto original = OriginalProc;
        if (message == WM_NCDESTROY)
        {
            Window = nullptr;
            OriginalProc = nullptr;
            InterlockedExchange(&Pending, 0);
        }
        return original ? CallWindowProcA(original, window, message, wParam, lParam) : DefWindowProcA(window, message, wParam, lParam);
    }

    HWND WINAPI Create(DWORD exStyle, LPCSTR className, LPCSTR title, DWORD style, int x, int y, int width, int height,
        HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter)
    {
        auto window = CreateWindowExA(exStyle, className, title, style, x, y, width, height, parent, menu, instance, parameter);
        if (window && !parent && !(style & WS_CHILD))
        {
            Window = window;
            OriginalProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)));
            Place();
        }
        return window;
    }

    BOOL WINAPI Move(HWND window, int x, int y, int width, int height, BOOL repaint)
    {
        if (Mode && window == Window)
        {
            return TRUE;
        }
        return MoveWindow(window, x, y, width, height, repaint);
    }

    BOOL WINAPI Position(HWND window, HWND after, int x, int y, int width, int height, UINT flags)
    {
        if (Mode && window == Window && !(flags & SWP_NOSIZE))
        {
            Place();
            return TRUE;
        }
        return SetWindowPos(window, after, x, y, width, height, flags);
    }

    BOOL WINAPI Show(HWND window, int command)
    {
        if ((Mode || Changing) && window == Window && (command == SW_MINIMIZE || command == SW_SHOWMINIMIZED || command == SW_SHOWMINNOACTIVE))
            return IsWindowVisible(window);
        return ShowWindow(window, command);
    }

    LONG WINAPI Display(DEVMODEA* mode, DWORD flags)
    {
        if (Mode || Changing)
            return DISP_CHANGE_SUCCESSFUL;
        if (!Window)
            return ChangeDisplaySettingsA(mode, flags);
        auto monitor = Monitor();
        return ChangeDisplaySettingsExA(monitor.szDevice, mode, nullptr, flags, nullptr);
    }

    int* __cdecl Options(int* options)
    {
        auto result = OptionsHook.ccall<int*>(options);
        WindowedMode = Mode != 0;
        BorderlessResolution();
        if (Mode == 2 && OnResolutionChanged)
            OnResolutionChanged();
        *options = DisplayWidth;
        options[1] = DisplayHeight;
        options[5] = (options[5] & ~0xFF) | (Mode != 0);
        RECT client{};
        if (Mode && Window && GetClientRect(Window, &client) &&
            (client.right != DisplayWidth || client.bottom != DisplayHeight))
            Place();
        return result;
    }

    char __cdecl Reset()
    {
        BorderlessResolution();
        WindowedMode = Mode != 0;
        const char result = ResetHook.ccall<char>();
        if (result)
        {
            Place();
            if (OnResolutionChanged)
                OnResolutionChanged();
        }
        return result;
    }

    LONG ChangeDisplay(int mode)
    {
        auto monitor = Monitor();
        DEVMODEA display{};
        display.dmSize = sizeof(display);
        if (mode)
            return ChangeDisplaySettingsExA(monitor.szDevice, nullptr, nullptr, 0, nullptr);
        display.dmPelsWidth = DisplayWidth;
        display.dmPelsHeight = DisplayHeight;
        display.dmBitsPerPel = 32;
        display.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
        return ChangeDisplaySettingsExA(monitor.szDevice, &display, nullptr, CDS_FULLSCREEN, nullptr);
    }

    void Toggle()
    {
        if (!Pending || Changing || !Window || GetForegroundWindow() != Window)
            return;
        if (!RendererState.is_initialized() || !RendererState.get() || (RendererState.get())[1272])
            return;
        auto device = *reinterpret_cast<IDirect3DDevice9**>(RendererState.get() + 164);
        if (!device || device->TestCooperativeLevel() != D3D_OK)
            return;
        if (!InterlockedExchange(&Pending, 0))
            return;
        const int previous = Mode;
        const int next = previous ? 0 : LastWindowMode;
        const int width = DisplayWidth, height = DisplayHeight;
        RECT previousRect{};
        GetWindowRect(Window, &previousRect);
        if (previous == 1)
        {
            FramedRect = previousRect;
            HaveFramedRect = true;
            FramedWidth = width;
            FramedHeight = height;
        }
        Changing = true;
        Mode = next;
        if (ChangeDisplay(next) == DISP_CHANGE_SUCCESSFUL && Reset())
        {
            if (previous)
                LastWindowMode = previous;
        }
        else
        {
            OutputDebugStringA("Bully: window mode change failed; restoring the previous mode.\n");
            Mode = previous;
            DisplayWidth = width;
            DisplayHeight = height;
            ChangeDisplay(previous);
            if (!Reset())
                OutputDebugStringA("Bully: previous mode reset failed; native device recovery remains active.\n");
            SetWindowPos(Window, nullptr, previousRect.left, previousRect.top,
                previousRect.right - previousRect.left, previousRect.bottom - previousRect.top,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
        }
        Changing = false;
        SyncActivation();
    }

    int __cdecl Frame()
    {
        BullyRawMouse::Attach(Window);
        if (RendererState.is_initialized() && RendererState.get())
            BullyOverlay::Attach(*reinterpret_cast<IDirect3DDevice9**>(RendererState.get() + 164));
        BullyOverlay::Keyboard(Window);
        static std::once_flag gameInit;
        std::call_once(gameInit, []()
        {
            WFP::onGameInitEvent().executeAll();
        });
        WFP::onGameProcessEvent().executeAll();
        const int result = FrameHook.ccall<int>();
        Toggle();
        static bool overlayInput = false;
        if (overlayInput != BullyOverlay::BlocksInput())
        {
            overlayInput = BullyOverlay::BlocksInput();
            CursorConfinement::Suspend(overlayInput);
        }
        CursorConfinement::Update(Window);
        CursorConfinement::Bind();
        return result;
    }

    void Install(int mode, HMODULE module = GetModuleHandle(nullptr), bool imports = true)
    {
        auto options = hook::module_pattern(module, "80 3D ? ? ? ? 00 74 4A C6 05 ? ? ? ? 00 E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ?");
        auto dimensions = hook::module_pattern(module, "8B 44 24 04 8B 15 ? ? ? ? 89 10 8B 15 ? ? ? ? 89 50 04");
        auto windowed = hook::module_pattern(module, "8B 0D ? ? ? ? 89 48 14 C3 CC CC CC");
        auto reset = hook::module_pattern(module, "83 EC 20 53 56 E8 ? ? ? ? 8B 0D ? ? ? ? 8B 51 2C 8B 35");
        auto frame = hook::module_pattern(module, "8B 0D ? ? ? ? B8 03 00 00 00 39 81 94 00 00 00 75 23 8B 01 8B 90 50 01 00 00");
        auto renderer = hook::module_pattern(module, "56 8B 35 ? ? ? ? 85 F6 75 04 B0 01 5E C3 53 8B 1D");
        auto validation = hook::module_pattern(module, "80 3D ? ? ? ? 00 75 11 68 ? ? ? ? E8 ? ? ? ? 83 C4 04 84 C0 74 26");
        Mode = std::clamp(mode, 0, 2);
        LastWindowMode = Mode ? Mode : 1;
        (void)*dimensions.get_first<int*>(6);
        (void)*dimensions.get_first<int*>(14);
        WindowedMode.SetAddress(*windowed.get_first<uint8_t*>(2));
        RendererState.SetAddress(*renderer.get_first<uint8_t**>(3));
        ValidationHook = safetyhook::create_mid(validation.get_first(), [](SafetyHookContext& regs)
        {
            // Saved fullscreen flags must not reject a windowed custom resolution.
            WindowedMode = Mode != 0;
        });
        OptionsHook = safetyhook::create_inline(options.get_first(), Options);
        ResetHook = safetyhook::create_inline(reset.get_first(), Reset);
        FrameHook = safetyhook::create_inline(frame.get_first(), Frame);
        if (imports)
        {
            IATHook::Replace(module, "USER32.dll",
                std::forward_as_tuple("CreateWindowExA", Create),
                std::forward_as_tuple("MoveWindow", Move),
                std::forward_as_tuple("SetWindowPos", Position),
                std::forward_as_tuple("ShowWindow", Show),
                std::forward_as_tuple("ChangeDisplaySettingsA", Display));
        }
    }
}
