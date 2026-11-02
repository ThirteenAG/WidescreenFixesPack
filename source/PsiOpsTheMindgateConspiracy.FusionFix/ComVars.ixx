module;
#include <stdafx.h>

export module ComVars;

export struct Screen
{
    int32_t Width;
    int32_t Height;
} Screen;

export namespace PsiOpsCamera
{
    GameRef<uint8_t> cinematicState;
    GameRef<float> gameplayFov;
}
