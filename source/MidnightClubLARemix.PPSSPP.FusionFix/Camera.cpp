#include "Game.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
extern "C" {
#include <pspdisplay.h>
}

namespace mc3 {
namespace {
SafetyMipsMid projection, sample, neutral, pan;
float stickX, stickY; // right stick, -1..1, y up

float Axis(uint8_t value) {
    const float axis = (float(value) - 127.5f) / 127.5f;
    return axis < -1.0f ? -1.0f : axis > 1.0f ? 1.0f : axis;
}
}

void InstallAspect() {
    char ratioBuffer[64];
    const char* ratio = inireader.ReadString("MAIN", "ForceAspectRatio", "auto", ratioBuffer, sizeof(ratioBuffer));
    const float forcedAspect = console::parse_forced_aspect(ratio);
    if (forcedAspect > 0.0f) { targetAspect = forcedAspect; automaticAspect = false; }
    RefreshAspect();
    fovFactor = std::clamp(inireader.ReadFloat("MAIN", "FOVFactor", 1.0f), 0.5f, 2.0f);
    hudScale = std::clamp(inireader.ReadFloat("MAIN", "HudScale", 1.0f), 0.5f, 1.5f);
    cutsceneBorders = std::clamp(inireader.ReadInteger("MAIN", "CutsceneBorders", 3), 0, 3);
    char box[64]{};
    hudAspect = console::parse_hud_aspect(inireader.ReadString("MAIN", "HudAspectRatioConstraint", "Auto", box, sizeof(box)));

    // Perspective setter: f12 is the vertical FOV in degrees, f24 the selected
    // aspect (the viewport default at +0x134 when the caller passed 0). The
    // caller's values are already stored, so the camera keeps its own FOV.
    // Full-screen cameras get Hor+; render-target and mirror cameras keep
    // their own view. Gameplay cameras take the FOV factor; the opening
    // fly-by keeps its framing (borders cover the rest, see Hud.cpp).
    const auto site = pattern.get_first("34 01 18 C6 0E 3C 04 3C 35 FA 84 34", 4);
    projection = safetymips::create_mid(site, [](SafetyMipsContext& regs) {
        const auto viewport = uintptr_t(regs.s0);
        if (at<int>(viewport + 0x1AC) != 480 || at<int>(viewport + 0x1B0) != 272) return;
        const float aspect = regs.f24;
        if (std::fabs(aspect - at<float>(viewport + 0x134)) > 0.001f) return;
        sceneVcount = sceDisplayGetVcount();
        RefreshAspect();
        const float widening = Widening();
        regs.f24 = aspect * widening;
        float scale = fovFactor;
        if (Cutscene())
            scale = widening < 1.0f && (cutsceneBorders & 1) ? 1.0f / widening : 1.0f;
        if (scale != 1.0f)
        {
            constexpr float radians = 0.01745329251994329577f;
            const float tangent = std::tan(float(regs.f12) * (radians * 0.5f)) * scale;
            regs.f12 = std::min(std::atan(tangent) * (2.0f / radians), 170.0f);
        }
    });
}

void InstallCamera() {
    if (!dualAnalog) return;
    const auto sampleSite = pattern.get_first("10 00 06 A2 11 00 05 A2", 0);
    const auto neutralSite = pattern.get_first("00 00 A0 A0 01 00 A7 A0 02 00 A6 A0", 0);
    const auto panSite = pattern.get_first("?? ?? 8C C7 02 00 00 10 07 63 00 46 06 B3 00 46 07 00 40 16 80 00 0C E6", 16);
    // Pad sampling: the game keeps the second stick in its pad record (+18/+19).
    sample = safetymips::create_mid(sampleSite, [](SafetyMipsContext& regs) {
        const auto object = uintptr_t(regs.s0);
        const auto pad = reinterpret_cast<const SceCtrlData*>(uintptr_t(regs.sp) + 4);
        const bool available = regs.v0 != 0 && !(uint32_t(regs.a3) & PSP_CTRL_HOLD);
        const uint8_t x = available ? pad->Rsrv[0] : 128, y = available ? pad->Rsrv[1] : 128;
        at<uint8_t>(object + 18) = x;
        at<uint8_t>(object + 19) = y;
        stickX = Axis(x);
        stickY = -Axis(y);
    });
    // Unbound analog actions rest at the centre instead of full deflection.
    safetymips::Options replace; replace.execute_original = false;
    neutral = safetymips::create_mid(neutralSite,
        [](SafetyMipsContext& regs) {
            const auto mapping = reinterpret_cast<const int*>(uintptr_t(regs.s0));
            const auto index = unsigned(regs.a0);
            const bool axis = index < unsigned(mapping[0]) && mapping[index + 2] == 9 &&
                              unsigned(mapping[index + 66]) < 4;
            auto state = reinterpret_cast<uint8_t*>(uintptr_t(regs.a1));
            state[0] = axis ? 128 : 0;
            state[1] = uint8_t(regs.a3);
        }, replace);
    // Chase camera pan: D-pad left/right select +-90 degrees (f12) and the
    // camera applies the change against the previous frame's pan. The right
    // stick supplies the same pan continuously: sideways looks left/right,
    // pulling back looks behind the car. The D-pad keeps priority.
    pan = safetymips::create_mid(panSite,
        [](SafetyMipsContext& regs) {
            if (float(regs.f12) != 0.0f) return;
            constexpr float deadzone = 0.2f;
            const float length = std::sqrt(stickX * stickX + stickY * stickY);
            if (length <= deadzone) return;
            const float amount = length >= 1.0f ? 1.0f : (length - deadzone) / (1.0f - deadzone);
            regs.f12 = -std::atan2(stickX, stickY) * amount;
        });
}
}
