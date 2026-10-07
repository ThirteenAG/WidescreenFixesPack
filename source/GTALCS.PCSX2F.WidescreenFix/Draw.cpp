#include "Game.hpp"
#include <limits>

namespace lcs {
namespace {
using RectVertices = uint64_t(const Rect*, const Color*, const Color*, const Color*, const Color*, bool);
using TexturedVertices = uint64_t(const Rect*, const Color*, const Color*, const Color*, const Color*,
                            float, float, float, float, float, float, float, float);
using BufferedVertices = uint64_t(Vertex*, const Rect*, const Color*, const Color*, const Color*, const Color*,
                            float, float, float, float, float, float, float, float);
using PolygonVertices = uint64_t(const Color*, const Color*, const Color*, const Color*,
                           float, float, float, float, float, float, float, float);
safetymips::GameInline<RectVertices> rectVertices;
safetymips::GameInline<TexturedVertices> texturedVertices;
safetymips::GameInline<BufferedVertices> bufferedVertices;
safetymips::GameInline<PolygonVertices> polygonVertices;
safetymips::GameInline<uint64_t(int, const float*, const float*, const Color*)> arrayVertices;
safetymips::GameInline<uint64_t(int, const float*)> maskVertices;
safetymips::GameInline<uint64_t(Sprite*, const Rect*, const Color*)> drawSprite;
safetymips::GameInline<uint64_t(Sprite*, const Rect*, const Color*, float, float, float, float,
                           float, float, float, float)> drawSpriteUV;
safetymips::GameInline<uint64_t(Sprite*, const Rect*, const Color*, const Color*, const Color*, const Color*)> drawSpriteColors;
safetymips::GameInline<uint64_t(const Rect*, const Color*, bool)> drawSolid;
safetymips::GameInline<uint64_t()> doFade;
SafetyMipsMid manualCrosshair;
bool artworkBackdrop = true;

void ManualCrosshair(SafetyMipsContext& regs) {
    // sub_2B6250 draws the free-aim reticle with RenderOneXLUSprite_Rotate_Aspect
    // (0x2E0DE0) directly into the 640x448 framebuffer: width f15 is 14 or 10.5
    // (widescreen preference), height f16 is 14. Derive the width from the real
    // display aspect so the reticle stays round, as VCS does at 0x319408.
    regs.f15 = regs.f16 * (640.0f / 448.0f) / settings.aspect;
}

bool FullWidth(const Rect& rect, bool physical) {
    return rect.left <= 0.0f && rect.right >= (physical ? 640.0f : 480.0f);
}
bool FullScreen(const Rect& rect, bool physical) {
    return FullWidth(rect, physical) && rect.top <= 0.0f && rect.bottom >= (physical ? 448.0f : 272.0f);
}
void ArtworkBackground(const Sprite* sprite, const Rect& rect) {
    if (!artworkBackdrop || drawMode == DrawMode::None || !sprite || !sprite->texture || !FullScreen(rect, false)) return;
    DrawScope scope(DrawMode::None);
    TextureScope solid(false);
    const Rect screen{0, 272, 480, 0};
    const Color black{0,0,0,255};
    drawSolid.call(&screen, &black, true);
}
uint64_t SetRectVertices(const Rect* rect, const Color* c0, const Color* c1, const Color* c2, const Color* c3, bool fraction) {
    auto corrected = CorrectRect(*rect, !fraction);
    return rectVertices.call(&corrected, c0, c1, c2, c3, fraction);
}
uint64_t SetTexturedVertices(const Rect* rect, const Color* c0, const Color* c1, const Color* c2, const Color* c3,
                        float u0, float v0, float u1, float v1, float u2, float v2, float u3, float v3) {
    TextureScope textured(true);
    auto corrected = CorrectRect(*rect, false);
    return texturedVertices.call(&corrected, c0, c1, c2, c3, u0, v0, u1, v1, u2, v2, u3, v3);
}
uint64_t SetBufferedVertices(Vertex* buffer, const Rect* rect, const Color* c0, const Color* c1, const Color* c2, const Color* c3,
                        float u0, float v0, float u1, float v1, float u2, float v2, float u3, float v3) {
    auto corrected = CorrectRect(*rect, false);
    return bufferedVertices.call(buffer, &corrected, c0, c1, c2, c3, u0, v0, u1, v1, u2, v2, u3, v3);
}
uint64_t SetPolygonVertices(const Color* c0, const Color* c1, const Color* c2, const Color* c3,
                        float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3) {
    auto result = polygonVertices.call(c0, c1, c2, c3, x0, y0, x1, y1, x2, y2, x3, y3);
    CorrectVertices(reinterpret_cast<Vertex*>(address::vertices), 4);
    return result;
}
uint64_t SetArrayVertices(int count, const float* positions, const float* uv, const Color* color) {
    auto result = arrayVertices.call(count, positions, uv, color);
    if (count > 0) CorrectVertices(reinterpret_cast<Vertex*>(address::vertices), static_cast<unsigned>(count));
    return result;
}
uint64_t SetMaskVertices(int count, const float* positions) {
    auto result = maskVertices.call(count, positions);
    if (count > 0) CorrectVertices(reinterpret_cast<Vertex*>(address::vertices), static_cast<unsigned>(count));
    return result;
}
uint64_t DrawSprite(Sprite* sprite, const Rect* rect, const Color* color) {
    ArtworkBackground(sprite, *rect);
    TextureScope textured(sprite && sprite->texture);
    return drawSprite.call(sprite, rect, color);
}
uint64_t DrawSpriteUV(Sprite* sprite, const Rect* rect, const Color* color,
                  float u0, float v0, float u1, float v1, float u2, float v2, float u3, float v3) {
    ArtworkBackground(sprite, *rect);
    TextureScope textured(sprite && sprite->texture);
    return drawSpriteUV.call(sprite, rect, color, u0, v0, u1, v1, u2, v2, u3, v3);
}
uint64_t DrawSpriteColors(Sprite* sprite, const Rect* rect, const Color* c0, const Color* c1, const Color* c2, const Color* c3) {
    ArtworkBackground(sprite, *rect);
    TextureScope textured(sprite && sprite->texture);
    return drawSpriteColors.call(sprite, rect, c0, c1, c2, c3);
}
uint64_t DrawSolid(const Rect* rect, const Color* color, bool fraction) {
    TextureScope textured(false);
    return drawSolid.call(rect, color, fraction);
}
uint64_t DoFade() {
    // DoFade draws the solid fade (full width, left untouched) and then fades the
    // 4:3 splash/loading artwork in over it. Contain the artwork, but without the
    // opaque backdrop: the splash is translucent while the fade progresses.
    DrawScope scope(DrawMode::Frontend);
    const bool saved = artworkBackdrop;
    artworkBackdrop = false;
    const auto result = doFade.call();
    artworkBackdrop = saved;
    return result;
}
int16_t Coordinate(float value) {
    // Finite inputs from the game become finite under the validated transform.
    // Saturate off-screen coordinates before narrowing to the packed vertex.
    return static_cast<int16_t>(value < -32768.0f ? -32768.0f : value > 32767.0f ? 32767.0f : value);
}
}

console::Transform DrawingTransform(bool physical, float centerX, float centerY) {
    const float width = physical ? 640.0f : 480.0f;
    const float height = physical ? 448.0f : 272.0f;
    float x = width * 0.5f, y = height * 0.5f, size = 1.0f;
    switch (drawMode) {
    case DrawMode::None: return {};
    case DrawMode::LeftBottom: x = 0.0f; y = height; size = settings.hudScale; break;
    case DrawMode::RightTop: x = width; y = 0.0f; size = settings.hudScale; break;
    case DrawMode::RightBottom: x = width; y = height; size = settings.hudScale; break;
    case DrawMode::World:
        x = centerX;
        y = centerY;
        break;
    case DrawMode::Center: case DrawMode::Frontend: break;
    }
    const float baseAspect = drawMode == DrawMode::Frontend ? 4.0f / 3.0f : settings.baseAspect;
    return console::Transform::anchored(baseAspect, settings.aspect, size, x, y);
}

Rect CorrectRect(Rect value, bool physical) {
    if (drawMode == DrawMode::None) return value;
    // Solid fades, cutscene borders, and scoped-weapon masks must continue to
    // cover the viewport. Textured full-screen artwork fits without stretching.
    if (!textureDraw && FullWidth(value, physical)) return value;
    if (textureDraw && FullScreen(value, physical)) {
        float width = physical ? 640.0f : 480.0f, height = physical ? 448.0f : 272.0f;
        const float baseAspect = drawMode == DrawMode::Frontend ? 4.0f / 3.0f : settings.baseAspect;
        float scaleX = baseAspect / settings.aspect, scaleY = 1.0f;
        if (scaleX > 1.0f) { scaleY = 1.0f / scaleX; scaleX = 1.0f; }
        return console::Transform{scaleX, scaleY, width * 0.5f * (1.0f - scaleX),
                                   height * 0.5f * (1.0f - scaleY)}.rect(value);
    }
    return DrawingTransform(physical, (value.left + value.right) * 0.5f,
                            (value.top + value.bottom) * 0.5f).rect(value);
}

void CorrectVertices(Vertex* vertices, unsigned count) {
    if (drawMode == DrawMode::None || !count) return;
    float x = 0.0f, y = 0.0f;
    if (drawMode == DrawMode::World) {
        for (unsigned i = 0; i < count; ++i) { x += vertices[i].x; y += vertices[i].y; }
        x /= count; y /= count;
    }
    auto transform = DrawingTransform(true, x, y);
    for (unsigned i = 0; i < count; ++i) {
        vertices[i].x = Coordinate(transform.x(vertices[i].x));
        vertices[i].y = Coordinate(transform.y(vertices[i].y));
    }
}

void InstallDrawing() {
    rectVertices = safetymips::create_inline_game(0x321068, SetRectVertices);
    texturedVertices = safetymips::create_inline_game(0x321390, SetTexturedVertices);
    bufferedVertices = safetymips::create_inline_game(0x321740, SetBufferedVertices);
    polygonVertices = safetymips::create_inline_game(0x321228, SetPolygonVertices);
    arrayVertices = safetymips::create_inline_game(0x3215D8, SetArrayVertices);
    maskVertices = safetymips::create_inline_game(0x3216D8, SetMaskVertices);
    drawSprite = safetymips::create_inline_game(0x320E00, DrawSprite);
    drawSpriteUV = safetymips::create_inline_game(0x320E60, DrawSpriteUV);
    drawSpriteColors = safetymips::create_inline_game(0x320EB0, DrawSpriteColors);
    drawSolid = safetymips::create_inline_game(0x3219C0, DrawSolid);
    doFade = safetymips::create_inline_game(0x1F4280, DoFade);
    manualCrosshair = safetymips::create_mid<&ManualCrosshair>(0x2B660C);
}
}
