#pragma once
#include "NativeMenuFont.hpp"

namespace FarCryMenu
{
    struct FittedText { std::string text; float size{}; NativeExtent extent{}; };
    inline std::optional<FittedText> FitRow(void* service, HMODULE fontModule,
        std::string label, const std::string& value, float width, float height, float preferred = 20, float minimum = 8)
    {
        if (!NativeLiteral(label) || !NativeLiteral(value) || !std::isfinite(width) ||
            !std::isfinite(height) || width < 64 || height < 8 || preferred < 8 || preferred > 64 || !std::isfinite(minimum) || minimum < 8 || minimum > preferred) return {};
        const auto suffix = value.empty() ? std::string() : ": " + value;
        // Native font size stays readable. Full values are never truncated or
        // converted to replacement glyphs. An unsupported row is unavailable.
        for (float size = preferred; size >= minimum; size -= 1)
        {
            auto suffixExtent = MeasureLiteral(service, fontModule, suffix, size);
            if (!suffixExtent || suffixExtent->x > width || suffixExtent->y > height) continue;
            auto extent = MeasureLiteral(service, fontModule, label + suffix, size);
            if (!extent) return {};
            if (extent->x <= width && extent->y <= height)
                return FittedText{*NativeLiteral(label + suffix), size, *extent};
            std::string shortened = label;
            while (!shortened.empty())
            {
                shortened.pop_back(); // NativeLiteral already proved ASCII.
                const auto combined = shortened + "..." + suffix;
                auto fitted = MeasureLiteral(service, fontModule, combined, size);
                if (!fitted) return {};
                if (fitted->x <= width && fitted->y <= height)
                    return FittedText{*NativeLiteral(combined), size, *fitted};
            }
        }
        return {};
    }
}
