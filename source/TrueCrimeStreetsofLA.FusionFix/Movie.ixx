module;

#include <stdafx.h>
#include "FixSettings.hpp"


export module Movie;

import Window;

export namespace TrueCrimeMovie
{
    inline SafetyHookInline DrawHook, QuadHook, PlayHook;
    inline thread_local bool Drawing = false;

    inline int __fastcall Draw(uintptr_t context, void*)
    {
        struct Scope
        {
            Scope() { Drawing = true; }
            ~Scope() { Drawing = false; }
        } scope;
        return DrawHook.thiscall<int>(context);
    }

    inline int __fastcall Quad(uintptr_t renderer, void*, float left, float top, float right, float bottom,
        float u0, float v0, float u1, float v1, uint32_t depth)
    {
        if (Drawing && left == 0.0f && top == 0.0f && right > 0 && bottom > 0 && u1 > u0 && v1 > v0)
        {
            const float width = u1 - u0;
            const float height = v1 - v0;
            // Both modes show the whole movie at its own aspect
            const float scale = std::min(right / width, bottom / height);
            const float fittedWidth = width * scale;
            const float fittedHeight = height * scale;
            left = (right - fittedWidth) * 0.5f;
            top = (bottom - fittedHeight) * 0.5f;
            right = left + fittedWidth;
            bottom = top + fittedHeight;
        }
        return QuadHook.thiscall<int>(renderer, left, top, right, bottom, u0, v0, u1, v1, depth);
    }

    // Startup logos: the publisher, developer and studio videos played before the title
    inline void __cdecl Play(const char* name)
    {
        if (FixSettings().Bool("MAIN.SkipIntro") && name &&
            (!_stricmp(name, "intro") || !_stricmp(name, "intro2") || !_stricmp(name, "GrayMatr")))
            return;
        PlayHook.ccall<void>(name);
    }

    inline void Init()
    {
        auto draw = hook::pattern("A1 ? ? ? ? 81 EC 24 04 00 00 85 C0 0F 84 ? ? ? ? 53 56 57");
        if (draw.empty())
            draw = hook::pattern("81 EC 2C 04 00 00 A1 ? ? ? ? 89 84 24 28 04 00 00 A1 ? ? ? ? 85 C0 0F 84 ? ? ? ? 53 56 57");
        auto quad = hook::pattern("55 8B EC 83 E4 F0 83 EC 14 53 56 33 C0 8B F1 8A 66 07");
        auto play = hook::pattern("81 EC 04 02 00 00 53 56 57 E8 ? ? ? ? A1 ? ? ? ? 33 DB 3B C3");
        if (play.empty())
            play = hook::pattern("81 EC 08 02 00 00 A1 ? ? ? ? 53 56 57 89 84 24 10 02 00 00 E8 ? ? ? ? A1 ? ? ? ? 33 DB 3B C3");
        DrawHook = safetyhook::create_inline(draw.get_first(), Draw);
        QuadHook = safetyhook::create_inline(quad.get_first(), Quad);
        PlayHook = safetyhook::create_inline(play.get_first(), Play);
    }
}
