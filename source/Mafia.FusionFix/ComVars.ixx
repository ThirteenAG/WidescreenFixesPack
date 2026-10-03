module;

#include <stdafx.h>

export module ComVars;

export struct Screen
{
    float fWidth = 800.0f;
    float fHeight = 600.0f;
    float fAspectRatio = 4.0f / 3.0f;
    float fBaseWidth = 800.0f;
    float fInvBaseWidth = 1.0f / 800.0f;
} Screen;

export int nFPSLimit;
export bool bWidescreenHUD, bWidescreenMap, bZoomFMVs;
export bool bNoCutsceneBorderAnimation;
export int nCutsceneBorders;
export bool bChangeDrawDistance;
export float fDrawDistance, fDrawDistanceMin, fDrawDistanceMax;
export float fCreditsTextX, fMapPlayerWidth, fMenuOffset, fMovieAspect = 1.0f;
export enum { MAFIA_1_0_ENG, MAFIA_1_1_ENG, MAFIA_1_2_ENG };
export int nGameVersion;
export int32_t *pResolutionWidth, *pResolutionHeight;

export template<class T> T& Field(uintptr_t object, size_t offset)
{
    return *reinterpret_cast<T*>(object + offset);
}

export HMODULE GetLS3DF()
{
    for (auto name : { L"LS3DF.dll", L"LSV11.dll", L"LSV10.dll" })
        if (auto hModule = GetModuleHandleW(name))
            return hModule;
    return nullptr;
}

