#include "stdafx.h"
#include "stdio.h"
#include <windows.h>
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"

HWND hWnd;
DWORD jmpAddress;
void asm_patch();
float _halfres_x;
float _halfres_y;

float _halfres_x1;
float _halfres_y2;

DWORD* _8F1CA0 = (DWORD*)0x8F1CA0;
int hud_patch;
int res_x;
int res_y;
DWORD WINAPI hud_handler(LPVOID);

void Init()
{
	CIniReader iniReader("nfsmwres.ini");
	res_x = iniReader.ReadInteger("MAIN", "X", 0);
	res_y = iniReader.ReadInteger("MAIN", "Y", 0);
	hud_patch = iniReader.ReadInteger("MAIN", "HUD_PATCH", 0);
	int shadows_patch = iniReader.ReadInteger("MAIN", "SHADOWS_RES", 1024);

	if (!res_x || !res_y) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		res_x = info.rcMonitor.right - info.rcMonitor.left;
		res_y = info.rcMonitor.bottom - info.rcMonitor.top;
	}


	//game

	CPatch::SetInt(0x6C27ED + 0x2, res_x);
	CPatch::SetInt(0x6C27F3 + 0x2, res_y);
	CPatch::SetInt(0x6C2804 + 0x2, res_x);
	CPatch::SetInt(0x6C280A + 0x2, res_y);
	CPatch::SetInt(0x6C281B + 0x2, res_x);
	CPatch::SetInt(0x6C2821 + 0x2, res_y);
	CPatch::SetInt(0x6C2832 + 0x2, res_x);
	CPatch::SetInt(0x6C2838 + 0x2, res_y);
	CPatch::SetInt(0x6C2849 + 0x2, res_x);
	CPatch::SetInt(0x6C284F + 0x2, res_y);
	CPatch::SetInt(0x6C2860 + 0x2, res_x);
	CPatch::SetInt(0x6C2866 + 0x2, res_y);
	CPatch::SetInt(0x6C28CD + 0x2, res_x);
	CPatch::SetInt(0x6C28D3 + 0x2, res_y);
	CPatch::SetInt(0x6C28E4 + 0x2, res_x);
	CPatch::SetInt(0x6C28EA + 0x2, res_y);
	CPatch::SetInt(0x6C28FB + 0x2, res_x);
	CPatch::SetInt(0x6C2901 + 0x2, res_y);
	CPatch::SetInt(0x6C2912 + 0x2, res_x);
	CPatch::SetInt(0x6C2918 + 0x2, res_y);
	CPatch::SetInt(0x6C2929 + 0x2, res_x);
	CPatch::SetInt(0x6C292F + 0x2, res_y);
	CPatch::SetInt(0x6C2940 + 0x2, res_x);
	CPatch::SetInt(0x6C2946 + 0x2, res_y);



		CPatch::SetInt(0x4F9ADC + 0x400000, res_x);
		CPatch::SetInt(0x53DACC + 0x400000, res_x);
		CPatch::SetInt(0x53DB04 + 0x400000, res_x);
		CPatch::SetInt(0x53E82C + 0x400000, res_x);
		CPatch::SetInt(0x582BE4 + 0x400000, res_x);
		//patch(0x8AF2C0,&res_x,4);

		CPatch::SetInt(0x4F9AE0 + 0x400000, res_y);
		CPatch::SetInt(0x53DAD0 + 0x400000, res_y);
		CPatch::SetInt(0x53DB08 + 0x400000, res_y);
		CPatch::SetInt(0x53E830 + 0x400000, res_y);
		CPatch::SetInt(0x582BE8 + 0x400000, res_y);
		//patch(0x8AE8F8,&res_y,4);


		if (shadows_patch) {
			
			CPatch::SetInt(0x6C86B0 + 0x1, shadows_patch);
			CPatch::SetInt(0x6C86C0 + 0x1, shadows_patch);
			CPatch::SetInt(0x6C8786 + 0x1, shadows_patch);
			CPatch::SetInt(0x6C878B + 0x1, shadows_patch);
			CPatch::SetInt(0x6C87B8 + 0x1, shadows_patch);
			CPatch::SetInt(0x6C87BD + 0x1, shadows_patch);


			CPatch::SetPointer(0x6C1499 + 0x2, _8F1CA0);
			CPatch::SetPointer(0x6C14DB + 0x2, _8F1CA0);
			CPatch::SetPointer(0x6C1510 + 0xC6 + 0x1, _8F1CA0);
			CPatch::SetPointer(0x6C1510 + 0x146 + 0x1, _8F1CA0);
			CPatch::SetPointer(0x6C1510 + 0x1C6 + 0x1, _8F1CA0);
			CPatch::SetPointer(0x6C16F8 + 0x1, _8F1CA0);
			CPatch::SetPointer(0x6C1990 + 0x101 + 0x2, _8F1CA0);
			CPatch::SetPointer(0x6C1B60 + 0x5 + 0x2, _8F1CA0);
			CPatch::SetPointer(0x6C5090 + 0x2, _8F1CA0);
			CPatch::SetPointer(0x6C50E0 + 0x2, _8F1CA0);
			CPatch::SetPointer(0x6C5E60 + 0xA2 + 0x2, _8F1CA0);
			CPatch::SetPointer(0x6C5E60 + 0xEC + 0x2, _8F1CA0);
			CPatch::SetPointer(0x6CFCE0 + 0xCF + 0x2, _8F1CA0);
			CPatch::SetPointer(0x6CFCE0 + 0x114 + 0x2, _8F1CA0);
			CPatch::SetPointer(0x6D2100 + 0x1, _8F1CA0);
			CPatch::SetPointer(0x6D5C10 + 0x24 + 0x1, _8F1CA0);
			CPatch::SetPointer(0x6D5DDC + 0x1, _8F1CA0);
			CPatch::SetPointer(0x6DF91B + 0x1, _8F1CA0);
			CPatch::SetPointer(0x6DF8F0 + 0xA9 + 0x2, _8F1CA0);
			CPatch::SetPointer(0x5009D0 + 0x1E6199 + 0x2, _8F1CA0);
		}

		if (hud_patch)
		{
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&hud_handler, NULL, 0, NULL);
		}

}

DWORD WINAPI hud_handler(LPVOID)
{
	float _halfres_x = 1.0f / (float)res_y * 2.0f;
	float _halfres_y = 1.0f / (float)res_x * 2.0f;

	float _halfres_x1 = 1.0f / (float)res_y;
	float _halfres_y2 = 1.0f / (float)res_x;

	float orig_halfres_x = 1.0f / 480.0f * 2.0f;
	float orig_halfres_y = 1.0f / 640.0f * 2.0f;

	float orig_halfres_x1 = 1.0f / 480.0f;
	float orig_halfres_y2 = 1.0f / 640.0f;
//float hud_multiplier_x = 1.0 / res_x * 2.0;
//float hud_multiplier_y = 1.0 / res_y * 2.0;
//float orig_multiplier_x = 1.0 / 640 * 2.0;
//float orig_multiplier_y = 1.0 / 480 * 2.0;
//HUD
while (true)
{
	Sleep(0);
	if ((char)*(DWORD*)0x909E6C == 0) {
		CPatch::SetFloat(0x8AF9A0, _halfres_x);
		CPatch::SetFloat(0x8AF9A4, _halfres_y);

		CPatch::SetFloat(0x8AFA04, _halfres_x1);
		CPatch::SetFloat(0x8AFA08, _halfres_y2);
	}
	else
	{
		CPatch::SetFloat(0x8AF9A0, orig_halfres_x);
		CPatch::SetFloat(0x8AF9A4, orig_halfres_y);

		CPatch::SetFloat(0x8AFA04, orig_halfres_x1);
		CPatch::SetFloat(0x8AFA08, orig_halfres_y2);
	}
}
	return 0;
}

void __declspec(naked)asm_patch()
{
	__asm mov jmpAddress, 0x400000
	__asm jmp jmpAddress
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}

