#include "CDraw.h"
#include "RenderWare.h"
#include "game.h"
#include "..\injector\injector.hpp"
#include "..\injector\calling.hpp"
extern float fCustomAspectRatioHor, fCustomAspectRatioVer;
extern float fHudWidthScale, fHudHeightScale;
extern float fCustomWideScreenWidthScaleDown;
extern float fCustomWideScreenHeightScaleDown;
extern float fRadarWidthScale, fCustomRadarWidthScale, fCustomRadarRingWidthScale, fCustomRadarRingHeightScale, fSubtitlesScale;
extern RsGlobalType* RsGlobal;

float*  CDraw::pfScreenAspectRatio;
float*  CDraw::pfScreenFieldOfView;

extern float fScreenAspectRatioAcd;
extern float fInvScreenAspectRatioAcd;
extern float fScreenFieldOfViewHStd;
extern float fScreenFieldOfViewVStd;

extern float fWideScreenWidthRatio, fWideScreenHeightRatio;
extern float fWideScreenWidthPerc, fWideScreenHeightPerc;
extern float fWideScreenWidthDiff, fWideScreenHeightDiff;
extern float fWideScreenWidthMult, fWideScreenHeightMult;
extern float fWideScreenWidthScaleUp, fWideScreenHeightScaleUp;
extern float fWideScreenWidthScaleMid, fWideScreenHeightScaleMid;
extern float fWideScreenWidthScaleDown, fWideScreenHeightScaleDown;
extern float fWideScreenWidthScaled, fWideScreenHeightScaled;
extern float fWideScreenWidthScaledInv, fWideScreenHeightScaledInv;
extern float fDynamicScreenFieldOfViewScale;

extern int RestoreCutsceneFOV;
extern float fCarSpeedDependantFOV;
extern int DontTouchFOV;
extern float fFOVControlValue;
extern int FOVControl;
extern float fRadarScaling;

extern float fvcLogoScale, fCrosshairHeightScaleDown;
extern float fEmergencyVehiclesFix;
extern float fCrosshairPosFactor;

extern int(__cdecl* FindPlayerVehicle)();
extern DWORD IsInCutscene;

void CDraw::CalculateAspectRatio()
{
	if (!fCustomAspectRatioHor && !fCustomAspectRatioVer)
	{
		*pfScreenAspectRatio = (float)RsGlobal->MaximumWidth / (float)RsGlobal->MaximumHeight;
	}
	else {
		*pfScreenAspectRatio = (float)fCustomAspectRatioHor / (float)fCustomAspectRatioVer;
	}

	if (*pfScreenAspectRatio >= 1.0f) {
		fWideScreenWidthRatio = 1.0f / *pfScreenAspectRatio;
		fWideScreenHeightRatio = 1.0f;
		fWideScreenWidthPerc = fWideScreenWidthRatio * SCREEN_AR_STRETCH;
		fWideScreenWidthDiff = 1.0f - fWideScreenWidthPerc;
		fWideScreenWidthMult = 2.0f - fWideScreenWidthPerc;
		fWideScreenWidthScaleUp = SCREEN_SCALE_WIDTH * fWideScreenWidthMult;
		fWideScreenWidthScaleMid = SCREEN_SCALE_WIDTH * fWideScreenWidthDiff;
		fWideScreenWidthScaleDown = SCREEN_SCALE_WIDTH * fWideScreenWidthPerc;
		fWideScreenHeightPerc = 1.0f;
		fWideScreenHeightDiff = 0.0f;
		fWideScreenHeightMult = 1.0f;
		fWideScreenHeightScaleUp = SCREEN_SCALE_HEIGHT;
		fWideScreenHeightScaleMid = 0.0f;
		fWideScreenHeightScaleDown = SCREEN_SCALE_HEIGHT;
	}
	else 
	{
		fWideScreenWidthRatio = 1.0f;
		fWideScreenHeightRatio = *pfScreenAspectRatio;
		fWideScreenWidthPerc = 1.0f;
		fWideScreenWidthDiff = 0.0f;
		fWideScreenWidthMult = 1.0f;
		fWideScreenWidthScaleUp = SCREEN_SCALE_WIDTH;
		fWideScreenWidthScaleMid = 0.0f;
		fWideScreenWidthScaleDown = SCREEN_SCALE_WIDTH;
		fWideScreenHeightPerc = fWideScreenHeightRatio / SCREEN_AR_STRETCH;
		fWideScreenHeightDiff = 1.0f - fWideScreenHeightPerc;
		fWideScreenHeightMult = 2.0f - fWideScreenHeightPerc;
		fWideScreenHeightScaleUp = SCREEN_SCALE_HEIGHT * fWideScreenHeightMult;
		fWideScreenHeightScaleMid = SCREEN_SCALE_HEIGHT * fWideScreenHeightDiff;
		fWideScreenHeightScaleDown = SCREEN_SCALE_HEIGHT * fWideScreenHeightPerc;
	}

	fWideScreenWidthScaled = (float)RsGlobal->MaximumWidth * fWideScreenWidthScaleDown;
	fWideScreenWidthScaledInv = 1.0f / fWideScreenWidthScaled;
	fWideScreenHeightScaled = (float)RsGlobal->MaximumHeight * fWideScreenHeightScaleDown;
	fWideScreenHeightScaledInv = 1.0f / fWideScreenHeightScaled;

	fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(fScreenFieldOfViewVStd * 0.5f)) * *pfScreenAspectRatio))
		* (1.0f / SCREEN_FOV_HORIZONTAL);

	//logo scaling
	fvcLogoScale = (245.312497f * fWideScreenWidthScaleDown) / 1.7f;
	fCrosshairPosFactor = ((0.52999997f - 0.5f) / ((*pfScreenAspectRatio) / (16.0f / 9.0f))) + 0.5f;

	//Ini options
	fCustomWideScreenWidthScaleDown = fWideScreenWidthScaleDown * fHudWidthScale;
	fCustomWideScreenHeightScaleDown = fWideScreenHeightScaleDown * fHudHeightScale;
	fCustomRadarWidthScale = fWideScreenWidthScaleDown * fRadarWidthScale;
	fCustomRadarRingWidthScale = fCustomRadarWidthScale + 0.000019f;
	fCustomRadarRingHeightScale = fWideScreenHeightScaleDown + 0.000019f;

	//Proportional elements
	fCrosshairHeightScaleDown = fWideScreenHeightScaleDown * fHudWidthScale;
	return;
}

inline float getDynamicScreenFieldOfView(float fFactor)
{
	fEmergencyVehiclesFix = 70.0f / fFactor;

	if ((!*(char*)IsInCutscene == 0 && RestoreCutsceneFOV) || DontTouchFOV)
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
	*pfScreenFieldOfView = /*!CLASS_pclCamera->m_bWidescreen ? */getDynamicScreenFieldOfView(fFactor)/* : fFactor*/;
}