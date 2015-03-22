#include "StdAfx.h"
#include "hooks.h"
#include "..\includes\injector\injector.hpp"
#include "..\includes\injector\hooking.hpp"
#include "..\includes\injector\calling.hpp"
#include "..\includes\injector\assembly.hpp"
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
float fRadarWidthScale, fCustomRadarWidthScale, fSubtitlesScale;
float fCrosshairHeightScaleDown;
float fMVLScopeFix;
int RestoreCutsceneFOV;
float fPlayerMarkerPos;
float fCarSpeedDependantFOV;
int DontTouchFOV;
int NoLoadingBarFix;
float fRadarScaling;
char *szForceAspectRatio;
char *szFOVControl;
int FOVControl;
int AspectRatioWidth, AspectRatioHeight;
int HideAABug, SmartCutsceneBorders;
int IVRadarScaling, ReplaceTextShadowWithOutline;

float fCustomRadarWidthIV = 102.0f;
float fCustomRadarHeightIV = 79.0f;
float fCustomRadarPosXIV = 109.0f;
float fCustomRadarPosYIV = 107.0f;
float fCustomRadarRingWidthIV = 101.0f;
float fCustomRadarRingHeightIV = 83.0f;
float fCustomRadarRingPosXIV = 98.0f;
float fCustomRadarRingPosYIV = 109.5f;
float fCustomRadarRingPosXIV2 = fCustomRadarRingPosXIV - 19.0f;
void IVRadar();

long double __cdecl CDraw__CalculateAspectRatio()
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

	//logo scaling
	fvcLogoScale = (245.312497f * fWideScreenWidthScaleDown) / 1.7f;

	//Ini options
	fCustomWideScreenWidthScaleDown = fWideScreenWidthScaleDown * fHudWidthScale;
	fCustomWideScreenHeightScaleDown = fWideScreenHeightScaleDown * fHudHeightScale;
	fCustomRadarWidthScale = fWideScreenWidthScaleDown * fRadarWidthScale;

	//Proportional elements
	fCrosshairHeightScaleDown = fWideScreenHeightScaleDown * fHudWidthScale;

	return fDynamicScreenFieldOfViewScale;
}

auto FindPlayerVehicle = (int(__cdecl *)()) 0x4BC1E0;
auto FindPlayerSpeed = (int (__cdecl *)()) 0x4BC210;
inline float getDynamicScreenFieldOfView(float fFactor) 
{
	fFiretruckAmblncFix = 70.0f / fFactor;

		if ((!(char)*(DWORD*)0x7E46F5 == 0 && RestoreCutsceneFOV) || DontTouchFOV)
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
		mov edx, fRadarScaling
		mov ds:[0x974BEC], edx
	}
	fRadarScaling -= 180.0f;
	_asm
	{
		mov edx, 0x4C5D10
		jmp edx
	}
}

void __declspec(naked)FiretruckAmblncFix()
{
	__asm mov eax, fFiretruckAmblncFix
	__asm mov[ebx + 0xF4], eax
	__asm mov eax, 0x46DEBD
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
		cmp byte ptr ds:[00869652h], 00
		jnz label1
		ret
	label1:
		sub esp, 38h
		cmp byte ptr ds:[007E46F5h], 00
		mov edx, 0x46FC26
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
		mov		ds: [0097F860h], ax
		retn
	}
}

CRGBA rgba;
float originalPosX, originalPosY;
unsigned char originalColorR, originalColorG, originalColorB, originalColorA;
unsigned char FontDropColorR, FontDropColorG, FontDropColorB, FontDropColorA;

template<uintptr_t addr>
void TextDrawOutlineHookNOP()
{
	using printstr_hook = injector::function_hooker<addr, void(float a1, float a2, int a3, unsigned int a4, unsigned int a5, float a6)>;
	injector::make_static_hook<printstr_hook>([](printstr_hook::func_type, float, float, int, unsigned int, unsigned int, float)
	{
		//PrintString(PosX, PosY, c);
		return;
	});
}

template<uintptr_t addr>
void TextDrawOutlineHook()
{
	using printstr_hook = injector::function_hooker<addr, void(float, float, unsigned short*)>;
	injector::make_static_hook<printstr_hook>([](printstr_hook::func_type PrintString, float PosX, float PosY, unsigned short* c)
	{
		//PrintString = injector::cstd<void(float, float, unsigned short*)>::call<0x551040>;
		*(short*)0x97F860 = 0;
		originalPosX = PosX;
		originalPosY = PosY;
		originalColorR = *(unsigned char*)0x97F820;
		originalColorG = *(unsigned char*)0x97F821;
		originalColorB = *(unsigned char*)0x97F822;
		originalColorA = *(unsigned char*)0x97F823;
		/*FontDropColorR = *(unsigned char*)0x97F862;
		FontDropColorG = *(unsigned char*)0x97F863;
		FontDropColorB = *(unsigned char*)0x97F864;
		FontDropColorA = *(unsigned char*)0x97F865;*/

		if (originalColorR != 0 || originalColorG != 0 || originalColorB != 0)
		{
			injector::thiscall<void(CRGBA*, uint8_t, uint8_t, uint8_t, uint8_t)>::call<0x541570>(&rgba, 0, 0, 0, originalColorA);
			injector::cstd<void(CRGBA*)>::call<0x550170>(&rgba);
			//injector::thiscall<void(CRGBA*, uint8_t, uint8_t, uint8_t, uint8_t)>::call<0x541570>(&rgba, 0, 0, 0, originalColorA);
			//injector::cstd<void(CRGBA*)>::call<0x54FF30>(&rgba);

			PosX = originalPosX + 1.0f;
			PosY = originalPosY + 1.0f;
			PrintString(PosX, PosY, c);

			PosX = originalPosX + 1.0f;
			PosY = originalPosY - 1.0f;
			PrintString(PosX, PosY, c);

			PosX = originalPosX - 1.0f;
			PosY = originalPosY - 1.0f;
			PrintString(PosX, PosY, c);

			PosX = originalPosX - 1.0f;
			PosY = originalPosY + 1.0f;
			PrintString(PosX, PosY, c);

			if (ReplaceTextShadowWithOutline > 1)
			{
				PosX = originalPosX + 2.0f;
				PosY = originalPosY + 2.0f;
				PrintString(PosX, PosY, c);

				PosX = originalPosX + 2.0f;
				PosY = originalPosY - 2.0f;
				PrintString(PosX, PosY, c);

				PosX = originalPosX - 2.0f;
				PosY = originalPosY - 2.0f;
				PrintString(PosX, PosY, c);

				PosX = originalPosX - 2.0f;
				PosY = originalPosY + 2.0f;
				PrintString(PosX, PosY, c);
			}
			injector::thiscall<void(CRGBA*, uint8_t, uint8_t, uint8_t, uint8_t)>::call<0x541570>(&rgba, originalColorR, originalColorG, originalColorB, originalColorA);
			injector::cstd<void(CRGBA*)>::call<0x550170>(&rgba);
		}
		PrintString(originalPosX, originalPosY, c);
		return;
	});
}

template<uintptr_t addr>
void TextDrawOutlineHookColor()
{
	using printstr_hook = injector::function_hooker<addr, void(float, float, unsigned int, unsigned short *, unsigned short *, float)>;
	injector::make_static_hook<printstr_hook>([](printstr_hook::func_type PrintString, float PosX, float PosY, unsigned int c, unsigned short *d, unsigned short *e, float f)
	{
		//PrintString = injector::cstd<void(float, float, unsigned int, unsigned short *, unsigned short *, float)>::call<0x5516C0>;

		originalPosX = PosX - 1.0f;
		originalPosY = PosY - 1.0f;

		PosX = originalPosX + 1.0f;
		PosY = originalPosY + 1.0f;
		PrintString(PosX, PosY, c, d, e, f);

		PosX = originalPosX + 1.0f;
		PosY = originalPosY - 1.0f;
		PrintString(PosX, PosY, c, d, e, f);

		PosX = originalPosX - 1.0f;
		PosY = originalPosY - 1.0f;
		PrintString(PosX, PosY, c, d, e, f);

		PosX = originalPosX - 1.0f;
		PosY = originalPosY + 1.0f;
		PrintString(PosX, PosY, c, d, e, f);

		if (ReplaceTextShadowWithOutline > 1)
		{
			PosX = originalPosX + 2.0f;
			PosY = originalPosY + 2.0f;
			PrintString(PosX, PosY, c, d, e, f);

			PosX = originalPosX + 2.0f;
			PosY = originalPosY - 2.0f;
			PrintString(PosX, PosY, c, d, e, f);

			PosX = originalPosX - 2.0f;
			PosY = originalPosY - 2.0f;
			PrintString(PosX, PosY, c, d, e, f);

			PosX = originalPosX - 2.0f;
			PosY = originalPosY + 2.0f;
			PrintString(PosX, PosY, c, d, e, f);
		}
		return;
	});
}

#define	 	CDarkel__DrawMessages	 	0x429FE0
#define	 	CGarages__PrintMessages	 	0x42F2B0
#define	 	CMenuManager__PrintMap	 	0x49A5B7
#define	 	CMenuManager__DrawContollerScreenExtraText	 	0x49150B
#define	 	CRadar__DrawRadarMask	 	0x4C1A20
#define	 	CRadar__DrawRadarSection	 	0x4C1D60
#define	 	CRadar__DrawYouAreHereSprite	 	0x4C2980
#define	 	CRadar__DrawRadarSprite	 	0x4C2D00
#define	 	CRadar__DrawRotatingRadarSprite	 	0x4C2E70
#define	 	CRadar__ShowRadarTraceWithHeight	 	0x4C32F0
#define	 	CRadar__DrawBlips	 	0x4C4200
#define	 	CRadar__DrawLegend	 	0x4C4A10
#define	 	CRadar__DrawEntityBlip	 	0x4C5170
#define	 	CRadar__DrawCoordBlip	 	0x4C5680
#define	 	CClouds__Render	 	0x53F380
#define	 	CHud__DrawAfterFade	 	0x5566E0
#define	 	CHud__Draw	 	0x557320
#define	 	CMBlur__AddRenderFx	 	0x55D160
#define	 	CParticle__Render	 	0x5608C0
#define	 	CSpecialFX__Render2DFXs	 	0x573F20
#define	 	cMusicManager__DisplayRadioStationName	 	0x5F9EE0
#define	 	CSceneEdit__Draw	 	0x605DB0
#define	 	CRunningScript__ProcessCommands1100To1199	 	0x606730
#define	 	CReplay__Display	 	0x620BE0
#define	 	Render2DStuff 0x4A6190
#define	 	LoadingScreen 0x4A69D0
#define	 	debug_internal_something 0x491997
#define	 	CMenuManager_PrintStringMenu 0x4985DD

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




	CPatch::SetPointer(0x48F429 + 0x94 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(debug_internal_something + 0xFD + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x492D82 + 0x653 + 0x2, &fWideScreenWidthScaleDown);
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

	/*CPatch::SetPointer(0x49DF40 + 0x2C7 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x140C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x146A + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x14C4 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x1524 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x18CB + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x1929 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x1983 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x19E3 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x1AB4 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x1B12 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x1B6C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x1BCC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x1D0C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x24A8 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x29D1 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x2F0D + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x3449 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x39A9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x3ECF + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(0x4A212D + 0x648 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A212D + 0x699 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A212D + 0x744 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A212D + 0x794 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A212D + 0x83E + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A212D + 0x890 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A212D + 0x947 + 0x2, &fWideScreenWidthScaleDown); Menu Background and Text
	CPatch::SetPointer(0x4A212D + 0x999 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A212D + 0xBD0 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A212D + 0xC21 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A212D + 0xCCC + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A212D + 0xD1C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A212D + 0xDC9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A212D + 0xE1B + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A212D + 0xED8 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x4A212D + 0xF2A + 0x2, &fWideScreenWidthScaleDown);*/

	CPatch::SetPointer(0x49066B + 0x4A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49066B + 0x4CC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49066B + 0x4F6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49066B + 0x67E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49066B + 0x6A8 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49066B + 0xE33 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__DrawContollerScreenExtraText + 0x116 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__DrawContollerScreenExtraText + 0x1A1 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(CMenuManager__DrawContollerScreenExtraText + 0x44E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(debug_internal_something + 0xB68 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(debug_internal_something + 0xBBD + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(debug_internal_something + 0xE91 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(debug_internal_something + 0xFF0 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(debug_internal_something + 0x12AB + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492D82 + 0xC6B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492D82 + 0xCBC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492D82 + 0xF35 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492D82 + 0xFF6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492D82 + 0x11F7 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492D82 + 0x124D + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492D82 + 0x1332 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492D82 + 0x138E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492D82 + 0x1DAC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492D82 + 0x23B8 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492D82 + 0x24A0 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492D82 + 0x25C9 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492D82 + 0x2608 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x492D82 + 0x2796 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49598B + 0xA10 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49598B + 0xA6B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4965BA + 0x14A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4965BA + 0x193 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49A36D + 0x83 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49A36D + 0x211 + 0x2, &fWideScreenHeightScaleDown);
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
	CPatch::SetPointer(0x49B971 + 0x421 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49B971 + 0x481 + 0x2, &fWideScreenHeightScaleDown);
	/*CPatch::SetPointer(0x49DF40 + 0x8B0 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x92F + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x12FA + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x143E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x1496 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x14F4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x154C + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x18FD + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x1955 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x19B3 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x1A0B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x1AE6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x1B3E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x1B9C + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x1BF4 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x1D49 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x1E16 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x1E78 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49DF40 + 0x1EF2 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A212D + 0x674 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A212D + 0x6B9 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A212D + 0x76E + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A212D + 0x7B3 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A212D + 0x86A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A212D + 0x8B0 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A212D + 0x973 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A212D + 0x9B9 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A212D + 0xBFC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A212D + 0xC41 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A212D + 0xCF6 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A212D + 0xD3B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A212D + 0xDF5 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A212D + 0xE3B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A212D + 0xF04 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x4A212D + 0xF4A + 0x2, &fWideScreenHeightScaleDown);*/




	CPatch::SetPointer(Render2DStuff + 0x287 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(Render2DStuff + 0x2DB + 0x2, &fWideScreenWidthScaleDown);
	if (!NoLoadingBarFix)
	{
		CPatch::SetPointer(LoadingScreen + 0x118 + 0x2, &fWideScreenWidthScaleDown);
		CPatch::SetPointer(LoadingScreen + 0x1DD + 0x2, &fWideScreenWidthScaleDown);
		CPatch::SetPointer(LoadingScreen + 0x290 + 0x2, &fWideScreenWidthScaleDown);
		CPatch::SetPointer(LoadingScreen + 0x328 + 0x2, &fWideScreenWidthScaleDown);
	}

	CPatch::SetPointer(Render2DStuff + 0x11A + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(Render2DStuff + 0x164 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(Render2DStuff + 0x1D3 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(Render2DStuff + 0x21D + 0x2, &fWideScreenHeightScaleDown);
	if (!NoLoadingBarFix)
	{
		CPatch::SetPointer(LoadingScreen + 0x147 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(LoadingScreen + 0x206 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(LoadingScreen + 0x2B9 + 0x2, &fWideScreenHeightScaleDown);
		CPatch::SetPointer(LoadingScreen + 0x312 + 0x2, &fWideScreenHeightScaleDown);
	}
	


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

	CPatch::SetFloat(0x6CD9B8, 0.875f);
	CPatch::SetFloat(0x6CD9C0, 0.518f);
	CPatch::SetFloat(0x6CD9CC, 1.0f);

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
	


	CPatch::SetPointer(0x61DEB0 + 0xC9 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x61DEB0 + 0x158 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x61DEB0 + 0x202 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x61DEB0 + 0x28C + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x61DEB0 + 0x2C1 + 0x2, &fWideScreenWidthScaleDown);
	CPatch::SetPointer(0x61DEB0 + 0x33E + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(0x61DEB0 + 0xEC + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x61DEB0 + 0x17B + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x61DEB0 + 0x225 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x61DEB0 + 0x270 + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x61DEB0 + 0x328 + 0x2, &fWideScreenHeightScaleDown);


	CPatch::SetPointer(CReplay__Display + 0x66 + 0x2, &fWideScreenWidthScaleDown);

	CPatch::SetPointer(CReplay__Display + 0x50 + 0x2, &fWideScreenHeightScaleDown);


	CPatch::SetDouble(0x697AD8, (1.8 / 1.50)); //h
	CPatch::SetDouble(0x697AE0, (1.7 / 1.50)); //w
	CPatch::SetDouble(0x697AE8, (1.5 / 1.50)); //w
}

void MenuFix()
{
	CPatch::SetUInt(0x1 + 0x48F482, CLASS_pclRsGlobal->m_iScreenWidth); // Map help text scaling
	//CPatch::SetUInt(0x1 + 0x48F52A, CLASS_pclRsGlobal->m_iScreenWidth); // Map help text alignment
	CPatch::SetUInt(0x1 + 0x49E1E6, CLASS_pclRsGlobal->m_iScreenWidth); // Text at right top corner scaling
	CPatch::SetUInt(0x1 + 0x49E824, CLASS_pclRsGlobal->m_iScreenWidth); // Left aligned text scaling 
	//CPatch::SetUInt(0x400000+0x1+0x9FC3D, CLASS_pclRsGlobal->m_iScreenWidth); // Game menu text alignment
	CPatch::SetUInt(0x1 + 0x49FD6D, CLASS_pclRsGlobal->m_iScreenWidth); // Right aligned text scaling 

	CPatch::SetUInt(0x1 + 0x490712, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x490DA1, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4912D0, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x491354, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4913E4, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49154C, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4919CE, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x491A79, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x491B25, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x491B91, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x491C55, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x491CC1, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49222C, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4923B0, CLASS_pclRsGlobal->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x4925BA, CLASS_pclRsGlobal->m_iScreenWidth); //green selection line
	CPatch::SetUInt(0x1 + 0x492724, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49276E, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49294C, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4929A6, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x492C03, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x492CDE, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x492D98, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4933BA, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x493454, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x493506, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x493633, CLASS_pclRsGlobal->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x493697, CLASS_pclRsGlobal->m_iScreenWidth); //skins page
	CPatch::SetUInt(0x1 + 0x49372E, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49377B, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4937C5, CLASS_pclRsGlobal->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x493825, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49389E, CLASS_pclRsGlobal->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x493C64, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4948B0, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4948FD, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49494A, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x494997, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4949E1, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x494A7B, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x494AEF, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x494B4D, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4954D9, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4955B4, CLASS_pclRsGlobal->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x4956C2, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4973DE, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4973FD, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x497520, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49753F, CLASS_pclRsGlobal->m_iScreenWidth);
		//CPatch::SetUInt(0x1 + 0x498315, CLASS_pclRsGlobal->m_iScreenWidth);
		//CPatch::SetUInt(0x1 + 0x49834E, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49878B, CLASS_pclRsGlobal->m_iScreenWidth); CPatch::SetFloat(0x68D508, 0.42000002f * 2.0f); //Loading game text
	CPatch::SetUInt(0x1 + 0x4987E9, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49A3B1, CLASS_pclRsGlobal->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x49A40F, CLASS_pclRsGlobal->m_iScreenWidth); // BRIEF text
	CPatch::SetUInt(0x1 + 0x49A4E9, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49A535, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B40F, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B55B, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B5A6, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B60C, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B694, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B6DE, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B7C2, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49B833, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49BB9C, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49BBE6, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49C1CF, CLASS_pclRsGlobal->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x49C22D, CLASS_pclRsGlobal->m_iScreenWidth); //criminal rating (stats)
	//CPatch::SetUInt(0x1 + 0x49C2D5, CLASS_pclRsGlobal->m_iScreenWidth); //criminal rating (stats)
	//CPatch::SetUInt(0x1 + 0x49C380, CLASS_pclRsGlobal->m_iScreenWidth); //criminal rating(stats)
	//CPatch::SetUInt(0x1 + 0x49C418, CLASS_pclRsGlobal->m_iScreenWidth); //criminal rating (stats)

		/*CPatch::SetUInt(0x1 + 0x49C4F6, CLASS_pclRsGlobal->m_iScreenWidth); // draw radio logos slider in menu
		CPatch::SetUInt(0x1 + 0x49C547, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49C586, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49C5C5, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49C904, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49C951, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49C9EB, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CA38, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CAE8, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CB35, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CBD3, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CC20, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CCB3, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CD00, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CDBD, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CE0A, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CEA8, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CEF5, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CF93, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49CFE0, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D073, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D0C0, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D17D, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D1CA, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D260, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D2AD, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D35E, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D3AD, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D61F, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D670, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D6AF, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D6EE, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D79E, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D7EF, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D82E, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D86D, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D91A, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D96B, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D9AA, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49D9E9, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DA9C, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DAED, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DB2C, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DB6B, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DC18, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DC69, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DCA8, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DCE7, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DD9A, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DDEB, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DE2A, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x49DE69, CLASS_pclRsGlobal->m_iScreenWidth);*/

	CPatch::SetUInt(0x1 + 0x49E357, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49E432, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49E7A5, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49F1EF, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x49FD0B, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4A0F98, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4A14D4, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4A15ED, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4A168A, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4A16AF, CLASS_pclRsGlobal->m_iScreenWidth);
	CPatch::SetUInt(0x1 + 0x4A1A34, CLASS_pclRsGlobal->m_iScreenWidth);

	//CPatch::SetUInt(0x1 + 0x4A1B1F, CLASS_pclRsGlobal->m_iScreenWidth);
	//CPatch::SetUInt(0x1 + 0x4A1BBC, CLASS_pclRsGlobal->m_iScreenWidth); // mouse sensitivity
	//CPatch::SetUInt(0x1 + 0x4A1BE1, CLASS_pclRsGlobal->m_iScreenWidth);
		/*CPatch::SetUInt(0x1 + 0x4A1F56, CLASS_pclRsGlobal->m_iScreenWidth);
		CPatch::SetUInt(0x1 + 0x4A3A2F, CLASS_pclRsGlobal->m_iScreenWidth);*/

	float fMenuTextScale = 0.6f*(CLASS_pclRsGlobal->m_iScreenHeight / SCREEN_RESOLUTION_HEIGHT);
	float fMenuTextScale2 = 0.48f*(CLASS_pclRsGlobal->m_iScreenHeight / SCREEN_RESOLUTION_HEIGHT);
	CPatch::SetFloat(0x68D0C0, fMenuTextScale);
	CPatch::SetFloat(0x68D5C0, fMenuTextScale2);

	CPatch::SetPointer(0x495792 + 0xFA + 0x2, &fvcLogoScale);
	CPatch::SetPointer(0x4A212D + 0x126C + 0x2, &fvcLogoScale);
	CPatch::SetPointer(0x4A212D + 0x1353 + 0x2, &fvcLogoScale);


	CPatch::SetFloat(0x68D0B8, 0.5f); // controls page text scaling
	CPatch::SetFloat(0x68D100, 0.8f); // controls page text scaling

	CPatch::SetFloat(0x68D094, 2.0f); // menu header scaling
	CPatch::SetFloat(0x68D118, 1.8f); // controls page (in car, on foot text)

	CPatch::SetFloat(0x68C34C, 1.2f); //Map help text
	CPatch::SetFloat(0x68C344, 0.5f);
	CPatch::Nop(0x48F4A8, 5);

	CPatch::SetFloat(0x68D680, 0.86f); //Stats text width scale
	CPatch::SetUInt(0x49C2E0 + 0x2, 0x68D094);
}


void __declspec(naked)RsSelectDeviceHook()
{
	MenuFix();
	if (IVRadarScaling)
	{
		IVRadar();
	}
	_asm
	{
		add     esp, 10h
		pop     ebp
		pop     esi
		pop     ebx
		ret
	}

}

auto CSprite2d__DrawRect = (int(__cdecl *)(CRect const &, CRGBA const &)) 0x577B00;
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
	if (!*(BYTE*)0x869652)
	return 0;

	CRect		ScreenRect;

	ScreenRect.x1 = -1000.0f;
	ScreenRect.y1 = -1000.0f;
	ScreenRect.x2 = -1000.0f;
	ScreenRect.y2 = -1000.0f;

	GetScreenRect(ScreenRect);

	if (!*(DWORD*)0x7E4738 || *(float*)0x7E4738 == 2)
	*(DWORD*)0x7E474C = 80;

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

void IVRadar()
{
	fCustomRadarPosXIV = 109.0f * ((float)CLASS_pclRsGlobal->m_iScreenWidth * (1.0f / 1920.0f));
	fCustomRadarRingPosXIV = 98.0f * ((float)CLASS_pclRsGlobal->m_iScreenWidth * (1.0f / 1920.0f));
	fCustomRadarRingPosXIV2 = 116.0f * ((float)CLASS_pclRsGlobal->m_iScreenWidth * (1.0f / 1920.0f));

	fPlayerMarkerPos = fCustomRadarRingWidthIV * fRadarWidthScale;

	CPatch::SetFloat(0x68FD24, fCustomRadarWidthIV);
	CPatch::SetFloat(0x68FD30, fCustomRadarHeightIV);
	CPatch::SetFloat(0x68FD2C, fCustomRadarPosXIV); CPatch::SetPointer(0x4C2996 + 0x2, (void*)0x690238); //+draw you are here sprite fix
	CPatch::SetFloat(0x68FD34, fCustomRadarPosYIV); CPatch::SetPointer(0x4C29CB + 0x2, (void*)0x690238);

	CPatch::SetPointer(0x55A9A6 + 0x2, &fCustomRadarRingWidthIV);
	CPatch::SetPointer(0x55AADF + 0x2, &fCustomRadarRingWidthIV);
	CPatch::SetPointer(0x55A9BF + 0x2, &fCustomRadarRingHeightIV);
	CPatch::SetPointer(0x55AAF8 + 0x2, &fCustomRadarRingHeightIV);
	CPatch::SetFloat(0x55A956, fCustomRadarRingPosXIV);
	CPatch::SetPointer(0x55A9AC + 0x2, &fCustomRadarRingPosXIV2);
	CPatch::SetFloat(0x55AA94, fCustomRadarRingPosXIV);
	CPatch::SetPointer(0x55AAE5 + 0x2, &fCustomRadarRingPosXIV2);

	CPatch::SetFloat(0x697C18, fCustomRadarRingPosYIV);
}

void ApplyINIchanges()
{
	CIniReader iniReader("GTAVC_widescreen_fix.ini");
	fHudWidthScale = iniReader.ReadFloat("MAIN", "HudWidthScale", 0.62221788786f);
	fHudHeightScale = iniReader.ReadFloat("MAIN", "HudHeightScale", 0.66666670937f);
	fRadarWidthScale = iniReader.ReadFloat("MAIN", "RadarWidthScale", 0.80354591724f);
	fSubtitlesScale = iniReader.ReadFloat("MAIN", "SubtitlesScale", 1.0f);

	fMVLScopeFix = iniReader.ReadFloat("MAIN", "MVLScopeFix", 0.0f);

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

	szFOVControl = iniReader.ReadString("MAIN", "FOVControl", "0x40104A");
	sscanf_s(szFOVControl, "%X", &FOVControl);
	CPatch::SetFloat(FOVControl, 1.0f);

	HideAABug = iniReader.ReadInteger("MAIN", "HideAABug", 1);

	SmartCutsceneBorders = iniReader.ReadInteger("MAIN", "SmartCutsceneBorders", 1);

	NoLoadingBarFix = iniReader.ReadInteger("LCS", "NoLoadingBarFix", 0);

	if (iniReader.ReadInteger("MAIN", "FixVehicleLights", 1))
	{
		CPatch::SetUInt(0x00543B92, 0x0067E188); //lamp corona stretch
		CPatch::SetUInt(0x00543A52, 0x00696AA0); //car lights stretch
		CPatch::SetFloat(0x69590C, 3.0f); //car lights stretch
	}

	IVRadarScaling = iniReader.ReadInteger("MAIN", "IVRadarScaling", 0);
	ReplaceTextShadowWithOutline = iniReader.ReadInteger("MAIN", "ReplaceTextShadowWithOutline", 0);

	if (!fHudWidthScale || !fHudHeightScale) { fHudWidthScale = 0.62221788786f; fHudHeightScale = 0.66666670937f; }
	if (!fRadarWidthScale) { fRadarWidthScale = 0.80354591724f; }
	if (!fSubtitlesScale) { fSubtitlesScale = 1.0f; }

	if (SmallerTextShadows)
	{ 
		CPatch::RedirectJump(0x54FF20, SetDropShadowPosition); 

		CPatch::SetFloat(0x6874E0, 0.8f);
		CPatch::SetFloat(0x6874E8, 0.93f);
		CPatch::SetDouble(0x6874F0, 0.5f);
	}

	fPlayerMarkerPos = 94.0f * fRadarWidthScale;

	if (fSubtitlesScale)
	{
		CPatch::SetFloat(0x697A80, 1.2f * fSubtitlesScale);
		CPatch::SetFloat(0x697C28, 0.57999998f * fSubtitlesScale);
	}

	CPatch::SetPointer(CHud__DrawAfterFade + 0x31C + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x343 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x433 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x45A + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x716 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x740 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x870 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x89C + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x908 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x944 + 0x2, &fCustomWideScreenWidthScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0xBF3 + 0x2, &fCustomWideScreenWidthScaleDown);
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


	CPatch::SetPointer(CHud__DrawAfterFade + 0x306 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x3B7 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x41D + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x4CE + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x6F8 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x7B4 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x85B + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x887 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x8EA + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0x926 + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__DrawAfterFade + 0xBCD + 0x2, &fCustomWideScreenHeightScaleDown);
	CPatch::SetPointer(CHud__Draw + 0x3B6 + 0x2, &fCrosshairHeightScaleDown); // let's make crosshair proportional
	CPatch::SetPointer(CHud__Draw + 0x49A + 0x2, &fCrosshairHeightScaleDown); // let's make crosshair proportional
	CPatch::SetPointer(CHud__Draw + 0x5B1 + 0x2, &fCrosshairHeightScaleDown); // let's make crosshair proportional
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

		CPatch::SetPointer(CRadar__DrawEntityBlip + 0x307 + 0x2 + 0x6, &fPlayerMarkerPos);

		CPatch::SetPointer(CHud__Draw + 0x3680 + 0x2, &fCustomRadarWidthScale);
		CPatch::SetPointer(CHud__Draw + 0x37B9 + 0x2, &fCustomRadarWidthScale);
	}

	if (IVRadarScaling)
	{
		IVRadar();
	}

	if (HideAABug)
	{
		CPatch::RedirectJump(0x57FAA0, _1pxBugFix);
	}

	if (SmartCutsceneBorders)
	{
		CPatch::RedirectCall(0x4A61EE, CCamera__DrawBordersForWideScreen);
		CPatch::RedirectCall(0x54A223, CCamera__DrawBordersForWideScreen);
	}

	if (ReplaceTextShadowWithOutline)
	{
		#if(1)
		//replacing original shadows
		TextDrawOutlineHookColor<(0x551853)>();
		
		//subs
		injector::WriteMemory<char>(0x55AE92, 0x02, true); // shadow size
		injector::MakeNOP(0x557249, 5, true);

		//textbox
		//injector::WriteMemory(0x55B59B, 0xFFFF4B31, true); //background 
		injector::WriteMemory(0x55B5A0, 0xFFFF4AEC, true); //enable shadow
		injector::WriteMemory<char>(0x55B5A5, 0x01, true); //shadow size

		TextDrawOutlineHook<(0x42A225)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		//TextDrawOutlineHook<(0x42A35C)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)(shadow)
		TextDrawOutlineHook<(0x42A3C9)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		//TextDrawOutlineHook<(0x42A468)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)(shadow)
		TextDrawOutlineHook<(0x42A637)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		//TextDrawOutlineHook<(0x42F411)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)(shadow)
		TextDrawOutlineHook<(0x42F4E2)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x43E809)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x48F9AF)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x490E97)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x490EFE)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		//TextDrawOutlineHook<(0x4910EF)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)(shadow)
		TextDrawOutlineHook<(0x4911A6)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x4912A2)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x491642)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x4916CB)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x491789)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x49187C)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x49197C)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x491BFA)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x491D2A)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		//TextDrawOutlineHook<(0x49232A)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)(shadow)
		TextDrawOutlineHook<(0x492396)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x492844)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x492D51)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x4934BD)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x49356F)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x493700)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x493884)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x493CD4)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x493D9D)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x494ADA)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		//TextDrawOutlineHook<(0x495627)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)(shadow)
		TextDrawOutlineHook<(0x49572B)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x498933)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x49A59F)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x49B66C)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
	//	TextDrawOutlineHook<(0x49C0F3)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)(shadow)
	//	TextDrawOutlineHook<(0x49C135)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)(shadow)
		TextDrawOutlineHook<(0x49C28D)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
	//	TextDrawOutlineHook<(0x49C40B)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)(shadow)
		TextDrawOutlineHook<(0x49C488)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		//TextDrawOutlineHook<(0x49E30E)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *) menu title
		//TextDrawOutlineHook<(0x49E3D9)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x49E656)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x49FCAD)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x49FE4E)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x4A9885)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		//TextDrawOutlineHook<(0x4C2CB6)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)(shadow)
		TextDrawOutlineHook<(0x4C5156)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		//TextDrawOutlineHook<(0x5446F2)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)(shadow)
		TextDrawOutlineHook<(0x54474D)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x5516A8)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x5568E7)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x556AC8)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)(shadow)
		TextDrawOutlineHook<(0x556BD9)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)(shadow)
		TextDrawOutlineHook<(0x556EBF)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)(shadow)
		TextDrawOutlineHook<(0x5572E6)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x55830F)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x558925)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x558C10)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x558CF4)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x558E80)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x558F64)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x5591A6)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x55943B)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x5594CD)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x559FF1)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x55A1DD)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x55A2AB)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x55A48E)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x55A82B)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x55AD18)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		//TextDrawOutlineHook<(0x55B113)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *) //subs
		//TextDrawOutlineHook<(0x55B66B)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *) //textbox
		TextDrawOutlineHook<(0x55BA42)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x55BCB2)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x571213)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x574043)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		//TextDrawOutlineHook<(0x5FA1FD)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)(shadow)
		TextDrawOutlineHook<(0x5FA28A)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		//TextDrawOutlineHook<(0x605EF9)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)(shadow)
		TextDrawOutlineHook<(0x605F6E)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		//TextDrawOutlineHook<(0x606010)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)(shadow)
		TextDrawOutlineHook<(0x606084)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		//TextDrawOutlineHook<(0x606197)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)(shadow)
		TextDrawOutlineHook<(0x606246)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x61E1FD)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		TextDrawOutlineHook<(0x620D1A)>(); //0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *)
		#endif
	}
}









///////////////////////////////////////////steam//////////////////////////////////

long double __cdecl CDraw__CalculateAspectRatio_steam()
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

	//logo scaling
	fvcLogoScale = (245.312497f * fWideScreenWidthScaleDown) / 1.7f;

	//Ini options
	fCustomWideScreenWidthScaleDown = fWideScreenWidthScaleDown * fHudWidthScale;
	fCustomWideScreenHeightScaleDown = fWideScreenHeightScaleDown * fHudHeightScale;
	fCustomRadarWidthScale = fWideScreenWidthScaleDown * fRadarWidthScale;

	//Proportional elements
	fCrosshairHeightScaleDown = fWideScreenHeightScaleDown * fHudWidthScale;

	return fDynamicScreenFieldOfViewScale;
}

auto FindPlayerVehicle_steam = (int(__cdecl *)()) 0x4BC0B0;
auto FindPlayerSpeed_steam = (int(__cdecl *)()) 0x4BC0E0;
inline float getDynamicScreenFieldOfView_steam(float fFactor)
{
	fFiretruckAmblncFix = 70.0f / fFactor;

	if ((!(char)*(DWORD*)0x7E36FD == 0 && RestoreCutsceneFOV) || DontTouchFOV)
	{
		return fFactor * (*(float*)FOVControl);
	}

	if (fCarSpeedDependantFOV)
	{
		if (FindPlayerVehicle_steam())
		{
			fRadarScaling = *(float *)0x973BF4 - 180.0f;
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
	__asm mov[ebx + 0xF4], eax
	__asm mov eax, 0x46DD9D
	__asm jmp eax
}


void __declspec(naked)DrawBordersForWideScreen_patch_steam()
{
	_asm
	{
			cmp byte ptr ds : [86865Ah], 00
			jnz label1
			ret
		label1 :
			sub esp, 38h
			cmp byte ptr ds : [7E36FDh], 00
			mov edx, 0x46FB06
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
		mov		ds : [97E868h], ax
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
	CPatch::SetPointer(0x49B892 + 0x3BF + 0x2, &fWideScreenHeightScaleDown);
	CPatch::SetPointer(0x49B892 + 0x41F + 0x2, &fWideScreenHeightScaleDown);
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

	CPatch::SetFloat(0x6CC9B0, 0.875f);
	CPatch::SetFloat(0x6CC9B8, 0.518f);
	CPatch::SetFloat(0x6CC9C4, 1.0f);

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

void MenuFix_steam()
{
	CPatch::SetUIntWithCheck(0x1 + 0x48F392, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640); // Map help text scaling
	//CPatch::SetUIntWithCheck(0x1 + 0x48F43A, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640); // Map help text alignment
	CPatch::SetUIntWithCheck(0x1 + 0x49E0A5, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640); // Text at right top corner scaling
	CPatch::SetUIntWithCheck(0x1 + 0x49E6E3, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640); // Left aligned text scaling 
	//CPatch::SetUInt(0x400000+0x1+0x9FC3D, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640); // Game menu text alignment
	CPatch::SetUIntWithCheck(0x1 + 0x49FC2C, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640); // Right aligned text scaling 

	CPatch::SetUIntWithCheck(0x1 + 0x490622, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x490CB1, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4911E0, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x491264, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4912F4, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49145C, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4918DE, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x491989, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x491A35, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x491AA1, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x491B65, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x491BD1, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49213C, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4922C0, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	//CPatch::SetUIntWithCheck(0x1 + 0x4924CA, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640); //green selection line
	CPatch::SetUIntWithCheck(0x1 + 0x492634, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49267E, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49285C, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4928B6, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x492B13, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x492BEE, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x492CA8, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4932CA, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x493364, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x493416, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x493543, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	//CPatch::SetUIntWithCheck(0x1 + 0x4935A7, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640); //skins page
	CPatch::SetUIntWithCheck(0x1 + 0x49363E, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49368B, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4936D5, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	//CPatch::SetUIntWithCheck(0x1 + 0x493735, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4937AE, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	//CPatch::SetUIntWithCheck(0x1 + 0x493B74, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4947C0, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49480D, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49485A, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4948A7, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4948F1, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49498B, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4949FF, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x494A5D, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4953E9, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4954C4, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	//CPatch::SetUIntWithCheck(0x1 + 0x4955D2, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4972EE, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49730D, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x497430, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49744F, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	//CPatch::SetUIntWithCheck(0x1 + 0x498225, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	//CPatch::SetUIntWithCheck(0x1 + 0x49825E, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49869B, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640); CPatch::SetFloat(0x68C508, 0.42000002f * 2.0f); //Loading game text
	CPatch::SetUIntWithCheck(0x1 + 0x4986F9, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49A2D2, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	//CPatch::SetUIntWithCheck(0x1 + 0x49A330, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640); // BRIEF text
	CPatch::SetUIntWithCheck(0x1 + 0x49A40A, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49A456, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49B330, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49B47C, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49B4C7, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49B52D, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49B5B5, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49B5FF, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49B6E3, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49B754, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49BA6B, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49BAB5, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49C08E, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	//CPatch::SetUIntWithCheck(0x1 + 0x49C0EC, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640); //criminal rating (stats)
	//CPatch::SetUIntWithCheck(0x1 + 0x49C194, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640); //criminal rating (stats)
	//CPatch::SetUIntWithCheck(0x1 + 0x49C23F, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640); //criminal rating(stats)
	//CPatch::SetUIntWithCheck(0x1 + 0x49C2D7, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640); //criminal rating (stats)

	/*CPatch::SetUIntWithCheck(0x1 + 0x49C3B5, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640); // draw radio logos slider in menu
	CPatch::SetUIntWithCheck(0x1 + 0x49C406, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49C445, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49C484, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49C7C3, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49C810, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49C8AA, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49C8F7, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49C9A7, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49C9F4, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49CA92, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49CADF, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49CB72, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49CBBF, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49CC7C, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49CCC9, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49CD67, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49CDB4, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49CE52, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49CE9F, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49CF32, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49CF7F, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D03C, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D089, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D11F, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D16C, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D21D, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D26C, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D4DE, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D52F, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D56E, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D5AD, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D65D, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D6AE, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D6ED, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D72C, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D7D9, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D82A, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D869, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D8A8, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D95B, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D9AC, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49D9EB, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49DA2A, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49DAD7, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49DB28, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49DB67, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49DBA6, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49DC59, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49DCAA, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49DCE9, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49DD28, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);*/

	CPatch::SetUIntWithCheck(0x1 + 0x49E216, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49E2F1, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49E664, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49F0AE, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x49FBCA, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4A0E57, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4A1393, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4A14AC, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4A1549, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4A156E, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4A18F3, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);

	//CPatch::SetUIntWithCheck(0x1 + 0x4A19DE, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	//CPatch::SetUIntWithCheck(0x1 + 0x4A1A7B, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640); // mouse sensitivity
	//CPatch::SetUIntWithCheck(0x1 + 0x4A1AA0, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	/*CPatch::SetUIntWithCheck(0x1 + 0x4A1E15, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);
	CPatch::SetUIntWithCheck(0x1 + 0x4A38EE, CLASS_pclRsGlobal_steam->m_iScreenWidth, 640);*/

	float fMenuTextScale = 0.6f*(CLASS_pclRsGlobal_steam->m_iScreenHeight / SCREEN_RESOLUTION_HEIGHT);
	float fMenuTextScale2 = 0.48f*(CLASS_pclRsGlobal_steam->m_iScreenHeight / SCREEN_RESOLUTION_HEIGHT);
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

auto CSprite2d__DrawRect_steam = (int(__cdecl *)(CRect const &, CRGBA const &)) 0x5779F0;
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
	if (!*(BYTE*)0x86865A)
		return 0;

	CRect		ScreenRect;

	ScreenRect.x1 = -1000.0f;
	ScreenRect.y1 = -1000.0f;
	ScreenRect.x2 = -1000.0f;
	ScreenRect.y2 = -1000.0f;

	GetScreenRect_steam(ScreenRect);

	if (!*(DWORD*)0x7E3740 || *(float*)0x7E3740 == 2)
		*(DWORD*)0x7E3754 = 80;

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
	CIniReader iniReader("GTAVC_widescreen_fix.ini");
	fHudWidthScale = iniReader.ReadFloat("MAIN", "HudWidthScale", 0.62221788786f);
	fHudHeightScale = iniReader.ReadFloat("MAIN", "HudHeightScale", 0.66666670937f);
	fRadarWidthScale = iniReader.ReadFloat("MAIN", "RadarWidthScale", 0.80354591724f);
	fSubtitlesScale = iniReader.ReadFloat("MAIN", "SubtitlesScale", 1.0f);

	fMVLScopeFix = iniReader.ReadFloat("MAIN", "MVLScopeFix", 0.0f);

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
	
	szFOVControl = iniReader.ReadString("MAIN", "FOVControl", "0x40104A");
	sscanf_s(szFOVControl, "%X", &FOVControl);
	CPatch::SetFloat(FOVControl, 1.0f);

	HideAABug = iniReader.ReadInteger("MAIN", "HideAABug", 1);

	SmartCutsceneBorders = iniReader.ReadInteger("MAIN", "SmartCutsceneBorders", 1);

	if (iniReader.ReadInteger("MAIN", "FixVehicleLights", 1))
	{
		CPatch::SetUInt(0x543A82, 0x67D188); //lamp corona stretch
		CPatch::SetUInt(0x543942, 0x695AA8); //car lights stretch
		CPatch::SetFloat(0x694914, 3.0f); //car lights stretch
	}

	if (!fHudWidthScale || !fHudHeightScale) { fHudWidthScale = 0.62221788786f; fHudHeightScale = 0.66666670937f; }
	if (!fRadarWidthScale) { fRadarWidthScale = 0.80354591724f; }

	if (SmallerTextShadows)
	{
		CPatch::RedirectJump(0x54FE10, SetDropShadowPosition_steam);

		CPatch::SetFloat(0x6864E0, 0.8f);
		CPatch::SetFloat(0x6864E8, 0.93f);
		CPatch::SetDouble(0x6864F0, 0.5f);
	}

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


	CPatch::SetPointer(CHud__Draw + 0x3B6 + 0x2, &fCrosshairHeightScaleDown); // let's make crosshair proportional
	CPatch::SetPointer(CHud__Draw + 0x49A + 0x2, &fCrosshairHeightScaleDown); // let's make crosshair proportional
	CPatch::SetPointer(CHud__Draw + 0x5B1 + 0x2, &fCrosshairHeightScaleDown); // let's make crosshair proportional
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

		CPatch::SetPointer(CRadar__DrawBlips + 0xE4 + 0x2 + 0x6, &fPlayerMarkerPos);
		CPatch::SetPointer(CRadar__DrawBlips + 0x2D6 + 0x2 + 0x6, &fPlayerMarkerPos);
		//CPatch::SetPointer(CRadar__DrawBlips + 0x32F + 0x2 + 0x6, &fPlayerMarkerPos);

		CPatch::SetPointer(CRadar__DrawEntityBlip + 0x307 + 0x2 + 0x6, &fPlayerMarkerPos);

		CPatch::SetPointer(CRadar__DrawBlips + 0x31F + 0x2, &fCustomRadarWidthScale);
		CPatch::SetPointer(CRadar__DrawCoordBlip + 0x2A7 + 0x2, &fCustomRadarWidthScale);

		CPatch::SetPointer(CHud__Draw + 0x3680 + 0x2, &fCustomRadarWidthScale);
		CPatch::SetPointer(CHud__Draw + 0x37B9 + 0x2, &fCustomRadarWidthScale);
	}

	if (HideAABug)
	{
		CPatch::RedirectJump(0x57F8D0, _1pxBugFix_steam);
	}

	if (SmartCutsceneBorders)
	{
		CPatch::RedirectCall(0x4A60BE, CCamera__DrawBordersForWideScreen_steam);
		CPatch::RedirectCall(0x54A113, CCamera__DrawBordersForWideScreen_steam);
	}
}