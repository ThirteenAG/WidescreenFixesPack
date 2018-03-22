#include "..\stdafx.h"
#include "common.h"
#include "CDraw.h"

extern RsGlobalType* RsGlobal;
float*  CDraw::pfScreenAspectRatio;
float*  CDraw::pfScreenFieldOfView;

extern float fWideScreenWidthScaleDown;
extern float fCustomAspectRatioHor, fCustomAspectRatioVer;
extern float fEmergencyVehiclesFix;
extern float fFOVControlValue;
extern uint32_t* FOVControl;
extern float fCarSpeedDependantFOV;
extern float fRadarScaling;
extern bool bRestoreCutsceneFOV;
extern bool bDontTouchFOV;
extern bool* bIsInCutscene;
extern int(__cdecl* FindPlayerVehicle)();

void CDraw::CalculateAspectRatio()
{
    if (!fCustomAspectRatioHor && !fCustomAspectRatioVer)
    {
        *pfScreenAspectRatio = (float)RsGlobal->MaximumWidth / (float)RsGlobal->MaximumHeight;
    }
    else {
        *pfScreenAspectRatio = fCustomAspectRatioHor / fCustomAspectRatioVer;
    }

    fWideScreenWidthScaleDown = (1.0f / 640.0f) / (*pfScreenAspectRatio / (4.0f / 3.0f));
}

void CDraw::SetFOV(float fFactor)
{
    fEmergencyVehiclesFix = 70.0f / fFactor;

    FOVControl ? fFOVControlValue = *(float*)FOVControl : fFOVControlValue = 1.0f;

    if ((*bIsInCutscene == true && bRestoreCutsceneFOV) || bDontTouchFOV)
    {
        *pfScreenFieldOfView = fFactor * fFOVControlValue;
        return;
    }

    if (fCarSpeedDependantFOV)
    {
        if (FindPlayerVehicle())
        {
            *pfScreenFieldOfView = (AdjustFOV(fFactor, *pfScreenAspectRatio) * fFOVControlValue) + (fRadarScaling / fCarSpeedDependantFOV);
            return;
        }
    }

    *pfScreenFieldOfView = AdjustFOV(fFactor, *pfScreenAspectRatio) * fFOVControlValue;
}