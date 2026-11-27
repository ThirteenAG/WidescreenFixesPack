module;
#include <stdafx.h>
#include "FixSettings.hpp"
#include "rawinput.hpp"
#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

export module Misc;
import ComVars;
import Display;
import Camera;
import Hud;
import DeveloperOverlay;

// The story films are opened from the registry's Video Path. When the game folder was moved or copied, that path
// points to a folder that no longer exists: the game asked for disc B and New Game crashed. A film that doesn't open
// there is opened from the game's own Video folder.
using BinkOpenFn = void* (__stdcall*)(const char*, uint32_t);
BinkOpenFn BinkOpenOriginal = nullptr;

void* __stdcall OpenFilm(const char* path, uint32_t flags)
{
    if (auto film = BinkOpenOriginal(path, flags))
        return film;
    if (!path)
        return nullptr;
    const auto local = GetExeModulePath() / "Video" / std::filesystem::path(path).filename();
    std::error_code error;
    if (!std::filesystem::exists(local, error))
        return nullptr;
    return BinkOpenOriginal(local.string().c_str(), flags);
}

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
    MessagePumpGuard::Install(GetModuleHandleW(nullptr));

    // The exclusive DirectInput keyboard crashes inside dinput.dll when the game acquires it again on returning
    // from another window; the non-exclusive foreground keyboard (still without the Windows key) doesn't
    auto keyboard = hook::pattern("6A 15 FF 75 08 50 FF 51 34");
    injector::WriteMemory<uint8_t>(keyboard.get_first(1), DISCL_NONEXCLUSIVE | DISCL_FOREGROUND | DISCL_NOWINKEY, true);

    auto film = hook::pattern("6A 00 FF 75 08 8B F1 FF 15 ? ? ? ? 89 46 04 85 C0 0F 95 C0");
    BinkOpenOriginal = **film.get_first<BinkOpenFn*>(9);
    injector::MakeCALL(film.get_first(7), OpenFilm, true);
    injector::MakeNOP(film.get_first(12), 1, true);

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
