module;

#include <stdafx.h>
#include <d3d9.h>

export module Frame;

import ComVars;

// The 640x480 2D frame (pillarboxed on wider screens, letterboxed on narrower ones) and the cutscene picture.
// Cutscenes switch the game to its letterbox screen: a 16:9 picture between bars in the 4:3 frame.
export namespace THUGFrame
{
    struct Rect
    {
        int x, y, width, height;
    };

    inline float FrameWidth = 640.0f, FrameHeight = 480.0f, OffsetY = 0.0f;
    inline float ScreenAspect = 4.0f / 3.0f;
    inline float ProjectionAspect = 4.0f / 3.0f;   // read by the game's projection through its aspect getter
    inline const int32_t* Borders = nullptr;
    inline int32_t LetterboxShift = 0;
    inline SafetyHookMid ScaleHook, ViewportHook, OverlayViewportHook;
    inline SafetyHookInline LetterboxHook, BarsHook;

    // The cutscene picture on the screen: Both keeps the original 16:9 picture of the 4:3 frame, Letterbox spans the
    // screen width between the original bars, Pillarbox fills the height up to 16:9, Off fills the screen
    inline Rect Picture()
    {
        const float width = static_cast<float>(FrameTargetWidth.get()), height = static_cast<float>(FrameTargetHeight.get());
        const int mode = Borders ? std::clamp(*Borders, 0, 3) : 3;
        float w = width, h = height;
        if (mode & 1)
            h = std::min(height, width * 0.75f) * 0.75f;
        if (mode & 2)
            w = std::min(width, h * (16.0f / 9.0f));
        const int pictureWidth = static_cast<int>(std::lround(w)), pictureHeight = static_cast<int>(std::lround(h));
        return { (FrameTargetWidth.get() - pictureWidth) / 2, (FrameTargetHeight.get() - pictureHeight) / 2, pictureWidth, pictureHeight };
    }

    inline float Aspect()
    {
        if (!FrameLetterboxed || FrameTargetWidth.get() <= 0 || FrameTargetHeight.get() <= 0)
            return ScreenAspect;
        const auto picture = Picture();
        return static_cast<float>(picture.width) / static_cast<float>(picture.height);
    }

    // The original FOV of a cutscene camera frames 16:9; the picture widens it Hor+
    inline float FovAspect()
    {
        return FrameLetterboxed ? Aspect() * 0.75f : ScreenAspect;
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
        ProjectionAspect = Aspect();
    }

    inline void SetViewport()
    {
        ProjectionAspect = Aspect();
        if (!FrameLetterboxed || FrameViewport.get()[0] != 0 || FrameViewport.get()[2] != FrameTargetWidth.get())
            return;
        const auto picture = Picture();
        FrameViewport.get()[0] = picture.x;
        FrameViewport.get()[1] = picture.y;
        FrameViewport.get()[2] = picture.width;
        FrameViewport.get()[3] = picture.height;
    }

    // Black outside the picture, over the finished frame as the native bars are
    inline void __cdecl DrawBars()
    {
        if (!GameDevice.get())
            return;
        const auto picture = Picture();
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

    inline void Install(float width, float height, const int32_t* borders)
    {
        Borders = borders;
        FrameWidth = std::min(width, height * (4.0f / 3.0f));
        FrameHeight = std::min(height, width * 0.75f);
        OffsetY = (height - FrameHeight) * 0.5f;
        ScreenAspect = width / height;
        ProjectionAspect = ScreenAspect;

        auto scale = hook::pattern("89 3D ? ? ? ? D9 1D ? ? ? ? D9 05 ? ? ? ? DC 0D ? ? ? ? E8 ? ? ? ? A3 ? ? ? ? 8B 16");
        auto letterbox = hook::pattern("80 7C 24 04 00 74 42 80 3D ? ? ? ? 00 75 76 DB 05");
        auto bars = hook::pattern("81 EC F8 00 00 00 6A 00 6A 00 6A 00 E8 ? ? ? ? D9 E8 83 C4 0C");
        auto viewport = hook::pattern("01 05 ? ? ? ? 8B C1 99 83 E2 03 03 C2 C1 F8 02 F7 D8 01 05 ? ? ? ? A1");
        auto overlayViewport = hook::pattern("A3 ? ? ? ? 8B C1 99 83 E2 03 03 C2 C1 F8 02 2B C8 89 0D ? ? ? ? A1");
        auto device = hook::pattern("A1 ? ? ? ? 8B 08 8B 51 40 68 ? ? ? ? 50 FF D2");
        auto target = hook::pattern("D9 EE A1 ? ? ? ? D9 1D ? ? ? ? D9 E8 8B 0D ? ? ? ? D9 1D");

        FrameScaleY.SetAddress(*scale.get_first<float*>(8));
        FrameOffset2D.SetAddress(*scale.get_first<int32_t*>(30));
        FrameLetterboxed.SetAddress(*letterbox.get_first<uint8_t*>(9));
        FrameViewport.SetAddress(*viewport.get_first<int32_t*>(2) - 1);
        GameDevice.SetAddress(*device.get_first<IDirect3DDevice9**>(1));
        FrameTargetWidth.SetAddress(*target.get_first<int32_t*>(3));
        FrameTargetHeight.SetAddress(*target.get_first<int32_t*>(17));

        // Screens narrower than 4:3: the frame keeps the full width and is centred vertically
        ScaleHook = safetyhook::create_mid(scale.get_first(34), [](SafetyHookContext&)
        {
            if (OffsetY <= 0.0f)
                return;
            FrameScaleY = FrameHeight / 480.0f;
            FrameOffset2D = static_cast<int32_t>(OffsetY) + static_cast<int32_t>(FrameScaleY * 16.0f);
        });
        LetterboxHook = safetyhook::create_inline(letterbox.get_first(), SetLetterbox);
        BarsHook = safetyhook::create_inline(bars.get_first(), DrawBars);
        ViewportHook = safetyhook::create_mid(viewport.get_first(25), [](SafetyHookContext&) { SetViewport(); });
        OverlayViewportHook = safetyhook::create_mid(overlayViewport.get_first(24), [](SafetyHookContext&) { SetViewport(); });
    }
}

export void InstallAspectRatio()
{
    hook::pattern pattern("");
    //Aspect Ratio
    pattern = hook::pattern("D9 05 ? ? ? ? 51 D9 1C 24 E8 ? ? ? ? 6A 00");
    injector::WriteMemory<float>(*pattern.get_first<float*>(2), Screen.fAspectRatio, true);

    pattern = hook::pattern("E8 ? ? ? ? D8 0C 24 D8 74 24 04 D9 5C 24 04 D9 44 24 04 83 C4 08 C3");
    injector::WriteMemory<float*>(reinterpret_cast<float**>(injector::GetBranchDestination(pattern.get_first()).as_int() + 2), &THUGFrame::ProjectionAspect, true);
}
