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

namespace
{
    SafetyHookInline initialiseHook;
    bool Initialise()
    {
        const auto result = initialiseHook.ccall<bool>();
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
    if (auto address = hook::pattern("41 55 41 56 48 83 EC 38 48 89 5C 24 50 45 33 ED 48 89 74 24 60 48 89 7C").get_first())
        initialiseHook = safetyhook::create_inline(address, Initialise);
    // CGame::Process, rather than CCamera::Process (which also runs in loads).
    if (auto address = hook::pattern("4C 8B DC 55 41 55 49 8D 6B A1 48 81 EC E8 00 00 00 45 0F 29 43 A8 48 8B").get_first())
    {
        static auto processHook = safetyhook::create_mid(address, [](SafetyHookContext&)
        {
            WFP::onGameProcessEvent().executeAll();
        });
    }
}

