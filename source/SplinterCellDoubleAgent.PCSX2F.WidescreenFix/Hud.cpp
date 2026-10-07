#include "Game.hpp"
#include <cstring>

namespace scda {
namespace {
struct alignas(16) DrawRect {
    float left, top, right, bottom, u1, v1, u2, v2;
    uint32_t color, texture;
};
static_assert(offsetof(DrawRect, texture) == 36);
safetymips::GameInline<uint64_t(void*)> image, border, nodeRender, childRender;
safetymips::GameInline<uint64_t(void*, void*, int32_t, int32_t, void*, int32_t, float)> text;
safetymips::GameInline<uint64_t(void*, DrawRect*)> rect;
SafetyMipsMid quad, vertex;
unsigned magmaDepth;
console::Transform quadTransform;
bool transformQuad;
struct Scope { Scope() { ++magmaDepth; } ~Scope() { --magmaDepth; } };

// Wide HUD. The gameplay HUD is the Magma package "hud" ("hud_split" and
// "hud_split2" for the coop players). Its root node draws about 40 top-level
// groups (radar/stealth meter, weapon, gadget, messages, objectives, timers,
// cinematic bars, ...), each a child wrapper {vtbl, CRC32 id, object}. Each
// visible group is measured from its template layout (area offsets, element
// rectangles) once per frame, so menus, pause, the inventory and the OPSAT
// (other packages) are never touched and no ID table is needed:
// - its frame: the 4:3 screen, or in coop the player's half when the group lies
//   in one half of the shared 640-unit layout (messages centred on the whole
//   screen keep the full frame);
// - its side: the edge it is laid out against (left gap against right gap);
// - a group inside another sided group (parts of the radar cluster) follows it.
// The frame is contained like the rest of the UI, then a sided group moves by
// the frame's margin to its edge. Groups without measurable elements (lists
// filled at run time) use the extent they drew in the previous frame.
struct Box { float left = 1e9f, right = -1e9f, top = 1e9f, bottom = -1e9f; };
bool Empty(const Box& b) { return b.left > b.right || b.top > b.bottom; }
void Add(Box& b, float l, float r, float t, float u) {
    if (l < b.left) b.left = l;
    if (r > b.right) b.right = r;
    if (t < b.top) b.top = t;
    if (u > b.bottom) b.bottom = u;
}
float Area(const Box& b) { return (b.right - b.left) * (b.bottom - b.top); }
bool Inside(const Box& a, const Box& b) {
    constexpr float slack = 4;
    return a.left >= b.left - slack && a.right <= b.right + slack && a.top >= b.top - slack && a.bottom <= b.bottom + slack;
}
struct Group {
    uintptr_t item;
    Box box;             // base units
    float center, half;  // frame centre and half width, in viewport widths
    int8_t side;
    bool valid, layout;  // measured (or seen drawn) / hidden or has template elements
};
constexpr unsigned MaxGroups = 64;
Group groups[MaxGroups];  // the groups of the hud root being drawn
unsigned groupCount;
// Extents drawn by groups without a measurable layout, in base units: the last
// complete frame of their root (box) and the one being drawn (next).
struct Drawn { uintptr_t item, root; Box box, next; };
constexpr unsigned MaxDrawn = 16;
Drawn drawn[MaxDrawn];
unsigned drawnNext;
uintptr_t hudRoot;        // the hud root node being drawn, else 0
unsigned hudChildDepth;   // child depth of its top-level groups
unsigned childDepth;
const Group* group;       // top-level group being drawn
Drawn* drawing;           // its drawn-extent record when it has no layout
bool wideHud;

bool Valid(uintptr_t address) { return address >= 0x100000 && address < 0x2000000 && !(address & 3); }
// Magma strings: {flags | length << 1, inline chars} or {flags | 1, length, chars*}.
// Returns 1 for "hud", 2 for the coop "hud_split" and "hud_split2".
int HudName(uintptr_t string) {
    const uint32_t head = Read<uint32_t>(string);
    uint32_t length; const char* chars;
    if (head & 1) {
        length = Read<uint32_t>(string + 4);
        const auto data = Read<uintptr_t>(string + 8);
        if (!data || data >= 0x2000000) return 0;
        chars = reinterpret_cast<const char*>(data);
    } else {
        length = (head & 0xFF) >> 1;
        chars = reinterpret_cast<const char*>(string + 1);
    }
    if (length == 3 && !std::memcmp(chars, "hud", 3)) return 1;
    if (length == 9 && !std::memcmp(chars, "hud_split", 9)) return 2;
    if (length == 10 && !std::memcmp(chars, "hud_split2", 10)) return 2;
    return 0;
}
// A root node {vtbl, capacity, count, children} belongs to the hud package when
// its area children {vtbl, &element, element, ..., +32 package} point to it.
int HudRoot(uintptr_t node) {
    const auto count = Read<uint32_t>(node + 8);
    const auto children = Read<uintptr_t>(node + 12);
    if (!count || count > 256 || !Valid(children)) return 0;
    for (uint32_t i = 0; i < count && i < 8; ++i) {
        const auto item = Read<uintptr_t>(children + 4 * i);
        if (!Valid(item)) return 0;
        const auto object = Read<uintptr_t>(item + 8);
        if (!Valid(object) || Read<uint32_t>(object) != game.areaInstance) continue;
        const auto package = Read<uintptr_t>(object + 32);
        return Valid(package) ? HudName(package + 56) : 0;
    }
    return 0;
}
// Extent of an object's elements in base units; x, y is its area origin.
void Measure(uintptr_t object, int x, int y, unsigned depth, Box& box, unsigned& budget) {
    if (depth > 8 || !budget || !Valid(object)) return;
    --budget;
    const auto element = Read<uintptr_t>(object + 4);
    if (!Valid(element)) return;
    const auto type = Read<uint32_t>(element);
    if (type == game.areaElement) {
        // Area and list objects keep their children {vtbl, capacity, count, items} at +28.
        x += Read<int16_t>(element + 8); y += Read<int16_t>(element + 10);
        const auto list = Read<uintptr_t>(object + 28);
        if (!Valid(list)) return;
        const auto count = Read<uint32_t>(list + 8);
        const auto items = Read<uintptr_t>(list + 12);
        if (!count || count > 64 || !Valid(items)) return;
        for (uint32_t i = 0; i < count; ++i) {
            const auto item = Read<uintptr_t>(items + 4 * i);
            if (Valid(item)) Measure(Read<uintptr_t>(item + 8), x, y, depth + 1, box, budget);
        }
    } else if (type == game.imageElement || type == game.textElement || type == game.rectElement) {
        const int l = x + Read<int16_t>(element + 8), r = x + Read<int16_t>(element + 10);
        const int t = y + Read<int16_t>(element + 12), b = y + Read<int16_t>(element + 14);
        if (l <= r && t <= b) Add(box, float(l), float(r), float(t), float(b));
    }
}
Drawn* FindDrawn(uintptr_t item, bool create) {
    for (auto& d : drawn) if (d.item == item) return &d;
    if (!create) return nullptr;
    Drawn& d = drawn[drawnNext++ % MaxDrawn];
    d = {item, hudRoot, {}, {}};
    return &d;
}
void Classify(uintptr_t root, bool split) {
    groupCount = 0;
    const auto count = Read<uint32_t>(root + 8);
    const auto children = Read<uintptr_t>(root + 12);
    const auto context = Read<uintptr_t>(game.context);
    const int baseW = Valid(context) ? Read<int16_t>(context + 132) : 0;
    if (baseW <= 0 || !Valid(children)) return;
    for (uint32_t i = 0; i < count && groupCount < MaxGroups; ++i) {
        const auto item = Read<uintptr_t>(children + 4 * i);
        if (!Valid(item)) continue;
        Group& g = groups[groupCount++];
        g = {item, {}, 0.5f, 0.5f, 0, false, false};
        // Hidden children (flag bit 7 of +21 clear) are skipped by the game.
        if (!(Read<uint8_t>(item + 21) & 0x80)) { g.layout = true; continue; }
        unsigned budget = 256;
        Measure(Read<uintptr_t>(item + 8), 0, 0, 0, g.box, budget);
        g.layout = !Empty(g.box);
        if (!g.layout) {
            const Drawn* d = FindDrawn(item, false);
            if (!d || Empty(d->box)) continue;
            g.box = d->box;
        }
        g.valid = true;
        const float left = g.box.left / float(baseW), right = g.box.right / float(baseW);
        const float middle = 0.5f * (left + right);
        if (split && (middle < 0.45f || middle > 0.55f)) { g.center = middle < 0.5f ? 0.25f : 0.75f; g.half = 0.25f; }
        // Gaps to the frame's edges, in frame widths.
        const float gapLeft = (left - (g.center - g.half)) / (2 * g.half);
        const float gapRight = ((g.center + g.half) - right) / (2 * g.half);
        g.side = gapLeft - gapRight < -0.25f ? -1 : gapLeft - gapRight > 0.25f ? 1 : 0;
    }
    // Parts drawn inside a sided group (stealth meter lights, weapon icon) follow it.
    for (unsigned i = 0; i < groupCount; ++i) {
        Group& g = groups[i];
        if (!g.valid) continue;
        const Group* owner = nullptr;
        for (unsigned j = 0; j < groupCount; ++j) {
            const Group& o = groups[j];
            if (j == i || !o.valid || !o.side || !Inside(g.box, o.box) || Area(o.box) <= Area(g.box)) continue;
            if (!owner || Area(o.box) > Area(owner->box)) owner = &o;
        }
        if (owner) { g.center = owner->center; g.half = owner->half; g.side = owner->side; }
    }
}
uint64_t NodeRender(void* node) {
    if (hudRoot || !wideHud) return nodeRender.call(node);
    const int kind = HudRoot(uintptr_t(node));
    if (!kind) return nodeRender.call(node);
    hudRoot = uintptr_t(node); hudChildDepth = childDepth;
    for (auto& d : drawn) if (d.root == hudRoot) { d.box = d.next; d.next = {}; }
    Classify(hudRoot, kind == 2);
    const auto result = nodeRender.call(node);
    hudRoot = 0; group = nullptr; drawing = nullptr;
    return result;
}
uint64_t ChildRender(void* child) {
    const bool top = hudRoot && childDepth == hudChildDepth;
    if (top) {
        group = nullptr; drawing = nullptr;
        for (unsigned i = 0; i < groupCount; ++i) {
            const Group& g = groups[i];
            if (g.item != uintptr_t(child)) continue;
            if (!g.layout) drawing = FindDrawn(g.item, true);
            if (g.valid) group = &g;
            break;
        }
    }
    ++childDepth; const auto result = childRender.call(child); --childDepth;
    if (top) { group = nullptr; drawing = nullptr; }
    return result;
}

bool Viewport(float& width, float& height, console::Transform& transform) {
    const auto engine = Read<uintptr_t>(game.engine), context = Read<uintptr_t>(game.context);
    if (!engine || !context) return false;
    const auto device = Read<uintptr_t>(engine + 48);
    if (!device) return false;
    auto canvas = Read<uintptr_t>(engine + 52);
    if (!canvas) {
        const auto owner = Read<uintptr_t>(device + 72);
        if (!owner) return false;
        const auto slot = Read<uintptr_t>(owner + 48);
        if (!slot) return false;
        canvas = Read<uintptr_t>(slot);
    }
    if (!canvas) return false;
    const int w = Read<int32_t>(canvas + 172), h = Read<int32_t>(canvas + 176);
    const int baseW = Read<int16_t>(context + 132), baseH = Read<int16_t>(context + 134);
    if (w <= 0 || h <= 0 || baseW <= 0 || baseH <= 0) return false;
    width = float(w); height = float(h);
    // The UI is laid out for the full 4:3 frame; a coop viewport is a part of
    // it. The full frame is the largest canvas seen (NTSC 640x448, PAL 512x512;
    // the Magma base size, 640x448 in both, is not the frame size on PAL).
    static int fullW, fullH;
    if (w > fullW) fullW = w;
    if (h > fullH) fullH = h;
    const float scale = (height / float(fullH)) / (width / float(fullW)) *
                        ((4.0f / 3.0f) / console::ps2Aspect(PCSX2Data));
    const float x = scale > 1.0f ? hudSize : scale * hudSize;
    const float y = scale > 1.0f ? hudSize / scale : hudSize;
    transform = {x, y, width * 0.5f * (1 - x), height * 0.5f * (1 - y)};
    return std::isfinite(scale) && scale > 0;
}
// The horizontal offset of the drawing group's transform (x' = x * scaleX + offset).
float GroupOffset(float left, float top, float right, float bottom, float width, float height, const console::Transform& transform) {
    if (drawing) {
        const auto context = Read<uintptr_t>(game.context);
        const float sx = float(Read<int16_t>(context + 132)) / width, sy = float(Read<int16_t>(context + 134)) / height;
        Add(drawing->next, left * sx, right * sx, top * sy, bottom * sy);
    }
    if (!group) return transform.offsetX;
    return (1 - transform.scaleX) * width * (group->center + group->side * group->half);
}
#ifdef WFP_PS2_DEBUG
// PINE-readable: magic 'MGDR', count, then {kind, l, r, t, b, texture, color, width} (floats as bits).
uint32_t drawTrace[2 + 8 * 256] = {0x5244474D};
void Trace(uint32_t kind, float l, float r, float t, float b, uint32_t texture, uint32_t color, float width) {
    if (!traceDraws || drawTrace[1] >= 256) return;
    float v[] = {l, r, t, b, width}; uint32_t bits[5]; std::memcpy(bits, v, sizeof(v));
    for (unsigned i = 0; i < drawTrace[1]; ++i) {
        const uint32_t* e = drawTrace + 2 + 8 * i;
        if (e[0] == kind && e[1] == bits[0] && e[2] == bits[1] && e[3] == bits[2] && e[4] == bits[3]) return;
    }
    uint32_t* e = drawTrace + 2 + 8 * drawTrace[1]++;
    e[0] = kind; e[1] = bits[0]; e[2] = bits[1]; e[3] = bits[2]; e[4] = bits[3]; e[5] = texture; e[6] = color; e[7] = bits[4];
}
// PINE-readable: magic 'GRUP', then the address, count and entry size of the
// group table of the last hud root drawn.
volatile uint32_t groupTrace[4] = {0x50555247, 0, 0, sizeof(Group)};
void TraceGroups() { groupTrace[1] = uint32_t(uintptr_t(groups)); groupTrace[2] = groupCount; }
#endif
bool Overlay(uint32_t color) {
    return color == 0x96DAFAEC || color == 0x96C3B081 || color == 0x00C3B081;
}
bool Black(uint32_t color) { return !(color & 0x00FFFFFF); }
bool SpansWidth(float left, float right, float width) { return left <= 1 && right >= width - 1; }
bool Full(float left, float top, float right, float bottom, float width, float height) {
    return SpansWidth(left, right, width) && top <= 1 && bottom >= height - 1;
}
uint64_t Draw(void* renderer, DrawRect* source) {
    float width, height; console::Transform transform;
    if (!magmaDepth || !source || !Viewport(width, height, transform) || Overlay(source->color)) return rect.call(renderer, source);
    DrawRect copy = *source;
#ifdef WFP_PS2_DEBUG
    Trace(0x43524746, copy.left, copy.right, copy.top, copy.bottom, copy.texture, copy.color, width);
    TraceGroups();
#endif
    const bool full = Full(copy.left, copy.top, copy.right, copy.bottom, width, height);
    // Screen fades remain full viewport; artwork is fitted without stretching.
    if (full && !copy.texture) return rect.call(renderer, source);
    if (full) {
        DrawRect background = copy;
        background.color = 0xFF000000; background.texture = 0;
        rect.call(renderer, &background);
    }
    // Black bars across the whole viewport (cinematic borders) keep their width;
    // only artwork and HUD elements are contained. Magma binds a default white
    // texture to solid fills, so the colour identifies them.
    if (!Black(copy.color) || !SpansWidth(copy.left, copy.right, width)) {
        const float offset = GroupOffset(copy.left, copy.top, copy.right, copy.bottom, width, height, transform);
        copy.left = copy.left * transform.scaleX + offset; copy.right = copy.right * transform.scaleX + offset;
    }
    copy.top = transform.y(copy.top); copy.bottom = transform.y(copy.bottom);
    return rect.call(renderer, &copy);
}
uint64_t Image(void* object) { Scope scope; return image.call(object); }
uint64_t Border(void* object) { Scope scope; return border.call(object); }
uint64_t Text(void* object, void* string, int32_t x, int32_t y, void* style, int32_t length, float scale) {
    Scope scope; return text.call(object, string, x, y, style, length, scale);
}
}
void InstallHud() {
    image.create(game.image, Image); text.create(game.text, Text); border.create(game.border, Border);
    rect.create(game.rect, Draw);
    wideHud = widescreenHud;
    if (wideHud) {
        nodeRender.create(game.nodeRender, NodeRender);
        childRender.create(game.childRender, ChildRender);
    }
    quad = safetymips::create_mid(game.quad, [](SafetyMipsContext& regs) {
        float width, height;
        transformQuad = magmaDepth && !Overlay(uint32_t(regs.a1)) && Viewport(width, height, quadTransform);
        if (!transformQuad) return;
        // The native quad rotates its vertices. Apply the aspect correction after
        // rotation, at its final vertex emitter, leaving UVs and rotation intact.
        const auto stack = uintptr_t(regs.sp);
        const float x[] = {regs.f12, regs.f16, Read<float>(stack), Read<float>(stack + 32)};
        const float y[] = {regs.f13, regs.f17, Read<float>(stack + 8), Read<float>(stack + 40)};
        float left = x[0], right = x[0], top = y[0], bottom = y[0];
        for (unsigned i = 1; i < 4; ++i) {
            if (x[i] < left) left = x[i];
            if (x[i] > right) right = x[i];
            if (y[i] < top) top = y[i];
            if (y[i] > bottom) bottom = y[i];
        }
#ifdef WFP_PS2_DEBUG
        Trace(0x44415146, left, right, top, bottom, uint32_t(regs.t1), uint32_t(regs.a1), width);
#endif
        if (!uint32_t(regs.t1) && Full(left, top, right, bottom, width, height)) transformQuad = false;
        else if (Black(uint32_t(regs.a1)) && SpansWidth(left, right, width)) { quadTransform.scaleX = 1; quadTransform.offsetX = 0; }
        else quadTransform.offsetX = GroupOffset(left, top, right, bottom, width, height, quadTransform);
    });
    vertex = safetymips::create_mid(game.vertex, [](SafetyMipsContext& regs) {
        if (transformQuad && magmaDepth && uintptr_t(regs.ra) >= game.quad && uintptr_t(regs.ra) < game.vertex) {
            regs.f12 = quadTransform.x(regs.f12); regs.f13 = quadTransform.y(regs.f13);
        }
    });
}
}
