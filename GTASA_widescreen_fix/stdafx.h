#ifndef _PRECOMPILED_H_
#define _PRECOMPILED_H_

#define SDDWG_TOGGLE_TRACING		0

#define STRINGIZE_BOOL(bVar)		bVar ? "true" : "false"
#define STRINGIZE_NULL(szVar)		!szVar[0] ? "" : szVar
#define STRINGIZE_PATTERN(sPattern)	#sPattern

#define GET_PTR_AT_OFFSET(dwPointer, dwOffset) \
	((PBYTE)dwPointer + dwOffset)

#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#define _USE_MATH_DEFINES

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <d3d9.h>
#include "game.h"

extern "C" {
	long _ftol(double dValue);

	inline long _ftol2(double dValue) {
		return _ftol(dValue);
	};
}

#endif // ndef _PRECOMPILED_H_
