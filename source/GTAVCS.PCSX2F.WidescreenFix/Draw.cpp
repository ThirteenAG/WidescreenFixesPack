#include "Game.hpp"
#include <limits>

namespace vcs {
namespace {
using RectVertices = int(const Rect*, const Color*, const Color*, const Color*, const Color*, bool);
using TexturedVertices = int(const Rect*, const Color*, const Color*, const Color*, const Color*,
                            float, float, float, float, float, float, float, float);
using BufferedVertices = int(Vertex*, const Rect*, const Color*, const Color*, const Color*, const Color*,
                            float, float, float, float, float, float, float, float);
using PolygonVertices = int(const Color*, const Color*, const Color*, const Color*,
                           float, float, float, float, float, float, float, float);
safetymips::GameInline<RectVertices> rectVertices;
safetymips::GameInline<TexturedVertices> texturedVertices;
safetymips::GameInline<BufferedVertices> bufferedVertices;
safetymips::GameInline<PolygonVertices> polygonVertices;
safetymips::GameInline<uint64_t(int, const float*, const float*, const Color*)> arrayVertices;
safetymips::GameInline<uint64_t(int, const float*)> maskVertices;
safetymips::GameInline<void(int, int, int, int)> drawWindow;
safetymips::GameInline<uint64_t(void*)> radarMap;
bool radarClip = false;
safetymips::GameInline<void(Sprite*, const Rect*, const Color*)> drawSprite;
safetymips::GameInline<void(Sprite*, const Rect*, const Color*, float, float, float, float,
                           float, float, float, float)> drawSpriteUV;
safetymips::GameInline<uint64_t(Sprite*, const Rect*, const Color*, const Color*, const Color*, const Color*)> drawSpriteColors;
safetymips::GameInline<void(const Rect*, const Color*, bool)> drawSolid;
safetymips::GameInline<void(const uint16_t*, int, int, const Color*, const Rect*, int)> printText;
safetymips::GameInline<uint64_t(uint8_t, uint8_t, uint8_t, int16_t, uint8_t,
                               float, float, float, float, float)> translucentSprite;
SafetyMipsMid manualCrosshair;

void ManualCrosshair(SafetyMipsContext& regs) {
    // CWeaponEffects draws the manual-aim reticle outside CHud, directly into
    // the 640x448 framebuffer. Account for its displayed pixel aspect without
    // moving the aim point or applying the HUD layout transform a second time.
    regs.f15 = regs.f16 * (640.0f / 448.0f) / settings.aspect;
}

bool FullWidth(const Rect& rect, bool physical) {
    return rect.left <= 0.0f && rect.right >= (physical ? 640.0f : 480.0f);
}
bool FullScreen(const Rect& rect, bool physical) {
    return FullWidth(rect, physical) && rect.top <= 0.0f && rect.bottom >= (physical ? 448.0f : 272.0f);
}
void ArtworkBackground(const Sprite* sprite, const Rect& rect) {
    if (drawMode == DrawMode::None || !sprite || !sprite->texture || !FullScreen(rect, false)) return;
    DrawScope scope(DrawMode::None);
    TextureScope solid(false);
    const Rect screen{0, 272, 480, 0};
    const Color black{0,0,0,255};
    drawSolid.call(&screen, &black, true);
}
void PrintText(const uint16_t* string, int x, int y, const Color* color, const Rect* rect, int shadow) {
    const bool savedWorldText = worldText;
    const auto savedAnchor = worldTextAnchor;
    if (drawMode == DrawMode::World) {
        worldText = true;
        worldTextAnchor = {float(x), float(y)};
    }
    printText.call(string, x, y, color, rect, shadow);
    worldText = savedWorldText;
    worldTextAnchor = savedAnchor;
}

int SetRectVertices(const Rect* rect, const Color* c0, const Color* c1, const Color* c2, const Color* c3, bool fraction) {
    auto corrected = CorrectRect(*rect, !fraction);
    return rectVertices.call(&corrected, c0, c1, c2, c3, fraction);
}
int SetTexturedVertices(const Rect* rect, const Color* c0, const Color* c1, const Color* c2, const Color* c3,
                        float u0, float v0, float u1, float v1, float u2, float v2, float u3, float v3) {
    TextureScope textured(true);
    auto corrected = CorrectRect(*rect, false);
    return texturedVertices.call(&corrected, c0, c1, c2, c3, u0, v0, u1, v1, u2, v2, u3, v3);
}
int SetBufferedVertices(Vertex* buffer, const Rect* rect, const Color* c0, const Color* c1, const Color* c2, const Color* c3,
                        float u0, float v0, float u1, float v1, float u2, float v2, float u3, float v3) {
    auto corrected = CorrectRect(*rect, false);
    return bufferedVertices.call(buffer, &corrected, c0, c1, c2, c3, u0, v0, u1, v1, u2, v2, u3, v3);
}
int SetPolygonVertices(const Color* c0, const Color* c1, const Color* c2, const Color* c3,
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
void SetDrawWindow(int left, int top, int width, int height) {
    // Fonts and radar sections share this logical 480x272 scissor API. Correct
    // their clips together with their vertices. A full-window call is a reset
    // and must leave the whole framebuffer available to subsequent phases.
    if (drawMode == DrawMode::None || (left == 0 && top == 0 && width >= 480 && height >= 272)) {
        drawWindow.call(left, top, width, height);
        return;
    }
    const auto transform = DrawingTransform(false, left + width * 0.5f, top + height * 0.5f);
    // The radar's depth mask ends at truncated integer vertices. Expanding its
    // scissor exposes map pixels beyond the mask, producing a vertical seam.
    // Keep that clip inside its native bounds; text clips still round outward.
    const int x = int(radarClip ? std::ceil(transform.x(float(left))) : std::floor(transform.x(float(left))));
    const int y = int(radarClip ? std::ceil(transform.y(float(top))) : std::floor(transform.y(float(top))));
    const int right = int(radarClip ? std::floor(transform.x(float(left + width))) : std::ceil(transform.x(float(left + width))));
    const int bottom = int(radarClip ? std::floor(transform.y(float(top + height))) : std::ceil(transform.y(float(top + height))));
    drawWindow.call(x, y, right - x, bottom - y);
}
uint64_t DrawRadarMap(void* radar) {
    const bool saved = radarClip;
    radarClip = true;
    const auto result = radarMap.call(radar);
    radarClip = saved;
    return result;
}
uint64_t RenderTranslucentSprite(uint8_t red, uint8_t green, uint8_t blue, int16_t intensity, uint8_t alpha,
                               float x, float y, float z, float halfWidth, float halfHeight) {
    // This renderer owns a separate vertex buffer; it never calls SetVertices.
    // Its float registers are x, y, z, halfWidth, halfHeight (f12 through f16).
    const auto transform = DrawingTransform(true, x, y);
    return translucentSprite.call(red, green, blue, intensity, alpha,
        transform.x(x), transform.y(y), z,
        halfWidth * transform.scaleX, halfHeight * transform.scaleY);
}
void DrawSprite(Sprite* sprite, const Rect* rect, const Color* color) {
    ArtworkBackground(sprite, *rect);
    TextureScope textured(sprite && sprite->texture);
    drawSprite.call(sprite, rect, color);
}
void DrawSpriteUV(Sprite* sprite, const Rect* rect, const Color* color,
                  float u0, float v0, float u1, float v1, float u2, float v2, float u3, float v3) {
    ArtworkBackground(sprite, *rect);
    TextureScope textured(sprite && sprite->texture);
    drawSpriteUV.call(sprite, rect, color, u0, v0, u1, v1, u2, v2, u3, v3);
}
uint64_t DrawSpriteColors(Sprite* sprite, const Rect* rect, const Color* c0, const Color* c1, const Color* c2, const Color* c3) {
    ArtworkBackground(sprite, *rect);
    TextureScope textured(sprite && sprite->texture);
    return drawSpriteColors.call(sprite, rect, c0, c1, c2, c3);
}
void DrawSolid(const Rect* rect, const Color* color, bool fraction) {
    TextureScope textured(false);
    drawSolid.call(rect, color, fraction);
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
        x = worldText ? worldTextAnchor.x * (physical ? 640.0f / 480.0f : 1.0f) : centerX;
        y = worldText ? worldTextAnchor.y * (physical ? 448.0f / 272.0f : 1.0f) : centerY;
        break;
    case DrawMode::Center: break;
    }
    return console::Transform::anchored(settings.baseAspect, settings.aspect, size, x, y);
}

Rect CorrectRect(Rect value, bool physical) {
    if (drawMode == DrawMode::None) return value;
    // Solid fades, cutscene borders, and scoped-weapon masks must continue to
    // cover the viewport. Textured full-screen artwork fits without stretching.
    if (!textureDraw && FullWidth(value, physical)) return value;
    if (textureDraw && FullScreen(value, physical)) {
        float width = physical ? 640.0f : 480.0f, height = physical ? 448.0f : 272.0f;
        float scaleX = settings.baseAspect / settings.aspect, scaleY = 1.0f;
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
    rectVertices = safetymips::create_inline_game(0x3E0198, SetRectVertices);
    texturedVertices = safetymips::create_inline_game(0x3E04D0, SetTexturedVertices);
    bufferedVertices = safetymips::create_inline_game(0x3E0888, SetBufferedVertices);
    polygonVertices = safetymips::create_inline_game(0x3E0368, SetPolygonVertices);
    arrayVertices = safetymips::create_inline_game(0x3E0720, SetArrayVertices);
    maskVertices = safetymips::create_inline_game(0x3E0820, SetMaskVertices);
    drawWindow = safetymips::create_inline_game(0x175408, SetDrawWindow);
    radarMap = safetymips::create_inline_game(0x1178F8, DrawRadarMap);
    drawSprite = safetymips::create_inline_game(0x3DFF20, DrawSprite);
    drawSpriteUV = safetymips::create_inline_game(0x3DFF80, DrawSpriteUV);
    drawSpriteColors = safetymips::create_inline_game(0x3DFFD0, DrawSpriteColors);
    drawSolid = safetymips::create_inline_game(0x3E0B10, DrawSolid);
    printText = safetymips::create_inline_game(0x3F3D88, PrintText);
    translucentSprite = safetymips::create_inline_game(0x3A03A8, RenderTranslucentSprite);
    manualCrosshair = safetymips::create_mid<&ManualCrosshair>(0x319408);
}
}
