module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <cmath>

export module Movie;

import Frontend;



export namespace CoCMovie
{
    struct Rectangle
    {
        int left = 0, top = 0, width = 0, height = 0;
    };

    inline bool Frame(Rectangle& rectangle, int width, int height, int movieWidth, int movieHeight, int mode)
    {
        if (width <= 0 || height <= 0 || width > 16384 || height > 16384 ||
            movieWidth <= 0 || movieHeight <= 0 || movieWidth > 16384 || movieHeight > 16384 || mode < 0 || mode > 1)
            return false;
        // The whole movie at its own aspect: inside the centred 4:3 frame (Original) or the screen (Fill)
        const double areaWidth = mode == 0 ? std::min(double(width), height * 4.0 / 3.0) : double(width);
        const double areaHeight = mode == 0 ? std::min(double(height), width * 3.0 / 4.0) : double(height);
        double drawWidth = areaWidth;
        double drawHeight = areaHeight;
        const double aspect = double(movieWidth) / movieHeight;
        if (areaWidth / areaHeight > aspect)
            drawWidth = areaHeight * aspect;
        else
            drawHeight = areaWidth / aspect;
        if (drawWidth < 1.0 || drawHeight < 1.0 || drawWidth > 1048576.0 || drawHeight > 1048576.0)
            return false;
        rectangle.width = int(std::lround(drawWidth));
        rectangle.height = int(std::lround(drawHeight));
        rectangle.left = (width - rectangle.width) / 2;
        rectangle.top = (height - rectangle.height) / 2;
        return true;
    }

    void Install()
    {
        // The decoder allocates this texture at the movie's native size. Read
        // it at the full-screen movie call site, which excludes the cursor blit.
        auto movieDraw = hook::pattern("6A 01 6A 00 51 52 50 6A 00 6A 00 56 E8 ? ? ? ? 83 C4 20 C7 05 ? ? ? ? 00 00 00 00 5E 83 C4 18 C3");
        static SafetyHookMid MovieDrawHook;
        MovieDrawHook = safetyhook::create_mid(movieDraw.get_first(12), [](SafetyHookContext& regs)
        {
            auto args = reinterpret_cast<uint32_t*>(regs.esp);
            auto texture = *reinterpret_cast<IDirect3DTexture9**>(args[5]);
            D3DSURFACE_DESC movie{};
            const int width = int(args[3]), height = int(args[4]);
            CoCMovie::Rectangle rectangle;
            if (FAILED(texture->GetLevelDesc(0, &movie)) ||
                !CoCMovie::Frame(rectangle, width, height, int(movie.Width), int(movie.Height),
                    std::clamp(FixSettings().Int("MAIN.FMVWidescreenMode"), 0, 1)))
            {
                CthulhuMenu::Log("CoC video: frame metadata or viewport rejected; native playback retained");
                return;
            }
            if (rectangle.width < width || rectangle.height < height)
            {
                auto renderer = reinterpret_cast<uint8_t*>(args[0]);
                auto device = renderer ? *reinterpret_cast<IDirect3DDevice9**>(renderer + 104) : nullptr;
                if (!device || FAILED(device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0)))
                {
                    CthulhuMenu::Log("CoC video: black-bar clear failed; native playback retained");
                    return;
                }
            }
            args[1] = rectangle.left;
            args[2] = rectangle.top;
            args[3] = rectangle.width;
            args[4] = rectangle.height;
        });
    }
}
