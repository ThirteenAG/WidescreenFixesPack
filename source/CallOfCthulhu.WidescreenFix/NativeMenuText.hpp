#pragma once

#include <Windows.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace CthulhuMenu
{
    inline bool Readable(const void* pointer, size_t bytes)
    {
        auto current = reinterpret_cast<uintptr_t>(pointer);
        if (!current || bytes > std::numeric_limits<uintptr_t>::max() - current)
            return false;
        const auto end = current + bytes;
        while (current < end)
        {
            MEMORY_BASIC_INFORMATION info{};
            if (!VirtualQuery(reinterpret_cast<const void*>(current), &info, sizeof(info)) ||
                info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
                return false;
            const auto next = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
            if (next <= current)
                return false;
            current = next;
        }
        return true;
    }

    template<class T> inline T Read(const void* object, size_t offset)
    {
        T value{};
        if (object && Readable(static_cast<const uint8_t*>(object) + offset, sizeof(value)))
            std::memcpy(&value, static_cast<const uint8_t*>(object) + offset, sizeof(value));
        return value;
    }

    // Use an already generated native Options row's font. Resolving a fresh font
    // through the resource manager would introduce another callback/lifetime path.
    struct FontProbe
    {
        void* renderer = nullptr;
        void* font = nullptr;
        uint32_t pool = 0;
        uint32_t* bits = nullptr;
        uint8_t* glyphs = nullptr;
        uint32_t glyphCount = 0;
        uint32_t characterCount = 0;
        float denominator = 0.0f;
        float horizontal = 0.0f;
        float vertical = 0.0f;

        bool Capture(void* row)
        {
            renderer = Read<void*>(row, 320);
            if (!Readable(renderer, 352))
                return false;
            pool = Read<uint32_t>(renderer, 308);
            if (pool >= 350 || !(Read<uint32_t>(reinterpret_cast<void*>(0x7A2658), (pool >> 5) * 4) &
                (uint32_t{1} << (pool & 31))))
                return false;
            font = Read<void*>(reinterpret_cast<void*>(0x7A2688), pool * 8);
            if (!Readable(font, 36))
                return false;
            bits = Read<uint32_t*>(font, 4);
            denominator = Read<float>(font, 8);
            glyphCount = Read<uint32_t>(font, 28);
            characterCount = Read<uint32_t>(font, 24);
            glyphs = Read<uint8_t*>(font, 32);
            horizontal = Read<float>(renderer, 316);
            vertical = Read<float>(renderer, 320);
            return characterCount >= 127 && characterCount <= 65536 &&
                glyphCount > 1 && glyphCount <= 65536 &&
                Readable(bits, ((std::min(characterCount, uint32_t{256}) + 31) / 32) * 4) && Readable(glyphs, glyphCount * 52) &&
                std::isfinite(denominator) && denominator > 0.0f &&
                std::isfinite(horizontal) && horizontal > 0.0f &&
                std::isfinite(vertical) && vertical > 0.0f;
        }

        bool Current() const
        {
            return pool < 350 && Readable(renderer, 352) && Readable(font, 36) &&
                Readable(bits, ((std::min(characterCount, uint32_t{256}) + 31) / 32) * 4) && glyphCount <= 65536 && Readable(glyphs, glyphCount * 52) &&
                (Read<uint32_t>(reinterpret_cast<void*>(0x7A2658), (pool >> 5) * 4) &
                    (uint32_t{1} << (pool & 31))) &&
                Read<uint32_t>(renderer, 308) == pool &&
                Read<void*>(reinterpret_cast<void*>(0x7A2688), pool * 8) == font &&
                Read<uint32_t*>(font, 4) == bits && Read<uint8_t*>(font, 32) == glyphs &&
                Read<uint32_t>(font, 28) == glyphCount && Read<float>(font, 8) == denominator &&
                Read<uint32_t>(font, 24) == characterCount &&
                Read<float>(renderer, 316) == horizontal && Read<float>(renderer, 320) == vertical;
        }

        bool Width(const std::string& text, float& width) const
        {
            width = 0.0f;
            if (!Current() || text.size() >= 4096)
                return false;
            for (unsigned char c : text)
            {
                if (c < 32 || c == 127 || c >= characterCount)
                    return false;
                if (c == 32)
                {
                    width += horizontal * (0.315f / 0.42f);
                    continue;
                }
                const auto word = Read<uint32_t>(bits, (c >> 5) * 4);
                if (!(word & (uint32_t{1} << (c & 31))))
                    return false;
                uint32_t rank = 0;
                for (unsigned value = 0; value < c; ++value)
                    rank += (Read<uint32_t>(bits, (value >> 5) * 4) >> (value & 31)) & 1;
                const auto glyph = rank + 1;
                if (glyph >= glyphCount)
                    return false;
                const auto advance = Read<int16_t>(glyphs, glyph * 52 + 20);
                if (advance <= 0)
                    return false;
                width += horizontal * advance / (denominator * 0.42f);
            }
            // Native focus draws a 1.04-scale copy. Fit the larger state too.
            width *= 1.04f;
            return std::isfinite(width) && Current();
        }
    };

    struct FontMetrics
    {
        std::array<float, 256> advances{};
        bool ready = false;

        bool Capture(const FontProbe& probe)
        {
            ready = false;
            advances.fill(-1.0f);
            for (unsigned c = 32; c < advances.size(); ++c)
            {
                float width = 0.0f;
                if (probe.Width(std::string(1, static_cast<char>(c)), width))
                    advances[c] = width;
            }
            ready = probe.Current() && advances[32] > 0.0f;
            return ready;
        }

        bool Width(const std::string& text, float& width) const
        {
            width = 0.0f;
            if (!ready || text.size() >= 4096)
                return false;
            for (unsigned char c : text)
            {
                if (c < 32 || c == 127 || advances[c] <= 0.0f)
                    return false;
                width += advances[c];
            }
            return std::isfinite(width);
        }

        bool Encode(const std::string& text, std::string& output) const
        {
            output.clear();
            if (!ready || text.size() >= 16384) return false;
            const auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
            if (!count && !text.empty()) return false;
            std::wstring wide(size_t(count), L'\0');
            if (count) MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), wide.data(), count);
            auto available = [&](unsigned char c) { return c >= 32 && c != 127 && advances[c] > 0.0f; };
            auto fallback = [&]() -> char
            {
                for (unsigned char c : { '?', '-', '*', '#' }) if (available(c)) return char(c);
                return 0;
            };
            for (size_t i = 0; i < wide.size(); ++i)
            {
                auto code = wide[i];
                if (code < 32 || code == 8232 || code == 8233) code = L' ';
                char value = 0; BOOL substituted = FALSE;
                // The native glyph parser consumes unsigned bytes, not UTF-8.
                // Preserve available western glyphs, then use visible fallbacks.
                if (code < 0xD800 || code > 0xDFFF)
                    if (WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, &code, 1, &value, 1, nullptr, &substituted) != 1 || substituted || !available(uint8_t(value))) value = 0;
                if (!value)
                {
                    wchar_t decomposed[8]{};
                    const auto size = FoldStringW(MAP_COMPOSITE, &code, 1, decomposed, int(std::size(decomposed)));
                    if (size > 0 && decomposed[0] >= 32 && decomposed[0] < 127 && available(uint8_t(decomposed[0]))) value = char(decomposed[0]);
                }
                if (!value)
                {
                    if (code >= 0xD800 && code <= 0xDBFF && i + 1 < wide.size() && wide[i + 1] >= 0xDC00 && wide[i + 1] <= 0xDFFF) ++i;
                    value = fallback();
                }
                if (!value) return false;
                output.push_back(value);
                if (output.size() >= 4096) return false;
            }
            return true;
        }

        bool Fit(std::string text, float maximum, std::string& output) const
        {
            output.clear();
            float width = 0.0f;
            if (!std::isfinite(maximum) || maximum < 0.0f || !Width(text, width))
                return false;
            if (width < maximum || text.empty())
            {
                output = std::move(text);
                return true;
            }
            const std::string dots = "...";
            if (!Width(dots, width))
                return false;
            if (width >= maximum)
                return true;
            while (!text.empty())
            {
                text.pop_back();
                if (!Width(text + dots, width))
                    return false;
                if (width < maximum)
                {
                    output = text + dots;
                    return true;
                }
            }
            output = dots;
            return true;
        }

        bool Split(const std::string& text, float maximum, std::vector<std::string>& output) const
        {
            output.clear();
            if (!ready || !std::isfinite(maximum) || maximum <= 0.0f || text.size() >= 4096)
                return false;
            if (text.empty())
            {
                output.emplace_back();
                return true;
            }
            size_t first = 0;
            while (first < text.size())
            {
                float width = 0.0f;
                size_t last = first;
                size_t boundary = first;
                while (last < text.size())
                {
                    const auto c = static_cast<unsigned char>(text[last]);
                    if (c < 32 || c == 127 || advances[c] <= 0.0f)
                        return false;
                    const auto next = width + advances[c];
                    if (!std::isfinite(next) || next >= maximum)
                        break;
                    width = next;
                    ++last;
                    if (c == ' ' || c == '/' || c == '\\' || c == '-' || c == ':' || c == '.')
                        boundary = last;
                }
                if (last == first)
                    return false;
                if (last < text.size() && boundary > first)
                    last = boundary;
                output.emplace_back(text.substr(first, last - first));
                first = last;
                if (output.size() > 128)
                    return false;
            }
            return true;
        }
    };
}
