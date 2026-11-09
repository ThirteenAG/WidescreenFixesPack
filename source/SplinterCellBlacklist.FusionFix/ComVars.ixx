module;

#include <stdafx.h>
#include <random>

export module ComVars;

export HWND WindowHandle = nullptr;
export void (*UpdateCameraCullBias)(float) = nullptr;
export std::atomic<bool> NativeMenuOpen{ false };
export std::atomic<bool> NativeHelpRequested{ false };

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
export GameRef<int> WindowStyleFinal;

// Viewport resolution, set by UD3DRenderDevice::SetRes
export GameRef<int> ViewportResolutionWidth;
export GameRef<int> ViewportResolutionHeight;

// The video options menu's resolution (DX9 exe; the DX11 one keeps it in the menu options)
export GameRef<int> MenuWidth;
export GameRef<int> MenuHeight;

// UI::SceneStart: the next Escape key release quits the game
export GameRef<uint8_t> EscapeQuits;
// The engine's tick counter, the input sign in waits for it to be past 60
export GameRef<int64_t> TickCounter;
// UI::SMISvMManual's state
export GameRef<uint8_t*> SMIState;
export GameRef<uint8_t*> NetOnlineManager;
export GameRef<uint8_t*> MatchManager;

// Split screen
export GameRef<int32_t> LastPad;
export GameRef<int32_t> GamepadMode;
export GameRef<uint8_t*> UIManager;
export GameRef<void**> CameraJob;
export GameRef<void***> LightJobs;
export GameRef<uint8_t*> ResourceDB;
export GameRef<uint8_t> Player1FullWidth;
export GameRef<uint8_t*> LoadoutGlobal;
export GameRef<uint32_t> GIsRequestingExit;

export constexpr float fDefaultAspectRatio = 16.0f / 9.0f;

// split screen is on (two viewports), each player's HUD is laid out by the game for its half
export bool bSplitscreen = false;

export float GetAspectRatio()
{
    if (!ViewportResolutionWidth.is_initialized() || !ViewportResolutionHeight.is_initialized() || ViewportResolutionWidth <= 0 || ViewportResolutionHeight <= 0)
        return fDefaultAspectRatio;
    return static_cast<float>(ViewportResolutionWidth) / static_cast<float>(ViewportResolutionHeight);
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
    if (rangeBegin > rangeEnd)
        std::swap(rangeBegin, rangeEnd);
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(rangeBegin, rangeEnd);
    return dis(gen);
}

export bool (WINAPI* GetOverloadedFilePathA)(const char* lpFilename, char* out, size_t out_size) = nullptr;

