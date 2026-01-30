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

export bool* bPause = nullptr;
export bool* bCutscene = nullptr;
export uint32_t* nLoading = nullptr;

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

export int CurrentCaseName = 0;
export int CurrentMissionName = 0;

export enum eCaseNames
{
    Mision0,
    Intro,
    BC01,
    BC02,
    BC03,
    BC04,
    EndGame,
    CIKingMission,
    CIMadamMission,
    CICabbieMission,
    FC,
    RedmanBonus,
    Police_Challenges,
    StreetRacing,
    SC03,
};

export enum eMissionNames_BC03
{
    BC3_Prologue,
    BC3_M0F,
    BC3_M1,
    BC3_M1F,
    BC3_M2,
    BC3_M2F,
    BC3_M4,
    BC3_M4F,
    BC3_M5,
    BC3_M5F,
    BC3_M6,
    BC3_M7,
    BC3_M8,
};
