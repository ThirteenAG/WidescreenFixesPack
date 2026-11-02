module;
#include <stdafx.h>
#include <limits>
#include <string>
#include <string_view>

export module NativeMenuFont;
import NativeMenuABI;



// Disconnected native literal metrics. This does not discover/pin a loaded font
// or establish its lifetime. A future host must supply that exact native lease.
export namespace ParadiseMenuFont
{
    struct Extent
    {
        int32_t width = 0;
        int32_t height = 0;
    };

    using LiteralMeasure = int32_t (__thiscall*)(const void*, const char*, int32_t*, int32_t*);

    inline bool Literal(std::string_view text) noexcept
    {
        if (text.size() > 4096) return false;
        // No guessed locale conversion, replacement glyphs, markup, tabs or
        // line breaks. Unsupported full values will require an unavailable row.
        for (const unsigned char byte : text)
            if (byte < 32 || byte > 126) return false;
        return true;
    }

    inline bool Measure(const void* font, std::string_view text, Extent& output)
    {
        output = {};
        if (!Literal(text)) return false;
        // 4451F6: height at4; each unsigned-byte glyph has three signed DWORD
        // advances at4104+12*c,4108+12*c,4112+12*c.
        static const auto measure = reinterpret_cast<LiteralMeasure>(
            hook::pattern("55 8B EC 53 56 8B 75 0C 8B D9 57 8B 7D 08 83 C9 FF 33 C0 F2").get_first());
        // 441310 assigns the font table; 441390 succeeds when the owned texture at7176 is non-null
        // after complete font-data loading.
        static const auto fontTable = *hook::pattern("83 A6 08 1C 00 00 00 FF 75 08 8B CE C7 06").get_first<uintptr_t>(14);
        const auto bytes = static_cast<const uint8_t*>(font);
        if (*reinterpret_cast<const uintptr_t*>(bytes) != fontTable || !*reinterpret_cast<void* const*>(bytes + 7176)) return false;
        const auto lineHeight = *reinterpret_cast<const int32_t*>(bytes + 4);
        if (lineHeight <= 0 || lineHeight > 8192) return false;
        int64_t expectedWidth = 0;
        for (const unsigned char byte : text)
        {
            const auto advance = reinterpret_cast<const int32_t*>(bytes + 4104 + 12 * byte);
            const auto width = int64_t(advance[0]) + advance[1] + advance[2];
            if (width < 0 || width > 8192) return false;
            expectedWidth += width;
            if (expectedWidth > (std::numeric_limits<int32_t>::max)()) return false;
        }
        const std::string terminated(text);
        Extent native;
        const auto result = measure(font, terminated.c_str(), &native.width, &native.height);
        if (result != lineHeight || native.height != lineHeight || native.width != expectedWidth ||
            (!text.empty() && native.width <= 0)) return false;
        output = native;
        return true;
    }
}
