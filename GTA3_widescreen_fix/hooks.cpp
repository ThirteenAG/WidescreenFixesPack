#include "StdAfx.h"
#include "hooks.h"
#include "..\includes\injector\injector.hpp"
#include "..\includes\injector\hooking.hpp"
#include "..\includes\injector\calling.hpp"
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
float fCrosshairHeightScaleDown;
int nMenuAlignment;
int SelectedMultisamplingLevels;
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
int HideAABug, SmartCutsceneBorders, nMenuFix;
float gtaLogo128Coord1, gtaLogo128Coord2;
int IVRadarScaling, ReplaceTextShadowWithOutline;

float fCustomRadarWidthIV = 101.5f;
float fCustomRadarHeightIV = 78.0f;
float fCustomRadarPosXIV = 109.0f;
float fCustomRadarPosYIV = 107.0f;
float fCustomRadarRingWidthIV = 101.0f;
float fCustomRadarRingHeightIV = 84.0f;
float fCustomRadarRingPosXIV = 111.0f;
float fCustomRadarRingPosYIV = 109.5f;
float fCustomRadarRingPosXIV2 = fCustomRadarRingPosXIV - 13.0f;
void IVRadar();

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

	nMenuAlignment = static_cast<int>(((*VAR_pfScreenAspectRatio / (4.0f/3.0f)) * 320.0f) - 30);// (CLASS_pclRsGlobal->m_iScreenWidth / 2) * fWideScreenWidthScaleDown;
	gtaLogo128Coord2 = static_cast<float>(nMenuAlignment - 95);
	gtaLogo128Coord1 = gtaLogo128Coord2 + 190.0f;
	MenuFix();

	//Ini options
	fCustomWideScreenWidthScaleDown = fWideScreenWidthScaleDown * fHudWidthScale;
	fCustomWideScreenHeightScaleDown = fWideScreenHeightScaleDown * fHudHeightScale;
	fCustomRadarWidthScale = fWideScreenWidthScaleDown * fRadarWidthScale;
	fCustomRadarRingWidthScale = fCustomRadarWidthScale + 0.000019f;
	fCustomRadarRingHeightScale = fWideScreenHeightScaleDown + 0.000019f;

	//Proportional elements
	fCrosshairHeightScaleDown = fWideScreenHeightScaleDown * fHudWidthScale;

	//IV Radar
	if (!fCustomRadarPosXIV)
	{
		fCustomRadarPosXIV = 109.0f * ((float)CLASS_pclRsGlobal->m_iScreenWidth * (1.0f / 1920.0f));
		fCustomRadarRingPosXIV = 111.0f * ((float)CLASS_pclRsGlobal->m_iScreenWidth * (1.0f / 1920.0f));
		fCustomRadarRingPosXIV2 = 98.0f * ((float)CLASS_pclRsGlobal->m_iScreenWidth * (1.0f / 1920.0f));
	}

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

CRGBA rgba;
float originalPosX, originalPosY;
unsigned char originalColorR, originalColorG, originalColorB, originalColorA;
template<uintptr_t addr>
void TextDrawOutlineHookNOP()
{
	using printstr_hook = injector::function_hooker<addr, void(float, float, unsigned short*)>;
	injector::make_static_hook<printstr_hook>([](printstr_hook::func_type /*PrintString*/, float /*PosX*/, float /*PosY*/, unsigned short* /*c*/)
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
		PrintString = injector::cstd<void(float, float, unsigned short*)>::call<0x500F50>;
		originalPosX = PosX;
		originalPosY = PosY;
		originalColorR = *(unsigned char*)0x8F317C;
		originalColorG = *(unsigned char*)0x8F317D;
		originalColorB = *(unsigned char*)0x8F317E;
		originalColorA = *(unsigned char*)0x8F317F;

		injector::thiscall<void(CRGBA*, uint8_t, uint8_t, uint8_t, uint8_t)>::call<0x4F8C20>(&rgba, 0, 0, 0, originalColorA);
		injector::cstd<void(CRGBA*)>::call<0x501BD0>(&rgba);

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

		injector::thiscall<void(CRGBA*, uint8_t, uint8_t, uint8_t, uint8_t)>::call<0x4F8C20>(&rgba, originalColorR, originalColorG, originalColorB, originalColorA);
		injector::cstd<void(CRGBA*)>::call<0x501BD0>(&rgba);
		PrintString(originalPosX, originalPosY, c);

		return;
	});
}

template<uintptr_t addr>
void TextDrawOutlineHookMenu()
{
	using printstr_hook = injector::function_hooker<addr, void(float, float, unsigned short*)>;
	injector::make_static_hook<printstr_hook>([](printstr_hook::func_type PrintString, float PosX, float PosY, unsigned short* c)
	{
		PrintString = injector::cstd<void(float, float, unsigned short*)>::call<0x500F50>;
		originalPosX = PosX;
		originalPosY = PosY;
		originalColorR = *(unsigned char*)0x8F317C;
		originalColorG = *(unsigned char*)0x8F317D;
		originalColorB = *(unsigned char*)0x8F317E;
		originalColorA = *(unsigned char*)0x8F317F;

		if (originalColorR != 0 && originalColorG != 0 && originalColorB != 0)
		{
			injector::thiscall<void(CRGBA*, uint8_t, uint8_t, uint8_t, uint8_t)>::call<0x4F8C20>(&rgba, 0, 0, 0, originalColorA);
			injector::cstd<void(CRGBA*)>::call<0x501BD0>(&rgba);

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

			injector::thiscall<void(CRGBA*, uint8_t, uint8_t, uint8_t, uint8_t)>::call<0x4F8C20>(&rgba, originalColorR, originalColorG, originalColorB, originalColorA);
			injector::cstd<void(CRGBA*)>::call<0x501BD0>(&rgba);
			PrintString(originalPosX, originalPosY, c);
		}
		return;
	});
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

	

	/*menu
	if (nMenuFix)
	{
		CPatch::SetPointer(0x48AC00 + 0x2, &fWideScreenWidthScaleDown);
		CPatch::SetPointer(0x48B7E0 + 0x19B + 0x2, &fWideScreenWidthScaleDown);

		CPatch::SetPointer(0x48AC40 + 0x2, &fWideScreenHeightScaleDown);
	}
	*/


	
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
	if (nMenuFix) 
	{
		CPatch::SetInt(0x47B174, nMenuAlignment);
		CPatch::SetInt(0x47B1E0, nMenuAlignment);
		CPatch::SetInt(0x47B33C, nMenuAlignment);
		CPatch::SetInt(0x47B3A8, nMenuAlignment);
		CPatch::SetInt(0x47B411, nMenuAlignment);
	}
}


void __declspec(naked)RsSelectDeviceHook()
{
	MenuFix();
	CDraw__CalculateAspectRatio();
	ApplyINIchanges();
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

void IVRadar()
{
	fCustomRadarPosXIV = 109.0f * ((float)CLASS_pclRsGlobal->m_iScreenWidth * (1.0f / 1920.0f));
	fCustomRadarRingPosXIV = 111.0f * ((float)CLASS_pclRsGlobal->m_iScreenWidth * (1.0f / 1920.0f));
	fCustomRadarRingPosXIV2 = 98.0f * ((float)CLASS_pclRsGlobal->m_iScreenWidth * (1.0f / 1920.0f));

	CPatch::SetPointer(0x4A505B + 0x2, &fCustomRadarWidthIV); //CRadar__DrawRadarMask
	CPatch::SetPointer(0x4A5093 + 0x2, &fCustomRadarHeightIV); //CRadar__DrawRadarMask
	CPatch::SetPointer(0x4A5075 + 0x2, &fCustomRadarPosXIV);
	CPatch::SetPointer(0x4A50B0 + 0x2, &fCustomRadarPosYIV);

	CPatch::SetFloat(0x5FDC70, fCustomRadarRingWidthIV);
	CPatch::SetFloat(0x5FDC68, fCustomRadarRingHeightIV);

	CPatch::SetPointer(0x50845F + 0x2, &fCustomRadarRingPosXIV);
	CPatch::SetPointer(0x508481 + 0x2, &fCustomRadarRingPosXIV2);

	CPatch::SetPointer(0x508433 + 0x2, &fCustomRadarRingPosYIV);
}

void ApplyINIchanges()
{
	CIniReader iniReader("GTAIII_widescreen_fix.ini");
	fHudWidthScale = iniReader.ReadFloat("MAIN", "HudWidthScale", 0.62221788786f);
	fHudHeightScale = iniReader.ReadFloat("MAIN", "HudHeightScale", 0.66666670937f);
	fRadarWidthScale = iniReader.ReadFloat("MAIN", "RadarWidthScale", 0.80354591724f);
	fSubtitlesScale = iniReader.ReadFloat("MAIN", "SubtitlesScale", 1.0f);
	nMenuFix = iniReader.ReadInteger("MAIN", "MenuFix", 1);

	szForceAspectRatio = iniReader.ReadString("MAIN", "ForceMultisamplingLevel", "");
	if (strncmp(szForceAspectRatio, "max", 3) != 0)
	{
		SelectedMultisamplingLevels = iniReader.ReadInteger("MAIN", "ForceMultisamplingLevel", 0);
		CPatch::SetPointer(0x5B7D75 + 0x1, &SelectedMultisamplingLevels);
	}
	else
	{
		CPatch::SetPointer(0x5B7D75 + 0x1, (void*)0x619468);
	}

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

	IVRadarScaling = iniReader.ReadInteger("MAIN", "IVRadarScaling", 0);
	ReplaceTextShadowWithOutline = iniReader.ReadInteger("MAIN", "ReplaceTextShadowWithOutline", 0);

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


	CPatch::SetPointer(CHud__Draw + 0x2D0 + 0x2, &fCrosshairHeightScaleDown); // let's make crosshair proportional
	CPatch::SetPointer(CHud__Draw + 0x3AB + 0x2, &fCrosshairHeightScaleDown); // let's make crosshair proportional
	CPatch::SetPointer(CHud__Draw + 0x4BC + 0x2, &fCrosshairHeightScaleDown); // let's make crosshair proportional
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

	if (nMenuFix)
	{
		CPatch::SetPointer(0x48AC00 + 0x2, &fWideScreenWidthScaleDown);
		CPatch::SetPointer(0x48B7E0 + 0x19B + 0x2, &fWideScreenWidthScaleDown);

		CPatch::SetPointer(0x48AC40 + 0x2, &fWideScreenHeightScaleDown);

		CPatch::SetPointer(0x47AA5F + 0x2, &gtaLogo128Coord1);
		CPatch::SetPointer(0x47AA81 + 0x2, &gtaLogo128Coord2);
	}

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

	if (IVRadarScaling)
	{
		IVRadar();
	}

	if (ReplaceTextShadowWithOutline)
	{
		//SP compat
		injector::WriteMemory(0x500D27, 0xD8C3BF0F, true); injector::WriteMemory(0x500D27 + 4, 0xDA582444, true);
		injector::WriteMemory<WORD>(0x500D4C, 0x4489, true);
		injector::WriteMemory<WORD>(0x500D5F, 0x44DB, true);
		injector::WriteMemory<WORD>(0x500D6E, 0x2444, true);
		injector::WriteMemory(0x500D4E, 0xBF0F1824, true);
		injector::WriteMemory(0x500D70, 0x44DB501C, true);
		injector::WriteMemory(0x500D61, 0xBF0F1C24, true);
		injector::WriteMemory<DWORD>(0x500D53, 0x182444DA, true);
		injector::WriteMemory<BYTE>(0x500D52, 0xC3, true);
		injector::WriteMemory<BYTE>(0x500D65, 0xC3, true);
		injector::WriteMemory<BYTE>(0x500D6D, 0x89, true);
		injector::WriteMemory<WORD>(0x500D74, 0x2024, true);

		TextDrawOutlineHook<(0x420AAD)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x420BBC)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHookNOP<(0x420C29)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHookNOP<(0x420CC8)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x420E3E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x426446)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x4264CD)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x426584)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x42665F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4266EB)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x432723)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookMenu<(0x47AF76)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow) //menu
		TextDrawOutlineHook<(0x47B132)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookMenu<(0x47C666)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow) //menu
		TextDrawOutlineHook<(0x47C74C)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookMenu<(0x47F3E8)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow) //menu
		TextDrawOutlineHook<(0x47F9F1)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x47FAE4)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x47FE87)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x47FF24)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4808A8)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x481010)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHookNOP<(0x48113C)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		//TextDrawOutlineHook<(0x4813DB)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *)) CLASSIC CONTROL CONFIGURATION
		TextDrawOutlineHook<(0x4818BB)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4818FD)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48193F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x481D0A)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		//TextDrawOutlineHookNOP<(0x48206F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x4824E1)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x482527)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4825AD)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x482620)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4826D0)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x4827AE)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x483BE0)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483C04)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483C28)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483C4C)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483C70)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483C94)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483CB8)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483CDC)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483D00)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483D24)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483D48)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483D6C)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483D90)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483DB4)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483DD8)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483E01)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483E25)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483E49)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483E6D)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483E91)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483EB5)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483ED9)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483EFD)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483F21)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483F45)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483F69)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483F8D)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483FB1)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483FD5)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x483FF9)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484022)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484046)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48406A)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48408E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4840B2)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4840D6)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4840FA)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48411E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484142)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484166)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48418A)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4841AE)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4841D2)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4841F6)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48421A)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484243)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484267)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48428B)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4842AF)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4842D3)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4842F7)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48431B)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48433F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484363)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484387)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4843AB)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4843CF)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4843F3)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484417)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48447B)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48449F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4844C3)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4844E7)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48450B)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48452F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484553)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484577)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48459B)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4845BF)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4845E3)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484607)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48462B)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48464F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484673)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484693)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4846BC)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4846E0)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484704)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484728)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48474C)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484770)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484794)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4847B8)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4847DC)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484800)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484824)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484848)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48486C)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484890)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4848B4)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4848D4)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4848FD)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484921)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484945)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484969)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48498D)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4849B1)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4849D5)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4849F9)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484A1D)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484A41)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484A65)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484A89)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484AAD)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484AD1)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484AF5)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484B36)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484B5A)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484B7E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484BA2)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484BC6)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484BEA)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484C0E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484C32)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484C56)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484C7A)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484C9E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484CC2)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484CE6)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484D0A)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484D2E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484D4E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x484F3A)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4850E5)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4893EF)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48945F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x489522)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x489610)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4896EF)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		//TextDrawOutlineHook<(0x48A019)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		//TextDrawOutlineHook<(0x48A0D2)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		//TextDrawOutlineHook<(0x48A241)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		//TextDrawOutlineHook<(0x48A318)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48A3DD)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48A4EA)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48A5FD)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48A741)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48A879)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48B7C4)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48BA25)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48DC67)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x48DD30)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x494413)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x4FE6B2)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x505F82)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x506018)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		//TextDrawOutlineHook<(0x506388)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHookNOP<(0x5065DA)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHookNOP<(0x50669B)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *)) (shadow)
		TextDrawOutlineHook<(0x50670E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x5067C9)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x50694B)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHookNOP<(0x506A0C)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x506A7F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x506B34)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x506C37)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x506CE8)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x5070FA)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x50719F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x507598)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x50763D)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x507754)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x5077CA)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x507944)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x5079EA)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x507AC8)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x507B47)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x507CF0)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x507D63)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x507FF1)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x508070)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		//TextDrawOutlineHook<(0x508363)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *)) //pager
		TextDrawOutlineHook<(0x508698)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x508953)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *)) (subtitles)
		TextDrawOutlineHookNOP<(0x508C6E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x508D0E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x508F09)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x508FA7)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookMenu<(0x509544)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(text box) 
		TextDrawOutlineHook<(0x509717)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x5098D6)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x509953)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x509A65)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x509AE2)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x509DDE)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x509E51)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x50A142)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x50A1DC)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x51AF43)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x57E9F5)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x57EA80)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x584DF9)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHook<(0x584E47)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x585786)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x58581F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x5858E5)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x58597E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		TextDrawOutlineHookNOP<(0x585AB7)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
		TextDrawOutlineHook<(0x585B89)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
		//TextDrawOutlineHook<(0x592BD8)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *)) with backgr
		TextDrawOutlineHook<(0x59601A)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
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
	gtaLogo128Coord2 = static_cast<float>(nMenuAlignment - 95);
	gtaLogo128Coord1 = gtaLogo128Coord2 + 190.0f;

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


	/*
	if (nMenuFix)
	{
		CPatch::SetPointer(0x48AD10 + 0x2, &fWideScreenWidthScaleDown);
		CPatch::SetPointer(0x48B860 + 0x19B + 0x2, &fWideScreenWidthScaleDown);

		CPatch::SetPointer(0x48AD50 + 0x2, &fWideScreenHeightScaleDown);
	}
	*/



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
	if (nMenuFix) {
		CPatch::SetInt(0x47B244, nMenuAlignment);
		CPatch::SetInt(0x47B2B0, nMenuAlignment);
		CPatch::SetInt(0x47B40C, nMenuAlignment);
		CPatch::SetInt(0x47B478, nMenuAlignment);
		CPatch::SetInt(0x47B4E1, nMenuAlignment);
	}
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
	nMenuFix = iniReader.ReadInteger("MAIN", "MenuFix", 1);

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




	CPatch::SetPointer(0x505310 + 0x2D0 + 0x2, &fCrosshairHeightScaleDown); // let's make crosshair proportional
	CPatch::SetPointer(0x505310 + 0x3AB + 0x2, &fCrosshairHeightScaleDown); // let's make crosshair proportional
	CPatch::SetPointer(0x505310 + 0x4BC + 0x2, &fCrosshairHeightScaleDown); // let's make crosshair proportional
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

	if (nMenuFix)
	{
		CPatch::SetPointer(0x48AD10 + 0x2, &fWideScreenWidthScaleDown);
		CPatch::SetPointer(0x48B860 + 0x19B + 0x2, &fWideScreenWidthScaleDown);

		CPatch::SetPointer(0x48AD50 + 0x2, &fWideScreenHeightScaleDown);

		CPatch::SetPointer(0x47AA4F + 0x2, &gtaLogo128Coord1);
		CPatch::SetPointer(0x47AA71 + 0x2, &gtaLogo128Coord2);
	}

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