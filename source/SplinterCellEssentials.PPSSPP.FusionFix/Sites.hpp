#pragma once
#include "../Shared/Console/PSP.hpp"
namespace essentials::sites {
inline uintptr_t Unique(const char* signature, int offset = 0) {
    const auto first = pattern.get_first(signature, offset);
    return first && !pattern.get(1, signature, offset) ? first : 0;
}
// DirectAxis, original reference 089B4924.
inline uintptr_t DirectAxis() { return pattern.get_first("70 FF BD 27 68 00 B0 AF 25 80 80 00 00 21 05 00 21 20 04 02 B0 02 84 24 78 00 B4 AF 04 00 85 8C", 0); }
// GroundSpeed, original reference 088EA548.
inline uintptr_t GroundSpeed() { return Unique("E0 FF BD 27 10 00 B4 E7 14 00 B0 AF 18 00 BF AF 25 80 80 00 ?? ?? 0C C6 02 63 0C 46 ?? ?? 0D C6"); }
// CameraConstructor, original reference 089F3FC8.
inline uintptr_t CameraConstructor() { return pattern.get_first("B0 FE BD 27 44 01 B0 AF 00 00 E9 8C 25 80 80 00 48 01 B1 AF 25 88 C0 00 04 00 E4 8C 08 00 E6 8C", 0); }
// DrawTile, original reference 089FB9E4.
inline uintptr_t DrawTile() { return pattern.get_first("80 FF BD 27 80 00 A0 C7 D8 00 87 8C 84 00 A6 AF 86 70 00 46 38 00 A0 E7 34 00 A2 E7 06 78 00 46", 0); }
// MainLoop, original reference 08805BC4.
inline uintptr_t MainLoop() { return pattern.get_first("A0 FB BD 27 38 04 B0 AF 25 80 80 00 01 00 04 34 ? ? 05 3C 3C 04 B1 AF 40 04 B2 AF ? ? A4 AC", 0); }
// GameDraw, original reference 089AD9D4.
inline uintptr_t GameDraw() { return pattern.get_first("C0 FA BD 27 0C 05 B0 AF 25 80 80 00 ? ? 04 3C ? ? 84 8C 10 05 B1 AF 14 05 B2 AF 18 05 B3 AF", 0); }
// WorldToScreen, original reference 089B6F64.
inline uintptr_t WorldToScreen() { return pattern.get_first("F0 FB BD 27 E8 03 B3 AF 25 98 A0 00 F4 03 B6 AF 0C 00 76 26 00 00 C5 8E 08 00 67 8E 01 00 A8 24", 0); }
// ScreenToWorld, original reference 089B6B68.
inline uintptr_t ScreenToWorld() { return pattern.get_first("D0 FB BD 27 08 04 B3 AF 25 98 A0 00 14 04 B6 AF 0C 00 76 26 00 00 C5 8E 08 00 67 8E 01 00 A8 24", 0); }
// VectorWorldToScreen, original reference 089B7604.
inline uintptr_t VectorWorldToScreen() {
    const auto disc = Unique("40 FC BD 27 B0 03 B6 AF B8 03 BE AF 25 B0 A0 00 30 00 9E 8C B4 03 B7 AF 25 28 C0 00 30 00 B7 27");
    return disc ? disc : Unique("40 FC BD 27 B8 03 BE AF 30 00 9E 8C 9C 03 B1 AF A4 03 B3 AF B0 03 B6 AF B4 03 B7 AF 30 00 B7 27");
}
// The PSN build changes object layouts and instruction scheduling, not just addresses.
inline bool IsPsn(uintptr_t ground) { return injector::ReadMemory<uint16_t>(ground + 0x14) == 0x50C; }
inline uintptr_t ScreenTransform(uintptr_t screen) {
    return screen ? range_pattern.get_first(screen, 0x400,
        "D0 03 A4 27 30 03 A5 27 ?? ?? ?? ?? 25 30 C0 03", 0) : 0;
}
// APlayerController::Tick rescales the filtered walk axes to fixed walk/run
// magnitudes (0, walk percentage or 1). Original reference 089C36B8 (both
// executables; the stored field offsets differ).
inline uintptr_t WalkQuantization() { return Unique("83 83 16 46 C2 93 0E 46 82 9B 0E 46 ?? ?? 0F E6 ?? ?? 0E E6"); }
// SoundBar, original reference 089FB690.
inline uintptr_t SoundBar() { return pattern.get_first("40 FF BD 27 4D 6B 00 46 25 30 A0 00 98 00 B2 AF 25 90 80 00 25 28 00 01 03 00 A8 28 00 68 04 44", 0); }
}
