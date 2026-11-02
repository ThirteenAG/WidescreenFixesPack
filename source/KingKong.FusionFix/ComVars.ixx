module;

#include "stdafx.h"

export module ComVars;

export struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int32_t Width43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fCutOffArea;
    float fFMVScale;
    bool bSignatureEdition;
    float fRawInputMouse;
    float fTextScale;
    float fTextBox;
} Screen;

export int bHideUntexturedObjects = false;
export bool bDisableBlur = true;
export bool bVSync = true;
export bool wasUsingLeftStick = false;
export uint32_t gCurrentLevel = 0;
export std::atomic<uint32_t> gMenuTick = 0;

// Input
export GameRef<float> aMouseX;
export GameRef<float> aMouseY;
export GameRef<uintptr_t> pSettings;                // the game's settings object
export ProtectedGameRef<uint32_t> bAppInactive;     // the game's pause while its window is in the background

// Game state
export GameRef<uintptr_t> pBarsOwner;               // its +0xCC is the cinematic bars state
export GameRef<float> fTimeStep;
export GameRef<int32_t> nReloadPauseCounter;        // frames until the scheduled pause after a world reload
export GameRef<char> szAspectRatioLabel;            // the aspect ratio option's text in the settings menu

// Textures: the Xbox 360 texture decoder of XenonTexture.dll
export using XeUncompressTextureFn = int(__cdecl*)(void*, int, void*);
export GameRef<XeUncompressTextureFn> pXeTexture_UncompressTexture;

// Native menu (Gamer's Edition)
export GameRef<void*> pCurrentBehavior;
export GameRef<void*> pCurrentMenuOwner;
export GameRef<int> nMenuState;
export GameRef<uint32_t> nLocale;
export GameRef<void*> pOptionsTarget;
export GameRef<std::array<uint32_t, 256>> aGlyphMap;
export GameRef<void*> pScratchObject;
export GameRef<uintptr_t> pTextureTable;
export GameRef<void*> pFontCache;
export GameRef<int> nFontCacheCapacity;
export GameRef<int> nFontCacheCount;
export GameRef<uint8_t> bMenuReady;
