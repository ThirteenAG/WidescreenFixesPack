module;

#include <stdafx.h>
#include <d3d9.h>

export module Misc;

import ComVars;

namespace IntroFMVScreen
{
    SafetyHookInline shNotificationMessage = {};
    void __fastcall NotificationMessage(void* FEPackage, void* edx, int a2, void* FEObject, int a4, int a5)
    {
        // pop the package once for the intro skip
        static bool bOnceFlag = false;
        if (!bOnceFlag)
        {
            bOnceFlag = true;
            cFEng::PopPackage(*(char**)((uintptr_t)FEPackage + 0xC));
            return;
        }

        return shNotificationMessage.unsafe_fastcall(FEPackage, edx, a2, FEObject, a4, a5);
    }
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEventAsync() += []()
        {
            CIniReader iniReader("");
            bool bSkipIntro = iniReader.ReadInteger("MISC", "SkipIntro", 0) != 0;
            bool bShowLangSelect = iniReader.ReadInteger("MISC", "ShowLangSelect", 0) != 0;
            uint32_t nForcedAudioSampleRate = iniReader.ReadInteger("MISC", "ForcedAudioSampleRate", 44100);
            static int nFPSLimit = iniReader.ReadInteger("MISC", "FPSLimit", -1);

            WFP::onGameInitEvent() += []()
            {
                SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)CreateIconFromResourceICO(IDR_NFSUICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON)));
                SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)CreateIconFromResourceICO(IDR_NFSUICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON)));

            };

            if (bSkipIntro)
            {
                // patch bootflow

                // LS_IntroFMV.fng NEEDS to be there in order to hear the splash screen music!
                // This means that the intro video technically still plays, which means e3_title.mad still needs to be in the movies folder
                static const char* BootFlowScreens[] = { "LS_IntroFMV.fng", "LS_Splash_PC.fng", "MU_UG_NEwOrLoad_PC2.fng" , "BootFlowEnd" };
                static const char* BootFlowScreensLangSelect[] = { "LS_LangSelect.fng", "LS_IntroFMV.fng", "LS_Splash_PC.fng", "MU_UG_NEwOrLoad_PC2.fng" , "BootFlowEnd" };

                const char** outScreens = BootFlowScreens;
                const char** outLastScreen = &BootFlowScreens[_countof(BootFlowScreens) - 1];
                if (bShowLangSelect)
                {
                    outScreens = BootFlowScreensLangSelect;
                    outLastScreen = &BootFlowScreensLangSelect[_countof(BootFlowScreensLangSelect) - 1];
                }

                uintptr_t loc_4DE21A = reinterpret_cast<uintptr_t>(hook::pattern("74 ? 85 C0 0F 95 C1").get_first(0));
                uintptr_t loc_4DE273 = reinterpret_cast<uintptr_t>(hook::pattern("BF ? ? ? ? 6A ? E8").get_first(0));
                uintptr_t loc_4DE2A0 = reinterpret_cast<uintptr_t>(hook::pattern("81 FF ? ? ? ? 89 46 ? 89 48 ? ? ? 7C ? ? ? 85 F6").get_first(0));

                // force US Retail bootflow
                injector::MakeJMP(loc_4DE21A, loc_4DE273);

                // list start
                injector::WriteMemory<const char**>(loc_4DE273 + 1, outScreens, true);

                // list end
                injector::WriteMemory<const char**>(loc_4DE2A0 + 2, outLastScreen, true);

                // IntroFMV NotificationMessage Hook -- for skipping the video for the first time it plays
                auto pattern = hook::pattern("8B 44 24 ? 3D ? ? ? ? 56 74");
                IntroFMVScreen::shNotificationMessage = safetyhook::create_inline(pattern.get_first(0), IntroFMVScreen::NotificationMessage);
            }
            else if (bShowLangSelect)
            {
                static const char* BootFlowScreensLangSelect[] = { "LS_LangSelect.fng", "LS_EAlogo.fng", "LS_PSAMovie.fng", "LS_IntroFMV.fng", "LS_Splash_PC.fng", "MU_UG_NEwOrLoad_PC2.fng" , "BootFlowEnd" };

                uintptr_t loc_4DE21A = reinterpret_cast<uintptr_t>(hook::pattern("89 36 89 76 04 A1 ? ? ? ? 83 F8 05").get_first(0)) + 0x15;
                uintptr_t loc_4DE273 = loc_4DE21A + 0x59;
                uintptr_t loc_4DE2A0 = reinterpret_cast<uintptr_t>(hook::pattern("8B 4E 04 89 01 83 C7 04 81 FF ? ? ? ? 89 46 04 89 48 04 89 30 7C C8 8B 36").get_first(0)) + 8;

                // force US Retail bootflow
                injector::MakeJMP(loc_4DE21A, loc_4DE273);

                // list start
                injector::WriteMemory<const char**>(loc_4DE273 + 1, BootFlowScreensLangSelect, true);

                // list end
                injector::WriteMemory<const char**>(loc_4DE2A0 + 2, &BootFlowScreensLangSelect[_countof(BootFlowScreensLangSelect) - 1], true);
            }

            if (nForcedAudioSampleRate)
            {
                uintptr_t loc_532B15 = reinterpret_cast<uintptr_t>(hook::pattern("C7 05 ? ? ? ? 22 56 00 00 A1 ? ? ? ? 85 C0").get_first(0)) + 0xA;
                uintptr_t dword_735610 = *(uintptr_t*)(loc_532B15 + 1);
                *(uint32_t*)dword_735610 = nForcedAudioSampleRate;
            }

            //__mbclen to strlen, "---" bug fix
            auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 04 85 C0 76 21 8D 43 60");
            if (!pattern.empty())
            {
                injector::MakeCALL(pattern.get_first(0), strlen, true);
            }

            if (nFPSLimit)
            {
                if (nFPSLimit < 0)
                {
                    if (nFPSLimit == -1)
                        nFPSLimit = GetDesktopRefreshRate();
                    else if (nFPSLimit == -2)
                        nFPSLimit = GetDesktopRefreshRate() * 2;
                    else
                        nFPSLimit = 60;
                }

                // the game limits FPS 2x over the frametime (or just reports it that way to D3D) -- this is the real game framerate here!
                static float FrameTime = 1.0f / nFPSLimit;
                // Video mode frametime
                uint32_t* dword_6CC7B0 = *hook::pattern("83 EC 10 A1 ? ? ? ? 89 44 24 04").count(1).get(0).get<uint32_t*>(31);
                injector::WriteMemory(dword_6CC7B0, FrameTime, true);
                // RealTimestep frametime
                uint32_t* dword_6F0890 = *hook::pattern("99 D9 05 ? ? ? ? B9 64 00 00 00").count(1).get(0).get<uint32_t*>(3);
                injector::WriteMemory(dword_6F0890, FrameTime, true);
                uint32_t* dword_6CCDEC = *hook::pattern("99 D9 05 ? ? ? ? B9 64 00 00 00").count(1).get(0).get<uint32_t*>(63);
                injector::WriteMemory(dword_6CCDEC, FrameTime * 2.0f, true);
                // a function in eDisplayFrame (particle effects?) frametime
                uint32_t* dword_40A744 = hook::pattern("68 89 88 88 3C").count(1).get(0).get<uint32_t>(1);
                injector::WriteMemory(dword_40A744, FrameTime, true);
                // something related to framerate and/or seconds. This value has to be an integer multiple of 60, otherwise the game can freeze. This affects some gameplay features such as NOS and menus.
                uint32_t* dword_6CC8B0 = *hook::pattern("83 E1 01 0B F9 D9 44 24 28 D8 1D ? ? ? ?").count(1).get(0).get<uint32_t*>(11);
                static float FrameSeconds = static_cast<float>(nFPSLimit);

                if (nFPSLimit % 60)
                    FrameSeconds = static_cast<float>(nFPSLimit - (nFPSLimit % 60));

                // needed to avoid crashes, anything above 120 seems to be problematic at the moment...
                if (FrameSeconds > 120.0f)
                    FrameSeconds = 120.0f;

                if (FrameSeconds < 60.0)
                    FrameSeconds = 60.0;
                injector::WriteMemory(dword_6CC8B0, FrameSeconds, true);
                // another frametime -- seems to affect some gameplay elements...
                uint32_t* dword_6B5C08 = *hook::pattern("DF E0 F6 C4 41 7A ? 8A 44 24 12 D9 05 ? ? ? ?").count(1).get(0).get<uint32_t*>(13);
                injector::WriteMemory(dword_6B5C08, FrameTime, true);

                // GAME BUGFIX: fix sticky steering (especially on high FPS)
                // kill the autocentering!
                uint32_t* dword_460A3D = hook::pattern("D9 54 24 14 D9 E1 D8 1D ? ? ? ? DF E0 F6 C4 05 7A 08").count(1).get(0).get<uint32_t>(0x11); //0x460A2C anchor
                injector::WriteMemory<uint8_t>(dword_460A3D, 0xEB, true);

                // explanation:
                // this is a native game bug which can sadly happen on a bone-stock game, it's just been exaggerated by the higher FPS and faster input
                // what happens is this: if the steering curve isn't finished with the steering (if the wheels aren't centered), it will outright ignore the other direction you're pressing
                // to exaggerate the issue even further: nop out the instruction at 00460DFA which will make the steering extremely slow, then try steering all the way to the left, then quickly go and hold right. Notice how it *won't* continue to go right.
                // so to summarize in a chain of events:
                // 1. game controller sets steering value (-1.0 to 1.0)
                // 2. World::DoTimestep updates the player info and somewhere
                // 3. Somewhere in that chain it calls PlayerSteering::DoUndergroundSteering with the steering value as argument
                // 4. Steering curves get processed at PlayerSteering::CalculateSteeringSpeed                                          <-- THIS IS WHERE THE BUG HAPPENS (or shortly thereafter)
                // 5. Steering value gets passed to the car
                // Skipping 4. is possible by enabling the bool at 00736514, but that is intended exclusively for wheel input and not gamepad/keyboard input.
            }

            // Disable the best lap time counter at the end of the race for better ultrawide support
            // (this element is not visible during 16:9 play)
            auto loc_49A222 = hook::pattern("8A 44 24 24 8B 6C 24 68 8B 7D 0C 83 CE FF 84 C0 8D 4C 24 24 74 ? 6B F6 21");
            pattern = hook::pattern("0F 84 B8 00 00 00 8D 4C 24 44");
            injector::MakeNOP(pattern.get_first(0), 6, true);
            injector::MakeJMP(pattern.get_first(0), loc_49A222.get_first(0), true);

            {
                auto pattern = hook::pattern("E8 ? ? ? ? 8B 2D ? ? ? ? BF");
                static auto EAXSoundUpdateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    *(float*)(regs.esp + 0x0) = std::clamp(*(float*)(regs.esp + 0x0), 1.0f / 60.0f, std::numeric_limits<float>::max());
                });
            }
        };
    }
} Misc;