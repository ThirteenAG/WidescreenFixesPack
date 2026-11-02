module;

#include "stdafx.h"
#include <d3d9.h>
#include <queue>
#include <functional>
#include <unordered_set>
#include <atomic>

export module ComVars;

export HWND hGameWindow = nullptr;
export void (*SuspendGameMouse)(bool) = nullptr;
export void (*ProcessWindowResize)() = nullptr;
export std::atomic<bool> FullscreenVideo = false;
export bool (*NativeMenuHelp)(bool) = nullptr;

export bool bSkipIntro;
export bool bWindowedMode;

export std::atomic<float> gVisibility{ 1.0f };
export std::atomic<int32_t> gBlacklistIndicators{ 0 };
export bool bDisableBlackAndWhiteFilter = false;
export bool bBlacklistControlScheme = true;
export int BackBufferWidth = 0;
export int BackBufferHeight = 0;
export bool bEnableSplitscreen = false;
export void* (*GetMouseViewport)() = nullptr; // split screen: the viewport the mouse is player 1's in, null for the usual one
export bool bInstance1 = true;
// The number of pads the game found (dword_1431E4C)
export GameRef<uint32_t> padsFound;

export constexpr float fDefaultAspectRatio = 16.0f / 9.0f;

export auto GetAspectRatio = []() -> float
{
    if (!BackBufferWidth || !BackBufferHeight)
        return fDefaultAspectRatio;
    return static_cast<float>(BackBufferWidth) / static_cast<float>(BackBufferHeight);
};

export float GetPresentationAspectRatio()
{
    RECT client{};
    if (hGameWindow && GetClientRect(hGameWindow, &client) && client.right > 0 && client.bottom > 0)
        return static_cast<float>(client.right) / static_cast<float>(client.bottom);
    return GetAspectRatio();
}

export bool (WINAPI* GetOverloadedFilePathA)(const char* lpFilename, char* out, size_t out_size) = nullptr;
