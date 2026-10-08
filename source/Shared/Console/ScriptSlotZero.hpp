#pragma once
#include <cstddef>
#include <cstdint>

// The Stories games test a script handle with `0 < (handle >> 8) < poolSize`, so a car,
// object or ped created in slot 0 of its pool counts as missing: DOES_VEHICLE_EXIST and
// DOES_OBJECT_EXIST fail and the "object destroyed" check succeeds (issue 1553: balloons
// that count as burst, mission vehicles that never register, ...). Slot 0 is a valid slot,
// so these checks are widened to `0 <= index`. Include after the platform's injector.
namespace console::slot_zero {

// PSP: sra x,y,8 ; blez x,L ; ... The blez becomes bltz (same target), so only a negative
// index is rejected. The game's code has no other `index > 0` test on a handle shifted by 8
// (US images: VCS 3, LCS 1, all script handle checks); a scan finding more patches nothing.
template<size_t Max>
inline unsigned PatchPortable(uintptr_t text, size_t size) {
    uintptr_t found[Max];
    unsigned count = 0;
    const auto* code = reinterpret_cast<const volatile uint32_t*>(text);
    for (size_t i = 1; i < size / 4; ++i) {
        const uint32_t branch = code[i];
        if (branch >> 26 != 6 || ((branch >> 16) & 31) != 0) continue;                  // blez x
        const uint32_t shift = code[i - 1];
        const uintptr_t at = text + i * 4;
        if (shift >> 26 != 0 || (shift & 63) != 3 || ((shift >> 6) & 31) != 8) continue;  // sra rd, rt, 8
        if (((shift >> 11) & 31) != ((branch >> 21) & 31)) continue;                    // blez tests rd
        if (count == Max) return 0;
        found[count++] = at;
    }
    for (unsigned i = 0; i < count; ++i) {
        const uint32_t branch = *reinterpret_cast<const volatile uint32_t*>(found[i]);
        injector::WriteMemory<uint32_t>(found[i], (branch & 0x03FFFFFFu) | 0x04000000u); // bltz x
    }
    return count;
}

// PS2: sra x,x,8 ; addiu x,x,-1 ; sltiu y,x,N-1 tests 1 <= index < N. Dropping the -1 and
// raising the bound to N tests 0 <= index < N. Each site is checked before it is written.
struct Ps2Site { uintptr_t at; uint32_t compare; };
template<size_t Count>
inline unsigned PatchPs2(const Ps2Site (&sites)[Count]) {
    for (const auto& site : sites)
        if (injector::ReadMemory<uint32_t>(site.at) != 0x2442FFFFu ||
            injector::ReadMemory<uint32_t>(site.at + 4) != site.compare) return 0;
    for (const auto& site : sites) {
        injector::WriteMemory<uint32_t>(site.at, 0x24420000u);              // addiu v0, v0, 0
        injector::WriteMemory<uint32_t>(site.at + 4, site.compare + 1);    // sltiu y, v0, N
    }
    return Count;
}

}
