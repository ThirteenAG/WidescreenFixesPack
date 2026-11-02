#include "Game.hpp"
#include "../Shared/Console/Aim.hpp"

namespace lcsws {
namespace {
using console::AimVector;
injector::hook_back<void(uintptr_t, AimVector*)> initialTarget;
// CPlayerPed::GetInitialFreeAimPosition, called only when on-foot free aim
// starts. The target starts along the rendered camera's view (its matrix
// forward row) at the weapon's native range; native code owns the rest.
void InitialTarget(uintptr_t player, AimVector* target) {
    initialTarget.fun(player, target);
    const auto slot = *reinterpret_cast<const int8_t*>(player + 1720);
    if (slot < 0 || slot >= 10) return;
    const auto type = *reinterpret_cast<const int*>(player + 1428 + 28 * slot);
    const auto info = reinterpret_cast<uintptr_t (*)(int)>(Address<0x894D97C>())(type);
    if (!info) return;
    const float range = *reinterpret_cast<const float*>(info + 4);
    const auto camera = Address<0x8B833A0>();
    const auto active = *reinterpret_cast<const uint8_t*>(camera + 0x7F);
    if (active >= 3) return;
    alignas(16) AimVector origin{};
    reinterpret_cast<void (*)(uintptr_t, AimVector*, uintptr_t)>(Address<0x88EBAD8>())(
        camera + 0x190 + active * 656, &origin, player);
    console::cameraAim(*target, origin, *reinterpret_cast<const AimVector*>(camera + 0x10), range);
}
}
void InstallAim() {
    initialTarget.fun = injector::MakeCALL(Address<0x894A2F4>(), InitialTarget).get();
}
}
