#pragma once
#include "../../external/injector/include/ps2/runtime.hpp"
extern "C" {
#include "../../external/injector/include/ps2/patterns.h"
}

namespace tcny::sites {
inline uintptr_t at_100350() { return pattern.get(0, "2D 20 40 02 ? ? ? ? 2D 40 20 02", -4); }
inline uintptr_t at_1FD788() { return pattern.get(0, "C0 26 05 8E ? ? ? ? BC 26 02 8E 09 F8 40 00", -8); }
inline uintptr_t at_1FE0C4() { return pattern.get(0, "C0 26 00 AE ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? C3 01 03 92 ? ? ? ? BC 00 02 8E", -20); }
inline uintptr_t at_4A1C60() { return pattern.get(0, "00 00 00 00 ? ? ? ? ? ? ? ? ? ? ? ? 23 10 46 00 2A 18 62 02", -12); }
inline uintptr_t at_4A1CC0() { return pattern.get(0, "10 00 A3 FB ? ? 12 0C 02 00 04 24", 4); }
inline uintptr_t at_204DB0() { return pattern.get(0, "CC 3A 01 3C CD CC 21 34 00 00 81 44 ? ? ? ? 03 01 02 24", 0); }
inline uintptr_t at_204EBC() { return pattern.get(0, "2B 28 05 00 20 00 B0 7F 10 00 B1 7F", 0); }
inline uintptr_t at_204DD8() { return pattern.get(0, "CD CC 21 34 00 00 81 44 ? ? ? ? ? ? ? ? ? ? ? ? 9A 99 21 34", -4); }
inline uintptr_t at_2346E8() { return pattern.get(0, "00 00 85 44 00 00 00 00 20 00 80 46 ? ? ? ? 00 08 81 44 ? ? ? ? CD CC 21 34", 0); }
inline uintptr_t at_23C638() { return pattern.get(0, "00 00 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 09 00 C9 2C", -8); }
inline uintptr_t at_20BA48() { return pattern.get(0, "00 20 81 44 ? ? ? ? CD CC 21 34 00 08 81 44", 0); }
inline uintptr_t at_255E88() { return pattern.get(0, "15 26 A5 34 02 00 01 46 ? ? ? ? 44 00 40 E6", 0); }
inline uintptr_t at_265744() { return pattern.get(0, "24 01 00 46 00 20 02 44 50 00 A2 A7", -4); }
inline uintptr_t at_265224() { return pattern.get(0, "00 00 81 44 00 10 A2 48 00 00 03 44 00 18 A3 48 44 10 43 4A", -12); }
inline uintptr_t at_267F74() { return pattern.get(0, "00 08 A2 48 ? ? ? ? 00 00 81 44 00 00 00 00 00 00 02 44", -8); }
inline uintptr_t at_268094() { return pattern.get(0, "00 08 81 44 00 10 03 44 00 10 A7 48", -4); }
inline uintptr_t at_2517A8() { return pattern.get(0, "20 02 B6 7F 10 02 B7 7F ? ? ? ? 80 02 B0 7F", -12); }
inline uintptr_t at_2478FC() { return pattern.get(0, "A4 00 00 46 00 10 02 44 24 08 00 46 00 00 03 44 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? FF FF 21 34", -4); }
inline uintptr_t at_2519D0() { return pattern.get(0, "00 39 07 00 ? ? ? ? ? ? ? ? 00 00 81 44 C0 01 A8 AF", 0); }
}
