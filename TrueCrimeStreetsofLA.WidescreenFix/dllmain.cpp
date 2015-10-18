#include "..\includes\stdafx.h"
#include "..\includes\CPatch.h"

HWND hWnd;
int res_x;
int res_y;
DWORD jmpAddr;
DWORD pResolutionWidth, pResolutionHeight;
DWORD _ESI, _EAX;

void __declspec(naked)patch_res()
{
	_asm
	{
			push    esi
			mov     esi, ecx
			mov     ecx, [esp + 8]
			mov		[esi + 55Ch], ecx
			mov     eax, [ecx]
			mov		[esi + 528h], eax //640
			mov		_ESI, esi
			mov     edx, [ecx + 4]
			mov		[esi + 52Ch], edx //480
			mov jmpAddr, 0x62BC0E
			jmp jmpAddr
	}
}

void __declspec(naked)patch_res_rld()
{
	_asm
	{
		push    esi
		mov     esi, [esp + 8]
		mov		[ecx + 55Ch], esi
		mov     eax, [esi]
		mov		[ecx + 528h], eax
		mov		_ESI, esi
		mov     edx, [esi + 4]
		mov		[ecx + 52Ch], edx
		mov     eax, [esi + 18h]
		mov     edx, [eax + 4]
		mov jmpAddr, 0x61D652
		jmp jmpAddr
	}
}

void __declspec(naked)patch_res2()
{
	_asm
	{
			mov _EAX, eax
			mov eax, res_x
			mov DWORD PTR DS : [ebx], eax
			add ebx, 4
			mov eax, res_y
			mov DWORD PTR DS : [ebx], eax
			sub ebx, 4
			mov eax, _EAX

			PUSH EBX
			PUSH EDX
			MOV EDX, DWORD PTR DS : [76B68Ch]

			mov jmpAddr, 0x62C203
			jmp jmpAddr
	}
}

void __declspec(naked)patch_res2_rld()
{
	_asm
	{
			mov _EAX, eax
			mov eax, res_x
			mov DWORD PTR DS : [ebx], eax
			add ebx, 4
			mov eax, res_y
			mov DWORD PTR DS : [ebx], eax
			sub ebx, 4
			mov eax, _EAX

			PUSH EBX
			PUSH EDX
			MOV EDX, DWORD PTR DS : [75119Ch]

			mov jmpAddr, 0x61DC23
			jmp jmpAddr
	}
}

void __declspec(naked)disp_crashfix()
{
	__asm ret 4
}

DWORD WINAPI Init(LPVOID)
{
	CIniReader iniReader("");
	res_x = iniReader.ReadInteger("MAIN", "X", 0);
	res_y = iniReader.ReadInteger("MAIN", "Y", 0);

	if (!res_x || !res_y) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		res_x = info.rcMonitor.right - info.rcMonitor.left;
		res_y = info.rcMonitor.bottom - info.rcMonitor.top;
	}

	if (*(DWORD *)0x6D151C == 32)
	{
		CPatch::RedirectJump(0x62BBF0, patch_res);
		CPatch::RedirectJump(0x62C1FB, patch_res2);
		CPatch::RedirectCall(0x580560 + 0xDB, disp_crashfix);

		while (!_ESI)
		{
			Sleep(0);
		}

		pResolutionWidth = _ESI + 0x528;
		pResolutionHeight = _ESI + 0x528 + 0x4;

		CPatch::Nop(0x5792C4, 6);
		CPatch::Nop(0x62FD14, 6);

		CPatch::Nop(0x5792CD, 6);
		CPatch::Nop(0x62FD58, 6);


		CPatch::SetUInt(0x6D1514, res_x);
		CPatch::SetUInt(0x6D1518, res_y);
		CPatch::SetUInt(pResolutionWidth, res_x);
		CPatch::SetUInt(pResolutionHeight, res_y);
	} else
		{
			CPatch::RedirectJump(0x61D630, patch_res_rld);
			CPatch::RedirectJump(0x61DC1B, patch_res2_rld);
			CPatch::RedirectCall(0x5781D0 + 0xDB, disp_crashfix);

			while (!_ESI)
			{
				Sleep(0);
			}

			pResolutionWidth = _ESI + 0x528;
			pResolutionHeight = _ESI + 0x528 + 0x4;


			CPatch::Nop(0x5714B4, 6);
			CPatch::Nop(0x62164B, 6);

			CPatch::Nop(0x5714BD, 6);
			CPatch::Nop(0x62168F, 6);


			CPatch::SetUInt(0x6B9B98, res_x);
			CPatch::SetUInt(0x6B9B9C, res_y);
			CPatch::SetUInt(pResolutionWidth, res_x);
			CPatch::SetUInt(pResolutionHeight, res_y);
		}
	return 0;
}





BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
	}
	return TRUE;
}

