#include <windows.h>
#include "CDraw.h"
#include "RenderWare.h"
#include "game.h"

extern float fCustomAspectRatioHor, fCustomAspectRatioVer;
extern float fHudWidthScale, fHudHeightScale;
extern float fCustomWideScreenWidthScaleDown;
extern float fCustomWideScreenHeightScaleDown;
extern float fRadarWidthScale, fCustomRadarWidthScale, fCustomRadarRingWidthScale, fCustomRadarRingHeightScale, fSubtitlesScale;
extern RsGlobalType* RsGlobal;

float*  CDraw::pfScreenAspectRatio;
float*  CDraw::pfScreenFieldOfView;

extern float fWideScreenWidthScaleDown;
extern float fDynamicScreenFieldOfViewScale;
extern float fScreenFieldOfViewVStd;

extern bool bRestoreCutsceneFOV;
extern float fCarSpeedDependantFOV;
extern bool bDontTouchFOV;
extern float fFOVControlValue;
extern int FOVControl;
extern float fRadarScaling;

extern float fEmergencyVehiclesFix;

extern int(__cdecl* FindPlayerVehicle)();
extern bool* bIsInCutscene;

void CDraw::CalculateAspectRatio()
{
	if (!fCustomAspectRatioHor && !fCustomAspectRatioVer)
	{
		*pfScreenAspectRatio = (float)RsGlobal->MaximumWidth / (float)RsGlobal->MaximumHeight;
	}
	else {
		*pfScreenAspectRatio = (float)fCustomAspectRatioHor / (float)fCustomAspectRatioVer;
	}

	fWideScreenWidthScaleDown = (SCREEN_SCALE_WIDTH) / (*pfScreenAspectRatio / SCREEN_AR_NARROW);

	fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE((float)atan(tan(DEGREE_TO_RADIAN(fScreenFieldOfViewVStd * 0.5f)) * *pfScreenAspectRatio))
		* (1.0f / SCREEN_FOV_HORIZONTAL);
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