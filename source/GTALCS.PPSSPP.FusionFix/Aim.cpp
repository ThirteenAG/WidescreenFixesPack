#include "Game.hpp"
#include "Weapon.hpp"
#include "Shared/Console/Aim.hpp"

namespace lcsws {
namespace {
using console::AimVector;
injector::hook_back<void(uintptr_t, AimVector*)> initialTarget;
// CPlayerPed::GetInitialFreeAimPosition, called only when on-foot free aim
// starts. The target starts along the rendered camera's view (its matrix
// forward row) at the weapon's native range; native code owns the rest.
void InitialTarget(uintptr_t player, AimVector* target) {
    initialTarget.fun(player, target);
    const auto* weapon = CurrentWeapon(player, CurrentWeaponSlot(player));
    if (!weapon) return;
    const auto* info = injector::cstd<const WeaponInfo*(int)>::call(Address<0x894D97C>(), weapon->type);
    if (!info) return;
    const auto camera = Address<0x8B833A0>();
    const auto active = *reinterpret_cast<const uint8_t*>(camera + 0x7F);
    if (active >= 3) return;
    alignas(16) AimVector origin{};
    injector::cstd<void(uintptr_t, AimVector*, uintptr_t)>::call(Address<0x88EBAD8>(), 
        camera + 0x190 + active * 656, &origin, player);
    console::cameraAim(*target, origin, *reinterpret_cast<const AimVector*>(camera + 0x10), info->range);
}
}
void InstallAim() {
    initialTarget.fun = injector::MakeCALL(Address<0x894A2F4>(), InitialTarget).get();
}
}
