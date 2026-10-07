#include "Game.hpp"
#include "../Shared/Console/Aim.hpp"

namespace vcsws {
namespace {
using console::AimVector;
injector::hook_back<void(uintptr_t, AimVector*)> initialTarget;
// Entering manual aim on foot starts along the rendered camera's view (its
// matrix forward row on PSP), at the
// weapon's native range (as on PS2). Native code owns everything afterwards.
void InitialTarget(uintptr_t player, AimVector* target) {
    initialTarget.fun(player, target);
    // This call also serves drive-bys, passengers and scripted attachments.
    if (reinterpret_cast<bool (*)(uintptr_t)>(Address<0x8910B38>())(player) ||
        *reinterpret_cast<const uint32_t*>(player + 0x848)) return;
    const auto slot = *reinterpret_cast<const int8_t*>(player + 0x789);
    if (slot < 0 || slot >= 10) return;
    const auto type = *reinterpret_cast<const int*>(player + 0x578 + 28 * slot);
    const auto info = reinterpret_cast<uintptr_t (*)(int)>(Address<0x8B1FD70>())(type);
    if (!info) return;
    const float range = *reinterpret_cast<const float*>(info + 8);
    const auto camera = Address<0x8BC7E30>();
    const auto active = *reinterpret_cast<const uint8_t*>(camera + 0x50);
    if (active >= 3) return;
    alignas(16) AimVector origin{};
    reinterpret_cast<void (*)(uintptr_t, AimVector*, uintptr_t)>(Address<0x89991D4>())(
        camera + 0x70 + active * 608, &origin, player);
    console::cameraAim(*target, origin, *reinterpret_cast<const AimVector*>(camera + 0x10), range);
}
}
void InstallAim() {
    initialTarget.fun = injector::MakeCALL(Address<0x894D1F0>(), InitialTarget).get();
}
}
