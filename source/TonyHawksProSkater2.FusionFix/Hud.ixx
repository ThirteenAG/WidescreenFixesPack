module;

#include <stdafx.h>
#include "Shared/Console/Viewport.hpp"

export module Hud;

import ComVars;

export namespace THPS2Hud
{
    inline SafetyHookInline FrameHook, ScoreHook, StanceHook, GoalsHook, GradientHook, RectangleHook;
    inline const std::string* Constraint = nullptr;
    inline float Width = 640.0f, Height = 480.0f, FrameWidth = 640.0f, FrameHeight = 480.0f;
    inline bool Drawing = false;
    inline int Anchor = 0;

    inline float Offset()
    {
        if (!Drawing)
            return 0.0f;
        const float aspect = Constraint ? console::parse_hud_aspect(Constraint->c_str()) : 0.0f;
        const float boxWidth = aspect > 0.0f ? std::min(Width, Height * aspect) : Width;
        return Anchor * (boxWidth - FrameWidth) * 0.5f;
    }

    inline void CorrectRectangle(uint8_t* packet, size_t size, uint8_t command, const std::array<int, 4>& vertices)
    {
        if (!Drawing || !packet || !HudPool.is_initialized() || HudPool.get() != packet + size || (packet[7] & 0xFC) != command)
            return;
        const float scale = FrameWidth / Width;
        const float scaleY = FrameHeight / Height;
        const float offset = Offset();
        for (auto vertex : vertices)
        {
            auto* x = reinterpret_cast<int16_t*>(packet + vertex);
            const float corrected = (x[0] - Width * 0.5f) * scale + Width * 0.5f + offset;
            const float correctedY = (x[1] - Height * 0.5f) * scaleY + Height * 0.5f;
            x[0] = static_cast<int16_t>(std::clamp(std::round(corrected), -32768.0f, 32767.0f));
            x[1] = static_cast<int16_t>(std::clamp(std::round(correctedY), -32768.0f, 32767.0f));
        }
    }

    inline int __cdecl DrawFrame()
    {
        const auto previous = std::exchange(Drawing, HudPlayers.is_initialized() && HudViewMode.is_initialized() && HudViewMode == 0 &&
            (HudPlayers == 1 || (HudPlayers == 2 && HudGameMode.is_initialized() && HudGameMode == 7)));
        const auto anchor = std::exchange(Anchor, 0);
        const auto result = FrameHook.ccall<int>();
        Anchor = anchor;
        Drawing = previous;
        return result;
    }

    inline int __cdecl DrawScore(uintptr_t skater, uint16_t x, uint16_t y)
    {
        // HORSE draws both players' scores together around the screen center.
        const auto previous = std::exchange(Anchor, HudPlayers.is_initialized() && HudGameMode.is_initialized() && HudPlayers == 2 && HudGameMode == 7
            ? 0 : (x < 256 ? -1 : 1));
        const auto result = ScoreHook.ccall<int>(skater, x, y);
        Anchor = previous;
        return result;
    }

    inline int __cdecl DrawStance(uintptr_t skater, uint16_t x, uint16_t y)
    {
        const auto previous = std::exchange(Anchor, x < 256 ? -1 : 1);
        const auto result = StanceHook.ccall<int>(skater, x, y);
        Anchor = previous;
        return result;
    }

    inline int __cdecl DrawGoals()
    {
        const auto previous = std::exchange(Anchor, 1);
        const auto result = GoalsHook.ccall<int>();
        Anchor = previous;
        return result;
    }

    inline int __cdecl DrawGradient(int16_t x, int16_t y, int16_t width, int16_t height,
        char r1, char g1, char b1, char r2, char g2, char b2, int layer)
    {
        auto packet = HudPool.is_initialized() ? HudPool.get() : nullptr;
        const auto result = GradientHook.ccall<int>(x, y, width, height, r1, g1, b1, r2, g2, b2, layer);
        CorrectRectangle(packet, 40, 0x38, { 8, 16, 24, 32 });
        return result;
    }

    inline int __cdecl DrawRectangle(int16_t x, int16_t y, int16_t width, int16_t height, char r, char g, char b, int layer)
    {
        auto packet = HudPool.is_initialized() ? HudPool.get() : nullptr;
        const auto result = RectangleHook.ccall<int>(x, y, width, height, r, g, b, layer);
        CorrectRectangle(packet, 28, 0x28, { 8, 12, 16, 20 });
        return result;
    }

    inline void Install(float width, float height, float frameWidth, float frameHeight, const std::string* constraint, HMODULE module = GetModuleHandleW(nullptr))
    {
        Width = width;
        Height = height;
        FrameWidth = frameWidth;
        FrameHeight = frameHeight;
        Constraint = constraint;
        auto frame = hook::module_pattern(module, "A1 ? ? ? ? 53 55 56 85 C0 57 0F 84 ? ? ? ? 8B 1D ? ? ? ? E8");
        auto score = hook::module_pattern(module, "83 EC 18 55 56 57 E8 ? ? ? ? F6 D8 1B C0 6A 06 40 8B F0 89 74 24 14");
        auto stance = hook::module_pattern(module, "53 56 57 E8 ? ? ? ? 84 C0 0F 85 ? ? ? ? 8B 5C 24 10 8B 83 B8 30 00 00");
        auto goals = hook::module_pattern(module, "56 8B 35 ? ? ? ? 8B 46 20 85 C0 0F 85 ? ? ? ? A1 ? ? ? ? 85");
        auto gradient = hook::module_pattern(module, "8B 0D ? ? ? ? 8B 15 ? ? ? ? 8D 41 28 3B C2 76 03 33 C0 C3 8A 54 24 14");
        auto rectangle = hook::module_pattern(module, "8B 0D ? ? ? ? 8B 15 ? ? ? ? 8D 41 1C 3B C2 76 03 33 C0 C3 A3 ? ? ? ? 8A 44 24");
        auto players = hook::module_pattern(module, "83 3D ? ? ? ? 01 7E ? A1 ? ? ? ? 83 F8 07 B8 00 0C 00 00 75 ? B8 00 10 00 00");
        auto view = hook::module_pattern(module, "8B 0D ? ? ? ? 33 C0 66 8B 83 70 01 00 00 83 C4 08 8D 04 48 83 F8 07");
        HudPool.SetAddress(*gradient.get_first<uint8_t**>(2));
        HudPlayers.SetAddress(*players.get_first<int*>(2));
        HudGameMode.SetAddress(*players.get_first<int*>(10));
        HudViewMode.SetAddress(*view.get_first<int*>(2));
        FrameHook = safetyhook::create_inline(frame.get_first(), DrawFrame);
        ScoreHook = safetyhook::create_inline(score.get_first(), DrawScore);
        StanceHook = safetyhook::create_inline(stance.get_first(), DrawStance);
        GoalsHook = safetyhook::create_inline(goals.get_first(), DrawGoals);
        GradientHook = safetyhook::create_inline(gradient.get_first(), DrawGradient);
        RectangleHook = safetyhook::create_inline(rectangle.get_first(), DrawRectangle);
    }
}
