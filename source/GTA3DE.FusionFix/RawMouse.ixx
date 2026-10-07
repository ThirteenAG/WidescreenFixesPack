module;

#include "stdafx.h"

export module RawMouse;

export struct MouseDelta
{
    float x = 0.0f;
    float y = 0.0f;
};

namespace
{
    std::mutex mouseMutex;
    int64_t pendingX = 0;
    int64_t pendingY = 0;
    std::atomic<HWND> inputWindow = nullptr;
    std::atomic<bool> registered = false;
    HWND ownedRegistrationWindow = nullptr;
    std::atomic<bool> relativeDevice = false;
}

export void ResetRawMouse()
{
    std::scoped_lock lock(mouseMutex);
    pendingX = pendingY = 0;
}

export bool IsRawMouseReady()
{
    const auto window = inputWindow.load();
    return registered.load() && relativeDevice.load() && window && window == GetForegroundWindow();
}

// Unreal removes its raw mouse registration when high-precision capture ends.
// Reconcile it each pad update rather than caching readiness for an HWND's
// lifetime. Keep Unreal's active registration and legacy menu/button messages.
export void RefreshRawMouseRegistration()
{
    const auto window = GetForegroundWindow();
    DWORD process = 0;
    if (!window || !GetWindowThreadProcessId(window, &process) || process != GetCurrentProcessId())
    {
        registered = false;
        relativeDevice = false;
        ResetRawMouse();
        return;
    }
    if (inputWindow.exchange(window) != window)
    {
        relativeDevice = false;
        ResetRawMouse();
    }
    UINT count = 0;
    if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
    {
        registered = false;
        return;
    }
    std::vector<RAWINPUTDEVICE> devices(count);
    if (count && GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
    {
        registered = false;
        return;
    }
    for (const auto& device : devices)
        if (device.usUsagePage == 1 && device.usUsage == 2)
        {
            registered = !device.hwndTarget || device.hwndTarget == window;
            return;
        }
    const RAWINPUTDEVICE device{1, 2, 0, window};
    registered = RegisterRawInputDevices(&device, 1, sizeof(device)) != FALSE;
    if (registered)
        ownedRegistrationWindow = window;
}

export void ReadRawMouseMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (window != inputWindow || window != GetForegroundWindow())
        return;

    if (message != WM_INPUT || GET_RAWINPUT_CODE_WPARAM(wParam) != RIM_INPUT || !registered)
        return;

    RAWINPUT input{};
    UINT size = sizeof(input);
    const auto read = GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT,
        &input, &size, sizeof(RAWINPUTHEADER));
    if (read == UINT(-1) || read < sizeof(RAWINPUTHEADER) + sizeof(RAWMOUSE)
        || input.header.dwType != RIM_TYPEMOUSE)
        return;

    if (input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
    {
        relativeDevice = false;
        ResetRawMouse();
        return; // let Unreal handle absolute pointing devices
    }
    relativeDevice = true;
    std::scoped_lock lock(mouseMutex);
    pendingX += input.data.mouse.lLastX;
    pendingY += input.data.mouse.lLastY;
}

export MouseDelta ConsumeRawMouse()
{
    std::scoped_lock lock(mouseMutex);
    const MouseDelta sample{static_cast<float>(pendingX), static_cast<float>(pendingY)};
    pendingX = pendingY = 0;
    return sample;
}

export void ShutdownRawMouse()
{
    ResetRawMouse();
    if (ownedRegistrationWindow)
    {
        UINT count = 0;
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) != UINT(-1) && count)
        {
            std::vector<RAWINPUTDEVICE> devices(count);
            if (GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) != UINT(-1))
                for (const auto& device : devices)
                    if (device.usUsagePage == 1 && device.usUsage == 2
                        && device.hwndTarget == ownedRegistrationWindow && device.dwFlags == 0)
                    {
                        const RAWINPUTDEVICE remove{1, 2, RIDEV_REMOVE, nullptr};
                        RegisterRawInputDevices(&remove, 1, sizeof(remove));
                        break;
                    }
        }
    }
    registered = false;
    ownedRegistrationWindow = nullptr;
    relativeDevice = false;
    inputWindow = nullptr;
}
