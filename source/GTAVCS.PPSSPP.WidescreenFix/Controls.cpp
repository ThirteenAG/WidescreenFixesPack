#include "Game.hpp"
#include <cstddef>
#include <cstdint>
using namespace mips_asm;
namespace vcsws {
namespace {
struct CControllerState {
    int16_t LEFTSTICKX, LEFTSTICKY, RIGHTSTICKX, RIGHTSTICKY;
    int16_t LEFTSHOULDER1, LEFTSHOULDER2, RIGHTSHOULDER1, RIGHTSHOULDER2;
    int16_t DPADUP, DPADDOWN, DPADLEFT, DPADRIGHT, unused[4];
    int16_t START, SELECT, SQUARE, TRIANGLE, CROSS, CIRCLE, LEFTSHOCK, RIGHTSHOCK;
};
struct CPad {
    int16_t index;
    CControllerState NewState;
    int16_t alignment;
    CControllerState OldState;
    uint8_t reserved[50];
    int16_t Mode, ShakeDur, DisablePlayerControls;
};
static_assert(sizeof(CControllerState) == 48 && offsetof(CPad, OldState) == 52);
uintptr_t (*FindPlayerPed)(){};
uintptr_t (*FindPlayerVehicle)(){};
uintptr_t (*WeaponInfo)(int){};
uintptr_t reloadWeapon;
SafetyMipsInline lookBehind;
bool Reloading() {
    auto ped = FindPlayerPed();
    if (!ped) { reloadWeapon = 0; return false; }
    uintptr_t current = ped + 0x574 + uint8_t(*reinterpret_cast<uint8_t*>(ped + 0x789)) * 28;
    if (reloadWeapon != current || *reinterpret_cast<int*>(current + 8) != 2) {
        reloadWeapon = 0; return false;
    }
    return true;
}
void Reload(CPad* pad) {
    if (!pad->NewState.LEFTSHOULDER1 || !pad->NewState.SQUARE || pad->OldState.SQUARE || Reloading()) return;
    auto ped = FindPlayerPed();
    if (!ped) return;
    unsigned slot = *reinterpret_cast<uint8_t*>(ped + 0x789);
    if (slot < 3 || slot > 8) return;
    uintptr_t weapon = ped + 0x574 + slot * 28;
    auto* state = reinterpret_cast<int*>(weapon + 8);
    auto* clip = reinterpret_cast<int*>(weapon + 12);
    auto* total = reinterpret_cast<int*>(weapon + 16);
    if (*total <= 0 || *clip >= *total || *state == 2) return;
    uintptr_t info = WeaponInfo(*reinterpret_cast<int*>(weapon + 4));
    if (!info || *clip >= *reinterpret_cast<int*>(info + 20)) return;
    unsigned duration = *reinterpret_cast<unsigned*>(info + 16);
    if (duration < 100 || duration > 10000) duration = 1000;
    // Enter the native reload state. CWeapon::Update owns animation completion,
    // clip refilling and reload audio; no code or controller state is rewritten.
    *state = 2;
    *reinterpret_cast<unsigned*>(weapon + 20) = *reinterpret_cast<unsigned*>(Address<0x8BB3B4C>()) + duration;
    reloadWeapon = weapon;
}
int16_t Held(CPad* pad, int16_t value) { return pad->DisablePlayerControls ? 0 : value; }
int16_t CPad__GetAccelerate(CPad* pad) { return Held(pad,pad->NewState.RIGHTSHOULDER1); }
int16_t CPad__GetAccelerateNormal(CPad* pad) { return Held(pad,pad->NewState.CROSS); }
int16_t CPad__GetBrake(CPad* pad) { return Held(pad,pad->NewState.LEFTSHOULDER1); }
int16_t CPad__GetBrakeNormal(CPad* pad) { return Held(pad,pad->NewState.SQUARE); }
int16_t CPad__GetAccelerateBovver(CPad* pad) {
    auto vehicle=FindPlayerVehicle();
    return vehicle && *reinterpret_cast<int16_t*>(vehicle+0x56)==198 ? CPad__GetAccelerate(pad) : CPad__GetAccelerateNormal(pad);
}
int16_t CPad__GetBrakeBovver(CPad* pad) {
    auto vehicle=FindPlayerVehicle();
    return vehicle && *reinterpret_cast<int16_t*>(vehicle+0x56)==198 ? CPad__GetBrake(pad) : CPad__GetBrakeNormal(pad);
}
int16_t CPad__GetRightStickX(CPad* pad) { return Held(pad,pad->NewState.RIGHTSTICKX); }
int16_t CPad__GetRightStickY(CPad* pad) { return Held(pad,pad->NewState.RIGHTSTICKY); }
int16_t Aim(CPad* pad, int16_t left, int16_t right) {
    int value=left ? left : right;
    if (!pad->NewState.CROSS && !pad->NewState.DPADDOWN) value *= 2;
    return Held(pad,int16_t(value));
}
int16_t aimX(CPad* pad) { return Aim(pad,pad->NewState.LEFTSTICKX,pad->NewState.RIGHTSTICKX); }
int16_t aimY(CPad* pad) { return Aim(pad,pad->NewState.LEFTSTICKY,pad->NewState.RIGHTSTICKY); }
int16_t CPad__GetTarget(CPad* pad) {
    if (pad->DisablePlayerControls) return 0;
    Reload(pad); return pad->NewState.LEFTSHOULDER1 != 0;
}
int16_t CPad__JumpJustDown(CPad* pad) {
    return Held(pad,pad->NewState.SQUARE && !pad->OldState.SQUARE && !CPad__GetTarget(pad));
}
int16_t CPad__TargetJustDown(CPad* pad) { return Held(pad,pad->NewState.LEFTSHOULDER1 && !pad->OldState.LEFTSHOULDER1); }
int16_t CPad__GetWeapon(CPad* pad) { return Held(pad,Reloading() ? 1 : pad->NewState.RIGHTSHOULDER1); }
int16_t CPad__WeaponJustDown(CPad* pad) { return Held(pad,pad->NewState.RIGHTSHOULDER1 && !pad->OldState.RIGHTSHOULDER1); }
int16_t CPad__GetLookBehindForPed(CPad* pad) { return Held(pad,pad->NewState.CIRCLE != 0); }
int16_t CPad__EnterFreeAim(CPad* pad) { return Held(pad,pad->NewState.LEFTSHOULDER1 && pad->NewState.DPADDOWN); }
}
void InstallControls() {
    FindPlayerPed = reinterpret_cast<uintptr_t (*)()>(Address<0x8960424>());
    FindPlayerVehicle = reinterpret_cast<uintptr_t (*)()>(Address<0x89602c8>());
    WeaponInfo = reinterpret_cast<uintptr_t (*)(int)>(Address<0x8b1fd70>());
    bool DualAnalogPatch=settings.dualAnalog, ModernControlScheme=settings.modernControls;
    if (DualAnalogPatch)
    {
        injector::WriteInstr(Address<0x08A1A3F0>(),
            sh(a1, sp, 0)
        );
        injector::WriteInstr(Address<0x08A1A3E8>(),
            lhu(a1, sp, 0xE)
        );
        injector::MakeNOP(Address<0x0898E098>());
        injector::WriteMemory<uint32_t>(Address<0x0898E0A8>(), (injector::ReadMemory<uint32_t>(Address<0x0898E0A8>()) & 0xFFFFu) | 0x10000000u);
        injector::MakeCALL(Address<0x0898E124>(), CPad__GetRightStickX);
        injector::MakeNOP(Address<0x0898E1A0>());
        injector::WriteMemory<uint32_t>(Address<0x0898E1B0>(), (injector::ReadMemory<uint32_t>(Address<0x0898E1B0>()) & 0xFFFFu) | 0x10000000u);
        injector::MakeCALL(Address<0x0898E1DC>(), CPad__GetRightStickY);
        injector::MakeCALL(Address<0x0898DE90>(), aimX);
        injector::MakeCALL(Address<0x0898DEBC>(), aimX);
        injector::MakeCALL(Address<0x0898DECC>(), aimX);
        injector::MakeCALL(Address<0x0898DF98>(), aimY);
        injector::MakeCALL(Address<0x0898DFFC>(), aimY);
        injector::MakeNOP(Address<0x0898C518>());
        injector::MakeNOP(Address<0x0898C5A4>());
        injector::MakeNOP(Address<0x0899F96C>());
    }
    if (ModernControlScheme)
    {
        injector::MakeJMP(Address<0x898d2d8>(), CPad__GetAccelerate);
        injector::MakeCALL(Address<0x089D9E30>(), CPad__GetAccelerateBovver);
        injector::MakeCALL(Address<0x089DA1E4>(), CPad__GetAccelerateNormal);
        injector::MakeCALL(Address<0x089EF2B0>(), CPad__GetAccelerateNormal);
        injector::WriteMemory<uint16_t>(Address<0x0898D08C>(), offsetof(struct CPad, NewState.RIGHTSHOULDER1));
        injector::WriteMemory<uint16_t>(Address<0x0898D0D4>(), offsetof(struct CPad, NewState.RIGHTSHOULDER1));
        injector::WriteMemory<uint16_t>(Address<0x0898D0EC>(), offsetof(struct CPad, NewState.CROSS));
        injector::MakeJMP(Address<0x898d140>(), CPad__GetBrake);
        injector::WriteMemory<uint16_t>(Address<0x898dd60>(), offsetof(struct CPad, NewState.SQUARE));
        injector::WriteMemory<uint16_t>(Address<0x898c90c>(), -110);
        injector::WriteMemory<uint16_t>(Address<0x898cb60>(), 110);
        injector::MakeInlineWithNOP(Address<0x898c86c>(),
            lh(a0, s0, offsetof(struct CPad, NewState.SQUARE)),
            lh(k0, s0, offsetof(struct CPad, NewState.CIRCLE)),
            _or(a0, a0, k0),
            beq(a0, zero, 2),
            nop(),
            j(Address<0x898c904>()),
            nop()
        );
        injector::MakeInlineWithNOP(Address<0x898cac0>(),
            lh(a0, s0, offsetof(struct CPad, NewState.SQUARE)),
            lh(k0, s0, offsetof(struct CPad, NewState.CIRCLE)),
            _or(a0, a0, k0),
            beq(a0, zero, 2),
            nop(),
            j(Address<0x898cb58>()),
            nop()
        );
        injector::MakeInlineWithNOP(Address<0x898cdac>(),
            lh(a0, s0, offsetof(struct CPad, NewState.SQUARE)),
            bne(a0, zero, 2),
            nop(),
            j(Address<0x898cdfc>()),
            nop()
        );
        injector::WriteMemory<uint16_t>(Address<0x898dbb0>(), offsetof(struct CPad, NewState.SQUARE));
        injector::WriteMemory<uint16_t>(Address<0x898f560>(), offsetof(struct CPad, NewState.SQUARE));
        injector::WriteMemory<uint16_t>(Address<0x898b918>(), offsetof(struct CPad, NewState.LEFTSHOULDER1));
        injector::WriteMemory<uint16_t>(Address<0x898eaac>(), offsetof(struct CPad, NewState.LEFTSHOULDER1));
        injector::MakeCALL(Address<0x089D9E44>(), CPad__GetBrakeBovver);
        injector::MakeCALL(Address<0x089DA10C>(), CPad__GetBrakeNormal);
        injector::MakeCALL(Address<0x089DA144>(), CPad__GetBrakeNormal);
        injector::MakeCALL(Address<0x089DA174>(), CPad__GetBrakeNormal);
        injector::MakeCALL(Address<0x089DA1C8>(), CPad__GetBrakeNormal);
        injector::MakeCALL(Address<0x89ef2bc>(), CPad__GetBrakeNormal);
        injector::MakeJMP(Address<0x898d560>(), CPad__GetTarget);
        injector::MakeJMP(Address<0x898d5bc>(), CPad__TargetJustDown);
        injector::MakeJMP(Address<0x898d22c>(), CPad__GetWeapon);
        injector::MakeJMP(Address<0x898d270>(), CPad__WeaponJustDown);
        // The native getter branches in its second word. The inline hook captures
        // that branch and its delay slot together, retaining checked ownership.
        lookBehind = safetymips::create_inline(Address<0x898ce14>(), CPad__GetLookBehindForPed);
        injector::WriteMemory<uint16_t>(Address<0x898c084>(), offsetof(struct CPad, OldState.RIGHTSHOULDER1));
        injector::WriteMemory<uint16_t>(Address<0x8862e58>(), offsetof(struct CPad, NewState.RIGHTSHOULDER1));
        injector::WriteMemory<uint16_t>(Address<0x0898C11A>(), 0x1000);
        injector::WriteMemory<uint16_t>(Address<0x08950CAA>(), 0x1000);
        injector::MakeNOP(Address<0x89513f4>());
        injector::WriteMemory<uint16_t>(Address<0x898d8bc>(), offsetof(struct CPad, OldState.LEFTSHOULDER1));
        injector::WriteMemory<uint16_t>(Address<0x898d914>(), offsetof(struct CPad, OldState.LEFTSHOULDER1));
        injector::WriteMemory<uint16_t>(Address<0x08A3AF58>(), offsetof(struct CPad, NewState.RIGHTSHOULDER1));
        injector::WriteMemory<uint16_t>(Address<0x08A3AF60>(), offsetof(struct CPad, NewState.RIGHTSHOULDER1));
        injector::WriteMemory<uint16_t>(Address<0x08A3AF64>(), offsetof(struct CPad, NewState.RIGHTSHOULDER1));
        injector::MakeCALL(Address<0x894ef54>(), CPad__JumpJustDown);
        injector::MakeNOP(Address<0x0894B7E0>());
        injector::MakeNOP(Address<0x0894B7F0>());
        injector::MakeNOP(Address<0x0894B7FC>());
        injector::MakeJMP(Address<0x898c0e8>(), CPad__EnterFreeAim);
    }
}
}
