module;

#include <stdafx.h>

export module ComVars;

export bool ProcessExiting = false;

export struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    int32_t nWidth43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fHudOffsetReal;
    float fFMVAspectRatio = 4.0f / 3.0f;
    float fFMVOffsetH;
    float fFMVOffsetV;
    float fRadarVerticalOffset;
    bool bStretch;
    std::optional<float> fHudAspectRatioConstraint;
    float fWidescreenHudOffset;
    float fFOV;
    float fIniFOV;
    bool bUnstretchedText = true;
    bool bBinocularsActive = false;     // set by the binoculars HUD draw, consumed by SetCamera

    void AdjustToRes(uint32_t x, uint32_t y)
    {
        nWidth = x;
        nHeight = y;
        fWidth = static_cast<float>(nWidth);
        fHeight = static_cast<float>(nHeight);
        fAspectRatio = fWidth / fHeight;
        nWidth43 = static_cast<uint32_t>(fHeight * (4.0f / 3.0f));
        fWidth43 = static_cast<float>(nWidth43);
        fHudOffsetReal = (fWidth - fHeight * (4.0f / 3.0f)) / 2.0f;
        fHudScale = 1.0f / (((4.0f / 3.0f)) / (fAspectRatio));
        fHudOffset = (((600.0f * fAspectRatio) - 800.0f) / 2.0f) / fHudScale;
        fRadarVerticalOffset = fHudOffset * (4.0f / 3.0f);
        fWidescreenHudOffset = -CalculateWidescreenOffset(fWidth, fHeight, 800.0f, 600.0f, 3.5f);
        if (fHudAspectRatioConstraint.has_value() && std::isfinite(*fHudAspectRatioConstraint))
        {
            const auto value = ClampHudAspectRatio(*fHudAspectRatioConstraint, fAspectRatio);
            fWidescreenHudOffset = -CalculateWidescreenOffset(fHeight * value, fHeight, 800.0f, 600.0f, 3.5f);
        }
        fWidescreenHudOffset /= fHudScale;
    }

    void AdjustFMVRes(uint32_t w, uint32_t h)
    {
        fFMVAspectRatio = static_cast<float>(w) / static_cast<float>(h);
        fFMVOffsetH = (((600.0f * fFMVAspectRatio) - 800.0f) / 2.0f) / fHudScale;
        fFMVOffsetV = (600.0f - (600.0f / (1.0f / (((4.0f / 3.0f)) / (fFMVAspectRatio))))) / 2.0f;
    }
} Screen;

