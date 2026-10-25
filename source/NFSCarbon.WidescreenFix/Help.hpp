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

namespace NFSCHelp
{
    using FontLookup = void* (__cdecl*)(uint32_t);

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
            return Hash(line ? "WFPC_HELP_1" : "WFPC_HELP_0");
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

        template<class Visit>
        static void Walk(List* list, Visit&& visit)
        {
            for (auto object = list->head; object; object = *reinterpret_cast<uint8_t**>(object + 4))
            {
                if ((*reinterpret_cast<uint32_t*>(object + 0x1C) & 0x41) != 0)
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
            auto factor = NFSCStaticText::RenderScale(*reinterpret_cast<uint32_t*>(object + 0x24), language);
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

        bool Place(void* screen, uint8_t* package, uint8_t* selected, FontLookup lookup, int language, Layout& layout, uint8_t*& style)
        {
            Frame selectedFrame;
            if (!GetFrame(selected, lookup, language, selectedFrame))
                return false;
            auto bytes = static_cast<uint8_t*>(screen);
            auto option = *reinterpret_cast<uint8_t**>(bytes + 0x34);
            if (!option || *reinterpret_cast<int*>(option + 60) != 3)
                return false;
            auto group = *reinterpret_cast<uint8_t**>(option + 16);
            auto widget = *reinterpret_cast<uint8_t**>(option + 72);
            auto label = widget ? *reinterpret_cast<uint8_t**>(widget + 24) : nullptr;
            auto master = *reinterpret_cast<uint8_t**>(bytes + 0x11C);
            auto rowState = State(group);
            auto labelState = State(label);
            if (!rowState || !labelState || !master ||
                *reinterpret_cast<uint16_t*>(master + 0x20) != *reinterpret_cast<uint16_t*>(group + 0x20))
                return false;
            auto parentX = selectedFrame.parentX / (rowState[14] * labelState[14]);
            auto parentY = selectedFrame.parentY / (rowState[15] * labelState[15]);
            auto originX = selectedFrame.originX - (rowState[7] + labelState[7] * rowState[14]) * parentX;
            auto originY = selectedFrame.originY - (rowState[8] + labelState[8] * rowState[15]) * parentY;
            auto width = *reinterpret_cast<float*>(bytes + 0xEC);
            auto height = *reinterpret_cast<float*>(bytes + 0xF0);
            auto centerX = *reinterpret_cast<float*>(bytes + 0xF4);
            auto centerY = *reinterpret_cast<float*>(bytes + 0xF8);
            auto visible = *reinterpret_cast<uint32_t*>(bytes + 0x54);
            if (!visible || visible > 64 || !std::isfinite(width) || !std::isfinite(height) ||
                !std::isfinite(centerX) || !std::isfinite(centerY) || width <= 0.0f || height <= 0.0f ||
                !std::isfinite(parentX) || !std::isfinite(parentY) || parentX <= 0.0f || parentY <= 0.0f)
                return false;
            // Native scroller dimensions already reserve every stock slot, including empty ones.
            auto bottom = originY + (centerY + height * 0.5f) * parentY;
            auto left = std::max(12.0f, originX + (centerX - width * 0.5f) * parentX);
            auto right = std::min(628.0f, originX + (centerX + width * 0.5f) * parentX);
            if (!std::isfinite(bottom) || right - left < 64.0f)
                return false;
            bool stable = true;
            auto end = reinterpret_cast<uintptr_t>(screen) + 0x2C;
            size_t count{};
            for (auto link = *reinterpret_cast<uintptr_t*>(end); link && link != end; link = *reinterpret_cast<uintptr_t*>(link))
            {
                if (++count > 4096)
                    return false;
                auto row = reinterpret_cast<uint8_t*>(link - 4);
                auto object = *reinterpret_cast<uint8_t**>(row + 16);
                if (!object || (*reinterpret_cast<uint32_t*>(object + 0x1C) & 0x41) != 0)
                    continue;
                auto visit = [&](uint8_t* leaf)
                {
                    if (!String(leaf))
                        return;
                    Frame current;
                    if (!GetFrame(leaf, lookup, language, current))
                        stable = false;
                    else
                        bottom = std::max(bottom, current.bottom);
                };
                visit(object);
                if (*reinterpret_cast<uint32_t*>(object + 0x18) == 5)
                    Walk(reinterpret_cast<List*>(object + 0x60), visit);
            }
            if (!stable)
                return false;
            float limit = 468.0f;
            float legendTop = 472.0f;
            bool legend{};
            Frame styleFrame;
            Walk(reinterpret_cast<List*>(package + 0x48), [&](uint8_t* object)
            {
                if (!String(object) || *reinterpret_cast<uint32_t*>(object + 0x10) == Name(0) ||
                    *reinterpret_cast<uint32_t*>(object + 0x10) == Name(1))
                    return;
                Frame current;
                if (GetFrame(object, lookup, language, current) && current.top > bottom && current.top < legendTop)
                {
                    legend = true;
                    legendTop = current.top;
                    limit = legendTop - 4.0f;
                    style = object;
                    styleFrame = current;
                }
            });
            Font font;
            if (!legend || !GetFont(style, lookup, font))
                return false;
            auto factor = NFSCStaticText::RenderScale(*reinterpret_cast<uint32_t*>(style + 0x24), language);
            auto scaleX = styleFrame.scaleX / factor;
            auto scaleY = styleFrame.scaleY / factor;
            auto textHeight = (font.glyphs.bottom - font.glyphs.top) * scaleY * factor;
            if (!std::isfinite(textHeight) || textHeight <= 0.0f || !std::isfinite(scaleX) || scaleX <= 0.0f)
                return false;
            auto top = bottom + 2.0f;
            auto linePitch = textHeight + 2.0f;
            size_t lines{};
            while (lines < 2 && top + float(lines) * linePitch + textHeight <= limit)
                ++lines;
            if (!lines)
                return false;
            layout = { left - 320.0f, top - 240.0f - (font.origin + font.glyphs.top) * scaleY * factor,
                selectedFrame.z, right - left, linePitch, scaleX, scaleY, lines, *reinterpret_cast<uint32_t*>(style + 0x24) };
            auto color = *reinterpret_cast<uint32_t**>(style + 0x2C);
            std::copy_n(color, 4, layout.color.begin());
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

        std::vector<uint8_t*> Prepare(void* screen, uint8_t* package, const std::string& name,
            uint8_t* selected, FontLookup lookup, int language, float& width, const std::function<bool()>& current = {})
        {
            Layout layout;
            uint8_t* style{};
            if ((current && !current()) || !package || !Place(screen, package, selected, lookup, language, layout, style))
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
                    objects[i] = NFSCStaticText::Clone(style);
                    if ((current && !current()) || !objects[i] || !*reinterpret_cast<uint32_t*>(objects[i] + 0x0C))
                    {
                        for (auto object : objects)
                            NFSCStaticText::Destroy(object);
                        return {};
                    }
                    *reinterpret_cast<uint32_t*>(objects[i] + 0x10) = Name(i);
                    *reinterpret_cast<uint32_t*>(objects[i] + 0x1C) = 0x2400821;
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
