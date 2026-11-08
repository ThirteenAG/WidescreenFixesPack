module;

#include <stdafx.h>
#include <minidx8/d3d8.h>

export module ComVars;

// Everything the fix derives from the render size. The game lays its 2D out on a 640x480 canvas; the fix keeps
// that canvas as a 4:3 box (Width43 wide) in the middle of the screen.
export struct ScreenState
{
    int Width = 640;
    int Height = 480;
    int Width43 = 640;
    int WidthFMV = 640;
    int HeightFMV = 480;
    int HeightHUD = 480;
    int OffsetX = 0;
    int OffsetY = 0;
    bool AspectRatioAffected = false;
    bool PreserveHorizontalPosition = false;
    int ContinuePositionChange = 0;
    float fWidth = 640.0f;
    float fHeight = 480.0f;
    float fHalfWidth = 320.0f;
    float fHalfHeight = 240.0f;
    float fAspectRatio = 4.0f / 3.0f;
    float fConditionalAspectRatio = 4.0f / 3.0f;
    float fZoomFactor = 1.0f;
    float fCameraScaleX = 1.0f;
    float fHudZoomFactor = 1.0f;
    float fHudScale = 1.0f;
    float fFovFactor = 1.0f;
    float fDustWidth = 40.0f;           // special stage dust sprites, 40 units of a 4:3 height
    float fWideXPerUnit = 1.0f / 640.0f; // pixels to canvas x for the 2D that spans the whole width
    float fFlareHalfSize = 0.5f;        // half the lens flare sprite size, grown with the resolution
} Screen;

export GameRef<uintptr_t> HudGameState;
export GameRef<IDirect3DDevice8*> HudDevice;
export GameRef<uintptr_t> GameplayState;
export GameRef<uintptr_t> RenderCamera;
export GameRef<uintptr_t> UpdateGameState;
export ProtectedGameRef<uint32_t> ResolutionX;
export ProtectedGameRef<uint32_t> ResolutionY;
export ProtectedGameRef<uint32_t> ResolutionX2;
export ProtectedGameRef<uint32_t> ResolutionY2;
export ProtectedGameRef<float> MaestroResX, MaestroResY;

// Number of the movie that is playing, -1 when none
export GameRef<int32_t> PlayingMovie;

// The game's main window
export GameRef<HWND> GameWindow;

// Sizes of the 640x480 canvas when it is fitted into the screen as a whole
export namespace Canvas
{
    // Pixels per canvas unit
    inline float Scale()
    {
        return std::min(Screen.fWidth / 640.0f, Screen.fHeight / 480.0f);
    }

    inline float OffsetX()
    {
        return (Screen.fWidth - 640.0f * Scale()) * 0.5f;
    }

    inline float OffsetY()
    {
        return (Screen.fHeight - 480.0f * Scale()) * 0.5f;
    }
}
