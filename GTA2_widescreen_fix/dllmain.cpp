#include "stdafx.h"
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"

HWND hWnd;
DWORD jmpAddress;
void asm_patch();
DWORD res_x;
DWORD res_y;

DWORD WINAPI Thread(LPVOID)
{
	CIniReader iniReader("gta2_res.ini");
	res_x = iniReader.ReadInteger("MAIN", "X", 640);
	res_y = iniReader.ReadInteger("MAIN", "Y", 480);


	if (!res_x || !res_y) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		res_x = info.rcMonitor.right - info.rcMonitor.left;
		res_y = info.rcMonitor.bottom - info.rcMonitor.top;
	}


	/*CPatch::SetInt(0x4CB2E9, 1080);
	CPatch::SetInt(0x4CB316, 1080);

	CPatch::SetInt(0x4CB2FD, 1920);
	CPatch::SetInt(0x4CB32F, 1920);*/

	CPatch::SetPointer(0x4CB290 + 0xF + 0x2, &res_x); //     ebx, width
	CPatch::SetPointer(0x4CB290 + 0x10E + 0x2, &res_x); //     width, ebx
	CPatch::SetPointer(0x4CB570 + 0x5 + 0x2, &res_x); //     ecx, width
	CPatch::SetPointer(0x4CB570 + 0x26 + 0x2, &res_x); //     ecx, width
	CPatch::SetPointer(0x4CB570 + 0x66 + 0x2, &res_x); //     edx, width
	CPatch::SetPointer(0x4CB570 + 0x124 + 0x1, &res_x); //     eax, width
	CPatch::SetPointer(0x4BACF0 + 0x2D + 0x1, &res_x); //     eax, width1
	CPatch::SetPointer(0x4BACF0 + 0xBE + 0x1, &res_x); //     eax, width1
	CPatch::SetPointer(0x4BACF0 + 0x166 + 0x2, &res_x); //     edx, width1
	CPatch::SetPointer(0x4BAEF0 + 0x22 + 0x1, &res_x); //     eax, width1
	CPatch::SetPointer(0x4CAEC0 + 0x32 + 0x1, &res_x); //     eax, width1
	CPatch::SetPointer(0x4CB150 + 0x15 + 0x1, &res_x); //     eax, width1
	CPatch::SetPointer(0x4CB150 + 0x31 + 0x1, &res_x); //     eax, width1
	CPatch::SetPointer(0x4CB290 + 0x3 + 0x1, &res_x); //     eax, width1
	CPatch::SetPointer(0x4CB290 + 0x126 + 0x1, &res_x); //     eax, width1
	CPatch::SetPointer(0x4CB730 + 0xAD + 0x1, &res_x); //     eax, width1
	CPatch::SetPointer(0x4CB8D2 + 0x2, &res_x); //     ecx, width1
	CPatch::SetPointer(0x4CC5C0 + 0x51 + 0x2, &res_x); //     ecx, width1
	CPatch::SetPointer(0x4CC5C0 + 0x78 + 0x2, &res_x); //     ecx, width1
	CPatch::SetPointer(0x4D1170 + 0x1ED + 0x2, &res_x); //     edi, width1


	CPatch::SetPointer(0x4BACF0 + 0x56 + 0x2, &res_y); //     ecx, height 
	CPatch::SetPointer(0x4BACF0 + 0xE7 + 0x2, &res_y); //     ecx, height 
	CPatch::SetPointer(0x4BACF0 + 0x190 + 0x1, &res_y); //     eax, height 
	CPatch::SetPointer(0x4BAEF0 + 0x52 + 0x2, &res_y); //     edx, height 
	CPatch::SetPointer(0x4CAEC0 + 0x37 + 0x2, &res_y); //     ecx, height 
	CPatch::SetPointer(0x4CB150 + 0x24 + 0x2, &res_y); //     edx, height 
	CPatch::SetPointer(0x4CB290 + 0x8 + 0x2, &res_y); //     ecx, height 
	CPatch::SetPointer(0x4CB290 + 0x133 + 0x2, &res_y); //     ecx, height 
	CPatch::SetPointer(0x4CB730 + 0x8C + 0x2, &res_y); //     edx, height 
	CPatch::SetPointer(0x4CB8DE + 0x1, &res_y); //     eax, height 
	CPatch::SetPointer(0x4CC5C0 + 0x8E + 0x2, &res_y); //     edx, height 
	CPatch::SetPointer(0x4D1170 + 0x219 + 0x2, &res_y); //     ecx, height 

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
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Thread, NULL, 0, NULL);
	}
	return TRUE;
}

