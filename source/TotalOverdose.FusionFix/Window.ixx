module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/D3D9Options.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include <atomic>
#include <limits>

export module Window;

import ComVars;
import Hud;
import DeveloperOverlay;

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

export void InstallWindowMode()
{
    nWindowMode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
    auto window = hook::pattern("8B 44 24 04 56 8B F1 8A 4E 28 80 E1 01 F6 D9 1B C9 81 E1 00 00 05 00 81 C1 00 00 C2 00");
    CreateGameWindowHook = safetyhook::create_inline(window.get_first(), CreateGameWindow); //0x43C2C0
    if (!nWindowMode)
        return;
    auto has = hook::pattern("8B 44 24 04 83 EC 10 53 56 8B F1 50 8D 4C 24 0C E8 ? ? ? ? 8B 56 08 8D 4C 24 08 51 52 8B CE");
    auto boolean = hook::pattern("83 EC 24 53 56 57 8B 7C 24 34 8B F1 57 8D 4C 24 14 C7 44 24 10 00 00 00 00 E8 ? ? ? ? 8B 4E");
    HasConfigHook = safetyhook::create_inline(has.get_first(), HasConfig); //0x410080
    ConfigBoolHook = safetyhook::create_inline(boolean.get_first(), ConfigBool); //0x410900
}

// Focus: with MAIN.PauseOnFocusLoss off, a window mode keeps the game running in the background (the game's
// deactivation clears its active flag, which pauses it). The overlay gets the window's messages first.
HWND hGameWindow = nullptr;
WNDPROC GameWindowProcedure = nullptr;
SafetyHookMid RawMouseSample;
std::atomic<bool> RawMouseEnabled{ true }, RawMouseSeen{ false };
std::atomic<int64_t> RawMouseX{ 0 }, RawMouseY{ 0 };
std::atomic<HWND> RawMouseWindow{ nullptr };

void ClearRawMouse()
{
    RawMouseX.exchange(0, std::memory_order_relaxed);
    RawMouseY.exchange(0, std::memory_order_relaxed);
    RawMouseSeen.store(false, std::memory_order_release);
}

void RawMouseMessage(HWND window, UINT message, LPARAM lParam)
{
    if (message == WM_KILLFOCUS || message == WM_NCDESTROY)
    {
        ClearRawMouse();
        if (message == WM_NCDESTROY && window == RawMouseWindow.load(std::memory_order_relaxed))
            RawMouseWindow.store(nullptr, std::memory_order_relaxed);
    }
    if (message != WM_INPUT || window != RawMouseWindow.load(std::memory_order_relaxed) ||
        GetForegroundWindow() != window || !RawMouseEnabled.load(std::memory_order_relaxed) || OverdoseOverlay::BlocksInput())
        return;
    RAWINPUT input{};
    UINT size = sizeof(input);
    if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER)) == UINT(-1) ||
        input.header.dwType != RIM_TYPEMOUSE || (input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE))
        return;
    RawMouseX.fetch_add(input.data.mouse.lLastX, std::memory_order_relaxed);
    RawMouseY.fetch_add(input.data.mouse.lLastY, std::memory_order_relaxed);
    if (input.data.mouse.lLastX || input.data.mouse.lLastY)
        RawMouseSeen.store(true, std::memory_order_release);
}

LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    RawMouseMessage(window, message, lParam);
    LRESULT result = 0;
    if (OverdoseOverlay::Message(window, message, wParam, lParam, result))
        return result;
    if (message == WM_DESTROY)
        RWGTA::UpdateCheck::OnExit();
    const bool inactive = (message == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE) || (message == WM_ACTIVATEAPP && !wParam);
    if (inactive && nWindowMode && !FixSettings().Bool("MAIN.PauseOnFocusLoss"))
    {
        CursorConfinement::Release();
        return 0;
    }
    return CallWindowProcA(GameWindowProcedure, window, message, wParam, lParam);
}

void AttachRawMouse(HWND window)
{
    if (!window || !RawMouseEnabled.load(std::memory_order_relaxed))
    {
        RawMouseWindow.store(nullptr, std::memory_order_relaxed);
        ClearRawMouse();
        return;
    }
    static ULONGLONG nextCheck = 0;
    const auto now = GetTickCount64();
    if (now < nextCheck)
        return;
    nextCheck = now + 250;
    UINT count = 0;
    if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
    {
        RawMouseWindow.store(nullptr, std::memory_order_relaxed);
        ClearRawMouse();
        return;
    }
    std::vector<RAWINPUTDEVICE> devices(count);
    if (count && GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
    {
        RawMouseWindow.store(nullptr, std::memory_order_relaxed);
        ClearRawMouse();
        return;
    }
    for (UINT i = 0; i < count; ++i)
    {
        if (devices[i].usUsagePage == 1 && devices[i].usUsage == 2)
        {
            const auto target = devices[i].hwndTarget ? devices[i].hwndTarget : window;
            if (target != window)
            {
                RawMouseWindow.store(nullptr, std::memory_order_relaxed);
                ClearRawMouse();
            }
            else if (RawMouseWindow.exchange(window, std::memory_order_relaxed) != window)
                ClearRawMouse();
            return;
        }
    }
    RAWINPUTDEVICE device{ 1, 2, 0, window };
    RawMouseWindow.store(RegisterRawInputDevices(&device, 1, sizeof(device)) ? window : nullptr, std::memory_order_relaxed);
    ClearRawMouse();
}

void SubclassGameWindow(HWND window)
{
    hGameWindow = window;
    if (!GameWindowProcedure)
        GameWindowProcedure = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProcedure)));
}

void UpdateFrameLimit()
{
    // -1 follows the monitor
    const int limit = FixSettings().Int("MAIN.FPSLimit");
    D3D9Options::frameLimit = limit < 0 ? int(GetDesktopRefreshRate()) : limit;
}

export void InstallFrame()
{
    RawMouseEnabled.store(FixSettings().Bool("MAIN.RawInputMouse"), std::memory_order_relaxed);
    FixSettings().OnChange("MAIN.RawInputMouse", []()
    {
        RawMouseEnabled.store(FixSettings().Bool("MAIN.RawInputMouse"), std::memory_order_relaxed);
        ClearRawMouse();
    });
    auto mouse = hook::pattern("8B 4C 24 18 8B 54 24 1C 53 89 4E 3C 55 89 56 40");
    RawMouseSample = safetyhook::create_mid(mouse.get_first(), [](SafetyHookContext& regs)
    {
        const auto window = RawMouseWindow.load(std::memory_order_relaxed);
        if (!RawMouseEnabled.load(std::memory_order_relaxed) || !window || GetForegroundWindow() != window || OverdoseOverlay::BlocksInput())
        {
            ClearRawMouse();
            return;
        }
        if (!RawMouseSeen.load(std::memory_order_acquire))
            return;
        auto* axes = reinterpret_cast<int32_t*>(regs.esp + 0x18);
        constexpr auto low = static_cast<int64_t>(std::numeric_limits<int32_t>::min());
        constexpr auto high = static_cast<int64_t>(std::numeric_limits<int32_t>::max());
        axes[0] = static_cast<int32_t>(std::clamp(RawMouseX.exchange(0, std::memory_order_relaxed), low, high));
        axes[1] = static_cast<int32_t>(std::clamp(RawMouseY.exchange(0, std::memory_order_relaxed), low, high));
    });
    if (!RawMouseSample)
        std::terminate();
    UpdateFrameLimit();
    FixSettings().OnChange("MAIN.FPSLimit", UpdateFrameLimit);
    CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
    FixSettings().OnChange("MAIN.ConfineCursor", []()
    {
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
    });
    D3D9Options::frame = [](IDirect3DDevice9* device)
    {
        static std::once_flag once;
        std::call_once(once, []()
        {
            WFP::onGameInitEvent().executeAll();
        });
        WFP::onGameProcessEvent().executeAll();
        ++Hud::frame;
        AttachRawMouse(hGameWindow);
        CursorConfinement::Suspend(OverdoseOverlay::BlocksInput());
        CursorConfinement::Frame(hGameWindow);
        RWGTA::UpdateCheck::Tick();
        OverdoseOverlay::Present(device, hGameWindow);
    };
    D3D9Options::beforeReset = OverdoseOverlay::BeforeReset;
    D3D9Options::HookCreation();
}

