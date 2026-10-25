#pragma once

#include "NativeMenuABI.hpp"
#include <string_view>
#include <string>
#include <vector>
#include <algorithm>

namespace DeerMenu
{
    // 4C9810 exposes91 embedded8x14 glyphs. 4C9E40 sign-extends each byte and
    // subtracts32 without validating it. No Unicode/replacement font is assumed.
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
            if (code < Native::FirstGlyph || code >= Native::FirstGlyph + Native::GlyphCount)
                code = '?';
            result += static_cast<char>(code);
        }
        return result;
    }

    inline bool NativeText(std::string_view text)
    {
        for (unsigned char ch : text)
            if (ch < Native::FirstGlyph || ch >= Native::FirstGlyph + Native::GlyphCount)
                return false;
        return true;
    }

    inline bool TextFits(std::string_view text, int width, int height)
    {
        const auto plain = PlainText(text);
        return NativeText(plain) && width >= 0 && height >= Native::GlyphHeight &&
            plain.size() <= static_cast<std::size_t>(width / Native::GlyphWidth);
    }

    inline bool FitText(std::string_view text, int width, std::string& output)
    {
        output.clear();
        const auto plain = PlainText(text); text = plain;
        if (!NativeText(text) || width < 0)
            return false;
        const auto columns = size_t(width / Native::GlyphWidth);
        if (text.size() <= columns)
            output.assign(text);
        else if (columns <= 3)
            output.assign(columns, '.');
        else
            output = std::string(text.substr(0, columns - 3)) + "...";
        return true;
    }

    inline bool SplitText(std::string_view text, int width, std::vector<std::string>& pieces)
    {
        pieces.clear();
        const auto plain = PlainText(text); text = plain;
        if (!NativeText(text) || width < Native::GlyphWidth)
            return false;
        const auto columns = static_cast<std::size_t>(width / Native::GlyphWidth);
        while (!text.empty())
        {
            auto count = std::min(columns, text.size());
            if (count < text.size())
            {
                // Whole words/path components only. Preserve the delimiter and
                // every original character; refuse a component wider than a row.
                const auto split = text.substr(0, count).find_last_of(" /\\:._-");
                if (split == std::string_view::npos)
                    return false;
                count = split + 1;
            }
            pieces.emplace_back(text.substr(0, count));
            text.remove_prefix(count);
            if (pieces.size() > 2048)
                return false;
        }
        if (pieces.empty())
            pieces.emplace_back();
        return true;
    }

}
