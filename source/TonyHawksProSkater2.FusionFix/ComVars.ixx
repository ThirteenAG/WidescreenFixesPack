module;

#include <stdafx.h>
#include <ddraw.h>

export module ComVars;

export GameRef<int> NativeIndex;
export GameRef<int16_t> ReplayPlayers;
export GameRef<uint8_t> Demo;

export GameRef<uint8_t*> HudPool;
export GameRef<int> HudPlayers;
export GameRef<int> HudGameMode;
export GameRef<int> HudViewMode;
export GameRef<int32_t> dword_29D6FE4;
export GameRef<int32_t> dword_29D6FE8;
export GameRef<int32_t> dword_55ED00;
export GameRef<int32_t> dword_55ED18;
export GameRef<int32_t> dword_5606CC;
export GameRef<int32_t> dword_5606D0;
export GameRef<uint32_t> dword_29D6FF0;
export GameRef<int> gameExit;
export GameRef<int> startScreen;
export GameRef<IDirectDrawSurface7*> CachedBack;

export GameRef<int> LastBinding;
export GameRef<int> KickBinding;
export struct THPS2MovieVertex
{
    float x, y, z, rhw;
    uint32_t color;
    float u, v;
};
export GameRef<std::array<THPS2MovieVertex, 8>> MovieVertices;
export GameRef<uint8_t> MovieSplit;
export GameRef<uint32_t> MovieWidth;
export GameRef<uint32_t> MovieHeight;
export GameRef<uint32_t> MovieTextureWidth;
export GameRef<DDSURFACEDESC2> MovieSurface;
export GameRef<uint8_t> TextureMovie;
export GameRef<int> MovieRenderMode;
export GameRef<void*> PauseRoot;
export GameRef<int> PauseState;
export GameRef<int> PauseMode;
export GameRef<uint8_t> GamePauseFont;
export GameRef<uint8_t> TitleChoose;
export GameRef<uint8_t> PauseChoose;
export GameRef<uint8_t> TitleBack;
export GameRef<uint8_t> PauseBack;

export struct ScreenState
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    int32_t Width43;
    int32_t Height43;
    float fAspectRatio;
    int32_t FOV;
    float fHudOffset;
    float fHudOffsetY;
    float fHudOffsetReal;
    float fHudOffsetRealY;
};

export ScreenState Screen;

