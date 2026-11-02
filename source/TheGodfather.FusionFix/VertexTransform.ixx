module;

#include <stdafx.h>
#include <array>
#include <cstddef>
#include <cstring>
#include <limits>
#include <vector>
#include <algorithm>
#include <cmath>

export module VertexTransform;

export namespace GodfatherHud
{
    void FitFrontend(void* vertices, size_t count, float width, float height)
    {
        if (!vertices || !count || !std::isfinite(width) || !std::isfinite(height)
            || width <= 0.0f || height <= 0.0f || width >= height * (4.0f / 3.0f))
            return;
        float left = width, right = 0.0f, top = height, bottom = 0.0f;
        for (size_t i = 0; i < count; ++i)
        {
            const auto point = reinterpret_cast<const float*>(static_cast<const char*>(vertices) + 28 * i);
            if (!std::isfinite(point[0]) || !std::isfinite(point[1]))
                return;
            left = std::min(left, point[0]);
            right = std::max(right, point[0]);
            top = std::min(top, point[1]);
            bottom = std::max(bottom, point[1]);
        }
        // Backgrounds, borders and screen effects retain their full coverage.
        if ((left <= 1.0f && right >= width - 1.0f) || (top <= 1.0f && bottom >= height - 1.0f))
            return;
        const auto scale = width / (height * (4.0f / 3.0f));
        for (size_t i = 0; i < count; ++i)
        {
            const auto point = reinterpret_cast<float*>(static_cast<char*>(vertices) + 28 * i);
            point[0] = (point[0] - width * 0.5f) * scale + width * 0.5f;
            point[1] = (point[1] - height * 0.5f) * scale + height * 0.5f;
        }
    }

    template <typename Draw, typename Transform>
    auto DrawCopy(Draw&& draw, Transform&& transform, int primitive, void* vertices, int count)
    {
        // The native renderer reads these vertices into its own dynamic buffer.
        if (!vertices || count <= 0 || count > (std::numeric_limits<int>::max)() / 28)
            return draw(primitive, vertices, count);
        alignas(float) std::array<std::byte, 28 * 64> local;
        std::vector<std::byte> large;
        const auto size = size_t(count) * 28;
        auto copy = local.data();
        if (size > local.size())
        {
            large.resize(size);
            copy = large.data();
        }
        std::memcpy(copy, vertices, size);
        transform(copy, size_t(count));
        return draw(primitive, copy, count);
    }

    SafetyHookInline drawHook;
    void(*transform)(void*, size_t, void*){};
    void(*postTransform)(void*, size_t, void*){};

    BOOL __cdecl Draw(int primitive, void* vertices, int count)
    {
        const auto caller = _ReturnAddress();
        return DrawCopy([](int type, void* data, int length)
        {
            return drawHook.unsafe_ccall<BOOL>(type, data, length);
        }, [caller](void* data, size_t length)
        {
            transform(data, length, caller);
            if (postTransform)
                postTransform(data, length, caller);
        }, primitive, vertices, count);
    }
}
