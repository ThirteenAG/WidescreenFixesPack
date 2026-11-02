module;

#include <stdafx.h>
#include <atomic>
#include <stdexcept>

export module Xidi;

import ComVars;

typedef bool (*XidiSendVibrationFunc)(short, unsigned short, unsigned short);
export std::atomic<XidiSendVibrationFunc> XidiSendVibration{ nullptr };

export void InitXidi()
{
    const auto xidiModule = GetModuleHandleW(L"Xidi.32.dll");
    if (!xidiModule)
        throw std::runtime_error("Controller proxy unavailable");
    const auto sendVibration = reinterpret_cast<XidiSendVibrationFunc>(GetProcAddress(xidiModule, "XidiSendVibration"));
    if (!sendVibration)
        throw std::runtime_error("Controller vibration export unavailable");
    XidiSendVibration.store(sendVibration, std::memory_order_release);
}
