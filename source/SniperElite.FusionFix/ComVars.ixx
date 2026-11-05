module;

#include <stdafx.h>

export module ComVars;

export struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    int32_t Width43;
    float fWidth43;
    float fHudOffset;
} Screen;


export namespace FMV
{
    struct VideoSize { uint32_t Width, Height; };
    struct Stream { const VideoSize* Video; };
}

export GameRef<int32_t> LoadingTexture;
export GameRef<int32_t> MovieHandle;
export GameRef<FMV::Stream**> MovieStreams;
export GameRef<int32_t> UpdateFont;
export GameRef<uint8_t> WindowedMode;
export GameRef<int32_t> StartupState;
export GameRef<uint32_t> StartupStep;
export GameRef<int32_t> LocalPlayers;
export ProtectedGameRef<float> GameplayFOV;
