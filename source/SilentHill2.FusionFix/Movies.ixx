module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/SilentHill/Display.hpp"


export module Movies;


export namespace SH2Movies
{
    inline float Aspect = 4.0f / 3.0f;
    inline std::function<void()> Changed;
    inline void* (WINAPI* NativeOpen)(const char*, uint32_t) = nullptr;
    inline int(WINAPI* NativeFrame)(void*) = nullptr;

    // The whole movie at its own aspect: Original inside the game's centred 4:3 area, Fill inside the whole screen
    inline std::pair<float, float> Offsets(float width, float height, float aspect, int mode)
    {
        const float areaWidth = mode == 0 ? std::min(width, height * 4.0f / 3.0f) : width;
        const float areaHeight = mode == 0 ? std::min(height, width * 3.0f / 4.0f) : height;
        const float scale = std::min(areaWidth / aspect, areaHeight);
        return { (width - scale * aspect) * 0.5f, (height - scale) * 0.5f };
    }

    inline void Update(void* movie)
    {
        if (!movie)
            return;
        const auto size = static_cast<const uint32_t*>(movie);
        if (size[0] == 0 || size[1] == 0 || size[0] > 16384 || size[1] > 16384)
            return;
        const float aspect = float(size[0]) / float(size[1]);
        if (aspect != Aspect)
        {
            Aspect = aspect;
            if (Changed)
                Changed();
        }
    }

    inline void* WINAPI Open(const char* filename, uint32_t flags)
    {
        auto movie = NativeOpen(filename, flags);
        Update(movie);
        return movie;
    }

    inline int WINAPI Frame(void* movie)
    {
        Update(movie);
        return NativeFrame(movie);
    }

    inline void Init(std::function<void()> changed)
    {
        Changed = std::move(changed);
        auto hooks = IATHook::Replace(GetModuleHandleW(nullptr), "binkw32.dll",
            std::forward_as_tuple("_BinkOpen@8", &Open),
            std::forward_as_tuple("_BinkDoFrame@4", &Frame));
        NativeOpen = reinterpret_cast<decltype(NativeOpen)>(hooks.at("_BinkOpen@8").get());
        NativeFrame = reinterpret_cast<decltype(NativeFrame)>(hooks.at("_BinkDoFrame@4").get());
    }
}
