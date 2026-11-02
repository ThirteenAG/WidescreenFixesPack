module;

#include <stdafx.h>

export module ComVars;

export struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    float fHudOffset;
    int32_t Width43;
    float TextOffset;
    int32_t FullscreenOffsetX;
    int32_t FullscreenOffsetY;
} Screen;


export ProtectedGameRef<float> CameraLens;
export GameRef<int> GameMode;
export GameRef<int> GamePhase;
export GameRef<std::array<int, 44>> ControllerBindings;
export GameRef<uint32_t> FontFlags;
export GameRef<uint32_t> FontIndex;
export GameRef<uint16_t> GlyphHeight;
export GameRef<std::array<int16_t, 32>> DefaultHeights;
export GameRef<uint32_t> MenuPanelHeight;
export GameRef<std::array<uint8_t, 29>> MenuState;
export ProtectedGameRef<int16_t> MenuSelection;
export GameRef<uint8_t> GameLanguage;
export GameRef<int> ConfirmMask;
export GameRef<int> BackMask;
export GameRef<uint32_t> dword_A36494;
export GameRef<uint32_t> dword_944F58;
export GameRef<int8_t> byteBarsOffsetY;
export GameRef<uint32_t> dword_96ED1C;
export GameRef<uint16_t> word_9467F0;
export GameRef<uint8_t> dword_01F7E3C4;
export GameRef<uint32_t> CurrentImage;
