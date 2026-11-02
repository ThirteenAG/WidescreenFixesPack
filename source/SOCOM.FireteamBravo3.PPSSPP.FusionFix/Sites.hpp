#pragma once
#include <cstdlib>
namespace sites {
inline uintptr_t Required(const char* signature, int offset = 0)
{
    const auto address = pattern.get_first(signature, offset);
    if (!address)
        std::abort();
    return address;
}
inline uintptr_t CameraActivityFirst() { return Required("00 00 00 00 01 00 08 34 25 38 E8 00 2B 38 07 00 00 00 C7 A0 1C 01 AC C4 49 00 06 26 00 00 C7 90", 8); }
inline uintptr_t CameraActivitySecond() { return Required("00 00 00 00 01 00 08 34 25 38 E8 00 2B 38 07 00 00 00 C7 A0 1C 01 AC C4 4A 00 06 26 00 00 C7 90", 8); }
inline uintptr_t CameraActivityIdle() { return Required("01 00 08 34 24 28 E8 00 00 00 C5 A0 02 00 05 34 10 00 85 14 00 00 00 00 38 01 B6 E7 30 03 2C C6", 4); }
inline uintptr_t Projection() { return Required("E1 3F 05 3C C0 00 0C C6 E2 E1 A5 34 C4 00 0E C6 00 68 85 44", 20); }
inline uintptr_t Movie() { return Required("70 3F 08 3C 06 B3 00 46 00 70 88 44 25 20 00 00 08 3F 08 3C", -48); }
inline uintptr_t ptr_88950F4() { return Required("18 00 04 8E ? ? ? ? 01 00 06 34 ? ? ? ? ? ? ? ? ? ? ? ? 18 00 04 8E", -8); }
inline uintptr_t ptr_8895108() { return Required("18 00 04 8E 01 00 06 34 ? ? ? ? ? ? ? ? ? ? ? ? 00 00 00 00", -12); }
inline uintptr_t ptr_889511C() { return Required("00 00 00 00 ? ? ? ? 18 00 04 8E ? ? ? ? 01 00 06 34 ? ? ? ? ? ? ? ? 18 00 04 8E ? ? ? ? 01 00 06 34 ? ? ? ? ? ? ? ? 80 3F 04 3C", -12); }
inline uintptr_t ptr_889513C() { return Required("18 00 04 8E ? ? ? ? 01 00 06 34 ? ? ? ? ? ? ? ? 80 3F 04 3C", -8); }
inline uintptr_t ptr_8A7A6F8() { return Required("01 00 05 34 04 00 A5 8F 10 00 A4 30", -4); }
inline uintptr_t ptr_8A7A834() { return Required("08 00 A4 93 00 00 4C E6", -4); }
inline uintptr_t ptr_89D9978() { return Required("25 28 A0 03 25 20 00 02 06 A3 00 46", -4); }
inline uintptr_t ptr_89DBB7C() { return Required("25 28 A0 03 04 00 05 8E", -4); }
inline uintptr_t ptr_89DAA64() { return Required("25 28 A0 03 ? ? ? ? 00 00 00 00 04 00 04 8E", -4); }
inline uintptr_t ptr_89DB5BC() { return Required("25 28 00 02 03 00 15 34", -4); }
inline uintptr_t ptr_89DB5D4() { return Required("25 28 00 02 00 00 15 34", -4); }
inline uintptr_t ptr_89DBD4C() { return Required("25 40 40 00 ? ? ? ? 25 20 40 02", -4); }
inline uintptr_t ptr_88877D4() { return Required("00 00 BA E7 25 20 00 02", -4); }
inline uintptr_t ptr_88877F0() { return Required("00 00 BA C7 01 00 04 34", -0); }
inline uintptr_t ptr_8887758() { return Required("06 A3 00 46 38 04 11 26", -4); }
inline uintptr_t ptr_888773C() { return Required("20 01 16 26", -8); }
inline uintptr_t ptr_888774C() { return Required("25 20 C0 02 25 20 C0 02 ? ? ? ? 06 A3 00 46", -4); }
inline uintptr_t ptr_89DAA84() { return Required("87 65 00 46", -4); }
inline uintptr_t ptr_89DAA90() { return Required("31 00 84 90 ? ? ? ? 02 A5 00 46", -8); }
inline uintptr_t ptr_881AB4C() { return Required("01 00 05 34 ? ? ? ? 25 20 40 00 ? ? ? ? ? ? ? ? ? ? ? ? 25 28 00 00", -4); }
inline uintptr_t ptr_881AC38() { return Required("01 00 05 34 ? ? ? ? 25 20 40 00 00 00 BF 8F", -4); }
inline uintptr_t ptr_881AD1C() { return Required("25 28 00 00 ? ? ? ? 25 20 40 00 ? ? ? ? ? ? ? ? ? ? ? ? 01 00 14 34", -4); }
inline uintptr_t ptr_880438C() { return Required("25 40 00 00 ? ? ? ? 28 00 A2 AF", -4); }
}
