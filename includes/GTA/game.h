#pragma once
#define _USE_MATH_DEFINES
#include "math.h"
#pragma warning(disable:4480)
#ifndef _GAME_H_
#define _GAME_H_

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

#define RWFORMAT_A1R5G5B5			0x0100
#define RWFORMAT_R5G6B5				0x0200
#define RWFORMAT_A8R8G8B8			0x0500
#define RWFORMAT_X1R8G8B8			0x0600
#define RWFORMAT_X1R5G5B5			0x0A00

#endif // ndef _GAME_H_
