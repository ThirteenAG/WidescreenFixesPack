module;

#include <stdafx.h>
#include <d3d9.h>
#include <atomic>
#include <unordered_set>
#include "FixSettings.hpp"

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
    inline std::atomic<int> Borders{ 3 };
    inline int32_t LetterboxShift = 0;
    inline SafetyHookMid ScaleHook, ViewportHook, OverlayViewportHook, ClearHook, SpriteHook, SpriteDoneHook;
    inline SafetyHookInline LetterboxHook, BarsHook;

    // The frontend menus (main menu, options, skater and level select...) are 4:3 pictures between graffiti strips: 2D
    // parts and 3D pictures made for a 4:3 screen, which spill past the frame on a wider one. These menus keep the 4:3
    // view in the frame; other screens (the create-a-skater room, the game) fill the screen
    inline std::unordered_set<uintptr_t> MenuStrips;    // the strips' drawn quads
    inline bool MenuStripDrawn = false;                 // in this frame's 2D layer
    inline bool FramedMenu = false;                     // the last frame showed a frontend menu

    inline void UpdateBorders()
    {
        const auto value = FixSettings().Get("MAIN.CutsceneBorders");
        Borders.store(std::clamp(std::atoi(value.c_str()), 0, 3), std::memory_order_relaxed);
    }

    // The cutscene picture on the screen: Both keeps the original 16:9 picture of the 4:3 frame, Letterbox spans the
    // screen width between the original bars, Pillarbox fills the height up to 16:9, Off fills the screen
    inline Rect Picture()
    {
        const float width = static_cast<float>(FrameTargetWidth.get()), height = static_cast<float>(FrameTargetHeight.get());
        const int mode = Borders.load(std::memory_order_relaxed);
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

    // The original FOV of a cutscene camera frames 16:9; the picture widens it Hor+. Frontend menus keep 4:3
    inline float FovAspect()
    {
        if (FrameLetterboxed)
            return Aspect() * 0.75f;
        return FramedMenu ? 4.0f / 3.0f : ScreenAspect;
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

    // The 640x480 frame on the render target
    inline RECT FrameRect()
    {
        const LONG width = FrameTargetWidth.get(), height = FrameTargetHeight.get();
        const LONG frameWidth = std::min(width, static_cast<LONG>(std::lround(height * (4.0f / 3.0f))));
        const LONG frameHeight = std::min(height, static_cast<LONG>(std::lround(width * 0.75f)));
        const LONG left = (width - frameWidth) / 2, top = (height - frameHeight) / 2;
        return { left, top, left + frameWidth, top + frameHeight };
    }

    inline void SetViewport()
    {
        ProjectionAspect = Aspect();
        auto& viewport = FrameViewport.get();
        const bool fullWidth = viewport[0] == 0 && viewport[2] == FrameTargetWidth.get();
        if (FramedMenu && !FrameLetterboxed && fullWidth && viewport[1] == 0 && viewport[3] == FrameTargetHeight.get())
        {
            const auto frame = FrameRect();
            viewport = { frame.left, frame.top, frame.right - frame.left, frame.bottom - frame.top };
            ProjectionAspect = static_cast<float>(viewport[2]) / static_cast<float>(viewport[3]);
            return;
        }
        if (!FrameLetterboxed || !fullWidth)
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

    // The game never clears the screen: its pictures cover the 4:3 frame, so the sides of a menu kept old frames
    inline void ClearScreen()
    {
        FramedMenu = MenuStripDrawn;
        MenuStripDrawn = false;
        auto device = GameDevice.get();
        if (!device)
            return;
        D3DVIEWPORT9 saved{};
        if (FAILED(device->GetViewport(&saved)))
            return;
        D3DVIEWPORT9 full{ 0, 0, static_cast<DWORD>(FrameTargetWidth.get()), static_cast<DWORD>(FrameTargetHeight.get()), 0.0f, 1.0f };
        device->SetViewport(&full);
        device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
        device->SetViewport(&saved);
    }

    inline DWORD SavedScissor = FALSE;
    inline RECT SavedScissorRect{};
    inline bool Scissored = false;

    // A screen element's sprite (its drawn quad), from the element's draw
    inline void PlaceSprite(uintptr_t element, bool menuStrip)
    {
        const auto sprite = *reinterpret_cast<uintptr_t*>(element + 0x160);
        if (!sprite)
            return;
        const auto quad = *reinterpret_cast<uintptr_t*>(sprite + 0x3C);
        if (!quad)
            return;
        if (menuStrip)
            MenuStrips.insert(quad);
        else
            MenuStrips.erase(quad);
    }

    // Menu pictures larger than the frame (off the screen on a 4:3 display) end at its edges
    inline bool BeforeSprite(uintptr_t quad)
    {
        auto device = GameDevice.get();
        if (!device)
            return true;
        if (MenuStrips.contains(quad))
            MenuStripDrawn = true;
        if (!FramedMenu)
            return true;
        RECT clip = FrameRect();
        if (FAILED(device->GetRenderState(D3DRS_SCISSORTESTENABLE, &SavedScissor)) ||
            FAILED(device->GetScissorRect(&SavedScissorRect)))
            return true;
        if (SavedScissor)
        {
            RECT intersection{};
            if (!IntersectRect(&intersection, &clip, &SavedScissorRect))
                return false;
            clip = intersection;
        }
        if (FAILED(device->SetScissorRect(&clip)))
            return true;
        if (!SavedScissor && FAILED(device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE)))
        {
            device->SetScissorRect(&SavedScissorRect);
            return true;
        }
        Scissored = true;
        return true;
    }

    inline void AfterSprite()
    {
        if (!Scissored)
            return;
        Scissored = false;
        auto device = GameDevice.get();
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, SavedScissor);
        device->SetScissorRect(&SavedScissorRect);
    }

    inline void Install(float width, float height)
    {
        FixSettings().OnChange("MAIN.CutsceneBorders", UpdateBorders);
        UpdateBorders();
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
        auto sprite = hook::pattern("6A 1C 8D 54 24 ? 52 6A 65 68 ? ? ? ? 6A 02 6A 04 6A 00 6A 04 50 8B 81 50 01 00 00 FF D0");
        auto render = hook::pattern("80 3D ? ? ? ? 00 53 55 56 57 0F 85 ? ? ? ? A1 ? ? ? ? 33 DB 3B C3 74 0D 50 FF 15");

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
        ClearHook = safetyhook::create_mid(render.get_first(17), [](SafetyHookContext&) { ClearScreen(); });
        // The sprite's draw call, the quad in esi
        static const auto spriteReturn = reinterpret_cast<uintptr_t>(sprite.get_first(31));
        SpriteHook = safetyhook::create_mid(sprite.get_first(23), [](SafetyHookContext& regs)
        {
            if (!BeforeSprite(regs.esi))
            {
                // Skip DrawIndexedPrimitiveUP when the two clip rectangles do not overlap.
                regs.esp += 9 * sizeof(uint32_t);
                regs.eax = D3D_OK;
                regs.eip = spriteReturn;
            }
        });
        SpriteDoneHook = safetyhook::create_mid(sprite.get_first(31), [](SafetyHookContext&) { AfterSprite(); });
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
