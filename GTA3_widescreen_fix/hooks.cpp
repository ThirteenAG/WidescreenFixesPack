#include "StdAfx.h"
#include "hooks.h"
#include <string>

float fScreenAspectRatioAcd = SCREEN_AR_ACADEMY;
float fInvScreenAspectRatioAcd = 1.0f / SCREEN_AR_ACADEMY;
float fScreenFieldOfViewHStd = SCREEN_FOV_HORIZONTAL;
float fScreenFieldOfViewVStd = SCREEN_FOV_VERTICAL;

float fWideScreenWidthRatio, fWideScreenHeightRatio;
float fWideScreenWidthPerc, fWideScreenHeightPerc;
float fWideScreenWidthDiff, fWideScreenHeightDiff;
float fWideScreenWidthMult, fWideScreenHeightMult;
float fWideScreenWidthScaleUp, fWideScreenHeightScaleUp;
float fWideScreenWidthScaleMid, fWideScreenHeightScaleMid;
float fWideScreenWidthScaleDown, fWideScreenHeightScaleDown;
float fWideScreenWidthScaled, fWideScreenHeightScaled;
float fWideScreenWidthScaledInv, fWideScreenHeightScaledInv;

float fDynamicScreenFieldOfViewScale;

float fvcLogoScale;
float fFiretruckAmblncFix;
float fHudWidthScale, fHudHeightScale;
float fCustomWideScreenWidthScaleDown;
float fCustomWideScreenHeightScaleDown;
float fRadarWidthScale, fCustomRadarWidthScale, fCustomRadarRingWidthScale, fCustomRadarRingHeightScale, fSubtitlesScale;
int nMenuAlignment;
int RestoreCutsceneFOV;
float fPlayerMarkerPos;
float fTextShadowsSize;
float fCarSpeedDependantFOV;
int DontTouchFOV;
float fRadarScaling;
char *szForceAspectRatio;
char *szFOVControl;
int FOVControl;
int AspectRatioWidth, AspectRatioHeight;
int HideAABug, SmartCutsceneBorders;

void __cdecl CDraw__CalculateAspectRatio()
{
	if (!AspectRatioWidth && !AspectRatioHeight)
	{
		*VAR_pfScreenAspectRatio = (float)CLASS_pclRsGlobal->m_iScreenWidth / (float)CLASS_pclRsGlobal->m_iScreenHeight;
	} else {
		*VAR_pfScreenAspectRatio = (float)AspectRatioWidth / (float)AspectRatioHeight;
	}

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

	fWideScreenWidthScaled = (float)CLASS_pclRsGlobal->m_iScreenWidth * fWideScreenWidthScaleDown;
	fWideScreenWidthScaledInv = 1.0f / fWideScreenWidthScaled;
	fWideScreenHeightScaled = (float)CLASS_pclRsGlobal->m_iScreenHeight * fWideScreenHeightScaleDown;
	fWideScreenHeightScaledInv = 1.0f / fWideScreenHeightScaled;

	fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(fScreenFieldOfViewVStd * 0.5f)) * *VAR_pfScreenAspectRatio))
		* (1.0f / SCREEN_FOV_HORIZONTAL);
	fDynamicScreenFieldOfViewScale += 0.042470217f; //small FOV adjustment, to make aiming point and crosshair matchicng more precise

	nMenuAlignment = static_cast<int>((*VAR_pfScreenAspectRatio / 1.333333f) * 320.0f);// (CLASS_pclRsGlobal->m_iScreenWidth / 2) * fWideScreenWidthScaleDown;
	MenuFix();

	//Ini options
	fCustomWideScreenWidthScaleDown = fWideScreenWidthScaleDown * fHudWidthScale;
	fCustomWideScreenHeightScaleDown = fWideScreenHeightScaleDown * fHudHeightScale;
	fCustomRadarWidthScale = fWideScreenWidthScaleDown * fRadarWidthScale;
	fCustomRadarRingWidthScale = fCustomRadarWidthScale + 0.000019f;
	fCustomRadarRingHeightScale = fWideScreenHeightScaleDown + 0.000019f;
	return;
}

auto FindPlayerVehicle = (int(__cdecl *)()) 0x4A10C0;
auto FindPlayerSpeed = (int(__cdecl *)()) 0x4A1090;
inline float getDynamicScreenFieldOfView(float fFactor)
{
	fFiretruckAmblncFix = 70.0f / fFactor;

	if ((!(char)*(DWORD*)0x6FAD68 == 0 && RestoreCutsceneFOV) || DontTouchFOV)
	{
		return fFactor * (*(float*)FOVControl);
	}

	if (fCarSpeedDependantFOV)
	{
		if (FindPlayerVehicle())
		{
			return (fFactor * (*(float*)FOVControl) * fDynamicScreenFieldOfViewScale) + (fRadarScaling / fCarSpeedDependantFOV);
		}

	}

	return fFactor * (*(float*)FOVControl) * fDynamicScreenFieldOfViewScale;
}

void setScreenFieldOfView(float fFactor) {
	*VAR_pfScreenFieldOfView = /*!CLASS_pclCamera->m_bWidescreen ? */getDynamicScreenFieldOfView(fFactor)/* : fFactor*/;
}


void __declspec(naked)RadarScaling_compat_patch()
{
	_asm
	{
		//fstp    ds : 0x974BEC
		fstp    ds : fRadarScaling
		mov ebp, fRadarScaling
		mov ds : [0x8E281C], ebp
	}
	fRadarScaling -= 120.0f;
	_asm
	{
		mov ebp, 0x4A42A6
		jmp ebp
	}
}


void __declspec(naked)FiretruckAmblncFix()
{
	__asm mov eax, fFiretruckAmblncFix
	__asm mov[ebx + 0xF0], eax
	__asm mov eax, 0x46EC13
	__asm jmp eax
}


void __declspec(naked)DrawBordersForWideScreen_patch()
{
	_asm
	{
		cmp byte ptr ds : [0095CD23h], 00
		jnz label1
		ret
	label1:
		mov     eax, ds : [0070AEE0h]
		push    ebx
		sub esp, 30h
		mov ebx, 0x46B439
		jmp ebx
	}
}


#define CDarkel__DrawMessages 0x420920
#define LoadingScreen 0x48D770
#define Render2dStuff 0x48E0E0
#define CClouds__Render 0x4F6D90
#define CHud__Draw 0x5052A0
#define cMusicManager__DisplayRadioStationName 0x57E6D0
float fCustomSniperScopeScaleWidth = 338.0f;
float fCustomSniperScopeScaleHeight = 316.0f;
void HudFix()
{
	CPatch::SetPointer(CDarkel__DrawMessages + 0x289 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x2F6 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x395 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x404 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(CDarkel__DrawMessages + 0x267 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x2D4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x373 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x3E3 + 0x2, &fWideScreenHeightScaleDown);

	

	/*menu*/
	CPatch::SetPointer(0x48AC00 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x48B7E0 + 0x19B + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(0x48AC40 + 0x2, &fWideScreenHeightScaleDown);
	/**/


	
	CPatch::SetPointer(LoadingScreen + 0x16B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(LoadingScreen + 0x213 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(LoadingScreen + 0x279 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x48DA50 + 0x1B9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x48DA50 + 0x28B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(Render2dStuff + 0x283 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(Render2dStuff + 0x2EC + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(LoadingScreen + 0x136 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(LoadingScreen + 0x1DE + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(LoadingScreen + 0x263 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x48DA50 + 0x1A3 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x48DA50 + 0x1E6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x48DA50 + 0x275 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x48DA50 + 0x2AF + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(Render2dStuff + 0x102 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(Render2dStuff + 0x176 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(Render2dStuff + 0x1BC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(Render2dStuff + 0x22F + 0x2, &fWideScreenHeightScaleDown);

	

		
	CPatch::SetPointer(0x4A5040 + 0x15 + 0x2, &fWideScreenWidthScaleDown); //CRadar__DrawRadarMask
	CPatch::SetPointer(0x4A5D10 + 0x1A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A5EF0 + 0xF + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A6C20 + 0x43 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(0x4A5040 + 0x4B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A5D10 + 0x4D + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A5EF0 + 0x42 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A6C20 + 0x19 + 0x2, &fWideScreenHeightScaleDown);
	
	


	CPatch::SetPointer(CClouds__Render + 0x5DC + 0x2, &fWideScreenWidthScaleDown);




	CPatch::SetPointer(CHud__Draw + 0x2F4 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3CF + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4F1 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x603 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x7BD + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x88B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x948 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xA60 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xB2A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xC30 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xCC9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xD65 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xF3B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xFE5 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x100C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x10D5 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1112 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1321 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1382 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x145B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x14B6 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1570 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1692 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x16F3 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x17CC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1827 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x18BE + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x192A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1A5A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1D52 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1D82 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1E41 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1EEC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x21F0 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2220 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x22DF + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x238A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x23D1 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x249B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2517 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x25F7 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x268B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x26C9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2737 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x27B7 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2805 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2884 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x295A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x29BD + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2A37 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2AB0 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2B49 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2C14 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2CC1 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2D2E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2DAD + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2F6F + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3011 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3065 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x30AE + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x31B3 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x327E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x32FC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3318 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x33DD + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3547 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3586 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3670 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x374E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3775 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3B6B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3B9C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3C50 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3CF4 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x3A7 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x3DC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x429 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x505 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x56F + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x5EC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x608 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x6CC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x7E5 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x80C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x96E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x995 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0xCC5 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0xCFA + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0xD94 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0xEA8 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0xED2 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x10F7 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x1199 + 0x2, &fWideScreenWidthScaleDown);


	CPatch::SetPointer(CHud__Draw + 0x2D0 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3AB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4BC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x5C8 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x792 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x86B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x96C + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xA11 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xAF9 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xC1A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xCA2 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xD44 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xF09 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xFCF + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x10B4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x10FC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x12FA + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x13C2 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x143A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x14F6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x155A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x166B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1733 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x17AB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1867 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x18A8 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1974 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1A35 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1D3D + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1D6D + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1E16 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1EC7 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x21DB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x220B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x22B4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2365 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x23BB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2474 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x24F6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x25E1 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2664 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x26B3 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2716 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x27A1 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x27DD + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2862 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2944 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2A10 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2A8F + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2B0B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2BD3 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2CAB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2D06 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2D8B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2F49 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3001 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3097 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3179 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3262 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x33AF + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3531 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3635 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3738 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x398B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3A37 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3B56 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3B87 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3C2B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3CCF + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0x38C + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0x3C7 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0x4DA + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0x553 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0x69F + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0x7CF + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0x958 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0xCA7 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0xD54 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0xDEB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0xE93 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0xEBD + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0x10C4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0x1174 + 0x2, &fWideScreenHeightScaleDown);





	CPatch::SetPointer(cMusicManager__DisplayRadioStationName + 0x284 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(cMusicManager__DisplayRadioStationName + 0x2B3 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetFloat(0x60D43C, 0.875f);
	CPatch::SetFloat(0x60D444, 0.518f);
	CPatch::SetFloat(0x60D450, 1.0f);

	CPatch::SetPointer(cMusicManager__DisplayRadioStationName + 0x26E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(cMusicManager__DisplayRadioStationName + 0x2EF + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(cMusicManager__DisplayRadioStationName + 0x386 + 0x2, &fWideScreenHeightScaleDown);





	CPatch::SetPointer(0x592880 + 0xE9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x592880 + 0x17E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x592880 + 0x231 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x592880 + 0x297 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x592880 + 0x2CC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x592880 + 0x349 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(0x592880 + 0xCF + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x592880 + 0x151 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x592880 + 0x208 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x592880 + 0x27B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x592880 + 0x333 + 0x2, &fWideScreenHeightScaleDown);




	CPatch::SetPointer(0x595EE0 + 0x66 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x595EE0 + 0x50 + 0x2, &fWideScreenHeightScaleDown);

}


void MenuFix()
{
	CPatch::SetInt(0x47B174, nMenuAlignment);
	CPatch::SetInt(0x47B1E0, nMenuAlignment);
	CPatch::SetInt(0x47B33C, nMenuAlignment);
	CPatch::SetInt(0x47B3A8, nMenuAlignment);
	CPatch::SetInt(0x47B411, nMenuAlignment);
}


void __declspec(naked)RsSelectDeviceHook()
{
	MenuFix();
	CDraw__CalculateAspectRatio();
	_asm
	{
		add     esp, 10h
		pop     ebp
		pop     esi
		pop     ebx
		ret
	}
}

auto CSprite2d__DrawRect = (int(__cdecl *)(CRect const &, CRGBA const &)) 0x51F970;
void _1pxBugFix()
{
	CSprite2d__DrawRect(CRect(0.0f, -5.0f, (float)CLASS_pclRsGlobal->m_iScreenWidth, 0.5f), CRGBA(0, 0, 0, 255));
	CSprite2d__DrawRect(CRect(-5.0f, -1.0f, 0.5f, (float)CLASS_pclRsGlobal->m_iScreenHeight), CRGBA(0, 0, 0, 255));
	if (HideAABug == 2)
	{
		CSprite2d__DrawRect(CRect(0.0f, (float)CLASS_pclRsGlobal->m_iScreenHeight - 1.5f, (float)CLASS_pclRsGlobal->m_iScreenWidth, (float)CLASS_pclRsGlobal->m_iScreenHeight + 5.0f), CRGBA(0, 0, 0, 255));
		CSprite2d__DrawRect(CRect((float)CLASS_pclRsGlobal->m_iScreenWidth - 1.0f, 0.0f, (float)CLASS_pclRsGlobal->m_iScreenWidth + 5.0f, (float)CLASS_pclRsGlobal->m_iScreenHeight), CRGBA(0, 0, 0, 255));
	}
}

void GetScreenRect(CRect& rect)
{
	float			fScreenRatio = *VAR_pfScreenAspectRatio;
	float			dScreenHeightWeWannaCut = ((-9.0f / 16.0f) * fScreenRatio + 1.0f);
	float			dBorderProportionsFix = ((-144643.0f / 50000.0f) * fScreenRatio * fScreenRatio) + ((807321.0f / 100000.0f) * fScreenRatio) - (551143.0f / 100000.0f);

	if (dBorderProportionsFix < 0.0)
		dBorderProportionsFix = 0.0;

	if (dScreenHeightWeWannaCut > 0.0)
	{
		// Letterbox
		rect.y1 = ((float)CLASS_pclRsGlobal->m_iScreenHeight / 2) * (dScreenHeightWeWannaCut - dBorderProportionsFix);
		rect.y2 = (float)CLASS_pclRsGlobal->m_iScreenHeight - (((float)CLASS_pclRsGlobal->m_iScreenHeight / 2) * (dScreenHeightWeWannaCut + dBorderProportionsFix));
	}
	else
	{
		// Pillarbox
		dScreenHeightWeWannaCut = -dScreenHeightWeWannaCut;

		rect.x1 = ((float)CLASS_pclRsGlobal->m_iScreenWidth / 4) * dScreenHeightWeWannaCut;
		rect.x2 = (float)CLASS_pclRsGlobal->m_iScreenWidth - ((float)CLASS_pclRsGlobal->m_iScreenWidth / 4) * dScreenHeightWeWannaCut;
	}
}

int CCamera__DrawBordersForWideScreen(int)
{
	if (!*(BYTE*)0x95CD23)
		return 0;

	CRect		ScreenRect;

	ScreenRect.x1 = -1000.0f;
	ScreenRect.y1 = -1000.0f;
	ScreenRect.x2 = -1000.0f;
	ScreenRect.y2 = -1000.0f;

	GetScreenRect(ScreenRect);

	if (!*(DWORD*)0x6FADA0 || *(float*)0x6FADA0 == 2)
		*(DWORD*)0x6FADB8 = 80;

	// Letterbox
	if (ScreenRect.y1 > 0.0 && ScreenRect.y2 > 0.0)
	{
		CSprite2d__DrawRect(CRect(-5.0f, -5.0f, (float)CLASS_pclRsGlobal->m_iScreenWidth + 5.0f, ScreenRect.y1), CRGBA(0, 0, 0, 255));
		CSprite2d__DrawRect(CRect(-5.0f, ScreenRect.y2, (float)CLASS_pclRsGlobal->m_iScreenWidth + 5.0f, (float)CLASS_pclRsGlobal->m_iScreenHeight + 5.0f), CRGBA(0, 0, 0, 255));
	}
	//Pillarbox
	else if (ScreenRect.x1 > 0.0 && ScreenRect.x2 > 0.0)
	{
		CSprite2d__DrawRect(CRect(-5.0f, -5.0f, ScreenRect.x1, (float)CLASS_pclRsGlobal->m_iScreenHeight + 5.0f), CRGBA(0, 0, 0, 255));
		CSprite2d__DrawRect(CRect(ScreenRect.x2, -5.0f, (float)CLASS_pclRsGlobal->m_iScreenWidth + 5.0f, (float)CLASS_pclRsGlobal->m_iScreenHeight + 5.0f), CRGBA(0, 0, 0, 255));
	}
	return 0;

}

void ApplyINIchanges()
{
	CIniReader iniReader("GTAIII_widescreen_fix.ini");
	fHudWidthScale = iniReader.ReadFloat("MAIN", "HudWidthScale", 0.62221788786f);
	fHudHeightScale = iniReader.ReadFloat("MAIN", "HudHeightScale", 0.66666670937f);
	fRadarWidthScale = iniReader.ReadFloat("MAIN", "RadarWidthScale", 0.80354591724f);
	fSubtitlesScale = iniReader.ReadFloat("MAIN", "SubtitlesScale", 1.0f);

	int SmallerTextShadows = iniReader.ReadInteger("MAIN", "SmallerTextShadows", 1);

	RestoreCutsceneFOV = iniReader.ReadInteger("MAIN", "RestoreCutsceneFOV", 1);

	fCarSpeedDependantFOV = iniReader.ReadFloat("MAIN", "CarSpeedDependantFOV", 0.0f);

	DontTouchFOV = iniReader.ReadInteger("MAIN", "DontTouchFOV", 0);

	szForceAspectRatio = iniReader.ReadString("MAIN", "ForceAspectRatio", "auto");
	if (strncmp(szForceAspectRatio, "auto", 4) != 0)
	{
		AspectRatioWidth = std::stoi(szForceAspectRatio);
		AspectRatioHeight = std::stoi(strchr(szForceAspectRatio, ':') + 1);
	}

	szFOVControl = iniReader.ReadString("MAIN", "FOVControl", "0x4012F9");
	sscanf_s(szFOVControl, "%X", &FOVControl);
	CPatch::SetFloat(FOVControl, 1.0f);

	HideAABug = iniReader.ReadInteger("MAIN", "HideAABug", 1);

	SmartCutsceneBorders = iniReader.ReadInteger("MAIN", "SmartCutsceneBorders", 1);

	if (iniReader.ReadInteger("MAIN", "FixVehicleLights", 1))
	{
		CPatch::SetUInt(0x004F95E6, 0x005E51B0); //lamp corona stretch
		CPatch::SetUInt(0x004F94A6, 0x005EC9BC); //car lights stretch
	}

	if (!fHudWidthScale || !fHudHeightScale) { fHudWidthScale = 0.62221788786f; fHudHeightScale = 0.66666670937f; }
	if (!fRadarWidthScale) { fRadarWidthScale = 0.80354591724f; }
	if (!fSubtitlesScale) { fSubtitlesScale = 1.0f; }

	if (SmallerTextShadows)
	{
		fTextShadowsSize = 1.0f;
		CPatch::SetPointer(0x505F7B, &fTextShadowsSize); CPatch::SetPointer(0x505F50, &fTextShadowsSize);
		CPatch::SetPointer(0x5065D3, &fTextShadowsSize); CPatch::SetPointer(0x5065A8, &fTextShadowsSize);
		CPatch::SetPointer(0x50668E, &fTextShadowsSize); CPatch::SetPointer(0x506670, &fTextShadowsSize);
		CPatch::SetPointer(0x506944, &fTextShadowsSize); CPatch::SetPointer(0x506919, &fTextShadowsSize);
		CPatch::SetPointer(0x5069FF, &fTextShadowsSize); CPatch::SetPointer(0x5069E1, &fTextShadowsSize);
		CPatch::SetPointer(0x506C2B, &fTextShadowsSize); CPatch::SetPointer(0x506C22, &fTextShadowsSize);
		CPatch::SetPointer(0x5070F3, &fTextShadowsSize); CPatch::SetPointer(0x5070C8, &fTextShadowsSize);
		CPatch::SetPointer(0x507591, &fTextShadowsSize); CPatch::SetPointer(0x507566, &fTextShadowsSize);
		CPatch::SetPointer(0x50774D, &fTextShadowsSize); CPatch::SetPointer(0x507722, &fTextShadowsSize);
		CPatch::SetPointer(0x50793D, &fTextShadowsSize); CPatch::SetPointer(0x507912, &fTextShadowsSize);
		CPatch::SetPointer(0x507A8B, &fTextShadowsSize); CPatch::SetPointer(0x507CE9, &fTextShadowsSize);
		CPatch::SetPointer(0x507CBE, &fTextShadowsSize); CPatch::SetPointer(0x507FB4, &fTextShadowsSize);
		CPatch::SetPointer(0x508C67, &fTextShadowsSize); CPatch::SetPointer(0x508C46, &fTextShadowsSize);
		CPatch::SetPointer(0x508F02, &fTextShadowsSize); CPatch::SetPointer(0x42643F, &fTextShadowsSize);
		CPatch::SetPointer(0x426418, &fTextShadowsSize); CPatch::SetPointer(0x42657D, &fTextShadowsSize);
		CPatch::SetPointer(0x426556, &fTextShadowsSize); CPatch::SetPointer(0x426658, &fTextShadowsSize);
		CPatch::SetPointer(0x426637, &fTextShadowsSize); CPatch::SetPointer(0x509A5E, &fTextShadowsSize);
		CPatch::SetPointer(0x509A3D, &fTextShadowsSize); CPatch::SetPointer(0x509A5E, &fTextShadowsSize);
		CPatch::SetPointer(0x509A3D, &fTextShadowsSize); CPatch::SetPointer(0x50A139, &fTextShadowsSize);
		CPatch::SetPointer(0x57E9EE, &fTextShadowsSize); CPatch::SetPointer(0x57E9CD, &fTextShadowsSize);

		CPatch::SetFloat(0x5ECCE4, 0.8f);
		CPatch::SetFloat(0x5ECCE8, 0.93f);
		CPatch::SetFloat(0x5ECCEC, 0.5f);
		CPatch::SetFloat(0x5ECFD0, 0.93f);
		CPatch::SetFloat(0x5ECFD4, 0.8f);
		
	}

	fPlayerMarkerPos = 94.0f * fRadarWidthScale;

	if (fSubtitlesScale)
	{
		CPatch::SetFloat(0x5FDC7C, 1.12f * fSubtitlesScale);
		CPatch::SetFloat(0x5FDC80, 0.48f * fSubtitlesScale);
	}

#pragma region CHud::CustomDraw
	CPatch::SetPointer(CHud__Draw + 0x2F4 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3CF + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4F1 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x603 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x7BD + 0x2, &fCustomWideScreenWidthScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0x88B + 0x2, &fCustomWideScreenWidthScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0x948 + 0x2, &fCustomWideScreenWidthScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0xA60 + 0x2, &fCustomWideScreenWidthScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0xB2A + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xC30 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xCC9 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xD65 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xF3B + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xFE5 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x100C + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x10D5 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1112 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1321 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1382 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x145B + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x14B6 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1570 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1692 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x16F3 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x17CC + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1827 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x18BE + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x192A + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1A5A + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1D52 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1D82 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1E41 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1EEC + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x21F0 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2220 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x22DF + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x238A + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x23D1 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x249B + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2517 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x25F7 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x268B + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x26C9 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2737 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x27B7 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2805 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2884 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x295A + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x29BD + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2A37 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2AB0 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2B49 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2C14 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2CC1 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2D2E + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2DAD + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2F6F + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3011 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3065 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x30AE + 0x2, &fCustomWideScreenWidthScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0x31B3 + 0x2, &fCustomWideScreenWidthScaleDown); //radar
	CPatch::SetPointer(CHud__Draw + 0x327E + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x32FC + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3318 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x33DD + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3547 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3586 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3670 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x374E + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3775 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3B6B + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3B9C + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3C50 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3CF4 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x3A7 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x3DC + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x429 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x505 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x56F + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x5EC + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x608 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x6CC + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x7E5 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x80C + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x96E + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x995 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0xCC5 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0xCFA + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0xD94 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0xEA8 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0xED2 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x10F7 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x509030 + 0x1199 + 0x2, &fCustomWideScreenWidthScaleDown);


	CPatch::SetPointer(CHud__Draw + 0x2D0 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3AB + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4BC + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x5C8 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x792 + 0x2, &fCustomWideScreenHeightScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0x86B + 0x2, &fCustomWideScreenHeightScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0x96C + 0x2, &fCustomWideScreenHeightScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0xA11 + 0x2, &fCustomWideScreenHeightScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0xAF9 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xC1A + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xCA2 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xD44 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xF09 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xFCF + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x10B4 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x10FC + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x12FA + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x13C2 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x143A + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x14F6 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x155A + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x166B + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1733 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x17AB + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1867 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x18A8 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1974 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1A35 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1D3D + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1D6D + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1E16 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1EC7 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x21DB + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x220B + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x22B4 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2365 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x23BB + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2474 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x24F6 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x25E1 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2664 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x26B3 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2716 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x27A1 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x27DD + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2862 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2944 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2A10 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2A8F + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2B0B + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2BD3 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2CAB + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2D06 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2D8B + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2F49 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3001 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3097 + 0x2, &fCustomWideScreenHeightScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0x3179 + 0x2, &fCustomWideScreenHeightScaleDown); //radar
	CPatch::SetPointer(CHud__Draw + 0x3262 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x33AF + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3531 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3635 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3738 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x398B + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3A37 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3B56 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3B87 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3C2B + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3CCF + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0x38C + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0x3C7 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0x4DA + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0x553 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0x69F + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0x7CF + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0x958 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0xCA7 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0xD54 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0xDEB + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0xE93 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0xEBD + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0x10C4 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x509030 + 0x1174 + 0x2, &fCustomWideScreenHeightScaleDown);

#pragma endregion CHud::CustomDraw

	if (fRadarWidthScale)
	{

		CPatch::SetPointer(CHud__Draw + 0x31B3 + 0x2, &fCustomRadarRingWidthScale);
		CPatch::SetPointer(CHud__Draw + 0x3179 + 0x2, &fCustomRadarRingHeightScale);
		CPatch::SetPointer(0x4A5040 + 0x15 + 0x2, &fCustomRadarWidthScale); //CRadar__DrawRadarMask
		//CPatch::SetPointer(0x4A5D10 + 0x1A + 0x2, &fCustomRadarWidthScale);
		//CPatch::SetPointer(0x4A5EF0 + 0xF + 0x2, &fCustomRadarWidthScale);
		//CPatch::SetPointer(0x4A6C20 + 0x43 + 0x2, &fCustomRadarWidthScale);

	}

	if (HideAABug)
	{
		CPatch::RedirectJump(0x48E0DB, _1pxBugFix);
	}

	if (SmartCutsceneBorders)
	{
		CPatch::RedirectCall(0x48E13E, CCamera__DrawBordersForWideScreen);
		CPatch::RedirectCall(0x4FE5D0, CCamera__DrawBordersForWideScreen);
	}
}









///////////////////////////////////////////steam//////////////////////////////////

void __cdecl CDraw__CalculateAspectRatio_steam()
{
	if (!AspectRatioWidth && !AspectRatioHeight)
	{
		*VAR_pfScreenAspectRatio_steam = (float)CLASS_pclRsGlobal_steam->m_iScreenWidth / (float)CLASS_pclRsGlobal_steam->m_iScreenHeight;
	}
	else {
		*VAR_pfScreenAspectRatio_steam = (float)AspectRatioWidth / (float)AspectRatioHeight;
	}

	if (*VAR_pfScreenAspectRatio_steam >= 1.0f) {
		fWideScreenWidthRatio = 1.0f / *VAR_pfScreenAspectRatio_steam;
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
		fWideScreenHeightRatio = *VAR_pfScreenAspectRatio_steam;
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

	fWideScreenWidthScaled = (float)CLASS_pclRsGlobal_steam->m_iScreenWidth * fWideScreenWidthScaleDown;
	fWideScreenWidthScaledInv = 1.0f / fWideScreenWidthScaled;
	fWideScreenHeightScaled = (float)CLASS_pclRsGlobal_steam->m_iScreenHeight * fWideScreenHeightScaleDown;
	fWideScreenHeightScaledInv = 1.0f / fWideScreenHeightScaled;

	fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(fScreenFieldOfViewVStd * 0.5f)) * *VAR_pfScreenAspectRatio_steam))
		* (1.0f / SCREEN_FOV_HORIZONTAL);
	fDynamicScreenFieldOfViewScale += 0.042470217f; //small FOV adjustment, to make aiming point and crosshair matchicng more precise

	nMenuAlignment = static_cast<int>((*VAR_pfScreenAspectRatio_steam / 1.333333f) * 320);// (CLASS_pclRsGlobal_steam->m_iScreenWidth / 2) * fWideScreenWidthScaleDown;

	//Ini options
	fCustomWideScreenWidthScaleDown = fWideScreenWidthScaleDown * fHudWidthScale;
	fCustomWideScreenHeightScaleDown = fWideScreenHeightScaleDown * fHudHeightScale;
	fCustomRadarWidthScale = fWideScreenWidthScaleDown * fRadarWidthScale;
	fCustomRadarRingWidthScale = fCustomRadarWidthScale + 0.000019f;
	fCustomRadarRingHeightScale = fWideScreenHeightScaleDown + 0.000019f;
	return;
}

auto FindPlayerVehicle_steam = (int(__cdecl *)()) 0x4A1140;
auto FindPlayerSpeed_steam = (int(__cdecl *)()) 0x4A1170;
inline float getDynamicScreenFieldOfView_steam(float fFactor)
{
	fFiretruckAmblncFix = 70.0f / fFactor;

	if ((!(char)*(DWORD*)0x70AEA8 == 0 && RestoreCutsceneFOV) || DontTouchFOV)
	{
		return fFactor * (*(float*)FOVControl);
	}

	if (fCarSpeedDependantFOV)
	{
		if (FindPlayerVehicle_steam())
		{
			fRadarScaling = *(float *)0x8F290C - 120.0f;
			return (fFactor * (*(float*)FOVControl) * fDynamicScreenFieldOfViewScale) + (fRadarScaling / fCarSpeedDependantFOV);
		}

	}

	return fFactor * (*(float*)FOVControl) * fDynamicScreenFieldOfViewScale;
}

void setScreenFieldOfView_steam(float fFactor) {
	*VAR_pfScreenFieldOfView_steam = /*!CLASS_pclCamera->m_bWidescreen ? */getDynamicScreenFieldOfView_steam(fFactor)/* : fFactor*/;
}


void __declspec(naked)FiretruckAmblncFix_steam()
{
	__asm mov eax, fFiretruckAmblncFix
	__asm mov[ebx + 0xF0], eax
	__asm mov eax, 0x46EBF3
	__asm jmp eax
}


void __declspec(naked)DrawBordersForWideScreen_patch_steam()
{
	_asm
	{
			cmp byte ptr ds : [0096D01Bh], 00
			jnz label1
			ret
		label1 :
			mov     eax, ds : [006FADA0h]
			push    ebx
			sub esp, 30h
			mov ebx, 0x46B409
			jmp ebx
	}
}



void HudFix_steam()
{
	CPatch::SetPointer(0x420920 + 0x289 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x420920 + 0x2F6 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x420920 + 0x395 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x420920 + 0x404 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(0x420920 + 0x267 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x420920 + 0x2D4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x420920 + 0x373 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x420920 + 0x3E3 + 0x2, &fWideScreenHeightScaleDown);




	CPatch::SetPointer(0x48AD10 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x48B860 + 0x19B + 0x2, &fWideScreenWidthScaleDown);



	CPatch::SetPointer(0x48AD50 + 0x2, &fWideScreenHeightScaleDown);




	CPatch::SetPointer(0x48D800 + 0x16B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x48D800 + 0x213 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x48D800 + 0x279 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x48DAB0 + 0x1B9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x48DAB0 + 0x28B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x48E130 + 0x283 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x48E130 + 0x2EC + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(0x48D800 + 0x136 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x48D800 + 0x1DE + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x48D800 + 0x263 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x48DAB0 + 0x1A3 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x48DAB0 + 0x1E6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x48DAB0 + 0x275 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x48DAB0 + 0x2AF + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x48E130 + 0x102 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x48E130 + 0x176 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x48E130 + 0x1BC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x48E130 + 0x22F + 0x2, &fWideScreenHeightScaleDown);



	CPatch::SetPointer(0x4A50C0 + 0x15 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A5D90 + 0x1A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A5F70 + 0xF + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A6CA0 + 0x43 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(0x4A50C0 + 0x4B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A5D90 + 0x4D + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A5F70 + 0x42 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A6CA0 + 0x19 + 0x2, &fWideScreenHeightScaleDown);



	CPatch::SetPointer(0x4F6E00 + 0x5DC + 0x2, &fWideScreenWidthScaleDown);




	CPatch::SetPointer(0x505310 + 0x2F4 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3CF + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x4F1 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x603 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x7BD + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x88B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x948 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0xA60 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0xB2A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0xC30 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0xCC9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0xD65 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0xF3B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0xFE5 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x100C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x10D5 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1112 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1321 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1382 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x145B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x14B6 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1570 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1692 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x16F3 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x17CC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1827 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x18BE + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x192A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1A5A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1D52 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1D82 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1E41 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1EEC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x21F0 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2220 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x22DF + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x238A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x23D1 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x249B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2517 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x25F7 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x268B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x26C9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2737 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x27B7 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2805 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2884 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x295A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x29BD + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2A37 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2AB0 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2B49 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2C14 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2CC1 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2D2E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2DAD + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2F6F + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3011 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3065 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x30AE + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x31B3 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x327E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x32FC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3318 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x33DD + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3547 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3586 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3670 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x374E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3775 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3B6B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3B9C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3C50 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3CF4 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x3A7 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x3DC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x429 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x505 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x56F + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x5EC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x608 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x6CC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x7E5 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x80C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x96E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x995 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0xCC5 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0xCFA + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0xD94 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0xEA8 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0xED2 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x10F7 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x1199 + 0x2, &fWideScreenWidthScaleDown);




	CPatch::SetPointer(0x505310 + 0x2D0 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3AB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x4BC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x5C8 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x792 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x86B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x96C + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0xA11 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0xAF9 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0xC1A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0xCA2 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0xD44 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0xF09 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0xFCF + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x10B4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x10FC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x12FA + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x13C2 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x143A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x14F6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x155A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x166B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x1733 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x17AB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x1867 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x18A8 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x1974 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x1A35 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x1D3D + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x1D6D + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x1E16 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x1EC7 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x21DB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x220B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x22B4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2365 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x23BB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2474 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x24F6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x25E1 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2664 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x26B3 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2716 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x27A1 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x27DD + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2862 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2944 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2A10 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2A8F + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2B0B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2BD3 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2CAB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2D06 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2D8B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2F49 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3001 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3097 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3179 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3262 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x33AF + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3531 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3635 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3738 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x398B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3A37 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3B56 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3B87 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3C2B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3CCF + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x38C + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x3C7 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x4DA + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x553 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x69F + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x7CF + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x958 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0xCA7 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0xD54 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0xDEB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0xE93 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0xEBD + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x10C4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x1174 + 0x2, &fWideScreenHeightScaleDown);





	CPatch::SetPointer(0x57E920 + 0x284 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x57E920 + 0x2B3 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetFloat(0x619CFC, 0.875f);
	CPatch::SetFloat(0x619D04, 0.518f);
	CPatch::SetFloat(0x619D10, 1.0f);

	CPatch::SetPointer(0x57E920 + 0x26E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x57E920 + 0x2EF + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x57E920 + 0x386 + 0x2, &fWideScreenHeightScaleDown);



	CPatch::SetPointer(0x592A20 + 0xE9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x592A20 + 0x17E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x592A20 + 0x231 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x592A20 + 0x297 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x592A20 + 0x2CC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x592A20 + 0x349 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(0x592A20 + 0xCF + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x592A20 + 0x151 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x592A20 + 0x208 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x592A20 + 0x27B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x592A20 + 0x333 + 0x2, &fWideScreenHeightScaleDown);



	CPatch::SetPointer(0x596080 + 0x66 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x596080 + 0x50 + 0x2, &fWideScreenHeightScaleDown);

}

void MenuFix_steam()
{
	CPatch::SetInt(0x47B244, nMenuAlignment);
	CPatch::SetInt(0x47B2B0, nMenuAlignment);
	CPatch::SetInt(0x47B40C, nMenuAlignment);
	CPatch::SetInt(0x47B478, nMenuAlignment);
	CPatch::SetInt(0x47B4E1, nMenuAlignment);
}

void __declspec(naked)RsSelectDeviceHook_steam()
{
	MenuFix_steam();
	CDraw__CalculateAspectRatio_steam();
	_asm
	{
			add     esp, 10h
			pop     ebp
			pop     esi
			pop     ebx
			ret
	}

}

auto CSprite2d__DrawRect_steam = (int(__cdecl *)(CRect const &, CRGBA const &)) 0x51FB30;
void _1pxBugFix_steam()
{
	CSprite2d__DrawRect_steam(CRect(0.0f, -5.0f, (float)CLASS_pclRsGlobal_steam->m_iScreenWidth, 0.5f), CRGBA(0, 0, 0, 255));
	CSprite2d__DrawRect_steam(CRect(-5.0f, -1.0f, 0.5f, (float)CLASS_pclRsGlobal_steam->m_iScreenHeight), CRGBA(0, 0, 0, 255));
	if (HideAABug == 2)
	{
		CSprite2d__DrawRect_steam(CRect(0.0f, (float)CLASS_pclRsGlobal_steam->m_iScreenHeight - 1.5f, (float)CLASS_pclRsGlobal_steam->m_iScreenWidth, (float)CLASS_pclRsGlobal_steam->m_iScreenHeight + 5.0f), CRGBA(0, 0, 0, 255));
		CSprite2d__DrawRect_steam(CRect((float)CLASS_pclRsGlobal_steam->m_iScreenWidth - 1.0f, 0.0f, (float)CLASS_pclRsGlobal_steam->m_iScreenWidth + 5.0f, (float)CLASS_pclRsGlobal_steam->m_iScreenHeight), CRGBA(0, 0, 0, 255));
	}
}

void GetScreenRect_steam(CRect& rect)
{
	float			fScreenRatio = *VAR_pfScreenAspectRatio_steam;
	float			dScreenHeightWeWannaCut = ((-9.0f / 16.0f) * fScreenRatio + 1.0f);
	float			dBorderProportionsFix = ((-144643.0f / 50000.0f) * fScreenRatio * fScreenRatio) + ((807321.0f / 100000.0f) * fScreenRatio) - (551143.0f / 100000.0f);

	if (dBorderProportionsFix < 0.0)
		dBorderProportionsFix = 0.0;

	if (dScreenHeightWeWannaCut > 0.0)
	{
		// Letterbox
		rect.y1 = ((float)CLASS_pclRsGlobal_steam->m_iScreenHeight / 2) * (dScreenHeightWeWannaCut - dBorderProportionsFix);
		rect.y2 = (float)CLASS_pclRsGlobal_steam->m_iScreenHeight - (((float)CLASS_pclRsGlobal_steam->m_iScreenHeight / 2) * (dScreenHeightWeWannaCut + dBorderProportionsFix));
	}
	else
	{
		// Pillarbox
		dScreenHeightWeWannaCut = -dScreenHeightWeWannaCut;

		rect.x1 = ((float)CLASS_pclRsGlobal_steam->m_iScreenWidth / 4) * dScreenHeightWeWannaCut;
		rect.x2 = (float)CLASS_pclRsGlobal_steam->m_iScreenWidth - ((float)CLASS_pclRsGlobal_steam->m_iScreenWidth / 4) * dScreenHeightWeWannaCut;
	}
}

int CCamera__DrawBordersForWideScreen_steam(int)
{
	if (!*(BYTE*)0x96D01B)
		return 0;

	CRect		ScreenRect;

	ScreenRect.x1 = -1000.0f;
	ScreenRect.y1 = -1000.0f;
	ScreenRect.x2 = -1000.0f;
	ScreenRect.y2 = -1000.0f;

	GetScreenRect_steam(ScreenRect);

	if (!*(DWORD*)0x70AEE0 || *(float*)0x70AEE0 == 2)
		*(DWORD*)0x70AEF8 = 80;

	// Letterbox
	if (ScreenRect.y1 > 0.0 && ScreenRect.y2 > 0.0)
	{
		CSprite2d__DrawRect_steam(CRect(-5.0f, -5.0f, (float)CLASS_pclRsGlobal_steam->m_iScreenWidth + 5.0f, ScreenRect.y1), CRGBA(0, 0, 0, 255));
		CSprite2d__DrawRect_steam(CRect(-5.0f, ScreenRect.y2, (float)CLASS_pclRsGlobal_steam->m_iScreenWidth + 5.0f, (float)CLASS_pclRsGlobal_steam->m_iScreenHeight + 5.0f), CRGBA(0, 0, 0, 255));
	}
	//Pillarbox
	else if (ScreenRect.x1 > 0.0 && ScreenRect.x2 > 0.0)
	{
		CSprite2d__DrawRect_steam(CRect(-5.0f, -5.0f, ScreenRect.x1, (float)CLASS_pclRsGlobal_steam->m_iScreenHeight + 5.0f), CRGBA(0, 0, 0, 255));
		CSprite2d__DrawRect_steam(CRect(ScreenRect.x2, -5.0f, (float)CLASS_pclRsGlobal_steam->m_iScreenWidth + 5.0f, (float)CLASS_pclRsGlobal_steam->m_iScreenHeight + 5.0f), CRGBA(0, 0, 0, 255));
	}
	return 0;

}

void ApplyINIchanges_steam()
{
	CIniReader iniReader("GTAIII_widescreen_fix.ini");
	fHudWidthScale = iniReader.ReadFloat("MAIN", "HudWidthScale", 0.62221788786f);
	fHudHeightScale = iniReader.ReadFloat("MAIN", "HudHeightScale", 0.66666670937f);
	fRadarWidthScale = iniReader.ReadFloat("MAIN", "RadarWidthScale", 0.80354591724f);
	fSubtitlesScale = iniReader.ReadFloat("MAIN", "SubtitlesScale", 1.0f);

	int SmallerTextShadows = iniReader.ReadInteger("MAIN", "SmallerTextShadows", 1);

	RestoreCutsceneFOV = iniReader.ReadInteger("MAIN", "RestoreCutsceneFOV", 1);

	fCarSpeedDependantFOV = iniReader.ReadFloat("MAIN", "CarSpeedDependantFOV", 0.0f);

	DontTouchFOV = iniReader.ReadInteger("MAIN", "DontTouchFOV", 0);

	szForceAspectRatio = iniReader.ReadString("MAIN", "ForceAspectRatio", "auto");
	if (strncmp(szForceAspectRatio, "auto", 4) != 0)
	{
		AspectRatioWidth = std::stoi(szForceAspectRatio);
		AspectRatioHeight = std::stoi(strchr(szForceAspectRatio, ':') + 1);
	}

	szFOVControl = iniReader.ReadString("MAIN", "FOVControl", "0x4012F9");
	sscanf_s(szFOVControl, "%X", &FOVControl);
	CPatch::SetFloat(FOVControl, 1.0f);

	HideAABug = iniReader.ReadInteger("MAIN", "HideAABug", 1);

	SmartCutsceneBorders = iniReader.ReadInteger("MAIN", "SmartCutsceneBorders", 1);

	if (iniReader.ReadInteger("MAIN", "FixVehicleLights", 1))
	{
		CPatch::SetUInt(0x004F9656, 0x005F7030); //lamp corona stretch
		CPatch::SetUInt(0x004F9516, 0x005F99BC); //car lights stretch
	}

	if (!fHudWidthScale || !fHudHeightScale) { fHudWidthScale = 0.62221788786f; fHudHeightScale = 0.66666670937f; }
	if (!fRadarWidthScale) { fRadarWidthScale = 0.80354591724f; }

	if (SmallerTextShadows)
	{
		fTextShadowsSize = 1.0f;
		CPatch::SetPointer(0x505FEB, &fTextShadowsSize); CPatch::SetPointer(0x505FC0, &fTextShadowsSize);
		CPatch::SetPointer(0x506643, &fTextShadowsSize); CPatch::SetPointer(0x506618, &fTextShadowsSize);
		CPatch::SetPointer(0x5066FE, &fTextShadowsSize); CPatch::SetPointer(0x5066E0, &fTextShadowsSize);
		CPatch::SetPointer(0x5069B4, &fTextShadowsSize); CPatch::SetPointer(0x506989, &fTextShadowsSize);
		CPatch::SetPointer(0x506A6F, &fTextShadowsSize); CPatch::SetPointer(0x506A51, &fTextShadowsSize);
		CPatch::SetPointer(0x506C9B, &fTextShadowsSize); CPatch::SetPointer(0x506C92, &fTextShadowsSize);
		CPatch::SetPointer(0x507163, &fTextShadowsSize); CPatch::SetPointer(0x507138, &fTextShadowsSize);
		CPatch::SetPointer(0x507601, &fTextShadowsSize); CPatch::SetPointer(0x5075D6, &fTextShadowsSize);
		CPatch::SetPointer(0x5077BD, &fTextShadowsSize); CPatch::SetPointer(0x507792, &fTextShadowsSize);
		CPatch::SetPointer(0x5079AD, &fTextShadowsSize); CPatch::SetPointer(0x507982, &fTextShadowsSize);
		CPatch::SetPointer(0x507AFB, &fTextShadowsSize); CPatch::SetPointer(0x507D59, &fTextShadowsSize);
		CPatch::SetPointer(0x507D2E, &fTextShadowsSize); CPatch::SetPointer(0x508024, &fTextShadowsSize);
		CPatch::SetPointer(0x508CD7, &fTextShadowsSize); CPatch::SetPointer(0x508CB6, &fTextShadowsSize);
		CPatch::SetPointer(0x508F72, &fTextShadowsSize); CPatch::SetPointer(0x42643F, &fTextShadowsSize);
		CPatch::SetPointer(0x426418, &fTextShadowsSize); CPatch::SetPointer(0x42657D, &fTextShadowsSize);
		CPatch::SetPointer(0x426556, &fTextShadowsSize); CPatch::SetPointer(0x426658, &fTextShadowsSize);
		CPatch::SetPointer(0x426637, &fTextShadowsSize); CPatch::SetPointer(0x509ACE, &fTextShadowsSize);
		CPatch::SetPointer(0x509AAD, &fTextShadowsSize); CPatch::SetPointer(0x509ACE, &fTextShadowsSize);
		CPatch::SetPointer(0x509AAD, &fTextShadowsSize); CPatch::SetPointer(0x50A1A9, &fTextShadowsSize);
		CPatch::SetPointer(0x57EC3E, &fTextShadowsSize); CPatch::SetPointer(0x57EC1D, &fTextShadowsSize);

		CPatch::SetFloat(0x5F9CE4, 0.8f);
		CPatch::SetFloat(0x5F9CE8, 0.93f);
		CPatch::SetFloat(0x5F9CEC, 0.5f);
		CPatch::SetFloat(0x5F9FD0, 0.93f);
		CPatch::SetFloat(0x5F9FD4, 0.8f);
	}

	fPlayerMarkerPos = 94.0f * fRadarWidthScale;

	if (fSubtitlesScale)
	{
		CPatch::SetFloat(0x60AA5C, 1.12f * fSubtitlesScale);
		CPatch::SetFloat(0x60AA60, 0.48f * fSubtitlesScale);
	}
#pragma region CHud::CustomDraw_steam
	CPatch::SetPointer(0x505310 + 0x2F4 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3CF + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x4F1 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x603 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x7BD + 0x2, &fCustomWideScreenWidthScaleDown);
	//CPatch::SetPointer(0x505310 + 0x88B + 0x2, &fCustomWideScreenWidthScaleDown);
	//CPatch::SetPointer(0x505310 + 0x948 + 0x2, &fCustomWideScreenWidthScaleDown);
	//CPatch::SetPointer(0x505310 + 0xA60 + 0x2, &fCustomWideScreenWidthScaleDown);
	//CPatch::SetPointer(0x505310 + 0xB2A + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0xC30 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0xCC9 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0xD65 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0xF3B + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0xFE5 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x100C + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x10D5 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1112 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1321 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1382 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x145B + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x14B6 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1570 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1692 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x16F3 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x17CC + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1827 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x18BE + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x192A + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1A5A + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1D52 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1D82 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1E41 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x1EEC + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x21F0 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2220 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x22DF + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x238A + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x23D1 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x249B + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2517 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x25F7 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x268B + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x26C9 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2737 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x27B7 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2805 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2884 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x295A + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x29BD + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2A37 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2AB0 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2B49 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2C14 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2CC1 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2D2E + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2DAD + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x2F6F + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3011 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3065 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x30AE + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x31B3 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x327E + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x32FC + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3318 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x33DD + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3547 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3586 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3670 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x374E + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3775 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3B6B + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3B9C + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3C50 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x505310 + 0x3CF4 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x3A7 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x3DC + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x429 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x505 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x56F + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x5EC + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x608 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x6CC + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x7E5 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x80C + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x96E + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x995 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0xCC5 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0xCFA + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0xD94 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0xEA8 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0xED2 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x10F7 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x1199 + 0x2, &fCustomWideScreenWidthScaleDown);




	CPatch::SetPointer(0x505310 + 0x2D0 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3AB + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x4BC + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x5C8 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x792 + 0x2, &fCustomWideScreenHeightScaleDown);
	//CPatch::SetPointer(0x505310 + 0x86B + 0x2, &fCustomWideScreenHeightScaleDown);
	//CPatch::SetPointer(0x505310 + 0x96C + 0x2, &fCustomWideScreenHeightScaleDown);
	//CPatch::SetPointer(0x505310 + 0xA11 + 0x2, &fCustomWideScreenHeightScaleDown);
	//CPatch::SetPointer(0x505310 + 0xAF9 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0xC1A + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0xCA2 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0xD44 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0xF09 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0xFCF + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x10B4 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x10FC + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x12FA + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x13C2 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x143A + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x14F6 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x155A + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x166B + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x1733 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x17AB + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x1867 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x18A8 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x1974 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x1A35 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x1D3D + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x1D6D + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x1E16 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x1EC7 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x21DB + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x220B + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x22B4 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2365 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x23BB + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2474 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x24F6 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x25E1 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2664 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x26B3 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2716 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x27A1 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x27DD + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2862 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2944 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2A10 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2A8F + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2B0B + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2BD3 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2CAB + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2D06 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2D8B + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x2F49 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3001 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3097 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3179 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3262 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x33AF + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3531 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3635 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3738 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x398B + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3A37 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3B56 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3B87 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3C2B + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x505310 + 0x3CCF + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x38C + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x3C7 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x4DA + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x553 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x69F + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x7CF + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x958 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0xCA7 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0xD54 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0xDEB + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0xE93 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0xEBD + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x10C4 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(0x5090A0 + 0x1174 + 0x2, &fCustomWideScreenHeightScaleDown);
#pragma endregion CHud::CustomDraw_steam
	if (fRadarWidthScale)
	{
		CPatch::SetPointer(0x505310 + 0x31B3 + 0x2, &fCustomRadarRingWidthScale);
		CPatch::SetPointer(0x505310 + 0x3179 + 0x2, &fCustomRadarRingHeightScale);
		CPatch::SetPointer(0x4A50C0 + 0x15 + 0x2, &fCustomRadarWidthScale); //CRadar__DrawRadarMask
	}

	if (HideAABug)
	{
		CPatch::RedirectJump(0x48E12B, _1pxBugFix_steam);
	}

	if (SmartCutsceneBorders)
	{
		CPatch::RedirectCall(0x48E18E, CCamera__DrawBordersForWideScreen_steam);
		CPatch::RedirectCall(0x4FE640, CCamera__DrawBordersForWideScreen_steam);
	}
}