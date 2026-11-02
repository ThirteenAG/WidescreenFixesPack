#pragma once
#include "../../external/injector/include/ps2/runtime.hpp"
extern "C"
{
#include "../../external/injector/include/ps2/patterns.h"
}

namespace tcny::sites
{
    inline uintptr_t at_100350() { return pattern.get(0, "2D 20 40 02 ? ? ? ? 2D 40 20 02", -4); }
    // Start-up program: textured sprite writer, copyright picture upload, MPEG picture decode (RGBA output).
    inline uintptr_t at_104C90() { return pattern.get(0, "50 FF BD 27 2D 68 C0 00 B0 00 A2 8F A0 00 B0 7F 90 00 B1 7F 2D 80 80 00", 0); }
    inline uintptr_t at_105948() { return pattern.get(0, "2D 20 40 02 2D 28 00 00 02 00 07 96 04 00 0C 96 3F 00 E2 24 2D 58 E0 00", 0); }
    inline uintptr_t at_111F08() { return pattern.get(0, "F0 FF BD 27 FF 0F 02 3C 00 00 BF FF FF FF 42 34 24 28 A2 00 00 20 03 3C 40 00 87 8C 25 28 A3 00 01 00 02 24 C4 00 E2 AC", 0); }
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

    // Cinematic bars: their draw function (entry), the bar height, the end of the drawing.
    inline uintptr_t at_20B4C0() { return pattern.get(0, "80 FF BD 27 01 00 02 24 50 00 B1 7F 60 00 B0 7F 2D 88 80 00 40 00 BF FF 78 00 B5 E7 70 00 B4 E7", 0); }
    inline uintptr_t at_20B66C() { return pattern.get(0, "70 3E 01 3C F1 F0 21 34 00 08 81 44 00 00 00 00 02 00 01 46 64 00 00 46 00 08 09 44", 0); }
    inline uintptr_t at_20B780() { return pattern.get(0, "64 00 00 46 00 08 06 44 ? ? ? 0C FF FF 4A 35 50 00 04 3C ? ? 84 8C", 0); }
    // Bars shown at once (cinematic start): bar height, the world pointer.
    inline uintptr_t at_20B430() { return pattern.get(0, "F0 FF BD 27 88 42 01 3C 00 00 81 44 00 00 BF FF 14 00 80 AC 18 00 80 E4 50 00 04 3C ? ? 84 8C", 0); }
    // Cinematic start: sets the world's cinematic flag.
    inline uintptr_t at_1F1318() { return pattern.get(0, "2D 90 A0 00 F0 00 BF FF ? ? 02 AE ? ? ? 0C ? ? 04 8E", 0); }
    // Game camera constructor: its update function.
    inline uintptr_t at_16B4D8() { return pattern.get(0, "CC 3F 01 3C CD CC 21 34 00 00 81 44 ? ? 03 3C 04 00 26 8E ? ? ? ? ? ? 63 24 0A 00 07 24", 0); }
    // Screen setup: the 2D manager (lui s0, hi ... addiu s0, s0, lo).
    inline uintptr_t at_204E1C() { return pattern.get(0, "? ? 10 3C 00 A0 82 44 00 00 00 00 20 A5 80 46 ? ? ? 0C ? ? 10 26", 0); }
    // World camera projection in the frame's world rendering.
    inline uintptr_t at_4A1924() { return pattern.get(0, "FA 44 01 3C 00 68 81 44 ? ? ? 0C 2D 20 20 02 ? ? 0C C6 ? ? ? 0C 2D 20 20 02", 0); }
} // namespace tcny::sites
