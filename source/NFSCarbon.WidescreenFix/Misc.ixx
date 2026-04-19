module;

#include <stdafx.h>
#include <d3d9.h>

export module Misc;

import ComVars;

class Misc
{
public:
    Misc()
    {
        WFP::onInitEventAsync() += []()
        {
            CIniReader iniReader("");
            bool bSkipIntro = iniReader.ReadInteger("MISC", "SkipIntro", 0) != 0;
            bool bExperimentalCrashFix = iniReader.ReadInteger("MISC", "CrashFix", 1) != 0;
            SimRate = iniReader.ReadInteger("MISC", "SimRate", -1);
            bool bCarShadowFix = iniReader.ReadInteger("GRAPHICS", "CarShadowFix", 1) != 0;

            if (bCarShadowFix)
            {
                uint32_t* dword_7BEA3A = hook::pattern("D8 05 ? ? ? ? DC C0 E8 ? ? ? ? 85 C0 7F 04 33 C0").count(1).get(0).get<uint32_t>(2);
                static float f60 = 60.0f;
                injector::WriteMemory(dword_7BEA3A, &f60, true);
            }

            if (bSkipIntro)
            {
                static auto counter = 0;
                static auto og_value = 0;
                auto pattern = hook::pattern("8B 0D ? ? ? ? 53 33 DB 3B CB ? ? 8B 47 04");
                static uint32_t* dword_A9E6D8 = *pattern.get_first<uint32_t*>(2);
                struct SkipIntroHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        if (counter < 3)
                        {
                            if (counter == 0)
                                og_value = *dword_A9E6D8;
                            *dword_A9E6D8 = 1;
                            counter++;
                        }
                        else
                        {
                            *dword_A9E6D8 = og_value;
                        }

                        regs.ecx = *(uint32_t*)dword_A9E6D8;
                    }
                }; injector::MakeInline<SkipIntroHook>(pattern.get_first(0), pattern.get_first(6));;
            }

            if (bExperimentalCrashFix)
            {
                auto pattern = hook::pattern("75 0B 8B 06 8B CE FF 50 14 3B D8"); //0x59606D
                injector::MakeNOP(pattern.get_first(0), 2, true);
                pattern = hook::pattern("74 20 8B 44 24 20 8B 55 00 6A 00 50 6A 00"); //0x5960A9
                injector::MakeNOP(pattern.get_first(0), 2, true);
            }

            if (SimRate)
            {
                if (SimRate < 0)
                {
                    if (SimRate == -1)
                        SimRate = GetDesktopRefreshRate();
                    else if (SimRate == -2)
                        SimRate = GetDesktopRefreshRate() * 2;
                    else
                        SimRate = 60;
                }

                // this shouldn't be necessary - if the game frametime/rate is matched with the sim frametime/rate, then everything is fine.
                // limit rate to avoid issues...
                //if (SimRate > 360)
                //    SimRate = 360;
                //if (SimRate < 60)
                //    SimRate = 60;

                static float FrameTime = 1.0f / SimRate;
                static float fSimRate = (float)SimRate;

                // Frame times
                // PrepareRealTimestep() NTSC video mode frametime, .rdata
                //float* flt_9CBC14 = *hook::pattern("D9 05 ? ? ? ? B9 ? ? ? ? D8 44 24 14 D9 5C 24 14").count(1).get(0).get<float*>(53); //0x006B4CFB
                uint32_t* dword_6B4D30 = hook::pattern("D9 05 ? ? ? ? B9 ? ? ? ? D8 44 24 14 D9 5C 24 14").count(1).get(0).get<uint32_t>(53); //0x006B4CFB
                // MainLoop frametime (FPS lock) .text
                float* flt_6B79E3 = hook::pattern("C7 44 24 1C 89 88 88 3C").count(1).get(0).get<float>(4); //0x6B79DF
                // FullSpeedMode frametime (10x speed) .text
                float* flt_6B79F4 = hook::pattern("C7 44 24 1C AB AA 2A 3E").count(1).get(0).get<float>(4); //0x6B79F0
                // Unknown frametime 1 .text
                float* flt_764A42 = hook::pattern("C7 46 14 89 88 88 3C").count(1).get(0).get<float>(3); //0x00764A3F
                // Unknown frametime 2 .text
                float* flt_76AD08 = hook::pattern("68 89 88 88 3C").count(1).get(0).get<float>(1); //0x76AD07
                // ESimTask frametime .rdata
                float* flt_9DB360 = *hook::pattern("83 C4 04 D9 46 14 42 D8 05 ? ? ? ?").count(1).get(0).get<float*>(9); //0x672EFB
                // Unknown frametime 3 .rdata (in some data structure, these values may need to be scaled up from 60.0 accordingly)
                //float* flt_9E24F0 = hook::pattern("00 00 70 42 89 88 88 3C 00 00 20 40 00 00 E0 40").count(1).get(0).get<float>(4); //0x9E24EC
                // Sim::QueueEvents frametime .text (this affects gameplay smoothness noticeably)
                float* flt_9EBB6C = *hook::pattern("D9 46 1C 8B 46 24 83 F8 03 D8 0D ? ? ? ?").count(1).get(0).get<float*>(11); //0x0076AD57

                //injector::WriteMemory(flt_9CBC14, FrameTime, true);
                injector::WriteMemory(dword_6B4D30, &FrameTime, true);
                injector::WriteMemory(flt_6B79E3, FrameTime, true);
                injector::WriteMemory(flt_6B79F4, FrameTime * 10.0f, true);
                injector::WriteMemory(flt_764A42, FrameTime, true);
                injector::WriteMemory(flt_76AD08, FrameTime, true);
                injector::WriteMemory(flt_9DB360, FrameTime, true);
                //injector::WriteMemory(flt_9E24F0, FrameTime, true);
                injector::WriteMemory(flt_9EBB6C, FrameTime, true);

                // Frame rates
                // Unknown framerate 1 .rdata (in some data structure, these values may need to be scaled up from 60.0 accordingly)
                //float* flt_9E24EC = hook::pattern("00 00 70 42 89 88 88 3C").count(1).get(0).get<float>(0); //0x9E24EC
                // Unknown framerate 2 .rdata (in some data structure, these values may need to be scaled up from 60.0 accordingly)
                //float* flt_9E2500 = hook::pattern("00 00 70 42 89 88 88 3C").count(1).get(0).get<float>(20); //0x9E24EC

                //injector::WriteMemory(flt_9E24EC, fSimRate, true);
                //injector::WriteMemory(flt_9E2500, fSimRate, true);

                // NOTE: drift scoring system has 60.0 values... it may be affected by this... it needs thorough testing to see if parts like that are affected!

                // fix WorldMap cursor & scale the update constants with frametime to keep consistent movement
                // WorldMap::UpdateAnalogInput()
                auto pattern = hook::pattern("D9 05 ? ? ? ? 83 EC 08 D8 0D ? ? ? ? 56 8B F1");//0x0058F770 anchor
                // WorldMap framerate .text
                uint32_t* dword_58F779 = pattern.count(1).get(0).get<uint32_t>(0xB); //0x0058F779
                // constants
                float* flt_9CEDF4 = *pattern.count(1).get(0).get<float*>(0x1B); //0x0058F789 dereference
                uint32_t* dword_58F78F = pattern.count(1).get(0).get<uint32_t>(0x21); //0x0058F78F
                uint32_t* dword_58F79E = pattern.count(1).get(0).get<uint32_t>(0x2E); //0x0058F79E
                uint32_t* dword_58F7A8 = pattern.count(1).get(0).get<uint32_t>(0x38); //0x0058F7A8
                uint32_t* dword_58F7B5 = pattern.count(1).get(0).get<uint32_t>(0x45); //0x0058F7B5

                injector::WriteMemory(dword_58F779, &fSimRate, true);

                // Scalar target FPS
                constexpr float TargetFPS = 60.0f;

                static float WorldMapConst5 = ((*flt_9CEDF4) / (1.0f / TargetFPS)) * FrameTime;
                injector::WriteMemory(flt_9CEDF4, WorldMapConst5, true);

                static float WorldMapConst1 = ((*(float*)dword_58F78F) / (1.0f / TargetFPS)) * FrameTime;
                injector::WriteMemory(dword_58F78F, &WorldMapConst1, true);

                static float WorldMapConst2 = ((*(float*)dword_58F79E) / (1.0f / TargetFPS)) * FrameTime;
                injector::WriteMemory(dword_58F79E, &WorldMapConst2, true);

                static float WorldMapConst3 = ((*(float*)dword_58F7A8) / (1.0f / TargetFPS)) * FrameTime;
                injector::WriteMemory(dword_58F7A8, &WorldMapConst3, true);

                static float WorldMapConst4 = ((*(float*)dword_58F7B5) / (1.0f / TargetFPS)) * FrameTime;
                injector::WriteMemory(dword_58F7B5, &WorldMapConst4, true);
            }
        };
    }
} Misc;