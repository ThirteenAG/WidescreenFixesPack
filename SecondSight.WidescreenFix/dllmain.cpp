#include "..\includes\stdafx.h"
#include "..\includes\CPatch.h"

HWND hWnd;

int res_x;
int res_y;
float fAspectRatio;
float FovMultiplier;

DWORD jmpAddr = 0x40F2B8;
DWORD _EAX;
void __declspec(naked) asm_patch_x()
{
	_asm
	{		
		mov _EAX, eax
		mov eax, res_x
		mov dword ptr ds : 0x74B4F4, eax //scr_width
		mov eax, _EAX
		jmp jmpAddr
	}
}

DWORD jmpAddr2 = 0x40F2D2;
void __declspec(naked) asm_patch_y()
{
	_asm
	{
		mov _EAX, eax
		mov eax, res_y
		mov dword ptr ds : 0x74B4F8, eax //scr_width
		mov eax, _EAX
		jmp jmpAddr2
	}
}

void Init()
{
	CIniReader iniReader("");
	res_x = iniReader.ReadInteger("MAIN", "ResX", 0);
	res_y = iniReader.ReadInteger("MAIN", "ResY", 0);
	FovMultiplier = iniReader.ReadFloat("MAIN", "FOVMultiplier", 0.5f);
	if (!FovMultiplier) { FovMultiplier = 0.5f; }

	if (!res_x || !res_y) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		res_x = info.rcMonitor.right - info.rcMonitor.left;
		res_y = info.rcMonitor.bottom - info.rcMonitor.top;
	}

	CPatch::RedirectJump(0x40F2B3, asm_patch_x);
	CPatch::RedirectJump(0x40F2CD, asm_patch_y);

	//stretching
	fAspectRatio = 0.25f * (static_cast<float>(res_x) / static_cast<float>(res_y));
	CPatch::SetPointer(0x43D26B + 0x2, &fAspectRatio);

	//FOV
	CPatch::SetPointer(0x5E4B08 + 0x2, &FovMultiplier);

	//HUD and 2D elements
	CPatch::SetFloat(0x55BEB0, (static_cast<float>(res_x) / static_cast<float>(res_y)));
	CPatch::SetFloat(0x71C714, (static_cast<float>(res_x) / static_cast<float>(res_y)));
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}