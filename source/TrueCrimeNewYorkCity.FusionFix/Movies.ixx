module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Movies;

import ComVars;

// The startup movies (publisher and studio logos, the legal notice): skipped with MAIN.SkipIntro; drawn whole at
// their own shape (Fill) or in the 4:3 frame the game showed them in (Original), never stretched.
namespace TCMovie
{
    SafetyHookInline PlayHook;
    using Quad = int(__thiscall*)(void*, float, float, float, float, float, float, float, float, float);
    Quad DrawQuad = nullptr;

    // Black over the whole back buffer: the game clears only its current viewport
    void Clear(float width, float height)
    {
        struct Viewport
        {
            DWORD x, y, width, height;
            float minZ, maxZ;
        };
        void* device = pMovieDevice;
        if (!device)
            return;
        auto table = *reinterpret_cast<void***>(device);
        using GetViewport = HRESULT(__stdcall*)(void*, Viewport*);
        using SetViewport = HRESULT(__stdcall*)(void*, const Viewport*);
        using ClearTarget = HRESULT(__stdcall*)(void*, DWORD, const void*, DWORD, DWORD, float, DWORD);
        Viewport saved{};
        if (FAILED(reinterpret_cast<GetViewport>(table[41])(device, &saved)))
            return;
        const Viewport whole{ 0, 0, DWORD(width), DWORD(height), saved.minZ, saved.maxZ };
        reinterpret_cast<SetViewport>(table[40])(device, &whole);
        reinterpret_cast<ClearTarget>(table[36])(device, 0, nullptr, 1, 0xFF000000, 1.0f, 0);
        reinterpret_cast<SetViewport>(table[40])(device, &saved);
    }

    void __cdecl Play(const char* name, int skippable, char skipOnInput, char clearAfter)
    {
        if (FixSettings().Bool("MAIN.SkipIntro") && name && (!_stricmp(name, "ATVI.bik") ||
            !_stricmp(name, "Luxo.bik") || !_stricmp(name, "Aspyr.bik") || !_stricmp(name, "Method.bik") ||
            !_strnicmp(name, "Legal.", 6)))
            return;
        PlayHook.unsafe_ccall(name, skippable, skipOnInput, clearAfter);
    }

    // The movie frame as a screen quad: (left, top, right, bottom) with texel coordinates of the movie's picture
    int __fastcall Draw(void* batch, void*, float left, float top, float right, float bottom,
        float u0, float v0, float u1, float v1, float depth)
    {
        const float width = right - left, height = bottom - top;
        const float movieWidth = u1 - u0, movieHeight = v1 - v0;
        if (width > 0.0f && height > 0.0f && movieWidth > 0.0f && movieHeight > 0.0f)
        {
            Clear(right, bottom);
            const float aspect = FixSettings().Int("MAIN.FMVWidescreenMode") == 1 ? movieWidth / movieHeight : 4.0f / 3.0f;
            const float fittedWidth = std::min(width, height * aspect);
            const float fittedHeight = std::min(height, width / aspect);
            left += (width - fittedWidth) * 0.5f;
            top += (height - fittedHeight) * 0.5f;
            right = left + fittedWidth;
            bottom = top + fittedHeight;
        }
        return DrawQuad(batch, left, top, right, bottom, u0, v0, u1, v1, depth);
    }

    void Install()
    {
        auto play = hook::pattern("6A FF 64 A1 00 00 00 00 68 ? ? ? ? 50 B8 08 3A 00 00 64 89 25");
        pMovieDevice.SetAddress(*play.get_first<void**>(32));
        PlayHook = safetyhook::create_inline(play.get_first(), Play);
        auto draw = hook::pattern("8D 8C 24 ? ? ? ? E8 ? ? ? ? 6A 02 E8 ? ? ? ? 83 C4 04 8D 8C 24");
        DrawQuad = reinterpret_cast<Quad>(injector::MakeCALL(draw.get_first(7), Draw, true).as_int());
    }
}

class Movies
{
public:
    Movies()
    {
        WFP::onInitEvent() += []()
        {
            TCMovie::Install();
        };
    }
} Movies;
