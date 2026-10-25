#pragma once
#include "StaticText.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace NFSPSHelp
{
    using FontLookup = void* (__cdecl*)(uint32_t);
    inline int (__cdecl* ReadWide)(wchar_t*, uint8_t*, int) = nullptr;
    inline double (__thiscall* Advance)(void*, uint32_t, uint16_t, uint32_t) = nullptr;


    inline float RenderScale(uint32_t font, int language)
    {
        return (language == 11 || language == 12 || language == 13) && font == 0x9583AA1A ? 2.0f : 1.0f;
    }


    struct List
    {
        int count;
        uint8_t* head;
        uint8_t* tail;
    };

    struct GlyphBounds
    {
        float top;
        float bottom;
    };

    struct Font
    {
        GlyphBounds glyphs;
        float origin;
        float height;
    };

    struct Frame
    {
        float x;
        float y;
        float z;
        float scaleX;
        float scaleY;
        float parentX;
        float parentY;
        float originX;
        float originY;
        float top;
        float bottom;
    };

    struct Layout
    {
        float x;
        float y;
        float z;
        float width;
        float pitch;
        float scaleX;
        float scaleY;
        size_t lines;
        uint32_t font;
        std::array<uint32_t, 4> color;
    };

    class Captions
    {
        struct Package
        {
            std::array<uint32_t, 2> ids{};
        };
        std::unordered_map<std::string, Package> packages;
        std::unordered_map<const uint8_t*, GlyphBounds> bounds;

        static uint32_t Hash(const char* text)
        {
            uint32_t result = 0xFFFFFFFF;
            while (*text)
                result = result * 33 + uint8_t(*text++);
            return result;
        }

        static uint32_t Name(size_t line)
        {
            return Hash(line ? "WFPPS_HELP_1" : "WFPPS_HELP_0");
        }

        static bool String(uint8_t* object)
        {
            return object && *reinterpret_cast<uint32_t*>(object + 0x18) == 2 &&
                *reinterpret_cast<uint32_t*>(object + 0x30) == 68 &&
                *reinterpret_cast<uintptr_t*>(object + 0x2C);
        }

        static uint8_t* Find(List* list, uint32_t hash)
        {
            for (auto object = list->head; object; object = *reinterpret_cast<uint8_t**>(object + 4))
            {
                if (*reinterpret_cast<uint32_t*>(object + 0x10) == hash)
                    return object;
                if (*reinterpret_cast<uint32_t*>(object + 0x18) == 5)
                {
                    if (auto child = Find(reinterpret_cast<List*>(object + 0x60), hash))
                        return child;
                }
            }
            return nullptr;
        }

        static uint32_t Opacity(List* list, uint8_t* selected, uint32_t parent, size_t depth, size_t& remaining)
        {
            if (depth >= 32)
                return 0;
            for (auto object = list->head; object; object = *reinterpret_cast<uint8_t**>(object + 4))
            {
                if (!remaining--)
                    return 0;
                if ((*reinterpret_cast<uint32_t*>(object + 0x1C) & 0x41) != 0)
                    continue;
                auto state = *reinterpret_cast<uint32_t**>(object + 0x2C);
                if (!state || *reinterpret_cast<uint32_t*>(object + 0x30) < 68 || state[3] > 255)
                    continue;
                auto alpha = (state[3] * parent + 128) >> 8;
                if (!alpha)
                    continue;
                if (object == selected)
                    return alpha;
                if (*reinterpret_cast<uint32_t*>(object + 0x18) == 5)
                {
                    if (auto child = Opacity(reinterpret_cast<List*>(object + 0x60), selected, alpha, depth + 1, remaining))
                        return child;
                }
            }
            return 0;
        }

        template<class Visit>
        static void Walk(List* list, Visit&& visit)
        {
            for (auto object = list->head; object; object = *reinterpret_cast<uint8_t**>(object + 4))
            {
                if ((*reinterpret_cast<uint32_t*>(object + 0x1C) & 0x41) != 0)
                    continue;
                auto state = *reinterpret_cast<uint32_t**>(object + 0x2C);
                if (state && *reinterpret_cast<uint32_t*>(object + 0x30) >= 68 && !state[3])
                    continue;
                visit(object);
                if (*reinterpret_cast<uint32_t*>(object + 0x18) == 5)
                    Walk(reinterpret_cast<List*>(object + 0x60), visit);
            }
        }

        bool GetFont(uint8_t* object, FontLookup lookup, Font& result)
        {
            if (!String(object) || !lookup)
                return false;
            auto font = static_cast<uint8_t*>(lookup(*reinterpret_cast<uint32_t*>(object + 0x24)));
            auto header = font ? *reinterpret_cast<const uint8_t**>(font + 24) : nullptr;
            if (!header || *reinterpret_cast<const uint32_t*>(header) != 0x46746E46)
                return false;
            auto found = bounds.find(header);
            if (found == bounds.end())
            {
                auto count = *reinterpret_cast<const uint16_t*>(header + 10);
                auto offset = *reinterpret_cast<const uint32_t*>(header + 20);
                if (!count || offset > 0x1000000)
                    return false;
                auto stride = *reinterpret_cast<const uint32_t*>(header + 12) & 0x40000 ? 16 : 12;
                GlyphBounds glyphs{ std::numeric_limits<float>::max(), std::numeric_limits<float>::lowest() };
                for (size_t i = 0; i < count; ++i)
                {
                    auto glyph = header + offset + i * stride;
                    auto y = float(*reinterpret_cast<const int8_t*>(glyph + 10));
                    glyphs.top = std::min(glyphs.top, y);
                    glyphs.bottom = std::max(glyphs.bottom, y + glyph[3]);
                }
                found = bounds.emplace(header, glyphs).first;
            }
            result = { found->second, *reinterpret_cast<float*>(font + 48), *reinterpret_cast<float*>(font + 44) };
            return std::isfinite(result.origin) && std::isfinite(result.height) && result.height > 0.0f &&
                result.glyphs.bottom > result.glyphs.top;
        }

        bool GetFrame(uint8_t* object, FontLookup lookup, int language, Frame& frame)
        {
            Font font;
            if (!GetFont(object, lookup, font) || (*reinterpret_cast<uint32_t*>(object + 0x1C) & 0x400041) != 0)
                return false;
            auto cache = *reinterpret_cast<uint8_t**>(object + 0x58);
            if (!cache || cache == reinterpret_cast<uint8_t*>(0x7FFFFFFF) || (*reinterpret_cast<uint32_t*>(cache + 12) & 2) == 0)
                return false;
            // Native text draw nodes retain the font-to-640x480 matrix at +0x30.
            auto matrix = reinterpret_cast<const float*>(cache + 48);
            for (size_t i = 0; i < 16; ++i)
            {
                if (!std::isfinite(matrix[i]))
                    return false;
            }
            if (matrix[0] <= 0.0f || matrix[5] <= 0.0f || std::abs(matrix[15] - 1.0f) > 0.001f ||
                std::abs(matrix[1]) > 0.001f || std::abs(matrix[4]) > 0.001f ||
                std::abs(matrix[3]) > 0.001f || std::abs(matrix[7]) > 0.001f || std::abs(matrix[11]) > 0.001f)
                return false;
            auto data = *reinterpret_cast<float**>(object + 0x2C);
            for (size_t i = 4; i < 17; ++i)
            {
                if (!std::isfinite(data[i]))
                    return false;
            }
            for (size_t i : { 4u, 5u, 6u, 10u, 11u, 12u })
            {
                if (std::abs(data[i]) > 0.001f)
                    return false;
            }
            if (std::abs(data[13] - 1.0f) > 0.001f)
                return false;
            auto factor = RenderScale(*reinterpret_cast<uint32_t*>(object + 0x24), language);
            if (!std::isfinite(data[14]) || !std::isfinite(data[15]) || data[14] <= 0.0f || data[15] <= 0.0f)
                return false;
            frame = { matrix[12], matrix[13], matrix[14], matrix[0], matrix[5],
                matrix[0] / (data[14] * factor), matrix[5] / (data[15] * factor) };
            frame.originX = frame.x - data[7] * frame.parentX;
            frame.originY = frame.y - data[8] * frame.parentY;
            auto justify = *reinterpret_cast<uint32_t*>(object + 0x6C);
            auto offset = justify & 4 ? -font.height * 0.5f : justify & 8 ? -font.height : 0.0f;
            frame.top = frame.y + (font.origin + font.glyphs.top + offset) * frame.scaleY;
            frame.bottom = frame.y + (font.origin + font.glyphs.bottom + offset) * frame.scaleY;
            return frame.parentX > 0.0f && frame.parentY > 0.0f;
        }

        static float* State(uint8_t* object)
        {
            if (!object || *reinterpret_cast<uint32_t*>(object + 0x30) < 68)
                return nullptr;
            auto state = *reinterpret_cast<float**>(object + 0x2C);
            if (!state)
                return nullptr;
            for (size_t i = 4; i < 17; ++i)
            {
                if (!std::isfinite(state[i]))
                    return nullptr;
            }
            for (size_t i : { 4u, 5u, 6u, 10u, 11u, 12u })
            {
                if (std::abs(state[i]) > 0.001f)
                    return nullptr;
            }
            return std::abs(state[13] - 1.0f) <= 0.001f && state[14] > 0.0f && state[15] > 0.0f ? state : nullptr;
        }

        template<class Visit>
        static bool Geometry(List* list, float x, float y, float sx, float sy, Visit&& visit, size_t depth, size_t& remaining)
        {
            if (depth >= 32)
                return false;
            for (auto object = list->head; object; object = *reinterpret_cast<uint8_t**>(object + 4))
            {
                if (!remaining--)
                    return false;
                if ((*reinterpret_cast<uint32_t*>(object + 0x1C) & 0x41) != 0)
                    continue;
                auto state = *reinterpret_cast<uint32_t**>(object + 0x2C);
                if (state && *reinterpret_cast<uint32_t*>(object + 0x30) >= 68 && !state[3])
                    continue;
                if (!visit(object, x, y, sx, sy))
                    return false;
                if (*reinterpret_cast<uint32_t*>(object + 0x18) == 5)
                {
                    auto data = State(object);
                    if (!data || !Geometry(reinterpret_cast<List*>(object + 0x60), x + data[7] * sx, y + data[8] * sy,
                        sx * data[14], sy * data[15], visit, depth + 1, remaining))
                        return false;
                }
            }
            return true;
        }

        std::array<float, 3> TextBounds(uint8_t* object, FontLookup lookup)
        {
            if (!ReadWide || !Advance)
                return { NAN, NAN, NAN };
            auto font = static_cast<uint8_t*>(lookup(*reinterpret_cast<uint32_t*>(object + 0x24)));
            auto header = font ? *reinterpret_cast<uint8_t**>(font + 24) : nullptr;
            if (!header || *reinterpret_cast<uint32_t*>(header) != 0x46746E46)
                return { NAN, NAN, NAN };
            std::array<wchar_t, 1024> text{};
            ReadWide(text.data(), object, int(text.size()));
            text.back() = 0;
            auto count = *reinterpret_cast<uint16_t*>(header + 10);
            auto offset = *reinterpret_cast<uint32_t*>(header + 20);
            if (!count || offset > 0x1000000)
                return { NAN, NAN, NAN };
            auto stride = *reinterpret_cast<uint32_t*>(header + 12) & 0x40000 ? 16 : 12;
            float left = std::numeric_limits<float>::max();
            float right = std::numeric_limits<float>::lowest();
            double pen = 0.0;
            uint16_t previous = 0;
            for (auto character : text)
            {
                if (!character)
                    break;
                auto code = uint16_t(character);
                auto glyphCode = code >= 0xFF80 ? uint16_t(uint8_t(code)) : code;
                glyphCode = glyphCode == 0x99 ? 0x2122 : glyphCode == 0x9C ? 0x153 : glyphCode == 0xA0 ? ' ' : glyphCode;
                size_t first = 0, last = count;
                uint8_t* glyph = nullptr;
                while (first < last)
                {
                    auto middle = first + (last - first) / 2;
                    auto candidate = header + offset + middle * stride;
                    auto value = *reinterpret_cast<uint16_t*>(candidate);
                    if (value == glyphCode)
                    {
                        glyph = candidate;
                        break;
                    }
                    if (value < glyphCode)
                        first = middle + 1;
                    else
                        last = middle;
                }
                auto advance = Advance(font, code, previous, 0);
                auto kerning = advance - Advance(font, code, 0, 0);
                if (glyph && glyph[3])
                {
                    auto start = float(pen + kerning) + float(*reinterpret_cast<int8_t*>(glyph + 9));
                    left = std::min(left, start);
                    right = std::max(right, start + std::max(4.0f, float(glyph[2])));
                }
                pen += advance;
                previous = code;
            }
            if (left == std::numeric_limits<float>::max())
                return { 0.0f, 0.0f, float(pen) };
            auto justify = *reinterpret_cast<uint32_t*>(object + 0x6C) & 3;
            auto align = justify == 1 ? float(pen * 0.5) : justify == 2 ? float(pen) : 0.0f;
            return { left - align, right - align, float(pen) };
        }

        float WidthScale(uint8_t* object, FontLookup lookup)
        {
            auto maximum = *reinterpret_cast<uint32_t*>(object + 0x74);
            if (!maximum || (*reinterpret_cast<uint32_t*>(object + 0x6C) & 0x10) != 0)
                return 1.0f;
            auto width = TextBounds(object, lookup)[2];
            if (!std::isfinite(width))
                return NAN;
            return width > float(maximum) ? float(maximum) / width : 1.0f;
        }

        bool FooterSpace(uint8_t* package, uint8_t* selected, const Frame& frame, FontLookup lookup, int language,
            float top, float bottom, float& left, float& right)
        {
            for (auto hash : { 0x17B1F254u, 0x499BD66Cu })
            {
                auto root = Find(reinterpret_cast<List*>(package + 0x48), hash);
                if (!State(root) || *reinterpret_cast<uint32_t*>(root + 0x18) != 5)
                    continue;
                std::array<float, 4> relative{};
                bool found = false;
                auto locate = [&](uint8_t* object, float x, float y, float sx, float sy)
                {
                    if (object == selected)
                    {
                        relative = { x, y, sx, sy };
                        found = true;
                    }
                    return true;
                };
                size_t remaining = 4096;
                if (!Geometry(reinterpret_cast<List*>(root + 0x60), 0.0f, 0.0f, 1.0f, 1.0f, locate, 0, remaining) || !found ||
                    relative[2] <= 0.0f || relative[3] <= 0.0f)
                    continue;
                auto shrink = WidthScale(selected, lookup);
                if (!std::isfinite(shrink) || shrink <= 0.0f)
                    return false;
                auto rootScaleX = frame.parentX / (relative[2] * shrink);
                auto rootScaleY = frame.parentY / relative[3];
                auto data = *reinterpret_cast<float**>(selected + 0x2C);
                auto originX = frame.x - data[7] * frame.parentX / shrink;
                auto rootX = originX - relative[0] * rootScaleX;
                auto rootY = frame.originY - relative[1] * rootScaleY;
                std::vector<std::array<float, 2>> occupied;
                auto bounds = [&](uint8_t* object, float x, float y, float sx, float sy)
                {
                    auto data = *reinterpret_cast<float**>(object + 0x2C);
                    if (!data || *reinterpret_cast<uint32_t*>(object + 0x30) < 68)
                        return false;
                    auto type = *reinterpret_cast<uint32_t*>(object + 0x18);
                    if (type == 5)
                        return true;
                    float x0, x1, y0, y1;
                    auto originX = rootX + (x + data[7] * sx) * rootScaleX;
                    auto originY = rootY + (y + data[8] * sy) * rootScaleY;
                    Font font;
                    if (GetFont(object, lookup, font))
                    {
                        auto horizontal = TextBounds(object, lookup);
                        if (!std::isfinite(horizontal[0]) || !std::isfinite(horizontal[1]))
                            return false;
                        if (horizontal[0] == horizontal[1])
                            return true;
                        auto factor = RenderScale(*reinterpret_cast<uint32_t*>(object + 0x24), language);
                        auto justify = *reinterpret_cast<uint32_t*>(object + 0x6C);
                        auto offset = justify & 4 ? -font.height * 0.5f : justify & 8 ? -font.height : 0.0f;
                        x0 = originX + horizontal[0] * data[14] * sx * rootScaleX * factor;
                        x1 = originX + horizontal[1] * data[14] * sx * rootScaleX * factor;
                        y0 = originY + (font.origin + font.glyphs.top + offset) * data[15] * sy * rootScaleY * factor;
                        y1 = originY + (font.origin + font.glyphs.bottom + offset) * data[15] * sy * rootScaleY * factor;
                    }
                    else if ((type == 1 || type == 9) && *reinterpret_cast<uint32_t*>(object + 0x30) >= 84)
                    {
                        auto width = data[14] * data[19];
                        auto height = data[15] * data[20];
                        if (width == 0.0f || height == 0.0f)
                            return true;
                        for (size_t i : { 4u, 5u, 6u, 10u, 11u })
                        {
                            if (!std::isfinite(data[i]) || std::abs(data[i]) > 0.001f)
                                return false;
                        }
                        auto z = data[12];
                        auto w = data[13];
                        if (!std::isfinite(z) || !std::isfinite(w) || std::abs(z * z + w * w - 1.0f) > 0.001f)
                            return false;
                        for (size_t i : { 16u, 17u, 18u })
                        {
                            if (!std::isfinite(data[i]) || std::abs(data[i]) > 0.001f)
                                return false;
                        }
                        auto cosine = 1.0f - 2.0f * z * z;
                        auto sine = 2.0f * z * w;
                        auto boxWidth = (std::abs(cosine * width) + std::abs(sine * height)) * sx * rootScaleX;
                        auto boxHeight = (std::abs(sine * width) + std::abs(cosine * height)) * sy * rootScaleY;
                        x0 = originX - boxWidth * 0.5f;
                        x1 = originX + boxWidth * 0.5f;
                        y0 = originY - boxHeight * 0.5f;
                        y1 = originY + boxHeight * 0.5f;
                    }
                    else
                        return false;
                    if (!std::isfinite(x0) || !std::isfinite(x1) || !std::isfinite(y0) || !std::isfinite(y1))
                        return false;
                    if (y1 > top && y0 < bottom && x1 > 12.0f && x0 < 628.0f)
                        occupied.push_back({ std::max(12.0f, x0 - 8.0f), std::min(628.0f, x1 + 8.0f) });
                    return true;
                };
                auto anchor = State(root);
                auto parentScaleX = rootScaleX / anchor[14];
                auto parentScaleY = rootScaleY / anchor[15];
                auto parentX = rootX - anchor[7] * parentScaleX;
                auto parentY = rootY - anchor[8] * parentScaleY;
                auto parent = *reinterpret_cast<uint16_t*>(root + 0x20);
                for (auto otherHash : { 0x17B1F254u, 0x499BD66Cu })
                {
                    auto other = Find(reinterpret_cast<List*>(package + 0x48), otherHash);
                    if (!other || (*reinterpret_cast<uint32_t*>(other + 0x1C) & 0x41) != 0)
                        continue;
                    auto state = State(other);
                    if (!state || *reinterpret_cast<uint16_t*>(other + 0x20) != parent)
                        return false;
                    rootScaleX = parentScaleX * state[14];
                    rootScaleY = parentScaleY * state[15];
                    rootX = parentX + state[7] * parentScaleX;
                    rootY = parentY + state[8] * parentScaleY;
                    remaining = 4096;
                    if (!Geometry(reinterpret_cast<List*>(other + 0x60), 0.0f, 0.0f, 1.0f, 1.0f, bounds, 0, remaining))
                        return false;
                }
                std::sort(occupied.begin(), occupied.end());
                float cursor = 12.0f;
                left = right = 12.0f;
                for (auto span : occupied)
                {
                    if (span[0] - cursor > right - left)
                    {
                        left = cursor;
                        right = span[0];
                    }
                    cursor = std::max(cursor, span[1]);
                }
                if (628.0f - cursor > right - left)
                {
                    left = cursor;
                    right = 628.0f;
                }
                return right - left >= 64.0f;
            }
            return false;
        }

        bool Place(uint8_t* package, uint8_t* selected, uint8_t* rowGroup, uint8_t* lastGroup,
            FontLookup lookup, int language, const std::function<uint32_t(uint8_t*)>& maximum,
            Layout& layout, uint8_t*& style)
        {
            size_t visibleRemaining = 4096;
            if (!Opacity(reinterpret_cast<List*>(package + 0x48), selected, 256, 0, visibleRemaining))
                return false;
            Frame selectedFrame;
            if (!GetFrame(selected, lookup, language, selectedFrame) || !rowGroup || !lastGroup ||
                *reinterpret_cast<uint32_t*>(rowGroup + 0x18) != 5 || *reinterpret_cast<uint32_t*>(lastGroup + 0x18) != 5 ||
                *reinterpret_cast<uint16_t*>(rowGroup + 0x20) != *reinterpret_cast<uint16_t*>(lastGroup + 0x20))
                return false;
            auto row = State(rowGroup);
            auto last = State(lastGroup);
            if (!row || !last)
                return false;
            auto parentX = selectedFrame.parentX / row[14];
            auto parentY = selectedFrame.parentY / row[15];
            auto originX = selectedFrame.originX - row[7] * parentX;
            auto originY = selectedFrame.originY - row[8] * parentY;
            auto groupX = originX + last[7] * parentX;
            auto groupY = originY + last[8] * parentY;
            auto scaleX = parentX * last[14];
            auto scaleY = parentY * last[15];
            float left = std::numeric_limits<float>::max();
            float right = std::numeric_limits<float>::lowest();
            float bottom = std::numeric_limits<float>::lowest();
            size_t remaining = 128;
            auto children = reinterpret_cast<List*>(lastGroup + 0x60);
            for (auto child = children->head; child && remaining; child = *reinterpret_cast<uint8_t**>(child + 4), --remaining)
            {
                auto data = State(child);
                if (!data)
                    continue;
                auto x = groupX + data[7] * scaleX;
                auto y = groupY + data[8] * scaleY;
                Font font;
                if (GetFont(child, lookup, font))
                {
                    auto factor = RenderScale(*reinterpret_cast<uint32_t*>(child + 0x24), language);
                    auto justify = *reinterpret_cast<uint32_t*>(child + 0x6C);
                    auto offset = justify & 4 ? -font.height * 0.5f : justify & 8 ? -font.height : 0.0f;
                    bottom = std::max(bottom, y + (font.origin + font.glyphs.bottom + offset) * data[15] * scaleY * factor);
                    auto width = float(maximum(child)) * data[14] * scaleX * factor;
                    if (std::isfinite(width) && width > 0.0f)
                    {
                        auto start = x - ((justify & 3) == 1 ? width * 0.5f : (justify & 3) == 2 ? width : 0.0f);
                        left = std::min(left, start);
                        right = std::max(right, start + width);
                    }
                }
                else if (*reinterpret_cast<uint32_t*>(child + 0x18) == 1 && *reinterpret_cast<uint32_t*>(child + 0x30) >= 84)
                {
                    auto width = data[14] * data[19] * scaleX;
                    auto height = data[15] * data[20] * scaleY;
                    if (std::isfinite(width) && std::isfinite(height) && width > 0.0f && height > 0.0f)
                    {
                        left = std::min(left, x - width * 0.5f);
                        right = std::max(right, x + width * 0.5f);
                        bottom = std::max(bottom, y + height * 0.5f);
                    }
                }
            }
            left = std::max(12.0f, left);
            right = std::min(628.0f, right);
            if (!remaining || !std::isfinite(bottom) || !std::isfinite(left) || !std::isfinite(right) ||
                !std::isfinite(scaleX) || !std::isfinite(scaleY) || scaleX <= 0.0f || scaleY <= 0.0f || right - left < 64.0f)
                return false;
            float legendTop = 472.0f;
            bool legend = false;
            Frame styleFrame;
            auto chooseStyle = [&](uint8_t* object)
            {
                if (!String(object) || *reinterpret_cast<uint32_t*>(object + 0x10) == Name(0) ||
                    *reinterpret_cast<uint32_t*>(object + 0x10) == Name(1))
                    return;
                Frame current;
                if (GetFrame(object, lookup, language, current) && current.top > bottom && current.top < legendTop)
                {
                    legend = true;
                    legendTop = current.top;
                    style = object;
                    styleFrame = current;
                }
            };
            for (auto hash : { 0x17B1F254u, 0x499BD66Cu })
            {
                auto root = Find(reinterpret_cast<List*>(package + 0x48), hash);
                size_t remaining = 4096;
                if (!root || *reinterpret_cast<uint32_t*>(root + 0x18) != 5 ||
                    !Opacity(reinterpret_cast<List*>(package + 0x48), root, 256, 0, remaining))
                    continue;
                Walk(reinterpret_cast<List*>(root + 0x60), chooseStyle);
            }
            Font font;
            if (!legend || !GetFont(style, lookup, font))
                return false;
            auto factor = RenderScale(*reinterpret_cast<uint32_t*>(style + 0x24), language);
            auto captionScaleX = styleFrame.scaleX / (factor * WidthScale(style, lookup));
            auto captionScaleY = styleFrame.scaleY / factor;
            auto textHeight = (font.glyphs.bottom - font.glyphs.top) * captionScaleY * factor;
            if (!std::isfinite(textHeight) || textHeight <= 0.0f || !std::isfinite(captionScaleX) || captionScaleX <= 0.0f)
                return false;
            auto top = bottom + 2.0f;
            auto pitch = textHeight + 2.0f;
            size_t lines = 0;
            auto panelLeft = left;
            auto panelRight = right;
            for (size_t count : { 2u, 1u })
            {
                auto end = top + float(count - 1) * pitch + textHeight;
                if (end <= 476.0f && FooterSpace(package, style, styleFrame, lookup, language, top, end, left, right))
                {
                    // Retain the panel alignment when its complete width is clear of the legend artwork.
                    if (panelLeft >= left && panelRight <= right)
                    {
                        left = panelLeft;
                        right = panelRight;
                    }
                    lines = count;
                    break;
                }
            }
            if (!lines)
                return false;
            layout = { left - 320.0f, top - 240.0f - (font.origin + font.glyphs.top) * captionScaleY * factor,
                selectedFrame.z, right - left, pitch, captionScaleX, captionScaleY, lines, *reinterpret_cast<uint32_t*>(style + 0x24) };
            // Keyboard key legends use black ink; help uses the menu's primary caption colour.
            visibleRemaining = 4096;
            auto alpha = Opacity(reinterpret_cast<List*>(package + 0x48), style, 256, 0, visibleRemaining);
            if (!alpha)
                return false;
            layout.color = { 255, 255, 255, alpha };
            return true;
        }

    public:
        bool HasPackages() const
        {
            return !packages.empty();
        }

        void ResetMetrics()
        {
            bounds.clear();
        }

        std::vector<uint8_t*> Prepare(uint8_t* package, const std::string& name, uint8_t* selected,
            uint8_t* rowGroup, uint8_t* lastGroup, FontLookup lookup, int language, float& width,
            const std::function<uint32_t(uint8_t*)>& maximum, const std::function<bool()>& current = {})
        {
            Layout layout;
            uint8_t* style{};
            if ((current && !current()) || !package || !Place(package, selected, rowGroup, lastGroup, lookup, language, maximum, layout, style))
                return {};
            auto list = reinterpret_cast<List*>(package + 0x48);
            auto& entry = packages[name];
            std::array<uint8_t*, 2> objects{};
            bool missing = false;
            for (size_t i = 0; i < objects.size(); ++i)
            {
                objects[i] = Find(list, Name(i));
                if (objects[i] && (!String(objects[i]) || !entry.ids[i] || *reinterpret_cast<uint32_t*>(objects[i] + 0x0C) != entry.ids[i]))
                    return {};
                missing |= !objects[i];
            }
            if (missing)
            {
                if (objects[0] || objects[1])
                    return {};
                for (size_t i = 0; i < objects.size(); ++i)
                {
                    objects[i] = NFSPSStaticText::Clone(style);
                    if ((current && !current()) || !objects[i] || !*reinterpret_cast<uint32_t*>(objects[i] + 0x0C))
                    {
                        for (auto object : objects)
                            NFSPSStaticText::Destroy(object);
                        return {};
                    }
                    *reinterpret_cast<uint32_t*>(objects[i] + 0x10) = Name(i);
                    *reinterpret_cast<uint32_t*>(objects[i] + 0x1C) = 0x2400001;
                }
                for (size_t i = 0; i < objects.size(); ++i)
                {
                    auto object = objects[i];
                    entry.ids[i] = *reinterpret_cast<uint32_t*>(object + 0x0C);
                    *reinterpret_cast<uint8_t**>(object + 4) = nullptr;
                    *reinterpret_cast<uint8_t**>(object + 8) = list->tail;
                    if (list->tail)
                        *reinterpret_cast<uint8_t**>(list->tail + 4) = object;
                    else
                        list->head = object;
                    list->tail = object;
                    ++list->count;
                }
            }
            for (size_t i = 0; i < objects.size(); ++i)
            {
                auto object = objects[i];
                auto data = *reinterpret_cast<uint8_t**>(object + 0x2C);
                alignas(float) std::array<uint8_t, 68> next{};
                auto floats = reinterpret_cast<float*>(next.data());
                std::copy(layout.color.begin(), layout.color.end(), reinterpret_cast<uint32_t*>(next.data()));
                floats[7] = layout.x;
                floats[8] = layout.y + float(i) * layout.pitch;
                floats[9] = layout.z;
                floats[13] = 1.0f;
                floats[14] = layout.scaleX;
                floats[15] = layout.scaleY;
                floats[16] = 1.0f;
                bool changed = std::memcmp(data, next.data(), next.size()) != 0 ||
                    *reinterpret_cast<uint32_t*>(object + 0x24) != layout.font;
                std::memcpy(data, next.data(), next.size());
                *reinterpret_cast<uint32_t*>(object + 0x24) = layout.font;
                *reinterpret_cast<uint16_t*>(object + 0x20) = 0;
                // The native glyph loop also reads this literal-control bit for character advances.
                *reinterpret_cast<uint32_t*>(object + 0x6C) = 0x20;
                *reinterpret_cast<uint32_t*>(object + 0x70) = 0;
                *reinterpret_cast<uint32_t*>(object + 0x74) = 0;
                if (changed)
                    *reinterpret_cast<uint32_t*>(object + 0x1C) |= 0x2400000;
                if (i >= layout.lines)
                    *reinterpret_cast<uint32_t*>(object + 0x1C) |= 1;
            }
            width = layout.width;
            return { objects.begin(), objects.begin() + layout.lines };
        }

        void Hide(uint8_t* package, const std::string& name)
        {
            auto entry = packages.find(name);
            if (!package || entry == packages.end())
                return;
            auto list = reinterpret_cast<List*>(package + 0x48);
            for (size_t i = 0; i < entry->second.ids.size(); ++i)
            {
                auto object = Find(list, Name(i));
                if (String(object) && entry->second.ids[i] && *reinterpret_cast<uint32_t*>(object + 0x0C) == entry->second.ids[i])
                    *reinterpret_cast<uint32_t*>(object + 0x1C) |= 0x2400001;
            }
        }
    };
}
