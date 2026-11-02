module;

#include "stdafx.h"

export module ComVars;

export struct Size2D
{
    int32_t w;
    int32_t h;
};

// The requested display mode, then the current 2D target
export struct DisplaySizes
{
    Size2D mode;
    Size2D target;
};

// A vertex of the final present quad (24 bytes)
export struct PresentVertex
{
    float x;
    float y;
    float rest[4];
};

// An entry of the auto-aim candidate list: +0 object, +4 kind, ...
export struct AimCandidate
{
    uint8_t data[20];
};

// Display
export ProtectedGameRef<uint8_t> bFullscreen;
export ProtectedGameRef<uint8_t> bForceDisplayReset;
export GameRef<Size2D> RenderTargetSize;            // the off-screen render target
export GameRef<Size2D> PresentSize;                 // the present parameters' back buffer
export GameRef<DisplaySizes> DisplaySize;
export GameRef<Size2D> VideoSize;                   // the movie frame
export GameRef<std::array<PresentVertex, 4>> aPresentQuad;
export GameRef<float> fPresentQuadRight;            // v2.x
export GameRef<float> fPresentQuadBottom;           // v3.y
export GameRef<float> fPresentQuadBottom2;          // v2.y
export GameRef<float> aBlendQuadX[4];
export GameRef<float> aBlendQuadY[4];
export GameRef<float> fCursorX;

// 3D view
export GameRef<float> fProjectionDistance;          // "h" on the 512x448 screen; the game's getter reads it too
export GameRef<float> fProjectionBase;

// Game state
export GameRef<uint8_t*> pPlayer;                   // player object: +0 position, +0x14 yaw
export GameRef<uint8_t> bAiming;
export GameRef<int32_t> nEventId;                   // -1 = no event script
export GameRef<int32_t> nCameraMode;                // 0 = gameplay camera
export GameRef<uint32_t> nCameraFlags;              // bits 0/1: a script forces the camera position or target
export GameRef<uint8_t> nRenderMode;                // 2 = in-engine cutscene
export GameRef<uint8_t> bPauseActive;
export GameRef<float> fPlayerDt;                    // seconds of the current player tick
export GameRef<std::array<uint8_t, 2>> aHeadingSlot;        // byte index into aHeadingReference, then the latch byte
export GameRef<std::array<float, 2>> aHeadingReference;     // two camera yaws used by camera-relative movement
export GameRef<int32_t> nLockedTarget;
export GameRef<int32_t> nAimCandidateCount;         // 20 - count
export GameRef<std::array<AimCandidate, 20>> aAimCandidates;

// Input
export GameRef<float> fStickLX;
export GameRef<float> fStickLY;
export GameRef<float> fStickRX;
export GameRef<float> fStickRY;
export GameRef<std::array<uint8_t, 64>> aPlayerInput;       // the input record of the current frame
export GameRef<uint8_t*> pPads;                     // logical pads of the input library, 344 bytes each

// Options screen (offsets from the selection byte)
export GameRef<int8_t> nOptionsSelection;           // row 1..28
export GameRef<int8_t> nOptionsSelectionNext;
export GameRef<int8_t> nOptionsMode;                // 0 navigate, 1 sub-screen, 2 language reload, 3 confirm, 4 done
export GameRef<int8_t> nOptionsModeNext;
export GameRef<int8_t> nOptionsAction;              // 1 Enter, 2 Cancel, 3 Up, 4 Down, 5 Right, 6 Left, 7 next page, 8 previous page
export GameRef<std::array<int8_t, 29>> aOptionsPendingValues;  // [row]
export GameRef<std::array<uint8_t, 28>> aTextColours;       // RGBA x 7
export GameRef<std::array<uint8_t, 3>> aTextBackground;     // RGB

// Text functions, read before the button icons hook them
export void(__cdecl* DrawGlyphs)(const uint16_t*, int, int) = nullptr;
export int(__cdecl* MeasureLines)(int*, const uint16_t*) = nullptr;
