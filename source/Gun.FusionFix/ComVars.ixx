module;

#include <stdafx.h>
#include <d3d9.h>

export module ComVars;

export struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    float fHudScaleX;
    float fHudOffset;
    int Width43;
} Screen;

export GameRef<uint32_t> dword_6B759C_X;
export GameRef<uint32_t> dword_6B75A0_Y;
export GameRef<float> DefaultAR;
export GameRef<uint8_t*> CutsceneOwner;
export GameRef<float> HudScale;
export GameRef<uint8_t> LetterboxFlag;
export GameRef<uint8_t> ResetRequested;
export GameRef<uint8_t> GameActive;
export GameRef<D3DPRESENT_PARAMETERS> PresentationParameters;
export GameRef<IDirect3D9*> Direct3D;
export GameRef<void*> MenuManager;
export GameRef<void*> MenuSymbols;
export GameRef<int> MenuLanguage;
