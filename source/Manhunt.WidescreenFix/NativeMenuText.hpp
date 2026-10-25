#pragma once
#include "NativeMenuABI.hpp"
#include <string_view>
#include <vector>

namespace MHMenu::Text
{
    inline std::wstring Token(uint32_t code)
    {
        static constexpr wchar_t hex[] = L"0123456789ABCDEF";
        std::wstring result = L"U+";
        for (int shift = code > 0xFFFF ? 20 : 12; shift >= 0; shift -= 4)
            result += hex[(code >> shift) & 15];
        return result;
    }

    inline bool Convert(const std::string& text, std::wstring& result)
    {
        if (text.size() > 16384) return false;
        auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
        if (!count && !text.empty()) return false;
        std::wstring wide(static_cast<size_t>(count), L'\0');
        if (count && MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), wide.data(), count) != count) return false;
        if (!wide.empty())
        {
            auto length = LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_UPPERCASE, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr, 0);
            if (!length) return false;
            std::wstring upper(static_cast<size_t>(length), L'\0');
            if (LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_UPPERCASE, wide.data(), static_cast<int>(wide.size()), upper.data(), length, nullptr, nullptr, 0) != length) return false;
            wide = std::move(upper);
        }
        result.clear();
        for (size_t i = 0; i < wide.size(); ++i)
        {
            uint32_t code = wide[i];
            if (code >= 0xD800 && code <= 0xDBFF)
            {
                if (++i >= wide.size() || wide[i] < 0xDC00 || wide[i] > 0xDFFF) return false;
                code = 0x10000 + ((code - 0xD800) << 10) + (wide[i] - 0xDC00);
            }
            if (code == '\r') continue;
            if (code == '\t') code = ' ';
            if (code == '\n') { result += L'\n'; continue; }
            // The frontend font's native input range is the western byte set.
            // Keep unsupported characters visible without indexing beyond it.
            if (code < 32 || code > 255 || code == '~') result += Token(code);
            else result += static_cast<wchar_t>(code);
        }
        return true;
    }

    inline bool Fits(const std::wstring& text, float maximum = -1.0f)
    {
        if (text.size() > 4096 || text.find(L'\n') != std::wstring::npos) return false;
        auto scale = ABI::ScaleX();
        if (!std::isfinite(scale) || scale <= 0 || scale > 4) return false;
        if (maximum < 0) maximum = 1.0f - ABI::X() - 0.05f;
        if (!std::isfinite(maximum) || maximum <= 0 || maximum > 1) return false;
        auto width = ABI::Width(text, scale);
        return std::isfinite(width) && width >= 0 && width <= maximum;
    }

    inline bool Lines(const std::string& text, std::vector<std::wstring>& result)
    {
        std::wstring wide;
        if (!Convert(text, wide)) return false;
        size_t position = 0;
        while (position < wide.size())
        {
            if (result.size() >= 512) return false;
            size_t accepted = 0, word = 0;
            bool newline = false;
            auto maximum = (std::min)(size_t(512), wide.size() - position);
            for (size_t length = 1; length <= maximum; ++length)
            {
                if (wide[position + length - 1] == L'\n') { newline = true; break; }
                if (!Fits(wide.substr(position, length))) break;
                accepted = length;
                if (wide[position + length - 1] == L' ') word = length;
            }
            if (!accepted && !newline) return false;
            if (!newline && accepted < wide.size() - position && word) accepted = word;
            result.push_back(wide.substr(position, accepted));
            position += accepted;
            if (newline) ++position;
        }
        if (result.empty()) result.emplace_back();
        return true;
    }
}
