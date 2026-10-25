#pragma once
#include "../Shared/CursorConfinement.hpp"

namespace MafiaWindow
{
    struct PresentParameters
    {
        uint32_t width, height, format, buffers, multisample, swapEffect;
        HWND window;
        BOOL windowed, depthBuffer;
        uint32_t depthFormat, flags, refreshRate, interval;
    };
    static_assert(sizeof(PresentParameters) == 52);

    inline PresentParameters* presentParameters = nullptr;
    inline uint32_t* engineFlags = nullptr;
    inline void** device = nullptr;
    inline int(__fastcall* resetDevice)(char) = nullptr;
    inline void(__fastcall* restartRenderer)(uint32_t) = nullptr;
    inline LONG toggleRequested = 0;
    inline bool changing = false;
    inline bool enabled = false;
    inline int lastWindowMode = 1;
    inline uint32_t windowFormat = 22;
    inline uint32_t fullscreenFormat = 22;
    inline uint32_t fullscreenInterval = 0;
    inline uint32_t fullscreenRefresh = 0;
    inline RECT windowRect{};
    inline bool haveWindowRect = false;
    constexpr wchar_t WindowProperty[] = L"Mafia.FusionFix.WindowMode";

    struct WindowState
    {
        WNDPROC original;
    };

    inline LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto state = reinterpret_cast<WindowState*>(GetPropW(window, WindowProperty));
        if (!state)
            return DefWindowProcW(window, message, wParam, lParam);
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

    inline void Attach(HWND window)
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

    inline void StoreParameters(const PresentParameters& value)
    {
        auto source = reinterpret_cast<const uint32_t*>(&value);
        auto target = reinterpret_cast<uint32_t*>(presentParameters);
        for (size_t i = 0; i < sizeof(value) / sizeof(uint32_t); ++i)
            ProtectedGameRef<uint32_t>{ target + i } = source[i];
    }

    inline void ApplyWindow(HWND window, int mode)
    {
        auto style = GetWindowLongW(window, GWL_STYLE);
        auto exStyle = GetWindowLongW(window, GWL_EXSTYLE);
        SetWindowLongW(window, GWL_EXSTYLE, exStyle & ~WS_EX_TOPMOST);
        if (mode)
        {
            WindowedModeWrapper::bBorderlessWindowed = mode == 2;
            WindowedModeWrapper::bStretchWindow = mode == 2;
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

    inline void Toggle(SafetyHookContext&)
    {
        if (!InterlockedExchange(&toggleRequested, 0))
            return;
        auto window = WindowedModeWrapper::GameHWND;
        if (!window || !*device || changing || GetForegroundWindow() != window)
            return;
        auto table = *reinterpret_cast<uintptr_t**>(*device);
        using TestCooperativeLevel = HRESULT(WINAPI*)(void*);
        if (reinterpret_cast<TestCooperativeLevel>(table[3])(*device) != D3D_OK)
            return;

        const auto saved = *presentParameters;
        const auto flags = *engineFlags;
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
        ProtectedGameRef<uint32_t>{ engineFlags } = nextMode ? flags & ~2u : flags | 2u;
        ApplyWindow(window, nextMode);
        StoreParameters(requested);
        if (resetDevice(1) >= 0)
        {
            if (previousMode)
                lastWindowMode = previousMode;
            if (flags & 0x100000u)
                restartRenderer(*engineFlags);
        }
        else
        {
            OutputDebugStringA("Mafia: window mode reset failed; restoring the previous mode.\n");
            nWindowMode = previousMode;
            ProtectedGameRef<uint32_t>{ engineFlags } = flags;
            SetWindowLongW(window, GWL_STYLE, style);
            SetWindowLongW(window, GWL_EXSTYLE, exStyle);
            SetWindowPos(window, (exStyle & WS_EX_TOPMOST) ? HWND_TOPMOST : HWND_NOTOPMOST,
                previousRect.left, previousRect.top, previousRect.right - previousRect.left,
                previousRect.bottom - previousRect.top, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
            WindowedModeWrapper::bBorderlessWindowed = previousMode == 2;
            WindowedModeWrapper::bStretchWindow = previousMode == 2;
            StoreParameters(saved);
            if (resetDevice(1) >= 0)
            {
                if (flags & 0x100000u)
                    restartRenderer(flags);
            }
            else
                OutputDebugStringA("Mafia: previous mode reset failed; native device recovery remains active.\n");
        }
        changing = false;
    }

    inline BOOL WINAPI ClipCursorHook(const RECT* rect)
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

    inline void Frame(SafetyHookContext& regs)
    {
        Toggle(regs);
        CursorConfinement::Update(WindowedModeWrapper::GameHWND);
        CursorConfinement::Bind();
    }

    inline void Init(HMODULE engine, uint32_t* flags)
    {
        auto present = hook::module_pattern(engine, "83 EC 34 53 56 E8 ? ? ? ? A0 ? ? ? ? 84 C0 0F 84 ? ? ? ? FF 05");
        auto reset = hook::module_pattern(engine, "A1 ? ? ? ? 83 EC 3C 85 C0 53 56 8A D9 74 ? 8B 0D");
        auto parameters = hook::module_pattern(engine, "A1 ? ? ? ? 8B 08 68 ? ? ? ? 50 FF 51 38 85 C0 7D");
        auto restart = hook::module_pattern(engine, "B1 01 E8 ? ? ? ? 85 C0 7D ? BE 68 08 76 88 EB ? 8B 0D ? ? ? ? F7 C1 00 00 10 00 74 ? E8 ? ? ? ? 68");
        if (present.size() != 1 || reset.size() != 1 || parameters.size() != 1 || restart.size() != 1)
        {
            OutputDebugStringA("Mafia: window mode reset patterns unavailable.\n");
            return;
        }
        DEVMODEW desktop{};
        desktop.dmSize = sizeof(desktop);
        if (EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &desktop) && desktop.dmBitsPerPel == 16)
            windowFormat = 23;
        fullscreenInterval = (*flags & 0x100u) ? 1u : 0x80000000u;
        engineFlags = flags;
        device = *parameters.get_first<void**>(1);
        presentParameters = *parameters.get_first<PresentParameters*>(8);
        resetDevice = reinterpret_cast<decltype(resetDevice)>(reset.get_first());
        restartRenderer = reinterpret_cast<decltype(restartRenderer)>(injector::GetBranchDestination(restart.get_first(32)).as_int());
        lastWindowMode = nWindowMode ? nWindowMode : 1;
        static auto presentHook = safetyhook::create_mid(present.get_first(), Frame);
        enabled = static_cast<bool>(presentHook);
        if (enabled)
            IATHook::Replace(engine, "USER32.dll", std::forward_as_tuple("ClipCursor", ClipCursorHook));
    }
}
