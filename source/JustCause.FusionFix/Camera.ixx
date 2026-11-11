module;
#include <stdafx.h>
#include <atomic>
#include <cmath>
#include "FixSettings.hpp"

export module Camera;


// Field of view factor and cutscene borders
export namespace JCCamera
{
    inline SafetyHookInline projectionHook;
    inline std::atomic<float> factor{ 1.0f };
    inline std::atomic<int> borders{ 3 };
    inline std::atomic<bool> cutscene{ false };
    inline std::atomic<uint32_t> barFrames{ 0 };
    inline uint32_t frame = 0;
    inline bool ready = false;

    inline void Refresh()
    {
        factor.store(std::clamp(FixSettings().Float("MAIN.FOVFactor"), 0.5f, 2.0f));
        borders.store(std::clamp(FixSettings().Int("MAIN.CutsceneBorders"), 0, 3));
    }

    inline float Scaled(float vertical, float aspect, float multiplier)
    {
        constexpr float limit = 150.0f * 3.14159265f / 180.0f;
        if (!(vertical > 0.0f) || !(aspect > 0.0f) || multiplier == 1.0f)
            return vertical;
        auto half = std::atan(std::tan(vertical * 0.5f) * multiplier);
        const auto widest = std::atan(std::tan(limit * 0.5f) / aspect);
        if (multiplier > 1.0f)
            half = std::max(vertical * 0.5f, std::min(half, widest));
        return half * 2.0f;
    }

    // Cinematic bars are plain sprites spanning the whole width at the top or bottom edge
    inline bool Bar(const uint8_t* sprite)
    {
        const auto x = *reinterpret_cast<const float*>(sprite + 12);
        const auto y = *reinterpret_cast<const float*>(sprite + 16);
        const auto width = *reinterpret_cast<const float*>(sprite + 28);
        const auto height = *reinterpret_cast<const float*>(sprite + 32);
        // Hidden bars wait just outside the screen
        const auto visible = std::min(y + height, 1.0f) - std::max(y, 0.0f);
        return x <= 0.001f && x + width >= 0.999f && height <= 0.25f && visible > 0.01f &&
            (y <= 0.001f || y + height >= 0.999f);
    }

    // Called for every plain sprite draw; false skips it
    inline bool Sprite(void* self)
    {
        if (!Bar(static_cast<const uint8_t*>(self)))
            return true;
        barFrames.store(frame);
        const auto mode = borders.load();
        return mode == 1 || mode == 3;
    }

    // Once per presented frame
    inline void Frame()
    {
        ++frame;
        cutscene.store(frame - barFrames.load() <= 2);
    }

    inline bool Pillarbox()
    {
        const auto mode = borders.load();
        return cutscene.load() && (mode == 2 || mode == 3);
    }

    inline void __fastcall Projection(uint8_t* camera, void*)
    {
        auto fov = reinterpret_cast<float*>(camera + 908);
        const auto width = *reinterpret_cast<int*>(camera + 932);
        const auto height = *reinterpret_cast<int*>(camera + 936);
        const auto multiplier = cutscene.load() ? 1.0f : factor.load();
        if (camera[16] || multiplier == 1.0f || height <= 0)
        {
            projectionHook.thiscall<void>(camera);
            return;
        }
        const auto original = *fov;
        *fov = Scaled(original, static_cast<float>(width) / height, multiplier);
        projectionHook.thiscall<void>(camera);
        *fov = original;
    }

    inline void Install()
    {
        Refresh();
        FixSettings().OnChange("MAIN.FOVFactor", Refresh);
        FixSettings().OnChange("MAIN.CutsceneBorders", Refresh);
        auto projection = hook::pattern("83 EC 08 56 8B F1 8B 86 A8 03 00 00 85 C0 C7 44 24 04 00 00 80 3F 89 44 24 08");
        projectionHook = safetyhook::create_inline(projection.get_first(), Projection);
        ready = true;
    }
}
