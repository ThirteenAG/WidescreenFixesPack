module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module ScreenPatches;

import ComVars;
import Hud;
import Cutscene;
import Window;

export namespace ScreenPatches
{
    void Install()
    {
        //Unlocking a bit more resolutions
        auto pattern = hook::pattern("81 FF ? ? ? ? 8B 54 24 14 7C 53 81 FA ? ? ? ? 7C 4B 8B 0D ? ? ? ? 85 C9");
        injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(2), 640, true);
        injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(14), 480, true);
        //Last resolution doesn't get reset
        pattern = hook::pattern("3D ? ? ? ? 0F 96 C0 C3");
        injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(1), 16384, true);

        //Resolution
        pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C6 05 ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 83 C4 28 C3");
        (void)*pattern.get_first<uint32_t*>(2);
        (void)*pattern.get_first<uint32_t*>(12);
        std::tie(Screen.nWidth, Screen.nHeight) = GetDesktopRes();
        injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(6), Screen.nWidth, true);
        injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(16), Screen.nHeight, true);

        onResChange() += []()
        {
            Screen.nWidth = DisplayWidth;
            Screen.nHeight = DisplayHeight;
            Screen.fWidth = static_cast<float>(Screen.nWidth);
            Screen.fHeight = static_cast<float>(Screen.nHeight);
            Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
            Screen.nWidth43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
            Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
            Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
            Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
            Screen.fHudScale = (4.0f / 3.0f) / Screen.fAspectRatio;
            Screen.fHudFullWidth = 640.0f * (Screen.fAspectRatio / (4.0f / 3.0f));
            Screen.fHudVerticalOffset = ((480.0f * (Screen.fAspectRatio / (4.0f / 3.0f))) - 480.0f) / 2.0f;
            Screen.fHudVerticalOffsetWide = ((480.0f * (Screen.fAspectRatio / (16.0f / 9.0f))) - 480.0f) / 2.0f;
        };

        static auto GetRes = []()
        {
            onResChange().executeAll();
        };
        BullyWindow::OnResolutionChanged = GetRes;
        BullyHud::Install(Screen.nWidth, Screen.nHeight, FixSettings().String("MAIN.HudAspectRatioConstraint"));
        BullyCutscene::Install(FixSettings().Int("MAIN.CutsceneBorders"), FixSettings().Int("MAIN.AnimatedCutsceneBorders"), Screen.fAspectRatio);

        pattern = hook::pattern("C6 44 24 18 00 8B 4C 24 18 52");
        struct ResHook1
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uint8_t*)(regs.esp + 0x18) = 0;
                GetRes();
            }
        }; injector::MakeInline<ResHook1>(pattern.get_first(0)); //0x4052F1

        pattern = hook::pattern("A3 ? ? ? ? A1 ? ? ? ? 89 15 ? ? ? ? A3 ? ? ? ? 89 0D ? ? ? ? E8 ? ? ? ? 84 C0");
        dword_BCBC68.SetAddress(*pattern.get_first<uint32_t*>(1));
        struct ResHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                dword_BCBC68 = regs.eax;
                GetRes();
            }
        }; injector::MakeInline<ResHook2>(pattern.get_first(0)); //0x406393

        pattern = hook::pattern("89 1D ? ? ? ? 89 2D ? ? ? ? 89 15 ? ? ? ? A3 ? ? ? ? 33 C9 33 D2 33 C0");
        struct ResHook3
        {
            void operator()(injector::reg_pack& regs)
            {
                dword_BCBC68 = regs.ebx;
                GetRes();
            }
        }; injector::MakeInline<ResHook3>(pattern.get_first(0)); //0x4063FA

        pattern = hook::pattern("D9 05 ? ? ? ? D9 14 24 D9 5C 24 04 EB 07 D9 14 24 D9 54 24 04");
        injector::WriteMemory<float*>(pattern.get_first<float*>(2), &Screen.fHudScale, true); //0x4548FE
        pattern = hook::pattern("D9 05 ? ? ? ? D9 54 24 08 D9 5C 24 0C");
        injector::WriteMemory<float*>(pattern.get_first<float*>(2), &Screen.fHudScale, true); //0x406127

        auto flt_93F228 = *hook::get_pattern<void*>("D9 05 ? ? ? ? EB 06 D9 05 ? ? ? ? 83 3D", 2);
        pattern = hook::pattern(pattern_str(0xD9, 0x05, to_bytes(flt_93F228))); //fld
        for (size_t i = 0; i < pattern.size(); ++i)
            injector::WriteMemory<float*>(pattern.get(i).get<float*>(2), &Screen.fHudFullWidth, true);

        pattern = hook::pattern("8B 15 ? ? ? ? D9 5C 24 04 A1 ? ? ? ? D9 05 ? ? ? ? 03 D2 D9 5C 24 08");
        dword_20DCD44.SetAddress(*pattern.get_first<uint32_t*>(2));
        struct BackgroundsHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.edx = dword_20DCD44;

                auto y = *(float*)(regs.esp + 0x14);
                auto h = *(float*)(regs.esp + 0x2C);
                if (y == 0.0f && h == 240.0f)
                {
                    *(float*)(regs.esp + 0x14) -= Screen.fHudVerticalOffset;
                    *(float*)(regs.esp + 0x20) -= Screen.fHudVerticalOffset;
                }
                else if (y == 240.0f && h == 480.0f)
                {
                    *(float*)(regs.esp + 0x2C) += Screen.fHudVerticalOffset;
                    *(float*)(regs.esp + 0x38) += Screen.fHudVerticalOffset;
                }
            }
        }; injector::MakeInline<BackgroundsHook>(pattern.get_first(0), pattern.get_first(6)); //0x73984B

        //In widescreen mode the game loads 16:9 loadscreens from TXD\lsw instead of 4:3 ones from TXD\ls
        pattern = hook::pattern("80 3D ? ? ? ? 00 57 75 0C 68");
        (void)*pattern.get_first<bool*>(2); //0x43BFBC

        pattern = hook::pattern("8B 3D ? ? ? ? D9 5C 24 08 85 FF D9 05 ? ? ? ? D9 5C 24 04 D9");
        dword_20DCCC8.SetAddress(*pattern.get_first<uint32_t*>(2));
        struct LoadscreensHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.edi = dword_20DCCC8;

                auto x = *(float*)(regs.esp + 0x1C);
                auto y = *(float*)(regs.esp + 0x20);
                auto w = *(float*)(regs.esp + 0x50);
                auto h = *(float*)(regs.esp + 0x4C);
                auto fVerticalOffset = bWidescreen ? Screen.fHudVerticalOffsetWide : Screen.fHudVerticalOffset;

                if (x == 0.0f && (static_cast<int>(w) == 639 || static_cast<int>(w) == 640) && y == 0.0f && static_cast<int>(h) == 480)
                {
                    *(float*)(regs.esp + 0x20) -= fVerticalOffset;
                    *(float*)(regs.esp + 0x4C) += fVerticalOffset;
                }
                else
                {
                    h = *(float*)(regs.esp + 0x20);
                    x = *(float*)(regs.esp + 0x24);
                    w = *(float*)(regs.esp + 0x28);
                    y = *(float*)(regs.esp + 0x2C);

                    if (x == 0.0f && (static_cast<int>(w) == 639 || static_cast<int>(w) == 640) && y == 0.0f && static_cast<int>(h) == 480)
                    {
                        *(float*)(regs.esp + 0x2C) -= fVerticalOffset;
                        *(float*)(regs.esp + 0x20) += fVerticalOffset;
                    }
                }
            }
        }; injector::MakeInline<LoadscreensHook>(pattern.get_first(0), pattern.get_first(6)); //0x73984B

        //Aspect Ratio
        pattern = hook::pattern("80 3D ? ? ? ? ? A1 ? ? ? ? D9 40 74 D8 70 78 D9 1C 24 74 0C D9 40 7C");
        struct ARHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)regs.esp = Screen.fAspectRatio;
            }
        }; injector::MakeInline<ARHook>(pattern.get_first(0), pattern.get_first(33)); //0x4F1360
    }
}
