#pragma once
#include <windows.h>

namespace SaboteurRuntime
{
    void Install();
    void AttachWindow(HWND window);
    HWND GetWindow();
    bool Cinematic();
    int CutsceneBorders();
}
