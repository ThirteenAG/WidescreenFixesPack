module;

#include <stdafx.h>
#include <d3d9.h>

export module ComVars;

export struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHudScale;
    float fHudOffset;
    float fTextOffset;
    float fFMVOffset;
    float fInvMenuHeartLineEnd; // On-Screen Range: -0.5 to 0.5
    float fInvMenuHeartLineWidth; // Full Screen Width is 0.002
    float fQuadUnits; // 2D quad corner steps per pixel
} Screen;


export GameRef<D3DPRESENT_PARAMETERS> PresentationParameters;
export GameRef<uint8_t> MenuManager;
export GameRef<uint32_t> MenuHistoryCount;
export GameRef<uint8_t> MenuHistory;
export GameRef<void*> FontBackend;
export GameRef<uint32_t> FontMask;
export GameRef<uint8_t> FontPool;
