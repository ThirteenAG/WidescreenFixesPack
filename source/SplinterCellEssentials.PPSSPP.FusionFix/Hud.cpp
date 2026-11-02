#include "Game.hpp"
namespace essentials {
namespace {
// Native HUD elements (AEQInvHUD / AEchelonMainHUD drawing functions). Each
// keeps its native side of the screen; everything else (menus, script text,
// projected markers) keeps the native aspect in the centered box. The
// multiplayer executable (also used for the title menus) has its own builds of
// these functions and no alarm box or sound meter.
struct Element { const char *single, *multiplayer; int8_t sideX, sideY; };
constexpr auto lifeBar = "90 FF BD 27 48 00 B0 AF 4C 00 B1 AF 25 80 80 00 25 88 A0 00 ?? ?? 04 34";
constexpr auto interactIcons = "60 FF BD 27 7C 00 B2 AF 88 00 B5 AF 25 A8 80 00 25 90 A0 00 80 00 B3 AF";
constexpr auto stealthMeter = "90 FF BD 27 48 00 B0 AF 25 80 80 00 ?? ?? 04 8E 4C 00 B1 AF 25 88 A0 00";
constexpr auto goggleBattery = "B0 FF BD 27 ?? ?? 86 8C 38 00 B1 AF 25 88 80 00 ?? ?? C4 8C 30 00 B4 E7";
constexpr auto goalNoteRecon = "C0 FF BD 27 ?? ?? 86 8C 24 00 B1 AF 25 88 80 00 ?? ?? C4 8C 20 00 B0 AF";
constexpr auto currentGoal = "70 FF BD 27 30 00 A0 AF 34 00 A0 AF 38 00 A0 AF 3C 00 A0 A3 40 00 A0 AF";
constexpr auto secondAmmo = "70 FF BD 27 20 00 A0 AF 5C 00 B0 AF 25 80 80 00 24 00 A0 AF ?? ?? 04 8E";
constexpr auto item = "70 FF BD 27 64 00 B0 AF 25 80 80 00 ?? ?? 04 8E 6C 00 B2 AF 25 90 A0 00";
constexpr Element elements[] = {
    {},
    {lifeBar, lifeBar, 1, 1},                                                                // DrawLifeBarPSP
    {"C0 FF BD 27 24 00 B0 AF 25 80 A0 00 ?? ?? 05 8E 28 00 B1 AF 25 88 80 00",
     "C0 FF BD 27 20 00 B0 AF 25 80 A0 00 ?? ?? 05 8E 24 00 B1 AF 25 88 80 00", -1, 1},       // DrawTrainingBoxPSP
    {"E0 FF BD 27 14 00 B0 AF 25 80 A0 00 ?? ?? 05 8E 18 00 B1 AF 25 88 80 00",
     "E0 FF BD 27 14 00 B1 AF 25 88 A0 00 ?? ?? 25 8E 10 00 B0 AF 25 80 80 00", -1, 1},       // DrawTrainingText
    {interactIcons, interactIcons, 1, 1},                                                    // DisplayInteractIcons
    {"80 FF BD 27 20 00 A0 AF 24 00 A0 AF 28 00 A0 AF 2C 00 A0 A3 30 00 A0 AF", nullptr, 1, 1}, // DrawAlarmBox
    {"F0 FE BD 27 38 00 A0 AF 3C 00 A0 AF 40 00 A0 AF E4 00 B1 AF 25 88 80 00",
     "00 FF BD 27 38 00 A0 AF 3C 00 A0 AF 40 00 A0 AF DC 00 B1 AF 25 88 80 00", 1, 1},       // DrawHandItemPSP
    {stealthMeter, stealthMeter, 1, 1},                                                      // DrawStealthMeterPSP
    {goggleBattery, goggleBattery, 1, 0},                                                    // DrawGoggleBatteryMeterPSP
    {goalNoteRecon, goalNoteRecon, 1, 1},                                                    // DisplayIconsGoalNoteRecon
    {currentGoal, currentGoal, -1, 1},                                                       // DisplayCurrentGoalPSP
    {secondAmmo, secondAmmo, 1, 1},                                                          // DrawSecondammo
    {item, item, 1, 1},                                                                      // DrawItem
    {"A0 FF BD 27 48 00 B0 AF 25 80 80 00 ?? ?? 04 8E ?? ?? 84 8C ?? ?? 84 8C", nullptr, 1, 1}, // DrawSoundMeterPSP
    {nullptr, nullptr, -1, -1},                                                                 // ECommunicationBox
};
// HUD parts drawn by UnrealScript: the script object issuing the canvas call
// (FFrame::Object) is matched by its class name.
struct ScriptElement { uint32_t name; int id; };
constexpr ScriptElement scriptElements[] = {
    {NameCrc("ECommunicationBox"), 14},
};
// Canvas natives called from script, as (this, FFrame&, result): signature,
// offset of the function start from the match, and which match.
struct CanvasNative { const char* signature; int offset, index; };
// AETextureManager::execDrawTileStretchedFromManager, execDrawFullTileFromManager
// and UCanvas::execDrawTileJustified share this start, in this order.
constexpr auto stretchedFromManager = "B0 FF BD 27 38 00 B1 AF 25 88 A0 00 3C 00 B2 AF 20 00 A0 AF 0C 00 32 26 00 00 45 8E 08 00 26 8E 01 00 A7 24 00 00 47 AE 00 00 A5 90 34 00 B0 AF 40 00 B3 AF 25 80 80 00 ?? ?? 13 3C C0 20 05 00";
constexpr CanvasNative canvasNatives[] = {
    {"B0 FF BD 27 10 00 A0 AF 14 00 A0 AF 28 00 B1 AF 25 88 A0 00 18 00 A0 AF 34 00 B4 AF", 0, 0}, // UCanvas::execDrawText
    {"A0 FF BD 27 44 00 B1 AF 25 88 A0 00 48 00 B2 AF 20 00 A0 AF 0C 00 32 26 00 00 45 8E 08 00 26 8E 01 00 A7 24 00 00 47 AE 00 00 A5 90 40 00 B0 AF 4C 00 B3 AF 25 80 80 00 ?? ?? 13 3C C0 20 05 00 ?? ?? 73 26 21 38 93 00 00 00 E4 84 02 00 E5 84 21 20 C4 00 3C 00 B4 E7", 0, 0}, // UCanvas::execDrawTile
    {"60 FF BD 27 20 00 A0 AF 24 00 A0 AF 74 00 B1 AF 25 88 A0 00 28 00 A0 AF", 0, 0}, // UCanvas::execDrawTextClipped
    {"40 FF BD 27 A8 00 B1 AF 25 88 A0 00 AC 00 B2 AF 40 00 A0 AF ?? ?? 32 26", 0, 0}, // UCanvas::execDrawTileClipped
    {"00 00 8C C4 28 00 AC E7 0C 00 24 8E 00 00 05 8E 01 00 84 24 0C 00 24 AE 78 01 A5 24 00 00 A4 84", -412, 0}, // UCanvas::execDrawTileStretched
    {"A0 FF BD 27 20 00 A0 AF 24 00 A0 AF 44 00 B1 AF 25 88 A0 00 28 00 A0 AF", 0, 0}, // UECanvas::execDrawTextAligned
    {"A0 FF BD 27 40 00 B1 AF 25 88 A0 00 44 00 B2 AF 20 00 A0 AF 0C 00 32 26 00 00 45 8E 08 00 26 8E 01 00 A7 24 00 00 47 AE 00 00 A5 90 3C 00 B0 AF 48 00 B3 AF 25 80 80 00 ?? ?? 13 3C C0 20 05 00 ?? ?? 73 26 21 38 93 00 00 00 E4 84 02 00 E5 84 21 20 C4 00 04 00 E6 8C", 0, 0}, // AEchelonMainHUD::execDrawBox
    {"80 FF BD 27 58 00 B1 AF 25 88 A0 00 5C 00 B2 AF 30 00 A0 AF 0C 00 32 26", 0, 0}, // AEQInvHUD::execDrawBox
    {"90 FF BD 27 58 00 B1 AF 25 88 A0 00 5C 00 B2 AF 30 00 A0 AF ?? ?? 32 26", 0, 0}, // AETextureManager::execDrawTileFromManager
    {stretchedFromManager, 0, 0},
    {stretchedFromManager, 0, 1},
    {stretchedFromManager, 0, 2},
};
constexpr int canvasNativeCount = sizeof(canvasNatives) / sizeof(canvasNatives[0]);
constexpr int elementCount = sizeof(elements) / sizeof(elements[0]);
SafetyMipsInline elementHooks[elementCount];
int element; // outermost native HUD element being drawn
template<int Id> int DrawElement(uintptr_t a0, uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t t0, uintptr_t t1, uintptr_t t2, uintptr_t t3) {
    const int previous = element;
    if (!previous) element = Id;
    const int result = elementHooks[Id].call<int>(a0, a1, a2, a3, t0, t1, t2, t3);
    element = previous;
    return result;
}
SafetyMipsInline canvasHooks[canvasNativeCount];
int ScriptElementOf(uintptr_t object) {
    // UObject::Class (+0x24) -> UObject::Name (+0x20).
    if (!object) return 0;
    const auto type = *reinterpret_cast<const uintptr_t*>(object + 0x24);
    if (!type) return 0;
    const auto name = *reinterpret_cast<const uint32_t*>(type + 0x20);
    for (const auto& entry : scriptElements) if (entry.name == name) return entry.id;
    return 0;
}
template<int Id> void CanvasCall(uintptr_t self, uintptr_t stack, uintptr_t result) {
    const int previous = element;
    if (!previous) element = ScriptElementOf(*reinterpret_cast<const uintptr_t*>(stack + 8));
    canvasHooks[Id].call<void>(self, stack, result);
    element = previous;
}
template<int Id> void HookCanvasNatives() {
    if constexpr (Id < canvasNativeCount) {
        canvasHooks[Id] = safetymips::create_inline(pattern.get(canvasNatives[Id].index, canvasNatives[Id].signature, canvasNatives[Id].offset), CanvasCall<Id>);
        HookCanvasNatives<Id + 1>();
    }
}
template<int Id> void HookElements(bool multiplayer) {
    if constexpr (Id < elementCount) {
        if (const auto signature = multiplayer ? elements[Id].multiplayer : elements[Id].single)
            elementHooks[Id] = safetymips::create_inline(pattern.get_first(signature, 0), DrawElement<Id>);
        HookElements<Id + 1>(multiplayer);
    }
}
SafetyMipsMid tiles, screenInput;
SafetyMipsInline worldOutput, vectorOutput;
int WorldToScreen(uintptr_t object, uintptr_t frame, float* result) {
    const int status = worldOutput.call<int>(object, frame, result);
    // The native result is already in display pixels. Undo the later screen
    // transform so projected targets receive that transform only once.
    if (result) UnprojectScreen(result);
    return status;
}
struct VectorArray { float (*data)[4]; unsigned size, capacity; };
int VectorWorldToScreen(uintptr_t object, VectorArray* vectors, void* location, void* rotation) {
    const int status = vectorOutput.call<int>(object, vectors, location, rotation);
    if (vectors && vectors->data && vectors->size <= vectors->capacity)
        for (unsigned i = 0; i < vectors->size; ++i) UnprojectScreen(vectors->data[i]);
    return status;
}
}
bool InstallHud() {
    const auto drawTile = sites::DrawTile();
    const auto worldToScreen = sites::WorldToScreen();
    const auto vectorWorldToScreen = sites::VectorWorldToScreen();
    const auto screenTransform = sites::ScreenTransform(sites::ScreenToWorld());
    if (!drawTile || !worldToScreen || !vectorWorldToScreen || !screenTransform) return false;
    for (int i = 1; i < elementCount; ++i)
    {
        const auto signature = sites::multiplayer ? elements[i].multiplayer : elements[i].single;
        if (signature && !pattern.get_first(signature, 0)) return false;
    }
    for (const auto& entry : canvasNatives)
        if (!pattern.get(entry.index, entry.signature, entry.offset)) return false;
    hudScale = console::bounded(inireader.ReadFloat("MAIN", "HudScale", 1), 0.5f, 1.5f, 1);
    UpdateDisplay();
    safetymips::Options replace{}; replace.instructions = 13; replace.execute_original = false;
    tiles = safetymips::create_mid(drawTile + 0x70, [](SafetyMipsContext& regs) {
        auto stack = reinterpret_cast<float*>(uintptr_t(regs.sp));
        stack[0x44 / 4] = regs.f19; // Displaced SWC1 in the native branch delay slot.
        float x0 = regs.f30, y0 = regs.f28, x1 = regs.f2, y1 = regs.f0;
        if (regs.a3) { x0 *= 0.75f; x1 *= 0.75f; y0 *= 272.0f / 480; y1 *= 272.0f / 480; }
        if (element) {
            const auto& at = elements[element];
            x0 = HudX(x0, at.sideX); x1 = HudX(x1, at.sideX);
            y0 = HudY(y0, at.sideY); y1 = HudY(y1, at.sideY);
        } else {
            // Full-screen fades and visor masks cover the screen; full-width
            // bands (menu bands, letterboxing) span its width at their height.
            const bool fullWidth = x0 <= 1 && x1 >= 479, fullScreen = fullWidth && y1 - y0 >= 270;
            const bool band = fullWidth && y1 - y0 <= 55;
            if (!fullScreen) { y0 = screen.y(y0); y1 = screen.y(y1); }
            if (!fullScreen && !band) { x0 = screen.x(x0); x1 = screen.x(x1); }
        }
        regs.f30 = x0; regs.f28 = y0;
        stack[0x34 / 4] = x1; stack[0x30 / 4] = y1;
    }, replace);
    HookElements<1>(sites::multiplayer);
    HookCanvasNatives<0>();
#ifndef NDEBUG
    for (int i = 1; i < elementCount; ++i) logger.WriteF("element %d: %d", i, int(elementHooks[i].error()));
    for (int i = 0; i < canvasNativeCount; ++i) logger.WriteF("native %d: %d", i, int(canvasHooks[i].error()));
#endif
    worldOutput = safetymips::create_inline(worldToScreen, WorldToScreen);
    vectorOutput = safetymips::create_inline(vectorWorldToScreen, VectorWorldToScreen);
    screenInput = safetymips::create_mid(screenTransform, [](SafetyMipsContext& regs) {
        auto point = reinterpret_cast<float*>(uintptr_t(regs.fp));
        point[0] = screen.x(point[0]); point[1] = screen.y(point[1]);
    });
    return true;
}
}
