#include "Controls.hpp"
#include "MouseInput.hpp"
#include "../../external/injector/include/ps2/patches.hpp"

namespace vcs {
namespace {
SafetyMipsMid firstPerson, followPedX, followPedY, followCarX, followCarY;
SafetyMipsMid scopedX, scopedY, aimWeapon, playerAim;
float Delta(uintptr_t camera, bool vertical, bool zoom = false) {
    if (!MouseControlsActive()) return 0.0f;
    float delta = mouseMotion.take(vertical, settings.mouseSensitivity, settings.invertMouse);
    if (zoom) delta *= console::bounded(*reinterpret_cast<const float*>(camera + 0x134), 1.0f, 160.0f, 80.0f) / 80.0f;
    return delta;
}
void FirstPerson(SafetyMipsContext& regs) {
    if (!MouseControlsActive()) return;
    regs.f0 = Delta(uintptr_t(regs.s0), false);
    regs.f4 = Delta(uintptr_t(regs.s0), true);
}
void FollowPedX(SafetyMipsContext& regs) {
    if (!MouseControlsActive()) return;
    regs.f1 = Delta(uintptr_t(regs.s2), false);
    *reinterpret_cast<float*>(uintptr_t(regs.s2) + 0x140) = 0.0f;
}
void FollowPedY(SafetyMipsContext& regs) {
    if (!MouseControlsActive()) return;
    // f0 = Alpha + fTargetDiff (f23), f1 = AlphaSpeed. Apply the mouse to
    // Alpha itself in this frame, like the PC games, instead of feeding the
    // target alpha that Alpha only reaches through the rate-limited blend
    // (which delayed vertical look). The native clamps follow this site.
    regs.f0 = regs.f0 - regs.f23 + Delta(uintptr_t(regs.s2), true);
    regs.f1 = 0.0f;
    *reinterpret_cast<float*>(uintptr_t(regs.s2) + 0x130) = 0.0f;
}
// In vehicles the camera stays where the mouse left it for one second of
// game time, then the native follow-behind resumes, as in the PC games.
bool carHold = false;
uint32_t carHoldUntil = 0;
void FollowCarX(SafetyMipsContext& regs) {
    if (!MouseControlsActive()) { carHold = false; return; }
    const uint32_t now = *reinterpret_cast<const uint32_t*>(0x4CD104); // CTimer::m_snTimeInMilliseconds
    if (mouseMotion.x != 0.0f || mouseMotion.y != 0.0f) { carHold = true; carHoldUntil = now + 1000; }
    else if (carHold && int32_t(now - carHoldUntil) >= 0) carHold = false;
    if (!carHold) return;
    regs.f0 = Delta(uintptr_t(regs.s2), false);
    *reinterpret_cast<float*>(uintptr_t(regs.s2) + 0x140) = 0.0f;
}
void FollowCarY(SafetyMipsContext& regs) {
    if (!MouseControlsActive() || !carHold) return;
    // f3 = Alpha + fTargetDiff (f22); see FollowPedY.
    regs.f3 = regs.f3 - regs.f22 + Delta(uintptr_t(regs.s2), true);
    regs.f1 = 0.0f;
    *reinterpret_cast<float*>(uintptr_t(regs.s2) + 0x130) = 0.0f;
}
void ScopedX(SafetyMipsContext& regs) {
    if (MouseControlsActive()) regs.f2 = Delta(uintptr_t(regs.s2), false, true);
}
void ScopedY(SafetyMipsContext& regs) {
    if (MouseControlsActive()) regs.f3 = Delta(uintptr_t(regs.s2), true, true);
}
void AimWeapon(SafetyMipsContext& regs) {
    if (!MouseControlsActive()) return;
    regs.f0 = Delta(uintptr_t(regs.s2), true);
    regs.f2 = Delta(uintptr_t(regs.s2), false);
    *reinterpret_cast<float*>(uintptr_t(regs.s2) + 0x130) = 0.0f;
    *reinterpret_cast<float*>(uintptr_t(regs.s2) + 0x140) = 0.0f;
}
void PlayerAim(SafetyMipsContext& regs) {
    // Runs after the native SniperModeLookLeftRight/UpDown conversion. The
    // game still updates its target object, attachment transforms, raycasts
    // and weapon range; with the PC scheme the mouse replaces the stick input.
    if (!settings.pcControls) return;
    if (!MouseControlsActive()) { regs.f22 = regs.f25 = 0.0f; return; }
    regs.f22 = mouseMotion.take(false, settings.mouseSensitivity, settings.invertMouse);
    regs.f25 = mouseMotion.take(true, settings.mouseSensitivity, settings.invertMouse);
}
}
// Native code changes of the PC scheme; switched with the menu toggle.
struct Word { uintptr_t at; uint32_t value; };
constexpr Word pcWords[] = {
    // Preserve the original PC mode's always-active mouse camera. Without this
    // branch patch, zero integer stick input triggers the automatic recenterer.
    {0x29DC18, 0},
    {0x2854F0, 0}, // D-pad controls movement, not the camera.
    {0x286704, 0}, // Flying camera input is supplied separately.
    // Free-aim look scales (lui/ori $at): -1 and 1 radian per mouse unit.
    {0x234F74, 0x3C01BF80}, {0x234F78, 0x34210000},
    {0x234BE4, 0x3C013F80}, {0x234BE8, 0x34210000},
    // Manual aim follows the player's dummy target through a separate path,
    // which bypasses AimWeapon above. Match the old mouse mode: use its target
    // angles immediately rather than catching up by 0.1 * timestep each frame.
    {0x29CC10, 0}, // Pitch target rate limit.
    {0x29CCC0, 0}, // Yaw target rate limit.
};
pcsx2::Patch pcPatches[sizeof(pcWords) / sizeof(pcWords[0])];
void ApplyMouseCamera() {
    for (auto& patch : pcPatches) if (patch) (void)(settings.pcControls ? patch.enable() : patch.disable());
}
void InstallMouseCamera() {
    firstPerson = safetymips::create_mid<&FirstPerson>(0x2956AC);
    followPedX = safetymips::create_mid<&FollowPedX>(0x29EDF0);
    followPedY = safetymips::create_mid<&FollowPedY>(0x29EEA8);
    followCarX = safetymips::create_mid<&FollowCarX>(0x2A1198);
    followCarY = safetymips::create_mid<&FollowCarY>(0x2A1340);
    scopedX = safetymips::create_mid<&ScopedX>(0x29820C);
    scopedY = safetymips::create_mid<&ScopedY>(0x298224);
    aimWeapon = safetymips::create_mid<&AimWeapon>(0x29CE04);
    playerAim = safetymips::create_mid<&PlayerAim>(0x234464);
    for (unsigned i = 0; i < sizeof(pcWords) / sizeof(pcWords[0]); ++i)
        (void)pcPatches[i].create(injector::detail::backend, pcWords[i].at, &pcWords[i].value, 1);
    ApplyMouseCamera();
}
}
