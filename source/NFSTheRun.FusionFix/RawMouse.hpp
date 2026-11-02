#pragma once

#include <atomic>
#include <limits>

// Raw mouse movement (WM_INPUT) for the free-look camera: counts straight from the mouse, without the pointer's
// acceleration and without the movement lost when the cursor is put back in the middle of the window. The game reads
// its keyboard and mouse as raw input itself; when nothing registered the mouse, it's registered for the game window.
namespace RawMouse
{
    inline std::atomic<bool> Enabled{ false };
    inline std::atomic<int64_t> DeltaX{ 0 }, DeltaY{ 0 };
    inline std::atomic<bool> Relative{ true };
    inline HWND Window = nullptr;
    inline bool Registered = false, Owned = false;

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
            RAWINPUTDEVICE device{ 1, 2, RIDEV_REMOVE, nullptr };
            RegisterRawInputDevices(&device, 1, sizeof(device));
        }
        Registered = Owned = false;
        Window = nullptr;
        Clear();
    }

    // Every frame, on the window's thread
    inline void Update(HWND window)
    {
        if (!Enabled.load() || !window || !IsWindow(window))
        {
            if (Window)
                Release();
            return;
        }
        if (Window == window)
            return;
        Release();
        Window = window;
        RAWINPUTDEVICE current{};
        if (Registration(current))
        {
            // The game's own registration, or another plugin's: used when it sends the mouse to this window
            Registered = (!current.hwndTarget || current.hwndTarget == window) && !(current.dwFlags & RIDEV_REMOVE);
        }
        else
        {
            RAWINPUTDEVICE mouse{ 1, 2, 0, window };
            Registered = Owned = RegisterRawInputDevices(&mouse, 1, sizeof(mouse)) != FALSE;
        }
        Relative.store(true);
    }

    // The window's messages, before the game's
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

    // The movement since the last call; false when raw input isn't in use (the cursor's movement is used then)
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
