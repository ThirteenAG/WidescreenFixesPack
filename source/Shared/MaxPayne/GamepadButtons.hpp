#pragma once
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include "Shared/PadInput.hpp"

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

    inline std::atomic<DWORD> selectedSlot{ XUSER_MAX_COUNT };
    inline std::atomic<int> selectedFamily{ Xbox };
    inline std::atomic<ULONGLONG> connectedAt{ 0 };
    inline std::atomic<ULONGLONG> nPadActivity{ 0 };

    inline int ControllerFamily(DWORD slot)
    {
        // XInput 1.4 ordinal 108 includes the vendor ID for the requested user index.
        struct Capabilities
        {
            XINPUT_CAPABILITIES standard;
            WORD vendor, product, revision, reserved;
            DWORD extra;
        };
        using Query = DWORD (WINAPI*)(DWORD, DWORD, DWORD, Capabilities*);
        const auto module = GetModuleHandleW(L"xinput1_4.dll");
        if (!module || GetProcAddress(module, "XInputGetState") != reinterpret_cast<FARPROC>(PadInput::XInput().getState))
            return Xbox;
        const auto query = reinterpret_cast<Query>(GetProcAddress(module, MAKEINTRESOURCEA(108)));
        Capabilities caps{};
        if (query && query(1, slot, 0, &caps) == ERROR_SUCCESS)
        {
            if (caps.vendor == 0x054C)
                return PlayStation;
            if (caps.vendor == 0x057E)
                return Nintendo;
        }
        return Xbox;
    }

    inline void SelectController(DWORD slot)
    {
        nPadActivity.store(0);
        connectedAt.store(GetTickCount64());
        selectedFamily.store(slot < XUSER_MAX_COUNT ? ControllerFamily(slot) : Xbox);
        selectedSlot.store(slot);
    }

    inline int DetectFamily()
    {
        return selectedFamily.load();
    }

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

        const auto slot = selectedSlot.load();
        if (slot >= XUSER_MAX_COUNT)
            return false;
        ULONGLONG nPad = nPadActivity.load(std::memory_order_relaxed);
        if (getActivity)
        {
            const auto tick = getActivity(slot);
            if (tick >= connectedAt.load())
                nPad = std::max(nPad, tick);
        }
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
