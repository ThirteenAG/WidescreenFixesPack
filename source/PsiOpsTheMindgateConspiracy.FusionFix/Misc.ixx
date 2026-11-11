module;
#include <stdafx.h>
#include "FixSettings.hpp"

export module Misc;
import ComVars;
import CameraMath;
import FloatingPoint;
import FrameRate;
import HudMath;
import MovieGeometry;
import Window;
import NativeMenuText;
import NativeMenuView;
import NativeMenu;
import DeveloperOverlay;
import Updates;
import Camera;
import Movies;
import Focus;
import Cinematics;
import Hud;

void InitializeGame()
{
    hook::pattern pattern;
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    PsiOpsMenu::Init();
    Screen.Width = settings.Int("MAIN.ResX");
    Screen.Height = settings.Int("MAIN.ResY");

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 32767 || Screen.Height > 32767)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    if (settings.Int("MAIN.WindowMode") == 2)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    PsiOpsWindow::Init(Screen.Width, Screen.Height);
    PsiOpsOverlay::Install();
    PsiOpsUpdates::Install();

    pattern = hook::pattern("FF 24 95 ? ? ? ? 8B 45 F8 0F B6 08 85 C9");
    injector::MakeNOP(pattern.get_first(), 7, true);

    // The resolution switch's first case (the other cases share its tail)
    pattern = hook::pattern("8B 45 F8 0F B6 08 85 C9 74 16 8B 55 08 C7 02 ? ? ? ? 8B 45 0C C7 00 ? ? ? ? C6 45 FF 01");
    injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(15), Screen.Width, true);
    injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(24), Screen.Height, true);

    PsiOpsMovies::Init();
    PsiOpsFocus::Init();

    PsiOpsHud::Init();

    PsiOpsCamera::Init();
    PsiOpsCinematics::Init();

    if (settings.Bool("MAIN.SkipIntro"))
    {
        // The startup logo movies (publisher, GameSpy, Havok, legal) play only when this flag is set; story movies stay
        pattern = hook::pattern("8A 0D ? ? ? ? 51 E8 ? ? ? ? 8B C8 E8 ? ? ? ? C6 05");
        static constexpr uint8_t clearFlag[] = { 0x31, 0xC9, 0x90, 0x90, 0x90, 0x90 }; // xor ecx, ecx
        for (size_t i = 0; i < sizeof(clearFlag); ++i)
            injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(i), clearFlag[i], true);
    }
}


class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            InitializeGame();
        };
    }
} Misc;
