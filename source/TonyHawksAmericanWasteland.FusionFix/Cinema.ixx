module;

#include <stdafx.h>
#include <d3d9.h>

export module Cinema;

import Frontend;
import ComVars;

// Story cutscenes and in-game scenes cover the 4:3 view with two black sprites (letterbox_anchor) and widen the camera
// angle, leaving a strip of 336 of the frame's 480 lines. MAIN.CutsceneBorders places that picture on the screen:
// Both keeps it as in the 4:3 frame, Top/bottom spans the screen width between the bars, Sides fills the screen height
// up to the strip's shape, None fills the whole screen. The screen outside the picture is cleared here (the bar
// sprites, hidden without top and bottom borders, still cover the scene's 2D parts) and the camera angle follows the
// picture (FovAspect).
export namespace THAWCinema
{
    struct Rect
    {
        LONG x, y, width, height;
    };

    inline SafetyHookMid PresentHook, OverlayHook;
    inline const int32_t* Mode = nullptr;
    inline int Width = 0, Height = 0;
    inline bool Active = false;
    inline float Strip = 0.0f;              // the picture's share of the 4:3 frame height, 0 without bars
    inline void (*BeforePresent)() = nullptr;

    inline bool InScene()
    {
        if (!NativeSymbols.is_initialized() || !NativeSymbols.get() || !THAWMenu::api.symbol)
            return false;
        auto* symbol = static_cast<const uint8_t*>(THAWMenu::api.symbol(THAWMenu::Checksum("cutscene_is_playing")));
        if (symbol && symbol[2] == 1 && *reinterpret_cast<const int32_t*>(symbol + 12))
            return true;
        return THAWMenu::Lookup("igc_anchor") || THAWMenu::Lookup("letterbox_anchor");
    }

    inline int Borders()
    {
        return Mode ? std::clamp(*Mode, 0, 3) : 3;
    }

    inline Rect Picture()
    {
        const float width = static_cast<float>(Width), height = static_cast<float>(Height);
        const float frameHeight = std::min(height, width * 0.75f);
        float w = width, h = height;
        if (Strip > 0.0f)
        {
            if (Borders() & 1)
                h = frameHeight * Strip;
            if (Borders() & 2)
                w = std::min(width, h * (4.0f / 3.0f) / Strip);
        }
        const LONG pictureWidth = std::lround(w), pictureHeight = std::lround(h);
        return { (Width - pictureWidth) / 2, (Height - pictureHeight) / 2, pictureWidth, pictureHeight };
    }

    // The aspect the camera angle is widened for (Hor+ from the 4:3 angle): the picture shows the strip's height
    inline float FovAspect(float screenAspect)
    {
        if (Strip <= 0.0f || Height <= 0)
            return screenAspect;
        const auto picture = Picture();
        return screenAspect * Strip * static_cast<float>(Height) / static_cast<float>(picture.height);
    }

    // Black outside the picture. Before the 2D layer it covers the 3D view (the scene's subtitles stay); before the
    // frame is shown it covers the parts outside the 4:3 frame, where some scenes draw their picture as a 2D element and
    // the native bars don't reach
    inline void ClearBars(IDirect3DDevice9* device, bool outsideFrame)
    {
        if (!device || Strip <= 0.0f || Width <= 0 || Height <= 0)
            return;
        const auto picture = Picture();
        const LONG left = picture.x, top = picture.y, right = picture.x + picture.width, bottom = picture.y + picture.height;
        const RECT all[4]{ { 0, 0, Width, top }, { 0, bottom, Width, Height }, { 0, top, left, bottom }, { right, top, Width, bottom } };
        const LONG frameWidth = std::min<LONG>(Width, Height * 4 / 3), frameHeight = std::min<LONG>(Height, Width * 3 / 4);
        const RECT frame{ (Width - frameWidth) / 2, (Height - frameHeight) / 2, (Width + frameWidth) / 2, (Height + frameHeight) / 2 };
        D3DRECT bars[16];
        DWORD count = 0;
        for (const auto& bar : all)
        {
            if (bar.right <= bar.left || bar.bottom <= bar.top)
                continue;
            if (!outsideFrame)
            {
                bars[count++] = { bar.left, bar.top, bar.right, bar.bottom };
                continue;
            }
            // The bar without the frame: up to four pieces around it
            const RECT pieces[4]{ { bar.left, bar.top, bar.right, std::min(bar.bottom, frame.top) },
                { bar.left, std::max(bar.top, frame.bottom), bar.right, bar.bottom },
                { bar.left, std::max(bar.top, frame.top), std::min(bar.right, frame.left), std::min(bar.bottom, frame.bottom) },
                { std::max(bar.left, frame.right), std::max(bar.top, frame.top), bar.right, std::min(bar.bottom, frame.bottom) } };
            for (const auto& piece : pieces)
            {
                if (piece.right > piece.left && piece.bottom > piece.top)
                    bars[count++] = { piece.left, piece.top, piece.right, piece.bottom };
            }
        }
        if (!count)
            return;
        D3DVIEWPORT9 saved{};
        if (FAILED(device->GetViewport(&saved)))
            return;
        D3DVIEWPORT9 full{ 0, 0, static_cast<DWORD>(Width), static_cast<DWORD>(Height), 0.0f, 1.0f };
        if (SUCCEEDED(device->SetViewport(&full)))
            device->Clear(count, bars, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
        device->SetViewport(&saved);
    }

    inline void Install(const int32_t* mode, int width, int height)
    {
        Mode = mode;
        Width = width;
        Height = height;
        auto present = hook::pattern("A1 ? ? ? ? 8B 08 6A 00 6A 00 6A 00 6A 00 50 FF 51 44");
        auto overlay = hook::pattern("6A 00 E8 ? ? ? ? 6A 00 E8 ? ? ? ? 0F 57 C0 83 C4 08 85 C0");
        PresentHook = safetyhook::create_mid(present.get_first(16), [](SafetyHookContext&)
        {
            ClearBars(GameDevice.get(), true);
            if (BeforePresent)
                BeforePresent();
            Active = InScene();
            Strip = Active && THAWMenu::Lookup("letterbox_anchor") ? 336.0f / 480.0f : 0.0f;
        });
        // The viewport covers the whole target here, after the screen effects and before the screen elements
        OverlayHook = safetyhook::create_mid(overlay.get_first(7), [](SafetyHookContext&) { ClearBars(GameDevice.get(), false); });
    }
}
