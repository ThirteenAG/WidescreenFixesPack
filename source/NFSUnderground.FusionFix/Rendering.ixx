module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Rendering;

import ComVars;
import Resolution;

class Rendering
{
public:
    Rendering()
    {
        WFP::onInitEvent() += []()
        {
            // FOV Scale in the options, applied from the next frame on
            static const int* pScaling = &FixSettings().Int("GRAPHICS.Scaling");

            {
                static float flt1 = 0.0f;
                static float flt2 = 0.0f;
                static float flt3 = 0.0f;

                auto pattern = hook::pattern("DB 40 18 C7 44 24");
                struct FOVHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        *(float*)(regs.esp + 0x20) = 1.0f;
                        regs.eax = *(uint32_t*)(regs.ebp + 4);

                        float hor3DScale = 1.0f / (GetAspectRatio() / (4.0f / 3.0f));
                        float ver3DScale = 1.0f; // don't touch this
                        float mirrorScale = 0.45f;
                        float f129 = 1.28f;
                        float f04525 = 0.4525f;
                        float f1 = 1.0f; // horizontal for vehicle reflection

                        if (*pScaling)
                            hor3DScale /= 1.0252904893f;

                        if (regs.eax == 1 || regs.eax == 4) //Headlights stretching, reflections etc
                        {
                            flt1 = hor3DScale;
                            flt2 = f04525;
                            flt3 = f129;
                        }
                        else
                        {
                            if (regs.eax > 9)
                            {
                                flt1 = f1;
                                flt2 = 0.5f;
                                flt3 = 1.0f;
                                _asm fld ds : f1
                                return;
                            }
                            else
                            {
                                flt1 = 1.0f;
                                flt2 = 0.5f;
                                flt3 = 1.0f;
                            }
                        }

                        if (regs.eax == 3) //if rearview mirror
                            _asm fld ds : mirrorScale
                        else
                            _asm fld ds : ver3DScale
                    }
                }; injector::MakeInline<FOVHook>(pattern.get_first(0), pattern.get_first(14));

                pattern = hook::pattern("D8 3D ? ? ? ? D9 1F E8 ? ? ? ? D9");
                injector::WriteMemory(pattern.get_first(2), &flt1, true);

                // FOV being different in menus and in-game fix
                pattern = hook::pattern("D8 0D ? ? ? ? ? ? E8 ? ? ? ? DD D8");
                injector::WriteMemory(pattern.get_first(2), &flt2, true);

                pattern = hook::pattern("D8 3D ? ? ? ? D9 E0 D9 5E 54 D9 44 24 20");
                injector::WriteMemory(pattern.get_first(2), &flt3, true);
            }

            // Brake calipers in the front end. CarRenderInfo::Render draws the brakes with their A model in the
            // front end and the B model in races. The PC A calipers sit 2-3 cm further out, inside the back of
            // the rims, so the garage and showroom showed them barely or not at all. The front end uses the
            // race model everywhere: setz cl -> mov cl, 1, and the check that a brake model exists looks at the
            // B model too.
            {
                auto pattern = hook::pattern("0F 94 C1 8D 0C 49 8D BC C8 6C 14 00 00");
                injector::WriteMemory<uint16_t>(pattern.get_first(0), 0x01B1, true);
                injector::WriteMemory<uint8_t>(pattern.get_first(2), 0x90, true);

                pattern = hook::pattern("39 B8 78 14 00 00 0F 84");
                injector::WriteMemory<uint32_t>(pattern.get_first(2), 0x1490, true);
            }
        };
    }
} Rendering;