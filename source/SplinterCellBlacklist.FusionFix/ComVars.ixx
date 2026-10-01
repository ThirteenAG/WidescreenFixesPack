module;

#include <stdafx.h>
#include <random>

export module ComVars;

export HWND WindowHandle = nullptr;

// UWindowsViewport, set by the viewport's input update every frame
export uintptr_t GameViewport = 0;

// The mouse moves a menu cursor instead of the camera (UWindowsViewport WndProc, WM_MOUSEMOVE: +510h, +514h set or +5CCh clear)
export bool IsMenuCursor()
{
    if (!GameViewport)
        return false;
    auto field = [](ptrdiff_t offset) { return *reinterpret_cast<int*>(GameViewport + offset); };
    return field(0x510) != 0 || field(0x514) != 0 || field(0x5CC) == 0;
}

// Window mode (UWindowsClient +B4h, WindowStyleFinal in videoSettings.ini, the video options menu has a copy)
export enum eWindowStyle
{
    Windowed,
    ExclusiveFullscreen,
    Borderless
};
export int* pWindowStyle = nullptr;

// Viewport resolution, set by UD3DRenderDevice::SetRes
export int* pViewportResolutionWidth = nullptr;
export int* pViewportResolutionHeight = nullptr;

export constexpr float fDefaultAspectRatio = 16.0f / 9.0f;

export float GetAspectRatio()
{
    if (!pViewportResolutionWidth || !pViewportResolutionHeight || *pViewportResolutionWidth <= 0 || *pViewportResolutionHeight <= 0)
        return fDefaultAspectRatio;
    return static_cast<float>(*pViewportResolutionWidth) / static_cast<float>(*pViewportResolutionHeight);
}

export enum eGameMode
{
    CAMPAIGN,
    PALADIN,
    HUNTER,
    GHOST,
    EXTRACTION,
    COOP,
};
export int CurrentGameMode = -1;

export int GetRandomInt(int rangeBegin, int rangeEnd)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(rangeBegin, rangeEnd);
    return dis(gen);
}

export bool (WINAPI* GetOverloadedFilePathA)(const char* lpFilename, char* out, size_t out_size) = nullptr;

