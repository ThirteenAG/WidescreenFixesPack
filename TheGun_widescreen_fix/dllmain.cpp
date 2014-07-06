#include "stdafx.h"
#include "stdio.h"
#include <windows.h>
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"
#include "tchar.h"
#include "Strsafe.h"


HWND hWnd;

int hud_patch;
int res_x;
int res_y;
HKEY hKey;
float FOV;
int GameSpeed;
float aspect_ratio;

double sub_4BA680()
{
	return aspect_ratio * ((*(float*)0x750FAC) / (4.0f / 3.0f));
}

void Init()
{
	CIniReader iniReader("gun_res.ini");
	res_x = iniReader.ReadInteger("MAIN", "X", 0);
	res_y = iniReader.ReadInteger("MAIN", "Y", 0);
	hud_patch = iniReader.ReadInteger("MAIN", "HUD_PATCH", 0);
	FOV = iniReader.ReadFloat("MAIN", "FOV", 114.59155f);
	GameSpeed = iniReader.ReadInteger("MAIN", "GameSpeed", 30);

	if (!res_x || !res_y) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		res_x = info.rcMonitor.right - info.rcMonitor.left;
		res_y = info.rcMonitor.bottom - info.rcMonitor.top;
	}

	DWORD dwDisp;
	RegCreateKeyExA(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Activision\\Gun\\Settings"), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &dwDisp);
	if (dwDisp == REG_CREATED_NEW_KEY || dwDisp == REG_OPENED_EXISTING_KEY)
	{
		char data[20];
		char res[20];

		sprintf_s(res, "%d", res_x);
		strcat_s(data, res);
		strcat_s(data, "x");
		sprintf_s(res, "%d", res_y);
		strcat_s(data, res);
		strcat_s(data, "\0");

		if (!RegSetValueEx(hKey, TEXT("Resolution"), NULL, REG_SZ, (LPBYTE)data, _tcslen(data) * sizeof(TCHAR)) == ERROR_SUCCESS)
			MessageBox(0, "Error setting the value of the registry key.", "GUN", 0);

		RegCloseKey(hKey);
	}
	else 
	{
		MessageBox(0, "Error creating the registry subkey.", "GUN", 0);
	}

		Sleep(1000); //steam

		//CPatch::Nop(0x4BA674, 5); //nop AR

		CPatch::SetInt(0x6B759C, res_x);
		CPatch::SetInt(0x6B75A0, res_y);
		CPatch::RedirectJump(0x4BA680, sub_4BA680);

		aspect_ratio = (float)res_x / (float)res_y;

		//CPatch::SetFloat(0x750FAC, aspect_ratio);
		//HUD
		float hud_multiplier_x = 1.0f / res_x * (res_y / 480.0f);
		//float hud_multiplier_y = 1.0f / res_y * (res_x / 640.0f); //1.0 / res_y;

		//float orig_multiplier_x = 1.0f / 640.0f;
		//float orig_multiplier_y = 1.0f / 480.0f;
		if (hud_patch)
		{
			CPatch::SetFloat(0x6814CC, hud_multiplier_x);
		}

		//FOV hack
		CPatch::SetPointer(0x498BA3 + 0x2, &FOV);

		//Game speed
		if (GameSpeed)
		{
			CPatch::SetInt(0x52E7CA, GameSpeed);
		}
		

}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
	}
	return TRUE;
}