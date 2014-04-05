#include "stdafx.h"
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"
#include <iostream>

int* g_Width = (int *)0x6B0D88;
int* g_Height = (int *)0x6B0D8C;

int g_CameraAspectRatio_x = 0x5069DA;
int g_CameraAspectRatio_y = 0x5069E0;
int g_hud_stretch_x = 0x667CE4;

float hud_stretch_new = 0.0f;
float FOV_multiplier = 0.017453292f;

unsigned char menu_flag;
int g_FOV_ptr = 0x48259C + 0x2;
float ini_FOV_multiplier;

DWORD WINAPI Thread(LPVOID)
{
	CIniReader iniReader("flatout_widescreen_fix.ini");

	while (((unsigned char)*(DWORD*)0x66C554 == 255u || !(unsigned char)*(DWORD*)0x66C554 == NULL) 
		&& ((unsigned char)*(DWORD*)0x66D554 == 255u || !(unsigned char)*(DWORD*)0x66D554 == NULL)) //just a random address i found to check whether player is in menu
	{
		Sleep(0);
		if ((unsigned char)*(DWORD*)0x66C554 == 130u) {
			break;
		}
	}

	ini_FOV_multiplier = iniReader.ReadFloat("MAIN", "FOV_multiplier", 1.0f);
	menu_flag = (unsigned char)*(DWORD*)0x68B699;

	if (!(*(float *)g_CameraAspectRatio_x == 4.0f)) {  //steam demo exe
		g_Width = (int *)0x676BC8;
		g_Height = (int *)0x676BCC;

		g_CameraAspectRatio_x = 0x4D812A;
		g_CameraAspectRatio_y = 0x4D8130;
		g_hud_stretch_x = 0x635804;
		g_FOV_ptr = 0x473E5C + 0x2;

		menu_flag = (unsigned char)*(DWORD*)0x658679;
	}

	if (!(*(float *)g_CameraAspectRatio_x == 4.0f)) { //exe v1.0 (+steam)
		g_Width = (int *)0x6B8128;
		g_Height = (int *)0x6B812C;

		g_CameraAspectRatio_x = 0x50576A;
		g_CameraAspectRatio_y = 0x505770;
		g_hud_stretch_x = 0x666B5C;
		g_FOV_ptr = 0x48222C + 0x2;

		menu_flag = (unsigned char)*(DWORD*)0x68A699;
	}


	if (*(float *)g_CameraAspectRatio_x == 4.0f) {

		hud_stretch_new = 1.0f / (480.0f * (((float)*g_Width) / ((float)*g_Height)));

		CPatch::SetFloat(g_CameraAspectRatio_x, (float)*g_Width);
		CPatch::SetFloat(g_CameraAspectRatio_y, (float)*g_Height);

		CPatch::SetPointer(g_FOV_ptr, &FOV_multiplier);
		if (((float)*g_Width) / ((float)*g_Height) <= 1.9f) {
			FOV_multiplier = (FOV_multiplier * (0.63f * ((float)*g_Width) / ((float)*g_Height)) * ini_FOV_multiplier);
		}
		else {
			FOV_multiplier = (FOV_multiplier * (0.63f * (16.0f) / (9.0f)) * ini_FOV_multiplier);
		}


		while (true)
		{
			Sleep(0);
			if ((unsigned char)*(DWORD*)0x68A699 == menu_flag || (unsigned char)*(DWORD*)0x68B699 == menu_flag || ((unsigned char)*(DWORD*)0x658679 == menu_flag))
			{
				CPatch::SetFloat(g_hud_stretch_x, (1.0f / 640.0f));
			}
			else 
			{
				CPatch::SetFloat(g_hud_stretch_x, hud_stretch_new);
			}
		}

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
