module;

#include "stdafx.h"

export module ComVars;

export struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fHudOffset;
    float fAspectRatio;
    float fHudScaleX;
    float fHudScaleX2;
    float fTextScaleX;
    int32_t FilmstripScaleX;
    int32_t FilmstripOffset;
    uint32_t pFilmstripTex;
    std::filesystem::path szLoadscPath;
} Screen;

export int32_t gBlacklistIndicators = 0;
export uint32_t bLightSyncRGB = false;
export float gVisibility = 1.0f;
export uint32_t gColor = 0;
export uint32_t curAmmoInClip = 1;
export uint32_t curClipCapacity = 1;

export namespace UObject
{
    std::unordered_map<std::wstring, std::wstring> objectStates;
    std::wstring_view GetState(const std::wstring& type)
    {
        auto it = objectStates.find(type);
        return (it != objectStates.end()) ? std::wstring_view(it->second) : std::wstring_view(L"");
    }
}
