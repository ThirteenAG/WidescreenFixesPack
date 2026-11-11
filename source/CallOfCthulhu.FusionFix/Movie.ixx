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
        double drawWidth = areaWidth;
        double drawHeight = height;
        const double aspect = double(movieWidth) / movieHeight;
        if (areaWidth / height > aspect)
            drawWidth = height * aspect;
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

    struct Metadata
    {
        void* texture = nullptr;
        int width = 0, height = 0;
    };

    inline Metadata FromDecoder(const void* decoder, void* texture, int height)
    {
        auto format = decoder ? *static_cast<const unsigned char* const*>(decoder) : nullptr;
        const int width = format ? *reinterpret_cast<const int*>(format + 352) : 0;
        if (!texture || width <= 0 || height <= 0 || width > 16384 || height > 16384)
            return {};
        return { texture, width, height };
    }

    inline Metadata frame;

    inline Metadata Take(void* texture)
    {
        auto published = frame;
        frame = {};
        return texture && published.texture == texture ? published : Metadata{};
    }
}

export namespace CoCMovie
{
    void Install()
    {
        // The decoder publishes one texture per frame. Its metadata is consumed
        // only by the full-viewport movie call, never by the cursor blit below it.
        auto moviePublish = hook::pattern("8B 4E 14 E8 ? ? ? ? 8B 4E 14 E8 ? ? ? ? 57 E8 ? ? ? ? 83 C4 04 5F 32 C0 5E C3");
        auto movieDraw = hook::pattern("6A 01 6A 00 51 52 50 6A 00 6A 00 56 E8 ? ? ? ? 83 C4 20 C7 05 ? ? ? ? 00 00 00 00 5E 83 C4 18 C3");
        static SafetyHookMid MoviePublishHook, MovieDrawHook;
        MoviePublishHook = safetyhook::create_mid(moviePublish.get_first(16), [](SafetyHookContext& regs)
        {
            CoCMovie::frame = {};
            auto player = reinterpret_cast<uint8_t*>(regs.esi);
            auto decoder = *reinterpret_cast<uint8_t**>(player + 20);
            CoCMovie::frame = CoCMovie::FromDecoder(decoder, reinterpret_cast<void*>(regs.edi), int(regs.eax));
        });
        MovieDrawHook = safetyhook::create_mid(movieDraw.get_first(12), [](SafetyHookContext& regs)
        {
            auto args = reinterpret_cast<uint32_t*>(regs.esp);
            const auto metadata = CoCMovie::Take(reinterpret_cast<void*>(args[5]));
            const int width = int(args[3]), height = int(args[4]);
            CoCMovie::Rectangle rectangle;
            if (!metadata.texture || args[1] || args[2] ||
                !CoCMovie::Frame(rectangle, width, height, metadata.width, metadata.height, std::clamp(FixSettings().Int("MAIN.FMVWidescreenMode"), 0, 1)))
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
