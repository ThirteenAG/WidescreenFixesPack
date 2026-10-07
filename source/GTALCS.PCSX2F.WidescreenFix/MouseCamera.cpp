#include "Controls.hpp"

// Direct mouse look for the LCS PS2 cameras. Mouse motion is applied to the
// camera angles in the frame it arrives, like the PC games: no stick response
// curve, no speed smoothing and no timestep scaling. CCam layout (reLCS):
// +0xE0 Alpha, +0xE4 AlphaSpeed, +0xE8 FOV, +0xF0 Beta, +0xF4 BetaSpeed.
namespace lcs {
namespace {
SafetyMipsMid followPedX, followPedY, followCarX, followCarY, firstPersonLook;
SafetyMipsMid scopedX, scopedY, aimLook, freeAim, aimFollow;
float& Field(uintptr_t cam, unsigned offset) { return *reinterpret_cast<float*>(cam + offset); }
float Fov(uintptr_t cam) { return console::bounded(Field(cam, 0xE8), 1.0f, 160.0f, 70.0f); }
float Take(bool vertical, uintptr_t cam = 0, bool zoom = false) {
    float delta = mouseMotion.take(vertical, settings.mouseSensitivity, settings.invertMouse);
    // Scoped views keep the same on-screen speed at every zoom level.
    if (zoom && cam) delta *= Fov(cam) / 70.0f;
    return delta;
}

// CCam::Process_FollowPed_SA. Beta += BetaSpeed * timestep (f2 += f1). The
// stick/auto-follow speed is discarded: mouse look owns the yaw.
void FollowPedX(SafetyMipsContext& regs) {
    if (!MouseControlsActive()) return;
    const auto cam = uintptr_t(regs.s2);
    regs.f1 = Take(false);
    Field(cam, 0xF4) = 0.0f;
}
// Alpha += clamp(fTargetDiff) (f0 += f24). Replace the automatic pitch return
// with the mouse; alpha limits are applied by the code that follows.
void FollowPedY(SafetyMipsContext& regs) {
    if (!MouseControlsActive()) return;
    const auto cam = uintptr_t(regs.s2);
    regs.f24 = Take(true);
    Field(cam, 0xE4) = 0.0f;
}

// CCam::Process_FollowCar_SA: after mouse movement, hold the camera where the
// player left it for one second (50 timesteps) before the native
// follow-behind resumes, as in the PC games.
float carHold = 0.0f;
void FollowCarX(SafetyMipsContext& regs) {
    if (!MouseControlsActive()) { carHold = 0.0f; return; }
    const auto cam = uintptr_t(regs.s2);
    if (mouseMotion.moving()) carHold = 50.0f;
    else if (carHold > 0.0f) carHold -= console::bounded(*reinterpret_cast<const float*>(address::timeStep), 0.0f, 10.0f, 1.0f);
    if (carHold <= 0.0f) return;
    regs.f0 = Take(false);
    Field(cam, 0xF4) = 0.0f;
}
void FollowCarY(SafetyMipsContext& regs) {
    if (!MouseControlsActive() || carHold <= 0.0f) return;
    const auto cam = uintptr_t(regs.s2);
    regs.f23 = Take(true);
    Field(cam, 0xE4) = 0.0f;
}

// CCam::Process_1stPerson: Beta += f0, Alpha += f4.
void FirstPersonLook(SafetyMipsContext& regs) {
    if (!MouseControlsActive()) return;
    regs.f0 = Take(false);
    regs.f4 = Take(true);
}

// CCam::Process_M16_1stPerson (scopes, rocket launcher, first-person weapons).
// BetaSpeed = f2 (old speed * blend) + f0; Beta += BetaSpeed.
void ScopedX(SafetyMipsContext& regs) {
    if (!MouseControlsActive()) return;
    regs.f2 = 0.0f;
    regs.f0 = Take(false, uintptr_t(regs.s2), true);
}
// AlphaSpeed = f3 + f1; Alpha += AlphaSpeed.
void ScopedY(SafetyMipsContext& regs) {
    if (!MouseControlsActive()) return;
    regs.f3 = 0.0f;
    regs.f1 = Take(true, uintptr_t(regs.s2), true);
}

// CCam::Process_AimWeapon free-look path: Beta += f3, Alpha += f2.
void AimLook(SafetyMipsContext& regs) {
    if (!MouseControlsActive()) return;
    const auto cam = uintptr_t(regs.s1);
    regs.f3 = Take(false);
    regs.f2 = Take(true);
    Field(cam, 0xF4) = 0.0f;
    Field(cam, 0xE4) = 0.0f;
}

// Manual aim: the camera follows the player's aim target by at most f3 per
// frame (rate * timestep). Use the target angles immediately, which is the
// native 1000-radian limit already used by scripted cameras.
void AimFollow(SafetyMipsContext& regs) {
    if (MouseControlsActive()) regs.f3 = 1000.0f;
}

// CPlayerPed::PlayerControlFreeAim: f23 = SniperModeLookLeftRight (the target
// turns by f23 * 0.0003 rad), f22 = SniperModeLookUpDown (the target rises by
// f22 * 0.000333 * distance). Supply the mouse as the equivalent stick values.
void FreeAim(SafetyMipsContext& regs) {
    if (!MouseControlsActive()) { regs.f22 = regs.f23 = 0.0f; return; }
    regs.f23 = -Take(false) / 0.0003f;
    regs.f22 = Take(true) / 0.00033333333f;
}
}
void InstallMouseCamera() {
    followPedX = safetymips::create_mid<&FollowPedX>(0x2830B4);
    followPedY = safetymips::create_mid<&FollowPedY>(0x283180);
    followCarX = safetymips::create_mid<&FollowCarX>(0x2853F0);
    followCarY = safetymips::create_mid<&FollowCarY>(0x2855A8);
    firstPersonLook = safetymips::create_mid<&FirstPersonLook>(0x27B248);
    scopedX = safetymips::create_mid<&ScopedX>(0x27D968);
    scopedY = safetymips::create_mid<&ScopedY>(0x27D980);
    aimLook = safetymips::create_mid<&AimLook>(0x28181C);
    freeAim = safetymips::create_mid<&FreeAim>(0x34A520);
    aimFollow = safetymips::create_mid<&AimFollow>(0x281600);
}
}
