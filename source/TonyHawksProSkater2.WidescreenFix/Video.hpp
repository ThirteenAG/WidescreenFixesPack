#pragma once
#include "../Shared/TonyHawk/Video.hpp"

namespace THPS2Video
{
    struct Vertex
    {
        float x, y, z, rhw;
        uint32_t color;
        float u, v;
    };
    static_assert(sizeof(Vertex) == 28);

    using TonyVideo::Box;
    using TonyVideo::Layout;

    inline SafetyHookInline SetupHook, DrawHook;
    inline Vertex* Vertices = nullptr;
    inline const uint8_t* Split = nullptr;
    inline const uint32_t* MovieSize = nullptr;
    inline const uint32_t* TextureWidth = nullptr;
    inline const int32_t* Mode = nullptr;
    inline float Width = 640.0f, Height = 480.0f;
    inline std::array<Vertex, 8> Native{};
    inline bool Ready = false;

    inline int __cdecl Setup()
    {
        const auto result = SetupHook.ccall<int>();
        Ready = MovieSize[0] > 0 && MovieSize[1] > 0 && MovieSize[0] <= 16384 && MovieSize[1] <= 16384;
        if (Ready)
            std::copy_n(Vertices, Native.size(), Native.begin());
        return result;
    }

    inline int __cdecl Draw()
    {
        if (!Ready || !Mode)
            return DrawHook.ccall<int>();
        const auto box = Layout(Width, Height, static_cast<float>(MovieSize[0]), static_cast<float>(MovieSize[1]), std::clamp(*Mode, 0, 2));
        const bool split = *Split != 0;
        const float edge = split ? std::clamp(static_cast<float>(*TextureWidth) / MovieSize[0], 0.0f, 1.0f) : 1.0f;
        const auto count = split ? 8u : 4u;
        for (size_t i = 0; i < count; ++i)
        {
            auto vertex = Native[i];
            const bool right = i % 4 == 1 || i % 4 == 2;
            const bool bottom = i % 4 >= 2;
            const float x = i < 4 ? (right ? edge : 0.0f) : (right ? 1.0f : edge);
            vertex.x = box.x + x * box.width;
            vertex.y = box.y + (bottom ? box.height : 0.0f);
            Vertices[i] = vertex;
        }
        const auto result = DrawHook.ccall<int>();
        for (size_t i = 0; i < count; ++i)
            Vertices[i] = Native[i];
        return result;
    }

    inline void Install(float width, float height, const int32_t* mode, HMODULE module = GetModuleHandleW(nullptr))
    {
        Width = width;
        Height = height;
        Mode = mode;
        auto setup = hook::module_pattern(module, "A0 ? ? ? ? 83 EC 10 53 33 DB 3A C3 0F 84 ? ? ? ? A1 ? ? ? ? 8B 0D");
        auto draw = hook::module_pattern(module, "A1 ? ? ? ? 83 EC 14 85 C0 56 0F 84 ? ? ? ? 8D 44 24 08 8D 4C 24 0C");
        auto summary = hook::module_pattern(module, "A1 ? ? ? ? 68 ? ? ? ? 50 FF 15 ? ? ? ? 8B 15 ? ? ? ? 8B 0D ? ? ? ? A1");
        auto textureWidth = hook::module_pattern(module, "3B 2D ? ? ? ? 77 ? 3B 1D ? ? ? ? 0F 86");
        if (setup.size() != 1 || draw.size() != 1 || summary.size() != 1 || textureWidth.size() != 1)
        {
            OutputDebugStringA("THPS2: fullscreen movie patterns unavailable.\n");
            return;
        }
        const auto start = reinterpret_cast<uintptr_t>(draw.get_first());
        auto quad = hook::range_pattern(start, start + 0x400, "6A 00 6A 04 68 ? ? ? ? 8B 10 68 44 01 00 00 6A 06 50 FF 52 64");
        if (quad.size() != 1)
        {
            OutputDebugStringA("THPS2: fullscreen movie vertices unavailable.\n");
            return;
        }
        Split = *setup.get_first<const uint8_t*>(1);
        MovieSize = *summary.get_first<const uint32_t*>(6);
        TextureWidth = *textureWidth.get_first<const uint32_t*>(2);
        Vertices = *quad.get_first<Vertex*>(5);
        SetupHook = safetyhook::create_inline(setup.get_first(), Setup);
        DrawHook = safetyhook::create_inline(draw.get_first(), Draw);
        if (!SetupHook || !DrawHook)
        {
            SetupHook.reset();
            DrawHook.reset();
            OutputDebugStringA("THPS2: fullscreen movie hooks unavailable.\n");
        }
    }
}
