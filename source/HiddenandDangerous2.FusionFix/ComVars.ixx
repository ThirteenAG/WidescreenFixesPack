module;

#include <stdafx.h>

export module ComVars;

export int WindowMode{};

export struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    int32_t Width43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fHudCameraX{ 1.0f };
    float fHudCameraY{ 1.0f };
    float fCursorScaleX{ 1.0f / 1024.0f };
} Screen;

export int UiDepth{};
export GameRef<int8_t> SetupByte;
export GameRef<int32_t> SetupHeight;
export GameRef<int32_t> SetupWidth;
export GameRef<uint16_t> PostWindowValue;
export GameRef<uint8_t*> GameCamera;
export GameRef<uint8_t> CutsceneActive;
export GameRef<uintptr_t*> HudDevice;
export GameRef<HANDLE> FocusWorker;
export GameRef<HWND> FocusWindow;
export GameRef<char> FocusTitle;
export GameRef<float> curs_pos_x;
export GameRef<float> curs_pos_y;
