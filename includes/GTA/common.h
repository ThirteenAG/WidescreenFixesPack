#include <iostream>
#include <string>
#include "..\IniReader.h"
#include "..\injector\injector.hpp"
#include "..\injector\calling.hpp"
#include "..\injector\hooking.hpp"
#include "..\injector\assembly.hpp"

#include "..\GTA\RenderWare.h"
#include "..\GTA\game.h"
#include "..\GTA\CDraw.h"
#include "..\GTA\CCamera.h"
#include "..\GTA\CRGBA.h"
#include "..\GTA\CRect.h"

HWND hWnd;
int ResX, ResY;

RsGlobalType* RsGlobal;
float fCustomAspectRatioHor, fCustomAspectRatioVer;
float fEmergencyVehiclesFix;
float fvcLogoScale;
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

float fHudWidthScale, fHudHeightScale;
float fCustomWideScreenWidthScaleDown;
float fCustomWideScreenHeightScaleDown;
float fRadarWidthScale, fCustomRadarWidthScale, fCustomRadarRingWidthScale, fCustomRadarRingHeightScale, fSubtitlesScale;
float fCrosshairHeightScaleDown;
float fMVLScopeFix;
int SelectedMultisamplingLevels;
int RestoreCutsceneFOV;
float fPlayerMarkerPos;
float fCarSpeedDependantFOV;
int DontTouchFOV;
int NoLoadingBarFix;
float fRadarScaling;
char *szForceAspectRatio;
char *szFOVControl;
int FOVControl;
float fFOVControlValue;
int AspectRatioWidth, AspectRatioHeight;
int HideAABug, SmartCutsceneBorders, nMenuFix, nMenuAlignment;
float gtaLogo128Coord1, gtaLogo128Coord2;
int IVRadarScaling, ReplaceTextShadowWithOutline;
int SmallerTextShadows;
float fTextShadowsSize;
float fCrosshairPosFactor;

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

DWORD bWideScreen, BordersVar1, BordersVar2, IsInCutscene;
int(__cdecl* CSprite2dDrawRect)(class CRect const &, class CRGBA const &);
int(__cdecl* FindPlayerVehicle)();