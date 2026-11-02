#pragma once
#include "FixSettings.hpp"
#include <string_view>
#include <vector>

namespace ScarfaceMenu::Text
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
}
