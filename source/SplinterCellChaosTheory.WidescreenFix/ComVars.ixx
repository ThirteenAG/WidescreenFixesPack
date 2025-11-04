module;

#include "stdafx.h"

export module ComVars;

export struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHUDScaleX;
    float fHudOffset;
    const float fHUDScaleXOriginal = 0.003125f;
    const float fHudOffsetOriginal = 1.0f;
    float fHUDScaleXDyn;
    float fHudOffsetDyn;
    float fTextScaleX;
    int32_t nHudOffsetReal;
    int32_t nScopeScale;
    float fFMVoffsetStartX;
    float fFMVoffsetEndX;
    float fFMVoffsetStartY;
    float fFMVoffsetEndY;
} Screen;

export union FColor
{
    uint32_t RGBA;
    struct
    {
        uint8_t B, G, R, A;
    };
};

export struct FLTColor
{
    float R, G, B, A = 1.0f;
    inline FLTColor()
    {
    }
    inline FLTColor(uint32_t color)
    {
        R = ((color >> 16) & 0xFF) / 255.0f;
        G = ((color >> 8) & 0xFF) / 255.0f;
        B = ((color) & 0xFF) / 255.0f;
        A = 1.0f;
    }
    bool empty()
    {
        return (R == 0.0f && G == 0.0f && B == 0.0f);
    }
    FLTColor& operator=(uint32_t color)
    {
        *this = FLTColor(color);
        return *this;
    }
};

export enum class GameLang : int
{
    English,
    French,
    German,
    Hungarian,
    Italian,
    Polish,
    Russian,
    Spanish
} eGameLang;

export bool bHudWidescreenMode;
export int32_t nWidescreenHudOffset;
export float fWidescreenHudOffset;
export bool bDisableAltTabFix;
export int32_t nShadowMapResolution;
export bool bEnableShadowFiltering;
export bool bPlayingVideo = false;
export bool bHackingGameplay = false;

export uint32_t* dword_1120B6BC = nullptr;
export uint32_t* dword_1120B6B0 = nullptr;
export uint32_t* dword_11223A7C = nullptr;
export HWND hGameWindow = NULL;
export bool bIsWindowed = false;

export namespace UObject
{
    std::unordered_map<std::wstring, std::wstring> objectStates;
    std::wstring_view GetState(const std::wstring& type)
    {
        auto it = objectStates.find(type);
        return (it != objectStates.end()) ? std::wstring_view(it->second) : std::wstring_view(L"");
    }

    wchar_t* (__fastcall* GetFullName)(void*, void*, wchar_t*) = nullptr;
}