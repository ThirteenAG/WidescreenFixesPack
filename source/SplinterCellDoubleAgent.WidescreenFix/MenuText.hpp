#pragma once
#include <Windows.h>
#include <algorithm>
#include <string>
#include <functional>
#include <vector>
#include <cmath>

namespace DAText
{
    struct Metrics
    {
        void* font = nullptr;
        std::function<bool(wchar_t)> available;
        double scale = 1, spacing = 0, width = 490;
        int Character(wchar_t c) const
        {
            if (!font || (available && !available(c)))
                return 0;
            auto method = (*static_cast<void***>(font))[3];
            return reinterpret_cast<int(__fastcall*)(void*, void*, unsigned)>(method)(font, nullptr, unsigned(c));
        }
        double Measure(const std::wstring& text) const
        {
            if (!font || text.empty())
                return 0;
            auto method = (*static_cast<void***>(font))[4];
            auto width = reinterpret_cast<int(__fastcall*)(void*, void*, const wchar_t*)>(method)(font, nullptr, text.c_str());
            return width * scale + (text.size() - 1) * spacing;
        }
    };
    inline void RemoveLast(std::wstring& text)
    {
        if (text.empty())
            return;
        text.pop_back();
        if (!text.empty() && text.back() >= 0xD800 && text.back() <= 0xDBFF)
            text.pop_back();
    }
    inline std::wstring Fit(std::wstring text, const Metrics& metrics, double width)
    {
        if (!metrics.font)
            return {};
        if (metrics.Measure(text) <= width)
            return text;
        const std::wstring dots = L"...";
        while (!text.empty() && metrics.Measure(text + dots) > width)
            RemoveLast(text);
        return metrics.Measure(dots) <= width ? text + dots : std::wstring{};
    }
    inline std::wstring Style(std::wstring text, const Metrics& metrics, const wchar_t* locale, bool uppercase = true)
    {
        if (!metrics.font)
            return {};
        const auto count =
            uppercase ? LCMapStringEx(locale, LCMAP_UPPERCASE, text.data(), int(text.size()), nullptr, 0, nullptr, nullptr, 0) : 0;
        if (count > 0 && count <= 65536)
        {
            std::wstring upper(size_t(count), L'\0');
            if (LCMapStringEx(locale, LCMAP_UPPERCASE, text.data(), int(text.size()), upper.data(), count, nullptr, nullptr, 0))
                text = std::move(upper);
        }
        std::wstring result;
        for (size_t i = 0; i < text.size(); ++i)
        {
            auto c = text[i];
            if (c < L' ')
                c = L' ';
            if (c >= 0xD800 && c <= 0xDBFF && i + 1 < text.size() && text[i + 1] >= 0xDC00 && text[i + 1] <= 0xDFFF)
            {
                ++i;
                c = L'?';
            }
            if (!metrics.Character(c))
            {
                wchar_t decomposed[8]{};
                const auto n = FoldStringW(MAP_COMPOSITE, &c, 1, decomposed, 8);
                c = n > 0 && metrics.Character(decomposed[0]) ? decomposed[0] : L'?';
            }
            result.push_back(c);
        }
        return result;
    }
    inline std::vector<std::wstring> Wrap(const std::wstring& text, double width, const std::function<double(const std::wstring&)>& measure)
    {
        if (!std::isfinite(width) || width <= 0)
            return {};
        std::vector<std::wstring> lines;
        std::wstring line;
        auto flush = [&]()
        {
            if (!line.empty())
            {
                lines.push_back(std::move(line));
                line.clear();
            }
        };
        for (size_t at = 0; at < text.size();)
        {
            if (text[at] == L'\n')
            {
                flush();
                ++at;
                continue;
            }
            if (iswspace(text[at]))
            {
                ++at;
                continue;
            }
            auto end = at;
            while (end < text.size() && !iswspace(text[end]))
                ++end;
            auto word = text.substr(at, end - at);
            at = end;
            if (!line.empty() && measure(line + L" " + word) > width)
                flush();
            while (!word.empty() && measure(word) > width)
            {
                size_t take = 0;
                for (size_t i = 0; i < word.size();)
                {
                    size_t next = i + 1;
                    if (word[i] >= 0xD800 && word[i] <= 0xDBFF && next < word.size() && word[next] >= 0xDC00 && word[next] <= 0xDFFF)
                        ++next;
                    if (measure(word.substr(0, next)) > width)
                        break;
                    take = next;
                    i = next;
                }
                if (!take)
                    return {};
                lines.push_back(word.substr(0, take));
                word.erase(0, take);
            }
            if (!word.empty())
            {
                if (!line.empty())
                    line += L' ';
                line += word;
            }
        }
        flush();
        return lines;
    }
} // namespace DAText
