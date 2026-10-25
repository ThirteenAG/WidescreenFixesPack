#pragma once
#include <Windows.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include "Hooking.Patterns.h"

namespace TCCamera
{
    inline std::atomic<float> factor{ 1.0f };
    inline bool ready = false;
    inline uintptr_t imageBase = 0x400000;

    inline uintptr_t Address(uintptr_t address) { return imageBase + (address - 0x400000); }

    inline bool Readable(const void* pointer, size_t size)
    {
        auto address = reinterpret_cast<uintptr_t>(pointer);
        if (!address || !size || size > std::numeric_limits<uintptr_t>::max() - address)
            return false;
        const auto end = address + size;
        while (address < end)
        {
            MEMORY_BASIC_INFORMATION region{};
            if (!VirtualQuery(reinterpret_cast<const void*>(address), &region, sizeof(region)) ||
                region.State != MEM_COMMIT || (region.Protect & (PAGE_GUARD | PAGE_NOACCESS)))
                return false;
            const DWORD protection = region.Protect & 0xFF;
            if (protection != PAGE_READONLY && protection != PAGE_READWRITE && protection != PAGE_WRITECOPY &&
                protection != PAGE_EXECUTE_READ && protection != PAGE_EXECUTE_READWRITE && protection != PAGE_EXECUTE_WRITECOPY)
                return false;
            const auto begin = reinterpret_cast<uintptr_t>(region.BaseAddress);
            if (region.RegionSize > std::numeric_limits<uintptr_t>::max() - begin)
                return false;
            const auto next = begin + region.RegionSize;
            if (next <= address)
                return false;
            address = next;
        }
        return true;
    }

    template<class T> T Read(const void* pointer, size_t offset)
    {
        T value{};
        std::memcpy(&value, static_cast<const uint8_t*>(pointer) + offset, sizeof(value));
        return value;
    }

    inline void SetFactor(float value)
    {
        factor.store(std::isfinite(value) ? std::clamp(value, 0.5f, 2.0f) : 1.0f, std::memory_order_relaxed);
    }

    inline bool Gameplay(const void* world, const void* stack, const void* player, bool blending)
    {
        if (blending || !Readable(world, 10869) || !Readable(stack, 44) || !Readable(player, 52) ||
            Read<int>(stack, 40) != 0 || Read<uint8_t>(world, 10868))
            return false;

        auto camera = Read<const void*>(world, 204);
        if (!Readable(camera, 432) || Read<const void*>(stack, 0) != camera ||
            (Read<uint8_t>(camera, 31) & 0x80) || Read<uintptr_t>(camera, 0) != Address(0x6CEB04) ||
            Read<uintptr_t>(camera, 24) != Address(0x41D5E0) || Read<uint8_t>(camera, 144) != 10 ||
            Read<uint8_t>(camera, 428) != 1 || Read<uint8_t>(camera, 409))
            return false;

        const auto mode = Read<uint8_t>(camera, 145);
        if (mode != 0 && mode != 13)
            return false;
        auto handler = Read<const void*>(camera, 404);
        if (!Readable(handler, mode == 13 ? 672 : 56) || Read<const void*>(camera, 308 + 4 * mode) != handler ||
            Read<const void*>(handler, 52) != camera ||
            Read<uintptr_t>(handler, 0) != Address(mode == 0 ? 0x6CDA48u : 0x6CD9C8u))
            return false;

        const auto target = Read<const void*>(camera, 412);
        if (mode == 0)
            return target == player;

        const auto vehicle = Read<const void*>(player, 48);
        // Vehicle entry has its own blend, in addition to the common camera blend.
        return Readable(vehicle, 145) && target == vehicle && Read<uint8_t>(vehicle, 144) == 2 &&
            Read<const void*>(handler, 584) == vehicle && !Read<uint8_t>(handler, 657) &&
            Read<float>(handler, 668) == 1.0f;
    }

    inline float Scale(float nativeScale, float verticalCoefficient, float multiplier)
    {
        if (!std::isfinite(nativeScale) || nativeScale <= 0.0f || !std::isfinite(verticalCoefficient) ||
            verticalCoefficient <= 0.0f || !std::isfinite(multiplier) || multiplier == 1.0f)
            return nativeScale;

        constexpr double pi = 3.14159265358979323846;
        const double halfAngle = std::atan(1.0 / (double(nativeScale) * verticalCoefficient));
        const double adjusted = std::clamp(halfAngle * std::clamp(double(multiplier), 0.5, 2.0), pi / 360.0, pi * 179.0 / 360.0);
        const double scale = 1.0 / (std::tan(adjusted) * verticalCoefficient);
        return std::isfinite(scale) && scale > 0.0 && scale <= std::numeric_limits<float>::max() ? float(scale) : nativeScale;
    }

    inline bool Init(HMODULE game = GetModuleHandleW(nullptr))
    {
        ready = false;
        imageBase = reinterpret_cast<uintptr_t>(game);
        // Verify this executable's camera contract independently of the generic Hor+ hook.
        if (!Readable(game, 0x1000) || !Readable(reinterpret_cast<const void*>(Address(0x79430C)), 4) ||
            !Readable(reinterpret_cast<const void*>(Address(0x778D38)), 44) ||
            !Readable(reinterpret_cast<const void*>(Address(0x7AEE5C)), 4) ||
            !Readable(reinterpret_cast<const void*>(Address(0x778B00)), 1))
            return false;
        auto getter = hook::pattern(game, "8B 81 CC 00 00 00 85 C0 74 08 F6 40 1F 80 74 02 33 C0 C3");
        auto creator = hook::pattern(game, "55 8B EC 83 E4 F0 6A FF 68 ? ? ? ? 64 A1 00 00 00 00");
        auto vehicle = hook::pattern(game, "55 8B EC 83 E4 F0 81 EC AC 00 00 00 83 7D 08 06 56 8B F1");
        // The shared SEH prologue appears in other functions; require its exact camera RVA.
        bool foundCreator = false;
        for (size_t i = 0; i < creator.size(); ++i)
            foundCreator |= reinterpret_cast<uintptr_t>(creator.get(i).get<void>()) == Address(0x41E0F0);
        const auto operand = [](uintptr_t address, uintptr_t expected)
        {
            const auto pointer = reinterpret_cast<const void*>(Address(address));
            return Readable(pointer, 4) && Read<uintptr_t>(pointer, 0) == Address(expected);
        };
        ready = getter.size() == 1 && foundCreator && vehicle.size() == 1 &&
            reinterpret_cast<uintptr_t>(getter.get_first()) == Address(0x46E6C0) &&
            reinterpret_cast<uintptr_t>(vehicle.get_first()) == Address(0x415B40) &&
            operand(0x41E141, 0x778D38) && operand(0x415CFA, 0x7AEE5C) &&
            operand(0x415271, 0x778B00) && operand(0x49BA98, 0x7A0390) && operand(0x49BA3E, 0x79430C);
        return ready;
    }

    inline float Apply(const void* projection, uintptr_t caller, float nativeScale, bool blocked)
    {
        const float multiplier = factor.load(std::memory_order_relaxed);
        if (multiplier == 1.0f || !ready || blocked || reinterpret_cast<uintptr_t>(projection) != Address(0x7A0390) ||
            caller != Address(0x49BAA1) || !Readable(projection, 20))
            return nativeScale;

        const auto world = Read<const void*>(reinterpret_cast<const void*>(Address(0x79430C)), 0);
        const auto stack = reinterpret_cast<const void*>(Address(0x778D38));
        const auto player = Read<const void*>(reinterpret_cast<const void*>(Address(0x7AEE5C)), 0);
        const bool blending = Read<uint8_t>(reinterpret_cast<const void*>(Address(0x778B00)), 0) != 0;
        return Gameplay(world, stack, player, blending) ? Scale(nativeScale, Read<float>(projection, 16), multiplier) : nativeScale;
    }
}
