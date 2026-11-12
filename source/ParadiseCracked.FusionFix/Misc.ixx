module;
#include <stdafx.h>
#include "FixSettings.hpp"
#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

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

    // The exclusive DirectInput keyboard crashes inside dinput.dll when the game acquires it again on returning
    // from another window; the non-exclusive foreground keyboard (still without the Windows key) doesn't
    auto keyboard = hook::pattern("6A 15 FF 75 08 50 FF 51 34");
    injector::WriteMemory<uint8_t>(keyboard.get_first(1), DISCL_NONEXCLUSIVE | DISCL_FOREGROUND | DISCL_NOWINKEY, true);

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
