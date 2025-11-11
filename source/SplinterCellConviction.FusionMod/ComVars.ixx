module;

#include "stdafx.h"
#include <d3d9.h>
#include <queue>
#include <functional>
#include <unordered_set>

export module ComVars;

export bool bSkipIntro;
export bool bWindowedMode;
export bool bDisableNegativeMouseAcceleration;
export bool bSkipSystemDetection;
export bool bForceCPUAffinityToAllCores;
export bool bPartialUltraWideSupport;

export float gVisibility = 1.0f;
export int32_t gBlacklistIndicators = 0;
export bool bDisableBlackAndWhiteFilter = false;
export bool bBlacklistControlScheme = true;
export IDirect3DDevice9* pDevice = nullptr;
export int BackBufferWidth = 0;
export int BackBufferHeight = 0;
export bool bVideoRender = false;
export std::list<int> OpenedVideosList;
export bool bEnableSplitscreen = false;
export bool bInstance1 = true;

export constexpr float fDefaultAspectRatio = 16.0f / 9.0f;

export auto GetAspectRatio = []() -> float
{
    if (!BackBufferWidth || !BackBufferHeight)
        return fDefaultAspectRatio;
    return static_cast<float>(BackBufferWidth) / static_cast<float>(BackBufferHeight);
};

export bool (WINAPI* GetOverloadedFilePathA)(const char* lpFilename, char* out, size_t out_size) = nullptr;
