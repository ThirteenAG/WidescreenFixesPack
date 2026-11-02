module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <array>
#include <atomic>

export module Video;

namespace VideoGeometry
{
    inline bool Quad(std::array<float, 16>& vertices, int width, int height, int targetWidth, int targetHeight, int mode)
    {
        if (width <= 0 || height <= 0 || targetWidth <= 0 || targetHeight <= 0 ||
            width > 16384 || height > 16384 || targetWidth > 16384 || targetHeight > 16384 || mode < 0 || mode > 1)
            return false;
        const float ratio = (float(width) * targetHeight) / (float(height) * targetWidth);
        const float x = mode == 0 ? (std::min)(1.0f, ratio) : 1.0f;
        const float y = mode == 0 ? (std::min)(1.0f, 1.0f / ratio) : 1.0f;
        const float u = mode == 1 ? (std::min)(1.0f, 1.0f / ratio) : 1.0f;
        const float v = mode == 1 ? (std::min)(1.0f, ratio) : 1.0f;
        vertices = {
            -x, y, 0.5f - 0.5f * u, 0.5f - 0.5f * v,
            -x, -y, 0.5f - 0.5f * u, 0.5f + 0.5f * v,
            x, y, 0.5f + 0.5f * u, 0.5f - 0.5f * v,
            x, -y, 0.5f + 0.5f * u, 0.5f + 0.5f * v
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
                if (!texture || !Read<void*>(movie, 88) || Read<int>(movie, 72) != 4) return;
                const auto packed = Read<uint32_t>(texture, 32);
                std::array<float, 16> geometry;
                if (!VideoGeometry::Quad(geometry, Read<int>(movie, 104), Read<int>(movie, 108),
                    packed & 65535, (packed >> 16) & 16383, videoMode.load())) return;
                auto output = reinterpret_cast<float*>(regs.edi);
                for (size_t i = 0; i < geometry.size(); ++i)
                    injector::WriteMemory(output + i, geometry[i], true);
            });
            if (!vertices)
            {
                FixSettings().Availability("MAIN.FMVWidescreenMode", [](std::string_view) { return false; });
                OutputDebugStringA("ResidentEvil6.FusionFix: movie vertex hook could not be installed.\n");
            }
        };
    }
} Video;
