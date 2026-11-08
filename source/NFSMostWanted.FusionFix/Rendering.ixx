module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/NFS/FieldOfView.hpp"

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
            // Changed in the options menu while the game runs
            static const int* pScaling = &FixSettings().Int("GRAPHICS.Scaling");
            static const float* pRainDropletsScale = &FixSettings().Float("GRAPHICS.RainDropletsScale");
            static const float* pFovFactor = &FixSettings().Float("MAIN.FOVFactor");

            // Driving-camera field-of-view multiplier. CubicCameraMover::Update, the
            // mover of the chase, hood and bumper cameras, stores the camera's field of view (a 16 bit angle, 65536
            // = 360 degrees); menus, cutscenes and replays have movers of their own.
            {
                auto pattern = hook::pattern("D1 E0 85 C9 75 ? 8B 4E 1C 66 89 81 C4 00 00 00");
                static std::vector<SafetyHookMid> DrivingFOVHooks;
                NFSFieldOfView::InstallDrivingHooks(pattern, 9, 1, DrivingFOVHooks, +[](SafetyHookContext& regs)
                {
                    // The player view's projection widens the angle's tangent by hor3DScale below
                    float tanScale = GetAspectRatio() * 0.75f * (std::clamp(*pScaling, 0, 2) ? 1.047485948f : 1.0f);
                    float maximum = NFSFieldOfView::HorizontalLimit(24576.0f, tanScale);
                    regs.eax = NFSFieldOfView::ScaleDrivingAngle(regs.eax, *pFovFactor, maximum);
                });

            }

            {
                static float hor3DScale = 1.0f;
                static float ver3DScale = 1.0f; // don't touch this
                static float flt1 = 0.0f;
                static float flt2 = 0.0f;
                static float flt3 = 0.0f;

                uint32_t* dword_6CF4F0 = hook::pattern("DB 40 18 DA 70 14").count(1).get(0).get<uint32_t>(0);
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

                        int nScaling = std::clamp(*pScaling, 0, 2);
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
                };
                auto projection = reinterpret_cast<uintptr_t>(dword_6CF4F0);
                injector::MakeInline<FOVHook>(projection - 6, projection + 6);
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

                // The shadow camera fits the shadow map to the view's frustum, built from the field of view and the
                // viewport's height / width. For a wider screen the view is wider (hor3DScale), not lower: the height
                // keeps its 4:3 share and the width scale follows the player view. Two frustums (both shadow maps).
                auto shadowHeight = hook::pattern("D8 0D ? ? ? ? D9 5C 24 18 DB 44 24 14 D8 0D ? ? ? ? D8 0D ? ? ? ? E8");
                auto shadowWidth = hook::pattern("D8 3D ? ? ? ? D9 5C 24 24 DB 44 24 1C D8 4C 24 20");
                if (shadowHeight.size() != 2 || shadowWidth.size() != 2)
                    std::abort();
                static std::vector<SafetyHookMid> ShadowFrustumHooks;
                for (size_t i = 0; i < 2; ++i)
                {
                    ShadowFrustumHooks.push_back(safetyhook::create_mid(shadowHeight.get(i).get<void>(10), [](SafetyHookContext& regs)
                    {
                        *reinterpret_cast<float*>(regs.esp + 0x18) = 0.75f;
                    }));
                    ShadowFrustumHooks.push_back(safetyhook::create_mid(shadowWidth.get(i).get<void>(10), [](SafetyHookContext& regs)
                    {
                        *reinterpret_cast<float*>(regs.esp + 0x24) *= hor3DScale;
                    }));
                }
            }

            static ProtectedGameRef<float> fAutosculptScaling;
            fAutosculptScaling.SetAddress(*hook::pattern("D8 0D ? ? ? ? DA 74 24 18 E8 ? ? ? ? 89 46 04 EB 03").count(1).get(0).get<float*>(2));

            static ProtectedGameRef<float> fArrestBlur;
            fArrestBlur.SetAddress(*hook::pattern("D8 0D ? ? ? ? 8B 4C 24 18 8B 54 24 1C").count(1).get(0).get<float*>(2));

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
                    float fRainDropletsScale = *pRainDropletsScale;
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
                    float fRainDropletsScale = *pRainDropletsScale;
                    _asm {fmul dword ptr[fRainDropletsScale]}
                    _asm {fadd dword ptr[esp08]}
                    *(uintptr_t*)(regs.esp + 0x38) = regs.ecx;
                }
            }; injector::MakeInline<RainDropletsYScaleHook>(pattern.get_first(30), pattern.get_first(30 + 8)); //6D482B
        };
    }
} Rendering;
