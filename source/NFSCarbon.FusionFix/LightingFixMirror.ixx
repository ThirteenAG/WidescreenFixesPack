module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module LightingFixMirror;

import ComVars;

// STUB: mirror lighting update code (LightingFixUpdateMirrorCave) authored by REDACTED removed

class LightingFixMirror
{
public:
    LightingFixMirror()
    {
        WFP::onInitEventAsync() += []()
        {
            if (!FixSettings().Bool("GRAPHICS.ConsoleLighting"))
                return;

            // The lighting values of the Xbox 360 version, also for the rear-view mirror
            {
                uint32_t* dword_7497B9 = hook::pattern("8B 0D ? ? ? ? 83 F8 06 89 0D ? ? ? ? C7 05 ? ? ? ? CD CC CC 3E").count(2).get(1).get<uint32_t>(2);
                static float f20 = 2.0f;
                injector::WriteMemory((uint32_t)dword_7497B9, &f20, true);
                static float f19 = 1.9f;
                uint32_t* dword_748A70 = hook::pattern("A1 ? ? ? ? A3 ? ? ? ? 83 3D ? ? ? ? 06 C7 05 ? ? ? ? CD CC CC 3E").get(1).get<uint32_t>(1); // the second of three
                injector::WriteMemory(dword_748A70, &f19, true);
                uint32_t* dword_73E7CB = *hook::pattern("C7 05 ? ? ? ? CD CC CC 3E B8 ? ? ? ? 74 05 B8 ? ? ? ? C3").count(1).get(0).get<uint32_t*>(11);
                injector::WriteMemory<float>(dword_73E7CB, 1.35f, true);

                //Lighting Fix Update (mirror)
                // STUB: removed code authored by REDACTED

            }
        };
    }
} LightingFixMirror;