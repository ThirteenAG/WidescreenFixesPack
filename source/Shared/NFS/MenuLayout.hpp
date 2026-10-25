#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>

namespace WFPMenuLayout
{
    struct Slice
    {
        size_t page{};
        size_t pages{ 1 };
        size_t start{};
        size_t count{};
        bool selector{};
    };

    inline Slice Root(size_t count, size_t capacity, size_t page)
    {
        if (!capacity || !count)
            return {};
        if (count <= capacity)
            return { 0, 1, 0, count, false };
        auto rows = std::max<size_t>(1, capacity - 1);
        auto pages = count / rows + (count % rows != 0);
        page %= pages;
        auto start = page * rows;
        return { page, pages, start, std::min(rows, count - start), capacity > 1 };
    }

    // Input is already converted to the game's single-byte font encoding.
    template<typename Measure>
    std::string Fit(std::string text, float width, Measure&& measure)
    {
        if (!std::isfinite(width) || width <= 0.0f)
            return {};
        auto length = measure(text);
        if (!std::isfinite(length) || length <= width)
            return text;
        while (!text.empty())
        {
            text.pop_back();
            auto candidate = text + "...";
            length = measure(candidate);
            if (std::isfinite(length) && length <= width)
                return candidate;
        }
        return measure("...") <= width ? "..." : "";
    }

    template<typename Measure>
    std::vector<std::string> Wrap(std::string_view text, float width, Measure&& measure)
    {
        std::vector<std::string> lines;
        if (!std::isfinite(width) || width <= 0.0f)
            return lines;
        std::string line;
        for (size_t at = 0; at < text.size();)
        {
            if (text[at] == '\n')
            {
                lines.push_back(std::move(line));
                line.clear();
                ++at;
                continue;
            }
            if (text[at] == ' ' || text[at] == '\r' || text[at] == '\t')
            {
                ++at;
                continue;
            }
            auto end = text.find_first_of(" \r\n\t", at);
            if (end == std::string_view::npos)
                end = text.size();
            auto word = std::string(text.substr(at, end - at));
            at = end;
            auto candidate = line.empty() ? word : line + " " + word;
            if (!line.empty() && measure(candidate) > width)
            {
                lines.push_back(std::move(line));
                line.clear();
            }
            while (!word.empty() && measure(word) > width)
            {
                size_t length = 1;
                while (length < word.size() && measure(word.substr(0, length + 1)) <= width)
                    ++length;
                lines.push_back(word.substr(0, length));
                word.erase(0, length);
            }
            if (!word.empty())
                line += (line.empty() ? "" : " ") + word;
        }
        if (!line.empty())
            lines.push_back(std::move(line));
        return lines;
    }
}
