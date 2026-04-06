module;

#include <stdafx.h>

export module Rendering;

import ComVars;



export void InitRendering()
{
    CIniReader iniReader("");
    bool bScaling = iniReader.ReadInteger("MAIN", "Scaling", 0) != 0;
    bool bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) != 0;
    static float fRainDropletsScale = iniReader.ReadFloat("MISC", "RainDropletsScale", 0.5f);

    //Rain droplets
    static float fRainScaleX = ((0.75f / Screen.fAspectRatio) * (4.0f / 3.0f));
    auto pattern = hook::pattern("D9 44 24 0C D8 44 24 10 8B 44 24 08 8B 54 24 10 8B C8");
    struct RainDropletsHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float esp0C = *(float*)(regs.esp + 0x0C);
            float esp10 = *(float*)(regs.esp + 0x10);
            _asm {fld dword ptr[esp0C]}
            _asm {fmul dword ptr[fRainScaleX]}
            _asm {fmul dword ptr[fRainDropletsScale]}
            _asm {fadd dword ptr[esp10]}
        }
    }; injector::MakeInline<RainDropletsHook>(pattern.get_first(0), pattern.get_first(8)); //5C7C35

    struct RainDropletsYScaleHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float esp08 = *(float*)(regs.esp + 0x08);
            _asm {fmul dword ptr[fRainDropletsScale]}
            _asm {fadd dword ptr[esp08]}
            *(uintptr_t*)(regs.esp + 0x38) = regs.eax;
        }
    }; injector::MakeInline<RainDropletsYScaleHook>(pattern.get_first(30), pattern.get_first(30 + 8)); //5C7C53

    if (bFixFOV)
    {
        static float hor3DScale = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
        static float ver3DScale = 1.0f; // don't touch this
        static float mirrorScale = 0.45f;
        static float f122 = 1.22f;
        static float f043511 = 0.43511f;
        static float f1 = 1.0f; // horizontal for vehicle reflection
        static float flt1 = 0.0f;
        static float flt2 = 0.0f;
        static float flt3 = 0.0f;

        if (bScaling)
        {
            hor3DScale /= 1.090909123f;
        }

        uint32_t* dword_5C7F56 = hook::pattern("DB 40 18 C7 44 24 20 00 00 80 3F DA 70 14").count(1).get(0).get<uint32_t>(0);
        struct FOVHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *(uint32_t*)(regs.ebp + 0x58);

                if (regs.ecx == 1 || regs.ecx == 4) //Headlights stretching, reflections etc
                {
                    flt1 = hor3DScale;
                    flt2 = f043511;
                    flt3 = f122;
                }
                else
                {
                    if (regs.ecx > 10)
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

                if (regs.ecx == 3) //if rearview mirror
                    _asm fld ds : mirrorScale
                else
                    _asm fld ds : ver3DScale
            }
        }; injector::MakeInline<FOVHook>((uint32_t)dword_5C7F56 - 6, (uint32_t)dword_5C7F56 + 3);
        injector::WriteMemory((uint32_t)dword_5C7F56 - 1, 0x9001F983, true); //cmp     ecx, 1
        injector::MakeNOP((uint32_t)dword_5C7F56 + 3 + 8, 3, true); //nop fidiv

        uint32_t* dword_5C7FE8 = hook::pattern("D8 3D ? ? ? ? D9 5C 24 1C DB 44 24 2C D8 4C 24 28 D8 0D ? ? ? ? E8").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_5C7FE8, &flt1, true);

        // FOV being different in menus and in-game fix
        uint32_t* dword_5C7FFA = hook::pattern("D8 0D ? ? ? ? E8 ? ? ? ? 8B D8 53").count(2).get(1).get<uint32_t>(2);
        injector::WriteMemory(dword_5C7FFA, &flt2, true);

        uint32_t* dword_5C801F = hook::pattern("D8 3D ? ? ? ? D9 5C 24 38 D9 44 24 24 D8 64 24 30 D8 7C 24 24 D9 5C 24 34").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_5C801F, &flt3, true);
    }
}