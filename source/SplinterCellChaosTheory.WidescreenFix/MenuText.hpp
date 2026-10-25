#pragma once
#include <Windows.h>
#include <algorithm>
#include <string>

namespace CTText
{
    struct Metrics
    {
        void* font = nullptr;
        double scale = 1, spacing = 0, width = 490;
        int Character(wchar_t c) const
        {
            if (!font)
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
    inline std::wstring Setting(const std::wstring& label, const std::wstring& value, const Metrics& metrics)
    {
        // Keep a setting identifiable even when its translated value is long.
        // Both complete strings remain available in the native Details page.
        const std::wstring separator = L": ";
        const auto gap = std::max(0.0, metrics.spacing);
        const auto available = std::max(0.0, metrics.width - metrics.Measure(separator) - 2 * gap);
        const auto labelWidth = metrics.Measure(label);
        const auto valueWidth = metrics.Measure(value);
        double valueBudget = valueWidth;
        if (labelWidth + valueWidth > available)
            valueBudget = std::min(valueWidth, available - std::min(labelWidth, available * 0.5));
        const auto shownValue = Fit(value, metrics, valueBudget);
        const auto shownLabel = Fit(label, metrics, std::max(0.0, available - metrics.Measure(shownValue)));
        return shownLabel + separator + shownValue;
    }
    inline std::wstring Style(std::wstring text, const Metrics& metrics, const wchar_t* locale, bool uppercase = true)
    {
        if (!metrics.font)
            return {};
        const auto count = uppercase ? LCMapStringEx(locale, LCMAP_UPPERCASE, text.data(), int(text.size()), nullptr, 0, nullptr, nullptr, 0) : 0;
        if (count > 0 && count <= 1024)
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
} // namespace CTText
