#include "Controls.hpp"

namespace vcs {
namespace {
SafetyMipsMid firstPerson, followPedX, followPedY, followCarX, followCarY;
SafetyMipsMid scopedX, scopedY, aimWeapon, playerAim;
SafetyMipsMid keyboardFollowPed, aimScaleX, aimScaleY, aimPitchLimit, aimYawLimit;
pcsx2::GameFunction<int16_t(Pad*)> nativeDpadSteer, nativeFlyingUp;
pcsx2::GameCallback<int16_t(Pad*)> dpadSteer, flyingUp;
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
VehicleCameraHold mouseCameraHold;
void FollowCarX(SafetyMipsContext& regs)
{
    if (!MouseControlsActive())
    {
        mouseCameraHold.Clear();
        return;
    }
    const uint32_t now = *reinterpret_cast<const uint32_t*>(0x4CD104); // CTimer::m_snTimeInMilliseconds
    const bool moved = mouseMotion.x != 0.0f || mouseMotion.y != 0.0f;
    if (!mouseCameraHold.Update(now, moved, 1000))
        return;
    regs.f0 = Delta(uintptr_t(regs.s2), false);
    *reinterpret_cast<float*>(uintptr_t(regs.s2) + 0x140) = 0.0f;
}
void FollowCarY(SafetyMipsContext& regs) {
    if (!MouseControlsActive() || !mouseCameraHold.Active()) return;
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
    if (!KeyboardActive()) return;
    if (!MouseControlsActive()) { regs.f22 = regs.f25 = 0.0f; return; }
    regs.f22 = mouseMotion.take(false, settings.mouseSensitivity, settings.invertMouse);
    regs.f25 = mouseMotion.take(true, settings.mouseSensitivity, settings.invertMouse);
}
void KeepKeyboardFollowPedActive(SafetyMipsContext& regs)
{
    // The next two instructions shift the right-stick magnitudes down by 16.
    // A positive result keeps the native follow camera's recenter gate open.
    if (KeyboardActive())
        regs.v1 = 1 << 16;
}
void AimScaleX(SafetyMipsContext& regs)
{
    if (KeyboardActive())
        regs.f23 = -1.0f;
}
void AimScaleY(SafetyMipsContext& regs)
{
    if (KeyboardActive())
        regs.f0 = 1.0f;
}
void FollowAimTarget(SafetyMipsContext& regs)
{
    // The native comparison snaps to the aim target below this angular limit.
    if (KeyboardActive())
        regs.f3 = 1000.0f;
}
int16_t ReadDpadSteer(Pad* pad)
{
    return KeyboardFor(pad) ? 0 : nativeDpadSteer(pad);
}
int16_t ReadFlyingUp(Pad* pad)
{
    return KeyboardFor(pad) ? 0 : nativeFlyingUp(pad);
}
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
    keyboardFollowPed = safetymips::create_mid<&KeepKeyboardFollowPedActive>(0x29DC0C);
    aimScaleX = safetymips::create_mid<&AimScaleX>(0x234F80);
    aimScaleY = safetymips::create_mid<&AimScaleY>(0x234BF0);
    aimPitchLimit = safetymips::create_mid<&FollowAimTarget>(0x29CC00);
    aimYawLimit = safetymips::create_mid<&FollowAimTarget>(0x29CCB0);
    nativeDpadSteer.bind(0x285858);
    nativeFlyingUp.bind(0x287430);
    dpadSteer.bind(ReadDpadSteer);
    flyingUp.bind(ReadFlyingUp);
    injector::MakeCALL(0x2854F0, dpadSteer.address());
    injector::MakeCALL(0x286704, flyingUp.address());
}
}
