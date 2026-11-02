#include "Game.hpp"
#include "../Shared/Console/Aim.hpp"

namespace lcs {
namespace {
using console::AimVector;
injector::hook_back<uint64_t(AimVector*, uintptr_t)> initialTarget;
pcsx2::GameCallback<uint64_t(AimVector*, uintptr_t)> initialTargetCallback;
uint64_t InitialTarget(AimVector* target, uintptr_t player) {
    const auto result = initialTarget.fun(target, player);
    // A lock-on target (CPlayerPed +0x6BC, checked by the native routine at
    // 0x34ED50) keeps the game's own aim direction.
    if (*reinterpret_cast<const uint32_t*>(player + 0x6BC)) return result;
    const auto slot = *reinterpret_cast<const int8_t*>(player + 1720);
    if (slot < 0 || slot >= 10) return result;
    const auto type = *reinterpret_cast<const int*>(player + 1428 + 28*slot);
    const auto info = reinterpret_cast<uintptr_t (*)(int)>(0x34F2F0)(type);
    const float range = *reinterpret_cast<const float*>(info + 4);
    constexpr uintptr_t camera = 0x43BBF0;
    const auto active = *reinterpret_cast<const uint8_t*>(camera + 268);
    if (active >= 3) return result;
    AimVector origin{};
    reinterpret_cast<void (*)(AimVector*, uintptr_t, uintptr_t)>(0x27BD68)(
        &origin, camera + 0x220 + active*688, player);
    // TheCamera's matrix rows are right (+0x00), forward (+0x10), up (+0x20);
    // the viewing direction is the forward row (verified with PINE in game).
    console::cameraAim(*target, origin, *reinterpret_cast<const AimVector*>(camera + 0x10), range);
    return result;
}
}
void InstallAim() {
    initialTargetCallback.bind(InitialTarget);
    initialTarget.fun = injector::MakeCALL(0x34B640, initialTargetCallback.address()).get();
}
}
