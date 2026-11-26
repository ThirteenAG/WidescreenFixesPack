#pragma once
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace REInputDrain
{
    template<class T> T Read(const void* object, size_t offset)
    {
        T value;
        std::memcpy(&value, static_cast<const uint8_t*>(object) + offset, sizeof(value));
        return value;
    }

    template<class T> void Write(void* object, size_t offset, T value)
    {
        std::memcpy(static_cast<uint8_t*>(object) + offset, &value, sizeof(value));
    }

    enum class InputAccess { Allowed, Blocked, Unfocused };

    struct HeldControls
    {
        uint32_t previous{};
        uint32_t held{};

        uint32_t Update(uint32_t down, InputAccess access)
        {
            // Inactive native devices report zero even if the player still holds a control.
            if (access == InputAccess::Unfocused)
                held |= previous | down;
            else if (access == InputAccess::Blocked)
                held = down;

            const auto mask = held;
            if (access == InputAccess::Allowed)
                held &= down;
            previous = down;
            return mask;
        }
    };


    template<size_t Length> void Clear(void* object, size_t offset)
    {
        std::memset(static_cast<uint8_t*>(object) + offset, 0, Length);
    }

    inline void MaskButtons(void* device, size_t offset, size_t stride, uint32_t held)
    {
        for (size_t i = 0; i < 6; ++i, offset += stride)
            Write(device, offset, Read<uint32_t>(device, offset) & ~held);
    }

    template<class T> void ClearTimers(void* device, size_t offset, uint32_t held)
    {
        for (; held; held &= held - 1)
            Write(device, offset + std::countr_zero(held) * sizeof(T), T{});
    }

    inline void FilterKeyboard(void* keyboard, InputAccess access, std::array<HeldControls, 8>& state)
    {
        for (size_t i = 0; i < state.size(); ++i)
        {
            const size_t offset = 288 + i * 4;
            const auto held = state[i].Update(Read<uint32_t>(keyboard, offset), access);
            if (access == InputAccess::Allowed && held)
            {
                MaskButtons(keyboard, offset, 32, held);
                ClearTimers<uint64_t>(keyboard, 480 + i * 32 * sizeof(uint64_t), held);
            }
        }
        if (access != InputAccess::Allowed)
            Clear<0x8C0>(keyboard, 288);
    }

    inline void FilterPad(void* manager, InputAccess access, std::array<HeldControls, 4>& state)
    {
        for (size_t i = 0; i < state.size(); ++i)
        {
            auto device = static_cast<uint8_t*>(manager) + 60 + 716 * i;
            const bool connected = Read<uint8_t>(device, 4) != 0 && Read<uint8_t>(device, 7) != 0;
            if (!connected)
                state[i] = {};
            const auto down = connected ? Read<uint32_t>(device, 348) : 0;
            const auto held = state[i].Update(down, access);
            if (access != InputAccess::Allowed)
            {
                Clear<44>(device, 348);
                Clear<48>(device, 30);
            }
            else if (held)
            {
                MaskButtons(device, 348, 4, held);
                ClearTimers<uint16_t>(device, 30, held & 0xFFFFFF);
                // The native button word also holds the deadzone-tested stick directions and triggers.
                constexpr std::array axisButtons{ 0xA00000u, 0x500000u, 0xA0000u, 0x50000u };
                for (size_t axis = 0; axis < axisButtons.size(); ++axis)
                    if (down & held & axisButtons[axis])
                        Write<int32_t>(device, 372 + axis * 4, 0);
                if (down & held & 0x800u) Write<uint8_t>(device, 388, 0);
                if (down & held & 0x400u) Write<uint8_t>(device, 389, 0);
            }
        }
    }


    inline void FilterMouse(void* mouse, InputAccess access, HeldControls& state)
    {
        const auto held = state.Update(Read<uint32_t>(mouse, 56), access);
        if (access != InputAccess::Allowed)
            Clear<100>(mouse, 44);
        else if (held)
        {
            MaskButtons(mouse, 56, 4, held);
            ClearTimers<uint64_t>(mouse, 80, held & 0xFF);
        }
    }
}
