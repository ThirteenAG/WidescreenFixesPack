module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include <stdexcept>

export module Movies;

import Window;
import MovieSurfaces;
import MovieRenderer;

namespace GodfatherMovies
{
    export void Init()
    {
        auto factory = hook::pattern("89 3B E8 ? ? ? ? 3B C7");
        dimensions = []() { return std::pair{ GodfatherWindow::width, GodfatherWindow::height }; };
        BindLiveSetting("MAIN.FMVWidescreenMode", mode);
        const auto call = factory.get_first<uint8_t>(2);
        injector::MakeCALL(call, Factory, true);
        const auto render = GodfatherWindow::frame;
        GodfatherWindow::frame = [render](IDirect3DDevice9* device)
        {
            RenderPending(device);
            if (render)
                render(device);
        };
    }
}
