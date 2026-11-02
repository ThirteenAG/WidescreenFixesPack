module;

#include <stdafx.h>
#include <dwmapi.h>
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"
#include <atomic>
#include <limits>

export module Windowed;

import Skeleton;

namespace
{
    int32_t nWindowMode = 0;
    bool bFirstStart = false;
    std::atomic<bool> pauseOnFocusLoss{ true };
    UINT focusMessage = 0;
    std::atomic<HWND> gameWindow{ nullptr };
    SafetyHookInline windowProc;
    SafetyHookMid rawMouseSample;
    std::atomic<bool> rawEnabled{ true }, rawSeen{ false };
    std::atomic<int64_t> rawX{ 0 }, rawY{ 0 };
    std::atomic<HWND> rawWindow{ nullptr };

    void ClearRawMouse()
    {
        rawX.exchange(0, std::memory_order_relaxed);
        rawY.exchange(0, std::memory_order_relaxed);
        rawSeen.store(false, std::memory_order_release);
    }

    void AttachRawMouse(HWND window)
    {
        if (!window || !rawEnabled.load(std::memory_order_relaxed))
        {
            rawWindow.store(nullptr, std::memory_order_relaxed);
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
            rawWindow.store(nullptr, std::memory_order_relaxed);
            ClearRawMouse();
            return;
        }
        std::vector<RAWINPUTDEVICE> devices(count);
        if (count && GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
        {
            rawWindow.store(nullptr, std::memory_order_relaxed);
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
                    rawWindow.store(nullptr, std::memory_order_relaxed);
                    ClearRawMouse();
                }
                else if (rawWindow.exchange(window, std::memory_order_relaxed) != window)
                    ClearRawMouse();
                return;
            }
        }
        RAWINPUTDEVICE device{ 1, 2, 0, window };
        rawWindow.store(RegisterRawInputDevices(&device, 1, sizeof(device)) ? window : nullptr, std::memory_order_relaxed);
        ClearRawMouse();
    }

    void RawMouseMessage(HWND window, UINT message, LPARAM lParam)
    {
        if (message == WM_KILLFOCUS || message == WM_NCDESTROY)
        {
            ClearRawMouse();
            if (message == WM_NCDESTROY && window == rawWindow.load(std::memory_order_relaxed))
                rawWindow.store(nullptr, std::memory_order_relaxed);
        }
        if (message != WM_INPUT || window != rawWindow.load(std::memory_order_relaxed) ||
            GetForegroundWindow() != window || !rawEnabled.load(std::memory_order_relaxed))
            return;
        RAWINPUT input{};
        UINT size = sizeof(input);
        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER)) == UINT(-1) ||
            input.header.dwType != RIM_TYPEMOUSE || (input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE))
            return;
        rawX.fetch_add(input.data.mouse.lLastX, std::memory_order_relaxed);
        rawY.fetch_add(input.data.mouse.lLastY, std::memory_order_relaxed);
        if (input.data.mouse.lLastX || input.data.mouse.lLastY)
            rawSeen.store(true, std::memory_order_release);
    }

    void UpdateFocus(HWND window)
    {
        if (nWindowMode && GetForegroundWindow() != window)
        {
            injector::WriteMemory<uint8_t>(focused.get_ptr(), pauseOnFocusLoss ? 0 : 1, true);
            if (!pauseOnFocusLoss)
                injector::WriteMemory<int32_t>(active.get_ptr(), 1, true);
        }
    }

    LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        gameWindow = window;
        RawMouseMessage(window, message, lParam);
        if (focusMessage && message == focusMessage)
        {
            UpdateFocus(window);
            return 0;
        }
        // Keep the native device and input handling; windowed rendering can continue in the background.
        if (nWindowMode && !pauseOnFocusLoss && message == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE)
            wParam = WA_ACTIVE;
        const auto result = windowProc.unsafe_stdcall<LRESULT>(window, message, wParam, lParam);
        if (message == WM_KILLFOCUS || message == WM_ACTIVATE || message == WM_ACTIVATEAPP)
            UpdateFocus(window);
        if (message == WM_NCDESTROY)
            gameWindow = nullptr;
        return result;
    }

    void InstallFocus()
    {
        auto procedure = hook::pattern("53 56 57 55 8B 44 24 18 83 EC 6C 83 E8 02");
        auto focus = hook::pattern("C6 05 ? ? ? ? 01 8B 1D ? ? ? ? 83 FB 04");
        auto activation = hook::pattern("C7 05 ? ? ? ? 00 00 00 00 8B 0D ? ? ? ? 83 79 1C 00");
        focused.SetAddress(*focus.get_first<uint8_t*>(2));
        active.SetAddress(*activation.get_first<int32_t*>(2));
        focusMessage = RegisterWindowMessageW(L"Manhunt.FusionFix.FocusSettings");
        pauseOnFocusLoss = FixSettings().Bool("MAIN.PauseOnFocusLoss");
        windowProc = safetyhook::create_inline(procedure.get_first(), WindowProc);
        FixSettings().Availability("MAIN.PauseOnFocusLoss", [](std::string_view value)
        {
            return value != "0" || nWindowMode != 0;
        });
        FixSettings().OnChange("MAIN.PauseOnFocusLoss", []()
        {
            pauseOnFocusLoss = FixSettings().Bool("MAIN.PauseOnFocusLoss");
            if (auto window = gameWindow.load(); window && focusMessage)
                PostMessageW(window, focusMessage, 0, 0);
        });
    }

    using GetNumVideoModesFn = int32_t(__cdecl*)();
    using GetVideoModeInfoFn = RwVideoMode*(__cdecl*)(RwVideoMode*, int32_t);
    GetNumVideoModesFn GetNumVideoModes = nullptr;
    GetVideoModeInfoFn GetVideoModeInfo = nullptr;

    using Direct3DCreate8Fn = void*(WINAPI*)(UINT);
    Direct3DCreate8Fn Direct3DCreate8 = nullptr;
    decltype(&CreateWindowExA) PreviousCreateWindowExA = CreateWindowExA;

    void SetThreadDpiAwareness()
    {
        using SetThreadContextFn = HANDLE(WINAPI*)(HANDLE);
        if (auto user32 = GetModuleHandleW(L"user32.dll"))
        {
            if (auto setThreadContext = reinterpret_cast<SetThreadContextFn>(GetProcAddress(user32, "SetThreadDpiAwarenessContext")))
                setThreadContext(reinterpret_cast<HANDLE>(-4));
        }
    }

    HWND WINAPI CreateWindowExAWithDpi(DWORD exStyle, LPCSTR className, LPCSTR title, DWORD style,
        int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter)
    {
        SetThreadDpiAwareness();
        return PreviousCreateWindowExA(exStyle, className, title, style, x, y, width, height, parent, menu, instance, parameter);
    }

    void SetDpiAwareness()
    {
        using SetContextFn = BOOL(WINAPI*)(HANDLE);
        if (auto user32 = GetModuleHandleW(L"user32.dll"))
        {
            if (auto setContext = reinterpret_cast<SetContextFn>(GetProcAddress(user32, "SetProcessDpiAwarenessContext")))
            {
                if (setContext(reinterpret_cast<HANDLE>(-4)))
                    return;
            }
            SetThreadDpiAwareness();
        }
        SetProcessDPIAware();
    }

    void DontRoundCorners(HWND window)
    {
        // DWMWA_WINDOW_CORNER_PREFERENCE, DWMWCP_DONOTROUND (Windows 11)
        using DwmSetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        static auto dwmapi = LoadLibraryW(L"dwmapi.dll");
        if (!dwmapi)
            return;
        if (auto setAttribute = reinterpret_cast<DwmSetWindowAttributeFn>(GetProcAddress(dwmapi, "DwmSetWindowAttribute")))
        {
            const DWORD preference = 1;
            setAttribute(window, 33, &preference, sizeof(preference));
        }
    }

    bool HasSavedVideoMode()
    {
        HKEY key = nullptr;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Rockstar Games\\Manhunt\\Video", 0, KEY_READ, &key) != ERROR_SUCCESS)
            return false;
        DWORD size = sizeof(DWORD);
        DWORD value = 0;
        const bool saved = RegQueryValueExA(key, "Mode", nullptr, nullptr, reinterpret_cast<BYTE*>(&value), &size) == ERROR_SUCCESS;
        RegCloseKey(key);
        return saved;
    }

    // The refresh rate for a fullscreen mode: the highest the adapter offers at that size (the game took the lowest
    // from 60 Hz up). -1 when the mode isn't found, 0 for the adapter's default.
    int32_t __cdecl FindRefreshRate(int32_t width, int32_t height, int32_t depth)
    {
        // D3DFMT_X8R8G8B8, D3DFMT_R8G8B8, D3DFMT_R5G6B5
        const uint32_t format = depth == 32 ? 22 : depth == 24 ? 20 : 23;

        struct D3DDISPLAYMODE { uint32_t Width, Height, RefreshRate, Format; };
        using GetAdapterModeCountFn = uint32_t(__stdcall*)(void*, uint32_t);
        using EnumAdapterModesFn = int32_t(__stdcall*)(void*, uint32_t, uint32_t, D3DDISPLAYMODE*);
        using ReleaseFn = uint32_t(__stdcall*)(void*);

        auto d3d = Direct3DCreate8(220);
        if (!d3d)
            return -1;
        auto vtable = *reinterpret_cast<void***>(d3d);
        const uint32_t adapter = adapterIndex.get();

        int32_t best = -1;
        const uint32_t count = reinterpret_cast<GetAdapterModeCountFn>(vtable[6])(d3d, adapter);
        for (uint32_t i = 0; i < count; i++)
        {
            D3DDISPLAYMODE mode = {};
            if (reinterpret_cast<EnumAdapterModesFn>(vtable[7])(d3d, adapter, i, &mode) < 0)
                continue;
            if (mode.Width != uint32_t(width) || mode.Height != uint32_t(height) || mode.Format != format)
                continue;
            if (mode.RefreshRate == 0)
            {
                best = 0;
                break;
            }
            best = std::max(best, int32_t(mode.RefreshRate));
        }
        reinterpret_cast<ReleaseFn>(vtable[2])(d3d);
        return best;
    }

    // Places the game's window for the window modes. The size is that of the chosen video mode; borderless covers the
    // monitor. A window as big as the monitor puts its frame outside the screen.
    void PlaceWindow(HWND window, int32_t& width, int32_t& height)
    {
        HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO info = { sizeof(info) };
        GetMonitorInfoW(monitor, &info);
        const RECT& screen = info.rcMonitor;
        const int32_t screenWidth = screen.right - screen.left;
        const int32_t screenHeight = screen.bottom - screen.top;

        DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        if (nWindowMode == 2)
        {
            style = WS_POPUP;
            width = screenWidth;
            height = screenHeight;
        }
        const DWORD visible = GetWindowLongW(window, GWL_STYLE) & WS_VISIBLE;
        SetWindowLongW(window, GWL_STYLE, style | visible);
        const DWORD exStyle = GetWindowLongW(window, GWL_EXSTYLE) & ~WS_EX_TOPMOST;
        SetWindowLongW(window, GWL_EXSTYLE, exStyle);

        RECT rect = { 0, 0, width, height };
        AdjustWindowRectEx(&rect, style, FALSE, exStyle);
        int32_t x, y;
        if (width >= screenWidth && height >= screenHeight)
        {
            // The client area covers the monitor, the frame lies outside it
            x = screen.left + rect.left;
            y = screen.top + rect.top;
        }
        else
        {
            const RECT& area = info.rcWork;
            x = area.left + ((area.right - area.left) - (rect.right - rect.left)) / 2;
            y = area.top + ((area.bottom - area.top) - (rect.bottom - rect.top)) / 2;
            y = std::max<int32_t>(y, area.top);
        }
        SetWindowPos(window, HWND_NOTOPMOST, x, y, rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE | SWP_FRAMECHANGED);
        DontRoundCorners(window);
    }
}

class Windowed
{
public:
    Windowed()
    {
        WFP::onInitEvent() += []()
        {
            SetDpiAwareness();
            auto imports = IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll",
                std::forward_as_tuple("CreateWindowExA", CreateWindowExAWithDpi));
            PreviousCreateWindowExA = reinterpret_cast<decltype(PreviousCreateWindowExA)>(imports.at("CreateWindowExA").get());
            nWindowMode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
            InstallFocus();
            rawEnabled.store(FixSettings().Bool("MAIN.RawInputMouse"), std::memory_order_relaxed);
            FixSettings().OnChange("MAIN.RawInputMouse", []()
            {
                rawEnabled.store(FixSettings().Bool("MAIN.RawInputMouse"), std::memory_order_relaxed);
                ClearRawMouse();
            });
            auto mouse = hook::pattern("DB 44 24 1C 0F B6 54 24 28 F7 C2 80 00 00 00");
            rawMouseSample = safetyhook::create_mid(mouse.get_first(), [](SafetyHookContext& regs)
            {
                // The native accumulator reads these locals before writing them.
                auto* motion = reinterpret_cast<uint32_t*>(regs.esp);
                motion[0] = 0;
                motion[1] = 0;
                const auto window = rawWindow.load(std::memory_order_relaxed);
                if (!rawEnabled.load(std::memory_order_relaxed) || !window || GetForegroundWindow() != window)
                {
                    ClearRawMouse();
                    return;
                }
                if (!rawSeen.load(std::memory_order_acquire))
                    return;
                auto* axes = reinterpret_cast<int32_t*>(regs.esp + 0x1C);
                constexpr auto low = static_cast<int64_t>(std::numeric_limits<int32_t>::min());
                constexpr auto high = static_cast<int64_t>(std::numeric_limits<int32_t>::max());
                axes[0] = static_cast<int32_t>(std::clamp(rawX.exchange(0, std::memory_order_relaxed), low, high));
                axes[1] = static_cast<int32_t>(std::clamp(rawY.exchange(0, std::memory_order_relaxed), low, high));
            });
            if (!rawMouseSample)
                std::terminate();
            bFirstStart = !HasSavedVideoMode();

            // Keeps the cursor inside the game's window while it's in front
            CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
            FixSettings().OnChange("MAIN.ConfineCursor", []()
            {
                CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
            });
            WFP::onGameProcessEvent() += []()
            {
                auto rs = RsGlobal.get_ptr();
                AttachRawMouse(rs->ps ? rs->ps->window : nullptr);
                CursorConfinement::Frame(rs->ps ? rs->ps->window : nullptr);
            };

            // The startup video mode dialog
            auto pattern = hook::pattern("53 56 57 55 83 C4 80 8B 9C 24 94 00 00 00 E8");
            GetNumVideoModes = reinterpret_cast<GetNumVideoModesFn>(injector::GetBranchDestination(pattern.get_first(14)).as_int());

            pattern = hook::pattern("FF 35 ? ? ? ? 8D 4C 24 04 51 E8 ? ? ? ? 59 59 C7 05");
            GetVideoModeInfo = reinterpret_cast<GetVideoModeInfoFn>(injector::GetBranchDestination(pattern.get_first(11)).as_int());

            // The first start selects the desktop resolution (the game would take the first mode of the list)
            auto dialog = hook::pattern("55 FF 15 ? ? ? ? 83 C4 60 5D 5F 5E 5B C3");
            static auto VideoModeDialog = safetyhook::create_mid(dialog.get_first(), [](SafetyHookContext& regs)
            {
                if (!bFirstStart)
                    return;
                auto list = reinterpret_cast<HWND>(regs.edi);
                auto [desktopWidth, desktopHeight] = GetDesktopRes();
                const auto count = SendMessageW(list, CB_GETCOUNT, 0, 0);
                for (LRESULT i = 0; i < count; i++)
                {
                    RwVideoMode mode = {};
                    const auto index = int32_t(SendMessageW(list, CB_GETITEMDATA, i, 0));
                    if (!GetVideoModeInfo(&mode, index))
                        continue;
                    if (mode.width == desktopWidth && mode.height == desktopHeight && mode.depth == 32)
                    {
                        SendMessageW(list, CB_SETCURSEL, i, 0);
                        break;
                    }
                }
            });

            // The refresh rate of the fullscreen mode
            auto refresh = hook::pattern("53 56 57 55 83 EC 18 8B 7C 24 34 68 DC 00 00 00 E8");
            auto refreshCall = hook::pattern("E8 ? ? ? ? 83 C4 0C 83 F8 FF 74 07 50 E8");
            Direct3DCreate8 = reinterpret_cast<Direct3DCreate8Fn>(injector::GetBranchDestination(refresh.get_first(16)).as_int());
            auto adapter = hook::pattern(reinterpret_cast<uintptr_t>(refresh.get_first(16)), reinterpret_cast<uintptr_t>(refresh.get_first(0x60)), "FF 35 ? ? ? ? 56");
            adapterIndex.SetAddress(*adapter.get_first<uint32_t*>(2));
            injector::MakeCALL(refreshCall.get_first(), FindRefreshRate, true);

            // psSelectDevice, after the chosen mode was read: windowed modes use the engine's windowed video mode at the
            // chosen size
            static const auto videoModeBound = [&]()
            {
                VideoModeIndex.SetAddress(*pattern.get_first<int32_t*>(2));
                return true;
            }();
            static auto SelectDevice = safetyhook::create_mid(pattern.get_first(18), [](SafetyHookContext& regs)
            {
                auto rs = RsGlobal.get_ptr();
                if (!rs->ps || !rs->ps->window)
                    return;
                DontRoundCorners(rs->ps->window);

                auto& mode = *reinterpret_cast<RwVideoMode*>(regs.esp);
                if (!nWindowMode || !(mode.flags & RwEngine::rwVIDEOMODEEXCLUSIVE))
                    return;

                const int32_t count = GetNumVideoModes();
                for (int32_t i = 0; i < count; i++)
                {
                    RwVideoMode windowed = {};
                    if (!GetVideoModeInfo(&windowed, i) || (windowed.flags & RwEngine::rwVIDEOMODEEXCLUSIVE))
                        continue;

                    int32_t width = mode.width;
                    int32_t height = mode.height;
                    PlaceWindow(rs->ps->window, width, height);

                    VideoModeIndex = i;
                    mode.flags &= ~RwEngine::rwVIDEOMODEEXCLUSIVE;  // the game keeps its window as it is
                    rs->width = width;
                    rs->height = height;
                    rs->maximumWidth = width;
                    rs->maximumHeight = height;
                    return;
                }
                std::terminate();
            });
            if (!SelectDevice || !VideoModeDialog || !windowProc)
                std::terminate();
        };
        WFP::onShutdownEvent() += []()
        {
            CursorConfinement::Shutdown();
        };
    }
} Windowed;
