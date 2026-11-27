module;
#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <array>

export module Movies;
import Window;
import MovieGeometry;
import FloatingPoint;


export namespace PsiOpsMovies
{
    inline SafetyHookInline geometryHook;
    inline SafetyHookMid textureSizeHook;

    inline uintptr_t __cdecl Geometry(uint8_t* vertices, int decodedWidth, int decodedHeight)
    {
        const auto result = geometryHook.unsafe_ccall<uintptr_t>(vertices, decodedWidth, decodedHeight);
        if (vertices && decodedWidth > 0 && decodedHeight > 0)
        {
            PsiOps::FloatingPointScope floatingPoint;
            std::array<uint8_t, 96> copy{};
            std::memcpy(copy.data(), vertices, copy.size());
            // The movies are 4:3 pictures
            const auto destination = Fit(PsiOpsWindow::width, PsiOpsWindow::height, 4.0f / 3.0f);
            if (Reframe(copy.data(), 4, 24, {}, PsiOpsWindow::width, PsiOpsWindow::height, destination))
            {
                for (size_t i = 0; i < 4; ++i)
                {
                    float position[2];
                    std::memcpy(position, copy.data() + i * 24, sizeof(position));
                    std::memcpy(vertices + i * 24, position, sizeof(position));
                }
            }
        }
        return result;
    }

    inline void Init()
    {
        auto geometry = hook::pattern("55 8B EC 83 EC 3C C7 45 E4 00 00 00 00 C7 45 FC 00 00 00 00 DB 45 0C 0F B7 05 ? ? ? ? 89 45 D8 DA 75 D8 D9 5D DC DB 45 10");
        auto clear = hook::pattern("6A 00 68 00 00 80 3F 68 FF 00 00 FF 6A 07 6A 00 6A 00");
        geometryHook = safetyhook::create_inline(geometry.get_first(), Geometry);
        injector::WriteMemory<uint32_t>(clear.get_first<uint32_t>(8), D3DCOLOR_XRGB(0, 0, 0), true);

        auto textureSize = hook::pattern("66 89 0D ? ? ? ? 8B 55 08 66 8B 82 CC 02 00 00 66 A3 ? ? ? ? 8B 4D 08 66 8B 91 CA 02 00 00 66 89 15 ? ? ? ? 8B 45 08 66 8B 88 C8 02 00 00 66 89 0D ? ? ? ? C7 45 FC 00 00 00 00");
        static const auto uvHeight = injector::ReadMemory<uint16_t*>(textureSize.get_first(3));
        static const auto uvWidth = injector::ReadMemory<uint16_t*>(textureSize.get_first(19));
        static const auto textureHeight = injector::ReadMemory<uint16_t*>(textureSize.get_first(36));
        static const auto textureWidth = injector::ReadMemory<uint16_t*>(textureSize.get_first(53));
        textureSizeHook = safetyhook::create_mid(textureSize.get_first(57),
            [](SafetyHookContext&)
            {
                // The bundled movies decode up to 640x480 even with a smaller backbuffer.
                // Keep the texture allocation and the UV denominator in sync.
                *uvWidth = *textureWidth = std::max<uint16_t>(*textureWidth, 640);
                *uvHeight = *textureHeight = std::max<uint16_t>(*textureHeight, 480);
            });
    }
}

