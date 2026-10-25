module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"
#include <d3d9.h>

export module Misc;

import ComVars;

// Start-up and the GPU sync workaround.

namespace
{
    void __fastcall GpuCommandBufferSync(IDirect3DDevice9** device, void* edx)
    {
    }
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();

            // Skip the startup logos: the logo scene (aRevelationsLogo) waits until its logo screen has loaded, then
            // until the logo animation has ended (its state 25) before it goes on to the title (aRevelationsTitle).
            // Without the second wait it goes on as soon as the logo screen has loaded.
            if (settings.Bool("MAIN.SkipIntro"))
            {
                auto pattern = hook::pattern("8B 4E 68 8B 11 8B 82 D4 00 00 00 FF D0 3C 01 75 ? 8B 0D");
                if (PatternUnique(pattern, "ResidentEvilRevelations.FusionFix: Misc"))
                    injector::MakeNOP(pattern.get_first(15), 2, true);
            }

            // GPU command buffer sync: a performance workaround that skips the wait for the GPU at the end of a frame
            if (settings.Bool("MAIN.DisableCreateQuery"))
            {
                auto pattern = hook::pattern("51 8B 81 ? ? ? ? 56 8B 35 ? ? ? ? 68 ? ? ? ? 8D 54 24 08");
                if (PatternAvailable(pattern, "ResidentEvilRevelations.FusionFix: Misc"))
                    injector::MakeJMP(pattern.get_first(), GpuCommandBufferSync, true);
            }
        };
    }
} Misc;
