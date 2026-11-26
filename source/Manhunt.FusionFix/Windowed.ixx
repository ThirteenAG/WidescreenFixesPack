module;

#include <stdafx.h>
#include <dwmapi.h>
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"
#include "rawinput.hpp"
#include <atomic>
#include <limits>
#include <mutex>
#include <optional>

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
    std::atomic<bool> rawEnabled{ true };
    std::atomic<HWND> rawWindow{ nullptr };
    std::atomic_uint64_t rawGeneration{};
    std::mutex motionMutex, registrationMutex;
    int64_t rawX = 0, rawY = 0;
    bool rawSeen = false;
    HWND rawGame = nullptr, rawReader = nullptr;
    std::optional<RAWINPUTDEVICE> originalMouse;

    void ClearRawMotion()
    {
        ++rawGeneration;
        rawX = rawY = 0;
        rawSeen = false;
    }

    void ClearRawMouse()
    {
        std::lock_guard lock(motionMutex);
        ClearRawMotion();
    }

    void SetRawEnabled(bool enabled)
    {
        std::lock_guard lock(motionMutex);
        if (rawEnabled.exchange(enabled) != enabled)
            ClearRawMotion();
    }

    void SetRawWindow(HWND window)
    {
        std::lock_guard lock(motionMutex);
        if (rawWindow.exchange(window) != window)
            ClearRawMotion();
    }

    void ReadRawMouse(LPARAM lParam)
    {
        const auto generation = rawGeneration.load();
        const auto window = rawWindow.load();
        if (!window || !rawEnabled || GetForegroundWindow() != window)
            return;

        int64_t x = 0, y = 0;
        bool absolute = false, moved = false;
        const auto add = [&](const RAWMOUSE& mouse)
        {
            if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
            {
                absolute = true;
                moved = false;
                x = y = 0;
            }
            else if (mouse.lLastX || mouse.lLastY)
            {
                x += mouse.lLastX;
                y += mouse.lLastY;
                moved = true;
            }
        };

        RAWINPUT first{};
        UINT size = sizeof(first);
        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &first, &size, sizeof(RAWINPUTHEADER)) != UINT(-1) &&
            first.header.dwType == RIM_TYPEMOUSE)
            add(first.data.mouse);

        // The window has already removed the dispatched report. Drain the rest of its queue in batches.
        static const bool wow64 = [] { BOOL value = FALSE; IsWow64Process(GetCurrentProcess(), &value); return value != FALSE; }();
        const auto alignment = wow64 ? uintptr_t{ 8 } : alignof(RAWINPUT);
        const auto mouseOffset = sizeof(RAWINPUTHEADER) + (wow64 ? 8 : 0);
        alignas(8) static unsigned char buffer[64 * 1024];
        for (;;)
        {
            size = sizeof(buffer);
            const auto count = GetRawInputBuffer(reinterpret_cast<RAWINPUT*>(buffer), &size, sizeof(RAWINPUTHEADER));
            if (!count || count == UINT(-1))
                break;
            auto next = reinterpret_cast<uintptr_t>(buffer);
            for (UINT i = 0; i < count; ++i)
            {
                const auto& header = *reinterpret_cast<const RAWINPUTHEADER*>(next);
                if (header.dwType == RIM_TYPEMOUSE)
                    add(*reinterpret_cast<const RAWMOUSE*>(next + mouseOffset));
                next = (next + header.dwSize + alignment - 1) & ~(alignment - 1);
            }
        }

        std::lock_guard lock(motionMutex);
        if (generation != rawGeneration.load() || rawWindow != window || !rawEnabled || GetForegroundWindow() != window)
            return;
        if (absolute)
        {
            rawX = rawY = 0;
            rawSeen = false;
        }
        rawX += x;
        rawY += y;
        rawSeen |= moved;
    }

    void SampleRawMouse(int32_t* axes)
    {
        if (!rawEnabled)
            return;
        std::lock_guard lock(motionMutex);
        const auto window = rawWindow.load();
        if (!rawEnabled || !window || GetForegroundWindow() != window)
        {
            ClearRawMotion();
            return;
        }
        if (!rawSeen)
            return;
        constexpr auto low = static_cast<int64_t>(std::numeric_limits<int32_t>::min());
        constexpr auto high = static_cast<int64_t>(std::numeric_limits<int32_t>::max());
        axes[0] = static_cast<int32_t>(std::clamp(std::exchange(rawX, int64_t{ 0 }), low, high));
        axes[1] = static_cast<int32_t>(std::clamp(std::exchange(rawY, int64_t{ 0 }), low, high));
    }

    enum class Registration { Error, Empty, Mouse };

    Registration MouseRegistration(RAWINPUTDEVICE& mouse)
    {
        static std::vector<RAWINPUTDEVICE> devices(8);
        UINT count = UINT(devices.size());
        UINT found = GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE));
        if (found == UINT(-1) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            devices.resize(count);
            found = GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE));
        }
        if (found == UINT(-1))
            return Registration::Error;
        for (UINT i = 0; i < found; ++i)
        {
            if (devices[i].usUsagePage == 1 && devices[i].usUsage == 2)
            {
                mouse = devices[i];
                return Registration::Mouse;
            }
        }
        return Registration::Empty;
    }

    bool AddedNoLegacy(const RAWINPUTDEVICE& mouse)
    {
        return originalMouse && mouse.hwndTarget == originalMouse->hwndTarget &&
            mouse.dwFlags == (originalMouse->dwFlags | RIDEV_NOLEGACY);
    }

    void SetLegacy(RAWINPUTDEVICE device, bool legacy)
    {
        if (originalMouse && !AddedNoLegacy(device))
            originalMouse.reset();
        if (legacy)
        {
            if (originalMouse && RegisterRawInputDevices(&*originalMouse, 1, sizeof(device)))
                originalMouse.reset();
        }
        else if ((device.dwFlags & RIDEV_NOLEGACY) != RIDEV_NOLEGACY)
        {
            const auto original = device;
            device.dwFlags |= RIDEV_NOLEGACY;
            if (RegisterRawInputDevices(&device, 1, sizeof(device)))
                originalMouse = original;
        }
    }

    void RestoreLegacyMouse()
    {
        if (!originalMouse)
            return;
        RAWINPUTDEVICE mouse{};
        switch (MouseRegistration(mouse))
        {
        case Registration::Mouse:
            SetLegacy(mouse, true);
            break;
        case Registration::Empty:
            originalMouse.reset();
            break;
        default:
            break;
        }
    }

    enum class ReaderRelease { Keep, Retry, Destroy };
    constexpr UINT ReleaseReaderMessage = WM_APP + 0x421;
    constexpr UINT_PTR ReleaseReaderTimer = 1;

    ReaderRelease ReleaseRawReader(HWND reader)
    {
        std::lock_guard lock(registrationMutex);
        if (rawGame || rawReader != reader)
            return ReaderRelease::Keep;
        RAWINPUTDEVICE mouse{};
        const auto status = MouseRegistration(mouse);
        if (status == Registration::Error)
            return ReaderRelease::Retry;
        if (status == Registration::Mouse && mouse.hwndTarget == reader)
        {
            if (mouse.dwFlags != RIDEV_INPUTSINK && !AddedNoLegacy(mouse))
                return ReaderRelease::Keep;
            const RAWINPUTDEVICE remove{ 1, 2, RIDEV_REMOVE, nullptr };
            if (!RegisterRawInputDevices(&remove, 1, sizeof(remove)))
                return ReaderRelease::Retry;
        }
        if (originalMouse && originalMouse->hwndTarget == reader)
            originalMouse.reset();
        rawReader = nullptr;
        return ReaderRelease::Destroy;
    }

    LRESULT CALLBACK RawInputProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_INPUT)
            ReadRawMouse(lParam);
        else if (message == ReleaseReaderMessage || (message == WM_TIMER && wParam == ReleaseReaderTimer))
        {
            const auto release = ReleaseRawReader(window);
            if (release == ReaderRelease::Retry)
                SetTimer(window, ReleaseReaderTimer, 50, nullptr);
            else
                KillTimer(window, ReleaseReaderTimer);
            if (release == ReaderRelease::Destroy)
                DestroyWindow(window);
            return 0;
        }
        else if (message == WM_NCDESTROY)
            PostQuitMessage(0);
        return DefWindowProcW(window, message, wParam, lParam);
    }

    // The game reads the mouse only through DirectInput. Every report of the mouse is also turned into window messages
    // for its thread (WM_NCHITTEST and WM_SETCURSOR sent, its window procedure calls ShowCursor and SetCursor for each,
    // WM_MOUSEMOVE posted): with a high polling rate mouse that's a large part of each frame. While the window is in
    // front and the cursor is over its client area, the mouse registration of the process (DirectInput's or ours) gets
    // RIDEV_NOLEGACY; DirectInput still gets every report, the title bar and other windows get their messages.
    bool WantLegacy(HWND window)
    {
        if (GetForegroundWindow() != window || IsIconic(window))
            return true;
        GUITHREADINFO info{ sizeof(info) };
        if (!GetGUIThreadInfo(GetCurrentThreadId(), &info) || (info.flags & (GUI_INMENUMODE | GUI_INMOVESIZE | GUI_POPUPMENUMODE | GUI_SYSTEMMENUMODE)) ||
            (info.hwndCapture && info.hwndCapture != window))
            return true;
        POINT cursor{};
        RECT client{};
        return !GetCursorPos(&cursor) || !ScreenToClient(window, &cursor) || !GetClientRect(window, &client) || !PtInRect(&client, cursor);
    }

    void DetachRawMouse()
    {
        rawGame = nullptr;
        SetRawWindow(nullptr);
        RestoreLegacyMouse();
        if (rawReader)
            PostMessageW(rawReader, ReleaseReaderMessage, 0, 0);
    }

    void AttachRawMouse(HWND window)
    {
        std::unique_lock lock(registrationMutex);
        if (!window)
        {
            if (rawGame)
                DetachRawMouse();
            return;
        }
        rawGame = window;
        // DirectInput registers its worker again on acquire and can replace the fallback.
        RAWINPUTDEVICE mouse{};
        const auto status = MouseRegistration(mouse);
        if (status == Registration::Error)
        {
            SetRawWindow(nullptr);
            return;
        }
        if (status == Registration::Empty)
        {
            // Off: no reader of our own, the game's DirectInput mouse stays as it is
            if (!rawEnabled)
            {
                SetRawWindow(nullptr);
                return;
            }
            if (!rawReader)
                rawReader = CreateRawInputWindow(L"ManhuntRawMouse", RawInputProc);
            mouse = { 1, 2, RIDEV_INPUTSINK, rawReader };
            if (!rawReader || !RegisterRawInputDevices(&mouse, 1, sizeof(mouse)))
            {
                SetRawWindow(nullptr);
                return;
            }
            originalMouse.reset();
        }
        SetRawWindow(rawReader && mouse.hwndTarget == rawReader && rawEnabled ? window : nullptr);
        DWORD owner = 0;
        GetWindowThreadProcessId(mouse.hwndTarget ? mouse.hwndTarget : window, &owner);
        if (owner != GetCurrentProcessId())
            return;
        const bool legacy = WantLegacy(window);
        SetLegacy(mouse, legacy);
        lock.unlock();
        if (!legacy && GetCapture() == window)
            ReleaseCapture(); // taken on a button press, the release won't come as a message
    }

    void RawMouseMessage(HWND window, UINT message, WPARAM wParam)
    {
        const bool lost = message == WM_KILLFOCUS || (message == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE) ||
            (message == WM_ACTIVATEAPP && !wParam);
        if (message != WM_NCDESTROY && !lost && message != WM_ENTERMENULOOP && message != WM_ENTERSIZEMOVE)
            return;

        std::lock_guard lock(registrationMutex);
        if (window != rawGame)
            return;
        if (message == WM_NCDESTROY)
            DetachRawMouse();
        else
        {
            if (lost)
                ClearRawMouse();
            RestoreLegacyMouse();
        }
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
        RawMouseMessage(window, message, wParam);
        if (focusMessage && message == focusMessage)
        {
            UpdateFocus(window);
            return 0;
        }
        // Keep the native device and input handling; windowed rendering can continue in the background.
        if (nWindowMode && !pauseOnFocusLoss && message == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE)
            wParam = WA_ACTIVE;
        const auto result = windowProc.unsafe_stdcall<LRESULT>(window, message, wParam, lParam);
        // Windows caps a window at the monitor size plus its frame, which leaves the client area of a window as big
        // as the monitor short by the caption, smaller than the engine's cameras
        if (nWindowMode && message == WM_GETMINMAXINFO)
        {
            auto info = reinterpret_cast<MINMAXINFO*>(lParam);
            info->ptMaxTrackSize.x = std::max<LONG>(info->ptMaxTrackSize.x, SHRT_MAX);
            info->ptMaxTrackSize.y = std::max<LONG>(info->ptMaxTrackSize.y, SHRT_MAX);
        }
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
    decltype(&GetClientRect) PreviousGetClientRect = GetClientRect;

    // A minimised window has an empty client area, and the engine refuses to create camera rasters larger than it
    // (the frontend camera at startup when the game starts minimised). The restored size is the one to render at.
    BOOL WINAPI GetClientRectRestored(HWND window, LPRECT rect)
    {
        const BOOL result = PreviousGetClientRect(window, rect);
        if (!result || !IsIconic(window))
            return result;
        WINDOWPLACEMENT placement = { sizeof(placement) };
        if (!GetWindowPlacement(window, &placement))
            return TRUE;
        RECT frame = {};
        AdjustWindowRectEx(&frame, GetWindowLongW(window, GWL_STYLE), GetMenu(window) != nullptr, GetWindowLongW(window, GWL_EXSTYLE));
        const auto& normal = placement.rcNormalPosition;
        rect->left = 0;
        rect->top = 0;
        rect->right = std::max<LONG>(0, (normal.right - normal.left) - (frame.right - frame.left));
        rect->bottom = std::max<LONG>(0, (normal.bottom - normal.top) - (frame.bottom - frame.top));
        return TRUE;
    }

    GameRef<void*> sceneCamera;
    GameRef<void*> frontendCamera;

    // The engine sizes its back buffer to the window's client area, but its cameras and the screen size the 2D
    // drawing goes by keep the size the game started at (it ignores the skeleton's camera size event): after a resize
    // the menus, texts and HUD were laid out for the old size. Both follow the client area, as the skeleton does.
    void FollowClientSize(HWND window)
    {
        RECT client{};
        if (!window || IsIconic(window) || !PreviousGetClientRect(window, &client) || client.right <= 0 || client.bottom <= 0)
            return;
        auto rs = RsGlobal.get_ptr();
        if (rs->width == client.right && rs->height == client.bottom)
            return;
        rs->width = client.right;
        rs->height = client.bottom;
        for (auto camera : { sceneCamera.get(), frontendCamera.get() })
        {
            if (!camera)
                continue;
            // The colour and depth rasters, and the screen rasters they are part of
            for (auto offset : { 0x60, 0x64 })
            {
                auto raster = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(camera) + offset);
                for (auto part : { raster, raster ? *reinterpret_cast<uint8_t**>(raster) : nullptr })
                {
                    if (!part)
                        continue;
                    *reinterpret_cast<int32_t*>(part + 0x0C) = client.right;
                    *reinterpret_cast<int32_t*>(part + 0x10) = client.bottom;
                }
            }
        }
    }

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
            // The main loop empties the message queue before each frame; a high polling rate mouse must not keep it going
            MessagePumpGuard::Install(GetModuleHandle(nullptr));
            auto imports = IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll",
                std::forward_as_tuple("CreateWindowExA", CreateWindowExAWithDpi),
                std::forward_as_tuple("GetClientRect", GetClientRectRestored));
            PreviousCreateWindowExA = reinterpret_cast<decltype(PreviousCreateWindowExA)>(imports.at("CreateWindowExA").get());
            PreviousGetClientRect = reinterpret_cast<decltype(PreviousGetClientRect)>(imports.at("GetClientRect").get());
            nWindowMode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
            auto cameras = hook::pattern("8B 35 ? ? ? ? 85 F6 75 06 8B 35 ? ? ? ? 56 E8");
            sceneCamera.SetAddress(*cameras.get_first<void**>(2));
            frontendCamera.SetAddress(*cameras.get_first<void**>(12));
            InstallFocus();
            const auto updateRawMouse = []()
            {
                SetRawEnabled(FixSettings().Get("MAIN.RawInputMouse") == "1");
            };
            updateRawMouse();
            FixSettings().OnChange("MAIN.RawInputMouse", updateRawMouse);
            auto mouse = hook::pattern("DB 44 24 1C 0F B6 54 24 28 F7 C2 80 00 00 00");
            rawMouseSample = safetyhook::create_mid(mouse.get_first(), [](SafetyHookContext& regs)
            {
                // The native accumulator reads these locals before writing them.
                auto* motion = reinterpret_cast<uint32_t*>(regs.esp);
                motion[0] = 0;
                motion[1] = 0;
                SampleRawMouse(reinterpret_cast<int32_t*>(regs.esp + 0x1C));
            });
            bFirstStart = !HasSavedVideoMode();

            // The first ShowWindow of the game's window uses the launch's show command. Started minimised, the engine
            // can't create its frontend camera nor reset the device, so the window opens in the background instead.
            auto show = hook::pattern("55 8B 35 ? ? ? ? 8B 7E 0C FF 77 18 FF 15");
            static auto FirstShow = safetyhook::create_mid(show.get_first(13), [](SafetyHookContext& regs)
            {
                auto& command = *reinterpret_cast<int32_t*>(regs.esp + 4);
                if (command == SW_SHOWMINIMIZED)
                    command = SW_SHOWNORMAL;
                else if (command == SW_MINIMIZE || command == SW_SHOWMINNOACTIVE || command == SW_FORCEMINIMIZE)
                    command = SW_SHOWNOACTIVATE;
            });

            // Keeps the cursor inside the game's window while it's in front
            CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
            FixSettings().OnChange("MAIN.ConfineCursor", []()
            {
                CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
            });
            WFP::onGameProcessEvent() += []()
            {
                auto rs = RsGlobal.get_ptr();
                FollowClientSize(rs->ps ? rs->ps->window : nullptr);
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
        };
        WFP::onShutdownEvent() += []()
        {
            CursorConfinement::Shutdown();
        };
    }
} Windowed;
