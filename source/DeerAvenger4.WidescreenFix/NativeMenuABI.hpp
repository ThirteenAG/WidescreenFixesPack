#pragma once

#include <cstdint>

namespace DeerMenu::Native
{
    // Native font functions of the supported executable.
    inline constexpr int GlyphWidth = 8;
    inline constexpr int GlyphHeight = 14;
    inline constexpr unsigned FirstGlyph = 32;
    inline constexpr unsigned GlyphCount = 91;

    inline void* CreateTypeface(const uint32_t* foreground, const uint32_t* background)
    {
        // Native cdecl wrapper owns the constructed raster. Its internal helper
        // uses a custom register ABI; calling that interior helper is prohibited.
        return reinterpret_cast<void* (__cdecl*)(const uint32_t*, const uint32_t*)>(0x4C9B50)(
            foreground, background);
    }

    inline void DestroyTypeface(void* raster)
    {
        reinterpret_cast<int (__cdecl*)(void*)>(0x4C9AE0)(raster);
    }

    inline bool DrawText(void* raster, const char* text, int x, int y)
    {
        // cdecl with four arguments.
        return reinterpret_cast<void* (__cdecl*)(void*, const char*, int, int)>(0x4CA200)(
            raster, text, x, y) == raster;
    }

    using KeyHandler = int (__cdecl*)(int event, const int* key);
    using Frame = void (__cdecl*)();
    using Render = int (__cdecl*)(void* world, void* camera);
}
