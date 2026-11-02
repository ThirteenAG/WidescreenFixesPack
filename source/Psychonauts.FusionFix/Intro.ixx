module;

#include "stdafx.h"
#include "FixSettings.hpp"

export module Intro;

SafetyHookInline StartupMovieHook;
char __fastcall PlayStartupMovie(void* self, void*, const char* name, uint8_t(*skip)(), uint8_t flags, float volume, int track, char update)
{
    std::string_view file{ name ? name : "" };
    if (auto slash = file.find_last_of("\\/"); slash != file.npos)
        file.remove_prefix(slash + 1);
    if (_stricmp(std::string(file).c_str(), "dflogo.bik") == 0)
        return 1;
    return StartupMovieHook.thiscall<char>(self, name, skip, flags, volume, track, update);
}

export void InstallLogoSkip()
{
    if (!FixSettings().Bool("MAIN.SkipIntro"))
        return;
    auto movie = hook::pattern("55 8B EC 81 EC 34 01 00 00 89 8D EC FE FF FF 68 ? ? ? ? FF 15 ? ? ? ? 8B 85 EC FE FF FF 0F B6 88 E5 00 00 00");
    StartupMovieHook = safetyhook::create_inline(movie.get_first(), PlayStartupMovie);
}

// The title's "Press ... to begin": answered as pressed once the title has been shown for a moment
char __cdecl TitleStartButton(int button)
{
    static ULONGLONG shown = 0, last = 0;
    static const auto pressed = reinterpret_cast<char(__cdecl*)(int)>(hook::get_pattern("55 8B EC 83 EC 08 C7 45 FC 00 00 00 00 EB ? 8B 45 FC 83 C0 01 89 45 FC 83 7D FC 03"));
    const auto now = GetTickCount64();
    if (now - last > 1000)
        shown = now;
    last = now;
    if (FixSettings().Bool("MAIN.SkipPressStart") && now - shown >= 500)
        return 1;
    return pressed(button);
}

export void InstallPressStartSkip()
{
    auto pattern = hook::pattern("C6 45 EF 00 6A 04 E8 ? ? ? ? 83 C4 04 0F B6 C0 85 C0 0F 84");
    injector::MakeCALL(pattern.get_first(6), TitleStartButton, true);
}

