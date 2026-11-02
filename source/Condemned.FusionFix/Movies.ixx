module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"

export module Movies;


// Movie screens (menu, extras, credits) draw the video into a box the game fits to the screen at 16:9 (its WideScreen
// default), stretching 4:3 videos. The video is fitted to the whole screen at its own aspect instead, never cut off or
// stretched. The square videos (extras, credits) are 16:9 pictures stored in square textures, as the game shows them;
// the others (logos) have square pixels.
export namespace CondemnedMovies
{
    inline SafetyHookMid rectangleHook;

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
        // The whole video inside the screen
        const bool wider = screen > video;
        const float width = wider ? video / screen : 1.0f;
        const float height = wider ? 1.0f : screen / video;
        rectangle[0] = (1.0f - width) * 0.5f;
        rectangle[1] = (1.0f - height) * 0.5f;
        rectangle[2] = 1.0f - rectangle[0];
        rectangle[3] = 1.0f - rectangle[1];
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
