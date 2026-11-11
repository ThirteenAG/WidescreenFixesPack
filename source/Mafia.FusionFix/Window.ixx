module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"

export module Window;

import ComVars;
import DeveloperOverlay;
import PostFX;
import WidescreenFix;

// Window modes (Alt+Enter switches between full screen and the window), cursor confinement, focus, the per-frame work
namespace MafiaWindow
{
    int(__fastcall* resetDevice)(char) = nullptr;
    void(__fastcall* restartRenderer)(uint32_t) = nullptr;
    LONG toggleRequested = 0;
    bool changing = false;
    bool enabled = false;
    int lastWindowMode = 1;
    uint32_t windowFormat = 22;
    uint32_t fullscreenFormat = 22;
    uint32_t fullscreenInterval = 0;
    uint32_t fullscreenRefresh = 0;
    RECT windowRect{};
    bool haveWindowRect = false;
    constexpr wchar_t WindowProperty[] = L"Mafia.FusionFix.WindowMode";

    struct WindowState
    {
        WNDPROC original;
    };

    LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto state = reinterpret_cast<WindowState*>(GetPropW(window, WindowProperty));
        if (!state)
            return DefWindowProcW(window, message, wParam, lParam);
        LRESULT overlayResult = 0;
        if (MafiaOverlay::Message(window, message, wParam, lParam, overlayResult))
            return overlayResult;
        if (window == WindowedModeWrapper::GameHWND)
            CursorConfinement::Message(message, wParam);
        if (enabled && wParam == VK_RETURN && (message == WM_SYSKEYDOWN || message == WM_SYSKEYUP))
        {
            if (message == WM_SYSKEYDOWN && (lParam & (1u << 29)) && !(lParam & (1u << 30)))
                InterlockedExchange(&toggleRequested, 1);
            return 0;
        }
        // Reset may synchronously activate the window while rebuilding the device.
        if (changing && message == WM_ACTIVATE)
            return DefWindowProcW(window, message, wParam, lParam);
        auto original = state->original;
        if (message == WM_NCDESTROY)
        {
            if (window == WindowedModeWrapper::GameHWND)
                WindowedModeWrapper::GameHWND = nullptr;
            RemovePropW(window, WindowProperty);
            InterlockedExchange(&toggleRequested, 0);
            delete state;
        }
        return CallWindowProcW(original, window, message, wParam, lParam);
    }

    void Attach(HWND window)
    {
        if (!enabled || !window || GetPropW(window, WindowProperty))
            return;
        auto state = new WindowState{ reinterpret_cast<WNDPROC>(GetWindowLongPtrW(window, GWLP_WNDPROC)) };
        if (!SetPropW(window, WindowProperty, state))
        {
            delete state;
            return;
        }
        SetLastError(0);
        if (!SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)) && GetLastError())
        {
            RemovePropW(window, WindowProperty);
            delete state;
        }
    }

    void StoreParameters(const PresentParameters& value)
    {
        EnginePresentParameters = value;
    }

    void ApplyWindow(HWND window, int mode)
    {
        auto style = GetWindowLongW(window, GWL_STYLE);
        auto exStyle = GetWindowLongW(window, GWL_EXSTYLE);
        SetWindowLongW(window, GWL_EXSTYLE, exStyle & ~WS_EX_TOPMOST);
        if (mode)
        {
            WindowedModeWrapper::bBorderlessWindowed = mode == 2;
            WindowedModeWrapper::bScaleWindow = mode == 2;
            WindowedModeWrapper::desiredClientWidth = nWindowWidth;
            WindowedModeWrapper::desiredClientHeight = nWindowHeight;
            WindowedModeWrapper::afterCreateWindow();
            if (mode == 2)
                WindowedModeWrapper::CenterWindowPosition(nWindowWidth, nWindowHeight);
            else if (haveWindowRect)
                SetWindowPos(window, HWND_NOTOPMOST, windowRect.left, windowRect.top,
                    windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
                    SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
        }
        else
        {
            MONITORINFO info{ sizeof(info) };
            GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &info);
            SetWindowLongW(window, GWL_STYLE, (style & ~WS_OVERLAPPEDWINDOW) | WS_POPUP);
            SetWindowPos(window, HWND_TOP, info.rcMonitor.left, info.rcMonitor.top,
                nWindowWidth, nWindowHeight, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
        }
    }

    void Toggle(SafetyHookContext&)
    {
        if (!InterlockedExchange(&toggleRequested, 0))
            return;
        auto window = WindowedModeWrapper::GameHWND;
        if (!window || !pEngineDevice || changing || GetForegroundWindow() != window)
            return;
        void* device = pEngineDevice;
        auto table = *reinterpret_cast<uintptr_t**>(device);
        using TestCooperativeLevel = HRESULT(WINAPI*)(void*);
        if (reinterpret_cast<TestCooperativeLevel>(table[3])(device) != D3D_OK)
            return;

        const PresentParameters saved = EnginePresentParameters;
        const uint32_t flags = nEngineFlags;
        const int previousMode = nWindowMode;
        const int nextMode = previousMode ? 0 : lastWindowMode;
        const auto style = GetWindowLongW(window, GWL_STYLE);
        const auto exStyle = GetWindowLongW(window, GWL_EXSTYLE);
        RECT previousRect{};
        GetWindowRect(window, &previousRect);
        if (previousMode == 1)
        {
            windowRect = previousRect;
            haveWindowRect = true;
        }
        if (!previousMode)
        {
            fullscreenFormat = saved.format;
            fullscreenInterval = saved.interval;
            fullscreenRefresh = saved.refreshRate;
        }
        else
            windowFormat = saved.format;
        auto requested = saved;
        requested.width = nWindowWidth;
        requested.height = nWindowHeight;
        requested.windowed = nextMode != 0;
        requested.format = nextMode ? windowFormat : fullscreenFormat;
        requested.refreshRate = nextMode ? 0 : fullscreenRefresh;
        requested.interval = nextMode ? 0 : fullscreenInterval;
        changing = true;
        nWindowMode = nextMode;
        nEngineFlags = nextMode ? flags & ~2u : flags | 2u;
        ApplyWindow(window, nextMode);
        StoreParameters(requested);
        if (resetDevice(1) >= 0)
        {
            if (previousMode)
                lastWindowMode = previousMode;
            if ((flags & 0x100000u) && restartRenderer)
                restartRenderer(nEngineFlags);
        }
        else
        {
            OutputDebugStringA("Mafia: window mode reset failed; restoring the previous mode.\n");
            nWindowMode = previousMode;
            nEngineFlags = flags;
            SetWindowLongW(window, GWL_STYLE, style);
            SetWindowLongW(window, GWL_EXSTYLE, exStyle);
            SetWindowPos(window, (exStyle & WS_EX_TOPMOST) ? HWND_TOPMOST : HWND_NOTOPMOST,
                previousRect.left, previousRect.top, previousRect.right - previousRect.left,
                previousRect.bottom - previousRect.top, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
            WindowedModeWrapper::bBorderlessWindowed = previousMode == 2;
            WindowedModeWrapper::bScaleWindow = previousMode == 2;
            StoreParameters(saved);
            if (resetDevice(1) >= 0)
            {
                if ((flags & 0x100000u) && restartRenderer)
                    restartRenderer(flags);
            }
            else
                OutputDebugStringA("Mafia: previous mode reset failed; native device recovery remains active.\n");
        }
        changing = false;
    }

    BOOL WINAPI ClipCursorHook(const RECT* rect)
    {
        auto window = WindowedModeWrapper::GameHWND;
        if (!window)
            return ClipCursor(rect);
        if (!rect)
            CursorConfinement::Release();
        else
            CursorConfinement::Update(window);
        return TRUE;
    }

    void Frame(SafetyHookContext& regs)
    {
        auto graphics = GetDevice9();
        if (graphics)
        {
            IDirect3DSurface9* buffer = nullptr;
            if (SUCCEEDED(graphics->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &buffer)))
            {
                D3DSURFACE_DESC description{};
                if (SUCCEEDED(buffer->GetDesc(&description)))
                    InitializePresentation(description.Width, description.Height);
                buffer->Release();
            }
        }
        else
            InitializePresentation(nWindowWidth, nWindowHeight);
        MafiaOverlay::Attach(graphics);
        MafiaOverlay::Keyboard(WindowedModeWrapper::GameHWND);
        WFP::onGameProcessEvent().executeAll();
        const bool overlay = MafiaOverlay::BlocksInput();
        static bool previousOverlay = false;
        if (overlay != previousOverlay)
            CursorConfinement::Suspend(overlay);
        previousOverlay = overlay;
        Toggle(regs);
        CursorConfinement::Update(WindowedModeWrapper::GameHWND);
        CursorConfinement::Bind();
    }

    void Init(HMODULE engine)
    {
        auto present = hook::module_pattern(engine, "83 EC 34 53 56 E8 ? ? ? ? A0 ? ? ? ? 84 C0 0F 84 ? ? ? ? FF 05");
        auto reset = hook::module_pattern(engine, "83 EC 3C 85 C0 53 56 8A D9 74 ? 8B 0D");
        auto parameters = hook::module_pattern(engine, "A1 ? ? ? ? 8B 08 68 ? ? ? ? 50 FF 51 38 85 C0 7D");
        auto restart = hook::module_pattern(engine, "B1 01 E8 ? ? ? ? 85 C0 7D ? BE 68 08 76 88 EB ? 8B 0D ? ? ? ? F7 C1 00 00 10 00 74 ? E8 ? ? ? ? 68");
        bool originalEngine = false;
        if (present.empty() && reset.empty()) // 1.0 engine
        {
            present = hook::module_pattern(engine, "83 EC 34 56 57 E8 ? ? ? ? A0 ? ? ? ? 84 C0 0F 84 ? ? ? ? FF 05");
            reset = hook::module_pattern(engine, "83 EC 3C 85 C0 53 55 56 8A D9 74 ? 8B 0D");
            originalEngine = true;
        }
        DEVMODEW desktop{};
        desktop.dmSize = sizeof(desktop);
        if (EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &desktop) && desktop.dmBitsPerPel == 16)
            windowFormat = 23;
        fullscreenInterval = (nEngineFlags & 0x100u) ? 1u : 0x80000000u;
        pEngineDevice.SetAddress(*parameters.get_first<void**>(1));
        EnginePresentParameters.SetAddress(*parameters.get_first<PresentParameters*>(8));
        // The function starts 5 bytes before the pattern: its first instruction is left out, other plugins hook it
        resetDevice = reinterpret_cast<decltype(resetDevice)>(reset.get_first(-5));
        if (!originalEngine)
            restartRenderer = reinterpret_cast<decltype(restartRenderer)>(injector::GetBranchDestination(restart.get_first(32)).as_int());
        lastWindowMode = nWindowMode ? nWindowMode : 1;
        static auto presentHook = safetyhook::create_mid(present.get_first(), Frame);
        enabled = true;
        IATHook::Replace(engine, "USER32.dll", std::forward_as_tuple("ClipCursor", ClipCursorHook));
    }
}

BOOL WINAPI SetWindowPosHook(HWND window, HWND after, int x, int y, int width, int height, UINT flags)
{
    if (!nWindowMode || window != WindowedModeWrapper::GameHWND ||
        (flags & (SWP_NOMOVE | SWP_NOSIZE)) == (SWP_NOMOVE | SWP_NOSIZE))
        return SetWindowPos(window, after, x, y, width, height, flags);
    WindowedModeWrapper::desiredClientWidth = nWindowWidth;
    WindowedModeWrapper::desiredClientHeight = nWindowHeight;
    WindowedModeWrapper::afterCreateWindow();
    if (nWindowMode == 2)
        WindowedModeWrapper::CenterWindowPosition(nWindowWidth, nWindowHeight);
    return SetWindowPos(window, after, 0, 0, 0, 0, (flags | SWP_NOMOVE | SWP_NOSIZE) & ~SWP_FRAMECHANGED);
}

HWND WINAPI CreateWindowExAHook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    if (nWindowMode && !(dwStyle & WS_CHILD))
    {
        WindowedModeWrapper::bBorderlessWindowed = nWindowMode == 2;
        // Borderless renders at the desktop size; scaling keeps the render aspect on a monitor of another shape
        WindowedModeWrapper::bScaleWindow = nWindowMode == 2;
        WindowedModeWrapper::desiredClientWidth = nWindowWidth;
        WindowedModeWrapper::desiredClientHeight = nWindowHeight;
        if (nWindowMode == 2 && nWindowWidth > 0 && nWindowHeight > 0)
        {
            nWidth = nWindowWidth;
            nHeight = nWindowHeight;
        }
        auto window = WindowedModeWrapper::CreateWindowExA_Hook(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
        MafiaWindow::Attach(window);
        return window;
    }
    auto window = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    if (window && !(dwStyle & WS_CHILD))
    {
        WindowedModeWrapper::GameHWND = window;
        MafiaWindow::Attach(window);
        using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        static auto setAttribute = []() -> SetAttribute
        {
            auto dwm = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            return dwm ? reinterpret_cast<SetAttribute>(GetProcAddress(dwm, "DwmSetWindowAttribute")) : nullptr;
        }();
        int preference = 1;
        if (setAttribute)
            setAttribute(window, 33, &preference, sizeof(preference));
    }
    return window;
}

export void InitWindowMode()
{
    auto engine = GetLS3DF();
    auto mode = hook::module_pattern(engine, "F6 05 ? ? ? ? 02 8B 35 ? ? ? ? 0F 84 ? ? ? ? 8B 3D ? ? ? ? 55 55 55 55");
    auto create = hook::module_pattern(engine, "FF 15 ? ? ? ? 3B C5 A3 ? ? ? ? 0F 84 ? ? ? ? 50");
    nWindowMode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
    nEngineFlags.SetAddress(*mode.get_first<uint32_t*>(2));
    nEngineWidth.SetAddress(nEngineFlags.get_ptr() - 7);
    nEngineHeight.SetAddress(nEngineFlags.get_ptr() - 6);
    CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
    FixSettings().OnChange("MAIN.ConfineCursor", []()
    {
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
    });
    MafiaWindow::Init(engine);
    static auto windowMode = safetyhook::create_mid(mode.get_first(), [](SafetyHookContext&)
    {
        uint32_t flags = nEngineFlags;
        flags = nWindowMode ? (flags & ~(2u | 8u | 0x80u)) : (flags | 2u);
        flags = FixSettings().Bool("MAIN.PauseOnFocusLoss") ? flags | 0x20u : flags & ~0x20u;
        nEngineFlags = flags;
        ResolveResolution(nWindowMode);
        nEngineWidth = nWindowWidth;
        nEngineHeight = nWindowHeight;
        if (nResolutionWidth.is_initialized() && nResolutionHeight.is_initialized())
        {
            nResolutionWidth = nWindowWidth;
            nResolutionHeight = nWindowHeight;
        }
    });
    FixSettings().OnChange("MAIN.PauseOnFocusLoss", []()
    {
        const bool pause = FixSettings().Bool("MAIN.PauseOnFocusLoss");
        nEngineFlags = pause ? nEngineFlags | 0x20u : nEngineFlags & ~0x20u;
        auto window = WindowedModeWrapper::GameHWND;
        if (!pause && window && GetForegroundWindow() != window)
            SendMessageW(window, WM_ACTIVATE, WA_INACTIVE, 0);
    });
    injector::WriteMemory<decltype(&CreateWindowExAHook)>(*create.get_first<uint32_t*>(2), CreateWindowExAHook, true);
    IATHook::Replace(engine, "USER32.dll", std::forward_as_tuple("SetWindowPos", SetWindowPosHook));
}

class Window
{
public:
    Window()
    {
        WFP::onShutdownEvent() += []()
        {
            CursorConfinement::Shutdown();
        };
    }
} Window;
