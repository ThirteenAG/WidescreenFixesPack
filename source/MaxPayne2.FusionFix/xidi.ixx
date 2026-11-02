module;

#include <stdafx.h>
#include <atomic>
#include <stdexcept>
#include "DeveloperOverlay.hpp"

export module xidi;

import ComVars;

typedef bool (*XidiSendVibrationFunc)(short, unsigned short, unsigned short);
export std::atomic<XidiSendVibrationFunc> XidiSendVibration{ nullptr };

export void InitXidi()
{
    using RegisterProfile = bool (*)(const wchar_t* (*)());
    const auto module = GetModuleHandleW(L"Xidi.32.dll");
    if (!module)
        throw std::runtime_error("Controller proxy unavailable");
    const auto registerProfile = reinterpret_cast<RegisterProfile>(GetProcAddress(module, "XidiRegisterProfileCallback"));
    const auto sendVibration = reinterpret_cast<XidiSendVibrationFunc>(GetProcAddress(module, "XidiSendVibration"));
    if (!registerProfile || !sendVibration)
        throw std::runtime_error("Controller proxy exports unavailable");
    if (!registerProfile([]() -> const wchar_t*
    {
        if (MaxPayneOverlay::BlocksInput())
            return L"Overlay";
        switch (GamepadProfile.load(std::memory_order_relaxed))
        {
            case eGamepadProfile::Main:
                return L"Main";
            case eGamepadProfile::Pause:
                return L"Pause";
            default:
                return L"Menu";
        }
    }))
        throw std::runtime_error("Controller profile callback rejected");
    XidiSendVibration.store(sendVibration, std::memory_order_release);
}
