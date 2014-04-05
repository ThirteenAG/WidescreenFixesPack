#define _USE_MATH_DEFINES
#include <math.h>
#include "stdafx.h"
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"

#define DEGREE_TO_RADIAN(fAngle) \
	((fAngle)* (float)M_PI / 180.0f)
#define RADIAN_TO_DEGREE(fAngle) \
	((fAngle)* 180.0f / (float)M_PI)


int* g_Width = (int *)0x6D68E8;
int* g_Height = (int *)0x6D68EC;

int g_CameraAspectRatio_x = 0x69BE18;
int g_CameraAspectRatio_y = 0x69BE1C;
int g_hud_stretch_x = 0x67DBE4;

float hud_stretch_new = 0.0f;
float hud_stretch_default = 1.0f / 640.0f;
float FOV_multiplier = 0.017453292f;
float SCREEN_FOV_HORIZONTAL;

unsigned char menu_flag;

float ini_FOV_multiplier;

void __declspec(naked)asm_patch()
{
	__asm ret 0x14
}

void Init()
{
	CIniReader iniReader("flatout2_widescreen_fix.ini");

	CPatch::RedirectCall(0x4C0AE8, asm_patch); //derby triangles
	CPatch::RedirectCall(0x4BB548, asm_patch); //black backround stripes
	CPatch::SetInt(0x4BB500, 0x560010C2);

	ini_FOV_multiplier = iniReader.ReadFloat("MAIN", "FOV_multiplier", 1.0f);

	SCREEN_FOV_HORIZONTAL = (2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(60.0f * 0.5f)) * ((float)*g_Width) / ((float)*g_Height))));
	CPatch::SetFloat(0x400040, SCREEN_FOV_HORIZONTAL);

	CPatch::SetPointer(0x4D0F0C + 0x2, &FOV_multiplier);

	FOV_multiplier = ((float)*g_Width) / ((float)*g_Height) / SCREEN_FOV_HORIZONTAL;
	//FOV_multiplier = FOV_multiplier * ini_FOV_multiplier;

	hud_stretch_new = 1.0f / (480.0f * (((float)*g_Width) / ((float)*g_Height)));

	CPatch::SetFloat(g_CameraAspectRatio_x, (float)*g_Width);
	CPatch::SetFloat(g_CameraAspectRatio_y, (float)*g_Height);

	CPatch::SetFloat(g_hud_stretch_x, hud_stretch_new);


	if (((float)*g_Width) / ((float)*g_Height) <= 1.9f) {
		CPatch::SetPointer(0x495100 + 0x66 + 0x2, &hud_stretch_default); //    flt_67DBE4 - надписи при паузе снизу (продолжить гонку, рестарт, звук. выход)
		CPatch::SetPointer(0x495CB0 + 0x17 + 0x2, &hud_stretch_default); //    flt_67DBE4 - затемнённый бокс перед стартом (общий затемнённый экран остаётся)
		CPatch::SetPointer(0x495D90 + 0x16 + 0x2, &hud_stretch_default); //    flt_67DBE4 - вертикальный разделитель полос на паузе
		CPatch::SetPointer(0x496880 + 0xE + 0x2, &hud_stretch_default); //    flt_67DBE4 - надпись "начать гонку"
		CPatch::SetPointer(0x496930 + 0x17 + 0x2, &hud_stretch_default); //    flt_67DBE4 - отключение делает затемнение при паузе и перед началом гонки на весь экран
		CPatch::SetPointer(0x496ED0 + 0xB9 + 0x2, &hud_stretch_default); //    flt_67DBE4 - при паузе на иконке "продолжить гонку" надписи по центру и сверху
		CPatch::SetPointer(0x4A6590 + 0x311 + 0x2, &hud_stretch_default); //    flt_67DBE4 - главное меню
		CPatch::SetPointer(0x4A9990 + 0xD2 + 0x2, &hud_stretch_default); //    flt_67DBE4 - черный прямоугольник при загрузках в меню
	}
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}
