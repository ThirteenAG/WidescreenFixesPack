module;

#include <stdafx.h>
#include <algorithm>
#include <cwctype>
#include <string>
#include <vector>

export module MenuText;

export namespace MenuText
{
    inline std::vector<std::wstring> Wrap(const std::wstring& text, size_t columns = 38)
    {
        columns = std::max<size_t>(2, columns);
        std::vector<std::wstring> lines;
        std::wstring line;
        size_t position = 0;
        while (position < text.size())
        {
            if (std::iswspace(text[position]))
            {
                if (text[position] == L'\n' && !line.empty())
                {
                    lines.push_back(std::move(line));
                    line.clear();
                }
                ++position;
                continue;
            }
            const auto start = position;
            while (position < text.size() && !std::iswspace(text[position]))
                ++position;
            auto word = text.substr(start, position - start);
            if (!line.empty() && line.size() + 1 + word.size() > columns)
            {
                lines.push_back(std::move(line));
                line.clear();
            }
            while (word.size() > columns)
            {
                auto count = columns;
                if (word[count] >= 0xDC00 && word[count] <= 0xDFFF)
                    --count;
                lines.push_back(word.substr(0, count));
                word.erase(0, count);
            }
            if (!line.empty())
                line += L' ';
            line += word;
        }
        if (!line.empty())
            lines.push_back(std::move(line));
        return lines;
    }
}
