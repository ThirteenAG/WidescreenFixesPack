module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "MouseInput.hpp"
#include "Shared/D3D9Options.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include <charconv>

export module Window;

import ComVars;
import Hud;
import DeveloperOverlay;
import Updates;

// Window mode through the engine's own "fullscreen" configuration value: the fix answers its lookup (present, false)
// for a window; the window's frame and size come from the shared window wrapper, borderless covers the monitor
int nWindowMode = 0;
SafetyHookInline HasConfigHook, ConfigBoolHook, CreateGameWindowHook;

bool IsFullscreenKey(const char* key)
{
    return nWindowMode && key && !std::strcmp(key, "fullscreen");
}

bool __fastcall HasConfig(void* config, void*, const char* key)
{
    return IsFullscreenKey(key) || HasConfigHook.unsafe_thiscall<bool>(config, key);
}

bool __fastcall ConfigBool(void* config, void*, const char* key)
{
    return IsFullscreenKey(key) ? false : ConfigBoolHook.unsafe_thiscall<bool>(config, key);
}

void SubclassGameWindow(HWND window);

HWND __fastcall CreateGameWindow(uintptr_t self, void*, int popup)
{
    const auto window = CreateGameWindowHook.unsafe_thiscall<HWND>(self, popup);
    if (window)
        SubclassGameWindow(window);
    if (nWindowMode && window)
    {
        WindowedModeWrapper::GameHWND = window;
        WindowedModeWrapper::bBorderlessWindowed = nWindowMode == 2;
        WindowedModeWrapper::desiredClientWidth = Screen.Width;
        WindowedModeWrapper::desiredClientHeight = Screen.Height;
        WindowedModeWrapper::afterCreateWindow();
        if (nWindowMode == 2)
        {
            const auto info = WindowedModeWrapper::WindowMonitor();
            SetWindowPos(window, nullptr, info.rcMonitor.left, info.rcMonitor.top, info.rcMonitor.right - info.rcMonitor.left,
                info.rcMonitor.bottom - info.rcMonitor.top, SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    }
    return window;
}

export void InstallWindowMode(int mode)
{
    nWindowMode = mode;
    auto window = hook::pattern("8B 44 24 04 56 8B F1 8A 4E 28 80 E1 01 F6 D9 1B C9 81 E1 00 00 05 00 81 C1 00 00 C2 00");
    CreateGameWindowHook = safetyhook::create_inline(window.get_first(), CreateGameWindow); //0x43C2C0
    if (!nWindowMode)
        return;
    auto has = hook::pattern("8B 44 24 04 83 EC 10 53 56 8B F1 50 8D 4C 24 0C E8 ? ? ? ? 8B 56 08 8D 4C 24 08 51 52 8B CE");
    auto boolean = hook::pattern("83 EC 24 53 56 57 8B 7C 24 34 8B F1 57 8D 4C 24 14 C7 44 24 10 00 00 00 00 E8 ? ? ? ? 8B 4E");
    HasConfigHook = safetyhook::create_inline(has.get_first(), HasConfig); //0x410080
    ConfigBoolHook = safetyhook::create_inline(boolean.get_first(), ConfigBool); //0x410900
}

std::atomic<HWND> hGameWindow{};
std::atomic_bool frameLimitChanged{}, pauseOnFocusLoss{ true };
SafetyHookMid WindowActivity;
bool followMonitorRate{};
ULONGLONG nextRefreshCheck{};

LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR)
{
    if (message == WM_NCDESTROY)
        RemoveWindowSubclass(window, WindowProcedure, id);
    if (window == hGameWindow.load())
    {
        const bool inactive = (message == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE) || (message == WM_ACTIVATEAPP && !wParam);
        if (inactive || message == WM_KILLFOCUS)
        {
            OverdoseMouse::Suspend(true);
            CursorConfinement::Release();
        }
        else if (message == WM_SETFOCUS)
            OverdoseMouse::Suspend(OverdoseOverlay::BlocksInput());
        if (message == WM_DESTROY)
            RWGTA::UpdateCheck::OnExit();
        if (message == WM_WINDOWPOSCHANGED || message == WM_DISPLAYCHANGE)
            frameLimitChanged.store(true, std::memory_order_relaxed);
        if (message == WM_NCDESTROY)
        {
            hGameWindow = nullptr;
            OverdoseMouse::SetWindow(nullptr);
        }
#ifdef DEBUG
        LRESULT result = 0;
        if (OverdoseOverlay::Message(window, message, wParam, lParam, result))
            return result;
#endif
    }
    return DefSubclassProc(window, message, wParam, lParam);
}

void SubclassGameWindow(HWND window)
{
    hGameWindow = window;
    OverdoseMouse::SetWindow(window);
    SetWindowSubclass(window, WindowProcedure, 0, 0);
}

void SetFrameLimit()
{
    const auto text = FixSettings().Get("MAIN.FPSLimit");
    int limit = 60;
    std::from_chars(text.data(), text.data() + text.size(), limit);
    followMonitorRate = limit < 0;
    nextRefreshCheck = 0;
    if (!followMonitorRate)
        D3D9Options::frameLimit = limit > 0 ? std::clamp(limit, 30, 1000) : 0;
}

void UpdateFrameLimit()
{
    if (frameLimitChanged.exchange(false, std::memory_order_relaxed))
        SetFrameLimit();
    if (!followMonitorRate)
        return;
    const auto now = GetTickCount64();
    if (now < nextRefreshCheck)
        return;

    // Refresh-only display mode changes may leave the window's placement and resolution unchanged.
    nextRefreshCheck = now + 1000;
    MONITORINFOEXW info{};
    info.cbSize = sizeof(info);
    DEVMODEW mode{ .dmSize = sizeof(mode) };
    const auto monitor = MonitorFromWindow(hGameWindow.load(), MONITOR_DEFAULTTOPRIMARY);
    D3D9Options::frameLimit = GetMonitorInfoW(monitor, &info) &&
        EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1 ?
        static_cast<int>(mode.dmDisplayFrequency) : static_cast<int>(GetDesktopRefreshRate());
}

export void InstallFrame()
{
    OverdoseMouse::Install(OverdoseOverlay::BlocksInput);
    auto activity = hook::pattern("38 5E 2C 74 2C 84 DB 88 5E 2C 8B 0D ? ? ? ? 74 0D 6A 00");
    WindowActivity = safetyhook::create_mid(activity.get_first(), [](SafetyHookContext& regs)
    {
        // Retain the engine's own pause/resume and audio handling, also for a window that stays visible.
        const auto window = hGameWindow.load();
        if (nWindowMode && window && pauseOnFocusLoss.load(std::memory_order_relaxed) && GetForegroundWindow() != window)
            regs.ebx &= ~0xffu;
    });
    const auto updatePause = []()
    {
        static std::mutex mutex;
        const std::lock_guard lock(mutex);
        const bool pause = FixSettings().Get("MAIN.PauseOnFocusLoss") != "0";
        const bool changed = pauseOnFocusLoss.exchange(pause, std::memory_order_relaxed) != pause;
        if (const auto window = hGameWindow.load(); changed && window)
            PostMessageW(window, WM_NULL, 0, 0);
    };
    FixSettings().OnChange("MAIN.PauseOnFocusLoss", updatePause);
    updatePause();
    FixSettings().OnChange("MAIN.FPSLimit", []()
    {
        frameLimitChanged.store(true, std::memory_order_relaxed);
    });
    SetFrameLimit();
    const auto updateCursor = []()
    {
        static std::mutex mutex;
        const std::lock_guard lock(mutex);
        CursorConfinement::SetEnabled(FixSettings().Get("MAIN.ConfineCursor") != "0");
    };
    FixSettings().OnChange("MAIN.ConfineCursor", updateCursor);
    updateCursor();
    D3D9Options::frame = [](IDirect3DDevice9* device)
    {
        static std::once_flag once;
        std::call_once(once, []()
        {
            WFP::onGameInitEvent().executeAll();
        });
        WFP::onGameProcessEvent().executeAll();
        Hud::AdvanceFrame();
        UpdateFrameLimit();
        const auto window = hGameWindow.load();
#ifdef DEBUG
        CursorConfinement::Suspend(OverdoseOverlay::BlocksInput());
#endif
        CursorConfinement::Frame(window);
        Updates::Tick();
#ifdef DEBUG
        OverdoseOverlay::Present(device, window);
#endif
    };
#ifdef DEBUG
    D3D9Options::beforeReset = OverdoseOverlay::BeforeReset;
#endif
    D3D9Options::HookCreation();
}

