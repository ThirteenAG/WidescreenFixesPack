module;

#include <stdafx.h>
#include <minidx8/d3d8.h>

export module ComVars;

export struct Screen
{
    int Width;
    int Width43;
    int WidthFMV;
    int OffsetX;
    int Height;
    int HeightFMV;
    int HeightHUD;
    int OffsetY;
    bool AspectRatioAffected;
    bool PreserveHorizontalPosition;
    int ContinuePositionChange;
    float fWidth;
    float fHeight;
    float fHalfWidth;
    float fHalfHeight;
    float fAspectRatio;
    float fConditionalAspectRatio;
    float fZoomFactor;
    float fCameraScaleX;
    float fHudZoomFactor;
    float fHudScale;
} Screen;


export GameRef<uintptr_t> HudGameState;
export GameRef<IDirect3DDevice8*> HudDevice;
export GameRef<uintptr_t> GameplayState;
export GameRef<uintptr_t> RenderCamera;
export GameRef<uintptr_t> UpdateGameState;
export GameRef<int32_t> SyncMovieID;
export ProtectedGameRef<uint32_t> ResolutionX;
export ProtectedGameRef<uint32_t> ResolutionY;
export ProtectedGameRef<uint32_t> ResolutionX2;
export ProtectedGameRef<uint32_t> ResolutionY2;
export ProtectedGameRef<float> MaestroResX, MaestroResY, OneDiv640;
export ProtectedGameRef<uint32_t> WindowSizeX;
