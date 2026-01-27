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
    float fHudOffset;
    std::optional<float> fHudAspectRatioConstraint;
    float fWidescreenHudOffset;
    float fWidescreenHudOffsetPhone;
    float fWidescreenHudOffsetRadar;
} Screen;

export float fFpsLimit;
export int32_t nFrameLimitType;

export enum CameraMode
{
    CameraOnFoot = 0,
    CameraPrecisionAim = 6,
    CameraCrosshair = 11,
    CameraInVehicle = 13,
};

export int8_t CurrentCameraMode = -1;
