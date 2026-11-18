module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/Console/Viewport.hpp"
#include <bit>

export module Hud;

import ComVars;

export namespace THPS4Hud
{
    inline std::array<SafetyHookMid, 4> ScaleHooks;
    inline SafetyHookInline TextHook, SpriteHook;
    inline float Width = 640.0f, Height = 480.0f, FrameWidth = 640.0f, Scale = 1.0f;
    inline const std::string* Constraint = nullptr;
    inline bool FullWidth = false;

    struct Placement
    {
        int anchor = 0;
        int player = -1;
        bool Active() const { return anchor != 0 || player >= 0; }
    };
    inline Placement Current;

    constexpr uint32_t Checksum(const char* name)
    {
        uint32_t value = 0xFFFFFFFF;
        for (; *name; ++name)
        {
            value ^= static_cast<uint8_t>(*name);
            for (int bit = 0; bit < 8; ++bit)
                value = (value >> 1) ^ ((value & 1) ? 0xEDB88320 : 0);
        }
        return value;
    }

    inline Placement Classify(uintptr_t element)
    {
        if (!CameraLayout.is_initialized())
            return {};
        Placement result;
        for (int depth = 0; element && depth < 32; ++depth)
        {
            const auto id = *reinterpret_cast<uint32_t*>(element + 20);
            if (id == Checksum("player1_panel_container"))
                result.player = 0;
            else if (id == Checksum("player2_panel_container"))
                result.player = 1;
            if (!result.anchor)
            {
                switch (id)
                {
                    case Checksum("the_score"):
                    case Checksum("the_score_sprite"):
                    case Checksum("the_special_bar_special"):
                    case Checksum("the_special_bar_sprite"):
                    case Checksum("the_boardstance_sprite"):
                    case Checksum("minigame_timer"):
                        result.anchor = -1;
                        break;
                    case Checksum("current_goal"):
                    case Checksum("cash_goal_sprite"):
                    case Checksum("cash_text"):
                    case Checksum("goal_points_text"):
                        result.anchor = 1;
                        break;
                }
            }
            if (*reinterpret_cast<uint32_t*>(element + 8) & 0x80000u)
                return {};
            element = *reinterpret_cast<uintptr_t*>(element + 64);
        }
        // Split screen: only the players' own panels follow their half
        if (CameraLayout != 0 && result.player < 0)
            return {};
        if (CameraLayout == 0)
            result.player = -1;
        return result;
    }

    // Screen pixels to move the current element by
    inline float Offset()
    {
        const float aspect = Constraint ? console::parse_hud_aspect(Constraint->c_str()) : 0.0f;
        const float anchor = static_cast<float>(Current.anchor);
        if (Current.player < 0 || CameraLayout != 1)
        {
            const float boxWidth = aspect > 0.0f ? std::min(Width, Height * aspect) : Width;
            return anchor * (boxWidth - FrameWidth) * 0.5f;
        }
        // Side by side: each player's half of the 4:3 frame goes to the middle of their half of the screen, and its
        // edges to that half's edges (or its HUD box)
        const float halfBox = aspect > 0.0f ? std::min(Width * 0.5f, Height * aspect) : Width * 0.5f;
        const float side = Current.player == 0 ? 0.25f : 0.75f;
        const float frameLeft = (Width - FrameWidth) * 0.5f;
        const float target = Width * side + anchor * halfBox * 0.5f;
        const float native = frameLeft + FrameWidth * side + anchor * FrameWidth * 0.25f;
        return target - native;
    }

    inline float Position(float x)
    {
        // Native X conversion includes the original 32-pixel canvas margins.
        return x * Scale + Offset() * (704.0f / Width);
    }

    inline void __fastcall DrawText(uintptr_t element, void*)
    {
        const auto previous = std::exchange(Current, Classify(element));
        TextHook.thiscall<void>(element);
        Current = previous;
    }

    inline int __fastcall DrawSprite(uintptr_t element, void*)
    {
        const auto previous = std::exchange(Current, Classify(element));
        // The bar behind the button help is wider than the 4:3 frame on purpose: it keeps spanning the whole screen
        const auto parent = *reinterpret_cast<uintptr_t*>(element + 64);
        const auto previousFull = std::exchange(FullWidth, parent && *reinterpret_cast<uint32_t*>(parent + 20) == Checksum("helper_text_anchor"));
        // Native sprites cache their transformed position until dirty. The HUD
        // box can change live, so refresh classified panels on every draw.
        auto* flags = reinterpret_cast<uint32_t*>(element + 8);
        const auto savedFlags = *flags;
        if (Current.Active()) *flags |= 0x800u;
        const auto result = SpriteHook.thiscall<int>(element);
        if (Current.Active()) *flags = savedFlags;
        FullWidth = previousFull;
        Current = previous;
        return result;
    }

    inline void Install(float width, float height, const std::string* constraint, HMODULE module = GetModuleHandleW(nullptr))
    {
        Width = width;
        Height = height;
        // The 640x480 frame: full height on wider screens, full width (letterboxed) on narrower ones
        FrameWidth = std::min(Width, Height * (4.0f / 3.0f));
        Scale = FrameWidth / Width;
        Constraint = constraint;
        const char* signatures[] =
        {
            "8B 44 24 04 8B 54 24 08 89 41 68 8B 01 89 51 6C FF 50 0C C2 08 00",
            "8B 44 24 04 8B 54 24 08 89 41 70 8B 01 89 51 74 FF 50 0C C2 08 00",
            "8B 44 24 04 8B 54 24 08 89 41 08 8B 01 89 51 0C FF 50 0C C2 08 00",
            "8B 44 24 04 8B 54 24 08 89 41 10 8B 01 89 51 14 FF 50 0C C2 08 00"
        };
        std::array<void*, 4> sites{};
        for (size_t i = 0; i < sites.size(); ++i)
        {
            auto pattern = hook::module_pattern(module, signatures[i]);
            sites[i] = pattern.get_first(8);
        }
        for (size_t i = 0; i < sites.size(); ++i)
        {
            if (i == 0 || i == 2)
                ScaleHooks[i] = safetyhook::create_mid(sites[i], [](SafetyHookContext& regs)
                {
                    regs.eax = std::bit_cast<uint32_t>(Position(std::bit_cast<float>(regs.eax)));
                });
            else
                ScaleHooks[i] = safetyhook::create_mid(sites[i], [](SafetyHookContext& regs)
                {
                    if (!FullWidth)
                        regs.eax = std::bit_cast<uint32_t>(std::bit_cast<float>(regs.eax) * Scale);
                });
        }
        auto text = hook::module_pattern(module, "83 EC 30 53 55 56 8B F1 33 DB 57 39 9E 44 01 00 00");
        auto sprite = hook::module_pattern(module, "83 EC 0C 56 8B F1 C6 44 24 08 00 D9 86 14 01 00 00");
        auto layout = hook::module_pattern(module, "8B F9 E8 ? ? ? ? 48 6A 00 74 ? 48 74 ? 68 ? ? ? ? EB ? 68 ? ? ? ? EB ? 68 ? ? ? ? E8");
        auto getter = injector::GetBranchDestination(layout.get_first(2)).as_int();
        (void)*reinterpret_cast<int**>(getter + 1);
        TextHook = safetyhook::create_inline(text.get_first(), DrawText);
        SpriteHook = safetyhook::create_inline(sprite.get_first(), DrawSprite);
    }
}

export void InstallHud()
{
    hook::pattern pattern("");
    //HUD
    THPS4Hud::Install(Screen.fWidth, Screen.fHeight, &FixSettings().String("MAIN.HudAspectRatioConstraint"));
    pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? 83 E2 10 89 2D ? ? ? ? D9 1D ? ? ? ? DF 6C 24 ? A3 ? ? ? ? 89 0D ? ? ? ? 83 C2 10 D8 0D");
    injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(6), static_cast<int32_t>(Screen.fHudOffsetReal) + 32, true);
    if (Screen.fHudOffsetY > 0.0f)
    {
        // 448 units of 2D height: on a narrower screen they cover the 4:3 height, centred
        injector::WriteMemory<float>(*pattern.get_first<float*>(45), (1.0f / 448.0f) * (Screen.fWidth * (3.0f / 4.0f)) / Screen.fHeight, true);
        OffsetY.SetAddress(*pattern.get_first<int32_t*>(15));
        static auto offsetHook = safetyhook::create_mid(pattern.get_first(19), [](SafetyHookContext&)
        {
            OffsetY = static_cast<int32_t>(Screen.fHudOffsetY);
        });
    }
}
