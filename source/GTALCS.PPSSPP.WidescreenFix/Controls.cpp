#include "Game.hpp"
#include <cstddef>
#include <cstdint>
using namespace mips_asm;
namespace lcsws {
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
    uint8_t reserved[30];
    int16_t Mode, ShakeDur, DisablePlayerControls;
};
static_assert(sizeof(CControllerState) == 48 && offsetof(CPad, OldState) == 52);
uintptr_t (*FindPlayerPed)(){};
uintptr_t (*FindPlayerVehicle)(){};
uintptr_t (*WeaponInfo)(int){};
uintptr_t reloadWeapon;
bool Reloading() {
    auto ped = FindPlayerPed();
    if (!ped) { reloadWeapon = 0; return false; }
    uintptr_t current = ped + 0x590 + uint8_t(*reinterpret_cast<uint8_t*>(ped + 0x6B8)) * 28;
    if (reloadWeapon != current || *reinterpret_cast<int*>(current + 4) != 2) {
        reloadWeapon = 0; return false;
    }
    return true;
}
void Reload(CPad* pad) {
    if (!pad->NewState.LEFTSHOULDER1 || !pad->NewState.SQUARE || pad->OldState.SQUARE || Reloading()) return;
    auto ped = FindPlayerPed();
    if (!ped) return;
    unsigned slot = *reinterpret_cast<uint8_t*>(ped + 0x6B8);
    if (slot < 3 || slot > 8) return;
    uintptr_t weapon = ped + 0x590 + slot * 28;
    auto* state = reinterpret_cast<int*>(weapon + 4);
    auto* clip = reinterpret_cast<int*>(weapon + 8);
    auto* total = reinterpret_cast<int*>(weapon + 12);
    if (*total <= 0 || *clip >= *total || *state == 2) return;
    uintptr_t info = WeaponInfo(*reinterpret_cast<int*>(weapon + 0));
    if (!info || *clip >= *reinterpret_cast<int*>(info + 16)) return;
    unsigned duration = *reinterpret_cast<unsigned*>(info + 12);
    if (duration < 100 || duration > 10000) duration = 1000;
    // Enter the native reload state. CWeapon::Update owns animation completion,
    // clip refilling and reload audio; no code or controller state is rewritten.
    *state = 2;
    *reinterpret_cast<unsigned*>(weapon + 16) = *reinterpret_cast<unsigned*>(Address<0x8B5E144>()) + duration;
    reloadWeapon = weapon;
}
int16_t Held(CPad* pad, int16_t value) { return pad->DisablePlayerControls ? 0 : value; }
int16_t CPad__GetAccelerate(CPad* pad) { return Held(pad,pad->NewState.RIGHTSHOULDER1); }
int16_t CPad__GetBrake(CPad* pad) { return Held(pad,pad->NewState.LEFTSHOULDER1); }


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
    FindPlayerPed = reinterpret_cast<uintptr_t (*)()>(Address<0x89d58b0>());
    FindPlayerVehicle = reinterpret_cast<uintptr_t (*)()>(Address<0x89d57b4>());
    WeaponInfo = reinterpret_cast<uintptr_t (*)(int)>(Address<0x894d97c>());
    bool ModernControlScheme=settings.modernControls;
    {
        injector::WriteInstr(Address<0x0886AD18>(),
            sh(a1, sp, 0x10)
        );
        injector::WriteInstr(Address<0x0886AD10>(),
            lhu(a1, sp, 0x1E)
        );
        injector::MakeNOP(Address<0x08A98CA4>());
        injector::WriteMemory<uint32_t>(Address<0x08A98CB4>(), (injector::ReadMemory<uint32_t>(Address<0x08A98CB4>()) & 0xFFFFu) | 0x10000000u);
        injector::MakeCALL(Address<0x08A98D28>(), CPad__GetRightStickX);
        injector::MakeNOP(Address<0x08A98DA4>());
        injector::WriteMemory<uint32_t>(Address<0x08A98DB4>(), (injector::ReadMemory<uint32_t>(Address<0x08A98DB4>()) & 0xFFFFu) | 0x10000000u);
        injector::MakeCALL(Address<0x08A98DE0>(), CPad__GetRightStickY);
        injector::MakeCALL(Address<0x08A98AA8>(), aimX);
        injector::MakeCALL(Address<0x08A98AD4>(), aimX);
        injector::MakeCALL(Address<0x08A98AE4>(), aimX);
        injector::MakeCALL(Address<0x08A98B9C>(), aimY);
        injector::MakeCALL(Address<0x08A98C04>(), aimY);
        injector::MakeNOP(Address<0x08A97554>());
        injector::MakeNOP(Address<0x08A975B8>());
        injector::MakeNOP(Address<0x088F35CC>());
    }
    if (ModernControlScheme)
    {
        injector::MakeJMP(Address<0x8a9802c>(), CPad__GetAccelerate);
        injector::WriteMemory<uint16_t>(Address<0x08A97E90>(), offsetof(struct CPad, NewState.CROSS));
        injector::WriteMemory<uint16_t>(Address<0x8a97e90>() - 0x30, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
        injector::MakeJMP(Address<0x8a97eb4>(), CPad__GetBrake);
        injector::WriteMemory<uint16_t>(Address<0x8a989a0>(), offsetof(struct CPad, NewState.SQUARE));
        injector::WriteMemory<uint16_t>(Address<0x8a9788c>(), -110);
        injector::WriteMemory<uint16_t>(Address<0x8a97a60>(), 110);
        injector::MakeInlineWithNOP(Address<0x8a977ec>(),
            lh(a0, s0, offsetof(struct CPad, NewState.SQUARE)),
            lh(k0, s0, offsetof(struct CPad, NewState.CIRCLE)),
            _or(a0, a0, k0),
            beq(a0, zero, 2),
            nop(),
            j(Address<0x8a97884>()),
            nop()
        );
        injector::MakeInlineWithNOP(Address<0x8a979c0>(),
            lh(a0, s0, offsetof(struct CPad, NewState.SQUARE)),
            lh(k0, s0, offsetof(struct CPad, NewState.CIRCLE)),
            _or(a0, a0, k0),
            beq(a0, zero, 2),
            nop(),
            j(Address<0x8a97a58>()),
            nop()
        );
        injector::MakeInlineWithNOP(Address<0x8a97bc8>(),
            lh(a0, s0, offsetof(struct CPad, NewState.SQUARE)),
            bne(a0, zero, 2),
            nop(),
            j(Address<0x8a97c18>()),
            nop()
        );
        injector::WriteMemory<uint16_t>(Address<0x8a98868>(), offsetof(struct CPad, NewState.SQUARE));
        injector::WriteMemory<uint16_t>(Address<0x8a972a4>(), offsetof(struct CPad, NewState.SQUARE));
        injector::WriteMemory<uint16_t>(Address<0x8a96a28>(), offsetof(struct CPad, NewState.LEFTSHOULDER1));
        injector::WriteMemory<uint16_t>(Address<0x8a995f4>(), offsetof(struct CPad, NewState.LEFTSHOULDER1));
        injector::MakeJMP(Address<0x8a98240>(), CPad__GetTarget);
        injector::MakeJMP(Address<0x8a98290>(), CPad__TargetJustDown);
        injector::MakeJMP(Address<0x8a97f90>(), CPad__GetWeapon);
        injector::MakeJMP(Address<0x8a97fcc>(), CPad__WeaponJustDown);
        injector::MakeJMP(Address<0x8a97c2c>(), CPad__GetLookBehindForPed);
        injector::WriteMemory<uint16_t>(Address<0x887bb90>(), offsetof(struct CPad, NewState.RIGHTSHOULDER1));
        injector::WriteMemory<uint16_t>(Address<0x08A97182>(), 0x1000);
        injector::WriteMemory<uint16_t>(Address<0x08949C32>(), 0x1000);
        injector::WriteMemory<uint16_t>(Address<0x0894A4A2>(), 0x1000);
        injector::WriteMemory<uint16_t>(Address<0x8a985b0>(), offsetof(struct CPad, OldState.LEFTSHOULDER1));
        injector::WriteMemory<uint16_t>(Address<0x8a98600>(), offsetof(struct CPad, OldState.LEFTSHOULDER1));
        injector::MakeCALL(Address<0x89432b4>(), CPad__JumpJustDown);
        injector::MakeJMP(Address<0x8a97150>(), CPad__EnterFreeAim);
    }
}
}
