#pragma once
#include <cstring>

namespace vcsws
{
    // Button names in help texts for the modern control scheme: the GXT keys C<config><action>
    // name the button of an action; these actions moved to other buttons.
    enum class ModernControlText { Original, Right, Left, Circle };

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
        return ModernControlText::Original;
    }
}
