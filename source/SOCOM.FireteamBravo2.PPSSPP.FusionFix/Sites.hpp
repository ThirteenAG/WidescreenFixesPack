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
// Intro message-box state setter (LUI, SW at +8); first of two in US, only one in EU.
inline uintptr_t ptr_886266C() { return Required("02 00 06 34 ? ? 04 3C 08 00 E0 03 ? ? 86 AC", 4); }
inline uintptr_t ptr_88628E8() { return Required("25 38 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? ? ? ? ? ? ? ? ? 0D 00 06 34", -4); }
// Loading message box call, both regions.
inline uintptr_t ptr_8862A08() { return Required("? ? C6 24 03 00 00 10 ? ? 00 AE 02 00 04 34 ? ? 04 AE", -4); }
inline uintptr_t ptr_8B57BDC() { return Required("06 B3 00 46 0B 00 44 2E", -4); }
inline uintptr_t ptr_8815BCC() { return Required("10 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? ? ? ? ? ? ? ? ? 10 00 BF AF ? ? ? ? 25 28 00 00 10 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? ? ? ? ? 1C 00 B3 AF", -8); }
inline uintptr_t ptr_889D880() { return Required("01 00 06 34 10 00 B0 8F 14 00 B1 8F 18 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 14 00 B1 AF", -4); }
inline uintptr_t ptr_889DD48() { return Required("01 00 06 34 80 BF 04 3C", -4); }
inline uintptr_t ptr_889D918() { return Required("00 00 80 44 25 28 00 00 ? ? ? ? 25 30 00 00 10 00 B0 8F 14 00 B1 8F 18 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 14 00 B1 AF ? ? ? ? ? ? ? ? 10 00 B0 AF 25 80 80 00 20 00 A4 24 78 00 85 8C", -4); }
inline uintptr_t ptr_889D7AC() { return Required("00 00 80 44 01 00 05 34 ? ? ? ? 25 30 00 00 10 00 B0 8F 14 00 B1 8F 18 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 25 28 80 00", -4); }
inline uintptr_t ptr_8A41C84() { return Required("06 B7 00 46 ? ? ? ? ? ? ? ? ? ? ? ? 80 03 44 26", -0); }
inline uintptr_t ptr_8A41C54() { return Required("1D 00 04 34 ? ? ? ? 25 28 20 02", -4); }
inline uintptr_t ptr_8B578EC() { return Required("80 4F 04 3C 00 60 84 44 ? ? ? ? ? ? ? ? 00 D0 80 44", 0); }
inline uintptr_t ptr_8B57C98() { return Required("25 20 00 00 25 10 00 00 24 00 B4 C7", -4); }
inline uintptr_t ptr_8B57834() { return Required("00 00 00 00 ? ? ? ? ? ? ? ? FF FF 04 24 ? ? ? ? 10 00 B0 8F", -12); }
inline uintptr_t ptr_88041F8() { return Required("25 40 00 00 34 00 A2 AF", -4); }
}
