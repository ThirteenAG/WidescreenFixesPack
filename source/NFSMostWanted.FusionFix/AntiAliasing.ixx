module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module AntiAliasing;

// SMAA itself is drawn by PostFX
export namespace AntiAliasing
{
    bool IsSMAA()
    {
        static const int* pAntiAliasing = &FixSettings().Int("GRAPHICS.AntiAliasing");
        return *pAntiAliasing == 1;
    }
}
