module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <charconv>
#include <unordered_map>
#include <minidx8/d3d8.h>

// The gameplay HUD is a set of 2D scenes (game_disp.one) plus numbers drawn with the game's font, all laid out on a
// 4:3 canvas that the fix fits into a centred 4:3 box. MAIN.HudAspectRatioConstraint Auto moves the side elements
// out to the screen edges; a ratio moves them to the sides of a centred box of that shape. Each scene keeps its own
// anchor (taken from its slot in game_disp.one) and is moved in the projection while it draws; the numbers follow the
// side they are on. Split screen keeps the game's own per-player layout.

export module Hud;

import ComVars;

export namespace SonicHud
{
    enum Anchor : int8_t
    {
        Left = -1,
        Center = 0,
        Right = 1,
    };

    inline std::atomic<float> Constraint{ 0.0f };
    inline const float* Width = nullptr;
    inline const float* Height = nullptr;
    inline const float* Scale = nullptr;
    inline const std::atomic<float>* FovFactor = nullptr;
    inline std::atomic<bool>* ProjectionAdjusted = nullptr;

    inline std::unordered_map<uintptr_t, Anchor> Scenes;
    inline bool Loading = false;
    inline bool Drawing = false;
    inline int32_t SceneSlot = -1;
    inline uintptr_t Archive = 0;
    inline uintptr_t Popups[3] = {};

    inline SafetyHookInline LoadHook;
    inline SafetyHookInline DrawHook;
    inline SafetyHookInline RenderHook;
    inline SafetyHookInline TextHook;
    inline SafetyHookMid UnloadHook;
    inline SafetyHookMid SlotHook;
    inline SafetyHookMid SetupHook;

    inline float Number(std::string_view text)
    {
        while (!text.empty() && text.front() == ' ') text.remove_prefix(1);
        while (!text.empty() && text.back() == ' ') text.remove_suffix(1);
        float value = 0.0f;
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
        if (text.empty() || parsed.ec != std::errc() || parsed.ptr != text.data() + text.size() || !std::isfinite(value) || value <= 0.0f)
            return 0.0f;
        return value;
    }

    // "Auto" (or anything unreadable) = 0, the whole screen; "W:H" or a decimal, clamped to 4:3 to 32:9
    inline float ParseAspect(std::string_view text)
    {
        const auto colon = text.find(':');
        float value = Number(colon == std::string_view::npos ? text : text.substr(0, colon));
        if (colon != std::string_view::npos)
        {
            const auto height = Number(text.substr(colon + 1));
            value = height > 0.0f ? value / height : 0.0f;
        }
        return std::isfinite(value) && value > 0.0f ? std::clamp(value, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f;
    }

    // Width of the 4:3 HUD box on screen
    inline float BoxWidth43()
    {
        return *Width * *Scale;
    }

    // How far the side elements move outwards, as a fraction of the 4:3 HUD box
    inline float Shift()
    {
        const float hud = BoxWidth43();
        if (hud <= 0.0f)
            return 0.0f;
        float box = *Width;
        const float ratio = Constraint.load(std::memory_order_relaxed);
        if (ratio > 0.0f)
            box = std::min(box, *Height * ratio);
        return std::max(0.0f, (box - hud) * 0.5f) / hud;
    }

    inline bool SplitScreen()
    {
        return *reinterpret_cast<uint8_t*>(HudGameState.get() + 30) != 0;
    }

    // Slots of game_disp.one for one player: 10 rings and 15 lives on the left; 2 action button, 3 leader frame,
    // 4 team blast gauge, 6/7 team members and 12 formation on the right; the rest (pause, settings, help, key) stays
    // centred
    inline Anchor SlotAnchor(int32_t slot)
    {
        switch (slot)
        {
            case 10:
            case 15:
                return Left;
            case 2:
            case 3:
            case 4:
            case 6:
            case 7:
            case 12:
                return Right;
            default:
                return Center;
        }
    }

    inline int __cdecl Load()
    {
        Scenes.clear();
        Loading = true;
        SceneSlot = -1;
        Archive = 0;
        const int result = LoadHook.ccall<int>();
        Loading = false;
        return result;
    }

    inline int __cdecl Draw()
    {
        auto device = HudDevice.get();
        const float factor = FovFactor->load(std::memory_order_relaxed);
        D3DMATRIX original{};
        const bool restore = device && factor != 1.0f && ProjectionAdjusted->load(std::memory_order_relaxed) &&
            SUCCEEDED(device->GetTransform(D3DTS_PROJECTION, &original));
        if (restore)
        {
            D3DMATRIX hud = original;
            hud._11 *= factor;
            hud._22 *= factor;
            hud._31 *= factor;
            hud._32 *= factor;
            hud._41 *= factor;
            hud._42 *= factor;
            device->SetTransform(D3DTS_PROJECTION, &hud);
        }
        Drawing = true;
        const int result = DrawHook.ccall<int>();
        Drawing = false;
        if (restore)
        {
            device->SetTransform(D3DTS_PROJECTION, &original);
            ProjectionAdjusted->store(true, std::memory_order_relaxed);
        }
        return result;
    }

    // A HUD scene is moved on screen for this draw only, in the projection: its objects and their cached transforms
    // stay as the game keeps them
    inline int __cdecl Render(uintptr_t scene)
    {
        const auto found = Scenes.find(scene);
        const float shift = found != Scenes.end() ? Shift() : 0.0f;
        auto device = HudDevice.get();
        if (shift <= 0.0f || !device)
            return RenderHook.ccall<int>(scene);

        // Clip-space x moves by the same amount at any depth: x += offset * w
        const float offset = float(found->second) * shift * BoxWidth43() * 2.0f / *Width;
        D3DMATRIX original{};
        device->GetTransform(D3DTS_PROJECTION, &original);
        D3DMATRIX moved = original;
        for (int row = 0; row < 4; ++row)
            moved.m[row][0] += offset * moved.m[row][3];
        device->SetTransform(D3DTS_PROJECTION, &moved);
        const int result = RenderHook.ccall<int>(scene);
        device->SetTransform(D3DTS_PROJECTION, &original);
        return result;
    }

    // Numbers of the HUD (score, time, rings, lives...): positions are fractions of the 4:3 canvas
    inline int __cdecl Text(uintptr_t font, uintptr_t text, float size, float* position, uintptr_t colour)
    {
        const auto caller = reinterpret_cast<uintptr_t>(_ReturnAddress());
        const bool popup = caller == Popups[0] || caller == Popups[1] || caller == Popups[2];
        const float shift = Drawing && position && !popup && !SplitScreen() ? Shift() : 0.0f;
        if (shift <= 0.0f)
            return TextHook.ccall<int>(font, text, size, position, colour);

        // The call advances the pen (the score's grey zeros and its digits share one), so only the move is undone
        const float x = position[0];
        const float move = x < 0.45f ? -shift : (x > 0.55f ? shift : 0.0f);
        position[0] += move;
        const int result = TextHook.ccall<int>(font, text, size, position, colour);
        position[0] -= move;
        return result;
    }

    inline void Init(const float* width, const float* height, const float* scale,
        const std::atomic<float>* fovFactor, std::atomic<bool>* projectionAdjusted)
    {
        Width = width;
        Height = height;
        Scale = scale;
        FovFactor = fovFactor;
        ProjectionAdjusted = projectionAdjusted;
        const auto refresh = []() { Constraint = ParseAspect(FixSettings().String("MAIN.HudAspectRatioConstraint")); };
        refresh();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", refresh);

        auto load = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 81 EC A0 00 00 00 53 33 C0 55 56 57 B9 40 00 00 00 BF");
        auto unload = hook::pattern("51 53 57 E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 8B 0D ? ? ? ? 33 DB 51 89 1D");
        auto draw = hook::pattern("A1 ? ? ? ? 8A 48 05 81 EC 88 00 00 00 84 C9 0F 85");
        auto slot = hook::pattern("83 EC 08 56 57 8B F0 33 FF E8 ? ? ? ? 85 C0 74 17 8B 44 24 18 56 50 8B 44 24 1C E8");
        auto setup = hook::pattern("83 EC 28 85 FF 0F 84 ? ? ? ? 53 56 8D 44 24 28 50 8D 4C 24 1C 51 8D 54 24 28 52 E8");
        auto render = hook::pattern("56 8B 74 24 08 8B 46 24 85 C0 74 0F 50 E8 ? ? ? ? 83 C4 04 85 C0 75 02 5E C3 8B C6 5E C3");
        auto text = hook::pattern("8B 54 24 0C 8B 44 24 04 89 54 24 0C 89 44 24 04 FF A0 2C 01 00 00");
        auto device = hook::pattern("A1 ? ? ? ? 56 8B 74 24 08 3B C6 74 27 A1 ? ? ? ? 56 50 8B 08 FF 91 30 01 00 00");
        auto popups = hook::pattern("D8 0D ? ? ? ? 8B 46 04 83 C4 14 51 8D 56 F8 52 51 8B 0D ? ? ? ? D9 1C 24 50 51 E8").count(2);
        auto popup = hook::pattern("68 ? ? ? ? 51 8B 0D ? ? ? ? D9 1C 24 51 52 E8");

        HudGameState.SetAddress(*draw.get_first<uintptr_t*>(1));
        HudDevice.SetAddress(*device.get_first<IDirect3DDevice8**>(15));
        Popups[0] = reinterpret_cast<uintptr_t>(popups.get(0).get<void>(0x1D + 5));
        Popups[1] = reinterpret_cast<uintptr_t>(popups.get(1).get<void>(0x1D + 5));
        Popups[2] = reinterpret_cast<uintptr_t>(popup.get_first(0x11 + 5));

        LoadHook = safetyhook::create_inline(load.get_first(), &Load);
        DrawHook = safetyhook::create_inline(draw.get_first(), &Draw);
        RenderHook = safetyhook::create_inline(render.get_first(), &Render);
        TextHook = safetyhook::create_inline(text.get_first(), &Text);
        UnloadHook = safetyhook::create_mid(unload.get_first(), [](SafetyHookContext&) { Scenes.clear(); });

        // Each HUD scene is loaded by its slot number and then given its camera; game_disp.one is the first archive
        // read, the boss display loaded after it has slots of its own
        SlotHook = safetyhook::create_mid(slot.get_first(), [](SafetyHookContext& context)
        {
            if (!Loading)
                return;
            const auto archive = *reinterpret_cast<uintptr_t*>(context.esp + 4);
            if (!Archive)
                Archive = archive;
            SceneSlot = archive == Archive ? *reinterpret_cast<int32_t*>(context.esp + 8) : -1;
        });
        SetupHook = safetyhook::create_mid(setup.get_first(), [](SafetyHookContext& context)
        {
            if (!Loading)
                return;
            const auto anchor = SceneSlot >= 0 && context.edi && !SplitScreen() ? SlotAnchor(SceneSlot) : Center;
            if (anchor != Center)
                Scenes[context.edi] = anchor;
            SceneSlot = -1;
        });
    }
}
