module;

#include "stdafx.h"

export module Movies;

import ComVars;

export void InstallMovies()
{
    //FMVs
    auto pattern = hook::pattern("8B 8D 68 FF FF FF E8 ? ? ? ? 6A 00 6A 00 8B 85 68 FF FF FF");
    struct MoviesHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = *(uint32_t*)(regs.ebp - 0x98);

            auto decoder = *reinterpret_cast<const uint32_t**>(regs.ebp - 0x0C);
            if (!decoder || !decoder[0] || !decoder[1] || Screen.nWidth <= 0 || Screen.nHeight <= 0)
                return;
            const auto videoAspect = static_cast<float>(decoder[0]) / static_cast<float>(decoder[1]);
            const auto screenAspect = Screen.fAspectRatio;
            // Both modes show the whole movie at its own aspect
            float width = 1.0f, height = 1.0f;
            if (screenAspect > videoAspect)
                width = videoAspect / screenAspect;
            else
                height = screenAspect / videoAspect;
            auto scale = *reinterpret_cast<float**>(regs.esp + 0x10);
            auto position = *reinterpret_cast<float**>(regs.esp + 0x0C);
            scale[0] = width;
            scale[1] = height;
            position[0] = Screen.fWidth * (1.0f - width) * 0.5f;
            position[1] = Screen.fHeight * (1.0f - height) * 0.5f;
        }
    }; injector::MakeInline<MoviesHook>(pattern.get_first(0), pattern.get_first(6)); //0x4891EF

}
