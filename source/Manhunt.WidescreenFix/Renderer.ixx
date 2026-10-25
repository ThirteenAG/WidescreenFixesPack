module;

#include <stdafx.h>
#include "../Shared/CheckedPattern.hpp"
#include "common.h"
#include "FixSettings.hpp"

export module Renderer;

import Skeleton;
import Draw;

// The game draws all of its 2D (HUD, menus, text, videos, screen effects) through the engine's immediate mode in
// coordinates made for a 640x480 screen and stretched to any resolution. The fix adjusts those vertices right before
// they are drawn. What is being drawn decides how: the layer is set by the code that draws it.
export enum class Layer2D
{
    Box,        // HUD, menus, text and loading screens: their 4:3 design in a centered box
    Stretch,    // full-screen effects that are meant to cover the screen (noise, scanlines, tints)
    Projected,  // sprites placed from the 3D view (lock-on, halos, lights, blood): already at the right place
    Movie,      // videos: the frame keeps its aspect ratio (MAIN.FMVWidescreenMode), the rest like Box
};

// Where an element of the Box layer sits when the HUD box is wider than 4:3
export enum class Anchor
{
    Center,
    Left,
    Right,
    Auto,       // by its own position: the left, middle or right third of the screen
};

struct RwIm2DVertex
{
    float x;
    float y;
    float z;
    float rhw;
    uint32_t emissiveColor;
    float u;
    float v;
};
static_assert(sizeof(RwIm2DVertex) == 28);

namespace
{
    Layer2D CurrentLayer = Layer2D::Box;
    Anchor CurrentAnchor = Anchor::Center;

    using RenderStateFn = int32_t(__cdecl*)(int32_t, void*);
    using RenderLineFn = int32_t(__cdecl*)(RwIm2DVertex*, int32_t, int32_t, int32_t);
    using RenderTriangleFn = int32_t(__cdecl*)(RwIm2DVertex*, int32_t, int32_t, int32_t, int32_t);
    using RenderPrimitiveFn = int32_t(__cdecl*)(int32_t, RwIm2DVertex*, int32_t);
    using RenderIndexedPrimitiveFn = int32_t(__cdecl*)(int32_t, RwIm2DVertex*, int32_t, uint16_t*, int32_t);

    RenderLineFn RenderLine = nullptr;
    RenderTriangleFn RenderTriangle = nullptr;
    RenderPrimitiveFn RenderPrimitive = nullptr;
    RenderIndexedPrimitiveFn RenderIndexedPrimitive = nullptr;

    constexpr int32_t rwPRIMTYPETRILIST = 3;

    // Shared by the HUD and the frontend to keep icons, the mouse cursor and the scope round
    GameRef<float> HudStretch([]() -> float*
    {
        auto pattern = hook::pattern("8B 1D ? ? ? ? 89 9C 24 ? ? ? ? D8 A4 24");
        if (!pattern.empty())
            return *pattern.get_first<float*>(2);
        return nullptr;
    });

    struct Frame
    {
        float width;
        float height;
        float aspect;
        float scale;     // the width of the 4:3 design on this screen, 1 = the whole width
        float hudWidth;  // the width of the HUD box on this screen
    };

    bool GetFrame(Frame& frame)
    {
        auto raster = GetCurrentRaster();
        if (!raster)
            return false;
        frame.width = float(RwRasterGetWidth(raster));
        frame.height = float(RwRasterGetHeight(raster));
        if (frame.width <= 0.0f || frame.height <= 0.0f)
            return false;
        frame.aspect = frame.width / frame.height;
        frame.scale = CDraw::GetDesignScale(frame.aspect);
        frame.hudWidth = frame.aspect > DEFAULT_ASPECT_RATIO ? CDraw::GetHudAspectRatio(frame.aspect) / frame.aspect : 1.0f;
        return true;
    }

    void* GetTextureRaster()
    {
        void* raster = nullptr;
        RwEngine::Field<RenderStateFn>(RwEngine::RenderStateGet)(RwEngine::rwRENDERSTATETEXTURERASTER, &raster);
        return raster;
    }

    Anchor Resolve(Anchor anchor, float minX, float maxX, const Frame& frame)
    {
        if (anchor != Anchor::Auto)
            return anchor;
        const float center = (minX + maxX) * 0.5f / frame.width;
        if (center < 1.0f / 3.0f)
            return Anchor::Left;
        if (center > 2.0f / 3.0f)
            return Anchor::Right;
        return Anchor::Center;
    }

    float MapX(float x, Anchor anchor, const Frame& frame)
    {
        const float u = x / frame.width;
        switch (anchor)
        {
            case Anchor::Left:
                return frame.width * ((1.0f - frame.hudWidth) * 0.5f + u * frame.scale);
            case Anchor::Right:
                return frame.width * ((1.0f + frame.hudWidth) * 0.5f - (1.0f - u) * frame.scale);
            default:
                return frame.width * (0.5f + (u - 0.5f) * frame.scale);
        }
    }

    void ScaleX(RwIm2DVertex* v, int32_t count, float factor, const Frame& frame)
    {
        for (int32_t i = 0; i < count; i++)
            v[i].x = frame.width * 0.5f + (v[i].x - frame.width * 0.5f) * factor;
    }

    void ScaleY(RwIm2DVertex* v, int32_t count, float factor, const Frame& frame)
    {
        for (int32_t i = 0; i < count; i++)
            v[i].y = frame.height * 0.5f + (v[i].y - frame.height * 0.5f) * factor;
    }

    void GetExtent(const RwIm2DVertex* v, int32_t count, float& minX, float& maxX)
    {
        minX = FLT_MAX;
        maxX = -FLT_MAX;
        for (int32_t i = 0; i < count; i++)
        {
            minX = std::min(minX, v[i].x);
            maxX = std::max(maxX, v[i].x);
        }
    }

    bool IsPowerOfTwo(int32_t value)
    {
        return value > 0 && (value & (value - 1)) == 0;
    }

    bool IsOpaque(const RwIm2DVertex* v, int32_t count)
    {
        for (int32_t i = 0; i < count; i++)
            if ((v[i].emissiveColor >> 24) != 0xFF)
                return false;
        return true;
    }

    // The video frame at its own aspect ratio. Returns whether black has to be drawn around it.
    bool FitMovie(RwIm2DVertex* v, void* raster, const Frame& frame)
    {
        static auto& mode = FixSettings().Int("MAIN.FMVWidescreenMode");
        if (mode == 2)
            return false;

        // The game's videos are 512x512 frames made for a 4:3 screen; other sizes are shown at their own shape
        const int32_t w = RwRasterGetWidth(raster);
        const int32_t h = RwRasterGetHeight(raster);
        const float movieAspect = (w == h || w <= 0 || h <= 0) ? DEFAULT_ASPECT_RATIO : float(w) / float(h);

        if (movieAspect < frame.aspect)
        {
            if (mode == 0)
            {
                ScaleX(v, 4, movieAspect / frame.aspect, frame);
                return true;
            }
            ScaleY(v, 4, frame.aspect / movieAspect, frame);
        }
        else
        {
            if (mode == 0)
            {
                ScaleY(v, 4, frame.aspect / movieAspect, frame);
                return true;
            }
            ScaleX(v, 4, movieAspect / frame.aspect, frame);
        }
        return false;
    }

    // One quad of the Box or Movie layer. Returns whether black has to be drawn behind the batch.
    bool TransformQuad(RwIm2DVertex* v, void* raster, const Frame& frame)
    {
        float minX, maxX;
        GetExtent(v, 4, minX, maxX);

        if (minX <= 0.5f && maxX >= frame.width - 0.5f)
        {
            // Untextured full-width quads are fades, tints and backdrops: they keep covering the screen
            if (!raster)
                return false;

            if (CurrentLayer == Layer2D::Movie)
                return FitMovie(v, raster, frame) && IsOpaque(v, 4);

            // Full-screen art: wider pictures (the widescreen frontend package) at their own shape, the original
            // 4:3 pictures in the box with black beside them
            const int32_t w = RwRasterGetWidth(raster);
            const int32_t h = RwRasterGetHeight(raster);
            const float artAspect = (w > 0 && h > 0) ? float(w) / float(h) : DEFAULT_ASPECT_RATIO;
            const float factor = (!IsPowerOfTwo(w) || !IsPowerOfTwo(h)) && artAspect >= 1.4f ? artAspect / frame.aspect : frame.scale;
            ScaleX(v, 4, factor, frame);
            return factor < 1.0f && IsOpaque(v, 4);
        }

        const Anchor anchor = Resolve(CurrentAnchor, minX, maxX, frame);
        for (int32_t i = 0; i < 4; i++)
        {
            const float x = v[i].x;
            v[i].x = MapX(x, anchor, frame);

            // Untextured quads at a screen edge (the scope's sides, bars) keep reaching that edge
            if (!raster)
            {
                if (x <= 0.5f)
                    v[i].x = 0.0f;
                else if (x >= frame.width - 0.5f)
                    v[i].x = frame.width;
            }
        }
        return false;
    }

    void TransformVertices(RwIm2DVertex* v, int32_t count, const Frame& frame)
    {
        float minX, maxX;
        GetExtent(v, count, minX, maxX);
        const Anchor anchor = Resolve(CurrentAnchor, minX, maxX, frame);
        for (int32_t i = 0; i < count; i++)
            v[i].x = MapX(v[i].x, anchor, frame);
    }

    bool IsAdjusted()
    {
        return CurrentLayer == Layer2D::Box || CurrentLayer == Layer2D::Movie;
    }

    void DrawBlackBehind(const RwIm2DVertex& reference, const Frame& frame)
    {
        auto setRenderState = RwEngine::Field<RenderStateFn>(RwEngine::RenderStateSet);
        void* raster = GetTextureRaster();
        setRenderState(RwEngine::rwRENDERSTATETEXTURERASTER, nullptr);

        RwIm2DVertex quad[4];
        for (auto& vertex : quad)
        {
            vertex = reference;
            vertex.emissiveColor = 0xFF000000;
        }
        quad[0].x = 0.0f;        quad[0].y = 0.0f;
        quad[1].x = frame.width; quad[1].y = 0.0f;
        quad[2].x = frame.width; quad[2].y = frame.height;
        quad[3].x = 0.0f;        quad[3].y = frame.height;
        uint16_t indices[6] = { 0, 1, 2, 0, 2, 3 };
        RenderIndexedPrimitive(rwPRIMTYPETRILIST, quad, 4, indices, 6);

        setRenderState(RwEngine::rwRENDERSTATETEXTURERASTER, raster);
    }

    int32_t __cdecl RenderIndexedPrimitiveHook(int32_t type, RwIm2DVertex* vertices, int32_t count, uint16_t* indices, int32_t numIndices)
    {
        Frame frame;
        if (vertices && count > 0 && IsAdjusted() && GetFrame(frame))
        {
            if (type == rwPRIMTYPETRILIST && count % 4 == 0)
            {
                // The game batches quads of four vertices
                void* raster = GetTextureRaster();
                bool blackBehind = false;
                for (int32_t i = 0; i < count; i += 4)
                    blackBehind |= TransformQuad(vertices + i, raster, frame);
                if (blackBehind)
                    DrawBlackBehind(vertices[0], frame);
            }
            else
                TransformVertices(vertices, count, frame);
        }
        return RenderIndexedPrimitive(type, vertices, count, indices, numIndices);
    }

    int32_t __cdecl RenderPrimitiveHook(int32_t type, RwIm2DVertex* vertices, int32_t count)
    {
        Frame frame;
        if (vertices && count > 0 && IsAdjusted() && GetFrame(frame))
            TransformVertices(vertices, count, frame);
        return RenderPrimitive(type, vertices, count);
    }

    int32_t __cdecl RenderTriangleHook(RwIm2DVertex* vertices, int32_t count, int32_t i1, int32_t i2, int32_t i3)
    {
        Frame frame;
        if (vertices && count > 0 && IsAdjusted() && GetFrame(frame))
            TransformVertices(vertices, count, frame);
        return RenderTriangle(vertices, count, i1, i2, i3);
    }

    int32_t __cdecl RenderLineHook(RwIm2DVertex* vertices, int32_t count, int32_t i1, int32_t i2)
    {
        Frame frame;
        if (vertices && count > 0 && IsAdjusted() && GetFrame(frame))
        {
            float minX, maxX;
            GetExtent(vertices, count, minX, maxX);
            // Lines across the whole screen (scanlines) stay across it
            if (minX > 1.5f || maxX < frame.width - 1.5f)
                TransformVertices(vertices, count, frame);
        }
        return RenderLine(vertices, count, i1, i2);
    }

    template<typename T>
    void Wrap(size_t offset, T& original, T hook)
    {
        auto& slot = RwEngine::Field<T>(offset);
        if (slot && slot != hook)
        {
            original = slot;
            slot = hook;
        }
    }

    void ApplyHudStretch()
    {
        auto stretch = HudStretch.get_ptr();
        if (!stretch)
            return;

        // Sprites placed from the 3D view are sized in screen units: they need the real shape of the screen. The rest
        // is drawn in the 4:3 box, which keeps the original factor.
        float value = DEFAULT_HUD_STRETCH;
        if (CurrentLayer == Layer2D::Projected)
        {
            const float aspect = GetScreenAspectRatio();
            if (aspect > 0.0f)
                value = 1.0f / aspect;
        }
        *stretch = value;
    }
}

export namespace CRenderer
{
    // Puts the 2D adjustments in place once the engine has a device; checked every frame, since the engine fills
    // these functions in when it starts
    void Install()
    {
        auto engine = RwEngine::Instance.get_ptr();
        if (!engine || !*engine)
            return;

        Wrap<RenderLineFn>(RwEngine::Im2DRenderLine, RenderLine, RenderLineHook);
        Wrap<RenderTriangleFn>(RwEngine::Im2DRenderTriangle, RenderTriangle, RenderTriangleHook);
        Wrap<RenderPrimitiveFn>(RwEngine::Im2DRenderPrimitive, RenderPrimitive, RenderPrimitiveHook);
        Wrap<RenderIndexedPrimitiveFn>(RwEngine::Im2DRenderIndexedPrimitive, RenderIndexedPrimitive, RenderIndexedPrimitiveHook);
    }

    Layer2D GetLayer() { return CurrentLayer; }

    Anchor SetAnchor(Anchor anchor)
    {
        auto previous = CurrentAnchor;
        CurrentAnchor = anchor;
        return previous;
    }
}

// The layer (and anchor) of the 2D drawn while it exists
export class Scope2D
{
    Layer2D previousLayer;
    Anchor previousAnchor;
public:
    explicit Scope2D(Layer2D layer, Anchor anchor = Anchor::Center) : previousLayer(CurrentLayer), previousAnchor(CurrentAnchor)
    {
        CurrentLayer = layer;
        CurrentAnchor = anchor;
        ApplyHudStretch();
    }
    ~Scope2D()
    {
        CurrentLayer = previousLayer;
        CurrentAnchor = previousAnchor;
        ApplyHudStretch();
    }
    Scope2D(const Scope2D&) = delete;
    Scope2D& operator=(const Scope2D&) = delete;
};

class Renderer
{
public:
    Renderer()
    {
        WFP::onInitEvent() += []()
        {
            // The frontend starts with the engine (the legal screen and the first loading screen come next)
            auto pattern = hook::pattern("53 56 55 83 EC 24 68 ? ? ? ? E8");
            if (!pattern.empty())
            {
                static auto FrontendInitialise = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    CRenderer::Install();
                });
            }
        };

        WFP::onGameProcessEvent() += []()
        {
            CRenderer::Install();
        };
    }
} Renderer;
