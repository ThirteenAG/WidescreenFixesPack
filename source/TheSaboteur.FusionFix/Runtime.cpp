#include "stdafx.h"
#include "RuntimeBridge.hpp"

import RuntimeCore;
import ButtonIcons;
import Cutscenes;
import Movies;
import HUD;

namespace SaboteurRuntime
{
    void Install()
    {
        SaboteurRuntimeCore::Initialize();
        SaboteurMovies::Install();
        SaboteurButtons::Install();
        SaboteurHUD::Install();
        SaboteurCutscenes::Install();
    }

    void AttachWindow(HWND window)
    {
        SaboteurRuntimeCore::Attach(window);
    }

    HWND GetWindow()
    {
        return SaboteurRuntimeCore::Window.load();
    }

    bool Cinematic()
    {
        return SaboteurCutscenes::Active();
    }

    int CutsceneBorders()
    {
        return SaboteurCutscenes::Ready() ? SaboteurCutscenes::Borders.load() : 0;
    }
}
