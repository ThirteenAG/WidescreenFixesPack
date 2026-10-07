#include "Game.hpp"
#include "../Shared/Console/Aim.hpp"

namespace vcs {
namespace {
using console::AimVector;
injector::hook_back<void(AimVector*, uintptr_t)> initialTarget;
pcsx2::GameCallback<void(AimVector*, uintptr_t)> initialTargetCallback;
void InitialTarget(AimVector* target, uintptr_t player) {
    initialTarget.fun(target, player);
    // This startup call also serves passengers and scripted attachments.
    if ((*reinterpret_cast<const uint32_t*>(player + 0x820) &&
         reinterpret_cast<int (*)(uintptr_t)>(0x219D50)(player)) ||
        *reinterpret_cast<const uint32_t*>(player + 0x858)) return;
    const auto slot = *reinterpret_cast<const int8_t*>(player + 0x789);
    if (slot < 0 || slot >= 10) return;
    const auto type = *reinterpret_cast<const int*>(player + 0x578 + 28*slot);
    const auto info = reinterpret_cast<uintptr_t (*)(int)>(0x407120)(type);
    const float range = *reinterpret_cast<const float*>(info + 8);
    const auto active = *reinterpret_cast<const uint8_t*>(address::camera + 0x50);
    if (active >= 3) return;
    AimVector origin{};
    reinterpret_cast<void (*)(AimVector*, uintptr_t, uintptr_t)>(0x2962D8)(
        &origin, address::camera + 0x70 + active*0x270, player);
    // TheCamera's matrix rows are right (+0x00), forward (+0x10), up (+0x20);
    // the viewing direction is the forward row (verified with PINE in game:
    // +0x20 is the up vector, which aimed the initial target straight up).
    console::cameraAim(*target, origin, *reinterpret_cast<const AimVector*>(address::camera + 0x10), range);
}
}
void InstallAim() {
    initialTargetCallback.bind(InitialTarget);
    initialTarget.fun = injector::MakeCALL(0x239520, initialTargetCallback.address()).get();
}
}
