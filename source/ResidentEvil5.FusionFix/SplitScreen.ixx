module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>

export module SplitScreen;

// Local split-screen co-op (game mode 1). The game's split mode option chooses between two layouts, both one player
// above the other: High gives each player a full-width half, Low a smaller 20:9 view in a staggered layout.
// MAIN.SplitScreenLayout Side by side gives each player the left or right half at full height instead, which on a
// 32:9 screen is a 16:9 view per player.
// Each player's HUD, prompts, subtitles and pause menu are drawn into that player's view in the 2D layout's units
// (1280x720 for the whole screen), within the box the game's split layout gives them: 20:9, as high as a top or
// bottom half. With side by side, that box is scaled to fit the player's half and centred in it.

namespace
{
    GameRef<uintptr_t> Screen;      // +0x50 width, +0x54 height
    GameRef<uintptr_t> Main;        // +0x1042C game state, its +0x558 the game mode
    GameRef<uintptr_t> Cameras;     // +0x3078 split HUD box width (of 1280), +0x307C its left edge (in 1280)

    std::atomic<bool> Pending{ false };

    template<class T>
    T& Field(uintptr_t object, uint32_t offset)
    {
        return *reinterpret_cast<T*>(object + offset);
    }

    bool SideBySideSetting()
    {
        return FixSettings().Int("MAIN.SplitScreenLayout") == 1;
    }

    bool SplitActive()
    {
        if (!Main)
            return false;
        const auto state = Field<uintptr_t>(Main, 0x1042C);
        return state && Field<int32_t>(state, 0x558) == 1;
    }

    void(__fastcall* SetViewportRect)(uintptr_t cameras, void*, int32_t player, int32_t* rect) = nullptr;

    // The rectangles of the two split views (x0, y0, x1, y1 in pixels); the game adds the screen origin at +0x3054
    void __fastcall SetSplitViewport(uintptr_t cameras, void* edx, int32_t player, int32_t* rect)
    {
        if (SideBySideSetting())
        {
            const int32_t width = Field<int32_t>(Screen, 0x50), height = Field<int32_t>(Screen, 0x54);
            const int32_t x = Field<int32_t>(cameras, 0x3054), y = Field<int32_t>(cameras, 0x3058);
            rect[0] = x + (player == 0 ? 0 : width / 2);
            rect[1] = y;
            rect[2] = x + (player == 0 ? width / 2 : width);
            rect[3] = y + height;
        }
        SetViewportRect(cameras, edx, player, rect);
    }

    // The split layout set up again (as after a change of the game's split mode option)
    void(__fastcall* Refresh)(uintptr_t cameras, void*) = nullptr;

    // The camera manager's update, once per frame on the game thread
    SafetyHookInline UpdateHook;
    void __fastcall Update(uintptr_t cameras, void*)
    {
        if (Pending.load())
        {
            if (SplitActive())
                Refresh(cameras, nullptr);
            Pending = false;
        }
        UpdateHook.unsafe_thiscall(cameras);
    }

    // The 2D transform of the screen's 2D (slot 31) in the current view: pixel x = x * scaleX + offsetX * width / 2,
    // pixel y = y * scaleY - offsetY * height / 2 (xmm3, xmm0, xmm4, xmm2). A view narrower than the screen in split
    // screen is a player's half.
    // A player's view and the HUD box in it, from the last split HUD drawn: the box's width in pixels and pixels per
    // layout unit
    std::atomic<float> ViewWidth{ 0.0f }, ViewHeight{ 0.0f }, BoxPixels{ 0.0f }, BoxScale{ 0.0f };

    void FitSplitHud(SafetyHookContext& regs)
    {
        const auto slot = *reinterpret_cast<int32_t*>(regs.ebp + 0x0C);
        if (slot != 31 || !SplitActive())
            return;

        const auto device = *reinterpret_cast<uintptr_t*>(regs.esi + 8);
        const int32_t viewW = Field<int32_t>(device, 0x208) - Field<int32_t>(device, 0x200);
        const int32_t viewH = Field<int32_t>(device, 0x20C) - Field<int32_t>(device, 0x204);
        const float boxX = Field<float>(Cameras, 0x307C);
        const float boxW = Field<float>(Cameras, 0x3078) * 1280.0f;
        const float boxH = 360.0f;
        if (viewW <= 0 || viewH <= 0 || boxW <= 0.0f)
            return;

        float scale = regs.xmm3.f32[0];
        if (SideBySideSetting() && viewW < Field<int32_t>(Screen, 0x50))
        {
            scale = std::min(float(viewW) / boxW, float(viewH) / boxH);
            const float left = (float(viewW) - boxW * scale) / 2.0f - boxX * scale;
            const float top = (float(viewH) - boxH * scale) / 2.0f;

            regs.xmm3.f32[0] = scale;
            regs.xmm0.f32[0] = scale;
            regs.xmm4.f32[0] = 2.0f * left / float(viewW);
            regs.xmm2.f32[0] = -2.0f * top / float(viewH);
        }
        ViewWidth = float(viewW);
        ViewHeight = float(viewH);
        BoxPixels = boxW * scale;
        BoxScale = scale;
    }
}

export namespace SplitScreen
{
    bool IsActive()
    {
        return Main.is_initialized() && SplitActive();
    }

    bool IsSideBySide()
    {
        return IsActive() && SideBySideSetting();
    }

    // The y of the pause menu's title in the 2D layout: in split screen the game places it in the player's box
    int PauseMenuTop()
    {
        if (!IsActive())
            return 180;
        const auto top = int(float(int(Field<float>(Cameras, 0x3070) * 144.0f)) + Field<float>(Cameras, 0x3080));
        return int16_t(top);
    }

    // How far the HUD elements at the sides of a player's HUD box move outwards (layout units) so that they sit at the
    // edges of the box of the given aspect in the player's view (0 = the whole view)
    float HudShift(float aspect)
    {
        const float scale = BoxScale.load();
        if (!IsActive() || !(scale > 0.0f))
            return 0.0f;
        float target = ViewWidth.load();
        if (aspect > 0.0f)
            target = std::min(target, ViewHeight.load() * aspect);
        return (target - BoxPixels.load()) / (2.0f * scale);
    }

    void Init()
    {
        auto screen = hook::pattern("A1 ? ? ? ? 8B 70 50 8B 78 54 A1 ? ? ? ? 8B 88 2C 04 01 00 E8");
        Screen.SetAddress(*screen.get_first<uintptr_t*>(1));
        Main.SetAddress(*screen.get_first<uintptr_t*>(12));

        auto top = hook::pattern("E8 ? ? ? ? 8B 4C 24 18 8B 6C 24 10 8D 04 0F 8B 0D");
        auto bottom = hook::pattern("E8 ? ? ? ? 5F C7 86 9C 2F 00 00 01 00 00 00 C7 86 A0 2F 00 00 02");
        Cameras.SetAddress(*top.get_first<uintptr_t*>(18));
        SetViewportRect = (decltype(SetViewportRect))injector::GetBranchDestination(top.get_first()).as_int();
        injector::MakeCALL(top.get_first(), SetSplitViewport, true);
        injector::MakeCALL(bottom.get_first(), SetSplitViewport, true);

        auto refresh = hook::pattern("83 EC 10 33 C0 89 04 24 89 44 24 04 F3 0F 7E 04 24 66 0F D6 81 54 30 00 00");
        Refresh = refresh.get_first<void __fastcall(uintptr_t, void*)>();
        auto update = hook::pattern("56 8B F1 E8 ? ? ? ? 8B 86 04 30 00 00 85 C0 74 1A");
        UpdateHook = safetyhook::create_inline(update.get_first(), Update);

        auto transform = hook::pattern("8B 46 08 8B 88 08 02 00 00 2B 88 00 02 00 00 8B 90 0C 02 00 00");
        static SafetyHookMid TransformHook = safetyhook::create_mid(transform.get_first(), FitSplitHud);

        FixSettings().OnChange("MAIN.SplitScreenLayout", [] { Pending = true; });
    }
}
