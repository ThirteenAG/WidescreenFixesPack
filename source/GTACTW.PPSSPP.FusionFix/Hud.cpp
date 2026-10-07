#include "Game.hpp"

namespace ctw {
namespace {
SafetyMipsInline projectionWriter;
SafetyMipsInline radarRenderer;
SafetyMipsInline primitiveRenderer;
injector::hook_back<void(uint32_t**, const float*)> writeProjection;
injector::hook_back<void(void*, int, float)> sprites;
injector::hook_back<void(void*, float)> sprite;
injector::hook_back<void(void*)> radar;
injector::hook_back<void(uint32_t**, int, uint32_t, unsigned, uintptr_t, uintptr_t)> primitive;
float currentProjection[16];
float activeProjection[16];
bool haveProjection;
float scale = 1.0f;
bool drawingHudSprites;
struct HudAnchor { float x, y; };
struct HintCluster {
    uintptr_t sprites[8]{};
    HudAnchor anchor{};
    bool contains(uintptr_t instance) const {
        for (const auto part : sprites)
            if (part && (part == instance ||
                (Guest(part, 76) && at<uintptr_t>(part + 8) == instance))) return true;
        return false;
    }
} hint;

// Sprite type byte: 0x0E is a text string, 0x10 its drop shadow, which is
// parented (+8) to the string it follows.
inline constexpr uint8_t textSprite = 0x0E, textShadow = 0x10;

// The button-help bar shown under gameplay overlays such as weapon select
// ("Navigate  Select  Cancel") is one bottom-row text line starting at the
// left margin. Its labels are laid out across the native 480 width, so a
// left-edge anchor detached it from the centred overlay above it. Keep the
// whole line centred, like the same bar on PDA and apartment screens.
bool HelpBarText(uintptr_t instance) {
    if (at<uint8_t>(instance + 19) == textShadow) {
        const auto parent = at<uintptr_t>(instance + 8);
        if (!Guest(parent, 76)) return false;
        instance = parent;
    }
    if (at<uint8_t>(instance + 19) != textSprite) return false;
    const int x = at<int16_t>(instance + 26) + at<int16_t>(instance + 30);
    const int y = 272 + at<int16_t>(instance + 32) - at<int16_t>(instance + 28);
    return x >= 0 && x <= 8 && y >= 0 && y <= 24;
}

HudAnchor SpriteAnchor(uintptr_t instance) {
    if (HelpBarText(instance)) return {240.0f, 0.0f};
    const float x = float(at<int16_t>(instance + 26) + at<int16_t>(instance + 30));
    const float y = float(272 + at<int16_t>(instance + 32) - at<int16_t>(instance + 28));
    return {x <= 120 ? 0.0f : x >= 360 ? 480.0f : 240.0f,
            y <= 96 ? 0.0f : y >= 176 ? 272.0f : 136.0f};
}

void FindHintCluster() {
    hint = {};
    // The native help layout owns a panel top/bottom, four middle sections,
    // an icon and the head string. Text/shadow positions can cross the generic
    // corner thresholds even though all eight belong to this same panel.
    const auto parts = Address<0x08B5B9FC>();
    hint.sprites[0] = at<uintptr_t>(parts + 8);
    hint.sprites[1] = at<uintptr_t>(parts + 4);
    hint.sprites[2] = at<uintptr_t>(parts);
    for (unsigned i = 0; i != 4; ++i)
        hint.sprites[3 + i] = at<uintptr_t>(Address<0x08D59010>() + i * 4);
    const auto queue = Address<0x08D58F88>();
    const auto head = at<uintptr_t>(queue);
    if (head != queue - 8 && Guest(head, 48)) hint.sprites[7] = at<uintptr_t>(head + 20);
    const auto panel = hint.sprites[0];
    if (Guest(panel, 76)) hint.anchor = SpriteAnchor(panel);
    else hint = {};
}

// The frame's GE buffer (context+4 .. context+8, 612 KiB) holds the command
// list growing up from *context and vertex data growing down from
// context+12. Native code never checks the gap. Each projection write costs 17
// words; extra writes need a reserve for the rest of the frame plus a per-frame
// cap. A new frame restarts the list, so a lower cursor resets the count.
inline constexpr uint32_t projectionBytes = 17 * 4;
inline constexpr uint32_t geReserve = 48 * 1024;
inline constexpr uint32_t extraBudget = 16 * 1024;
uint32_t extraBytes, lastCursor;
#if defined(DEBUG) || defined(_DEBUG)
uint32_t minimumFree = ~0u;
#endif
bool GeRoom(unsigned writes) {
    const auto context = Address<0x08C11EC0>();
    const uint32_t cursor = at<uint32_t>(context), top = at<uint32_t>(context + 12);
    if (cursor < lastCursor) extraBytes = 0;
    lastCursor = cursor;
    if (!Guest(cursor) || top <= cursor) return false;
    const uint32_t needed = writes * projectionBytes;
#if defined(DEBUG) || defined(_DEBUG)
    if (top - cursor < minimumFree) minimumFree = top - cursor;
#endif
    if (top - cursor < geReserve + needed || extraBytes + needed > extraBudget) return false;
    extraBytes += needed;
    return true;
}

// HUD projection state inside one sprite-layer pass. Consecutive sprites with
// the same anchor share one projection write; the native projection is written
// back once when room runs out or the pass ends. A native
// projection write in between (WriteProjection) invalidates the shared state.
struct HudState {
    bool adjusted = false;
    HudAnchor anchor{};
} hud;

void EmitProjection(uint32_t** context, const float* matrix) {
    if (context == reinterpret_cast<uint32_t**>(Address<0x08C11EC0>()))
        memcpy(activeProjection, matrix, sizeof(activeProjection));
    writeProjection.fun(context, matrix);
}

void FitProjection(float* output, const float* input) {
    memcpy(output, input, sizeof(currentProjection));
    const float horizontal = HorizontalScale();
    for (unsigned column = 0; column != 4; ++column)
        output[column * 4] = input[column * 4] * horizontal;
}

// The native writer copies matrix values into GE commands immediately. These
// stack matrices therefore never become asynchronous vertex-buffer pointers.
void WriteProjection(uint32_t** context, const float* matrix) {
    if (context == reinterpret_cast<uint32_t**>(Address<0x08C11EC0>())) {
        memcpy(currentProjection, matrix, sizeof(currentProjection));
        haveProjection = true;
        hud.adjusted = false;
        // Orthographic PDA, menu, map and loading artwork stays centred at
        // its original proportions instead of stretching with the viewport.
        // Perspective matrices already use the corrected native camera.
        if (matrix[3] == 0 && matrix[7] == 0 && matrix[11] == 0 && matrix[15] != 0) {
            RefreshAspect();
            float adjusted[16]; FitProjection(adjusted, matrix);
            EmitProjection(context, adjusted);
            return;
        }
    }
    EmitProjection(context, matrix);
}

bool GameplayHud() {
    return haveProjection && GameplayApp();
}

bool AnchoredProjection(HudAnchor anchor) {
    const auto& base = currentProjection;
    // Compose in clip space around the chosen native screen coordinate.
    // CTW's projection has w=2048, rather than the usual w=1.
    const float w = base[3] * anchor.x + base[7] * anchor.y + base[15];
    if (w == 0.0f) return false;
    // Adjusting needs room for this write and the later restore.
    if (!GeRoom(hud.adjusted ? 1 : 2)) return false;
    float adjusted[16];
    memcpy(adjusted, base, sizeof(adjusted));
    const float x = (base[0] * anchor.x + base[4] * anchor.y + base[12]) / w;
    const float y = (base[1] * anchor.x + base[5] * anchor.y + base[13]) / w;
    const float horizontal = scale * HorizontalScale();
    for (unsigned column = 0; column != 4; ++column) {
        adjusted[column * 4] = base[column * 4] * horizontal + (1.0f - horizontal) * x * base[column * 4 + 3];
        adjusted[column * 4 + 1] = base[column * 4 + 1] * scale + (1.0f - scale) * y * base[column * 4 + 3];
    }
    EmitProjection(reinterpret_cast<uint32_t**>(Address<0x08C11EC0>()), adjusted);
    hud.adjusted = true;
    hud.anchor = anchor;
    return true;
}

void RestoreHudProjection() {
    if (!hud.adjusted) return;
    hud.adjusted = false;
    // Restore before PDA overlays, fullscreen fades or the next render layer.
    // Its room was reserved together with the first adjusted write.
    float fitted[16]; FitProjection(fitted, currentProjection);
    EmitProjection(reinterpret_cast<uint32_t**>(Address<0x08C11EC0>()), fitted);
}

void UseHudAnchor(HudAnchor anchor) {
    if (hud.adjusted && hud.anchor.x == anchor.x && hud.anchor.y == anchor.y) return;
    // Without room the sprite keeps the native (fitted) projection.
    if (!AnchoredProjection(anchor)) RestoreHudProjection();
}

// The apartment (application 50) is a panned 2D room: its layers move with
// the pan and stop exactly at the native 0/480 edges at either end, with no
// art beyond the background there; foreground pieces (shelves) continue as
// stretched strips. Under the fitted projection more than 480 units are
// visible, so the ends showed a black band on one side and smeared strips on
// the other. The room is rendered into an off-screen target and composited
// before the sprite passes; after each sprite pass the framebuffer columns
// outside the native 480-unit area are cleared to black with the game's own
// clear-mode rectangles (as its screen clear helper, US 0x08910C1C, draws
// them), so the room stays centred with clean pillarbox bars like the PDA
// screens at any pan position.
bool ApartmentApp() {
    const auto app = at<uintptr_t>(Address<0x08C11258>());
    return Guest(app, 200) && at<int>(app + 196) == 50;
}

void ClearSideBars() {
    const float horizontal = HorizontalScale();
    if (horizontal >= 1.0f || !ApartmentApp()) return;
    // Framebuffer columns covered by native x 0..480 under the fitted projection.
    const unsigned left = unsigned(240.0f * (1.0f - horizontal) + 0.5f);
    if (left == 0 || left >= 240 || !GeRoom(2)) return;
    const auto context = reinterpret_cast<uint32_t**>(Address<0x08C11EC0>());
    // Two through-mode rectangles, 12-byte vertices (colour, x | y << 16, depth),
    // taken from the top of the frame's vertex area like the native helper.
    auto& top = at<uintptr_t>(Address<0x08C11EC0>() + 12);
    top = (top - 4 * 12) & ~uintptr_t(3);
    const auto vertices = reinterpret_cast<uint32_t*>(top);
    const uint32_t right = 480 - left;
    const uint32_t corners[4][2] = {{0, 0}, {left, 272}, {right, 0}, {480, 272}};
    for (unsigned i = 0; i != 4; ++i) {
        vertices[i * 3] = 0xFF000000u;
        vertices[i * 3 + 1] = corners[i][0] | corners[i][1] << 16;
        vertices[i * 3 + 2] = 0;
    }
    *(*context)++ = 0xD3000101u;   // CLEAR on, colour buffer only
    primitive.fun(context, 6, 0x80011C, 4, 0, top);
    *(*context)++ = 0xD3000000u;   // CLEAR off
}

void DrawSprites(void* manager, int layer, float alpha) {
    const bool previous = drawingHudSprites;
    drawingHudSprites = GameplayHud();
    if (drawingHudSprites) FindHintCluster();
    sprites.fun(manager, layer, alpha);
    if (drawingHudSprites) RestoreHudProjection();
    else ClearSideBars();
    drawingHudSprites = previous;
}

void DrawSprite(void* instance, float alpha) {
    if (!drawingHudSprites) { sprite.fun(instance, alpha); return; }
    const auto address = reinterpret_cast<uintptr_t>(instance);
    // Native sprites use bottom-left screen coordinates, including text via
    // model matrices. Keep corner clusters together; centred notifications stay
    // centred. The radar's entire 96x96 cluster uses the bottom-left anchor.
    UseHudAnchor(hint.contains(address) ? hint.anchor : SpriteAnchor(address));
    sprite.fun(instance, alpha);
}

void DrawRadar(void* instance) {
    if (!GameplayHud()) { radar.fun(instance); return; }
    // Radar tiles, stencil mask and health ring are drawn before the sprite
    // manager draws the frame/blips. Both passes need the same projection.
    UseHudAnchor({0.0f, 0.0f});
    radar.fun(instance);
    RestoreHudProjection();
}

void DrawPrimitive(uint32_t** context, int type, uint32_t format, unsigned count, uintptr_t indices, uintptr_t vertices) {
    // Both the rectangle helper and the cinematic renderer submit solid quads
    // here. Textured artwork and world geometry retain their own projection.
    bool fullWidth = false;
    if (haveProjection && context == reinterpret_cast<uint32_t**>(Address<0x08C11EC0>()) &&
        currentProjection[3] == 0 && currentProjection[7] == 0 && currentProjection[11] == 0 &&
        currentProjection[15] != 0 && !indices && vertices &&
        (((type == 4 || type == 5) && count == 4) || (type == 6 && count == 2)) &&
        (format == 0x100 || format == 0x11C)) {
        const unsigned stride = format == 0x100 ? 6 : 12;
        const unsigned offset = format == 0x100 ? 0 : 4;
        if (Guest(vertices, stride * count)) {
            int left = 32767, right = -32768;
            for (unsigned i = 0; i != count; ++i) {
                const int x = at<int16_t>(vertices + stride * i + offset);
                if (x < left) left = x;
                if (x > right) right = x;
            }
            fullWidth = left <= 0 && right >= 480;
        }
    }
    if (!fullWidth || !GeRoom(2)) { primitive.fun(context,type,format,count,indices,vertices); return; }
    float saved[16]; memcpy(saved,activeProjection,sizeof(saved));
    EmitProjection(context,currentProjection);
    primitive.fun(context,type,format,count,indices,vertices);
    EmitProjection(context,saved);
}
}

void InstallHud() {
    scale = console::bounded(inireader.ReadFloat("MAIN", "HUDScale", 1.0f), 0.5f, 1.0f, 1.0f);
    projectionWriter = safetymips::create_inline(Address<0x08988CA4>(), WriteProjection);
    writeProjection.fun = projectionWriter.original<void(*)(uint32_t**, const float*)>();
    const auto spriteManager = injector::GetBranchDestination(Address<0x0898D388>()).as_int();
    sprites.fun = injector::MakeCALL(Address<0x0898D388>(), DrawSprites).get();
    // The per-instance draw call is fingerprinted by its own address rule; only
    // hook it when it is the call inside the manager the layer call reaches.
    if (Address<0x088E12B8>() == spriteManager + 0x3C)
        sprite.fun = injector::MakeCALL(Address<0x088E12B8>(), DrawSprite).get();
    radarRenderer = safetymips::create_inline(Address<0x089E7270>(), DrawRadar);
    radar.fun = radarRenderer.original<void(*)(void*)>();
    primitiveRenderer = safetymips::create_inline(Address<0x089100C0>(), DrawPrimitive);
    primitive.fun = primitiveRenderer.original<void(*)(uint32_t**, int, uint32_t, unsigned, uintptr_t, uintptr_t)>();
}
}
