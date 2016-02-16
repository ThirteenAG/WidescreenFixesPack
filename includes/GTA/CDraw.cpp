#include "..\stdafx.h"
#include "common.h"
#include "CDraw.h"
#define _USE_MATH_DEFINES
#include "math.h"

#define DEGREE_TO_RADIAN(fAngle) \
	((fAngle) * (float)M_PI / 180.0f)
#define RADIAN_TO_DEGREE(fAngle) \
	((fAngle) * 180.0f / (float)M_PI)

extern RsGlobalType* RsGlobal;
float*  CDraw::pfScreenAspectRatio;
float*  CDraw::pfScreenFieldOfView;
float fScreenFieldOfViewVStd = (2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(70.0f * 0.5f)) / (4.0f / 3.0f))));

extern float fWideScreenWidthScaleDown;
extern float fCustomAspectRatioHor, fCustomAspectRatioVer;
float fDynamicScreenFieldOfViewScale;
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
		*pfScreenAspectRatio = (float)fCustomAspectRatioHor / (float)fCustomAspectRatioVer;
	}

	fWideScreenWidthScaleDown = (1.0f / 640.0f) / (*pfScreenAspectRatio / (4.0f / 3.0f));
	fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE((float)atan(tan(DEGREE_TO_RADIAN(fScreenFieldOfViewVStd * 0.5f)) * *pfScreenAspectRatio)) * (1.0f / 70.0f);
}

inline float getDynamicScreenFieldOfView(float fFactor)
{
	fEmergencyVehiclesFix = 70.0f / fFactor;
	if (FOVControl)
	{
		fFOVControlValue = *(float*)FOVControl;
	}
	else
	{
		fFOVControlValue = 1.0f;
	}

	if ((!*bIsInCutscene == false && bRestoreCutsceneFOV) || bDontTouchFOV)
	{
		return fFactor * fFOVControlValue;
	}

	if (fCarSpeedDependantFOV)
	{
		if (FindPlayerVehicle())
		{
			return (fFactor * fFOVControlValue * fDynamicScreenFieldOfViewScale) + (fRadarScaling / fCarSpeedDependantFOV);
		}
	}

	return fFactor * fFOVControlValue * fDynamicScreenFieldOfViewScale;
}

void CDraw::SetFOV(float fFactor)
{
	*pfScreenFieldOfView = getDynamicScreenFieldOfView(fFactor);
}