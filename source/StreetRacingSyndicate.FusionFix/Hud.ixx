module;

#include <stdafx.h>
#include "FixSettings.hpp"

// Every 2D vertex of the game passes the renderer's vertex function, which turns the 640x480 layout into screen pixels
// around the screen centre. The fix keeps that layout at its 4:3 shape (narrower in x on wide screens, lower in y on
// screens narrower than 4:3). The vertices of one element (an image, a string, a solid rectangle) are collected until
// the element ends and then placed as a whole:
// - solid rectangles over the whole layout (fades, dimming) cover the whole screen;
// - in the race HUD, elements in the left or right third of the layout move to the edge of the HUD box.

export module Hud;

import ComVars;

export namespace SRSHud
{
    inline float Width = 640.0f, Height = 480.0f;
    inline float Scale = 1.0f;                          // screen aspect / 4:3
    inline std::atomic<float> Constraint{ 0.0f };       // HUD box aspect, 0 = the whole screen
    inline float PictureWidth = 640.0f;                 // the 3D picture's width (narrower inside cutscene borders)
    inline int Depth = 0;                               // inside the race HUD
    inline bool Driving = false;                        // the race view is the active screen (no menu over it)
    inline bool Raw = false;                            // vertices placed by the fix itself
    inline std::vector<float*> Group;
    inline bool GroupRaw = false;
    inline uint8_t* Renderer = nullptr;                 // the renderer of the last 2D vertex
    inline SafetyHookInline EndHook, TextHook, HudHook, HudExtraHook, RaceHudHook, MovieHook;

    // Distance from the 4:3 frame to the edge of the HUD box, in pixels
    inline float Edge()
    {
        const float frame = Height * (4.0f / 3.0f);
        const float ratio = Constraint.load(std::memory_order_relaxed);
        const float box = std::min(PictureWidth, ratio > 0.0f ? Height * ratio : Width);
        return std::max(0.0f, (box - frame) * 0.5f);
    }

    inline void CloseGroup()
    {
        if (Group.empty())
            return;
        if (std::exchange(GroupRaw, false))
        {
            Group.clear();
            return;
        }
        float left = FLT_MAX, right = -FLT_MAX, top = FLT_MAX, bottom = -FLT_MAX;
        for (auto vertex : Group)
        {
            left = std::min(left, vertex[0]);
            right = std::max(right, vertex[0]);
            top = std::min(top, vertex[1]);
            bottom = std::max(bottom, vertex[1]);
        }
        // The renderer's current material (+0x4C) and texture (+0x50): neither for a solid colour
        const bool solid = !*reinterpret_cast<void**>(Renderer + 0x4C) && !*reinterpret_cast<void**>(Renderer + 0x50);
        const float frameWidth = Width / std::max(Scale, 1.0f);
        const float frameHeight = Height * std::min(Scale, 1.0f);
        if (solid && right - left >= frameWidth - 2.0f && bottom - top >= frameHeight - 2.0f)
        {
            // A solid layer over the whole layout: over the whole screen
            for (auto vertex : Group)
            {
                vertex[0] = vertex[0] < (left + right) * 0.5f ? -Width * 0.5f - 0.5f : Width * 0.5f - 0.5f;
                vertex[1] = vertex[1] < (top + bottom) * 0.5f ? -Height * 0.5f - 0.5f : Height * 0.5f - 0.5f;
            }
        }
        else if (Depth > 0 && !SplitScreen.get())
        {
            const float center = (left + right) * 0.5f;
            const float third = frameWidth / 6.0f;
            const float shift = center < -third ? -Edge() : center > third ? Edge() : 0.0f;
            if (shift != 0.0f)
            {
                for (auto vertex : Group)
                    vertex[0] += shift;
            }
        }
        Group.clear();
    }

    // The renderer's 2D vertex: screen pixels around the centre, x at +0, y at +4
    inline void Vertex(uint8_t* renderer, float* vertex)
    {
        Renderer = renderer;
        GroupRaw |= Raw;
        if (!Raw)
        {
            if (Scale >= 1.0f)
                vertex[0] = (vertex[0] + 0.5f) / Scale - 0.5f;
            else
                vertex[1] = (vertex[1] + 0.5f) * Scale - 0.5f;
        }
        Group.push_back(vertex);
    }

    inline int __fastcall End(void* renderer, void*)
    {
        CloseGroup();
        return EndHook.thiscall<int>(renderer);
    }

    inline void* __fastcall Text(void* font, void*, void* a2, int a3, int* rectangle, int z)
    {
        CloseGroup();
        auto result = TextHook.thiscall<void*>(font, a2, a3, rectangle, z);
        CloseGroup();
        return result;
    }

    inline void __fastcall Hud(void* hud, void*)
    {
        ++Depth;
        HudHook.thiscall<void>(hud);
        CloseGroup();
        --Depth;
    }

    inline void __fastcall HudExtra(void* hud, void*)
    {
        ++Depth;
        HudExtraHook.thiscall<void>(hud);
        CloseGroup();
        --Depth;
    }

    // The race screens' timers, laps and position
    inline char __fastcall RaceHud(void* race, void*, int a1, int a2)
    {
        ++Depth;
        const auto result = RaceHudHook.thiscall<char>(race, a1, a2);
        CloseGroup();
        --Depth;
        return result;
    }

    // Movie frames: Original keeps the game's picture in the 4:3 frame, Fill shows it as large as the screen allows at
    // the aspect the game gives it (the story movies are 16:9 pictures stored anamorphic in 4:3 frames)
    inline char __cdecl Movie(float* rectangle)
    {
        if (std::clamp(FixSettings().Int("MAIN.FMVWidescreenMode"), 0, 1) == 0)
            return MovieHook.ccall<char>(rectangle);
        const float aspect = (rectangle[6] - rectangle[0]) / std::max(1.0f, rectangle[7] - rectangle[1]);
        const float height = std::min(Height, Width / aspect);
        const float width = height * aspect;
        float placed[8];
        std::copy_n(rectangle, 8, placed);
        placed[0] = 320.0f - width / Width * 320.0f;
        placed[6] = 320.0f + width / Width * 320.0f;
        placed[1] = 240.0f - height / Height * 240.0f;
        placed[7] = 240.0f + height / Height * 240.0f;
        Raw = true;
        const auto result = MovieHook.ccall<char>(placed);
        Raw = false;
        return result;
    }

    inline void Init(int width, int height)
    {
        Width = static_cast<float>(width);
        Height = static_cast<float>(height);
        PictureWidth = Width;
        Scale = (Width / Height) / (4.0f / 3.0f);
        Group.reserve(4096);

        const auto update = []()
        {
            const auto ratio = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
            Constraint.store(ratio ? std::clamp(*ratio, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f, std::memory_order_relaxed);
        };
        update();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", update);

        auto vertex = hook::pattern("8B 81 ? ? ? ? D8 40 6C 5E D9 58 6C");
        struct VertexHook
        {
            void operator()(injector::reg_pack& regs)
            {
                Vertex(reinterpret_cast<uint8_t*>(regs.ecx), *reinterpret_cast<float**>(regs.ecx + 0x0A859C));
                regs.eax = *reinterpret_cast<uintptr_t*>(regs.ecx + 0x0C3B40);
            }
        }; injector::MakeInline<VertexHook>(vertex.get_first(0), vertex.get_first(6));

        auto end = hook::pattern("8B 81 40 3B 0C 00 8B 50 0C 85 D2 76 0E 50 E8");
        EndHook = safetyhook::create_inline(end.get_first(), End);
        auto text = hook::pattern("83 EC 44 53 55 33 C0 56 57 8B F9 8D 4C 24 44 89 44 24 1C");
        TextHook = safetyhook::create_inline(text.get_first(), Text);

        auto split = hook::pattern("A0 ? ? ? ? 84 C0 56 8B F1 74 0E E8 ? ? ? ? C6 86 4C 06 00 00 01");
        SplitScreen.SetAddress(*split.get_first<uint8_t*>(1));
        auto hud = hook::pattern("8B 8E ? ? ? ? 85 C9 74 10 E8 ? ? ? ? 8B 8E ? ? ? ? E8");
        HudHook = safetyhook::create_inline(injector::GetBranchDestination(hud.get_first(10)).as_int(), Hud);
        HudExtraHook = safetyhook::create_inline(injector::GetBranchDestination(hud.get_first(21)).as_int(), HudExtra);
        auto raceHud = hook::pattern("83 EC 54 55 56 57 8B F9 89 7C 24 1C E8");
        RaceHudHook = safetyhook::create_inline(raceHud.get_first(), RaceHud);

        auto movie = hook::pattern("A0 ? ? ? ? 81 EC 84 00 00 00 84 C0 0F 85");
        MovieHook = safetyhook::create_inline(movie.get_first(), Movie);
    }
}
