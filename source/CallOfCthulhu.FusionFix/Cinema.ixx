module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

export module Cinema;


// Cinematic bars. The game frames its in-engine cutscenes for 4:3 with bars over the top and bottom quarter
// (ScreenBordersSize), so the director's picture is 4:3 / (1 - size) = 16:9. MAIN.CutsceneBorders keeps that picture
// at any screen shape: the view inside it is the director's and bars cover what lies outside (Letterbox above and
// below, Pillarbox at the sides, Both); Off removes the bars and shows the gameplay view instead.
export namespace CoCCinema
{
    inline float width = 640.0f, height = 480.0f, aspect = 4.0f / 3.0f;
    inline float visible = 0.0f;           // how far the bars are in, 0 to 1, as the game fades them
    inline bool drawn = false;             // the effect ran since the last frame
    inline float nativeSize = -1.0f;       // the game's ScreenBordersSize
    inline const int* mode = nullptr;
    inline uint8_t* (__cdecl* gameSettings)() = nullptr;
    inline void(__fastcall* setTransform)(void* renderer, void*, int state, const float* matrix) = nullptr;
    inline void(__fastcall* draw)(void* renderer, void*, int type, int base, int count, int start, int primitives) = nullptr;
    inline SafetyHookInline effectHook, projectionHook;

    inline int Mode()
    {
        return mode ? std::clamp(*mode, 0, 3) : 3;
    }

    inline float Band()
    {
        return (4.0f / 3.0f) / std::max(0.05f, 1.0f - std::clamp(nativeSize, 0.0f, 0.9f));
    }

    // The director's picture fitted to the screen: its width in pixels
    inline float Fit()
    {
        return std::min(width, height * Band());
    }

    // The vertical field of view's tangent while the bars are fully in: the director's picture keeps its 4:3
    // horizontal view across its width
    inline float Scale()
    {
        if (Mode() == 0 || nativeSize < 0.0f)
            return 1.0f;
        return (4.0f / 3.0f) * height / Fit();
    }

    inline float Visibility(const uint8_t* effect)
    {
        float fade = 0.0f;
        std::memcpy(&fade, effect + 12, sizeof(fade));
        const bool shown = effect[8] != 0;
        if (fade > 0.0f)
            return std::clamp(shown ? fade : 1.0f - fade, 0.0f, 1.0f);
        return shown ? 1.0f : 0.0f;
    }

    inline void __fastcall Effect(uint8_t* effect, void*, void* renderer)
    {
        float* size = gameSettings ? reinterpret_cast<float*>(gameSettings() + 0x644) : nullptr;
        if (size && nativeSize < 0.0f)
            nativeSize = *size;
        visible = Visibility(effect);
        drawn = true;
        const int current = Mode();
        const float fit = Fit();
        const float pictureHeight = (current & 1) ? fit / Band() : height;
        const float saved = size ? *size : 0.0f;
        if (size)
            *size = current == 0 ? 0.0f : std::clamp(1.0f - pictureHeight / height, 0.0f, 0.9f);
        effectHook.unsafe_thiscall<void>(effect, renderer);
        if (size)
            *size = saved;

        // Side bars: the game's bar quad, moved beside the picture, with the alpha the game set for its own bars
        if (current & 2 && visible > 0.0f && fit < width - 0.5f && setTransform && draw && renderer)
        {
            const float edge = fit / width * 0.5f;
            for (float side : { -1.0f, 1.0f })
            {
                const float matrix[16] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                    side * (1.0f + edge), 0.0f, 0.0f, 1.0f };
                setTransform(renderer, nullptr, 256, matrix);
                draw(renderer, nullptr, 4, 0, 4, 0, 2);
            }
        }
    }

    // Cameras that render the screen (built with the screen's aspect) get the cutscene framing; the stored field of
    // view stays the game's, which other cameras copy
    inline int __fastcall Projection(uint8_t* camera, void*, float fov, float aspect)
    {
        const float scale = 1.0f + (Scale() - 1.0f) * visible;
        if (aspect != CoCCinema::aspect || std::abs(scale - 1.0f) < 0.0001f || !std::isfinite(fov) || fov <= 0.0f || fov >= 179.0f)
            return projectionHook.unsafe_thiscall<int>(camera, fov, aspect);
        constexpr float radians = 3.14159265f / 180.0f;
        const float framed = 2.0f * std::atan(std::tan(fov * radians * 0.5f) * scale) / radians;
        const int result = projectionHook.unsafe_thiscall<int>(camera, framed, aspect);
        std::memcpy(camera + 72, &fov, sizeof(fov));
        return result;
    }

    // Once per presented frame: a cutscene effect that is gone leaves no framing behind
    inline void Frame()
    {
        if (!std::exchange(drawn, false))
            visible = 0.0f;
    }

    inline void Init(const int* borders)
    {
        mode = borders;
        auto effect = hook::pattern("0F 57 C0 83 EC 48 57 8B F9 0F 2F 47 0C 72 0A 80 7F 08 00 0F 84 ? ? ? ? E8");
        auto bars = hook::pattern("E8 ? ? ? ? 6A 02 55 6A 04 55 6A 04 8B CE E8 ? ? ? ? 0F 57 C0 F3 0F 5C 44 24 4C");
        auto projection = hook::pattern("F3 0F 10 44 24 04 F3 0F 10 4C 24 08 83 EC 0C 56 8B F1 8B 86 64 01 00 00");
        gameSettings = reinterpret_cast<decltype(gameSettings)>(injector::GetBranchDestination(effect.get_first(25)).as_int());
        setTransform = reinterpret_cast<decltype(setTransform)>(injector::GetBranchDestination(bars.get_first(0)).as_int());
        draw = reinterpret_cast<decltype(draw)>(injector::GetBranchDestination(bars.get_first(15)).as_int());
        effectHook = safetyhook::create_inline(effect.get_first(), Effect);
        projectionHook = safetyhook::create_inline(projection.get_first(), Projection);
        if (!effectHook || !projectionHook)
        {
            effectHook.reset();
            projectionHook.reset();
            OutputDebugStringA("Call of Cthulhu: cutscene border hooks failed; native bars retained.\n");
        }
    }
}
