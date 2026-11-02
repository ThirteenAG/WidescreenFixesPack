module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Frame.hpp"
#include "VibrationInput.hpp"
#include <stdexcept>

export module Vibration;

import ComVars;

import Focus;

export namespace ScarfaceVibration
{
    inline SafetyHookInline updateHook;
    inline bool bound = false;

    inline bool Allowed()
    {
        return bound && !vibrationDisabled.get() && !ScarfaceFocus::Background();
    }

    inline void Stop()
    {
        const auto list = bound ? vibrationOutputs.get() : nullptr;
        if (!list || reinterpret_cast<uintptr_t>(list) == 0xDDDDDDDD)
            return;
        const auto count = *reinterpret_cast<const int16_t*>(list + 8);
        const auto items = *reinterpret_cast<uint8_t* const* const*>(list);
        if (!items || count < 0 || count > 256)
            return;
        for (int i = 0; i < count; ++i)
        {
            const auto item = items[i];
            if (!item || !*reinterpret_cast<const int*>(item + 8))
                continue;
            const auto name = *reinterpret_cast<const char* const*>(item + 4);
            auto device = *reinterpret_cast<void**>(item + 12);
            if (!name || !device || ((name[0] != '0' && name[0] != '1') || name[1]))
                continue;
            const auto methods = *static_cast<uintptr_t**>(device);
            reinterpret_cast<void(__thiscall*)(void*, int)>(methods[7])(device, 0);
        }
    }

    inline int __cdecl Update(void* time)
    {
        const auto previous = vibrationDisabled.get();
        if (!Allowed() && !previous)
            vibrationDisabled = 1;
        const auto result = updateHook.unsafe_ccall<int>(time);
        if (vibrationDisabled.get() != previous)
            vibrationDisabled = previous;
        return result;
    }

    inline bool Install()
    {
        auto update = hook::pattern("83 EC 0C A1 ? ? ? ? 0F BF 40 08 53 33 DB 85 C0 89 44 24 0C 0F 8E");
        const auto address = update.get_first<uint8_t>();
        auto flag = hook::range_pattern(reinterpret_cast<uintptr_t>(address), reinterpret_cast<uintptr_t>(address + 0x100),
            "80 3D ? ? ? ? 00 74 08 C7 44 24 10 00 00 00 00 80 3D");
        vibrationOutputs.SetAddress(*reinterpret_cast<uint8_t***>(address + 4));
        vibrationDisabled.SetAddress(*flag.get_first<uint8_t*>(2));
        bound = true;
        updateHook = safetyhook::create_inline(address, Update);
        if (!updateHook)
        {
            bound = false;
            throw std::runtime_error("Vibration update hook unavailable");
        }
        ScarfaceVibrationInput::stop = Stop;
        ScarfaceFrame::Handlers.push_back([](void*) { if (!Allowed()) Stop(); });
        WFP::onShutdownEvent() += []()
        {
            ScarfaceVibrationInput::stop = nullptr;
            updateHook.reset();
            bound = false;
        };
        return true;
    }
}
