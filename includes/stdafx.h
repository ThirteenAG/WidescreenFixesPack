#pragma once
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#pragma warning(disable: 4178 4305 4309 4510 4996)
#include <windows.h>
#include <stdint.h>
#include "IniReader.h"
#include "injector\injector.hpp"
#include "injector\calling.hpp"
#include "injector\hooking.hpp"
#include "injector\assembly.hpp"
#include "injector\utility.hpp"
#include "Hooking.Patterns.h"
#pragma warning(default: 4178 4305 4309 4510 4996)

#define _USE_MATH_DEFINES
#include "math.h"
#define DEGREE_TO_RADIAN(fAngle) ((fAngle)* (float)M_PI / 180.0f)
#define RADIAN_TO_DEGREE(fAngle) ((fAngle)* 180.0f / (float)M_PI)
#define SCREEN_AR_NARROW (4.0f / 3.0f)
#define SCREEN_FOV_HORIZONTAL 90.0f
#define SCREEN_FOV_VERTICAL (2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / SCREEN_AR_NARROW)))