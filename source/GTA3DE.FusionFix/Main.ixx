module;

#include "stdafx.h"

export module Main;

import Build;
import Settings;
import Startup;
import Save;
import Hud;
import Window;
import Input;
import Camera;
import CameraProfiles;
import Weather;
import Timecycle;
import PostEffects;
import Renderer;
import World;

namespace
{
    SafetyHookInline initialiseHook;
    bool Initialise(const char* filename)
    {
        const auto result = initialiseHook.ccall<bool>(filename);
        if (result)
            WFP::onGameInitEvent().executeAll();
        return result;
    }
}

export void InitFusionFix()
{
    if (!IsSupportedBuild())
    {
        OutputDebugStringW(L"FusionFix: unsupported DE executable; patches were not installed.\n");
        return;
    }
    ReadSettings();
    WFP::onInitEvent().executeAll();
    if (auto address = hook::pattern("48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 48 89 4C 24 08 57 41 54 41 55 41 56 41 57 48 83 EC 40 45 33 ED 4C 8D 3D ?").get_first())
        initialiseHook = safetyhook::create_inline(address, Initialise);
    // CGame::Process, rather than CCamera::Process (which also runs in loads).
    if (auto address = hook::pattern("40 53 55 41 54 48 81 EC A0 00 00 00 48 8B 05 ? ? ? ? 48 33 C4 48 89").get_first())
    {
        static auto processHook = safetyhook::create_mid(address, [](SafetyHookContext&)
        {
            WFP::onGameProcessEvent().executeAll();
        });
    }
}
