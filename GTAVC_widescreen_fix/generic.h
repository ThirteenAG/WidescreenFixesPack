#include "stdafx.h"
#define _USE_MATH_DEFINES
#include "math.h"


#define DEGREE_TO_RADIAN(fAngle) \
	((fAngle) * (float)M_PI / 180.0f)
#define RADIAN_TO_DEGREE(fAngle) \
	((fAngle) * 180.0f / (float)M_PI)

#define SCREEN_RESOLUTION_MINWIDTH	640
#define SCREEN_RESOLUTION_MINHEIGHT	448

#define SCREEN_RESOLUTION_WIDTH		640.0f
#define SCREEN_RESOLUTION_HEIGHT	448.0f

#define SCREEN_SCALE_WIDTH			(1.0f / SCREEN_RESOLUTION_WIDTH)
#define SCREEN_SCALE_HEIGHT			(1.0f / SCREEN_RESOLUTION_HEIGHT)

#define SCREEN_AR_ACADEMY			(11.0f / 8.0f)	// 660.0f / 480.0f
#define SCREEN_AR_NARROW			( 4.0f / 3.0f)	// 640.0f / 480.0f
#define SCREEN_AR_STRETCH			(10.0f / 7.0f)	// 640.0f / 448.0f
#define SCREEN_AR_CUTSCENE			( 5.0f / 4.0f)	// 560.0f / 448.0f

#define SCREEN_FOV_HORIZONTAL		70.0f
#define SCREEN_FOV_VERTICAL			(2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / SCREEN_AR_NARROW)))	// Default is ~55.0f.

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