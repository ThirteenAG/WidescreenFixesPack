module;
#include <stdafx.h>
#include "FixSettings.hpp"

export module Misc;
import ComVars;
import Display;
import Camera;
import Hud;
import DeveloperOverlay;

void InitializeGame()
{
    auto& settings = FixSettings();
    int width = settings.Int("MAIN.ResX");
    int height = settings.Int("MAIN.ResY");
    const int mode = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2);
    if (width <= 0 || height <= 0 || width > 16384 || height > 16384)
        std::tie(width, height) = GetDesktopRes();
    // The menus are an 800x600 picture: missions use at least that
    if (width < 800 || height < 600)
        std::tie(width, height) = std::make_tuple(800, 600);

    ParadiseDisplay::Init(mode, width, height);
    ParadiseCamera::Init();
    ParadiseHud::Init();
    ParadiseOverlay::Init();

    // Skip intro: the game's own -nointro switch (publisher and studio logos, title movie)
    if (settings.Bool("MAIN.SkipIntro"))
    {
        auto intro = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 84 C0 59 0F 85 ? ? ? ? 57");
        injector::WriteMemory<uint16_t>(intro.get_first(13), 0xE990, true);
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
