module;

#include <stdafx.h>
#include <array>
#include <dinput.h>
#include <stdexcept>
#include "Shared/PadInput.hpp"

export module Input;

import Window;

import ComVars;

export namespace GodfatherInput
{
    bool(*blocked)(){};
    SafetyHookMid cooperativeHook;

    void Cooperative(SafetyHookContext& context)
    {
        if (!GodfatherWindow::mode)
            return;
        const auto size = *reinterpret_cast<DWORD*>(context.esp + 0x0C);
        const auto type = *reinterpret_cast<DWORD*>(context.esp + 0x30) & 0xFF;
        if (size != sizeof(DIDEVICEINSTANCEA) || type != DI8DEVTYPE_MOUSE)
            return;
        const auto flags = reinterpret_cast<DWORD*>(context.esp);
        // Windowed mouse input must not depend on exclusive desktop ownership.
        *flags = (*flags & ~DISCL_EXCLUSIVE) | DISCL_NONEXCLUSIVE;
    }

    bool Blocked()
    {
        return blocked && blocked();
    }

    void Mute(void* input)
    {
        if (!input)
            return;
        const auto records = static_cast<uint8_t*>(input) + 64;
        for (size_t slot = 0; slot < 7; ++slot)
            for (size_t offset = 0; offset < 124; offset += sizeof(uint32_t))
                *reinterpret_cast<uint32_t*>(records + 132 * slot + offset) = 0;
    }

    void MutePc(void* input)
    {
        Mute(input);
        if (!input)
            return;
        const auto bytes = static_cast<uint8_t*>(input);
        for (size_t i = 0; i < 3; ++i)
        {
            const auto count = *reinterpret_cast<uint32_t*>(bytes + 1408 + 36 * i);
            const auto current = *reinterpret_cast<uint8_t**>(bytes + 1424 + 36 * i);
            const auto previous = *reinterpret_cast<uint8_t**>(bytes + 1428 + 36 * i);
            if (count > 256 || !current || !previous)
                continue;
            for (size_t key = 0; key < count; ++key)
            {
                current[key] = 0;
                previous[key] = 0;
            }
        }
        for (size_t offset = 1252; offset < 1272; offset += sizeof(uint32_t))
            *reinterpret_cast<uint32_t*>(bytes + offset) = 0;
        *reinterpret_cast<uint16_t*>(bytes + 1272) = 0;
        bytes[2352] = 0;
    }

    uint32_t MenuButtons(const XINPUT_GAMEPAD& pad)
    {
        constexpr WORD buttons[]{ XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN,
            XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT, XINPUT_GAMEPAD_A,
            XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_Y };
        uint32_t mask{};
        for (size_t i = 0; i < std::size(buttons); ++i)
            if (pad.wButtons & buttons[i])
                mask |= 1u << i;
        constexpr int half = 16384;
        if (pad.sThumbLY > half) mask |= 1u << 0;
        if (pad.sThumbLY < -half) mask |= 1u << 1;
        if (pad.sThumbLX < -half) mask |= 1u << 2;
        if (pad.sThumbLX > half) mask |= 1u << 3;
        return mask;
    }

    uint32_t MenuPresses(bool acceptInput)
    {
        static std::array<uint32_t, XUSER_MAX_COUNT> previous{};
        static DWORD owner = XUSER_MAX_COUNT;
        const auto getState = PadInput::XInput().getState;
        if (!getState)
            return 0;

        DWORD nextOwner = XUSER_MAX_COUNT;
        uint32_t nextPresses{};
        for (DWORD slot = 0; slot < XUSER_MAX_COUNT; ++slot)
        {
            XINPUT_STATE state{};
            const bool connected = getState(slot, &state) == ERROR_SUCCESS;
            if (slot == owner && !connected)
                owner = XUSER_MAX_COUNT;
            const auto mask = connected ? MenuButtons(state.Gamepad) : 0;
            const auto presses = mask & ~std::exchange(previous[slot], mask);
            if (presses && (nextOwner == XUSER_MAX_COUNT || slot == owner))
            {
                nextOwner = slot;
                nextPresses = presses;
            }
        }
        if (!acceptInput || !nextPresses)
            return 0;
        owner = nextOwner;
        return nextPresses;
    }

    void Init()
    {
        auto cooperative = hook::pattern("8B 86 64 04 00 00 8B 48 08 85 C9 74 28 8B 07 8B 10 6A 05 51 50 FF 52 34 85 C0 74 19");
        cooperativeHook = safetyhook::create_mid(cooperative.get_first(19), Cooperative);
        auto root = hook::pattern("56 8B F1 8B 0D ? ? ? ? 6A 00 E8 ? ? ? ? 85 C0 74 ? 8B 48 30 85 C9");
        GodfatherInputSlot.SetAddress(*root.get_first<void**>(5));
    }
}
