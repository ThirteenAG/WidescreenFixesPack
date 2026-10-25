#pragma once

#include <Windows.h>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace KongMenu::Native
{
    inline bool Readable(const void* address, size_t size)
    {
        auto current = reinterpret_cast<uintptr_t>(address);
        if (!current || size > std::numeric_limits<uintptr_t>::max() - current)
            return false;
        const auto end = current + size;
        while (current < end)
        {
            MEMORY_BASIC_INFORMATION info{};
            if (!VirtualQuery(reinterpret_cast<const void*>(current), &info, sizeof(info)) ||
                info.State != MEM_COMMIT || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS)))
                return false;
            const auto next = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
            if (next <= current)
                return false;
            current = next;
        }
        return true;
    }

    template<class T> inline T Read(const void* object, size_t offset)
    {
        T value{};
        if (object && Readable(static_cast<const uint8_t*>(object) + offset, sizeof(value)))
            std::memcpy(&value, static_cast<const uint8_t*>(object) + offset, sizeof(value));
        return value;
    }

    struct Glyph
    {
        uint32_t symbol;
        int16_t x, y;
        uint16_t width, height;
        uint32_t color;
    };
    static_assert(sizeof(Glyph) == 16);

    struct TextSlot
    {
        uint32_t flags;
        uint16_t start, count, first, visible;
        float x, y, z;
        uint32_t remaining[6];
    };
    static_assert(sizeof(TextSlot) == 48);

    inline void* Model(void* object)
    {
        return reinterpret_cast<void* (__cdecl*)(void*)>(0x9937E0)(object);
    }

    inline TextSlot* Slot(void* model, int index)
    {
        return reinterpret_cast<TextSlot* (__cdecl*)(void*, int)>(0x993810)(model, index);
    }

    inline void SetText(void* object, int index, char* text)
    {
        reinterpret_cast<void (__cdecl*)(void*, int, char*)>(0x992B70)(object, index, text);
    }

    inline void Move(void* object, int index, const float* position)
    {
        reinterpret_cast<void (__cdecl*)(void*, int, const float*)>(0x40F4E0)(object, index, position);
    }

    inline void Flags(void* object, int index, int16_t flags)
    {
        reinterpret_cast<void (__cdecl*)(void*, int, int16_t)>(0x40F690)(object, index, flags);
    }

    inline void Color(void* object, int index, uint32_t color)
    {
        reinterpret_cast<void (__cdecl*)(void*, int, int, int, uint32_t)>(0x40F7D0)(object, index, 0, -1, color);
    }

    inline void Size(void* object, int index, const float* size)
    {
        reinterpret_cast<void (__cdecl*)(void*, int, int, int, const float*)>(0x993F20)(object, index, 0, -1, size);
    }

    inline void Align(void* object, int index, int mode)
    {
        reinterpret_cast<void (__cdecl*)(void*, int, int, int, int)>(0x994BF0)(object, index, 0, -1, mode);
    }

    inline void Measure(void* object, int index, float* extent)
    {
        reinterpret_cast<void (__cdecl*)(void*, int, int, int, float*)>(0x994AE0)(object, index, 0, -1, extent);
    }

    inline void ClearEffects(void* object, int index)
    {
        reinterpret_cast<void (__cdecl*)(void*, int, int, int)>(0x9951B0)(object, 1, -1, index);
    }

    inline void* Font(void* group, uint32_t index)
    {
        return reinterpret_cast<void* (__cdecl*)(void*, uint32_t)>(0x9C2700)(group, index);
    }

    inline bool Pressed(int action)
    {
        return reinterpret_cast<int (__cdecl*)(int)>(0x43CDF0)(action) != 0;
    }

    inline bool Key(int code)
    {
        return reinterpret_cast<int (__cdecl*)(int)>(0x43C820)(code) != 0;
    }

    // The setter parses percent substitutions before backslash commands. Encoding
    // each mapped glyph as a native 'a' command bypasses both literal grammars.
    inline bool Literal(const std::string& ascii, uint32_t fontGlyphCount, std::string& encoded)
    {
        encoded.clear();
        if (ascii.size() > 256 || !fontGlyphCount || fontGlyphCount > 4096 ||
            !Readable(reinterpret_cast<void*>(0xCC09E0), 256 * 4))
            return false;
        for (unsigned char c : ascii)
        {
            if (c < 32 || c > 126)
                return false;
            const auto glyph = Read<uint32_t>(reinterpret_cast<void*>(0xCC09E0), c * 4);
            if (glyph < 32 || glyph > 255 || glyph - 32 >= fontGlyphCount)
                return false;
            encoded += "\\a" + std::to_string(glyph) + "\\";
        }
        return encoded.size() < 4095;
    }
}
