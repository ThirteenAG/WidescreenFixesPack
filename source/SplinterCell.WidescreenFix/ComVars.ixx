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
    float HUDScaleX;
    float fHudOffset;
    float fHudOffsetRight;
    float fFMVoffsetStartX;
    float fFMVoffsetEndX;
    float fFMVoffsetStartY;
    float fFMVoffsetEndY;
    float fIniHudOffset;
    float fWidescreenHudOffset;
    bool bHudWidescreenMode;
} Screen;

export union FColor
{
    uint32_t RGBA;
    struct
    {
        uint8_t B, G, R, A;
    };
};

export uint32_t nFMVWidescreenMode;
export std::vector<uintptr_t> EchelonGameInfoPtrs;
export bool isIngameText;
