#include "StdAfx.h"
#include "hooks.h"

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
float fRadarWidthScale, fCustomRadarWidthScale, fSubtitlesScale;
float fMVLScopeFix;
int RestoreCutsceneFOV;
float fPlayerMarkerPos;

long double __cdecl CDraw__CalculateAspectRatio()
{
	*VAR_pfScreenAspectRatio = (float)CLASS_pclRsGlobal->m_iScreenWidth / (float)CLASS_pclRsGlobal->m_iScreenHeight;

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

	//logo scaling
	fvcLogoScale = (245.312497f * fWideScreenWidthScaleDown) / 1.7f;

	//Ini options
	fCustomWideScreenWidthScaleDown = fWideScreenWidthScaleDown * fHudWidthScale;
	fCustomWideScreenHeightScaleDown = fWideScreenHeightScaleDown * fHudHeightScale;
	fCustomRadarWidthScale = fWideScreenWidthScaleDown * fRadarWidthScale;

	return fDynamicScreenFieldOfViewScale;
}

inline float getDynamicScreenFieldOfView(float fFactor) 
{
	fFiretruckAmblncFix = 70.0f / fFactor;

		if (!(char)*(DWORD*)0x7E36FD == 0 && RestoreCutsceneFOV)
		{
			return fFactor;
		}

	return fFactor * fDynamicScreenFieldOfViewScale;
}

void setScreenFieldOfView(float fFactor) {
	*VAR_pfScreenFieldOfView = /*!CLASS_pclCamera->m_bWidescreen ? */getDynamicScreenFieldOfView(fFactor)/* : fFactor*/;
}

void __declspec(naked)FiretruckAmblncFix()
{
	__asm mov eax, fFiretruckAmblncFix
	__asm mov[ebx + 0xF4], eax
	__asm mov eax, 0x46DD9D
	__asm jmp eax
}

void __declspec(naked)CameraSize_patch()
{
	_asm
	{


	}
}

void __declspec(naked)DrawBordersForWideScreen_patch()
{
	_asm
	{
		cmp byte ptr ds:[86865Ah], 00
		jz label1
		ret
	label1:
		sub esp, 38h
		cmp byte ptr ds:[7E36FDh], 00
		mov edx, 0x46FB06
		jmp edx
	}
}


int __declspec(naked)SetDropShadowPosition(short)
{
	_asm
	{
		mov     eax, [esp + 4]
		cmp eax, 2
		jne label
		dec eax
	label:
		mov		ds: [97E868h], ax
		retn
	}
}



#define	 	CDarkel__DrawMessages	 	0x429FB0
#define	 	CGarages__PrintMessages	 	0x42F280
#define	 	CMenuManager__PrintMap	 	0x49A4D8
#define	 	CMenuManager__DrawContollerScreenExtraText	 	0x49141B
#define	 	CRadar__DrawRadarMask	 	0x4C18F0
#define	 	CRadar__DrawRadarSection	 	0x4C1C30
#define	 	CRadar__DrawYouAreHereSprite	 	0x4C2850
#define	 	CRadar__DrawRadarSprite	 	0x4C2BD0
#define	 	CRadar__DrawRotatingRadarSprite	 	0x4C2D40
#define	 	CRadar__ShowRadarTraceWithHeight	 	0x4C31C0
#define	 	CRadar__DrawBlips	 	0x4C40D0
#define	 	CRadar__DrawLegend	 	0x4C48D0
#define	 	CRadar__DrawEntityBlip	 	0x4C5030
#define	 	CRadar__DrawCoordBlip	 	0x4C5540
#define	 	CClouds__Render	 	0x53F270
#define	 	CHud__DrawAfterFade	 	0x5565D0
#define	 	CHud__Draw	 	0x557210
#define	 	CMBlur__AddRenderFx	 	0x55D050
#define	 	CParticle__Render	 	0x5607B0
#define	 	CSpecialFX__Render2DFXs	 	0x573E10
#define	 	cMusicManager__DisplayRadioStationName	 	0x5F9B40
#define	 	CSceneEdit__Draw	 	0x6059D0
#define	 	CRunningScript__ProcessCommands1100To1199	 	0x606350
#define	 	CReplay__Display	 	0x620820
#define	 	Render2DStuff 0x4A6060
#define	 	LoadingScreen 0x4A68A0
#define	 	debug_internal_something 0x4918A7
#define	 	CMenuManager_PrintStringMenu 0x4984ED

void HudFix()
{

	CPatch::SetPointer(CDarkel__DrawMessages + 0xA7 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x168 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x269 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x369 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x3D6 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x475 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x4DE + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x569 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(CDarkel__DrawMessages + 0x91 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x152 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x254 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x347 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x3B4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x453 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x4BD + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x553 + 0x2, &fWideScreenHeightScaleDown);




	CPatch::SetPointer(CGarages__PrintMessages + 0x45 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(CGarages__PrintMessages + 0x2F + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CGarages__PrintMessages + 0x126 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CGarages__PrintMessages + 0x1B4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CGarages__PrintMessages + 0x208 + 0x2, &fWideScreenHeightScaleDown);




	CPatch::SetPointer(0x48F339 + 0x94 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(debug_internal_something + 0xFD + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x492C92 + 0x653 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager_PrintStringMenu + 0x124 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x4D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x12B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x178 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x1D2 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x29F + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x2E8 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x340 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x412 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x45A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x4B3 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x581 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x5CA + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x622 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x6F3 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x73D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x79B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x86B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x8B5 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x91E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x9F4 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xA3C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xAA8 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xB78 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xBC2 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xC2C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xCFA + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xD44 + 0x2, &fWideScreenWidthScaleDown);

	/*CPatch::SetPointer(0x49DDFF + 0x2C7 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x140C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x146A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x14C4 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x1524 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x18CB + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x1929 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x1983 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x19E3 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x1AB4 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x1B12 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x1B6C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x1BCC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x1D0C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x24A8 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x29D1 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x2F0D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x3449 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x39A9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x3ECF + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(0x4A1FEC + 0x648 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0x699 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0x744 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0x794 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0x83E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0x890 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0x947 + 0x2, &fWideScreenWidthScaleDown); Menu Background and Text
	CPatch::SetPointer(0x4A1FEC + 0x999 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0xBD0 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0xC21 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0xCCC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0xD1C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0xDC9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0xE1B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0xED8 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0xF2A + 0x2, &fWideScreenWidthScaleDown);*/

	CPatch::SetPointer(0x49057B + 0x4A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49057B + 0x4CC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49057B + 0x4F6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49057B + 0x67E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49057B + 0x6A8 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49057B + 0xE33 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__DrawContollerScreenExtraText + 0x116 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__DrawContollerScreenExtraText + 0x1A1 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__DrawContollerScreenExtraText + 0x44E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(debug_internal_something + 0xB68 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(debug_internal_something + 0xBBD + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(debug_internal_something + 0xE91 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(debug_internal_something + 0xFF0 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(debug_internal_something + 0x12AB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492C92 + 0xC6B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492C92 + 0xCBC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492C92 + 0xF35 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492C92 + 0xFF6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492C92 + 0x11F7 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492C92 + 0x124D + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492C92 + 0x1332 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492C92 + 0x138E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492C92 + 0x1DAC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492C92 + 0x23B8 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492C92 + 0x24A0 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492C92 + 0x25C9 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492C92 + 0x2608 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492C92 + 0x2796 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49589B + 0xA10 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49589B + 0xA6B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4964CA + 0x14A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4964CA + 0x193 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49A28E + 0x83 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49A28E + 0x211 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x89 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xFA + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x154 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x207 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x274 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x2C2 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x375 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x3E4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x434 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x4E9 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x556 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x5A5 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x658 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x6C8 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x717 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x7D1 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x841 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x88F + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x954 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x9C9 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xA18 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xADE + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xB4D + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xB9C + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xC62 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xCD1 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xD1E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49B892 + 0x421 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49B892 + 0x481 + 0x2, &fWideScreenHeightScaleDown);
	/*CPatch::SetPointer(0x49DDFF + 0x8B0 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x92F + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x12FA + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x143E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x1496 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x14F4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x154C + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x18FD + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x1955 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x19B3 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x1A0B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x1AE6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x1B3E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x1B9C + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x1BF4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x1D49 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x1E16 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x1E78 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DDFF + 0x1EF2 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0x674 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0x6B9 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0x76E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0x7B3 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0x86A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0x8B0 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0x973 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0x9B9 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0xBFC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0xC41 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0xCF6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0xD3B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0xDF5 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0xE3B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0xF04 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1FEC + 0xF4A + 0x2, &fWideScreenHeightScaleDown);*/




	CPatch::SetPointer(Render2DStuff + 0x287 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(Render2DStuff + 0x2DB + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(LoadingScreen + 0x118 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(LoadingScreen + 0x1DD + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(LoadingScreen + 0x290 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(LoadingScreen + 0x328 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(Render2DStuff + 0x11A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(Render2DStuff + 0x164 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(Render2DStuff + 0x1D3 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(Render2DStuff + 0x21D + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(LoadingScreen + 0x147 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(LoadingScreen + 0x206 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(LoadingScreen + 0x2B9 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(LoadingScreen + 0x312 + 0x2, &fWideScreenHeightScaleDown);

	


	CPatch::SetPointer(CRadar__DrawRadarMask + 0xF8 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarMask + 0x13E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarMask + 0x23D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarMask + 0x282 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarSection + 0x382 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarSection + 0x3C1 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawYouAreHereSprite + 0xB + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawYouAreHereSprite + 0x308 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarSprite + 0x9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawRotatingRadarSprite + 0x1A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__ShowRadarTraceWithHeight + 0xD2 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__ShowRadarTraceWithHeight + 0x1A3 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__ShowRadarTraceWithHeight + 0x255 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__ShowRadarTraceWithHeight + 0x330 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__ShowRadarTraceWithHeight + 0x3B6 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__ShowRadarTraceWithHeight + 0x43D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0xAB + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0xF4 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0x2E6 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0x32F + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0x71D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0x762 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x6A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x1C2 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x279 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x348 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x3F3 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x497 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x51D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x733 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawEntityBlip + 0x2BF + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawEntityBlip + 0x307 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawCoordBlip + 0x25F + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawCoordBlip + 0x2A7 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(CRadar__DrawRadarMask + 0x124 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarMask + 0x170 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarMask + 0x269 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarMask + 0x2B7 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarSection + 0x3A1 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarSection + 0x3FC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawYouAreHereSprite + 0x45 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawYouAreHereSprite + 0x2F2 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarSprite + 0x47 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawRotatingRadarSprite + 0x4D + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__ShowRadarTraceWithHeight + 0x72 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__ShowRadarTraceWithHeight + 0x14B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__ShowRadarTraceWithHeight + 0x217 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__ShowRadarTraceWithHeight + 0x2F2 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0xDB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0x12C + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0x310 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0x361 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0x749 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0x796 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x19C + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x251 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x322 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x3CB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x4C2 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x549 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x715 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawEntityBlip + 0x2ED + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawEntityBlip + 0x33B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawCoordBlip + 0x28D + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawCoordBlip + 0x2DB + 0x2, &fWideScreenHeightScaleDown);






	//CPatch::SetPointer(CHud__DrawAfterFade + 0x8E + 0x2, &fWideScreenWidthScaleDown);
	//CPatch::SetPointer(CHud__DrawAfterFade + 0x10C + 0x2, &fWideScreenWidthScaleDown);
	//CPatch::SetPointer(CHud__DrawAfterFade + 0x128 + 0x2, &fWideScreenWidthScaleDown);
	//CPatch::SetPointer(CHud__DrawAfterFade + 0x1EC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x31C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x343 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x433 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x45A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x716 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x740 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x870 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x89C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x908 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x944 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0xBF3 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x358 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x43C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x559 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x67D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x87A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x91E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x9C2 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xD90 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xECB + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xFDC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x12A1 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x138B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1431 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1458 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x15F2 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1691 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x18DD + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x193A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x19B9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1A1B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1B4D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1BAA + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1C29 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1CAE + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1D1A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1E98 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2011 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2108 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x219A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x24EF + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x251F + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x256D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x268F + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x29C1 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x29F1 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2A3F + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2B5B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2BBB + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2CBE + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2DB6 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2E39 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2EAA + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2F68 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3050 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x308B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3159 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3218 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x32D8 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3371 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3429 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x34E8 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3680 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x37B9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x387E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x38FC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3918 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x39DD + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3C02 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3C38 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3D00 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3D2D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3DB6 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x41BF + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x41EF + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4247 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x433C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x43FF + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x442C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4454 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x484C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x487C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x497F + 0x2, &fWideScreenWidthScaleDown);

		//CPatch::SetPointer(CHud__DrawAfterFade + 0x72 + 0x2, &fWideScreenHeightScaleDown);
		//CPatch::SetPointer(CHud__DrawAfterFade + 0x1BF + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__DrawAfterFade + 0x306 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__DrawAfterFade + 0x3B7 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__DrawAfterFade + 0x41D + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__DrawAfterFade + 0x4CE + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__DrawAfterFade + 0x6F8 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__DrawAfterFade + 0x7B4 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__DrawAfterFade + 0x85B + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__DrawAfterFade + 0x887 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__DrawAfterFade + 0x8EA + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__DrawAfterFade + 0x926 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__DrawAfterFade + 0xBCD + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x3B6 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x49A + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x5B1 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x6E2 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x849 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x932 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x9D6 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0xD6F + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0xEB5 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0xFBB + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x1254 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x13BC + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x141B + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x15D1 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x167B + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x18BC + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x1998 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x1A05 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x1B2C + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x1C08 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x1C98 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x1E73 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x1FFB + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x20E7 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x2178 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x24DA + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x250A + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x2548 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x266A + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x29AC + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x29DC + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x2A1A + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x2B36 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x2BA5 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x2C9D + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x2DA0 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x2E23 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x2E89 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x2F46 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x303A + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x3114 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x319B + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x3413 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x34A7 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x363A + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x3778 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x3862 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x39AF + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x3C22 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x3C64 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x3CEA + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x3D81 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x41A4 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x41DA + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x4311 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x43EA + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x4417 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x46EB + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x4837 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x4867 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(CHud__Draw + 0x4959 + 0x2, &fWideScreenHeightScaleDown);

		

	CPatch::SetPointer(CMBlur__AddRenderFx + 0x14A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMBlur__AddRenderFx + 0x330 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(CMBlur__AddRenderFx + 0x120 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMBlur__AddRenderFx + 0x306 + 0x2, &fWideScreenHeightScaleDown);




	CPatch::SetPointer(CParticle__Render + 0x3A2 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CParticle__Render + 0x523 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CParticle__Render + 0x648 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CParticle__Render + 0x773 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CParticle__Render + 0x80C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CParticle__Render + 0x8A6 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CParticle__Render + 0xDA8 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CParticle__Render + 0xE1C + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(CParticle__Render + 0x3E0 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CParticle__Render + 0x561 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CParticle__Render + 0x68A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CParticle__Render + 0x7B9 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CParticle__Render + 0x84C + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CParticle__Render + 0x8E6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CParticle__Render + 0xE9F + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CParticle__Render + 0xF16 + 0x2, &fWideScreenHeightScaleDown);



	CPatch::SetPointer(CSpecialFX__Render2DFXs + 0x30 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CSpecialFX__Render2DFXs + 0x3B8 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(CSpecialFX__Render2DFXs + 0x1A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CSpecialFX__Render2DFXs + 0x3A2 + 0x2, &fWideScreenHeightScaleDown);



	CPatch::SetPointer(cMusicManager__DisplayRadioStationName + 0x27C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(cMusicManager__DisplayRadioStationName + 0x2AB + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetFloat(0x6CC9B0, 0.675f);
	CPatch::SetFloat(0x6CC9B8, 0.4f);

	CPatch::SetPointer(cMusicManager__DisplayRadioStationName + 0x266 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(cMusicManager__DisplayRadioStationName + 0x2E7 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(cMusicManager__DisplayRadioStationName + 0x380 + 0x2, &fWideScreenHeightScaleDown);




	CPatch::SetPointer(CSceneEdit__Draw + 0x71 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x134 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x1AF + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x24B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x2C5 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x3D2 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x487 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(CSceneEdit__Draw + 0x5B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x114 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x195 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x22B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x2AB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x3AA + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x465 + 0x2, &fWideScreenHeightScaleDown);




	//CPatch::SetPointer(CRunningScript__ProcessCommands1100To1199 + 0x423 + 0x2, &fWideScreenWidthScaleDown);


	//CPatch::SetPointer(CClouds__Render + 0x66B + 0x2, &fWideScreenWidthScaleDown);
	


	CPatch::SetPointer(0x61DAF0 + 0xC9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x61DAF0 + 0x158 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x61DAF0 + 0x202 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x61DAF0 + 0x28C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x61DAF0 + 0x2C1 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x61DAF0 + 0x33E + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(0x61DAF0 + 0xEC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x61DAF0 + 0x17B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x61DAF0 + 0x225 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x61DAF0 + 0x270 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x61DAF0 + 0x328 + 0x2, &fWideScreenHeightScaleDown);


	CPatch::SetPointer(CReplay__Display + 0x66 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(CReplay__Display + 0x50 + 0x2, &fWideScreenHeightScaleDown);


	CPatch::SetDouble(0x696AE0, (1.8 / 1.50)); //h
	CPatch::SetDouble(0x696AE8, (1.7 / 1.50)); //w
	CPatch::SetDouble(0x696AF0, (1.5 / 1.50)); //w
}

void MenuFix()
{
	CPatch::SetUInt(0x1 + 0x48F392, CLASS_pclRsGlobal->m_iScreenWidth); // Map help text scaling
	//CPatch::SetUInt(0x1 + 0x48F43A, CLASS_pclRsGlobal->m_iScreenWidth); // Map help text alignment
	CPatch::SetUInt(0x1 + 0x49E0A5, CLASS_pclRsGlobal->m_iScreenWidth); // Text at right top corner scaling
	CPatch::SetUInt(0x1 + 0x49E6E3, CLASS_pclRsGlobal->m_iScreenWidth); // Left aligned text scaling 
	//CPatch::SetUInt(0x400000+0x1+0x9FC3D, CLASS_pclRsGlobal->m_iScreenWidth); // Game menu text alignment
	CPatch::SetUInt(0x1 + 0x49FC2C, CLASS_pclRsGlobal->m_iScreenWidth); // Right aligned text scaling 

	CPatch::SetUInt(0x1 + 0x490622, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x490CB1, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4911E0, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x491264, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4912F4, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49145C, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4918DE, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x491989, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x491A35, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x491AA1, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x491B65, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x491BD1, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49213C, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4922C0, CLASS_pclRsGlobal->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x4924CA, CLASS_pclRsGlobal->m_iScreenWidth); //green selection line
	CPatch::SetUInt(0x1 + 0x492634, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49267E, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49285C, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4928B6, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x492B13, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x492BEE, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x492CA8, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4932CA, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x493364, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x493416, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x493543, CLASS_pclRsGlobal->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x4935A7, CLASS_pclRsGlobal->m_iScreenWidth); //skins page
	CPatch::SetUInt(0x1 + 0x49363E, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49368B, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4936D5, CLASS_pclRsGlobal->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x493735, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4937AE, CLASS_pclRsGlobal->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x493B74, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4947C0, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49480D, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49485A, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4948A7, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4948F1, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49498B, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4949FF, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x494A5D, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4953E9, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4954C4, CLASS_pclRsGlobal->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x4955D2, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4972EE, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49730D, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x497430, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49744F, CLASS_pclRsGlobal->m_iScreenWidth);
		//CPatch::SetUInt(0x1 + 0x498225, CLASS_pclRsGlobal->m_iScreenWidth);
		//CPatch::SetUInt(0x1 + 0x49825E, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49869B, CLASS_pclRsGlobal->m_iScreenWidth); CPatch::SetFloat(0x68C508, 0.42000002f * 2.0f); //Loading game text
	CPatch::SetUInt(0x1 + 0x4986F9, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49A2D2, CLASS_pclRsGlobal->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x49A330, CLASS_pclRsGlobal->m_iScreenWidth); // BRIEF text
	CPatch::SetUInt(0x1 + 0x49A40A, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49A456, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B330, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B47C, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B4C7, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B52D, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B5B5, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B5FF, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B6E3, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B754, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49BA6B, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49BAB5, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49C08E, CLASS_pclRsGlobal->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x49C0EC, CLASS_pclRsGlobal->m_iScreenWidth); //criminal rating (stats)
	//CPatch::SetUInt(0x1 + 0x49C194, CLASS_pclRsGlobal->m_iScreenWidth); //criminal rating (stats)
	//CPatch::SetUInt(0x1 + 0x49C23F, CLASS_pclRsGlobal->m_iScreenWidth); //criminal rating(stats)
	//CPatch::SetUInt(0x1 + 0x49C2D7, CLASS_pclRsGlobal->m_iScreenWidth); //criminal rating (stats)

		/*CPatch::SetUInt(0x1 + 0x49C3B5, CLASS_pclRsGlobal->m_iScreenWidth); // draw radio logos slider in menu
		CPatch::SetUInt(0x1 + 0x49C406, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49C445, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49C484, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49C7C3, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49C810, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49C8AA, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49C8F7, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49C9A7, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49C9F4, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CA92, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CADF, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CB72, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CBBF, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CC7C, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CCC9, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CD67, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CDB4, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CE52, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CE9F, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CF32, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CF7F, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D03C, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D089, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D11F, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D16C, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D21D, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D26C, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D4DE, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D52F, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D56E, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D5AD, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D65D, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D6AE, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D6ED, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D72C, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D7D9, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D82A, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D869, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D8A8, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D95B, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D9AC, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D9EB, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DA2A, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DAD7, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DB28, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DB67, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DBA6, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DC59, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DCAA, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DCE9, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DD28, CLASS_pclRsGlobal->m_iScreenWidth);*/

	CPatch::SetUInt(0x1 + 0x49E216, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49E2F1, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49E664, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49F0AE, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49FBCA, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4A0E57, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4A1393, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4A14AC, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4A1549, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4A156E, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4A18F3, CLASS_pclRsGlobal->m_iScreenWidth);

	//CPatch::SetUInt(0x1 + 0x4A19DE, CLASS_pclRsGlobal->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x4A1A7B, CLASS_pclRsGlobal->m_iScreenWidth); // mouse sensitivity
	//CPatch::SetUInt(0x1 + 0x4A1AA0, CLASS_pclRsGlobal->m_iScreenWidth);
		/*CPatch::SetUInt(0x1 + 0x4A1E15, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x4A38EE, CLASS_pclRsGlobal->m_iScreenWidth);*/

	float fMenuTextScale = 0.6f*(CLASS_pclRsGlobal->m_iScreenHeight / SCREEN_RESOLUTION_HEIGHT);
	float fMenuTextScale2 = 0.48f*(CLASS_pclRsGlobal->m_iScreenHeight / SCREEN_RESOLUTION_HEIGHT);
	CPatch::SetFloat(0x68C0C0, fMenuTextScale);
	CPatch::SetFloat(0x68C5C0, fMenuTextScale2);

	CPatch::SetPointer(0x4956A2 + 0xFA + 0x2, &fvcLogoScale);
	CPatch::SetPointer(0x4A1FEC + 0x126C + 0x2, &fvcLogoScale);
	CPatch::SetPointer(0x4A1FEC + 0x1353 + 0x2, &fvcLogoScale);


	CPatch::SetFloat(0x68C0B8, 0.5f); // controls page text scaling
	CPatch::SetFloat(0x68C100, 0.8f); // controls page text scaling

	CPatch::SetFloat(0x68C094, 2.0f); // menu header scaling
	CPatch::SetFloat(0x68C118, 1.8f); // controls page (in car, on foot text)

	CPatch::SetFloat(0x68B34C, 1.2f); //Map help text
	CPatch::SetFloat(0x68B344, 0.5f);
	CPatch::Nop(0x48F3B8, 5);

	CPatch::SetFloat(0x68C680, 0.86f); //Stats text width scale
	CPatch::SetUInt(0x49C19F + 0x2, 0x68C094);
}


void __declspec(naked)RsSelectDeviceHook()
{
	MenuFix();
	_asm
	{
		add     esp, 10h
		pop     ebp
		pop     esi
		pop     ebx
		ret
	}

}

void ApplyINIchanges()
{
	CIniReader iniReader("GTAVC_widescreen_fix.ini");
	fHudWidthScale = iniReader.ReadFloat("MAIN", "HudWidthScale", 0.62221788786f);
	fHudHeightScale = iniReader.ReadFloat("MAIN", "HudHeightScale", 0.66666670937f);
	fRadarWidthScale = iniReader.ReadFloat("MAIN", "RadarWidthScale", 0.80354591724f);
	fSubtitlesScale = iniReader.ReadFloat("MAIN", "SubtitlesScale", 0.80354591724f);

	fMVLScopeFix = iniReader.ReadFloat("MAIN", "MVLScopeFix", 0.0f);

	int SmallerTextShadows = iniReader.ReadInteger("MAIN", "SmallerTextShadows", 1);

	RestoreCutsceneFOV = iniReader.ReadInteger("MAIN", "RestoreCutsceneFOV", 1);

	if (iniReader.ReadInteger("MAIN", "FixVehicleLights", 1))
	{
		CPatch::SetUInt(0x543A82, 0x67D188); //lamp corona stretch
		CPatch::SetUInt(0x543942, 0x695AA8); //car lights stretch
		CPatch::SetFloat(0x694914, 3.0f); //car lights stretch
	}

	if (!fHudWidthScale || !fHudHeightScale) { fHudWidthScale = 0.62221788786f; fHudHeightScale = 0.66666670937f; }
	if (!fRadarWidthScale) { fRadarWidthScale = 0.80354591724f; }

	if (SmallerTextShadows){ CPatch::RedirectJump(0x54FE10, SetDropShadowPosition); }

	fPlayerMarkerPos = 94.0f * fRadarWidthScale;

	if (fSubtitlesScale)
	{
		CPatch::SetFloat(0x696A88, 1.2f * fSubtitlesScale);
		CPatch::SetFloat(0x696C30, 0.57999998f * fSubtitlesScale);
	}

	CPatch::SetPointer(CHud__Draw + 0x358 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x43C + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x559 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x67D + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x87A + 0x2, &fCustomWideScreenWidthScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0x91E + 0x2, &fCustomWideScreenWidthScaleDown); //sniper crosshair fix
	CPatch::SetPointer(CHud__Draw + 0x9C2 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xD90 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xECB + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xFDC + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x12A1 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x138B + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1431 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1458 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x15F2 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1691 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x18DD + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x193A + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x19B9 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1A1B + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1B4D + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1BAA + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1C29 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1CAE + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1D1A + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1E98 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2011 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2108 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x219A + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x24EF + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x251F + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x256D + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x268F + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x29C1 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x29F1 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2A3F + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2B5B + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2BBB + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2CBE + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2DB6 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2E39 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2EAA + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2F68 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3050 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x308B + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3159 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3218 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x32D8 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3371 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3429 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x34E8 + 0x2, &fCustomWideScreenWidthScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0x3680 + 0x2, &fCustomWideScreenWidthScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0x37B9 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x387E + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x38FC + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3918 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x39DD + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3C02 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3C38 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3D00 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3D2D + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3DB6 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x41BF + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x41EF + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4247 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x433C + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x43FF + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x442C + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4454 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x484C + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x487C + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x497F + 0x2, &fCustomWideScreenWidthScaleDown);


	CPatch::SetPointer(CHud__Draw + 0x3B6 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x49A + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x5B1 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x6E2 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x849 + 0x2, &fCustomWideScreenHeightScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0x932 + 0x2, &fCustomWideScreenHeightScaleDown); //sniper crosshair fix
	CPatch::SetPointer(CHud__Draw + 0x9D6 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xD6F + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xEB5 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xFBB + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1254 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x13BC + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x141B + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x15D1 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x167B + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x18BC + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1998 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1A05 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1B2C + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1C08 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1C98 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1E73 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1FFB + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x20E7 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2178 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x24DA + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x250A + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2548 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x266A + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x29AC + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x29DC + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2A1A + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2B36 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2BA5 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2C9D + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2DA0 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2E23 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2E89 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2F46 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x303A + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3114 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x319B + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3413 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x34A7 + 0x2, &fCustomWideScreenHeightScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0x363A + 0x2, &fCustomWideScreenHeightScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0x3778 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3862 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x39AF + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3C22 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3C64 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3CEA + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3D81 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x41A4 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x41DA + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4311 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x43EA + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4417 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x46EB + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4837 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4867 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4959 + 0x2, &fCustomWideScreenHeightScaleDown);


	if (fRadarWidthScale)
	{
		CPatch::SetPointer(CRadar__DrawRadarMask + 0xF8 + 0x2, &fCustomRadarWidthScale);
		CPatch::SetPointer(CRadar__DrawRadarMask + 0x13E + 0x2, &fCustomRadarWidthScale);
		CPatch::SetPointer(CRadar__DrawRadarMask + 0x23D + 0x2, &fCustomRadarWidthScale);
		CPatch::SetPointer(CRadar__DrawRadarMask + 0x282 + 0x2, &fCustomRadarWidthScale);
		CPatch::SetPointer(CRadar__DrawRadarSection + 0x382 + 0x2, &fCustomRadarWidthScale);
		CPatch::SetPointer(CRadar__DrawRadarSection + 0x3C1 + 0x2, &fCustomRadarWidthScale);


		CPatch::SetPointer(CRadar__DrawRotatingRadarSprite + 0x1A + 0x2, &fCustomRadarWidthScale);

		CPatch::SetPointer(CRadar__DrawBlips + 0xF4 + 0x2 + 0x6, &fPlayerMarkerPos);
		CPatch::SetPointer(CRadar__DrawBlips + 0x2E6 + 0x2 + 0x6, &fPlayerMarkerPos);
		//CPatch::SetPointer(CRadar__DrawBlips + 0x32F + 0x2 + 0x6, &fPlayerMarkerPos);

		CPatch::SetPointer(CRadar__DrawBlips + 0x32F + 0x2, &fCustomRadarWidthScale);
		CPatch::SetPointer(CRadar__DrawCoordBlip + 0x2A7 + 0x2, &fCustomRadarWidthScale);

		CPatch::SetPointer(CRadar__DrawEntityBlip + 0x2BF + 0x2 + 0x6, &fPlayerMarkerPos);
		CPatch::SetPointer(CRadar__DrawEntityBlip + 0x307 + 0x2 + 0x6, &fPlayerMarkerPos);

		CPatch::SetPointer(CHud__Draw + 0x3680 + 0x2, &fCustomRadarWidthScale);
		CPatch::SetPointer(CHud__Draw + 0x37B9 + 0x2, &fCustomRadarWidthScale);
	}
}









///////////////////////////////////////////steam//////////////////////////////////

long double __cdecl CDraw__CalculateAspectRatio_steam()
{
	*VAR_pfScreenAspectRatio_steam = (float)CLASS_pclRsGlobal_steam->m_iScreenWidth / (float)CLASS_pclRsGlobal_steam->m_iScreenHeight;

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

	//logo scaling
	fvcLogoScale = (245.312497f * fWideScreenWidthScaleDown) / 1.7f;

	//Ini options
	fCustomWideScreenWidthScaleDown = fWideScreenWidthScaleDown * fHudWidthScale;
	fCustomWideScreenHeightScaleDown = fWideScreenHeightScaleDown * fHudHeightScale;
	fCustomRadarWidthScale = fWideScreenWidthScaleDown * fRadarWidthScale;

	return fDynamicScreenFieldOfViewScale;
}

inline float getDynamicScreenFieldOfView_steam(float fFactor)
{
	fFiretruckAmblncFix = 70.0f / fFactor;

	if (!(char)*(DWORD*)0x7E2705 == 0 && RestoreCutsceneFOV)
	{
		return fFactor;
	}

	return fFactor * fDynamicScreenFieldOfViewScale;
}

void setScreenFieldOfView_steam(float fFactor) {
	*VAR_pfScreenFieldOfView_steam = /*!CLASS_pclCamera->m_bWidescreen ? */getDynamicScreenFieldOfView_steam(fFactor)/* : fFactor*/;
}

void __declspec(naked)FiretruckAmblncFix_steam()
{
	__asm mov eax, fFiretruckAmblncFix
	__asm mov[ebx + 0xF4], eax
	__asm mov eax, 0x46DC7D
	__asm jmp eax
}


void __declspec(naked)DrawBordersForWideScreen_patch_steam()
{
	_asm
	{
			cmp byte ptr ds : [867662h], 00
			jz label1
			ret
		label1 :
			sub esp, 38h
			cmp byte ptr ds : [7E2705h], 00
			mov edx, 0x46F9E6
			jmp edx
	}
}


int __declspec(naked)SetDropShadowPosition_steam(short)
{
	_asm
	{
		mov     eax, [esp + 4]
		cmp eax, 2
		jne label
		dec eax
	label :
		mov		ds : [97D870h], ax
		retn
	}
}


#undef	 	CDarkel__DrawMessages
#undef	 	CGarages__PrintMessages
#undef	 	CMenuManager__PrintMap
#undef	 	CMenuManager__DrawContollerScreenExtraText
#undef	 	CRadar__DrawRadarMask
#undef	 	CRadar__DrawRadarSection
#undef	 	CRadar__DrawYouAreHereSprite
#undef	 	CRadar__DrawRadarSprite
#undef	 	CRadar__DrawRotatingRadarSprite
#undef	 	CRadar__ShowRadarTraceWithHeight
#undef	 	CRadar__DrawBlips
#undef	 	CRadar__DrawLegend
#undef	 	CRadar__DrawEntityBlip
#undef	 	CRadar__DrawCoordBlip
#undef	 	CClouds__Render
#undef	 	CHud__DrawAfterFade
#undef	 	CHud__Draw
#undef	 	CMBlur__AddRenderFx
#undef	 	CParticle__Render
#undef	 	CSpecialFX__Render2DFXs
#undef	 	cMusicManager__DisplayRadioStationName
#undef	 	CSceneEdit__Draw
#undef	 	CRunningScript__ProcessCommands1100To1199
#undef	 	CReplay__Display
#undef	 	Render2DStuff
#undef	 	LoadingScreen
#undef	 	debug_internal_something 
#undef	 	CMenuManager_PrintStringMenu 


#define	 	CDarkel__DrawMessages	 	0x429F80
#define	 	CGarages__PrintMessages	 	0x42F250
#define	 	CMenuManager__PrintMap	 	0x49A3F9
#define	 	CMenuManager__DrawContollerScreenExtraText	 	0x49132B
#define	 	CRadar__DrawRadarMask	 	0x4C17C0
#define	 	CRadar__DrawRadarSection	 	0x4C1B00
#define	 	CRadar__DrawYouAreHereSprite	 	0x4C2720
#define	 	CRadar__DrawRadarSprite	 	0x4C2AA0
#define	 	CRadar__DrawRotatingRadarSprite	 	0x4C2C10
#define	 	CRadar__ShowRadarTraceWithHeight	 	0x4C3090
#define	 	CRadar__DrawBlips	 	0x4C3FA0
#define	 	CRadar__DrawLegend	 	0x4C4790
#define	 	CRadar__DrawEntityBlip	 	0x4C4EF0
#define	 	CRadar__DrawCoordBlip	 	0x4C5400
#define	 	CClouds__Render	 	0x53F160
#define	 	CHud__DrawAfterFade	 	0x5564C0
#define	 	CHud__Draw	 	0x557100
#define	 	CMBlur__AddRenderFx	 	0x55CF40
#define	 	CParticle__Render	 	0x5606A0
#define	 	CSpecialFX__Render2DFXs	 	0x573D00
#define	 	cMusicManager__DisplayRadioStationName	 	0x5F97A0
#define	 	CSceneEdit__Draw	 	0x6055F0
#define	 	CRunningScript__ProcessCommands1100To1199	 	0x605F70
#define	 	CReplay__Display	 	0x620460
#define	 	Render2DStuff 0x4A5F30
#define	 	LoadingScreen 0x4A6770
#define	 	debug_internal_something 0x4917B7
#define	 	CMenuManager_PrintStringMenu 0x4983FD

void HudFix_steam()
{

	CPatch::SetPointer(CDarkel__DrawMessages + 0xA7 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x168 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x269 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x369 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x3D6 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x475 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x4DE + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x569 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(CDarkel__DrawMessages + 0x91 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x152 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x254 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x347 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x3B4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x453 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x4BD + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CDarkel__DrawMessages + 0x553 + 0x2, &fWideScreenHeightScaleDown);




	CPatch::SetPointer(CGarages__PrintMessages + 0x45 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(CGarages__PrintMessages + 0x2F + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CGarages__PrintMessages + 0x126 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CGarages__PrintMessages + 0x1B4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CGarages__PrintMessages + 0x208 + 0x2, &fWideScreenHeightScaleDown);




	CPatch::SetPointer(0x48F249 + 0x94 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(debug_internal_something + 0xFD + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x492BA2 + 0x653 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager_PrintStringMenu + 0x124 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x4D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x12B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x178 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x1D2 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x29F + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x2E8 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x340 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x412 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x45A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x4B3 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x581 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x5CA + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x622 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x6F3 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x73D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x79B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x86B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x8B5 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x91E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x9F4 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xA3C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xAA8 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xB78 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xBC2 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xC2C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xCFA + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xD44 + 0x2, &fWideScreenWidthScaleDown);

	/*CPatch::SetPointer(0x49DCBE + 0x2C7 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x140C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x146A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x14C4 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x1524 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x18CB + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x1929 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x1983 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x19E3 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x1AB4 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x1B12 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x1B6C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x1BCC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x1D0C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x24A8 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x29D1 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x2F0D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x3449 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x39A9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x3ECF + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(0x4A1EAB + 0x648 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0x699 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0x744 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0x794 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0x83E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0x890 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0x947 + 0x2, &fWideScreenWidthScaleDown); Menu Background and Text
	CPatch::SetPointer(0x4A1EAB + 0x999 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0xBD0 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0xC21 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0xCCC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0xD1C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0xDC9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0xE1B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0xED8 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0xF2A + 0x2, &fWideScreenWidthScaleDown);*/

	CPatch::SetPointer(0x49048B + 0x4A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49048B + 0x4CC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49048B + 0x4F6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49048B + 0x67E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49048B + 0x6A8 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49048B + 0xE33 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__DrawContollerScreenExtraText + 0x116 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__DrawContollerScreenExtraText + 0x1A1 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__DrawContollerScreenExtraText + 0x44E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(debug_internal_something + 0xB68 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(debug_internal_something + 0xBBD + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(debug_internal_something + 0xE91 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(debug_internal_something + 0xFF0 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(debug_internal_something + 0x12AB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492BA2 + 0xC6B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492BA2 + 0xCBC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492BA2 + 0xF35 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492BA2 + 0xFF6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492BA2 + 0x11F7 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492BA2 + 0x124D + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492BA2 + 0x1332 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492BA2 + 0x138E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492BA2 + 0x1DAC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492BA2 + 0x23B8 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492BA2 + 0x24A0 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492BA2 + 0x25C9 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492BA2 + 0x2608 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492BA2 + 0x2796 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4957AB + 0xA10 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4957AB + 0xA6B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4963DA + 0x14A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4963DA + 0x193 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49A1AF + 0x83 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49A1AF + 0x211 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x89 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xFA + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x154 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x207 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x274 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x2C2 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x375 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x3E4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x434 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x4E9 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x556 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x5A5 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x658 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x6C8 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x717 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x7D1 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x841 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x88F + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x954 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0x9C9 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xA18 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xADE + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xB4D + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xB9C + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xC62 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xCD1 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__PrintMap + 0xD1E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49B7B3 + 0x421 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49B7B3 + 0x481 + 0x2, &fWideScreenHeightScaleDown);
	/*CPatch::SetPointer(0x49DCBE + 0x8B0 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x92F + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x12FA + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x143E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x1496 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x14F4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x154C + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x18FD + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x1955 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x19B3 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x1A0B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x1AE6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x1B3E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x1B9C + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x1BF4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x1D49 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x1E16 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x1E78 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DCBE + 0x1EF2 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0x674 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0x6B9 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0x76E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0x7B3 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0x86A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0x8B0 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0x973 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0x9B9 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0xBFC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0xC41 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0xCF6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0xD3B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0xDF5 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0xE3B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0xF04 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A1EAB + 0xF4A + 0x2, &fWideScreenHeightScaleDown);*/




	CPatch::SetPointer(Render2DStuff + 0x287 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(Render2DStuff + 0x2DB + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(LoadingScreen + 0x118 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(LoadingScreen + 0x1DD + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(LoadingScreen + 0x290 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(LoadingScreen + 0x328 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(Render2DStuff + 0x11A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(Render2DStuff + 0x164 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(Render2DStuff + 0x1D3 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(Render2DStuff + 0x21D + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(LoadingScreen + 0x147 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(LoadingScreen + 0x206 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(LoadingScreen + 0x2B9 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(LoadingScreen + 0x312 + 0x2, &fWideScreenHeightScaleDown);




	CPatch::SetPointer(CRadar__DrawRadarMask + 0xF8 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarMask + 0x13E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarMask + 0x23D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarMask + 0x282 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarSection + 0x382 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarSection + 0x3C1 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawYouAreHereSprite + 0xB + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawYouAreHereSprite + 0x308 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarSprite + 0x9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawRotatingRadarSprite + 0x1A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__ShowRadarTraceWithHeight + 0xD2 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__ShowRadarTraceWithHeight + 0x1A3 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__ShowRadarTraceWithHeight + 0x255 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__ShowRadarTraceWithHeight + 0x330 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__ShowRadarTraceWithHeight + 0x3B6 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__ShowRadarTraceWithHeight + 0x43D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0x9B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0xE4 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0x2D6 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0x31F + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0x70D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0x752 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x6A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x1C2 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x279 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x348 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x3F3 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x497 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x51D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x733 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawEntityBlip + 0x2BF + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawEntityBlip + 0x307 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawCoordBlip + 0x25F + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CRadar__DrawCoordBlip + 0x2A7 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(CRadar__DrawRadarMask + 0x124 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarMask + 0x170 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarMask + 0x269 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarMask + 0x2B7 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarSection + 0x3A1 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarSection + 0x3FC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawYouAreHereSprite + 0x45 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawYouAreHereSprite + 0x2F2 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawRadarSprite + 0x47 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawRotatingRadarSprite + 0x4D + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__ShowRadarTraceWithHeight + 0x72 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__ShowRadarTraceWithHeight + 0x14B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__ShowRadarTraceWithHeight + 0x217 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__ShowRadarTraceWithHeight + 0x2F2 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0xCB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0x11C + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0x300 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0x351 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0x739 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawBlips + 0x786 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x19C + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x251 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x322 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x3CB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x4C2 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x549 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawLegend + 0x715 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawEntityBlip + 0x2ED + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawEntityBlip + 0x33B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawCoordBlip + 0x28D + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CRadar__DrawCoordBlip + 0x2DB + 0x2, &fWideScreenHeightScaleDown);






	//CPatch::SetPointer(CHud__DrawAfterFade + 0x8E + 0x2, &fWideScreenWidthScaleDown);
	//CPatch::SetPointer(CHud__DrawAfterFade + 0x10C + 0x2, &fWideScreenWidthScaleDown);
	//CPatch::SetPointer(CHud__DrawAfterFade + 0x128 + 0x2, &fWideScreenWidthScaleDown);
	//CPatch::SetPointer(CHud__DrawAfterFade + 0x1EC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x31C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x343 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x433 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x45A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x716 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x740 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x870 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x89C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x908 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x944 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0xBF3 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x358 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x43C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x559 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x67D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x87A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x91E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x9C2 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xD90 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xECB + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xFDC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x12A1 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x138B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1431 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1458 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x15F2 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1691 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x18DD + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x193A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x19B9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1A1B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1B4D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1BAA + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1C29 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1CAE + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1D1A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1E98 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2011 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2108 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x219A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x24EF + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x251F + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x256D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x268F + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x29C1 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x29F1 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2A3F + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2B5B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2BBB + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2CBE + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2DB6 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2E39 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2EAA + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2F68 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3050 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x308B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3159 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3218 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x32D8 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3371 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3429 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x34E8 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3680 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x37B9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x387E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x38FC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3918 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x39DD + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3C02 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3C38 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3D00 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3D2D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3DB6 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x41BF + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x41EF + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4247 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x433C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x43FF + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x442C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4454 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x484C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x487C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x497F + 0x2, &fWideScreenWidthScaleDown);

	//CPatch::SetPointer(CHud__DrawAfterFade + 0x72 + 0x2, &fWideScreenHeightScaleDown);
	//CPatch::SetPointer(CHud__DrawAfterFade + 0x1BF + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x306 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x3B7 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x41D + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x4CE + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x6F8 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x7B4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x85B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x887 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x8EA + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x926 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0xBCD + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3B6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x49A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x5B1 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x6E2 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x849 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x932 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x9D6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xD6F + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xEB5 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xFBB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1254 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x13BC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x141B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x15D1 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x167B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x18BC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1998 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1A05 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1B2C + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1C08 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1C98 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1E73 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1FFB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x20E7 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2178 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x24DA + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x250A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2548 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x266A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x29AC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x29DC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2A1A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2B36 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2BA5 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2C9D + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2DA0 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2E23 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2E89 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2F46 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x303A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3114 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x319B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3413 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x34A7 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x363A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3778 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3862 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x39AF + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3C22 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3C64 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3CEA + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3D81 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x41A4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x41DA + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4311 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x43EA + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4417 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x46EB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4837 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4867 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4959 + 0x2, &fWideScreenHeightScaleDown);



	CPatch::SetPointer(CMBlur__AddRenderFx + 0x14A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CMBlur__AddRenderFx + 0x330 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(CMBlur__AddRenderFx + 0x120 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMBlur__AddRenderFx + 0x306 + 0x2, &fWideScreenHeightScaleDown);




	CPatch::SetPointer(CParticle__Render + 0x3A2 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CParticle__Render + 0x523 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CParticle__Render + 0x648 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CParticle__Render + 0x773 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CParticle__Render + 0x80C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CParticle__Render + 0x8A6 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CParticle__Render + 0xDA8 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CParticle__Render + 0xE1C + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(CParticle__Render + 0x3E0 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CParticle__Render + 0x561 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CParticle__Render + 0x68A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CParticle__Render + 0x7B9 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CParticle__Render + 0x84C + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CParticle__Render + 0x8E6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CParticle__Render + 0xE9F + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CParticle__Render + 0xF16 + 0x2, &fWideScreenHeightScaleDown);



	CPatch::SetPointer(CSpecialFX__Render2DFXs + 0x30 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CSpecialFX__Render2DFXs + 0x3B8 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(CSpecialFX__Render2DFXs + 0x1A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CSpecialFX__Render2DFXs + 0x3A2 + 0x2, &fWideScreenHeightScaleDown);



	CPatch::SetPointer(cMusicManager__DisplayRadioStationName + 0x27C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(cMusicManager__DisplayRadioStationName + 0x2AB + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetFloat(0x6CB9A8, 0.675f);
	CPatch::SetFloat(0x6CB9B0, 0.4f);

	CPatch::SetPointer(cMusicManager__DisplayRadioStationName + 0x266 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(cMusicManager__DisplayRadioStationName + 0x2E7 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(cMusicManager__DisplayRadioStationName + 0x380 + 0x2, &fWideScreenHeightScaleDown);




	CPatch::SetPointer(CSceneEdit__Draw + 0x71 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x134 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x1AF + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x24B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x2C5 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x3D2 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x487 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(CSceneEdit__Draw + 0x5B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x114 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x195 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x22B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x2AB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x3AA + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CSceneEdit__Draw + 0x465 + 0x2, &fWideScreenHeightScaleDown);




	//CPatch::SetPointer(CRunningScript__ProcessCommands1100To1199 + 0x423 + 0x2, &fWideScreenWidthScaleDown);


	//CPatch::SetPointer(CClouds__Render + 0x66B + 0x2, &fWideScreenWidthScaleDown);



	CPatch::SetPointer(0x61D730 + 0xC9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x61D730 + 0x158 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x61D730 + 0x202 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x61D730 + 0x28C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x61D730 + 0x2C1 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x61D730 + 0x33E + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(0x61D730 + 0xEC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x61D730 + 0x17B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x61D730 + 0x225 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x61D730 + 0x270 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x61D730 + 0x328 + 0x2, &fWideScreenHeightScaleDown);


	CPatch::SetPointer(CReplay__Display + 0x66 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(CReplay__Display + 0x50 + 0x2, &fWideScreenHeightScaleDown);


	CPatch::SetDouble(0x695AE8, (1.8 / 1.50)); //h
	CPatch::SetDouble(0x695AF0, (1.7 / 1.50)); //w
	CPatch::SetDouble(0x695AF8, (1.5 / 1.50)); //w
}

void MenuFix_steam()
{
	CPatch::SetUInt(0x1 + 0x48F2A2, CLASS_pclRsGlobal_steam->m_iScreenWidth); // Map help text scaling
	//CPatch::SetUInt(0x1 + 0x48F34A, CLASS_pclRsGlobal_steam->m_iScreenWidth); // Map help text alignment
	CPatch::SetUInt(0x1 + 0x49DF64, CLASS_pclRsGlobal_steam->m_iScreenWidth); // Text at right top corner scaling
	CPatch::SetUInt(0x1 + 0x49E5A2, CLASS_pclRsGlobal_steam->m_iScreenWidth); // Left aligned text scaling 
	//CPatch::SetUInt(0x400000+0x1+0x9FC3D, CLASS_pclRsGlobal_steam->m_iScreenWidth); // Game menu text alignment
	CPatch::SetUInt(0x1 + 0x49FAEB, CLASS_pclRsGlobal_steam->m_iScreenWidth); // Right aligned text scaling 

	CPatch::SetUInt(0x1 + 0x490532, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x490BC1, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4910F0, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x491174, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x491204, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49136C, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4917EE, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x491899, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x491945, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4919B1, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x491A75, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x491AE1, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49204C, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4921D0, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x4923DA, CLASS_pclRsGlobal_steam->m_iScreenWidth); //green selection line
	CPatch::SetUInt(0x1 + 0x492544, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49258E, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49276C, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4927C6, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x492A23, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x492AFE, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x492BB8, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4931DA, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x493274, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x493326, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x493453, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4934B7, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49354E, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49359B, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4935E5, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x493645, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4936BE, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x493A84, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4946D0, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49471D, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49476A, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4947B7, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x494801, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49489B, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49490F, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49496D, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4952F9, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4953D4, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4954E2, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4971FE, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49721D, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x497340, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49735F, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x498135, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x49816E, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4985AB, CLASS_pclRsGlobal_steam->m_iScreenWidth); CPatch::SetFloat(0x68B508, 0.42000002f * *VAR_pfScreenAspectRatio_steam); //Loading game text
	CPatch::SetUInt(0x1 + 0x498609, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49A1F3, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x49A251, CLASS_pclRsGlobal_steam->m_iScreenWidth); // BRIEF text
	CPatch::SetUInt(0x1 + 0x49A32B, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49A377, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B251, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B39D, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B3E8, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B44E, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B4D6, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B520, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B604, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B675, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B975, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B9BF, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49BF4D, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x49BFAB, CLASS_pclRsGlobal_steam->m_iScreenWidth); //criminal rating (stats)
	//CPatch::SetUInt(0x1 + 0x49C053, CLASS_pclRsGlobal_steam->m_iScreenWidth); //criminal rating (stats)
	//CPatch::SetUInt(0x1 + 0x49C0FE, CLASS_pclRsGlobal_steam->m_iScreenWidth); //criminal rating(stats)
	//CPatch::SetUInt(0x1 + 0x49C196, CLASS_pclRsGlobal_steam->m_iScreenWidth); //criminal rating (stats)

	/*CPatch::SetUInt(0x1 + 0x49C274, CLASS_pclRsGlobal_steam->m_iScreenWidth); // draw radio logos slider in menu
	CPatch::SetUInt(0x1 + 0x49C2C5, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49C304, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49C343, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49C682, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49C6CF, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49C769, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49C7B6, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49C866, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49C8B3, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49C951, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49C99E, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49CA31, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49CA7E, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49CB3B, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49CB88, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49CC26, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49CC73, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49CD11, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49CD5E, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49CDF1, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49CE3E, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49CEFB, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49CF48, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49CFDE, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D02B, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D0DC, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D12B, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D39D, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D3EE, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D42D, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D46C, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D51C, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D56D, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D5AC, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D5EB, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D698, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D6E9, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D728, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D767, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D81A, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D86B, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D8AA, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D8E9, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D996, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49D9E7, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49DA26, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49DA65, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49DB18, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49DB69, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49DBA8, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49DBE7, CLASS_pclRsGlobal_steam->m_iScreenWidth);*/

	CPatch::SetUInt(0x1 + 0x49E0D5, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49E1B0, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49E523, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49EF6D, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49FA89, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4A0D16, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4A1252, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4A136B, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4A1408, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4A142D, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4A17B2, CLASS_pclRsGlobal_steam->m_iScreenWidth);

	//CPatch::SetUInt(0x1 + 0x4A189D, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x4A193A, CLASS_pclRsGlobal_steam->m_iScreenWidth); // mouse sensitivity
	//CPatch::SetUInt(0x1 + 0x4A195F, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	/*CPatch::SetUInt(0x1 + 0x4A1CD4, CLASS_pclRsGlobal_steam->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4A37AD, CLASS_pclRsGlobal_steam->m_iScreenWidth);*/

	float fMenuTextScale = 0.6f*(CLASS_pclRsGlobal_steam->m_iScreenHeight / SCREEN_RESOLUTION_HEIGHT);
	float fMenuTextScale2 = 0.48f*(CLASS_pclRsGlobal_steam->m_iScreenHeight / SCREEN_RESOLUTION_HEIGHT);
	CPatch::SetFloat(0x68B0C0, fMenuTextScale);
	CPatch::SetFloat(0x68B5C0, fMenuTextScale2);

	CPatch::SetPointer(0x4956A2 + 0xFA + 0x2, &fvcLogoScale);
	CPatch::SetPointer(0x4A1FEC + 0x126C + 0x2, &fvcLogoScale);
	CPatch::SetPointer(0x4A1FEC + 0x1353 + 0x2, &fvcLogoScale);

	CPatch::SetFloat(0x68B0B8, 0.5f); // controls page text scaling
	CPatch::SetFloat(0x68B100, 0.8f); // controls page text scaling

	CPatch::SetFloat(0x68B094, 2.0f); // menu header scaling
	CPatch::SetFloat(0x68B118, 1.8f); // controls page (in car, on foot text)

	CPatch::SetFloat(0x68A34C, 1.2f); //Map help text
	CPatch::SetFloat(0x68A344, 0.5f);
	CPatch::Nop(0x48F2C8, 5);

	CPatch::SetFloat(0x68B680, 0.86f); //Stats text width scale
	CPatch::SetUInt(0x49C05E + 0x2, 0x68B094);
}


void __declspec(naked)RsSelectDeviceHook_steam()
{
	MenuFix_steam();
	_asm
	{
			add     esp, 10h
			pop     ebp
			pop     esi
			pop     ebx
			ret
	}

}

void ApplyINIchanges_steam()
{
	CIniReader iniReader("GTAVC_widescreen_fix.ini");
	fHudWidthScale = iniReader.ReadFloat("MAIN", "HudWidthScale", 0.62221788786f);
	fHudHeightScale = iniReader.ReadFloat("MAIN", "HudHeightScale", 0.66666670937f);
	fRadarWidthScale = iniReader.ReadFloat("MAIN", "RadarWidthScale", 0.80354591724f);
	fSubtitlesScale = iniReader.ReadFloat("MAIN", "SubtitlesScale", 0.80354591724f);

	fMVLScopeFix = iniReader.ReadFloat("MAIN", "MVLScopeFix", 0.0f);

	int SmallerTextShadows = iniReader.ReadInteger("MAIN", "SmallerTextShadows", 1);

	RestoreCutsceneFOV = iniReader.ReadInteger("MAIN", "RestoreCutsceneFOV", 1);

	if (iniReader.ReadInteger("MAIN", "FixVehicleLights", 1))
	{
		CPatch::SetUInt(0x543972, 0x67C138); //lamp corona stretch
		CPatch::SetUInt(0x543832, 0x694AB0); //car lights stretch
		CPatch::SetFloat(0x69391C, 3.0f); //car lights stretch
	}

	if (!fHudWidthScale || !fHudHeightScale) { fHudWidthScale = 0.62221788786f; fHudHeightScale = 0.66666670937f; }
	if (!fRadarWidthScale) { fRadarWidthScale = 0.80354591724f; }

	if (SmallerTextShadows){ CPatch::RedirectJump(0x54FD00, SetDropShadowPosition); }

	fPlayerMarkerPos = 94.0f * fRadarWidthScale;

	if (fSubtitlesScale)
	{
		CPatch::SetFloat(0x695A90, 1.2f * fSubtitlesScale);
		CPatch::SetFloat(0x695C38, 0.57999998f * fSubtitlesScale);
	}

	CPatch::SetPointer(CHud__Draw + 0x358 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x43C + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x559 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x67D + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x87A + 0x2, &fCustomWideScreenWidthScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0x91E + 0x2, &fCustomWideScreenWidthScaleDown); //sniper crosshair fix
	CPatch::SetPointer(CHud__Draw + 0x9C2 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xD90 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xECB + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xFDC + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x12A1 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x138B + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1431 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1458 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x15F2 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1691 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x18DD + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x193A + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x19B9 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1A1B + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1B4D + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1BAA + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1C29 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1CAE + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1D1A + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1E98 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2011 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2108 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x219A + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x24EF + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x251F + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x256D + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x268F + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x29C1 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x29F1 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2A3F + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2B5B + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2BBB + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2CBE + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2DB6 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2E39 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2EAA + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2F68 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3050 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x308B + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3159 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3218 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x32D8 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3371 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3429 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x34E8 + 0x2, &fCustomWideScreenWidthScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0x3680 + 0x2, &fCustomWideScreenWidthScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0x37B9 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x387E + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x38FC + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3918 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x39DD + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3C02 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3C38 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3D00 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3D2D + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3DB6 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x41BF + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x41EF + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4247 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x433C + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x43FF + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x442C + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4454 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x484C + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x487C + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x497F + 0x2, &fCustomWideScreenWidthScaleDown);


	CPatch::SetPointer(CHud__Draw + 0x3B6 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x49A + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x5B1 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x6E2 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x849 + 0x2, &fCustomWideScreenHeightScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0x932 + 0x2, &fCustomWideScreenHeightScaleDown); //sniper crosshair fix
	CPatch::SetPointer(CHud__Draw + 0x9D6 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xD6F + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xEB5 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0xFBB + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1254 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x13BC + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x141B + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x15D1 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x167B + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x18BC + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1998 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1A05 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1B2C + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1C08 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1C98 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1E73 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x1FFB + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x20E7 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2178 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x24DA + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x250A + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2548 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x266A + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x29AC + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x29DC + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2A1A + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2B36 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2BA5 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2C9D + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2DA0 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2E23 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2E89 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x2F46 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x303A + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3114 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x319B + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3413 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x34A7 + 0x2, &fCustomWideScreenHeightScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0x363A + 0x2, &fCustomWideScreenHeightScaleDown);
	//CPatch::SetPointer(CHud__Draw + 0x3778 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3862 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x39AF + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3C22 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3C64 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3CEA + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3D81 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x41A4 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x41DA + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4311 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x43EA + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4417 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x46EB + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4837 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4867 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x4959 + 0x2, &fCustomWideScreenHeightScaleDown);


	if (fRadarWidthScale)
	{
		CPatch::SetPointer(CRadar__DrawRadarMask + 0xF8 + 0x2, &fCustomRadarWidthScale);
		CPatch::SetPointer(CRadar__DrawRadarMask + 0x13E + 0x2, &fCustomRadarWidthScale);
		CPatch::SetPointer(CRadar__DrawRadarMask + 0x23D + 0x2, &fCustomRadarWidthScale);
		CPatch::SetPointer(CRadar__DrawRadarMask + 0x282 + 0x2, &fCustomRadarWidthScale);
		CPatch::SetPointer(CRadar__DrawRadarSection + 0x382 + 0x2, &fCustomRadarWidthScale);
		CPatch::SetPointer(CRadar__DrawRadarSection + 0x3C1 + 0x2, &fCustomRadarWidthScale);


		CPatch::SetPointer(CRadar__DrawRotatingRadarSprite + 0x1A + 0x2, &fCustomRadarWidthScale);

		CPatch::SetPointer(CRadar__DrawBlips + 0xF4 + 0x2 + 0x6, &fPlayerMarkerPos);
		CPatch::SetPointer(CRadar__DrawBlips + 0x2E6 + 0x2 + 0x6, &fPlayerMarkerPos);
		//CPatch::SetPointer(CRadar__DrawBlips + 0x32F + 0x2 + 0x6, &fPlayerMarkerPos);

		CPatch::SetPointer(CRadar__DrawEntityBlip + 0x2BF + 0x2 + 0x6, &fPlayerMarkerPos);
		CPatch::SetPointer(CRadar__DrawEntityBlip + 0x307 + 0x2 + 0x6, &fPlayerMarkerPos);

		CPatch::SetPointer(CRadar__DrawBlips + 0x32F + 0x2, &fCustomRadarWidthScale);
		CPatch::SetPointer(CRadar__DrawCoordBlip + 0x2A7 + 0x2, &fCustomRadarWidthScale);

		CPatch::SetPointer(CHud__Draw + 0x3680 + 0x2, &fCustomRadarWidthScale);
		CPatch::SetPointer(CHud__Draw + 0x37B9 + 0x2, &fCustomRadarWidthScale);
	}
}