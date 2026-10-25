#pragma once
#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include "NativeMenuContext.hpp"

namespace THPS3Menu
{
    struct Caption
    {
        std::string label, suffix, rendered;
        void* font = nullptr;
        void* render = nullptr;
        float width = 0.0f, height = 0.0f, spacing = 0.0f, border = 0.0f, nativeHeight = 0.0f;
    };
    inline std::map<void*, Caption> captions;

    // Measure the native glyph layout at a readable height. Only the label is
    // shortened; a value which cannot fit by itself refuses publication.
    template<class Measure> bool FitCaption(const std::string& label, const std::string& suffix,
        float width, float nativeHeight, Measure measure, std::string& text, float& height)
    {
        if (!std::isfinite(width) || width <= 0.0f || !std::isfinite(nativeHeight) || nativeHeight <= 0.0f) return false;
        const auto minimum = std::min(nativeHeight, 14.0f);
        text = label + suffix;
        height = nativeHeight;
        auto measured = measure(text, height);
        if (!std::isfinite(measured) || measured < 0.0f) return false;
        if (measured <= width) return true;
        height = std::max(minimum, nativeHeight * width / measured);
        measured = measure(text, height);
        if (!std::isfinite(measured) || measured < 0.0f) return false;
        if (measured <= width) return true;
        height = minimum;
        auto fits = [&](const std::string& candidate)
        {
            auto value = measure(candidate, height);
            return std::isfinite(value) && value >= 0.0f && value <= width;
        };
        if (!fits("..." + suffix)) return false;
        size_t low = 0, high = label.size();
        while (low < high)
        {
            auto middle = low + (high - low + 1) / 2;
            if (fits(label.substr(0, middle) + "..." + suffix)) low = middle;
            else high = middle - 1;
        }
        text = label.substr(0, low) + "..." + suffix;
        return fits(text);
    }

    inline bool Fit(void* node, float limit = 420.0f)
    {
        auto found = captions.find(node);
        if (found == captions.end() || !Object(node, 0x58F074, 476) || !Read<uint8_t>(node, 92) || Read<uint8_t>(node, 475)) return false;
        reinterpret_cast<void(__thiscall*)(void*)>(0x4D4720)(node);
        auto render = Read<void*>(node, 448);
        auto drawer = Read<void*>(node, 452);
        if (!Readable(render, 84) || !Readable(drawer, 28)) return false;
        auto font = Read<void*>(render, 4);
        if (!Readable(font, 280)) return false;
        auto denominator = Read<float>(font, 272);
        auto nativeHeight = denominator * Read<float>(drawer, 8);
        auto spacing = Read<float>(node, 464), border = Read<float>(node, 468);
        if (!std::isfinite(denominator) || denominator <= 0.0f || !std::isfinite(spacing) || spacing < 0.0f
            || !std::isfinite(border) || border < 0.0f) return false;
        auto& caption = found->second;
        if (caption.font == font && caption.render == render && caption.width == limit && caption.spacing == spacing
            && caption.border == border && caption.nativeHeight == nativeHeight && caption.height == Read<float>(node, 80)) return true;
        auto measure = [&](const std::string& text, float height)
        {
            // Native SetText clones the string; no borrowed pointer survives.
            reinterpret_cast<void(__thiscall*)(void*, const char*)>(0x4D47D0)(node, text.c_str());
            reinterpret_cast<void(__thiscall*)(void*)>(0x4D4720)(node);
            reinterpret_cast<void(__thiscall*)(void*, float)>(0x4F8270)(render, height);
            return reinterpret_cast<float(__thiscall*)(void*, int32_t)>(0x4F82A0)(render, -1);
        };
        std::string text;
        float height = 0.0f;
        if (!FitCaption(caption.label, caption.suffix, limit - 2.0f * border, nativeHeight, measure, text, height)) return false;
        // The last binary-search measurement need not be the chosen prefix.
        measure(text, height);
        *reinterpret_cast<float*>(static_cast<uint8_t*>(node) + 80) = height + spacing + 2.0f * border;
        caption.font = font;
        caption.render = render;
        caption.width = limit;
        caption.height = Read<float>(node, 80);
        caption.spacing = spacing;
        caption.border = border;
        caption.nativeHeight = nativeHeight;
        caption.rendered = std::move(text);
        return true;
    }
}
