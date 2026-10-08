#include "Game.hpp"
#include "../Shared/Console/Cutscene.hpp"
#include "../Shared/Console/ScriptSlotZero.hpp"

namespace lcs {
namespace {
// CDraw::ms_fFOV / ms_fAspectRatio (gp-0x1AE8 / gp-0x1AE4, game gp 0x3D6EF0).
// Use absolute addresses: inside a game callback $gp is not the game's.
constexpr uintptr_t drawFov = 0x3D5408, drawAspect = 0x3D540C;
// TheCamera: m_WideScreenOn +0xE4, m_fFLOATingFade +0x1BB4 (sub_2920E0).
constexpr uintptr_t camera = 0x43BBF0;
constexpr uintptr_t timeInMilliseconds = 0x3D9B70;

pcsx2::GameCallback<void(float)> fovCallback;
pcsx2::GameCallback<float()> aspectCallback;
pcsx2::GameCallback<void(void*)> borderCallback;
pcsx2::GameFunction<void(const Rect*, const Color*, bool)> drawRect;
constexpr console::CutsceneFrame cinematic{16.0f / 9.0f};
console::CutsceneBorderAnimation borders;

bool CutsceneActive() {
    return *reinterpret_cast<const uint32_t*>(camera + 0xE4) != 0;
}
void SetFOV(float value) {
    // Call the game's tanf/atanf through lambdas. Passed directly as constant
    // function pointers, GCC 14 (r5900) folded horizontal_fov to "return degrees",
    // so the previous build never changed the FOV (CDraw FOV stayed 70 at 21:9).
    const auto tanf = reinterpret_cast<float (*)(float)>(0x361698);
    const auto atanf = reinterpret_cast<float (*)(float)>(0x361038);
    const auto tangent = [&](float angle) { return tanf(angle); };
    const auto arcTangent = [&](float value) { return atanf(value); };
    if (CutsceneActive()) {
        // Keep the 16:9 cinematic composition of the native widescreen mode:
        // horizontal coverage up to 16:9, vertical coverage beyond it.
        *reinterpret_cast<float*>(drawFov) = console::horizontal_fov(
            value, cinematic.aspect, cinematic.fovAspect(settings.aspect), tangent, arcTangent);
        return;
    }
    const auto area = *reinterpret_cast<const uint32_t*>(address::currentArea);
    *reinterpret_cast<float*>(drawFov) = console::horizontal_fov(
        value, area == 0 ? 16.0f / 9.0f : 4.0f / 3.0f, settings.aspect, tangent, arcTangent);
}
float CalculateAspectRatio() {
    UpdateViewport();
    // LCS reads its native preference directly, rather than through a query
    // function. Select the matching HUD layout when its camera aspect updates.
    // The frontend draws with the 4:3 layout (Frontend.cpp holds the
    // preference at 0 there, and RenderMenus/DrawLoading reach this function).
    if (drawMode != DrawMode::Frontend)
        *reinterpret_cast<uint8_t*>(address::wideScreenPreference) = settings.baseAspect == 16.0f / 9.0f;
    *reinterpret_cast<float*>(drawAspect) = settings.aspect;
    return settings.aspect;
}
void DrawCutsceneBorders(void*) {
    // Replaces CCamera::DrawBordersForWideScreen in Render2DStuff, which only drew
    // fixed 4:3-mode bars. Called every frame, so the bars also animate out.
    const auto now = *reinterpret_cast<const uint32_t*>(timeInMilliseconds);
    const bool dark = *reinterpret_cast<const float*>(camera + 0x1BB4) >= 255.0f;
    borders.tick(now, CutsceneActive(), settings.cutsceneBorders, dark);
    DrawScope scope(DrawMode::None);
    TextureScope texture(false);
    const Color black{0,0,0,255};
    borders.draw(cinematic.window(settings.aspect, 480, 272), 480, 272,
        [&](const Rect& rect) { drawRect(&rect, &black, true); });
}
}
void InstallCamera() {
    // Script handles in slot 0 are valid (DOES_VEHICLE_EXIST).
    static constexpr console::slot_zero::Ps2Site slotZero[] = {{0x189D10, 0x2C540045}};
    console::slot_zero::PatchPs2(slotZero);
    fovCallback.bind(SetFOV); aspectCallback.bind(CalculateAspectRatio);
    injector::MakeJMP(0x2083A0, fovCallback.address());
    injector::MakeJMP(0x2083A8, aspectCallback.address());
    drawRect.bind(0x3219C0);
    borderCallback.bind(DrawCutsceneBorders);
    // Render2DStuff: "if (m_WideScreenOn) DrawBordersForWideScreen(&TheCamera)".
    // Drop the condition (BEQZ, its delay slot is a NOP) and call our pass instead.
    injector::MakeNOP(0x1F63B8);
    injector::MakeCALL(0x1F63C0, borderCallback.address());
}
}
