module;

#include "stdafx.h"

export module ComVars;

export struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHUDScaleX;
    double dHUDScaleX;
    double dHUDScaleXInv;
    float fHudOffset;
    float fHudOffsetRight;
    float fFMVoffsetStartX;
    float fFMVoffsetEndX;
    float fFMVoffsetStartY;
    float fFMVoffsetEndY;
    float fIniHudOffset;
    float fWidescreenHudOffset;
    bool bHudWidescreenMode;
    bool bOpsatWidescreenMode;
    uint32_t nPostProcessFixedScale;
    uint32_t nFMVWidescreenMode;
    uint32_t nShadowMapResolution;
    uint32_t nReflectionsResolution;
    uint32_t nBloomResolution;
} Screen;

export union FColor
{
    uint32_t RGBA;
    struct
    {
        uint8_t B, G, R, A;
    };
} gColor;

export std::vector<uintptr_t> EchelonGameInfoPtrs;
export uintptr_t pDrawTile;
export bool bSkipIntro = false;
export int EPlayerControllerState = -1;
export int EchelonMainHUDState = -1;
export int EGameInteractionState = -1;
export int EDoorMarkerState = -1;
export bool bPlayingVideo = false;
export bool bPressStartToContinue = false;
export bool bSkipPressStartToContinue = false;
export bool bKeyPad = false;
export bool bElevatorPanel = false;

export std::vector<std::pair<const std::wstring, std::wstring>> ResList =
{
    { L"640x480",   L"" },
    { L"800x600",   L"" },
    { L"1024x768",  L"" },
    { L"1280x1024", L"" },
    { L"1600x1200", L"" },
};

export
{
    constexpr auto s_Turret = 8330;
    constexpr auto s_KeyPadInteract = 8338;
    constexpr auto s_Zooming = 6942;
    constexpr auto s_PlayerSniping = 7059;
    constexpr auto s_UsingPalm = 8274;
    constexpr auto s_LaserMicTargeting = 10015;
    constexpr auto s_GameInteractionMenu = 8122;
    constexpr auto s_FirstPersonTargeting = 7060;
    constexpr auto s_RappellingTargeting = 7294;
    constexpr auto s_PlayerBTWTargeting = 7548;
    constexpr auto s_HOHFUTargeting = 8571;
}