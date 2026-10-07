#pragma once
namespace sites {
inline uintptr_t ptr_890F6D0() { return pattern.get(0, "18 00 B1 AF 25 88 80 00 02 00 04 34", -4); }
// Intro message-box state (LUI/LW pair), checked against 2 by the intro state machine.
// US 0883BC3C, EU (ULES00038) 08804000+37FB4.
inline uintptr_t ptr_883BC3C() { return pattern.get(0, "? ? ? 3C ? ? ? ? 02 00 11 34 ? ? 91 14 00 00 00 00", 0); }
// "Data was loaded" message box. The EU release looks its text up by hash.
inline uintptr_t ptr_883BB24() {
    const auto us = pattern.get(0, "20 00 BF 8F 08 00 E0 03 30 00 BD 27 ? ? ? ? 20 00 BF 8F 08 00 E0 03 30 00 BD 27 ? ? ? ? ? ? ? ? ? ? ? ? 20 00 B0 AF", -8);
    return us ? us : pattern.get(0, "25 20 00 02 01 00 05 34 25 30 40 00 25 40 00 00 ? ? ? ? ? ? E7 24 02 00 00 10", 16);
}
// "Loading data" message box, both regions.
inline uintptr_t ptr_883BBD0() { return pattern.get(0, "25 20 40 00 25 28 00 00 25 30 00 00 25 40 00 00 ? ? ? ? ? ? E7 24 03 00 00 10", 16); }
inline uintptr_t ptr_88577F8() { return pattern.get(0, "2C 00 A7 A3 38 00 B0 AF", -0); }
inline uintptr_t ptr_8856EFC() { return pattern.get(0, "74 00 18 E6 ? ? ? ? 00 00 04 AE ? ? ? ? ? ? ? ? 68 00 04 AE 7C 00 13 26", -4); }
inline uintptr_t ptr_88577C8() { return pattern.get(0, "00 00 06 34 00 00 07 34 2E 00 A6 A3", -4); }
inline uintptr_t ptr_8809724() { return pattern.get(0, "10 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? ? ? ? ? ? ? ? ? 10 00 BF AF ? ? ? ? 25 28 00 00 10 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? ? ? ? ? 1C 00 B3 AF", -8); }
inline uintptr_t ptr_88885DC() { return pattern.get(0, "01 00 06 34 10 00 B0 8F 14 00 B1 8F 18 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 14 00 B1 AF", -4); }
inline uintptr_t ptr_8888ADC() { return pattern.get(0, "01 00 06 34 80 BF 04 3C", -4); }
inline uintptr_t ptr_8888674() { return pattern.get(0, "00 00 80 44 25 28 00 00 ? ? ? ? 25 30 00 00 10 00 B0 8F 14 00 B1 8F 18 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 14 00 B1 AF ? ? ? ? ? ? ? ? 10 00 B0 AF 25 80 80 00 20 00 A4 24 78 00 85 8C", -4); }
inline uintptr_t ptr_8888508() { return pattern.get(0, "00 00 80 44 01 00 05 34 ? ? ? ? 25 30 00 00 10 00 B0 8F 14 00 B1 8F 18 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 25 28 80 00", -4); }
inline uintptr_t ptr_89DDF9C() { return pattern.get(0, "86 B6 00 46 ? ? ? ? ? ? ? ? ? ? ? ? 84 03 44 26", -0); }
inline uintptr_t ptr_89DDF6C() { return pattern.get(0, "1D 00 04 34 ? ? ? ? 25 28 20 02", -4); }
inline uintptr_t ptr_89DE278() { return pattern.get(0, "04 00 04 26 ? ? ? ? 00 00 00 00 ? ? ? ? 82 D6 00 46", -4); }
inline uintptr_t ptr_89DE240() { return pattern.get(0, "25 20 60 02 ? ? ? ? 00 00 00 00 ? ? ? ? 02 C6 00 46", -4); }
inline uintptr_t ptr_8A9C224() { return pattern.get(0, "80 4F 04 3C 00 A0 84 44", -0); }
inline uintptr_t ptr_8A9C368() { return pattern.get(0, "25 20 00 00 25 10 00 00 10 00 B4 C7", -4); }
inline uintptr_t ptr_8A9C17C() { return pattern.get(0, "10 00 B0 8F 14 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 00 00 BF AF ? ? ? ? 00 00 00 00 ? ? ? ? BB 44 04 3C", -24); }
}
