module;

#include <stdafx.h>

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
            CIniReader iniReader("");
            bool bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) != 0;
            static bool bScaling = iniReader.ReadInteger("MAIN", "Scaling", 0);

            if (bFixFOV)
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

                        if (bScaling)
                        {
                            hor3DScale /= 1.0252904893f;
                        }

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
        };
    }
} Rendering;