module;

#include <stdafx.h>

export module Movie;

import Renderer;

// The Bink player draws each frame as a full-screen quad (and the subtitles over it). The Movie layer gives the frame
// its own aspect ratio; black covers the rest of the screen.
namespace
{
    SafetyHookInline shPlayMovieFrame = {};
    void __cdecl PlayMovieFrame()
    {
        Scope2D scope(Layer2D::Movie);
        shPlayMovieFrame.unsafe_ccall();
    }
}

class Movie
{
public:
    Movie()
    {
        WFP::onInitEvent() += []()
        {
            auto pattern = hook::pattern("53 56 55 81 EC E4 01 00 00");
            shPlayMovieFrame = safetyhook::create_inline(pattern.get_first(), PlayMovieFrame);
        };
    }
} Movie;
