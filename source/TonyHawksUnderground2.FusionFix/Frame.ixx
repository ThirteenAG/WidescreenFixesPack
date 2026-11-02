module;

#include <stdafx.h>
#include <d3d9.h>

export module Frame;

import ComVars;
import Frontend;

// The 640x480 2D frame (pillarboxed on wider screens, letterboxed on narrower ones) and the cutscene picture.
// Story cutscenes cover the 4:3 view with black sprite bars (a strip of about 1.9:1 with a wider camera angle), chapter
// intro cameras do the same with taller bars, and goal cameras switch the game to its letterbox screen (a 16:9 picture
// between bars, the 2D layer squeezed into it). The bars are replaced by the picture rect and clears outside it.
export namespace THUG2Frame
{
    struct Rect
    {
        int x, y, width, height;
    };

    inline float FrameWidth = 640.0f, FrameHeight = 480.0f, OffsetY = 0.0f;
    inline float ScreenAspect = 4.0f / 3.0f;
    inline float ProjectionAspect = 4.0f / 3.0f;   // read by the game's projection through its aspect getter
    inline float Strip = 0.0f;                      // the picture's share of the 4:3 frame height, 0 without bars
    inline const int32_t* Borders = nullptr;
    inline int32_t LetterboxShift = 0;
    inline SafetyHookMid ScaleHook, ViewportHook, OverlayViewportHook;
    inline SafetyHookInline LetterboxHook;

    constexpr uint32_t BarsId = THUG2Menu::Checksum("letterbox_anchor");

    // The bars in use: the native letterbox screen keeps 360 of 480 lines, the story cutscene sprites 336, the
    // chapter intro sprites 280 (in the 448-line 2D layer below its 16-line top offset)
    inline float FindStrip()
    {
        if (FrameLetterboxed)
            return 0.75f;
        THUG2Menu::Weak bars;
        bars.Find(BarsId);
        if (!bars.Get())
            return 0.0f;
        return AngleRatio.get() > 1.01f ? 336.0f / 480.0f : 280.0f / 480.0f;
    }

    // The cutscene picture on the screen: Both keeps the original picture of the 4:3 frame, Letterbox spans the
    // screen width between the original bars, Pillarbox fills the height up to the picture's aspect, Off fills the
    // screen
    inline Rect Picture()
    {
        const float width = static_cast<float>(FrameTargetWidth.get()), height = static_cast<float>(FrameTargetHeight.get());
        const int mode = Borders ? std::clamp(*Borders, 0, 3) : 3;
        float w = width, h = height;
        if (mode & 1)
            h = std::min(height, width * 0.75f) * Strip;
        if (mode & 2)
            w = std::min(width, h * (4.0f / 3.0f) / Strip);
        const int pictureWidth = static_cast<int>(std::lround(w)), pictureHeight = static_cast<int>(std::lround(h));
        return { (FrameTargetWidth.get() - pictureWidth) / 2, (FrameTargetHeight.get() - pictureHeight) / 2, pictureWidth, pictureHeight };
    }

    inline float Aspect()
    {
        if (Strip <= 0.0f || FrameTargetWidth.get() <= 0 || FrameTargetHeight.get() <= 0)
            return ScreenAspect;
        const auto picture = Picture();
        return static_cast<float>(picture.width) / static_cast<float>(picture.height);
    }

    // The original camera angle frames the bars' strip of the 4:3 view; the picture widens it Hor+
    inline float FovAspect()
    {
        return Strip > 0.0f ? Aspect() * Strip : ScreenAspect;
    }

    // The native switch squeezes the 2D layer into the letterbox area of the frame
    inline void __cdecl SetLetterbox(char on)
    {
        if (on && !FrameLetterboxed)
        {
            FrameLetterboxed = 1;
            LetterboxShift = static_cast<int32_t>(FrameHeight) / 4 / 2;
            FrameOffset2D += LetterboxShift;
            FrameScaleY = FrameHeight / 480.0f * 0.75f;
        }
        else if (!on && FrameLetterboxed)
        {
            FrameLetterboxed = 0;
            FrameOffset2D -= LetterboxShift;
            FrameScaleY = FrameHeight / 480.0f;
        }
        Strip = FindStrip();
        ProjectionAspect = Aspect();
    }

    // Black outside the picture; called while the viewport still covers the whole target
    inline void ClearBars(const Rect& picture)
    {
        if (!GameDevice.get())
            return;
        const LONG width = FrameTargetWidth.get(), height = FrameTargetHeight.get();
        const LONG left = picture.x, top = picture.y, right = picture.x + picture.width, bottom = picture.y + picture.height;
        D3DRECT bars[4];
        DWORD count = 0;
        if (top > 0)
            bars[count++] = { 0, 0, width, top };
        if (bottom < height)
            bars[count++] = { 0, bottom, width, height };
        if (left > 0)
            bars[count++] = { 0, top, left, bottom };
        if (right < width)
            bars[count++] = { right, top, width, bottom };
        if (count)
            (GameDevice.get())->Clear(count, bars, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
    }

    // The 3D view of one viewport (overlay: the 2D layer's viewport, squeezed only by the native letterbox)
    inline void SetViewport(bool overlay)
    {
        if (!overlay)
            Strip = FindStrip();
        ProjectionAspect = Aspect();
        if (Strip <= 0.0f || (overlay && !FrameLetterboxed) || FrameViewport.get()[0] != 0 || FrameViewport.get()[2] != FrameTargetWidth.get())
            return;
        const auto picture = Picture();
        if (!overlay)
            ClearBars(picture);
        FrameViewport.get()[0] = picture.x;
        FrameViewport.get()[1] = picture.y;
        FrameViewport.get()[2] = picture.width;
        FrameViewport.get()[3] = picture.height;
    }

    inline void Install(float width, float height, const int32_t* borders)
    {
        Borders = borders;
        FrameWidth = std::min(width, height * (4.0f / 3.0f));
        FrameHeight = std::min(height, width * 0.75f);
        OffsetY = (height - FrameHeight) * 0.5f;
        ScreenAspect = width / height;
        ProjectionAspect = ScreenAspect;

        auto scale = hook::pattern("89 1D ? ? ? ? D9 1D ? ? ? ? D9 05 ? ? ? ? D8 0D ? ? ? ? E8 ? ? ? ? A3 ? ? ? ? 8B 16");
        auto letterbox = hook::pattern("8A 44 24 04 84 C0 A0 ? ? ? ? 74 37");
        auto viewport = hook::pattern("F7 D8 03 C8 89 0D ? ? ? ? A1 ? ? ? ? 8B 08 68 ? ? ? ? 50 FF 91 BC 00 00 00");
        auto overlayViewport = hook::pattern("2B C8 89 0D ? ? ? ? A1 ? ? ? ? 8B 08 68 ? ? ? ? 50 FF 91 BC 00 00 00");
        auto target = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? A3 ? ? ? ? A0 ? ? ? ? 84 C0 89 1D");
        auto angle = hook::pattern("D9 C9 D9 C9 DE F9 D9 1D");

        FrameScaleY.SetAddress(*scale.get_first<float*>(8));
        FrameOffset2D.SetAddress(*scale.get_first<int32_t*>(30));
        FrameLetterboxed.SetAddress(*letterbox.get_first<uint8_t*>(7));
        FrameViewport.SetAddress(*viewport.get_first<int32_t*>(18));
        GameDevice.SetAddress(*viewport.get_first<IDirect3DDevice9**>(11));
        FrameTargetWidth.SetAddress(*target.get_first<int32_t*>(1));
        FrameTargetHeight.SetAddress(*target.get_first<int32_t*>(7));
        AngleRatio.SetAddress(*angle.get_first<float*>(8));

        // Screens narrower than 4:3: the frame keeps the full width and is centred vertically
        ScaleHook = safetyhook::create_mid(scale.get_first(34), [](SafetyHookContext&)
        {
            if (OffsetY <= 0.0f)
                return;
            FrameScaleY = FrameHeight / 480.0f;
            FrameOffset2D = static_cast<int32_t>(OffsetY) + static_cast<int32_t>(FrameScaleY * 16.0f);
        });
        LetterboxHook = safetyhook::create_inline(letterbox.get_first(), SetLetterbox);
        ViewportHook = safetyhook::create_mid(viewport.get_first(10), [](SafetyHookContext&) { SetViewport(false); });
        OverlayViewportHook = safetyhook::create_mid(overlayViewport.get_first(8), [](SafetyHookContext&) { SetViewport(true); });
        if (!ScaleHook || !LetterboxHook || !ViewportHook || !OverlayViewportHook)
            std::terminate();
    }
}

export void InstallAspectRatio()
{
    hook::pattern pattern("");
    //Aspect Ratio
    pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 6A 00 68 ? ? ? ? E8 ? ? ? ? D9");
    injector::WriteMemory<float>(pattern.get_first<float>(1), Screen.fAspectRatio, true);
    pattern = hook::pattern("E8 ? ? ? ? D9 04 24 D8 74 24 04 DE C9");
    injector::WriteMemory<float*>(reinterpret_cast<float**>(injector::GetBranchDestination(pattern.get_first()).as_int() + 2), &THUG2Frame::ProjectionAspect, true);
}
