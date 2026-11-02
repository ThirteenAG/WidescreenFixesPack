#pragma once
#include <cstring>

namespace lcsws
{
    enum class ModernControlText { Original, Right, Left, Circle, Cross, RightAndSquare };

    inline ModernControlText ControlText(const char* key, bool active)
    {
        if (!active || !key || key[0] != 'C' || key[1] < '0' || key[1] > '3')
            return ModernControlText::Original;
        const char* action = key + 2;
        if (!std::strcmp(action, "VEACC") || !std::strcmp(action, "PDFW"))
            return ModernControlText::Right;
        if (!std::strcmp(action, "VEBRK") || !std::strcmp(action, "PDLT"))
            return ModernControlText::Left;
        if (!std::strcmp(action, "PDBAK"))
            return ModernControlText::Circle;
        if (!std::strcmp(action, "VEHB"))
            return key[1] < '2' ? ModernControlText::RightAndSquare : ModernControlText::Cross;
        return ModernControlText::Original;
    }
}
