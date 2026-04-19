module;

#include <stdafx.h>
#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

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
            bool bForceHighSpecAudio = iniReader.ReadInteger("MISC", "ForceHighSpecAudio", 1) != 0;
            SimRate = iniReader.ReadInteger("MISC", "SimRate", -1);

            if (bForceHighSpecAudio)
            {
                uint32_t* dword_4C41F5 = hook::pattern("C7 05 ? ? ? ? ? ? 00 00 A1 ? ? ? ? 85 C0 ? ? A3 ? ? ? ? 8D 44 24 08").count(1).get(0).get<uint32_t>(6);
                injector::WriteMemory<int>(dword_4C41F5, 44100, true);
            }

            if (bSkipIntro)
            {
                static auto counter = 0;
                static auto og_value = 0;
                auto pattern = hook::pattern("A1 ? ? ? ? 85 C0 74 1C 8B 45 04");
                static uint32_t* dword_926144 = *pattern.get_first<uint32_t*>(1);
                struct SkipIntroHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        if (counter < 3)
                        {
                            if (counter == 0)
                                og_value = *dword_926144;
                            *dword_926144 = 1;
                            counter++;
                        }
                        else
                        {
                            *dword_926144 = og_value;
                        }

                        regs.eax = *(uint32_t*)dword_926144;
                    }
                }; injector::MakeInline<SkipIntroHook>(pattern.get_first(0));
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
                //static float fnFPSLimit = (float)SimRate;

                // Frame times
                // PrepareRealTimestep() NTSC video mode frametime .rdata
                uint32_t* dword_6612EC = hook::pattern("D9 05 ? ? ? ? B9 ? ? ? ? D8 44 24 14 D9 5C 24 14").count(1).get(0).get<uint32_t>(53); //0x006612B7 anchor
                // MainLoop frametime .text
                float* flt_666100 = hook::pattern("E8 ? ? ? ? 68 89 88 88 3C").count(1).get(0).get<float>(6);
                // World_Service UglyTimestepHack initial state .data
                float* flt_903290 = *hook::pattern("8B 0D ? ? ? ? 85 C9 C7 05 ? ? ? ? 00 00 00 00 74 05").count(1).get(0).get<float*>(10); //0x0075AAD8 dereference
                // GetDebugRealTime() NTSC frametime 1 .text
                uint32_t* dword_65C78F = hook::pattern("83 EC 08 A1 ? ? ? ? 89 44 24 04 A1 ? ? ? ? 85 C0 75 08").count(1).get(0).get<uint32_t>(0x1F); //0x0065C770 anchor
                // GetDebugRealTime() NTSC frametime 2 .text
                uint32_t* dword_65C7D9 = hook::pattern("83 EC 08 A1 ? ? ? ? 89 44 24 04 A1 ? ? ? ? 85 C0 75 08").count(1).get(0).get<uint32_t>(0x69); //0x0065C770 anchor

                injector::WriteMemory(dword_6612EC, &FrameTime, true);
                //injector::WriteMemory(flt_8970F0, FrameTime, true);
                injector::WriteMemory(flt_666100, FrameTime, true);
                *flt_903290 = FrameTime;
                injector::WriteMemory(dword_65C78F, &FrameTime, true);
                injector::WriteMemory(dword_65C7D9, &FrameTime, true);

                // Frame rates
                // TODO: if any issues arise, figure out where 60.0 values are used and update the constants...
            }
        };
    }
} Misc;