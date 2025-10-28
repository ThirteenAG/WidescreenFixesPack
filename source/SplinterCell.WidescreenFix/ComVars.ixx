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
    uint32_t nFMVWidescreenMode;
} Screen;

export union FColor
{
    uint32_t RGBA;
    struct
    {
        uint8_t B, G, R, A;
    };
};

export std::vector<std::pair<const std::wstring, std::wstring>> ResList =
{
    { L"640x480",   L"" },
    { L"800x600",   L"" },
    { L"1024x768",  L"" },
    { L"1280x1024", L"" },
    { L"1600x1200", L"" },
};

export const wchar_t* a640x480 = nullptr;
export const wchar_t* a800x600 = nullptr;
export const wchar_t* a1024x768 = nullptr;
export const wchar_t* a1280x1024 = nullptr;
export const wchar_t* a1600x1200 = nullptr;

export uint32_t nFMVWidescreenMode;
export std::vector<uintptr_t> EchelonGameInfoPtrs;

export bool bSkipIntro = false;
export bool bPlayingVideo = false;
export bool bPressStartToContinue = false;
export bool bSkipPressStartToContinue = false;

export namespace UObject
{
    std::unordered_map<std::wstring, std::wstring> objectStates;
    std::wstring_view GetState(const std::wstring& type)
    {
        auto it = objectStates.find(type);
        return (it != objectStates.end()) ? std::wstring_view(it->second) : std::wstring_view(L"");
    }
}

// Tolerance for floating-point comparisons (±1.5 units)
const float CANVAS_TOLERANCE = 1.5f;

// Helper function for tolerance-based float comparison
export inline bool FloatEqual(float a, float b, float tolerance = CANVAS_TOLERANCE)
{
    return std::abs(a - b) < tolerance;
}

// Helper function for tolerance-based range checks
export inline bool FloatInRange(uint32_t value, uint32_t minVal, uint32_t maxVal, float tolerance = CANVAS_TOLERANCE)
{
    return static_cast<int32_t>(value) >= static_cast<int32_t>(minVal) - tolerance &&
        static_cast<int32_t>(value) <= static_cast<int32_t>(maxVal) + tolerance;
}