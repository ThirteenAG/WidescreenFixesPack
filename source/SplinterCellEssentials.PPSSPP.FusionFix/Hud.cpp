#include "Game.hpp"
namespace essentials {
namespace {
SafetyMipsMid tiles, screenInput;
SafetyMipsInline worldOutput, vectorOutput;
int WorldToScreen(uintptr_t object, uintptr_t frame, float* result) {
    const int status = worldOutput.call<int>(object, frame, result);
    // The native result is already in display pixels. Undo the later UI
    // transform so projected hacking targets receive that transform only once.
    if (result) UnprojectHud(result);
    return status;
}
struct VectorArray { float (*data)[4]; unsigned size, capacity; };
int VectorWorldToScreen(uintptr_t object, VectorArray* vectors, void* location, void* rotation) {
    const int status = vectorOutput.call<int>(object, vectors, location, rotation);
    if (vectors && vectors->data && vectors->size <= vectors->capacity)
        for (unsigned i = 0; i < vectors->size; ++i) UnprojectHud(vectors->data[i]);
    return status;
}
}
void InstallHud() {
    hudScale = console::bounded(inireader.ReadFloat("MAIN", "HudScale", 1), 0.5f, 1.5f, 1);
    UpdateDisplay();
    safetymips::Options replace{}; replace.instructions = 13; replace.execute_original = false;
    tiles = safetymips::create_mid(sites::DrawTile() + 0x70, [](SafetyMipsContext& regs) {
        auto stack = reinterpret_cast<float*>(uintptr_t(regs.sp));
        stack[0x44 / 4] = regs.f19; // Displaced SWC1 in the native branch delay slot.
        float x0 = regs.f30, y0 = regs.f28, x1 = regs.f2, y1 = regs.f0;
        if (regs.a3) { x0 *= 0.75f; x1 *= 0.75f; y0 *= 272.0f / 480; y1 *= 272.0f / 480; }
        const bool fullWidth = x0 <= 1 && x1 >= 479;
        const bool screenOverlay = fullWidth && (y1 - y0 >= 270 || y1 - y0 <= 55);
        // Native fullscreen fades, letterboxing and visor masks cover the viewport.
        if (!screenOverlay) { x0 = hud.x(x0); x1 = hud.x(x1); y0 = hud.y(y0); y1 = hud.y(y1); }
        regs.f30 = x0; regs.f28 = y0;
        stack[0x34 / 4] = x1; stack[0x30 / 4] = y1;
    }, replace);
    worldOutput = safetymips::create_inline(sites::WorldToScreen(), WorldToScreen);
    vectorOutput = safetymips::create_inline(sites::VectorWorldToScreen(), VectorWorldToScreen);
    screenInput = safetymips::create_mid(sites::ScreenTransform(sites::ScreenToWorld()), [](SafetyMipsContext& regs) {
        auto point = reinterpret_cast<float*>(uintptr_t(regs.fp));
        point[0] = hud.x(point[0]); point[1] = hud.y(point[1]);
    });
}
}
