#include "Game.hpp"
#include "../Shared/Console/Cutscene.hpp"
#include "../Shared/Console/ScriptSlotZero.hpp"
namespace lcsws {
namespace {
injector::hook_back<void(void*,int,float,float)> cameraSize;
constexpr console::CutsceneFrame cinematic{16.0f / 9.0f};
console::CutsceneBorderAnimation borders;
// CCamera::m_WideScreenOn: set for cutscenes and scripted widescreen scenes.
bool CutsceneActive() { return *reinterpret_cast<const uint8_t*>(Address<0x8B833A0>() + 0x75) != 0; }
void CameraSize(void* camera, int flags, float viewWindow, float aspect) {
    if (autoAspect) Drawing::settings.aspect=console::portable::Aspect();
    cameraSize.fun(camera,flags,viewWindow,Drawing::settings.aspect);
}
void SetFov(float degrees) {
    // Cutscenes keep the original 16:9 composition: wider screens add view,
    // narrower screens keep the cinematic width. Borders are independent.
    const bool cutscene = CutsceneActive();
    const float aspect = cutscene ? cinematic.fovAspect(Drawing::settings.aspect) : Drawing::settings.aspect;
    degrees = console::horizontal_fov(degrees, cinematic.aspect, aspect);
    const float factor = cutscene && settings.restoreCutsceneFov ? 1.0f : settings.fov;
    *reinterpret_cast<float*>(Address<0x8B30D64>()) = console::bounded(degrees * factor, 1.0f, 175.0f, 70.0f);
}
void DrawCutsceneBorders(void*) {
    const auto now = *reinterpret_cast<const uint32_t*>(Address<0x8B5E144>());
    const bool dark = *reinterpret_cast<const uint8_t*>(Address<0x8B30D6F>()) >= 255;
    borders.tick(now, CutsceneActive(), settings.cutsceneBorders, dark);
    Drawing::Scope scope(Anchor::None);
    Drawing::TextureScope texture(false);
    const console::portable::StoryColor black{0,0,0,255};
    borders.draw(cinematic.window(Drawing::settings.aspect, 480, 272), 480, 272,
        [&](const console::Rect& rect) { Drawing::solidHook.call<void>(&rect, &black, true); });
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
