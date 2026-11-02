module;
#include <stdafx.h>
#include "FixSettings.hpp"
#include <string_view>
#include <vector>

export module NativeMenuText;


export namespace OnimushaMenu::Text
{
    inline bool Convert(const std::string& text, std::wstring& result)
    {
        if (text.size() > 16384)
            return false;
        result = asi_settings::text::Widen(text);
        for (auto& character : result)
        {
            if (character == 0x2018 || character == 0x2019)
                character = L'\'';
            else if (character == 0x2013 || character == 0x2014)
                character = L'-';
            else if (character == 0xA0 || character == L'\t')
                character = L' ';
        }
        return true;
    }

    inline bool Lines(const std::string& text, std::vector<std::wstring>& lines)
    {
        std::wstring wide;
        if (!Convert(text, wide))
            return false;
        size_t position = 0;
        while (position < wide.size())
        {
            if (lines.size() >= 512)
                return false;
            while (position < wide.size() && (wide[position] == L' ' || wide[position] == L'\r'))
                ++position;
            if (position == wide.size())
                break;
            size_t count = std::min(size_t(30), wide.size() - position);
            const auto newline = wide.find(L'\n', position);
            if (newline < position + count)
                count = newline - position;
            else if (position + count < wide.size())
            {
                const auto space = wide.rfind(L' ', position + count);
                if (space != std::wstring::npos && space > position)
                    count = space - position;
            }
            lines.push_back(wide.substr(position, count));
            position += count;
            if (position < wide.size() && wide[position] == L'\n')
                ++position;
        }
        if (lines.empty())
            lines.emplace_back();
        return true;
    }

    inline bool Fits(const std::wstring& text)
    {
        return text.size() <= 96 && text.find(L'\n') == std::wstring::npos;
    }
    inline uint8_t Glyph(wchar_t character)
    {
        if (character == L' ' || character == 0xA0) return 133;
        if (character >= 33 && character <= 126) return uint8_t(character - 32);
        constexpr wchar_t latin[] = L"\u00C4\u00E4\u00D6\u00F6\u00DC\u00FC\u00DF\u00C0\u00E0\u00C2\u00E2\u00C8\u00E8\u00C9\u00E9\u00CA\u00EA\u00CF\u00EF\u00CE\u00EE\u00D4\u00F4\u00D9\u00F9\u00C7\u00E7\u00D1\u00F1\u00CB\u00EB\u00C1\u00E1\u00CD\u00ED\u00D3\u00F3\u00DA\u00FA\u00CC\u00EC\u00D2\u00F2";
        for (size_t i = 0; i + 1 < std::size(latin); ++i)
            if (latin[i] == character) return uint8_t(144 + i);
        switch (character)
        {
            case 0x00DB: return 189;
            case 0x00FB: return 190;
            case 0x2018:
            case 0x2019: return 7;
            case 0x201C:
            case 0x201D: return 2;
            case 0x2013:
            case 0x2014: return 13;
            case 0x2026: return 116;
            case 0x2192: return 99;
            case 0x2190: return 100;
            case 0x2191: return 101;
            case 0x2193: return 102;
            case 0x00D7: return 104;
            case 0x2022: return 103;
            default: return 31;
        }
    }

    inline std::string Encode(const std::wstring& text, bool chinese)
    {
        std::string result;
        for (const auto character : text)
        {
            result += char(0xFA);
            if (chinese) result += char(0);
            result += char(Glyph(character));
        }
        result += char(0xFF);
        return result;
    }

}
