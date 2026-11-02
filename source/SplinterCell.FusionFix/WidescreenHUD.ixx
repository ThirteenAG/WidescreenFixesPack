module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module WidescreenHUD;

import ComVars;
import CutsceneFrame;

// Once per frame on the game thread: the live HUD settings, consumed by the HUD draw hooks
export void RefreshHudConstraint()
{
    Screen.fGrainScale = std::clamp(FixSettings().Float("MAIN.GrainScale"), 0.0f, 1.0f);
    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.fWidth <= 0.0f || Screen.fHeight <= 0.0f ||
        !std::isfinite(Screen.fWidth) || !std::isfinite(Screen.fHeight) || !std::isfinite(Screen.fAspectRatio))
        return;

    // Pillarboxed cinematics keep their transmissions inside the 16:9 picture
    const bool pillarbox = UObject::GetState(L"EchelonMainHUD") == L"s_Cinematic" && SCCutscene::Pillarbox(Screen.fAspectRatio);
    const auto wanted = FixSettings().Wanted("MAIN.HudAspectRatioConstraint");
    Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(wanted);
    auto constraint = Screen.fHudAspectRatioConstraint;
    if (pillarbox && (!constraint || (*constraint >= 0.0f && *constraint > SCCutscene::kPictureAspect)))
        constraint = SCCutscene::kPictureAspect;
    const float height = Screen.fHeight;
    float width = Screen.fWidth;
    if (constraint && (*constraint < 0.0f || *constraint > 32.0f / 9.0f))
        Screen.fWidescreenHudOffset = *constraint;
    else
    {
        if (constraint)
            width = height * ClampHudAspectRatio(*constraint, Screen.fAspectRatio);
        Screen.fWidescreenHudOffset = CalculateWidescreenOffset(width, height, 640.0f, 480.0f, 24.0f);
    }
}

export void InitWidescreenHUD()
{
    // The scripts' HUD margin, moved out to the edges of the HUD box (24 at 4:3, as in the game)
    UIntOverrides::Register(L"IntProperty Echelon.EchelonGameInfo.HUD_OFFSET_X", +[]() -> int
    {
        return static_cast<int>(Screen.fWidescreenHudOffset);
    });
}
