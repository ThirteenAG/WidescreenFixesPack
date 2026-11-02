module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>
#include <mutex>
#include <unordered_map>

export module Hud;

// The HUD. Every 2D layout is drawn in a 1280x720 space that the renderer fits into a centred 16:9 box on wider
// screens. The HUD layouts (cockpit: life gauges, ammo, partner, action buttons, prompts, saving notice) are spread to
// the box of MAIN.HudAspectRatioConstraint (Auto = the whole screen): each top-level element (one whose parents all
// sit at x 0) on the left third of the layout moves left by the box's extra margin, one on the right third moves
// right, the rest stays centred. The move is made in the element's own position before the game builds its world
// matrix, so its children and the texts the game places from that position move with it. Markers projected from 3D
// positions keep their place. Menus and full screens (inventory, map, loading) stay in the 16:9 box; in split screen the game's own
// layout stays.

namespace
{
    template<class T>
    T& Field(uintptr_t object, uint32_t offset)
    {
        return *reinterpret_cast<T*>(object + offset);
    }

    GameRef<uintptr_t> Screen;      // +0x50 width, +0x54 height
    GameRef<uintptr_t> Renderer;    // +0x4EE0 2D fit mode: 2 = fit height (16:9 box), 3 = fit width
    GameRef<uintptr_t> Main;        // +0x1042C game state, its +0x558 the game mode (1 = split screen)

    std::atomic<float> HudAspect{ 0.0f };   // 0 = Auto

    void ReadHudAspect()
    {
        const auto text = FixSettings().String("MAIN.HudAspectRatioConstraint");
        float value = 0.0f, w = 0.0f, h = 0.0f;
        if (sscanf_s(text.c_str(), "%f:%f", &w, &h) == 2 && w > 0.0f && h > 0.0f)
            value = w / h;
        else if (sscanf_s(text.c_str(), "%f", &w) == 1 && w > 0.0f)
            value = w;
        if (value > 0.0f)
            value = std::clamp(value, 4.0f / 3.0f, 32.0f / 9.0f);
        HudAspect = value;
    }

    // How far side elements move outwards, in layout units (1280 across the 16:9 box)
    float HudShift()
    {
        if (!Screen.is_initialized() || !Screen || !Renderer.is_initialized() || !Renderer)
            return 0.0f;
        if (Main.is_initialized() && Main)
        {
            const auto state = Field<uintptr_t>(Main, 0x1042C);
            if (state && Field<int32_t>(state, 0x558) == 1)
                return 0.0f;
        }
        const int width = Field<int32_t>(Screen, 0x50);
        const int height = Field<int32_t>(Screen, 0x54);
        if (width <= 0 || height <= 0)
            return 0.0f;
        const float screen = float(width) / float(height);
        const float box = Field<int32_t>(Renderer, 0x4EE0) == 2 ? 16.0f / 9.0f : screen;
        float target = HudAspect.load();
        if (target <= 0.0f || target > screen)
            target = screen;
        return (target / box - 1.0f) * 640.0f;
    }

    struct Element
    {
        bool hud = false;
        bool projected = false;
        bool shifted = false;
        float base = 0.0f;
        float written = 0.0f;
    };

    std::mutex Mutex;
    std::unordered_map<uintptr_t, Element> Elements;
    std::unordered_map<uintptr_t, uintptr_t> Containers;   // element list -> its items, registered once

    bool IsHudLayout(std::string_view name)
    {
        for (auto screen : { "cp_load", "cp_map" })
            if (name.find(screen) != name.npos)
                return false;
        // The inventory (slot\cp_bukislot) stays where the game draws it
        return name.find("\\cockpit\\") != name.npos;
    }

    // A layout (uLayoutSpr) updates its elements: the first time, its elements are registered as HUD or not
    SafetyHookInline LayoutMoveHook;
    void __fastcall LayoutMove(uintptr_t layout, void*)
    {
        if (const auto resource = Field<uintptr_t>(layout, 0x18))
        {
            const auto list = Field<uint8_t>(layout, 0x38) ? resource + 0x60 : layout + 0x1C;
            const auto count = Field<uint32_t>(list, 0x0C);
            const auto items = Field<uintptr_t>(list, 0x18);
            std::lock_guard lock(Mutex);
            auto& known = Containers[list];
            if (known != items + count)
            {
                known = items + count;
                const char* path = reinterpret_cast<const char*>(resource + 4);
                const bool hud = IsHudLayout(std::string_view(path, strnlen(path, 64)));
                for (uint32_t i = 0; i < count; ++i)
                {
                    if (const auto e = reinterpret_cast<uintptr_t*>(items)[i])
                    {
                        auto& state = Elements[e];
                        state = {};
                        state.hud = hud;
                    }
                }
            }
        }
        LayoutMoveHook.unsafe_thiscall(layout);
    }

    thread_local int Projecting = 0;

    // An element's position (+0x50) right before its world matrix is built
    void PlaceElement(uintptr_t e)
    {
        auto& x = Field<float>(e, 0x50);
        std::lock_guard lock(Mutex);
        auto it = Elements.find(e);
        if (it == Elements.end() || !it->second.hud)
            return;
        auto& state = it->second;
        if (Projecting)
            state.projected = true;
        if (state.projected)
            return;
        // A position written by the game since our last move is the new base
        if (!state.shifted || x != state.written)
            state.base = x;
        // Only elements whose parents all sit at x 0: their position is a place on the layout
        for (auto parent = Field<uintptr_t>(e, 0x144); parent; parent = Field<uintptr_t>(parent, 0x144))
        {
            if (Field<float>(parent, 0x50) != 0.0f)
            {
                x = state.base;
                state.shifted = false;
                return;
            }
        }
        // A top-level element at x 0 is a group: its children are placed one by one
        float shift = 0.0f;
        if (state.base != 0.0f && state.base < 1280.0f / 3.0f)
            shift = -HudShift();
        else if (state.base > 1280.0f * 2.0f / 3.0f)
            shift = HudShift();
        x = state.base + shift;
        state.written = x;
        state.shifted = true;
    }

    SafetyHookInline ProjectHooks[3];
    uint8_t __fastcall Project0(uintptr_t unit, void*)
    {
        ++Projecting;
        const auto result = ProjectHooks[0].unsafe_thiscall<uint8_t>(unit);
        --Projecting;
        return result;
    }

    template<int N>
    uint8_t __fastcall Project(uintptr_t unit, void*, int player)
    {
        ++Projecting;
        const auto result = ProjectHooks[N].unsafe_thiscall<uint8_t>(unit, player);
        --Projecting;
        return result;
    }
}

export namespace Hud
{
    void Init()
    {
        ReadHudAspect();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", [] { ReadHudAspect(); });

        // The black side panels of the menus on screens wider than 16:9 (uSidePanel) are given in pixels but drawn in
        // the 2D layout's units, so at most screen sizes they cover only part of the sides: they get the whole side
        // margin and height in layout units
        auto panels = hook::pattern("DB 44 24 08 68 00 01 00 00 6A 00 68 ? ? ? ? D9 54 24 14");
        static SafetyHookMid PanelsHook = safetyhook::create_mid(panels.get_first(), [](SafetyHookContext& regs)
        {
            auto& width = *reinterpret_cast<int32_t*>(regs.esp + 8);
            auto& height = *reinterpret_cast<float*>(regs.esp + 0x14);
            if (height <= 0.0f)
                return;
            width = int32_t(std::ceil(float(width) * 720.0f / height)) + 1;
            height = 720.0f;
            regs.esi = uint32_t(width);
        });

        auto screen = hook::pattern("A1 ? ? ? ? 8B 70 50 8B 78 54 A1 ? ? ? ? 8B 88 2C 04 01 00 E8");
        auto renderer = hook::pattern("8B 0D ? ? ? ? 5F 5E 5D C7 81 E0 4E 00 00 02 00 00 00");
        auto move = hook::pattern("56 8B F1 8B 46 14 83 E8 00 74 07 83 E8 01 74 0F EB 50");
        auto animated = hook::pattern("F3 0F 10 46 68 F3 0F 59 05 ? ? ? ? 0F 5A C0 F2 0F 11 44 24 78 E8");
        auto placed = hook::pattern("F3 0F 10 46 50 F3 0F 11 44 24 40 F3 0F 10 46 54 F3 0F 11 44 24 44 F3 0F 10 46 58");

        Screen.SetAddress(*screen.get_first<uintptr_t*>(1));
        Main.SetAddress(*screen.get_first<uintptr_t*>(12));
        Renderer.SetAddress(*renderer.get_first<uintptr_t*>(2));

        // Markers placed from 3D positions on the screen (partner, targets, items)
        const char* projectors[] = {
            "55 8B EC 83 E4 F0 81 EC B4 00 00 00 53 56 57 8B F9 8B 9F A0 01 00 00 8B 0D",
            "55 8B EC 83 E4 F0 81 EC 84 00 00 00 53 56 8B F1 0F B6 86 A8 01 00 00 8B 0D",
            "55 8B EC 83 E4 F0 83 EC 44 53 56 57 8B F9 0F B6 87 A8 01 00 00 8B 0D",
        };
        void* handlers[] = { reinterpret_cast<void*>(Project0), reinterpret_cast<void*>(Project<1>), reinterpret_cast<void*>(Project<2>) };
        for (int i = 0; i < 3; ++i)
        {
            auto pattern = hook::pattern(projectors[i]);
            ProjectHooks[i] = safetyhook::create_inline(pattern.get_first(), handlers[i]);
        }

        LayoutMoveHook = safetyhook::create_inline(move.get_first(), LayoutMove);
        static SafetyHookMid AnimatedHook = safetyhook::create_mid(animated.get_first(), [](SafetyHookContext& regs) { PlaceElement(regs.esi); });
        static SafetyHookMid PlacedHook = safetyhook::create_mid(placed.get_first(), [](SafetyHookContext& regs) { PlaceElement(regs.esi); });
    }
}
