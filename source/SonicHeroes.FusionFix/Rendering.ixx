module;

#include <stdafx.h>
#include <minidx8/d3d8.h>
#include "FixSettings.hpp"
#include "LegacyHooks.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Rendering;

import ComVars;

export
{
// STUB: resolution update, window procedure, client rect, lens flare, demo restore, Advertise window, staff roll,
// text drawing, window class / window update hooks and the SystemMode enum authored by REDACTED have been removed.
// The two variables below are kept because other code depends on them.
float iniZoomFactor = 1.0f;
float fShadowScale = 1.7f;

void UpdateViewScales()
{
    Screen.fHudZoomFactor = Screen.fAspectRatio < Screen.fConditionalAspectRatio ? Screen.fAspectRatio / Screen.fConditionalAspectRatio : 1.0f;
    Screen.fZoomFactor = Screen.fHudZoomFactor / iniZoomFactor;
    Screen.fCameraScaleX = ((4.0f / 3.0f) / Screen.fAspectRatio) * Screen.fZoomFactor;
    Screen.fHudScale = ((4.0f / 3.0f) / Screen.fAspectRatio) * Screen.fHudZoomFactor;
    Screen.HeightHUD = static_cast<int>(Screen.Height * Screen.fHudZoomFactor);
    Screen.OffsetX = static_cast<int>(Screen.Width - Screen.Width * Screen.fHudScale) / 2;
    Screen.OffsetY = static_cast<int>(Screen.Height - Screen.Height * Screen.fHudZoomFactor) / 2;
}

uintptr_t ptrWindowSizeX = 0;
uintptr_t loc_6567BF = 0;
void InvalidateWindowSize()
{
    WindowSizeX = 640;
}

// The notice comes as a message box like the game's own quit question, once the main menu runs
}
