#define _SCL_SECURE_NO_WARNINGS
#include "stdafx.h"
#include <windows.h>
#include "stdio.h"
#include "float.h"
#include "math.h"
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"
#define _USE_MATH_DEFINES
#include "math.h"

float height_multipl;
float height_stretch;
float width_stretch;

float aspect_ratio;
DWORD FOV;
float FOV_ptr;
float shadows_fix;
float hud_stretch;
float ini_fov;

int* g_Width = (int *)0x829584;
int* g_Height = (int *)0x829588;
float* g_some = (float *)0x7D3450;
float* g_fov = (float *)0x715C94;

#define width (float)*g_Width
#define height (float)*g_Height
#define some (float)*g_some
#define FOV (float)*g_fov



#define DEGREE_TO_RADIAN(fAngle) \
	((fAngle)* (float)M_PI / 180.0f)
#define RADIAN_TO_DEGREE(fAngle) \
	((fAngle)* 180.0f / (float)M_PI)
#define SCREEN_FOV_HORIZONTAL		0.7f
#define SCREEN_FOV_VERTICAL			(2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / (4.0f / 3.0f))))	// Default is ~55.0f.

void setFOV()
{
	if (ini_fov)
	{
		FOV_ptr = ini_fov;
		return;
	}

	FOV_ptr = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * aspect_ratio));
	return;
}



DWORD WINAPI Thread(LPVOID)
{
	Sleep(10);
	CIniReader iniReader("");
	ini_fov = iniReader.ReadFloat("MAIN", "FOV", 0.0f);

	aspect_ratio = width / height;
	if (aspect_ratio >= 1.4f) {

		CPatch::SetPointer(0x475BC0 + 0x89 + 0x2, &FOV_ptr);
		CPatch::SetPointer(0x475BC0 + 0x7C + 0x2, &FOV_ptr);

		height_multipl = 640.0f / aspect_ratio; // HUD PATCH
		hud_stretch = height / ((height / height_multipl) / (some) * width);
		CPatch::SetFloat(0x7D3458, hud_stretch);

		if (aspect_ratio < 2.0f)
		{
			height_stretch = 1.0f / height_multipl /*480.0*/ * 335.99999999f; //i dunno a good formula for this
		}
		else 
		{
			height_stretch = aspect_ratio * (0.458f / (640.0f / 480.0f));
		}

		CPatch::SetPointer(0x475BE2 + 0x2, &height_stretch);
		CPatch::SetPointer(0x475C09 + 0x2, &height_stretch);
		CPatch::SetPointer(0x476246 + 0x1, &height_stretch);

		//Disable menu aspect ratio switch
		CPatch::Nop(0x5DAA20, 9);
		CPatch::SetChar(0x5DAA30, 0xC3u); //ret

		//if ( *(float *)&height_stretch > (double)flt_715AA8 (1.9) )
		CPatch::SetChar(0x475BF3, 0xEBu);

		setFOV();
	}
	return 0;
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Thread, NULL, 0, NULL);
	}
	return TRUE;
}