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
            static int32_t nScaling = iniReader.ReadInteger("MAIN", "Scaling", 1);
            bool bDisableMotionBlur = iniReader.ReadInteger("GRAPHICS", "DisableMotionBlur", 0) != 0;
            static float fRainDropletsScale = iniReader.ReadFloat("GRAPHICS", "RainDropletsScale", 0.5f);
            bool bLightStreaksEnable = iniReader.ReadInteger("GRAPHICS", "LightStreaksEnable", 0) != 0;
            bool bBleachByPassEnable = iniReader.ReadInteger("GRAPHICS", "BleachByPassEnable", 0) != 0;
            static uint32_t ForcedGPUVendor = static_cast<uint32_t>(iniReader.ReadInteger("GRAPHICS", "ForcedGPUVendor", 0x10DE));

            if (bFixFOV)
            {
                static float hor3DScale = 1.0f;
                static float ver3DScale = 1.0f; // don't touch this
                static float flt1 = 0.0f;
                static float flt2 = 0.0f;
                static float flt3 = 0.0f;

                uint32_t* dword_6CF4F0 = hook::pattern("DB 40 18 DA 70 14").count(1).get(0).get<uint32_t>(0);
                uintptr_t loc_6CF4EA = (uintptr_t)dword_6CF4F0 - 6;
                uintptr_t loc_6CF4F6 = (uintptr_t)dword_6CF4F0 + 6;
                struct FOVHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        regs.eax = *(uint32_t*)(regs.edi + 0x60);

                        hor3DScale = 1.0f / (GetAspectRatio() / (4.0f / 3.0f));

                        float mirrorScale = 0.4f;
                        float f1215 = 1.215f;
                        float f043434 = 0.43434f;
                        float f1 = 1.0f; // horizontal for vehicle reflection

                        if (nScaling)
                        {
                            hor3DScale /= 1.047485948f;
                            f1215 = 1.21f;

                            if (nScaling == 2)
                            {
                                f1215 = 1.27f;
                            }
                        }

                        if (regs.ecx == 1 || regs.ecx == 4) //Headlights stretching, reflections etc 
                        {
                            flt1 = hor3DScale;
                            flt2 = f043434;
                            flt3 = f1215;
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
                            _asm {fld ds : mirrorScale}
                        else
                            _asm {fld ds : ver3DScale}
                    }
                }; injector::MakeInline<FOVHook>(loc_6CF4EA, loc_6CF4F6);
                injector::WriteMemory(dword_6CF4F0, 0x9001F983, true); //cmp     ecx, 1

                uint32_t* dword_6CF566 = hook::pattern("D8 3D ? ? ? ? D9 5C 24 20 DB 44 24 30 D8 4C 24 2C").count(1).get(0).get<uint32_t>(2);
                injector::WriteMemory(dword_6CF566, &flt1, true);

                // FOV being different in menus and in-game fix
                uint32_t* dword_6CF578 = hook::pattern("D8 0D ? ? ? ? E8 ? ? ? ? 8B F8 57 E8").count(2).get(1).get<uint32_t>(2);
                injector::WriteMemory(dword_6CF578, &flt2, true);

                uint32_t* dword_6CF5DC = hook::pattern("D8 3D ? ? ? ? D9 44 24 20 D8 64 24 24").count(1).get(0).get<uint32_t>(2);
                injector::WriteMemory(dword_6CF5DC, &flt3, true);

                //Shadow pop-in fix
                uint32_t* dword_6C9653 = hook::pattern("D8 0D ? ? ? ? D9 5C 24 ? E8 ? ? ? ? 8A").count(1).get(0).get<uint32_t>(2);
                static float fShadowDistanceMultiplier = 10.0f;
                injector::WriteMemory((uint32_t)dword_6C9653, &fShadowDistanceMultiplier, true);

                // Shadow camera FOV fix
                uintptr_t loc_6E4652 = reinterpret_cast<uintptr_t>(hook::pattern("8B 46 60 8B 48 18 8B 50 14 89 4C 24 14 DB 44 24 14").get_first(0)) + 9;
                uintptr_t loc_6E4668 = loc_6E4652 + 0x16;

                uintptr_t loc_6E47E4 = reinterpret_cast<uintptr_t>(hook::pattern("8B 40 60 8B 50 18 8B 40 14 89 54 24 14 89 44 24 18 0F B7 8B C4 00 00 00").get_first(0)) + 0x18;
                uintptr_t loc_6E47EC = loc_6E47E4 + 8;

                uintptr_t loc_6E46B0 = reinterpret_cast<uintptr_t>(hook::pattern("D8 7C 24 1C 0F B7 8B C4 00 00 00 89 4C 24 1C").get_first(0)) + 0xF;
                uintptr_t loc_6E4830 = reinterpret_cast<uintptr_t>(hook::pattern("D8 7C 24 1C 0F B7 93 C4 00 00 00 89 54 24 1C").get_first(0)) + 0xF;

                struct ShadowCameraFix1
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        _asm {fld ver3DScale}
                    }
                }; injector::MakeInline<ShadowCameraFix1>(loc_6E4652, loc_6E4652 + 8);
                injector::MakeNOP(loc_6E4668, 4);

                struct ShadowCameraFix2
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        *(uint32_t*)(regs.esp + 0x14) = regs.ecx;
                        _asm {fld ver3DScale}
                    }
                }; injector::MakeInline<ShadowCameraFix2>(loc_6E47E4, loc_6E47E4 + 8);
                injector::MakeNOP(loc_6E47EC, 4);

                injector::WriteMemory(loc_6E46B0 + 2, &hor3DScale, true);
                injector::WriteMemory(loc_6E4830 + 2, &hor3DScale, true);
            }

            static GameRef<float> fAutosculptScaling;
            fAutosculptScaling.SetAddress(*hook::pattern("D8 0D ? ? ? ? DA 74 24 18 E8 ? ? ? ? 89 46 04 EB 03").count(1).get(0).get<float*>(2));
            injector::UnprotectMemory(fAutosculptScaling.get_ptr(), sizeof(float));

            static GameRef<float> fArrestBlur;
            fArrestBlur.SetAddress(*hook::pattern("D8 0D ? ? ? ? 8B 4C 24 18 8B 54 24 1C").count(1).get(0).get<float*>(2));
            injector::UnprotectMemory(fArrestBlur.get_ptr(), sizeof(float));

            onResChange() += [](int Width, int Height)
            {
                float fAspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
                fAutosculptScaling = 480.0f * fAspectRatio;
                fArrestBlur = (1.0f / 640.0f) * ((4.0f / 3.0f) / fAspectRatio);
            };

            //Rain droplets
            auto pattern = hook::pattern("D9 44 24 0C D8 44 24 10 8B 4C 24 08 8B 44 24 10 8B D1");
            struct RainDropletsHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    float fRainScaleX = ((0.75f / GetAspectRatio()) * (4.0f / 3.0f));
                    float esp0C = *(float*)(regs.esp + 0x0C);
                    float esp10 = *(float*)(regs.esp + 0x10);
                    _asm {fld dword ptr[esp0C]}
                    _asm {fmul dword ptr[fRainScaleX]}
                    _asm {fmul dword ptr[fRainDropletsScale]}
                    _asm {fadd dword ptr[esp10]}
                }
            }; injector::MakeInline<RainDropletsHook>(pattern.get_first(0), pattern.get_first(8)); //6D480D

            struct RainDropletsYScaleHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    float esp08 = *(float*)(regs.esp + 0x08);
                    _asm {fmul dword ptr[fRainDropletsScale]}
                    _asm {fadd dword ptr[esp08]}
                    *(uintptr_t*)(regs.esp + 0x38) = regs.ecx;
                }
            }; injector::MakeInline<RainDropletsYScaleHook>(pattern.get_first(30), pattern.get_first(30 + 8)); //6D482B

            if (ForcedGPUVendor)
            {
                uint32_t* dword_93D898 = *hook::pattern("A1 ? ? ? ? 49 3D 02 10 00 00 89 0D").count(1).get(0).get<uint32_t*>(1);

                for (size_t i = 0; i < 20; i++)
                {
                    uint32_t* dword__93D898 = hook::pattern(pattern_str(to_bytes(dword_93D898))).count(1).get(0).get<uint32_t>(0);
                    injector::WriteMemory(dword__93D898, &ForcedGPUVendor, true);
                }
            }

            if (bDisableMotionBlur)
            {
                uint32_t* dword_6DF1D2 = hook::pattern("FF 91 ? ? ? ? 89 1D ? ? ? ? 39 1D ? ? ? ? ? ? 39 1D").count(1).get(0).get<uint32_t>(18);
                injector::WriteMemory<uint8_t>(dword_6DF1D2, 0xEB, true);
            }

            if (bLightStreaksEnable)
            {
                uintptr_t loc_6C1841 = reinterpret_cast<uintptr_t>(hook::pattern("A1 ? ? ? ? 8B 0C 85 ? ? ? ? 85 C9 75 20 85 C0").get_first(0)) - 0x113;
                uintptr_t loc_6C19EC = reinterpret_cast<uintptr_t>(hook::pattern("C7 44 24 10 40 00 00 00 FF 15 ? ? ? ? 39 7C 24 18").get_first(0)) - 0x134;
                uintptr_t loc_6C1AD8 = loc_6C19EC + 0xEC;
                uintptr_t loc_6C310B = reinterpret_cast<uintptr_t>(hook::pattern("99 83 E2 03 03 C2 8B C8 8B C7 99 83 E2 03 03 C2 C1 F8 ? A3").get_first(0)) - 0x20;

                uintptr_t g_LightStreaksEnable = *reinterpret_cast<uintptr_t*>(loc_6C1841 + 1);

                // disable control of the variable
                injector::MakeNOP(loc_6C1841, 5);
                injector::MakeNOP(loc_6C19EC, 6);
                injector::MakeNOP(loc_6C1AD8, 6);
                injector::MakeNOP(loc_6C310B, 6);

                *(uint32_t*)g_LightStreaksEnable = 1;
            }

            if (bBleachByPassEnable)
            {
                uintptr_t loc_6C18D1 = reinterpret_cast<uintptr_t>(hook::pattern("A1 ? ? ? ? 8B 0C 85 ? ? ? ? 85 C9 75 20 85 C0").get_first(0)) - 0x83;
                uintptr_t loc_6C1A9F = reinterpret_cast<uintptr_t>(hook::pattern("C7 44 24 10 40 00 00 00 FF 15 ? ? ? ? 39 7C 24 18").get_first(0)) - 0x81;
                uintptr_t loc_6C1B0E = loc_6C1A9F + 0x6F;
                uintptr_t loc_6C2FC0 = reinterpret_cast<uintptr_t>(hook::pattern("99 83 E2 03 03 C2 8B C8 8B C7 99 83 E2 03 03 C2 C1 F8 ? A3").get_first(0)) - 0x16B;

                uintptr_t g_BleachByPassEnable = *reinterpret_cast<uintptr_t*>(loc_6C18D1 + 1);

                // disable control of the variable
                injector::MakeNOP(loc_6C18D1, 5);
                injector::MakeNOP(loc_6C1A9F, 6);
                injector::MakeNOP(loc_6C1B0E, 6);
                injector::MakeNOP(loc_6C2FC0, 6);

                *(uint32_t*)g_BleachByPassEnable = 1;
            }
        };
    }
} Rendering;