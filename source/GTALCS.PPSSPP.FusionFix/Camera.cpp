#include "Game.hpp"
#include "Cinematic.hpp"
#include "../Shared/Console/Cutscene.hpp"
#include "../Shared/Console/ScriptSlotZero.hpp"
namespace lcsws {
namespace {
injector::hook_back<void(void*,int,float,float)> cameraSize;
constexpr console::CutsceneFrame cinematic{16.0f / 9.0f};
console::CutsceneBorderAnimation borders;
// CCamera::m_WideScreenOn: set for cutscenes and scripted widescreen scenes.
bool CutsceneActive() { return *reinterpret_cast<const uint8_t*>(Address<0x8B833A0>() + 0x75) != 0; }
void UpdateBorders()
{
    const auto now = *reinterpret_cast<const uint32_t*>(Address<0x8B5E144>());
    const bool dark = *reinterpret_cast<const uint8_t*>(Address<0x8B30D6F>()) >= 255;
    borders.tick(now, CutsceneActive(), settings.cutsceneBorders != 0, dark);
}
void CameraSize(void* camera, int flags, float viewWindow, float aspect) {
    if (autoAspect) Drawing::settings.aspect=console::portable::Aspect();
    cameraSize.fun(camera,flags,viewWindow,Drawing::settings.aspect);
}
void SetFov(float degrees) {
    UpdateBorders();
    const bool cutscene = CutsceneActive();
    // Keep the original shot inside the visible window throughout the mask transition.
    // Receding side bars reveal the gameplay view; do not zoom into the shot
    // during their exit and then snap back to the wider gameplay projection.
    const bool framing = cutscene || (borders.amount > 0.0f && Drawing::settings.aspect < cinematic.aspect);
    const float aspect = framing ? CinematicAspect(Drawing::settings.aspect, cinematic.aspect,
        borders.amount, settings.cutsceneBorders) : Drawing::settings.aspect;
    const float factor = cutscene ? 1.0f : settings.fov;
    degrees = console::horizontal_fov(degrees, cinematic.aspect, aspect * factor);
    *reinterpret_cast<float*>(Address<0x8B30D64>()) = console::bounded(degrees, 1.0f, 175.0f, 70.0f);
}
void DrawCutsceneBorders(void*) {
    UpdateBorders();
    Drawing::Scope scope(Anchor::None);
    Drawing::TextureScope texture(false);
    const console::portable::StoryColor black{0,0,0,255};
    borders.draw(cinematic.window(Drawing::settings.aspect, 480, 272), 480, 272,
        [&](const console::Rect& rect) { Drawing::solidHook.call<void>(&rect, &black, true); }, settings.cutsceneBorders);
}
}
void InstallCamera() {
    console::slot_zero::PatchPortable<4>(pattern.text_addr, pattern.text_size); // Script handles in slot 0 are valid.
    cameraSize.fun=injector::MakeCALL(Address<0x89C1374>(),CameraSize).get();
    injector::MakeCALL(Address<0x8902530>(),SetFov);
    // Animated borders run every frame (including their exit transition) in
    // place of the native fixed widescreen bars after the pickup text.
    injector::MakeNOP(Address<0x89C2BEC>());
    injector::MakeCALL(Address<0x89C2BF4>(), DrawCutsceneBorders);
}
}
