#pragma once

#include <atomic>
#include <limits>

namespace RawMouse
{
    inline std::atomic<bool> Enabled{ false };
    inline std::atomic<int64_t> DeltaX{ 0 }, DeltaY{ 0 };
    inline std::atomic<bool> Relative{ true };
    inline HWND Window = nullptr;
    inline bool Registered = false, Owned = false, HadPrevious = false;
    inline RAWINPUTDEVICE Previous{};

    inline void Clear()
    {
        DeltaX.store(0);
        DeltaY.store(0);
    }

    inline bool Registration(RAWINPUTDEVICE& mouse)
    {
        UINT count = 0;
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(mouse)) == UINT(-1) || !count)
            return false;
        std::vector<RAWINPUTDEVICE> devices(count);
        if (GetRegisteredRawInputDevices(devices.data(), &count, sizeof(mouse)) == UINT(-1))
            return false;
        for (const auto& device : devices)
        {
            if (device.usUsagePage == 1 && device.usUsage == 2)
            {
                mouse = device;
                return true;
            }
        }
        return false;
    }

    inline void Release()
    {
        RAWINPUTDEVICE current{};
        if (Owned && Registration(current) && current.hwndTarget == Window && current.dwFlags == 0)
        {
            auto device = HadPrevious ? Previous : RAWINPUTDEVICE{ 1, 2, RIDEV_REMOVE, nullptr };
            RegisterRawInputDevices(&device, 1, sizeof(device));
        }
        Registered = Owned = false;
        Window = nullptr;
        Clear();
    }

    inline void Pump(HWND window)
    {
        if (!Registered || GetWindowThreadProcessId(window, nullptr) != GetCurrentThreadId())
            return;
        // The native loop dispatches one message per rendered frame.
        MSG message{};
        for (unsigned count = 0; count < 8192 && PeekMessageW(&message, window, WM_INPUT, WM_INPUT, PM_REMOVE); ++count)
            DispatchMessageW(&message);
    }

    inline void Update(HWND window)
    {
        if (!Enabled.load() || !window || !IsWindow(window))
        {
            if (Window)
                Release();
            return;
        }
        if (Window == window)
        {
            if (Registered)
            {
                RAWINPUTDEVICE current{};
                if (!Registration(current) || current.hwndTarget != window || current.dwFlags != 0)
                {
                    Registered = Owned = false;
                    Clear();
                    OutputDebugStringA("Undercover: raw mouse ownership changed; using DirectInput.\n");
                }
            }
            Pump(window);
            return;
        }
        Release();
        Window = window;
        RAWINPUTDEVICE current{};
        HadPrevious = Registration(current);
        Previous = current;
        // Respect another plugin's explicit target and legacy mouse policy.
        if (HadPrevious && ((current.hwndTarget && current.hwndTarget != window) || current.dwFlags != 0))
        {
            OutputDebugStringA("Undercover: another raw mouse registration is active; using DirectInput.\n");
            return;
        }
        if (HadPrevious && current.hwndTarget == window)
            Registered = true;
        else
        {
            RAWINPUTDEVICE mouse{ 1, 2, 0, window };
            Registered = Owned = RegisterRawInputDevices(&mouse, 1, sizeof(mouse)) != FALSE;
            if (!Registered)
                OutputDebugStringA("Undercover: raw mouse registration failed; using DirectInput.\n");
        }
        Relative.store(true);
        Pump(window);
    }

    inline void Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_INPUT && Registered && Enabled.load() && window == Window && GetForegroundWindow() == window)
        {
            RAWINPUT input{};
            UINT size = sizeof(input);
            if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER)) != UINT(-1) &&
                input.header.dwType == RIM_TYPEMOUSE)
            {
                bool relative = !(input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE);
                Relative.store(relative);
                if (relative)
                {
                    DeltaX.fetch_add(input.data.mouse.lLastX);
                    DeltaY.fetch_add(input.data.mouse.lLastY);
                }
                else
                    Clear();
            }
        }
        else if (message == WM_KILLFOCUS || (message == WM_ACTIVATEAPP && !wParam))
            Clear();
        else if (message == WM_NCDESTROY && window == Window)
            Release();
    }

    inline bool Consume(int& x, int& y)
    {
        auto dx = DeltaX.exchange(0);
        auto dy = DeltaY.exchange(0);
        if (!Registered || !Enabled.load() || !Relative.load())
            return false;
        x = static_cast<int>(std::clamp<int64_t>(dx, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
        y = static_cast<int>(std::clamp<int64_t>(dy, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
        return true;
    }
}
