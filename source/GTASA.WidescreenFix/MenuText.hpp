#pragma once
#include <cmath>
#include <string>

namespace MenuText
{
    template<typename Measure>
    std::string FitLabel(std::string label, float room, Measure&& measure)
    {
        if (!std::isfinite(room) || room <= 0.0f)
            return {};
        if (measure(label) <= room)
            return label;
        const std::string suffix{ "..." };
        if (measure(suffix) > room)
            return {};
        while (!label.empty())
        {
            label.pop_back();
            if (measure(label + suffix) <= room)
                return label + suffix;
        }
        return suffix;
    }
}
