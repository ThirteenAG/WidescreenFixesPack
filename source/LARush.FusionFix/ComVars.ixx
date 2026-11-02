module;
#include <stdafx.h>
#include <d3d9.h>

export module ComVars;

export struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int32_t nWidth43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fHudOffsetReal;
} Screen;

export namespace IntroSkip
{
    struct Movie
    {
        const char* name;
        int32_t state;
        int32_t skipState;
        int32_t skipMode;
    };

    GameRef<const Movie> pMovies;
}
export namespace VideoCrop
{
    GameRef<IDirect3DVertexBuffer9*> Vertices;
}
export namespace LARushWindow
{
    GameRef<IDirect3DDevice9*> Device;
    GameRef<int32_t> GameState;
    GameRef<int32_t> CursorDisplayCount;
}
export namespace LARushOverlay
{
    GameRef<int32_t> GamePaused;
}
export namespace LARushCamera
{
    GameRef<int32_t> CurrentCamera;
    GameRef<uint8_t> Cameras;
    GameRef<uint8_t*> Player;
}
export namespace LARushHud
{
    GameRef<int32_t> MinimapCamera;
}
export namespace LARushPadLabels
{
    GameRef<uint8_t> Bindings;
    GameRef<uint8_t*> MenuTable;
}
