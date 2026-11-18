module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"

export module Movies;


// Movie screens (menu, extras, credits) draw the video into a box the game fits to the screen at 16:9 (its WideScreen
// default), stretching 4:3 videos. The video keeps its own aspect, never cut off or stretched: inside that 16:9 box
// (MAIN.FMVWidescreenMode Original) or fitted to the whole screen (Fill). The square videos (extras, credits) are 16:9
// pictures stored in square textures, as the game shows them; the others (logos) have square pixels.
export namespace CondemnedMovies
{
    inline SafetyHookMid rectangleHook;

    // The largest box of the given aspect inside the screen, centred, as parts of the screen
    inline void Inside(float* rectangle, float screen, float aspect)
    {
        const bool wider = screen > aspect;
        const float width = wider ? aspect / screen : 1.0f;
        const float height = wider ? 1.0f : screen / aspect;
        rectangle[0] = (1.0f - width) * 0.5f;
        rectangle[1] = (1.0f - height) * 0.5f;
        rectangle[2] = 1.0f - rectangle[0];
        rectangle[3] = 1.0f - rectangle[1];
    }

    inline void Fit(float* rectangle, float screenWidth, float screenHeight, float videoWidth, float videoHeight)
    {
        if (screenWidth <= 0.0f || screenHeight <= 0.0f || videoWidth <= 0.0f || videoHeight <= 0.0f)
        {
            rectangle[0] = rectangle[1] = 0.0f;
            rectangle[2] = rectangle[3] = 1.0f;
            return;
        }
        const float screen = screenWidth / screenHeight;
        const float video = videoWidth == videoHeight ? 16.0f / 9.0f : videoWidth / videoHeight;
        if (FixSettings().Int("MAIN.FMVWidescreenMode") != 0)
        {
            Inside(rectangle, screen, video);
            return;
        }
        // The game's 16:9 box, with the video inside it
        constexpr float box = 16.0f / 9.0f;
        float outer[4]{}, inner[4]{};
        Inside(outer, screen, box);
        Inside(inner, box, video);
        const float width = outer[2] - outer[0], height = outer[3] - outer[1];
        rectangle[0] = outer[0] + inner[0] * width;
        rectangle[1] = outer[1] + inner[1] * height;
        rectangle[2] = outer[0] + inner[2] * width;
        rectangle[3] = outer[1] + inner[3] * height;
    }

    inline void Init(HMODULE gameClient)
    {
        // Right after the movie screen's call for the interface rectangle: the rectangle (left, top, right, bottom as
        // parts of the screen), the screen size and the video size are on the stack.
        auto pattern = hook::pattern(gameClient, "E8 ? ? ? ? D9 44 24 14 D8 4C 24 0C 8D 4C 24 40");
        rectangleHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& context)
        {
            const auto stack = context.esp;
            Fit(reinterpret_cast<float*>(stack + 0x14), *reinterpret_cast<float*>(stack + 0x0C), *reinterpret_cast<float*>(stack + 0x10),
                static_cast<float>(*reinterpret_cast<uint32_t*>(stack + 0x34)), static_cast<float>(*reinterpret_cast<uint32_t*>(stack + 0x2C)));
        });
    }
}
