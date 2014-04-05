#include "stdafx.h"
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"

void asm_patch();

int* g_Width = (int *)0x7242B8;
int* g_Height = (int *)0x7242BC;

int g_CameraAspectRatio_x = 0x724BB4;
int g_CameraAspectRatio_y = 0x724BB8;

float FOV_multiplier = 0.017453292f;

unsigned char menu_flag;

float ini_FOV_multiplier;


void Init()
{
	CIniReader iniReader("FlatoutUC_widescreen_fix.ini");

	ini_FOV_multiplier = iniReader.ReadFloat("MAIN", "FOV_multiplier", 1.0f);

	//CPatch::SetPointer(0x4D0F0C+0x2, &FOV_multiplier);

	if (((float)*g_Width) / ((float)*g_Height) <= 1.9f) {
		FOV_multiplier = (FOV_multiplier * (0.68796f * ((float)*g_Width) / ((float)*g_Height))) * ini_FOV_multiplier;
	}
	else {
		FOV_multiplier = (FOV_multiplier * (0.68796f * (16.0f / 9.0f)) * ini_FOV_multiplier);
	}

	//hud_stretch_new = 1.0/(480.0*(((float)*g_Width) / ((float)*g_Height)));

	CPatch::SetFloat(g_CameraAspectRatio_x, (float)*g_Width);
	CPatch::SetFloat(g_CameraAspectRatio_y, (float)*g_Height);

	//CPatch::SetFloat(g_hud_stretch_x, hud_stretch_new);
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}
