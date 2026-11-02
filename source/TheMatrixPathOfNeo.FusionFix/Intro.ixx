module;

#include <stdafx.h>
#include "FixSettings.hpp"

// Skip intro: the startup logo screens end at once when their movies are missing; the legal screen runs a five second
// timeline, which is moved on to its last moments so it ends the way it always does.

export module Intro;

import Hud;

export namespace NeoIntro
{
    inline bool Skip = true;

    inline void Finish(void* instance, const char* name)
    {
        if (!Skip || !NeoHud::Contains(name, "ip_legal.lou_"))
            return;
        auto data = static_cast<uint8_t*>(instance);
        auto milliseconds = reinterpret_cast<int*>(data + 4);
        auto frames = reinterpret_cast<int*>(data + 12);
        if (*milliseconds < 4700)
        {
            *milliseconds = 4700;
            *frames = 141;
        }
    }
}
