module;

#include <stdafx.h>
#include <cmath>
#include <numbers>
#include <random>

export module WaterCommon;

// What the water renderers need for one draw; matrices are stored as shader constant rows
export struct WaterDraw
{
    float worldViewProj[4][4];
    float world[3][4];
    float camera[4];
    float reflection[4][4];
    float waves[6][4];
    float fog[4];
    float sun[4];
    float sunColor[4];
    float tint[4];
    float params[4];
};

// Where the position, the vertex color and the texture coordinates are in one vertex; the position is at 0
export struct WaterLayout
{
    uint32_t stride = 0;
    int32_t color = -1;
    int32_t uv = -1;

    bool operator==(const WaterLayout&) const = default;
};

// Tileable wave normals, one mip level: a sum of waves whose frequencies fit the tile, so every level is band
// limited instead of averaged. Texels are A8R8G8B8 with the normal stored as (x, z, up).
export std::vector<uint32_t> WaveNormals(uint32_t size, uint32_t level)
{
    struct Wave
    {
        int kx, kz;
        float amplitude, phase;
    };
    static const std::vector<Wave> waves = []()
    {
        std::vector<Wave> list;
        std::mt19937 random(1931);
        std::uniform_real_distribution<float> unit(0.0f, 1.0f);
        const float wind = 0.35f;
        while (list.size() < 64)
        {
            const float angle = wind + (unit(random) - 0.5f) * 2.4f;
            const float frequency = 2.0f + std::pow(unit(random), 1.6f) * 58.0f;
            const int kx = static_cast<int>(std::lround(std::cos(angle) * frequency));
            const int kz = static_cast<int>(std::lround(std::sin(angle) * frequency));
            if (!kx && !kz)
                continue;
            const float length = std::sqrt(static_cast<float>(kx * kx + kz * kz));
            list.push_back({ kx, kz, (0.6f + 0.4f * unit(random)) / std::pow(length, 1.25f), unit(random) * 2.0f * std::numbers::pi_v<float> });
        }
        return list;
    }();

    const uint32_t dimension = std::max(1u, size >> level);
    const float nyquist = dimension * 0.5f;
    std::vector<uint32_t> texels(dimension * dimension);
    for (uint32_t y = 0; y < dimension; ++y)
    {
        for (uint32_t x = 0; x < dimension; ++x)
        {
            const float u = (x + 0.5f) / dimension;
            const float v = (y + 0.5f) / dimension;
            float dx = 0.0f, dz = 0.0f;
            for (const auto& wave : waves)
            {
                const float reach = static_cast<float>(std::max(std::abs(wave.kx), std::abs(wave.kz)));
                const float keep = std::clamp((nyquist - reach) / (nyquist * 0.25f + 1.0f), 0.0f, 1.0f);
                if (keep <= 0.0f)
                    continue;
                const float slope = wave.amplitude * keep * std::cos(2.0f * std::numbers::pi_v<float> * (wave.kx * u + wave.kz * v) + wave.phase);
                dx += slope * wave.kx;
                dz += slope * wave.kz;
            }
            constexpr float strength = 0.11f;
            float nx = -dx * strength, ny = 1.0f, nz = -dz * strength;
            const float length = std::sqrt(nx * nx + ny * ny + nz * nz);
            nx /= length;
            ny /= length;
            nz /= length;
            const auto encode = [](float value)
            {
                return static_cast<uint32_t>(std::clamp(std::lround(value * 127.5f + 127.5f), 0l, 255l));
            };
            texels[y * dimension + x] = 0xFF000000u | (encode(nx) << 16) | (encode(nz) << 8) | encode(ny);
        }
    }
    return texels;
}

export constexpr uint32_t WaveNormalSize = 256;
