#pragma once
#include "../Shared/Console/PortableLights.hpp"
namespace vcsfx {
inline constexpr uintptr_t poolPointers[][2] = {
    {0x08981174, 0x08981180},
    {0x089812D4, 0x089812DC},
    {0x089813DC, 0x089813EC},
    {0x08982410, 0x0898242C},
    {0x08982D94, 0x08982DD8},
    {0x08983CB0, 0x08983CC0},
};
inline constexpr uintptr_t poolLimits[] = {0x0898122C, 0x089812FC, 0x08981424, 0x08981428, 0x08982AF0, 0x08982D08, 0x08983218, 0x08983CF0, 0x08983CBC, 0x08983D44};
inline constexpr uintptr_t trafficCalls[] = {0x08A1003C, 0x08A100EC, 0x08A10638, 0x08A11020, 0x08A110CC};
}
