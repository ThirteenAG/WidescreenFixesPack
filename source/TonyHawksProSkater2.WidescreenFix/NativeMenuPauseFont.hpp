#pragma once

#include "NativeMenuWidgets.hpp"

namespace THPS2Menu
{
    // 44BA30 -> 473480/473970 -> 44A010. This is the pause backend,
    // not the title TextProperties backend. Its native glyph objects remain
    // borrowed; the host must retain the native font owner for the whole draw.
    class PauseFont
    {
        Native::Font measured{};
        std::array<uint8_t, 256> sourceMap{};
        bool ready = false;

    public:
        void Clear() { measured = {}; sourceMap = {}; ready = false; }

        bool Prepare(const Native::Font& source, const std::vector<DisplayLine>& lines, int scale = 4096, int maximumHeight = 16)
        {
            Clear();
            if (!nativeReady || source.vtable != reinterpret_cast<void*>(0x5159E4) || !FontUsable(source) || lines.empty() || lines.size() > 12 || scale < 4096 || scale > 8192 || maximumHeight < 8 || maximumHeight > 24)
                return false;
            measured = source;
            sourceMap = source.glyphMap;
            // All private rows use scale256; 473480 multiplies this by16.
            measured.scale = scale;
            measured.proportional = 0;
            measured.effect = nullptr;
            measured.alignment = 1;
            measured.verticalAlignment = 1;
            measured.shadowX = measured.shadowY = 0;
            measured.blue = 255; // Byte33 shadow switch is zero for RGB0..255.
            if (!LiteralFont(measured))
                return false;

            for (const auto& line : lines)
            {
                if (line.text.empty() || line.text.size() > 31 || std::any_of(line.text.begin(), line.text.end(), [](char code) { return static_cast<uint8_t>(code) >= 0x80 || code == 0; }))
                    return false;
                // The metric indexes eight-byte native glyph records. Validate
                // every mapped record before either metric touches native data.
                for (const unsigned char code : line.text)
                {
                    const auto glyph = measured.glyphMap[code];
                    if (glyph != 255 && !Contracts::Readable(static_cast<const uint8_t*>(measured.glyphs) + 8 * glyph, 8))
                        return false;
                }
                const int width = FunctionAt<Native::FontWidth>(0x44A470)(&measured, line.text.c_str());
                const int height = FunctionAt<Native::FontHeight>(0x44A4E0)(&measured, line.text.c_str());
                // Centered at256 in the native512x240 plane, with margins.
                // Space measurement is conservative versus the native drawer.
                if (width <= 0 || width > 440 || height < 8 || height > maximumHeight)
                    return false;
            }
            ready = true;
            return true;
        }

        bool Current(const Native::Font& source) const
        {
            // Scale/color/alignment are changed by the stock pause renderer.
            // The owned literal map and measured geometry only remain valid
            // while the underlying glyph owner/map/spacing are unchanged.
            return ready && source.vtable == measured.vtable && source.glyphs == measured.glyphs && source.glyphMap == sourceMap && !source.effect && source.spacing == measured.spacing;
        }

        bool DrawCopy(const Native::Font& source, Native::Font& output) const
        {
            if (!Current(source) || source.scale != measured.scale || source.alpha < 0 || source.alpha > 255 || source.red < 0 || source.red > 255 || source.green < 0 || source.green > 255 || source.blue < 0 || source.blue > 255)
                return false;
            output = measured;
            output.alpha = source.alpha;
            output.red = source.red;
            output.green = source.green;
            output.blue = source.blue;
            return true;
        }
    };
}
