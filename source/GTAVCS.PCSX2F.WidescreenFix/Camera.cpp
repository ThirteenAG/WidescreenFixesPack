#include "Game.hpp"
#include "../Shared/Console/Cutscene.hpp"

namespace vcs {
namespace {
safetymips::GameInline<void(float)> fovHook;
pcsx2::GameCallback<float()> aspectCallback;
pcsx2::GameCallback<int(void*)> wideCallback;
pcsx2::GameCallback<void(void*)> borderCallback;
pcsx2::GameFunction<void(const Rect*, const Color*, bool)> drawRect;
constexpr console::CutsceneFrame cinematic{16.0f / 9.0f};
console::CutsceneBorderAnimation borders;
bool CutsceneActive() {
    return *reinterpret_cast<const uint32_t*>(address::camera + 2116) != 0;
}
void SetFOV(float value) {
    const auto area = *reinterpret_cast<const uint32_t*>(address::currentArea);
    // Retain the original fix's 70-degree multiplier. The game changes its
    // input angle for interior cameras and zoom; transforming each angle in
    // tangent space changes that established framing.
    const auto atan2 = reinterpret_cast<float (*)(float, float)>(0x44A648);
    constexpr float defaultFov = 70.0f;
    constexpr float halfAngleTangent = 0.7002075382097097f;
    constexpr float degreesPerRadian = 57.29577951308232f;
    if (CutsceneActive()) {
        const auto tan = reinterpret_cast<float (*)(float)>(0x44A3C0);
        const float aspect = cinematic.fovAspect(settings.aspect);
        value = console::horizontal_fov(value, cinematic.aspect, aspect,
            [&](float angle) { return tan(angle); }, [&](float tangent) { return atan2(tangent, 1.0f); });
        *reinterpret_cast<float*>(address::fov) = value;
        return;
    }
    const float baseAspect = area == 0 ? 16.0f / 9.0f : 4.0f / 3.0f;
    const float adjusted = (2.0f * atan2(settings.aspect / baseAspect * halfAngleTangent, 1.0f))
        * degreesPerRadian;
    *reinterpret_cast<float*>(address::fov) = value * (adjusted / defaultFov);
}
float CalculateAspectRatio() {
    *reinterpret_cast<float*>(address::aspect) = settings.aspect;
    return settings.aspect;
}
int GetWideScreenPreference(void*) {
    // Choose the game's own HUD layout without overwriting saved preferences.
    return settings.baseAspect == 16.0f / 9.0f;
}
void DrawCutsceneBorders(void*) {
    const auto now = *reinterpret_cast<const uint32_t*>(0x4CD104);
    const bool dark = *reinterpret_cast<const float*>(address::camera + 2804) >= 255.0f;
    borders.tick(now, CutsceneActive(), settings.cutsceneBorders, dark);
    DrawScope scope(DrawMode::None);
    TextureScope texture(false);
    const Color black{0,0,0,255};
    borders.draw(cinematic.window(settings.aspect, 480, 272), 480, 272,
        [&](const Rect& rect) { drawRect(&rect, &black, true); });
}
}
void InstallCamera() {
    // The second instruction is JR RA; the inline hook also captures its delay
    // slot, keeping the complete three-instruction setter in one owned patch.
    fovHook = safetymips::create_inline_game(0x2653E0, SetFOV);
    aspectCallback.bind(CalculateAspectRatio); wideCallback.bind(GetWideScreenPreference);
    injector::MakeJMP(0x2653F0, aspectCallback.address());
    injector::MakeJMP(address::wideScreenPreference, wideCallback.address());
    drawRect.bind(0x3E0B10);
    borderCallback.bind(DrawCutsceneBorders);
    // Replace the always-called cutscene-manager pass. It also runs while the
    // camera's WideScreenOn flag is clearing, so the bars can animate out.
    injector::MakeNOP(0x21F3E0);
    injector::MakeCALL(0x21F404, borderCallback.address());
}
}
