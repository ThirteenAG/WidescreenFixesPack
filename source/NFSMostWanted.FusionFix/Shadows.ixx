module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Shadows;

import ComVars;
import Resolution;

export namespace ShadowRes
{
    // STUB: shadow resolution / depth bias / shadow map format code authored by REDACTED removed
    uint32_t Resolution = 2048;
    int CurrentTexFormat = 0;

    void update(uint32_t /*res*/, float /*fAspectRatio*/)
    {
        // STUB: removed code authored by REDACTED
    }
}

class Shadows
{
public:
    Shadows()
    {
        WFP::onInitEvent() += []()
        {
            ShadowRes::Resolution = std::clamp(FixSettings().Int("GRAPHICS.ShadowResolution"), 1024, 16384);
            ShadowRes::CurrentTexFormat = FixSettings().Bool("GRAPHICS.SoftShadows") ? 1 : 0;   // INTZ or D24S8

            {
                //dynamic shadow fix that stops them from disappearing when going into tunnels, under bridges, etc.
                uint32_t* dword_6DE377 = hook::pattern("75 3B C7 05 ? ? ? ? 00 00 80 3F").count(1).get(0).get<uint32_t>(0);
                injector::MakeNOP(dword_6DE377, 2, true);
                injector::WriteMemory((uint32_t)dword_6DE377 + 8, 0, true);

                //car shadow opacity
                uint32_t* dword_8A0E50 = *hook::pattern("D9 05 ? ? ? ? 8B 54 24 70 D9 1A E9 D1").count(1).get(0).get<uint32_t*>(2);
                injector::WriteMemory(dword_8A0E50, 60.0f, true);

            }

            {
                // STUB: shadow depth bias and shadow resolution hooks authored by REDACTED removed

                onResChange() += [](int Width, int Height)
                {
                    float fAspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
                    ShadowRes::update(ShadowRes::Resolution, fAspectRatio);
                };

            }

            // STUB: shadow map texture format code authored by REDACTED removed

            {
                uint32_t* dword_6E5174 = hook::pattern("68 ? ? ? ? EB ? A1 ? ? ? ? 0D").count(1).get(0).get<uint32_t>(1);
                injector::WriteMemory(dword_6E5174, 0x00000000, true);
                uint32_t* dword_6BFFA2 = hook::pattern("68 ? ? ? ? 50 41 68").count(2).get(1).get<uint32_t>(1);
                injector::WriteMemory(dword_6BFFA2, 0x00006102, true);

            }
        };
    }
} Shadows;