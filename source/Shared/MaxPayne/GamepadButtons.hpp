#pragma once
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <vector>

// Controller button names for the tips of both Max Payne games (MAIN.GamepadIcons). The tips name the key of an action
// by a token ("Press #USE# to ..."), which the game replaces with the key's name; while a controller is the device
// used last, the token gets the name of the controller button the fix maps the action to. The games draw tips as text
// in their own font, so the names are text: "A", "Cross", ...
namespace MaxPayneGamepadButtons
{
    // The button numbers of the fix's controller mapping (the Xidi controller's buttons)
    enum Button
    {
        A, B, X, Y, LB, RB, LT, RT, Back, Start, LS, RS, DpadUp, DpadDown, DpadLeft, DpadRight, Count
    };

    enum Family
    {
        Auto,
        Xbox,
        PlayStation,
        Nintendo,
    };

    inline const char* Name(int button, int family)
    {
        static constexpr const char* Names[3][Count] =
        {
            { "A", "B", "X", "Y", "LB", "RB", "LT", "RT", "Back", "Start", "LS", "RS", "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right" },
            { "Cross", "Circle", "Square", "Triangle", "L1", "R1", "L2", "R2", "Share", "Options", "L3", "R3", "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right" },
            // by position: the bottom button of a Nintendo controller is B
            { "B", "A", "Y", "X", "L", "R", "ZL", "ZR", "-", "+", "L Stick", "R Stick", "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right" },
        };
        if (button < 0 || button >= Count)
            return nullptr;
        return Names[std::clamp(family, int(Xbox), int(Nintendo)) - 1][button];
    }

    // The family of the controller connected, by its maker: Sony or Nintendo, else Xbox names. Read from the raw input
    // device list (no registration, only the list of devices), looked up again now and then for controllers plugged
    // in later.
    inline int DetectFamily()
    {
        static int family = Xbox;
        static ULONGLONG lastCheck = 0;
        if (lastCheck && GetTickCount64() - lastCheck < 3000)
            return family;
        lastCheck = GetTickCount64();

        UINT nDevices = 0;
        if (GetRawInputDeviceList(nullptr, &nDevices, sizeof(RAWINPUTDEVICELIST)) != 0 || nDevices == 0)
            return family;
        std::vector<RAWINPUTDEVICELIST> devices(nDevices);
        nDevices = GetRawInputDeviceList(devices.data(), &nDevices, sizeof(RAWINPUTDEVICELIST));
        if (nDevices == UINT(-1))
            return family;

        family = Xbox;
        for (UINT i = 0; i < nDevices; ++i)
        {
            if (devices[i].dwType != RIM_TYPEHID)
                continue;
            RID_DEVICE_INFO info = { sizeof(RID_DEVICE_INFO) };
            UINT size = sizeof(info);
            if (GetRawInputDeviceInfoW(devices[i].hDevice, RIDI_DEVICEINFO, &info, &size) == UINT(-1))
                continue;
            // joysticks and gamepads
            if (info.hid.usUsagePage != 1 || (info.hid.usUsage != 4 && info.hid.usUsage != 5))
                continue;
            if (info.hid.dwVendorId == 0x054C)
            {
                family = PlayStation;
                break;
            }
            if (info.hid.dwVendorId == 0x057E)
            {
                family = Nintendo;
                break;
            }
        }
        return family;
    }

    // Last device: the controller when it was used after the keyboard and mouse. Xidi turns some controller buttons
    // into key presses, and those count as keyboard input for Windows: keyboard input up to a moment after controller
    // input is taken as coming from the controller.
    inline std::atomic<ULONGLONG> nPadActivity = 0;

    // From the fix's controller reading, whenever a button or a stick moved
    inline void OnPadActivity()
    {
        nPadActivity.store(GetTickCount64(), std::memory_order_relaxed);
    }

    inline bool IsPadInUse()
    {
        using GetActivity = uint64_t(__cdecl*)(unsigned);
        static auto getActivity = []() -> GetActivity
        {
            auto xidi = GetModuleHandleW(L"Xidi.32.dll");
            return xidi ? reinterpret_cast<GetActivity>(GetProcAddress(xidi, "XidiGetLastControllerActivity")) : nullptr;
        }();

        ULONGLONG nPad = nPadActivity.load(std::memory_order_relaxed);
        for (unsigned i = 0; getActivity && i < 4; ++i)
            nPad = std::max<ULONGLONG>(nPad, getActivity(i));
        if (nPad == 0)
            return false;

        LASTINPUTINFO lastInput = { sizeof(LASTINPUTINFO) };
        if (!GetLastInputInfo(&lastInput))
            return true;

        // GetLastInputInfo has the 32-bit tick count
        constexpr DWORD nInjectedDelay = 300;
        return int32_t(lastInput.dwTime - DWORD(nPad)) <= int32_t(nInjectedDelay);
    }

    // The name to show for a button, or nullptr for the game's own key name
    inline const char* NameInUse(int button, int nSetting)
    {
        if (button < 0 || button >= Count || !IsPadInUse())
            return nullptr;
        return Name(button, nSetting == Auto ? DetectFamily() : nSetting);
    }
}
