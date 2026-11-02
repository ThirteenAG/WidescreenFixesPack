module;

#include <stdafx.h>

export module ComVars;

export struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    float fAspectRatioDiff;
    int Width43;
    float fWidth43;
    float fHudScale;
} Screen;


export ProtectedGameRef<int> EngineState;
export GameRef<uintptr_t> Controller;
// Game state: 0 and 1 play, 2 and above menus
export GameRef<int> GameMode;
// The player; control mode at +9: 1 normal, 2 guns, 3 fight, 4 stealth, 5 driving
export GameRef<uint8_t*> PlayerObject;
// Action flags held this update (the game's controller state)
export GameRef<uint32_t> HeldFlags;
export ProtectedGameRef<int> HudLeftBound, HudRightBound;
