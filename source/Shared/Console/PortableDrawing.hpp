#pragma once
#include "PSP.hpp"
#include <cstdint>

namespace console::portable {
enum class DrawAnchor { None, Center, TopRight, BottomRight, Radar, World };
struct StoryDrawingSettings {
    float aspect = 16.0f / 9.0f, hud = 1.0f, radar = 1.0f, radarX = 12.0f, radarY = 196.0f;
    float multiplayerRadar = 1.0f, multiplayerRadarY = 196.0f;
};
struct StoryDrawingProfile {
    uintptr_t rect, textured, buffered, polygon, array, mask;
    uintptr_t sprite, spriteUV, spriteColors, solid, vertices, width, height, multiplayer;
    uintptr_t printText = 0;
    uintptr_t drawWindow = 0, translucentSprite = 0;
    // CSprite2d::DrawTxRect: frontend images with an already bound raster.
    uintptr_t texturedRect = 0;
};
struct StoryVertex {
    int16_t u, v;
    uint8_t color[4];
    int16_t x, y, z, padding;
};
static_assert(sizeof(StoryVertex) == 16 && offsetof(StoryVertex, x) == 8);
struct StoryColor { uint8_t r, g, b, a; };
struct StorySprite { void* texture; };

// One instance per module. Hooks operate on the game's final sprite vertices;
// font metrics and layout remain native, so kerning, wrapping and shadows agree.
template<class Game> struct StoryDrawing {
    inline static StoryDrawingSettings settings;
    inline static StoryDrawingProfile profile;
    inline static DrawAnchor anchor = DrawAnchor::None;
    inline static bool textured, worldText;
    inline static Point textAnchor;
    inline static SafetyMipsInline rectHook, texturedHook, bufferedHook, polygonHook,
        arrayHook, maskHook, spriteHook, spriteUVHook, spriteColorsHook, solidHook, printHook,
        windowHook, translucentHook, texturedRectHook;
    struct Scope {
        DrawAnchor previous;
        explicit Scope(DrawAnchor value) : previous(anchor) { anchor = value; }
        ~Scope() { anchor = previous; }
    };
    struct TextureScope {
        bool previous;
        explicit TextureScope(bool value) : previous(textured) { textured = value; }
        ~TextureScope() { textured = previous; }
    };
    static void PrintText(void* font, const uint16_t* text, int x, int y,
                          const StoryColor* color, const Rect* rect, uint8_t shadow, uint8_t flags) {
        const bool previous = worldText;
        const auto previousAnchor = textAnchor;
        if (anchor == DrawAnchor::World) { worldText = true; textAnchor = {float(x),float(y)}; }
        printHook.call<void>(font,text,x,y,color,rect,shadow,flags);
        worldText = previous;
        textAnchor = previousAnchor;
    }
    static float Width(bool physical) { return physical ? float(*reinterpret_cast<int*>(profile.width)) : 480.0f; }
    static float Height(bool physical) { return physical ? float(*reinterpret_cast<int*>(profile.height)) : 272.0f; }
    static Transform TransformFor(bool physical, float centerX = 240.0f, float centerY = 136.0f) {
        if (anchor == DrawAnchor::None) return {};
        const float w = Width(physical), h = Height(physical);
        float x = w * 0.5f, y = h * 0.5f, size = 1.0f;
        switch (anchor) {
        case DrawAnchor::TopRight: x = w; y = 0; size = settings.hud; break;
        case DrawAnchor::BottomRight: x = w; y = h; size = settings.hud; break;
        case DrawAnchor::Radar: x = 0; y = h; size = settings.radar; break;
        case DrawAnchor::World:
            x = worldText ? textAnchor.x * w / 480.0f : centerX;
            y = worldText ? textAnchor.y * h / 272.0f : centerY;
            break;
        default: break;
        }
        if (anchor == DrawAnchor::Radar && profile.multiplayer && *reinterpret_cast<uint8_t*>(profile.multiplayer))
            size = settings.multiplayerRadar;
        return Transform::anchored(480.0f / 272.0f, settings.aspect, size, x, y);
    }
    static Rect Correct(Rect r, bool physical) {
        if (anchor == DrawAnchor::None) return r;
        const float w = Width(physical), h = Height(physical);
        if (anchor == DrawAnchor::World && r.left <= 0 && r.right >= w) return r;
        if (r.left <= 0 && r.right >= w) {
            if (!textured) return r; // Native fades and cutscene/weapon masks cover the viewport.
            if (r.top <= 0 && r.bottom >= h) {
                float sx = (480.0f / 272.0f) / settings.aspect, sy = 1.0f;
                if (sx > 1) { sy = 1 / sx; sx = 1; }
                return Transform{sx, sy, w * 0.5f * (1 - sx), h * 0.5f * (1 - sy)}.rect(r);
            }
        }
        return TransformFor(physical, (r.left+r.right)*0.5f, (r.top+r.bottom)*0.5f).rect(r);
    }
    static int16_t Coordinate(float value) {
        return int16_t(value < -32768.0f ? -32768.0f : value > 32767.0f ? 32767.0f : value);
    }
    static void CorrectVertices(unsigned count) {
        if (anchor == DrawAnchor::None || !count) return;
        auto* vertices = reinterpret_cast<StoryVertex*>(profile.vertices);
        float x = 0, y = 0;
        if (anchor == DrawAnchor::World) {
            for (unsigned i = 0; i < count; ++i) { x += vertices[i].x; y += vertices[i].y; }
            x /= count; y /= count;
        }
        auto transform = TransformFor(true, x, y);
        for (unsigned i = 0; i < count; ++i) {
            vertices[i].x = Coordinate(transform.x(vertices[i].x));
            vertices[i].y = Coordinate(transform.y(vertices[i].y));
        }
    }
    static void SetRect(const Rect* r, const StoryColor* a, const StoryColor* b,
                        const StoryColor* c, const StoryColor* d, bool fraction) {
        auto corrected = Correct(*r, !fraction);
        rectHook.call<void>(&corrected, a, b, c, d, fraction);
    }
    static void SetTextured(const Rect* r, const StoryColor* a, const StoryColor* b,
                            const StoryColor* c, const StoryColor* d,
                            float u0, float v0, float u1, float v1, float u2, float v2, float u3, float v3) {
        TextureScope scope(true);
        auto corrected = Correct(*r, false);
        texturedHook.call<void>(&corrected, a, b, c, d, u0,v0,u1,v1,u2,v2,u3,v3);
    }
    static void SetBuffered(StoryVertex* buffer, const Rect* r, const StoryColor* a, const StoryColor* b,
                            const StoryColor* c, const StoryColor* d,
                            float u0, float v0, float u1, float v1, float u2, float v2, float u3, float v3) {
        auto corrected = Correct(*r, false);
        bufferedHook.call<void>(buffer, &corrected, a,b,c,d,u0,v0,u1,v1,u2,v2,u3,v3);
    }
    static void SetPolygon(const StoryColor* a, const StoryColor* b, const StoryColor* c, const StoryColor* d,
                            float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3) {
        polygonHook.call<void>(a,b,c,d,x0,y0,x1,y1,x2,y2,x3,y3);
        CorrectVertices(4);
    }
    static void SetArray(int count, const float* p, const float* uv, const StoryColor* color) {
        arrayHook.call<void>(count,p,uv,color);
        if (count > 0) CorrectVertices(unsigned(count));
    }
    static void SetMask(int count, const float* p) {
        maskHook.call<void>(count,p);
        if (count > 0) CorrectVertices(unsigned(count));
    }
    static void SetDrawWindow(int16_t x, int16_t y, int16_t width, int16_t height) {
        const float w = Width(true), h = Height(true);
        if (anchor == DrawAnchor::None ||
            (x <= 0 && y <= 0 && x + width >= w - 1 && y + height >= h - 1)) {
            windowHook.call<void>(x,y,width,height);
            return;
        }
        const auto transform = TransformFor(true);
        // PSP scissor endpoints are inclusive. Round radar clips inward so
        // tiles cannot escape the corner mask; round font clips outward.
        const bool radar = anchor == DrawAnchor::Radar;
        const float left = transform.x(x), top = transform.y(y);
        const float right = transform.x(float(x) + width + 1), bottom = transform.y(float(y) + height + 1);
        const float minX = radar ? std::ceil(left) : std::floor(left);
        const float minY = radar ? std::ceil(top) : std::floor(top);
        const float maxX = radar ? std::floor(right) - 1 : std::ceil(right) - 1;
        const float maxY = radar ? std::floor(bottom) - 1 : std::ceil(bottom) - 1;
        if (minX > maxX || minY > maxY || minX >= w || minY >= h || maxX < 0 || maxY < 0) {
            windowHook.call<void>(int16_t(1),int16_t(1),int16_t(-1),int16_t(-1));
            return;
        }
        const auto lowX = Coordinate(minX < 0 ? 0 : minX), lowY = Coordinate(minY < 0 ? 0 : minY);
        const auto highX = Coordinate(maxX >= w ? w - 1 : maxX), highY = Coordinate(maxY >= h ? h - 1 : maxY);
        windowHook.call<void>(lowX,lowY,int16_t(highX-lowX),int16_t(highY-lowY));
    }
    static uint64_t RenderTranslucentSprite(uint8_t red, uint8_t green, uint8_t blue,
        int16_t intensity, uint8_t alpha, float x, float y, float z, float halfWidth, float halfHeight, float recipZ) {
        const auto transform = TransformFor(true,x,y);
        return translucentHook.call<uint64_t>(red,green,blue,intensity,alpha,
            transform.x(x),transform.y(y),z,halfWidth*transform.scaleX,halfHeight*transform.scaleY,recipZ);
    }
    static void Background(const StorySprite* sprite, const Rect& r) {
        if (sprite && sprite->texture) Background(r);
    }
    static void Background(const Rect& r) {
        if (anchor == DrawAnchor::None || anchor == DrawAnchor::World ||
            r.left > 0 || r.right < 480 || r.top > 0 || r.bottom < 272) return;
        Scope scope(DrawAnchor::None);
        const Rect screen{0,272,480,0}; const StoryColor black{0,0,0,255};
        solidHook.call<void>(&screen, &black, true);
    }
    static void DrawSprite(StorySprite* sprite, const Rect* r, const StoryColor* color) {
        Background(sprite,*r);
        TextureScope scope(sprite && sprite->texture);
        spriteHook.call<void>(sprite,r,color);
    }
    static void DrawUV(StorySprite* sprite, const Rect* r, const StoryColor* color,
                        float u0, float v0, float u1, float v1, float u2, float v2, float u3, float v3) {
        Background(sprite,*r);
        TextureScope scope(sprite && sprite->texture);
        spriteUVHook.call<void>(sprite,r,color,u0,v0,u1,v1,u2,v2,u3,v3);
    }
    static void DrawColors(StorySprite* sprite, const Rect* r, const StoryColor* a, const StoryColor* b,
                            const StoryColor* c, const StoryColor* d) {
        Background(sprite,*r);
        TextureScope scope(sprite && sprite->texture);
        spriteColorsHook.call<void>(sprite,r,a,b,c,d);
    }
    static void DrawTexturedRect(const Rect* r, const StoryColor* color) {
        // Menu images bind their raster first; the shared rectangle path would
        // otherwise treat full-screen artwork as an untextured fill. Callers
        // fill the uncovered sides before binding (a fill unbinds the raster).
        TextureScope scope(true);
        texturedRectHook.call<void>(r,color);
    }
    static void DrawSolid(const Rect* r, const StoryColor* color, bool fraction) {
        TextureScope scope(false);
        solidHook.call<void>(r,color,fraction);
    }
    static void Install(StoryDrawingProfile value) {
        profile = value;
        rectHook = safetymips::create_inline(value.rect, SetRect);
        texturedHook = safetymips::create_inline(value.textured, SetTextured);
        bufferedHook = safetymips::create_inline(value.buffered, SetBuffered);
        polygonHook = safetymips::create_inline(value.polygon, SetPolygon);
        arrayHook = safetymips::create_inline(value.array, SetArray);
        maskHook = safetymips::create_inline(value.mask, SetMask);
        spriteHook = safetymips::create_inline(value.sprite, DrawSprite);
        spriteUVHook = safetymips::create_inline(value.spriteUV, DrawUV);
        spriteColorsHook = safetymips::create_inline(value.spriteColors, DrawColors);
        solidHook = safetymips::create_inline(value.solid, DrawSolid);
        if (value.printText) printHook = safetymips::create_inline(value.printText, PrintText);
        if (value.drawWindow) windowHook = safetymips::create_inline(value.drawWindow, SetDrawWindow);
        if (value.texturedRect) texturedRectHook = safetymips::create_inline(value.texturedRect, DrawTexturedRect);
        if (value.translucentSprite) translucentHook = safetymips::create_inline(value.translucentSprite, RenderTranslucentSprite);
    }
};
template<class Drawing, uintptr_t Address, DrawAnchor Anchor, class Signature> struct StoryDrawPhase;
template<class Drawing, uintptr_t Address, DrawAnchor Anchor, class Return, class... Args>
struct StoryDrawPhase<Drawing, Address, Anchor, Return(Args...)> {
    inline static SafetyMipsInline hook;
    static Return Draw(Args... args) {
        typename Drawing::Scope scope(Anchor);
        return hook.template call<Return>(args...);
    }
    static void Install(uintptr_t address = Address) { hook = safetymips::create_inline(address, Draw); }
};
}
