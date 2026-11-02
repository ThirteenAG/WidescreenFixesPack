module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Misc;

import ComVars;
import Hud;
import Window;
import Focus;
import Cinematics;
import Camera;
import Intro;
import NativeText;
import Updates;
import Startup;
import Renderer;

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            hook::pattern pattern;
            auto& settings = FixSettings();
            Screen.nWidth = settings.Int("MAIN.ResX");
            Screen.nHeight = settings.Int("MAIN.ResY");

            if (settings.Int("MAIN.WindowMode") == 2)
                std::tie(Screen.nWidth, Screen.nHeight) = GetDesktopRes();

            if (Screen.nWidth <= 0 || Screen.nHeight <= 0 || Screen.nWidth > 16384 || Screen.nHeight > 16384)
                std::tie(Screen.nWidth, Screen.nHeight) = GetDesktopRes();
            if (Screen.nWidth <= 0 || Screen.nHeight <= 0 || Screen.nWidth > 32767 || Screen.nHeight > 32767)
            {
                Screen.nWidth = 640;
                Screen.nHeight = 480;
            }

            auto pattern1 = hook::pattern("68 ? ? ? ? 6A 01 68 ? ? ? ? 68 ? ? ? ? FF D6 48 83 F8 0B");
            auto pattern2 = hook::pattern("C6 05 ? ? ? ? 01 C7 05 ? ? ? ? 01 00 00 00 E9 ? ? ? ? 6A 64");
            dword_6CACDC.SetAddress(*hook::get_pattern<int32_t*>("C7 05 ? ? ? ? 00 04 00 00 C7 05 ? ? ? ? 00 03 00 00", 2));
            dword_6CACE0.SetAddress(*hook::get_pattern<int32_t*>("C7 05 ? ? ? ? 00 04 00 00 C7 05 ? ? ? ? 00 03 00 00", 12));
            struct ResHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    dword_6CACDC = Screen.nWidth;
                    dword_6CACE0 = Screen.nHeight;

                    Screen.fWidth = static_cast<float>(Screen.nWidth);
                    Screen.fHeight = static_cast<float>(Screen.nHeight);
                    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
                    Screen.nWidth43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
                    Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
                    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
                    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
                    Screen.fHudScale = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
                    Screen.nAspectRatioDiff = static_cast<int32_t>(Screen.fWidth / (((4.0f / 3.0f)) / (Screen.fAspectRatio)));
                }
            }; injector::MakeInline<ResHook>(pattern1.get_first(), pattern2.get_first()); //0x40DD33, 0x40DE21

            //forcing 4:3 aspect, maybe better to hook sub_4BC320
            pattern = hook::pattern("A3 ? ? ? ? A1 ? ? ? ? 85 C0 0F 84 ? ? ? ? 6A 01");
            injector::MakeNOP(pattern.get_first(0), 5, true);
            injector::WriteMemory<uint32_t>(pattern2.get_first<uint32_t>(13), 0, true);
            injector::WriteMemory<uint32_t>(reinterpret_cast<uint32_t*>(*pattern2.get_first<uint32_t>(9)), 0, true);

            pattern = hook::pattern("05 94 00 00 00 D9 5C 24 08");
            struct AspectHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.eax += 0x94;
                    injector::WriteMemory<int32_t>(reinterpret_cast<int32_t*>(regs.eax - 0x4C), Screen.nAspectRatioDiff, true);
                }
            }; injector::MakeInline<AspectHook>(pattern.get_first(0)); //0x41F45A

            NeoStartup::Install();
            NeoHud::Install();
            NeoWindow::Install();
            NeoRenderer::Init();
            NeoFocus::Install();
            NeoCinema::Install();
            NeoCamera::Install();
            NeoIntro::Skip = FixSettings().Bool("MAIN.SkipIntro");
            NeoHud::PrepareRoot = NeoIntro::Finish;
            NeoText::Install();
            NeoUpdates::Install();

        };
    }
} Misc;
