module;

#include "stdafx.h"

export module RawMouse;

export struct MouseDelta
{
    float x = 0.0f;
    float y = 0.0f;
    uint64_t generation = 0;
};

namespace
{
    std::mutex mouseMutex;
    int64_t pendingX = 0;
    int64_t pendingY = 0;
    HWND inputWindow = nullptr;
    bool registered = false;
    bool relativeDevice = false;
    uint64_t mouseGeneration = 0;
    HWND ownedRegistrationWindow = nullptr;
    std::mutex registrationMutex;
    std::optional<RAWINPUTDEVICE> legacyChange;
    RAWINPUTDEVICE restoreLegacy{};
    std::atomic<bool> onlyMouse = false;

    void ResetMotionLocked()
    {
        ++mouseGeneration;
        pendingX = pendingY = 0;
        relativeDevice = false;
    }

    void PublishRegistration(HWND window, bool ready, bool created = false)
    {
        std::scoped_lock lock(mouseMutex);
        if (created || inputWindow != window || registered != ready)
            ResetMotionLocked();
        inputWindow = window;
        registered = ready;
    }

    bool MouseReadyLocked()
    {
        return registered && relativeDevice && inputWindow && inputWindow == GetForegroundWindow();
    }

    bool MouseRegistration(RAWINPUTDEVICE& mouse)
    {
        UINT count = 0;
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
            return false;
        std::vector<RAWINPUTDEVICE> devices(count);
        if (count && GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
            return false;
        for (const auto& device : devices)
            if (device.usUsagePage == 1 && device.usUsage == 2)
                mouse = device;
        onlyMouse = count == 1 && mouse.usUsage == 2;
        if (legacyChange && (mouse.usUsage != 2 || mouse.hwndTarget != legacyChange->hwndTarget || mouse.dwFlags != legacyChange->dwFlags))
        {
            legacyChange.reset();
            ownedRegistrationWindow = nullptr;
        }
        if (ownedRegistrationWindow && (mouse.hwndTarget != ownedRegistrationWindow || (mouse.dwFlags && !legacyChange)))
            ownedRegistrationWindow = nullptr;
        return true;
    }

    bool SetLegacyLocked(bool legacy, HWND window)
    {
        RAWINPUTDEVICE mouse{};
        if (!MouseRegistration(mouse))
            return false;
        if (legacy && legacyChange)
        {
            if (RegisterRawInputDevices(&restoreLegacy, 1, sizeof(restoreLegacy)))
                legacyChange.reset();
            return true;
        }
        if (!window || (mouse.hwndTarget && mouse.hwndTarget != window))
            return false;
        const bool noLegacy = (mouse.dwFlags & RIDEV_NOLEGACY) == RIDEV_NOLEGACY;
        if (!legacy && !noLegacy)
        {
            const RAWINPUTDEVICE target{ 1, 2, mouse.dwFlags | RIDEV_NOLEGACY, mouse.usUsage ? mouse.hwndTarget : window };
            if (RegisterRawInputDevices(&target, 1, sizeof(target)))
            {
                restoreLegacy = mouse.usUsage ? mouse : RAWINPUTDEVICE{ 1, 2, RIDEV_REMOVE, nullptr };
                legacyChange = target;
            }
        }
        return noLegacy;
    }
}

// Every report of a mouse is also turned into legacy input for the window thread: a hit test, WM_NCHITTEST and
// WM_SETCURSOR (Unreal asks Slate for the cursor each time) and WM_MOUSEMOVE. With a high polling rate mouse that
// takes the window thread most of each frame. While the cursor is in the client area of the window in front, the
// mouse registration gets RIDEV_NOLEGACY (one is made for the window when there's none) and the window procedure
// passes those messages on once per read (see Window). Returns whether legacy messages were off before.
export bool SetLegacyMouse(bool legacy, HWND window = nullptr)
{
    std::lock_guard guard(registrationMutex);
    return SetLegacyLocked(legacy, window);
}

export bool IsOnlyRawMouse()
{
    return onlyMouse;
}

export void ResetRawMouse()
{
    std::scoped_lock lock(mouseMutex);
    ResetMotionLocked();
}

export uint64_t MouseReadGeneration()
{
    std::scoped_lock lock(mouseMutex);
    return mouseGeneration;
}

export bool IsRawMouseReady()
{
    std::scoped_lock lock(mouseMutex);
    return MouseReadyLocked();
}

export bool IsRawMouseReady(const MouseDelta& sample)
{
    std::scoped_lock lock(mouseMutex);
    return sample.generation == mouseGeneration && MouseReadyLocked();
}

// Unreal removes raw registration when high-precision capture ends.
// Keep its active registration and reconcile before each pad update.
export void RefreshRawMouseRegistration()
{
    const auto window = GetForegroundWindow();
    DWORD process = 0;
    if (!window || !GetWindowThreadProcessId(window, &process) || process != GetCurrentProcessId())
    {
        PublishRegistration(nullptr, false);
        return;
    }
    std::lock_guard guard(registrationMutex);
    RAWINPUTDEVICE mouse{};
    if (!MouseRegistration(mouse))
    {
        PublishRegistration(window, false);
        return;
    }
    if (mouse.usUsage)
    {
        const bool ready = !mouse.hwndTarget || mouse.hwndTarget == window;
        if (ready && legacyChange && restoreLegacy.dwFlags == RIDEV_REMOVE)
        {
            ownedRegistrationWindow = window;
            restoreLegacy = { 1, 2, 0, window };
        }
        PublishRegistration(window, ready);
        return;
    }
    const RAWINPUTDEVICE device{ 1, 2, 0, window };
    const bool ready = RegisterRawInputDevices(&device, 1, sizeof(device)) != FALSE;
    if (ready)
        ownedRegistrationWindow = window;
    PublishRegistration(window, ready, ready);
}

export void ReadRawMouse(HWND window, WPARAM wParam, const RAWMOUSE& mouse, uint64_t& generation)
{
    if (GET_RAWINPUT_CODE_WPARAM(wParam) != RIM_INPUT)
        return;

    std::scoped_lock lock(mouseMutex);
    if (generation != mouseGeneration || !registered || window != inputWindow || window != GetForegroundWindow())
        return;
    if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
    {
        ResetMotionLocked();
        generation = mouseGeneration;
        return;
    }
    if (!mouse.lLastX && !mouse.lLastY)
        return;
    relativeDevice = true;
    pendingX += mouse.lLastX;
    pendingY += mouse.lLastY;
}

export MouseDelta ConsumeRawMouse()
{
    std::scoped_lock lock(mouseMutex);
    const MouseDelta sample{ static_cast<float>(pendingX), static_cast<float>(pendingY), mouseGeneration };
    pendingX = pendingY = 0;
    return sample;
}

// The mouse registration made here goes when raw mouse input is switched off: the game reads the cursor then
export void ReleaseRawMouseRegistration()
{
    std::lock_guard guard(registrationMutex);
    PublishRegistration(nullptr, false);
    if (!ownedRegistrationWindow)
        return;

    RAWINPUTDEVICE mouse{};
    if (!MouseRegistration(mouse) || !ownedRegistrationWindow)
        return;
    if (legacyChange)
    {
        if (!RegisterRawInputDevices(&restoreLegacy, 1, sizeof(restoreLegacy)))
            return;
        mouse = restoreLegacy;
        legacyChange.reset();
    }
    if (ownedRegistrationWindow && mouse.hwndTarget == ownedRegistrationWindow && mouse.dwFlags == 0)
    {
        const RAWINPUTDEVICE remove{ 1, 2, RIDEV_REMOVE, nullptr };
        if (!RegisterRawInputDevices(&remove, 1, sizeof(remove)))
            return;
    }
    ownedRegistrationWindow = nullptr;
}

export void ShutdownRawMouse()
{
    ResetRawMouse();
    SetLegacyMouse(true);
    ReleaseRawMouseRegistration();
}
