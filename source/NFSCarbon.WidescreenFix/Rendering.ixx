module;

#include <stdafx.h>
#include "../Shared/PatternPreflight.hpp"
#include "FixSettings.hpp"
#include "../Shared/NFS/FieldOfView.hpp"

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
            // Scaling, FOV and DisableMotionBlur are also changed in the game's video options and apply at once
            static const int* pScaling = &FixSettings().Int("GRAPHICS.Scaling");
            static const float* pFovFactor = &FixSettings().Float("MAIN.FOVFactor");
            static const int* pDisableMotionBlur = &FixSettings().Int("GRAPHICS.DisableMotionBlur");
            static const int* pAntiAliasing = &FixSettings().Int("GRAPHICS.AntiAliasing");
            static float fRainDropletsScale = FixSettings().Float("GRAPHICS.RainDropletsScale");

            {
                [&]()
                {
                    if (!PatternPreflight::Check("NFSCarbon Rendering", {
                        { "DB 05 ? ? ? ? 8B 45 08 83 F8 01 DA 35 ? ? ? ? D9 5C 24 24", 1, true },
                        { "D8 3D ? ? ? ? D9 5C 24 30 DB 44 24 20 D8 4C 24 2C", 1, true },
                        { "D8 0D ? ? ? ? E8 ? ? ? ? 8B E8 55 E8", 2, true },
                        { "D8 3D ? ? ? ? D9 5C 24 2C 8B 54 24 2C 52 50 55", 1, true },
                    }))
                        return;

                    static float flt1 = 0.0f;
                    static float flt2 = 0.0f;
                    static float flt3 = 0.0f;

                    uint32_t* dword_71B858 = hook::pattern("DB 05 ? ? ? ? 8B 45 08 83 F8 01 DA 35 ? ? ? ? D9 5C 24 24").count(1).get(0).get<uint32_t>(0);
                    struct FOVHook
                    {
                        void operator()(injector::reg_pack& regs)
                        {
                            regs.eax = *(uint32_t*)(regs.ebp + 8);

                            float hor3DScale = 1.0f / (GetAspectRatio() / (4.0f / 3.0f));
                            float ver3DScale = 1.0f; // don't touch this
                            float mirrorScale = 0.925f;
                            float f1205 = 1.205f;
                            float f0434665 = 0.434665f;

                            int nScaling = std::clamp(*pScaling, 0, 2);
                            if (nScaling)
                            {
                                hor3DScale /= 1.034482718f;
                                f1205 = 1.225f;

                                if (nScaling == 2)
                                {
                                    f1205 = 1.28f;
                                }
                            }

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
                }();

            }

            //Rain droplets
            auto pattern = hook::pattern("D9 44 24 08 D8 44 24 10 8B 4C 24 0C 8B 44 24 10 8B D1");
            struct RainDropletsHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    float fRainScaleX = ((0.75f / GetAspectRatio()) * (4.0f / 3.0f));
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

            // Motion blur off: the blur pass is skipped (jnz -> jmp), the other screen effects stay. Also while temporal
            // anti-aliasing (TAA, DLAA, FSR) is on: the blur would smear its history.
            {
                [&]()
                {
                    if (!PatternPreflight::Check("NFSCarbon Rendering", {
                        { "D9 87 B4 00 00 00 D8 1D ? ? ? ? DF E0 F6 C4 41 75 76", 1, true },
                    }))
                        return;

                    static ProtectedGameRef<uint8_t> MotionBlurBranch;
                    MotionBlurBranch.SetAddress(hook::pattern("D9 87 B4 00 00 00 D8 1D ? ? ? ? DF E0 F6 C4 41 75 76").count(1).get(0).get<uint8_t>(17)); //0x0071356B
                    auto update = []()
                    {
                        uint8_t opcode = *pDisableMotionBlur || *pAntiAliasing >= 2 ? 0xEB : 0x75;
                        if (MotionBlurBranch != opcode)
                            MotionBlurBranch = opcode;
                    };
                    update();
                    WFP::onGameProcessEvent() += update;
                }();

            }

            // Field of view: the driving cameras (CubicCameraMover, used by CDActionDrive) get a live multiplier.
            // The mover writes the camera's FOV (a 16-bit angle, 65536 = 360 degrees) from eax each update;
            // the garage, menu, movie and race intro cameras use other movers and keep theirs.
            {
                auto pattern = hook::pattern("D1 E0 84 C9 75 ? 8B 4E 1C 66 89 81 E4 00 00 00");
                static std::vector<SafetyHookMid> DrivingFOVHooks;
                NFSFieldOfView::InstallDrivingHooks(pattern, 6, 1, DrivingFOVHooks, +[](SafetyHookContext& regs)
                {
                    regs.eax = NFSFieldOfView::ScaleDrivingAngle(regs.eax, *pFovFactor, 150.0f * 65536.0f / 360.0f);
                });

            }

            static ProtectedGameRef<float> fAutosculptScaling;
            fAutosculptScaling.SetAddress(*hook::pattern("D8 0D ? ? ? ? DA 74 24 18 E8 ? ? ? ? 89 46 04 EB 03").count(1).get(0).get<float*>(2));

            static ProtectedGameRef<float> fArrestBlur;
            fArrestBlur.SetAddress(*hook::pattern("D8 0D ? ? ? ? 8B 4C 24 14 8B 54").count(1).get(0).get<float*>(2));

            onResChange() += [](int Width, int Height)
            {
                float fAspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
                fAutosculptScaling = 480.0f * fAspectRatio;
                fArrestBlur = (1.0f / 640.0f) * ((4.0f / 3.0f) / fAspectRatio);
            };

            //Water Reflections fix
            {
                [&]()
                {
                    if (!PatternPreflight::Check("NFSCarbon Rendering", {
                        { "8B 0D ? ? ? ? B8 56 55 55 55 89 15 ? ? ? ? F7 E9", 2, true },
                        { "8B ? ? ? ? ? ? 50 FF ? ? 85 C0", 4, true },
                        { "BA ? ? ? ? ? ? 85 FF 89 15", 1, true },
                    }))
                        return;

                    static uint32_t n768 = 768;
                    static uint32_t n320 = 320;
                    static uint32_t n240 = 240;
                    uint32_t* dword_71A9B1 = hook::pattern("8B 0D ? ? ? ? B8 56 55 55 55 89 15 ? ? ? ? F7 E9").count(2).get(1).get<uint32_t>(2);
                    injector::WriteMemory(dword_71A9B1, &n768, true);
                    auto pattern = hook::pattern("8B ? ? ? ? ? ? 50 FF ? ? 85 C0");
                    uint32_t* dword_71AA22 = pattern.count(4).get(2).get<uint32_t>(2);
                    uint32_t* dword_71AA72 = pattern.count(4).get(3).get<uint32_t>(2);
                    injector::WriteMemory(dword_71AA22, &n320, true);
                    injector::WriteMemory(dword_71AA72, &n320, true);
                    pattern = hook::pattern("BA ? ? ? ? ? ? 85 FF 89 15");
                    uint32_t* dword_71A9F8 = pattern.count(1).get(0).get<uint32_t>(1);
                    injector::WriteMemory(dword_71A9F8, n240, true);
                    uint32_t* dword_71A9FC = pattern.count(1).get(0).get<uint32_t>(5);
                    injector::MakeNOP(dword_71A9FC, 2, true);
                }();

            }
        };
    }
} Rendering;
