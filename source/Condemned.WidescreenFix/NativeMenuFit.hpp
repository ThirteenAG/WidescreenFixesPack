#pragma once
#include "NativeMenuABI.hpp"
#include <algorithm>
#include <string>

// Disconnected physical text fitting. Caller owns the native/module/font lease
// and supplies an exact current-owner check after every native boundary.
namespace CondemnedMenuNative
{
    inline std::optional<std::wstring> Wide(std::string_view text)
    {
        if (text.size() > 4096 || text.find('\0') != std::string_view::npos) return {};
        if (text.empty()) return std::wstring{};
        const auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
            static_cast<int>(text.size()), nullptr, 0);
        if (count <= 0) return {};
        std::wstring out(static_cast<size_t>(count), L'\0');
        if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
            static_cast<int>(text.size()), out.data(), count) != count) return {};
        return out;
    }
    inline size_t PrefixBoundary(std::wstring_view text, size_t count)
    {
        count = (std::min)(count, text.size());
        if (count && count < text.size() && text[count - 1] >= 0xD800 && text[count - 1] <= 0xDBFF &&
            text[count] >= 0xDC00 && text[count] <= 0xDFFF) --count;
        return count;
    }
    struct Fitted { std::wstring text; uint32_t fontHeight{}; RECT bounds{}; };
    template<class Current>
    inline std::optional<Fitted> Fit(const Calls& calls, void* row, std::wstring_view text,
        uint32_t initialHeight, uint32_t minimumHeight, int pixelWidth, int pixelHeight,
        bool fullValue, Current&& current, std::wstring_view requiredTail = {})
    {
        if (!initialHeight || initialHeight > 256 || !minimumHeight || minimumHeight > initialHeight || pixelWidth <= 0 ||
            pixelHeight < 8 || text.size() > 4096 || text.find(L'\0') != std::wstring_view::npos) return {};
        // Multiline text cannot silently escape a single native menu row.
        if (text.find_first_of(L"\r\n") != std::wstring_view::npos) return {};
        auto measure = [&](std::wstring_view value, uint32_t height) -> std::optional<Fitted>
        {
            if (!current()) return {};
            std::wstring copy(value);
            if (!calls.SetText(row, copy.c_str()) || !current()) return {};
            if (!calls.FontSize(row, height) || !current()) return {};
            if (!calls.BuildString(row) || !current()) return {};
            const auto box = calls.Bounds(row);
            if (!current() || !box) return {};
            const int64_t width = static_cast<int64_t>(box->right) - box->left;
            const int64_t measuredHeight = static_cast<int64_t>(box->bottom) - box->top;
            if (width > pixelWidth || measuredHeight > pixelHeight || (!value.empty() && measuredHeight < 8)) return {};
            return Fitted{std::move(copy), height, *box};
        };
        for (auto height = initialHeight; ; --height)
        {
            if (auto fitted = measure(text, height)) return fitted;
            if (!current() || height == minimumHeight) break;
        }
        if (fullValue || !current()) return {}; // Explicit unavailable, never partial value.
        if (!requiredTail.empty() && !text.ends_with(requiredTail)) return {};
        const auto prefix = text.substr(0, text.size() - requiredTail.size());
        const std::wstring suffix = std::wstring(L"...") + std::wstring(requiredTail);
        // At most13 metric candidates for a4096-unit label. Every accepted
        // candidate is measured; no monotonic-width assumption is needed for
        // safety (unusual kerning can only make the result more conservative).
        auto best = measure(suffix, minimumHeight);
        if (!best || !current()) return {};
        size_t low = 0, high = prefix.size();
        for (size_t attempt = 0; low < high && attempt < 13; ++attempt)
        {
            const auto midpoint = low + (high - low + 1) / 2;
            const auto count = PrefixBoundary(prefix, midpoint);
            std::wstring shortened(prefix.substr(0, count)); shortened += suffix;
            if (auto fitted = measure(shortened, minimumHeight))
            {
                best = std::move(fitted);
                low = midpoint;
            }
            else high = midpoint - 1;
            if (!current()) return {};
        }
        // Leave the native object's text/font consistent with the chosen result.
        return measure(best->text, best->fontHeight);
    }
}
