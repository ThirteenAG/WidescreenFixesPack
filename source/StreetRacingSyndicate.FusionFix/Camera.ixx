module;

#include <stdafx.h>
#include "FixSettings.hpp"

// The projection: the game keeps the horizontal view and derives the vertical one from the viewport's shape. The fix
// keeps the vertical view of the 4:3 picture for full-screen views (Hor+), with MAIN.FOVFactor while driving.
// Scripted camera moments narrow the view to a 16:9 band inside the 4:3 picture, with black bars above and below. The fix
// places that band by MAIN.CutsceneBorders and keeps the band's framing: the full band is always visible, the rest of
// the screen shows more around it.

export module Camera;

import ComVars;
import Hud;

export namespace SRSCamera
{
    inline int Width = 640, Height = 480;
    inline int BandPixels[4]{ -1, -1, -1, -1 };         // the band's viewport in pixels while a scripted camera runs
    inline char (__cdecl* DrawRect)(const int*, uint32_t, uint32_t) = nullptr;
    inline float View[6]{};                             // the band as the fix places it
    inline SafetyHookInline ProjectionHook, ViewHook, BarsHook, ProjectHook, ProjectClippedHook;

    // The band's progress (0 = full view, 1 = band) and its rectangle in pixels
    inline float Progress()
    {
        if (!FixSettings().Bool("MAIN.AnimatedCutsceneBorders"))
            return BandDirection.get() == -1 ? 0.0f : 1.0f;
        return BandTarget.get()[1] > 0.0f ? std::clamp(BandRect.get()[1] / BandTarget.get()[1], 0.0f, 1.0f) : 1.0f;
    }

    inline void Band(float& x, float& y, float& w, float& h)
    {
        const float screenWidth = static_cast<float>(Width), screenHeight = static_cast<float>(Height);
        constexpr float band = 16.0f / 9.0f;
        float bandWidth = screenWidth, bandHeight = screenHeight;
        switch (std::clamp(FixSettings().Int("MAIN.CutsceneBorders"), 0, 3))
        {
            case 1: // bars above and below where the screen is narrower than the band
                bandHeight = std::min(screenHeight, screenWidth / band);
                break;
            case 2: // bars at the sides where the screen is wider than the band
                bandWidth = std::min(screenWidth, screenHeight * band);
                break;
            case 3:
                bandWidth = std::min(screenWidth, screenHeight * band);
                bandHeight = bandWidth / band;
                break;
        }
        const float progress = Progress();
        w = screenWidth + (bandWidth - screenWidth) * progress;
        h = screenHeight + (bandHeight - screenHeight) * progress;
        x = (screenWidth - w) * 0.5f;
        y = (screenHeight - h) * 0.5f;
    }

    inline void __fastcall Projection(uint8_t* renderer, void*)
    {
        ProjectionHook.thiscall<void>(renderer);
        const float fov = *reinterpret_cast<float*>(renderer + 28);
        const float aspect = *reinterpret_cast<float*>(renderer + 68);
        const int* viewport = reinterpret_cast<int*>(renderer + 36); // x, y, width, height in pixels
        const int viewWidth = viewport[2];
        const int viewHeight = viewport[3];
        const bool band = BandActive.get() && std::equal(viewport, viewport + 4, BandPixels, [](int a, int b) { return std::abs(a - b) <= 1; });
        float horizontal = 0.0f, vertical = 0.0f;
        if (band)
        {
            // The band's own framing: 16:9 with the game's horizontal view; more around it, never less
            const float tangent = std::tan(fov * 0.5f);
            vertical = tangent * (9.0f / 16.0f);
            horizontal = vertical * aspect;
            if (aspect < 16.0f / 9.0f)
            {
                horizontal = tangent;
                vertical = tangent / aspect;
            }
        }
        else if (viewWidth == Width && viewHeight == Height)
        {
            const float factor = SRSHud::Driving ? std::clamp(FixSettings().Float("MAIN.FOVFactor"), 0.5f, 2.0f) : 1.0f;
            vertical = std::tan(std::min(fov * factor * 0.5f, 1.45f)) * 0.75f;
            horizontal = vertical * aspect;
        }
        else
            return; // mirrors, reflections and other views keep the game's projection
        *reinterpret_cast<float*>(renderer + 496) = 1.0f / horizontal;
        *reinterpret_cast<float*>(renderer + 516) = 1.0f / vertical;
    }

    // A player view: the camera's viewport is the band while a scripted camera runs
    inline int __fastcall SetupView(uint8_t* camera, void*)
    {
        auto& viewport = *reinterpret_cast<float**>(camera + 368);
        if (viewport != BandRect.get().data())
            return ViewHook.thiscall<int>(camera);
        float x, y, w, h;
        Band(x, y, w, h);
        const float units = 480.0f / static_cast<float>(Height); // the viewport's 640x480 units scale with the height
        View[0] = x * units;
        View[1] = y * units;
        View[2] = w * units;
        View[3] = h * units;
        View[4] = BandRect.get()[4];
        View[5] = BandRect.get()[5];
        BandPixels[0] = static_cast<int>(x);
        BandPixels[1] = static_cast<int>(y);
        BandPixels[2] = static_cast<int>(w);
        BandPixels[3] = static_cast<int>(h);
        viewport = View;
        const int result = ViewHook.thiscall<int>(camera);
        viewport = BandRect.get().data();
        return result;
    }

    // The bars around the band, over the whole screen
    inline char __cdecl Bars()
    {
        SRSHud::PictureWidth = static_cast<float>(Width);
        if (!BandActive.get())
            return 0;
        float x, y, w, h;
        Band(x, y, w, h);
        // The HUD stays on the picture
        SRSHud::PictureWidth = w;
        const float screenWidth = static_cast<float>(Width), screenHeight = static_cast<float>(Height);
        const auto bar = [&](float left, float top, float right, float bottom)
        {
            if (right - left < 0.5f || bottom - top < 0.5f)
                return;
            // Placed by the fix: 640x480 units over the whole screen
            const int rectangle[]{ static_cast<int>(std::floor(left / screenWidth * 640.0f)), static_cast<int>(std::floor(top / screenHeight * 480.0f)),
                static_cast<int>(std::ceil((right - left) / screenWidth * 640.0f)) + 1, static_cast<int>(std::ceil((bottom - top) / screenHeight * 480.0f)) + 1 };
            SRSHud::Raw = true;
            DrawRect(rectangle, BarDepth.get(), 0xFF000000);
            SRSHud::Raw = false;
        };
        bar(0.0f, 0.0f, screenWidth, y);
        bar(0.0f, y + h, screenWidth, screenHeight);
        bar(0.0f, y, x, y + h);
        bar(x + w, y, screenWidth, y + h);
        return 1;
    }

    // Points projected from the world to the 640x480 layout land where the Hor+ view shows them
    inline void Unsqueeze(int* point)
    {
        if (SRSHud::Scale >= 1.0f)
            point[0] = static_cast<int>(320.0f + (point[0] - 320.0f) * SRSHud::Scale);
        else
            point[1] = static_cast<int>(240.0f + (point[1] - 240.0f) / SRSHud::Scale);
    }

    inline bool __cdecl Project(void* world, int* point)
    {
        const bool result = ProjectHook.ccall<bool>(world, point);
        Unsqueeze(point);
        return result;
    }

    inline bool __cdecl ProjectClipped(void* world, int* point)
    {
        const bool result = ProjectClippedHook.ccall<bool>(world, point);
        Unsqueeze(point);
        return result;
    }

    inline void Init(int width, int height)
    {
        Width = width;
        Height = height;
        auto projection = hook::pattern("51 8B D1 D9 42 40 56 D9 42 1C 57 D8 0D ? ? ? ? 8D B2");
        ProjectionHook = safetyhook::create_inline(projection.get_first(), Projection);
        auto view = hook::pattern("81 EC 88 00 00 00 53 8B D9 D9 43 40 56 D8 83 24 01 00 00 57");
        ViewHook = safetyhook::create_inline(view.get_first(), SetupView);

        auto bars = hook::pattern("A0 ? ? ? ? 84 C0 74 32 A1 ? ? ? ? 6A 00 68 00 00 00 FF 50 68");
        BandActive.SetAddress(*bars.get_first<uint8_t*>(1));
        BarDepth.SetAddress(*bars.get_first<uint32_t*>(10));
        auto barRects = *bars.get_first<uint8_t*>(23);
        // The band's globals follow the two bar rectangles
        BandRect.SetAddress(reinterpret_cast<float*>(barRects + 0x20));
        BandTarget.SetAddress(reinterpret_cast<float*>(barRects + 0x38));
        DrawRect = reinterpret_cast<decltype(DrawRect)>(injector::GetBranchDestination(bars.get_first(27)).as_int());
        auto animation = hook::pattern("51 A0 ? ? ? ? 53 33 DB 3A C3 0F 84");
        BandDirection.SetAddress(*animation.get_first<int8_t*>(2));
        BarsHook = safetyhook::create_inline(bars.get_first(), Bars);

        auto project = hook::pattern("8B 0D ? ? ? ? 83 EC 18 56 8B F1 E8");
        ProjectHook = safetyhook::create_inline(project.get_first(), Project);
        auto projectClipped = hook::pattern("8B 0D ? ? ? ? 83 EC 58 56 57 8B F1 E8");
        ProjectClippedHook = safetyhook::create_inline(projectClipped.get_first(), ProjectClipped);
    }
}
