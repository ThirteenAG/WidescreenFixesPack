module;

#include <stdafx.h>
#include <array>
#include <dinput.h>
#include <stdexcept>

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

    bool(__thiscall* button)(void*, int, uint32_t, int){};
    void*(__thiscall* record)(void*, int){};

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

    uint32_t MenuMask()
    {
        if (Blocked() || !GodfatherInputSlot.is_initialized() || !button || !record)
            return 0;
        auto input = GodfatherInputSlot.get();
        if (!input)
            return 0;
        const auto bytes = static_cast<const uint8_t*>(input);
        if (bytes[56] && bytes[988] >= 5)
            return 0;
        auto pad = static_cast<const uint8_t*>(record(input, 0));
        if (!pad)
            return 0;
        constexpr uint32_t buttons[]{ 0x1000, 0x2000, 0x0200, 0x0100, 0x0400, 0x0008, 0x0800 };
        uint32_t mask = 0;
        for (size_t i = 0; i < std::size(buttons); ++i)
            if (button(input, 0, buttons[i], 1))
                mask |= 1u << i;
        const auto axes = reinterpret_cast<const float*>(pad);
        if (std::isfinite(axes[0]))
        {
            if (axes[0] < -0.5f) mask |= 1u << 2;
            if (axes[0] > 0.5f) mask |= 1u << 3;
        }
        if (std::isfinite(axes[1]))
        {
            if (axes[1] < -0.5f) mask |= 1u << 0;
            if (axes[1] > 0.5f) mask |= 1u << 1;
        }
        return mask;
    }

    void Init()
    {
        auto cooperative = hook::pattern("8B 86 64 04 00 00 8B 48 08 85 C9 74 28 8B 07 8B 10 6A 05 51 50 FF 52 34 85 C0 74 19");
        cooperativeHook = safetyhook::create_mid(cooperative.get_first(19), Cooperative);
        auto root = hook::pattern("56 8B F1 8B 0D ? ? ? ? 6A 00 E8 ? ? ? ? 85 C0 74 ? 8B 48 30 85 C9");
        auto query = hook::pattern("8B 44 24 04 53 50 32 DB E8 ? ? ? ? 85 C0 74 ? 8B 4C 24 0C 33 D2 66 8B 50 38 "
            "66 F7 D2 23 CA 0F B6 54 24 10 83 FA 03 77 ? FF 24 95 ? ? ? ? 8B 50 30 0F B7 C9 85 D1 0F 95 C3");
        GodfatherInputSlot.SetAddress(*root.get_first<void**>(5));
        button = reinterpret_cast<decltype(button)>(query.get_first());
        record = reinterpret_cast<decltype(record)>(injector::GetBranchDestination(query.get_first<uint8_t>(8)).as_int());
    }
}
