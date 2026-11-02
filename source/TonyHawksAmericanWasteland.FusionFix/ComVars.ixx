module;

#include <stdafx.h>
#include <d3d9.h>

export module ComVars;

export GameRef<void*> NativeManager;
export GameRef<void*> NativeSymbols;
export GameRef<int> NativeLanguage;

export GameRef<float> HudNativeScaleX;
export GameRef<float> HudTextQuantum;
export GameRef<int> HudLayout;

export GameRef<GUID> GameSelected;
export GameRef<std::array<uint8_t, 1088>> PreviousPad;
export GameRef<std::array<uint8_t, 256>> PreviousKeys;
export GameRef<std::array<uint8_t, 256>> CurrentKeys;
export GameRef<std::array<int32_t, 25>> KeyboardMap;
export GameRef<int32_t> PreviousMouse;

export GameRef<uintptr_t> VibrationManager;
export GameRef<char> MetaTable;
export GameRef<uint8_t*> ButtonFont;

export GameRef<IDirect3DDevice9*> GameDevice;
export GameRef<uint8_t> NativeWindowed;
export GameRef<int32_t> dword_6D2224;
export GameRef<int32_t> dword_6D2228;
export GameRef<int32_t> dword_7F3A2C;
export GameRef<int32_t> dword_7F3A30;
export GameRef<int32_t> dword_787D88;
export GameRef<int32_t> GameYOffset;
export GameRef<float> GameScaleX;
export GameRef<float> GameScaleY;
export ProtectedGameRef<float> AspectImmediate;
export GameRef<uint8_t> Letterboxed;
export GameRef<D3DPRESENT_PARAMETERS> Presentation;
export GameRef<int32_t> dword_69B718;
export GameRef<int32_t> dword_681D14;

export struct ScreenState
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    int32_t Width43;
    float fAspectRatio;
    float fAspectRatioDiff;
    float fHUDScaleX;
    float fHudOffset;
    float fHudOffsetReal;
};

export ScreenState Screen;

