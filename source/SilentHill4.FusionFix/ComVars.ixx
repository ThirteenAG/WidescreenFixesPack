module;

#include "stdafx.h"
#include <minidx8/d3d8.h>

export module ComVars;

// The render camera: +0x40 angle (horizontal, 4:3), +0x44 zoom (aspect scale), +0x280 dirty flags
export struct RenderCameraData
{
    uint8_t unknown0[0x40];
    float fAngle;
    float fZoom;
    uint8_t unknown48[0x280 - 0x48];
    uint32_t nFlags;
};

// A header of one of the flush's two vertex batches (12 bytes apart)
export struct FlushBatchCount
{
    uint32_t count;
    uint32_t unknown4[2];
};

// Display
export ProtectedGameRef<int32_t> bFullscreen;
export GameRef<int32_t> nDisplayWidth;
export GameRef<int32_t> nDisplayHeight;
export GameRef<RenderCameraData> RenderCamera;
export GameRef<uint8_t*> pCurrentCamera;
export GameRef<int32_t> aCinemaBarFlags[2];
export GameRef<std::array<FlushBatchCount, 2>> aFlushBatchCounts;
export GameRef<std::array<std::array<float, 70000>, 2>> aFlushBatchVertices;      // 28-byte vertices {x, y, z, w, colour, u, v}
export GameRef<IDirect3DBaseTexture8*> pScreenCopyTexture;
export GameRef<std::array<IDirect3DBaseTexture8*, 15>> aRenderTargetTextures;

// Game state
export GameRef<std::array<uint8_t, 0xC8>> aPlayerStatus;     // per-frame player summary: +0 position, +0x84 yaw, +0xC4 status code
export GameRef<uint8_t*> pCharacter;                        // current character: +0x23C lock-on target
export GameRef<std::array<float, 21>> aMoveIntent;          // movement intent: +0x08 desired yaw, +0x50 ready weapon held
export GameRef<std::array<float, 16>> aCameraMatrix;        // the camera task's result, applied next frame
export GameRef<float> fCameraFov;                           // radians
export GameRef<int32_t> bThirdPerson;
export GameRef<int32_t> nGameMode;                          // 0 play, 1 event, 2 overlay
export GameRef<int32_t> nSceneFlags;                        // bit 0: scripted scene
export GameRef<int32_t> nSceneFlag2;
export GameRef<int32_t> bControlLocked;
export GameRef<int32_t> bScriptedCamera;
export GameRef<int32_t> bAutoWalk;
export GameRef<int32_t> bCameraOverride;
export GameRef<int32_t> bGameOver;
export GameRef<float> fMoveBasisLength;                     // camera-relative movement basis
export GameRef<float> fMoveBasisYaw;

// Hands the camera task's result to the renderer
export void(__cdecl* ApplyCamera)() = nullptr;

// Input
export GameRef<std::array<uint32_t, 29>> aPadBindings;      // [21] digital, then the analog [8]
export GameRef<int32_t> nMouseX;
export GameRef<int32_t> nMouseY;
export GameRef<std::array<int32_t, 45>> aPadMessageIds;     // message id of each pad element of the configuration screen

// System
export GameRef<std::array<uint8_t, 36>> aOptionsConfig;     // the options block
export GameRef<int32_t> nCpuClass;
export GameRef<int32_t> nVideoMemory;
export GameRef<int32_t> bLanguageLeftArrowHovered;
export GameRef<int32_t> bLanguageRightArrowHovered;
