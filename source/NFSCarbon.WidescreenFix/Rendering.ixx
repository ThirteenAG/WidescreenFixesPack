module;

#include <stdafx.h>

export module Rendering;

import ComVars;

export void InitRendering()
{
    CIniReader iniReader("");
    bool bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) != 0;
    int32_t nScaling = iniReader.ReadInteger("MAIN", "Scaling", 1);
    bool bDisableMotionBlur = iniReader.ReadInteger("GRAPHICS", "DisableMotionBlur", 0) != 0;
    static float fRainDropletsScale = iniReader.ReadFloat("GRAPHICS", "RainDropletsScale", 0.5f);

    if (bFixFOV)
    {
        static float hor3DScale = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
        static float ver3DScale = 1.0f; // don't touch this
        static float mirrorScale = 0.925f;
        static float f1205 = 1.205f;
        static float f0434665 = 0.434665f;
        static float flt1 = 0.0f;
        static float flt2 = 0.0f;
        static float flt3 = 0.0f;

        if (nScaling)
        {
            hor3DScale /= 1.034482718f;
            f1205 = 1.225f;

            if (nScaling == 2)
            {
                f1205 = 1.28f;
            }
        }

        uint32_t* dword_71B858 = hook::pattern("DB 05 ? ? ? ? 8B 45 08 83 F8 01 DA 35 ? ? ? ? D9 5C 24 24").count(1).get(0).get<uint32_t>(0);
        struct FOVHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *(uint32_t*)(regs.ebp + 8);

                if (regs.eax == 1 || regs.eax == 4) //Headlights stretching, reflections etc 
                {
                    flt1 = hor3DScale;
                    flt2 = f0434665;
                    flt3 = f1205;
                }
                else
                {
                    flt1 = 1.0f;
                    flt2 = 0.375f;
                    flt3 = 1.0f;
                }

                if (regs.eax == 3) //if rearview mirror
                    _asm fld ds : mirrorScale
                else
                    _asm fld ds : ver3DScale
            }
        }; injector::MakeInline<FOVHook>((uint32_t)dword_71B858, (uint32_t)dword_71B858 + 18);
        injector::WriteMemory((uint32_t)dword_71B858 + 5, 0x9001F883, true); //cmp     eax, 1

        uint32_t* dword_71B8DA = hook::pattern("D8 3D ? ? ? ? D9 5C 24 30 DB 44 24 20 D8 4C 24 2C").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_71B8DA, &flt1, true);

        // FOV being different in menus and in-game fix
        uint32_t* dword_71B8EC = hook::pattern("D8 0D ? ? ? ? E8 ? ? ? ? 8B E8 55 E8").count(2).get(1).get<uint32_t>(2);
        injector::WriteMemory(dword_71B8EC, &flt2, true);

        uint32_t* dword_71B923 = hook::pattern("D8 3D ? ? ? ? D9 5C 24 2C 8B 54 24 2C 52 50 55").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_71B923, &flt3, true);
    }

    //Rain droplets
    static float fRainScaleX = ((0.75f / Screen.fAspectRatio) * (4.0f / 3.0f));
    auto pattern = hook::pattern("D9 44 24 08 D8 44 24 10 8B 4C 24 0C 8B 44 24 10 8B D1");
    struct RainDropletsHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float esp08 = *(float*)(regs.esp + 0x08);
            float esp10 = *(float*)(regs.esp + 0x10);
            _asm {fld  dword ptr[esp08]}
            _asm {fmul dword ptr[fRainScaleX]}
            _asm {fmul dword ptr[fRainDropletsScale]}
            _asm {fadd dword ptr[esp10]}
        }
    }; injector::MakeInline<RainDropletsHook>(pattern.get_first(0), pattern.get_first(8)); //0x722E78

    struct RainDropletsYScaleHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float esp0C = *(float*)(regs.esp + 0x0C);
            _asm {fmul dword ptr[fRainDropletsScale]}
            _asm {fadd dword ptr[esp0C]}
            *(uintptr_t*)(regs.esp + 0x34) = regs.eax;
        }
    }; injector::MakeInline<RainDropletsYScaleHook>(pattern.get_first(36), pattern.get_first(36 + 8)); //0x722E9C

    if (bDisableMotionBlur)
    {
        uint32_t* dword_71356B = hook::pattern("D9 87 B4 00 00 00 D8 1D ? ? ? ? DF E0 F6 C4 41 75 76").count(1).get(0).get<uint32_t>(17); //0x0071355A
        injector::WriteMemory<uint8_t>(dword_71356B, 0xEB, true);
    }
}