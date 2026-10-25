#pragma once
#include "NativeMenuState.hpp"
#include <cmath>

// Disconnected extension for the saved native TextCtrl scale and Base enable
// methods. Caller owns the complete traversal lease and must revalidate its
// source after each native method; disabling can invoke native selection logic.
namespace CondemnedMenuNative
{
    class ControlCalls
    {
        uintptr_t module{};
        static constexpr std::array<Prefix, 2> guards{{
            Prefix{0x12BD50, {0x83,0xEC,0x08,0x56,0x8B,0xF1,0xD9,0x46,0x28,0x8B,0x4C,0x24,0x10,0xD9,0x01,0xDA,0xE9,0xDF,0xE0,0xF6}, {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}},
            Prefix{0x133190, {0x8A,0x44,0x24,0x04,0x84,0xC0,0x88,0x41,0x41,0x75,0x13,0x8A,0x41,0x40,0x84,0xC0,0x74,0x0C,0x8B,0x01}, {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}},
        }};
    public:
        bool Bind(const Calls& calls, const View& view, void* originalRow)
        {
            module = 0;
            if (!calls.Row(originalRow) || Read<uintptr_t>(originalRow, 0) != view.module + 0x150AE0) return false;
            for (const auto& guard : guards)
            {
                const auto bytes = reinterpret_cast<const uint8_t*>(view.module + guard.rva);
                if (!Readable(bytes, guard.bytes.size())) return false;
                for (size_t i = 0; i < guard.bytes.size(); ++i)
                    if ((bytes[i] & guard.mask[i]) != (guard.bytes[i] & guard.mask[i])) return false;
            }
            module = view.module;
            return true;
        }
        bool Scale(const Calls& calls, void* row, float x, float y) const
        {
            if (!module || !calls.Row(row) || Read<uintptr_t>(row, 0) != module + 0x150AE0 ||
                !std::isfinite(x) || !std::isfinite(y) || x <= 0 || y <= 0) return false;
            float scale[2]{x, y};
            reinterpret_cast<void (__thiscall*)(void*, const float*)>(module + 0x12BD50)(row, scale);
            return true;
        }
        bool Enable(const Calls& calls, void* row, bool enabled) const
        {
            if (!module || !calls.Row(row) || Read<uintptr_t>(row, 0) != module + 0x150AE0) return false;
            reinterpret_cast<char (__thiscall*)(void*, char)>(module + 0x133190)(row, enabled ? 1 : 0);
            return true;
        }
    };
}
