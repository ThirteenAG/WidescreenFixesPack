#pragma once
#include <algorithm>
#include <cstdint>
#include <cstddef>

namespace GodfatherMovies
{
    inline uint32_t Color(int y, int u, int v)
    {
        const int luminance = 298 * (y - 16);
        const int blueDifference = u - 128;
        const int redDifference = v - 128;
        const auto byte = [](int value) { return uint32_t(std::clamp(value >> 8, 0, 255)); };
        const auto red = byte(luminance + 409 * redDifference + 128);
        const auto green = byte(luminance - 100 * blueDifference - 208 * redDifference + 128);
        const auto blue = byte(luminance + 516 * blueDifference + 128);
        return 0xFF000000u | red << 16 | green << 8 | blue;
    }

    inline bool Convert(const uint8_t* input, size_t inputSize, int inputPitch, uint8_t* output, int outputPitch, int width, int height)
    {
        if (!input || !output || width <= 0 || height <= 0 || width > 8192 || height > 8192
            || inputPitch < ((width + 1) / 2) * 4 || outputPitch < width * 4 || inputSize < size_t(inputPitch) * height)
            return false;
        for (int y = 0; y < height; ++y)
        {
            const auto source = input + size_t(inputPitch) * y;
            const auto destination = reinterpret_cast<uint32_t*>(output + size_t(outputPitch) * y);
            for (int x = 0; x < width; x += 2)
            {
                const auto pair = source + size_t(x / 2) * 4;
                destination[x] = Color(pair[0], pair[1], pair[3]);
                if (x + 1 < width)
                    destination[x + 1] = Color(pair[2], pair[1], pair[3]);
            }
        }
        return true;
    }
}
