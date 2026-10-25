#pragma once
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace GTA2MenuText
{
    inline std::wstring Native(std::wstring text)
    {
        // Frontend fonts use compact Latin glyph numbers, not Unicode code points.
        constexpr std::wstring_view capitals{ L"ÀÁÂÄÆÇÈÉËÊÌÍÏÎÒÓÔÖÙÚÛÜß" };
        constexpr std::wstring_view lowercase{ L"àáâäæçèéëêìíïîòóôöùúûüß" };
        std::wstring out;
        for (auto c : text)
        {
            auto upper = capitals.find(c);
            auto lower = lowercase.find(c);
            if (upper != std::wstring_view::npos || lower != std::wstring_view::npos)
                out += wchar_t(0x80 + (upper != std::wstring_view::npos ? upper : lower));
            else if (c == L'Ñ' || c == L'ñ')
                out += wchar_t(0xAD);
            else if (c == L'¿')
                out += wchar_t(0xAF);
            else if (c == L'¡')
                out += wchar_t(0xB0);
            else if (c == L'Œ' || c == L'œ')
                out += L"OE";
            else if (c == L'’' || c == L'‘' || c == L'`')
                out += L'\'';
            else if (c == L'“' || c == L'”')
                out += L'"';
            else if (c == L'–' || c == L'—' || c == L'-' || c == L'_')
                out += L'?';
            else if (c == L'[' || c == L'{')
                out += L'(';
            else if (c == L']' || c == L'}')
                out += L')';
            else if (c == L'\\')
                out += L'/';
            else if (c == L'=')
                out += L':';
            else if (c >= L'a' && c <= L'z')
                out += wchar_t(c - L'a' + L'A');
            else if ((c >= L' ' && c <= L'Z' && c != L'#' && c != L'&' && c != L'*' && c != L'@') || c == L'\n')
                out += c;
            else
                out += L'?';
        }
        return out;
    }

    template <typename Measure>
    std::wstring Fit(std::wstring text, int width, size_t limit, Measure measure)
    {
        if (text.size() <= limit && measure(text) <= width)
            return text;
        const std::wstring dots{ L"..." };
        if (limit < dots.size() || measure(dots) > width)
            return {};
        while (!text.empty() && (text.size() + dots.size() > limit || measure(text + dots) > width))
            text.pop_back();
        return text + dots;
    }

    template <typename Measure>
    std::wstring Join(const std::wstring& label, const std::wstring& value, int width, size_t limit, Measure measure)
    {
        if (value.empty())
            return Fit(label, width, limit, measure);
        if (value.size() > limit || measure(value) > width)
            return {};
        const std::wstring gap{ L"   " };
        auto remaining = width - measure(value) - measure(gap);
        auto characters = limit > value.size() + gap.size() ? limit - value.size() - gap.size() : 0;
        auto fitted = Fit(label, remaining, characters, measure);
        return fitted.empty() ? value : fitted + gap + value;
    }

    template <typename Measure>
    std::vector<std::wstring> Wrap(std::wstring text, int width, size_t limit, Measure measure)
    {
        std::vector<std::wstring> lines;
        while (!text.empty())
        {
            size_t count{};
            while (count < text.size() && count < limit && text[count] != L'\n' && measure(text.substr(0, count + 1)) <= width)
                ++count;
            if (!count)
            {
                if (text[0] != L'\n')
                    return {};
                text.erase(0, 1);
                continue;
            }
            auto consumed = count;
            if (count < text.size() && text[count] != L'\n')
            {
                auto space = text.rfind(L' ', count);
                if (space != std::wstring::npos && space)
                    count = consumed = space;
            }
            lines.push_back(text.substr(0, count));
            text.erase(0, consumed);
            while (!text.empty() && (text[0] == L' ' || text[0] == L'\n'))
                text.erase(0, 1);
        }
        return lines;
    }
}
