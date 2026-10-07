#include "Game.hpp"
#include <cmath>
#include <cstring>

// 2D output is corrected in framebuffer space, after the game has laid it out
// for 480x272. Two paths reach the GE:
//  - Transformed geometry (the immediate-mode quads, HUD meshes, Flash shapes)
//    is placed by the viewport the engine emits; its GE viewport and scissor
//    words are rewritten as the engine emits them.
//  - Through-mode sprites, rectangles and glyph runs carry screen coordinates;
//    their freshly written vertices are rewritten in place.
// A transform is only active while a HUD element or a Flash movie draws, so
// the 3D scene and the full-screen post-processing passes are never touched.
namespace mc3 {
namespace {
uintptr_t context;          // GE context: +8 command cursor, +12 vertex top (grows downward)
uintptr_t currentViewport;  // address of the engine's current-viewport pointer
SafetyMipsInline viewportHook, spriteHook, glyphHook, rectHook, movieHook, flashGlyphHook, pageHook, videoHook, promptHook;
SafetyMipsMid elementHooks[40];
injector::hook_back<uint32_t(uintptr_t)> applyViewport;
injector::hook_back<uint32_t(int, int, int, int, int, int, int, int, unsigned, int, int, int)> sprite;
injector::hook_back<uint32_t(uintptr_t, uintptr_t, uintptr_t, int, unsigned)> glyphs;
injector::hook_back<uint32_t(int, int, int, int, unsigned, int)> rect;
injector::hook_back<uint32_t(uintptr_t, int)> movie;
injector::hook_back<uint32_t(uintptr_t, int, int, int, float)> flashGlyph;
injector::hook_back<uint32_t(uintptr_t)> page;
injector::hook_back<uint32_t(unsigned)> video;
injector::hook_back<void(uintptr_t)> prompt;
bool videoFrame;

// The engine's GE OFFSET places screen pixel 0 at 2048-240 / 2048-136.
constexpr float originX = 1808.0f, originY = 1912.0f;

float Float24(uint32_t word) {
    const uint32_t bits = word << 8; float value; std::memcpy(&value, &bits, 4); return value;
}
uint32_t Word24(uint32_t word, float value) {
    uint32_t bits; std::memcpy(&bits, &value, 4); return (word & 0xFF000000u) | (bits >> 8);
}
int Clamp(int value, int low, int high) { return value < low ? low : value > high ? high : value; }

uintptr_t& Cursor() { return at<uintptr_t>(context + 8); }
uintptr_t& VertexTop() { return at<uintptr_t>(context + 12); }

void PatchViewport(uintptr_t begin, uintptr_t end) {
    const auto& t = screen;
    for (auto p = begin; p + 4 <= end; p += 4) {
        auto& word = at<uint32_t>(p);
        switch (word >> 24) {
        case 0x42: word = Word24(word, Float24(word) * t.ax); break;
        case 0x43: word = Word24(word, Float24(word) * t.ay); break;
        case 0x45: word = Word24(word, t.x(Float24(word) - originX) + originX); break;
        case 0x46: word = Word24(word, t.y(Float24(word) - originY) + originY); break;
        case 0xD4: {
            const int x = Clamp(int(std::floor(t.x(float(word & 0x3FF)))), 0, 479);
            const int y = Clamp(int(std::floor(t.y(float((word >> 10) & 0x3FF)))), 0, 271);
            word = (word & 0xFF000000u) | unsigned(x) | (unsigned(y) << 10);
            break;
        }
        case 0xD5: {
            const int x = Clamp(int(std::ceil(t.x(float((word & 0x3FF) + 1)))) - 1, 0, 479);
            const int y = Clamp(int(std::ceil(t.y(float(((word >> 10) & 0x3FF) + 1)))) - 1, 0, 271);
            word = (word & 0xFF000000u) | unsigned(x) | (unsigned(y) << 10);
            break;
        }
        }
    }
}

// Through-mode vertices with 16-bit positions. Sprites and glyph runs use
// u16 u, v; s16 x, y, z (10 bytes); Flash glyphs add a colour (16 bytes).
// Masks (Expand) cover every pixel the transformed scissor admits: corner
// pairs round outward, so stencil clears never leave a stale edge row.
template<unsigned Stride, unsigned Position, bool Expand>
void PatchVertices(uintptr_t begin, uintptr_t end) {
    const auto& t = screen;
    unsigned index = 0;
    for (auto p = begin; p + Stride <= end; p += Stride, ++index) {
        auto& x = at<int16_t>(p + Position);
        auto& y = at<int16_t>(p + Position + 2);
        const float tx = t.x(float(x)), ty = t.y(float(y));
        if (Expand) {
            const bool last = index & 1;
            x = int16_t(last ? std::ceil(tx) : std::floor(tx));
            y = int16_t(last ? std::ceil(ty) : std::floor(ty));
        } else {
            x = int16_t(std::lround(tx));
            y = int16_t(std::lround(ty));
        }
    }
}

uint32_t ApplyViewport(uintptr_t viewport) {
    if (!screen.active) return applyViewport.fun(viewport);
    const auto begin = Cursor();
    const auto result = applyViewport.fun(viewport);
    const auto end = Cursor();
    if (end > begin && end - begin < 0x100) PatchViewport(begin, end);
    return result;
}

// Re-emit the current viewport so the GE state follows a transform change.
void Reapply() {
    const auto viewport = at<uintptr_t>(currentViewport);
    if (!Guest(viewport, 0x1C0)) return;
    at<uint32_t>(viewport + 0x130) |= 1;
    ApplyViewport(viewport);
}

void SetTransform(const ScreenTransform& value) {
    const bool changed = screen.active || value.active;
    screen = value;
    if (changed) Reapply();
}

template<unsigned Stride = 10, unsigned Position = 4, bool Expand = false, class Draw> uint32_t Through(bool fullWidth, Draw draw) {
    if (!screen.active || fullWidth) return draw();
    const auto top = VertexTop();
    const auto result = draw();
    const auto bottom = VertexTop();
    if (bottom < top && top - bottom < 0x10000) PatchVertices<Stride, Position, Expand>(bottom, top);
    return result;
}

uint32_t Sprite(int x0, int y0, int x1, int y1, int u0, int v0, int u1, int v1, unsigned color, int a10, int a11, int a12) {
    if (videoFrame) {
        // Pillarboxed movie frame: clear the bars, then draw the frame narrowed.
        rect.fun(0, 0, 480, 272, 0xFF000000u, 1);
        return Through(false, [&] { return sprite.fun(x0, y0, x1, y1, u0, v0, u1, v1, color, a10, a11, a12); });
    }
    // Full-width fills and fades keep covering the whole screen.
    return Through(x0 <= 0 && x1 >= 480, [&] { return sprite.fun(x0, y0, x1, y1, u0, v0, u1, v1, color, a10, a11, a12); });
}
uint32_t Glyphs(uintptr_t origin, uintptr_t positions, uintptr_t uvs, int count, unsigned color) {
    return Through(false, [&] { return glyphs.fun(origin, positions, uvs, count, color); });
}
uint32_t Rect(int x, int y, int w, int h, unsigned color, int raw) {
    return Through<10, 4, true>(x <= 0 && x + w >= 480, [&] { return rect.fun(x, y, w, h, color, raw); });
}

uint32_t FlashGlyph(uintptr_t font, int x, int y, int glyph, float scale) {
    return Through<16, 8>(false, [&] { return flashGlyph.fun(font, x, y, glyph, scale); });
}

// Flash menus, loading screens and overlays keep their proportions, centred.
uint32_t Movie(uintptr_t object, int flag) {
    if (screen.active) return movie.fun(object, flag);
    RefreshAspect();
    SetTransform(ScreenTransform::around(240.0f, 136.0f, 1.0f));
    const auto result = movie.fun(object, flag);
    SetTransform({});
    return result;
}

// Menu pages: the Flash frame and the native-font items drawn over it.
uint32_t Page(uintptr_t widget) {
    if (screen.active) return page.fun(widget);
    RefreshAspect();
    SetTransform(ScreenTransform::around(240.0f, 136.0f, 1.0f));
    const auto result = page.fun(widget);
    SetTransform({});
    return result;
}

// Movie frames are drawn by the MPEG display thread with through-mode
// sprites only, so no viewport needs re-emitting.
uint32_t Video(unsigned texture) {
    RefreshAspect();
    const auto t = ScreenTransform::around(240.0f, 136.0f, 1.0f);
    if (screen.active || !t.active || t.ax >= 1.0f) return video.fun(texture);
    screen = t; videoFrame = true;
    const auto result = video.fun(texture);
    videoFrame = false; screen = {};
    return result;
}

// "To skip/pause camera" prompt: right-aligned text above the bottom edge.
void Prompt(uintptr_t object) {
    if (screen.active) return prompt.fun(object);
    RefreshAspect();
    SetTransform(ScreenTransform::around(480.0f, 272.0f, hudScale));
    prompt.fun(object);
    SetTransform({});
}

// HUD elements are laid out by the engine's split-screen layout: relative
// position between two screen sides, with minimum/maximum side distances.
ScreenTransform ElementTransform(uintptr_t object) {
    float anchorX = 240.0f, anchorY = 136.0f;
    bool known = false;
    const auto layout = at<uintptr_t>(object + 24);
    const int index = at<int>(object + 28);
    if (Guest(layout, 0x400) && index >= 0 && index < 16) {
        const auto entry = layout + 156 + 24 * index;
        const float rx = at<float>(entry), ry = at<float>(entry + 12);
        const int mx = at<int>(entry + 8), my = at<int>(entry + 20);
        if (rx >= 0.0f && rx <= 1.0f && ry >= 0.0f && ry <= 1.0f) {
            known = true;
            // An unbounded side distance means the element spans that axis
            // (full-screen overlays, the centred mirror): keep it centred.
            if (mx < 1000) anchorX = rx < 0.4f ? 0.0f : rx > 0.6f ? 480.0f : 240.0f;
            if (my < 1000) anchorY = ry < 0.4f ? 0.0f : ry > 0.6f ? 272.0f : 136.0f;
        }
    }
    if (!known) {
        const int x = at<int>(object + 16), y = at<int>(object + 20);
        anchorX = x < 160 ? 0.0f : x > 320 ? 480.0f : 240.0f;
        anchorY = y < 90 ? 0.0f : y > 182 ? 272.0f : 136.0f;
    }
    return ScreenTransform::around(anchorX, anchorY, hudScale);
}

void DrawElement(uintptr_t object, uintptr_t function) {
    if (screen.active || !Guest(object, 0x40)) {
        reinterpret_cast<void (*)(uintptr_t)>(function)(object);
        return;
    }
    RefreshAspect();
    SetTransform(ElementTransform(object));
    reinterpret_cast<void (*)(uintptr_t)>(function)(object);
    SetTransform({});
}
}

void InstallHud() {
    if (!inireader.ReadInteger("MAIN", "FixHUD", 1)) return;
    const auto rectSite = pattern.get_first("FF 00 2A 31 ?? ?? 09 3C 1D 00 40 15 ?? ?? 29 25 40 52 04 00", 0);
    const auto viewportSite = pattern.get_first("F0 FF BD 27 30 01 85 8C 00 00 B0 AF 25 80 80 00 01 00 A4 30", 0);
    const auto selectSite = pattern.get_first("F0 FF BD 27 ?? ?? 85 8F 00 00 BF AF 05 00 85 10", 0);
    const auto spriteSite = pattern.get_first("0C 00 A2 8F 00 00 AC 8F FF 00 43 30", 0);
    const auto glyphSite = pattern.get_first("00 00 8F C4 00 49 07 00 80 50 07 00 04 00 8C C4", 0);
    const auto movieSite = pattern.get_first("D0 FF BD 27 0C 00 B1 AF FF 00 B1 30 04 00 85 8C", 0);
    const auto flashGlyphSite = pattern.get_first("C0 FF BD 27 1C 00 B0 AF 25 80 80 00 20 00 B1 AF 25 88 A0 00 0C 00 04 8E", 0);
    const auto pageSite = pattern.get_first("F0 FF BD 27 44 00 85 8C 00 00 BF AF 01 00 A6 30 0B 00 C0 10 00 02 A5 30", 0);
    const auto videoSite = pattern.get_first("E0 FF BD 27 ?? ?? 05 3C 10 00 B0 AF ?? ?? B0 24 08 00 05 8E 00 CC 06 3C 00 00 A6 AC", 0);
    const auto promptSite = pattern.get_first("90 FF BD 27 54 00 B0 AF 25 80 80 00 ?? ?? 84 8F 7F 43 07 3C 04 00 86 8C", 0);
    const auto manager = pattern.get_first("D0 FF BD 27 0C 00 B1 AF ?? ?? 11 3C ?? ?? 31 26 08 00 25 8E 00 C9 06 3C", 0);
    if (!rectSite || !viewportSite || !selectSite || !spriteSite || !glyphSite || !movieSite || !flashGlyphSite || !pageSite || !videoSite || !promptSite || !manager) {
#ifndef NDEBUG
        logger.WriteF("HUD sites missing: %X %X %X %X %X %X %X", rectSite, viewportSite, selectSite, spriteSite, glyphSite, movieSite, manager);
#endif
        return;
    }
    context = Absolute(rectSite, 4, 12);
    currentViewport = gameGP + int16_t(injector::ReadMemory<uint16_t>(selectSite + 4));

    viewportHook = safetymips::create_inline(viewportSite, ApplyViewport);
    applyViewport.fun = viewportHook.original<decltype(applyViewport.fun)>();
    spriteHook = safetymips::create_inline(spriteSite, Sprite);
    sprite.fun = spriteHook.original<decltype(sprite.fun)>();
    glyphHook = safetymips::create_inline(glyphSite, Glyphs);
    glyphs.fun = glyphHook.original<decltype(glyphs.fun)>();
    rectHook = safetymips::create_inline(rectSite, Rect);
    rect.fun = rectHook.original<decltype(rect.fun)>();
    movieHook = safetymips::create_inline(movieSite, Movie);
    movie.fun = movieHook.original<decltype(movie.fun)>();
    flashGlyphHook = safetymips::create_inline(flashGlyphSite, FlashGlyph);
    flashGlyph.fun = flashGlyphHook.original<decltype(flashGlyph.fun)>();
    pageHook = safetymips::create_inline(pageSite, Page);
    page.fun = pageHook.original<decltype(page.fun)>();
    videoHook = safetymips::create_inline(videoSite, Video);
    video.fun = videoHook.original<decltype(video.fun)>();
    promptHook = safetymips::create_inline(promptSite, Prompt);
    prompt.fun = promptHook.original<decltype(prompt.fun)>();

    // Each HUD element draw is a virtual call: lw a1,4(a1); jalr a1; addu a0,a2.
    // The wrapper performs the same call inside the element's transform.
    safetymips::Options replace;
    replace.execute_original = false;
    replace.instructions = 3;
    replace.preserve = 0; // a call site: caller-saved FPU/VFPU state is dead here
    unsigned count = 0;
    for (size_t i = 0; count < sizeof(elementHooks) / sizeof(elementHooks[0]); ++i) {
        const auto site = range_pattern.get(i, manager, 0x7C0, "04 00 A5 8C 09 F8 A0 00 21 20 86 00", 0);
        if (!site) break;
        elementHooks[count++] = safetymips::create_mid(site, [](SafetyMipsContext& regs) {
            DrawElement(regs.a0 + regs.a2, at<uintptr_t>(regs.a1 + 4));
        }, replace);
    }
#ifndef NDEBUG
    logger.WriteF("HUD: context %08X viewport %08X elements %u", unsigned(context), unsigned(currentViewport), count);
#endif
}
}
