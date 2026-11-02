#pragma once
#include <algorithm>
#include <vector>

namespace GodfatherResolutions
{
    struct Mode
    {
        int native = 0;
        int ordinal = 0;
        int width = 0, height = 0, depth = 0;
        unsigned flags = 0;
        int refresh = 0;
    };

    inline std::vector<Mode> Normalize(std::vector<Mode> modes)
    {
        std::erase_if(modes, [](const Mode& mode)
        {
            return mode.width <= 0 || mode.height <= 0 || mode.width > 16384 || mode.height > 16384 || mode.depth != 32;
        });
        std::stable_sort(modes.begin(), modes.end(), [](const Mode& left, const Mode& right)
        {
            if (left.width != right.width)
                return left.width < right.width;
            if (left.height != right.height)
                return left.height < right.height;
            if (left.refresh != right.refresh)
                return left.refresh > right.refresh;
            return left.ordinal < right.ordinal;
        });
        modes.erase(std::unique(modes.begin(), modes.end(), [](const Mode& left, const Mode& right)
        {
            return left.width == right.width && left.height == right.height;
        }), modes.end());
        return modes;
    }
}
