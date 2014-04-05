#include "stdafx.h"
#include "generic.h"
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"


float *const VAR_pfScreenAspectRatio = (float*)0x94DD38;
void updateScreenAspectRatio() {
	*VAR_pfScreenAspectRatio = 1920.0f / 540.0f /*(float)CLASS_pclRsGlobal->m_iScreenWidth / (float)CLASS_pclRsGlobal->m_iScreenHeight*/;

	if (*VAR_pfScreenAspectRatio >= 1.0f) {
		fWideScreenWidthRatio = 1.0f / *VAR_pfScreenAspectRatio;
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
	else {
		fWideScreenWidthRatio = 1.0f;
		fWideScreenHeightRatio = *VAR_pfScreenAspectRatio;
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

	fWideScreenWidthScaled = 1920.0f /*(float)CLASS_pclRsGlobal->m_iScreenWidth*/ * fWideScreenWidthScaleDown;
	fWideScreenWidthScaledInv = 1.0f / fWideScreenWidthScaled;
	fWideScreenHeightScaled = 540.0f /*(float)CLASS_pclRsGlobal->m_iScreenHeight*/ * fWideScreenHeightScaleDown;
	fWideScreenHeightScaledInv = 1.0f / fWideScreenHeightScaled;

	fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(fScreenFieldOfViewVStd * 0.5f)) * *VAR_pfScreenAspectRatio))
		* (1.0f / SCREEN_FOV_HORIZONTAL);
}



float* g_FOV = (float *)0x696658;
#define FOV (float *)g_FOV
void __cdecl change_fov_fixed(float a1)
{
	float a2;
	a2 = a1;
	if ((char)*(DWORD*)0x7E46F5 == 0) {
		a2 = a1 * 1.777777777777778;
	}
	*FOV = 70.0f * fDynamicScreenFieldOfViewScale;
}

void __declspec(naked)CameraSize_patch()
{
	_asm 
	{


	}
}



BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		//CPatch::RedirectJump(0x, CameraSize_patch);

		CPatch::RedirectCall(0x4A5C63, updateScreenAspectRatio);
		CPatch::RedirectCall(0x4A5E95, updateScreenAspectRatio);
		CPatch::RedirectCall(0x4A5F95, updateScreenAspectRatio);
		CPatch::RedirectCall(0x4A6098, updateScreenAspectRatio);
		CPatch::RedirectCall(0x4A72C8, updateScreenAspectRatio);


		CPatch::RedirectCall(0x46DCC8, change_fov_fixed);
		CPatch::RedirectCall(0x4A48AB, change_fov_fixed);
		CPatch::RedirectCall(0x4A4D3A, change_fov_fixed);
		CPatch::RedirectCall(0x620BC6, change_fov_fixed);
		CPatch::RedirectCall(0x627CFC, change_fov_fixed);
		CPatch::RedirectCall(0x627D49, change_fov_fixed);
	}
	return TRUE;
}

