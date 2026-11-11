module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>

export module Video;

namespace VideoGeometry
{
    // picture: the share of the movie height that holds the picture; the rest is black bars in the file
    inline bool Quad(std::array<float, 16>& vertices, int width, int height, int targetWidth, int targetHeight, int mode, float picture)
    {
        if (width <= 0 || height <= 0 || targetWidth <= 0 || targetHeight <= 0 ||
            width > 16384 || height > 16384 || targetWidth > 16384 || targetHeight > 16384 || mode < 0 || mode > 1)
            return false;
        // The whole movie at its own aspect: Original inside the game's centred 16:9 frame, Fill inside the whole screen.
        // Fill fits the picture instead of the frame, so only the file's own black bars leave the screen.
        const float screen = float(targetWidth) / targetHeight;
        const float area = mode == 0 ? 16.0f / 9.0f : screen;
        const float movie = float(width) / height;
        const float shown = mode == 0 ? 1.0f : std::clamp(picture, 0.1f, 1.0f);
        const float tall = (std::min)(1.0f, screen / area) * (std::min)(1.0f, area / (movie / shown)) / shown;
        const float x = tall * movie / screen;
        const float y = (std::min)(1.0f, tall);
        const float v = 0.5f * (std::min)(1.0f, 1.0f / tall);
        vertices = {
            -x, y, 0.0f, 0.5f - v,
            -x, -y, 0.0f, 0.5f + v,
            x, y, 1.0f, 0.5f - v,
            x, -y, 1.0f, 0.5f + v
        };
        return true;
    }
}

namespace
{
    std::atomic<int> videoMode{ 1 };

    template<class T> T Read(const void* object, size_t offset)
    {
        return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(object) + offset);
    }
}

class Video
{
public:
    Video()
    {
        WFP::onInitEvent() += []()
        {
            videoMode.store(FixSettings().Int("MAIN.FMVWidescreenMode"));
            FixSettings().OnChange("MAIN.FMVWidescreenMode", []()
            {
                videoMode.store(FixSettings().Int("MAIN.FMVWidescreenMode"));
            });
            auto quad = hook::pattern("89 57 34 89 47 38 89 4F 3C 8B CE E8 ? ? ? ? 8B CE E8 ? ? ? ? 5F 5E 5D");
            static auto vertices = safetyhook::create_mid(quad.get_first(9), [](SafetyHookContext& regs)
            {
                const auto movie = reinterpret_cast<const void*>(regs.ebp);
                const auto target = Read<void*>(movie, 96);
                const auto texture = target ? Read<void*>(target, 212) : nullptr;
                const auto source = Read<void*>(movie, 88);
                if (!texture || !source || Read<int>(movie, 72) != 4) return;
                // The two story movies are 2.35:1 pictures with black bars in their 1280x720 frames
                const auto path = (*reinterpret_cast<const char* (__thiscall***)(void*)>(source))[19](source);
                const bool bars = path && (std::strstr(path, "e5110") || std::strstr(path, "e5120"));
                const auto packed = Read<uint32_t>(texture, 32);
                std::array<float, 16> geometry;
                if (!VideoGeometry::Quad(geometry, Read<int>(movie, 104), Read<int>(movie, 108),
                    packed & 65535, (packed >> 16) & 16383, videoMode.load(), bars ? 544.0f / 720.0f : 1.0f)) return;
                auto output = reinterpret_cast<float*>(regs.edi);
                for (size_t i = 0; i < geometry.size(); ++i)
                    injector::WriteMemory(output + i, geometry[i], true);
            });
        };
    }
} Video;
