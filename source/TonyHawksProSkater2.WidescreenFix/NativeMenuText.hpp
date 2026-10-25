#pragma once

#include "NativeMenuABI.hpp"
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace THPS2Menu
{
    // THPS2 does not have THUG's unsigned glyph lookup. Never send raw UTF-8
    // or another game's encoding to its signed-byte map. Fold western letters
    // and punctuation to ASCII; unsupported codepoints get one visible '?'.
    inline std::string PlainText(std::string_view utf8)
    {
        if (utf8.size() > 16384) return "?";
        std::string result;
        for (size_t i = 0; i < utf8.size();)
        {
            uint32_t code = static_cast<uint8_t>(utf8[i++]);
            if (code >= 0x80)
            {
                size_t count = code >= 0xC2 && code <= 0xDF ? 1 : code >= 0xE0 && code <= 0xEF ? 2 : code >= 0xF0 && code <= 0xF4 ? 3 : 0;
                uint32_t minimum = count == 1 ? 0x80 : count == 2 ? 0x800 : 0x10000;
                bool valid = count && count <= utf8.size() - i;
                uint32_t decoded = code & (count == 1 ? 0x1F : count == 2 ? 0x0F : 0x07);
                for (size_t k = 0; valid && k < count; ++k)
                {
                    auto byte = static_cast<uint8_t>(utf8[i + k]);
                    valid = (byte & 0xC0) == 0x80;
                    decoded = (decoded << 6) | (byte & 0x3F);
                }
                if (valid)
                {
                    valid = decoded >= minimum && decoded <= 0x10FFFF && !(decoded >= 0xD800 && decoded <= 0xDFFF);
                    if (valid)
                        i += count;
                }
                code = valid ? decoded : '?';
            }
            if (code == 0x2018 || code == 0x2019)
                code = '\'';
            else if (code == 0x201C || code == 0x201D)
                code = '"';
            else if (code == 0x2013 || code == 0x2014)
                code = '-';
            else if (code == 0xA0 || code == '\r' || code == '\n' || code == '\t')
                code = ' ';
            if (code == 0xDF) { result += "ss"; continue; }
            if (code == 0xC6 || code == 0xE6) { result += code == 0xC6 ? "AE" : "ae"; continue; }
            if (code == 0x152 || code == 0x153) { result += code == 0x152 ? "OE" : "oe"; continue; }
            if (code == 0xD8 || code == 0xF8) code = code == 0xD8 ? 'O' : 'o';
            if (code >= 0x80 && code <= 0xFFFF)
            {
                wchar_t original = wchar_t(code), decomposed[8]{};
                if (FoldStringW(MAP_COMPOSITE, &original, 1, decomposed, int(std::size(decomposed))) > 0 && decomposed[0] >= 32 && decomposed[0] <= 126)
                    code = uint32_t(decomposed[0]);
            }
            if (code < 0x20 || code > 0x7E)
                code = '?';
            result += static_cast<char>(code);
        }
        return result;
    }

    // The native font turns missing reserved characters into pad-button icons
    // and other missing characters into blank advance. Private map copies use
    // a visible '?' for unsupported printable characters; spaces retain their
    // native blank advance. Native game fonts and shared drawers stay intact.
    inline bool LiteralFont(Native::Font& font)
    {
        auto replacement = font.glyphMap[static_cast<size_t>('?')];
        if (replacement == 255 || !font.glyphs || font.effect)
            return false;
        for (size_t code = 0x21; code <= 0x7E; ++code)
        {
            auto& mapped = font.glyphMap[code];
            if (mapped == 255)
                mapped = replacement;
        }
        return true;
    }

    inline std::string FitText(const std::string& text, size_t maximum)
    {
        if (text.size() <= maximum) return text;
        if (maximum < 3) return text.substr(0, maximum);
        return text.substr(0, maximum - 3) + "...";
    }

    // Preserve all converted text in continuation rows. Callers must bound the
    // resulting native layout and fail closed if the complete text cannot fit;
    // a 31-byte native button buffer is not permission to silently truncate.
    inline std::vector<std::string> TextLines(const std::string& text, size_t maximum)
    {
        std::vector<std::string> lines;
        if (!maximum)
            return lines;
        for (size_t offset = 0; offset < text.size();)
        {
            size_t count = std::min(maximum, text.size() - offset);
            if (count < text.size() - offset)
            {
                auto space = text.rfind(' ', offset + count - 1);
                if (space != std::string::npos && space > offset)
                    count = space - offset;
            }
            lines.emplace_back(text.substr(offset, count));
            offset += count;
            if (offset < text.size() && text[offset] == ' ')
                ++offset;
        }
        if (lines.empty())
            lines.emplace_back();
        return lines;
    }
}
