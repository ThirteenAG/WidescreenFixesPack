#include "stdafx.h"
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"

HWND hWnd;
DWORD jmpAddress;
void patch_res();

void patch_res_x1(), patch_res_x2(), patch_res_x3(), patch_res_x4(), patch_res_x5(), patch_res_x6(), patch_res_x7(), patch_res_x8(), patch_res_x9(), patch_res_x10();
void patch_res_y1(), patch_res_y2(), patch_res_y3(), patch_res_y4(), patch_res_y5(), patch_res_y6(), patch_res_y7();
void patch_player_a(); void patch_player_a2();

int res_x;
int res_y;
int res_x43;

DWORD WINAPI Thread(LPVOID)
{
	CPatch::RedirectJump(0x42B512, patch_player_a);
	CPatch::RedirectJump(0x4274FF, patch_player_a2);

	CIniReader iniReader("gta1_res.ini");
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

	CPatch::RedirectJump(0x491E4C, patch_res);

	CPatch::RedirectJump(0x414FF7, patch_res_x1);
	CPatch::RedirectJump(0x43B7CF, patch_res_x2);

	res_x43 = static_cast<int>(res_y * (4.0f / 3.0f));
	CPatch::RedirectJump(0x46453B, patch_res_x3);
	//CPatch::RedirectJump(0x46452C, patch_res_x4);
	//CPatch::RedirectJump(0x486848, patch_res_x5);
	//CPatch::RedirectJump(0x486852, patch_res_x6);
	/*CPatch::RedirectJump(0x48C137, patch_res_x7);
	CPatch::RedirectJump(0x48C276, patch_res_x8);
	CPatch::RedirectJump(0x48C159, patch_res_x9);*/
	//CPatch::RedirectJump(0x49168B, patch_res_x10);

	CPatch::RedirectJump(0x415008, patch_res_y1);
	CPatch::RedirectJump(0x43B7D8, patch_res_y2);
	CPatch::RedirectJump(0x464532, patch_res_y3);
	CPatch::RedirectJump(0x48683A, patch_res_y4);
	CPatch::RedirectJump(0x48C13D, patch_res_y5);
	CPatch::RedirectJump(0x48C2B0, patch_res_y6);
	//CPatch::RedirectJump(0x, patch_res_y7);

	//menu 
	CPatch::SetUInt(0x4898C3 + 0x1, res_x);
	return 0;
}

void __declspec(naked)patch_res()
{
	_asm
	{
		mov eax, res_x
		MOV DWORD PTR DS : [0x787310], EAX
		MOV DWORD PTR DS : [0x787370], EAX
		MOV EAX, DWORD PTR DS : [EBX + 1B4h]
		INC EAX
		TEST ECX, ECX
		mov eax, res_y
		MOV DWORD PTR DS : [0x787314], EAX
		MOV DWORD PTR DS : [0x787388], EAX

		mov jmpAddress, 0x491E69
		jmp jmpAddress
	}
}

void __declspec(naked)patch_res_x1()
{
	_asm
	{
		mov eax, res_x
		MOV DWORD PTR DS : [0x504CC0], EAX
		mov jmpAddress, 0x414FFC
		jmp jmpAddress
	}
}

void __declspec(naked)patch_res_x2()
{
	_asm
	{
		mov edx, res_x
		MOV DWORD PTR DS : [0x5C0C00], EDX
		mov jmpAddress, 0x43B7D5
		jmp jmpAddress
	}
}

void __declspec(naked)patch_res_x3()
{
	_asm
	{
		mov esi, res_x43
		MOV DWORD PTR DS : [ECX + 0x74F168], ESI
		mov jmpAddress, 0x464541
		jmp jmpAddress
	}
}

void __declspec(naked)patch_res_x4()
{
	_asm
	{
		mov edx, res_x
		MOV DWORD PTR DS : [ECX + 0x74F170], EDX

		mov jmpAddress, 0x464532
		jmp jmpAddress
	}
}

void __declspec(naked)patch_res_x5()
{
	_asm
	{
		mov ecx, res_x
		MOV DWORD PTR DS : [0x785170], ECX
		mov jmpAddress, 0x48684E
		jmp jmpAddress
	}
}

void __declspec(naked)patch_res_x6()
{
	_asm
	{
		mov ecx, res_x
		MOV DWORD PTR DS : [0x785178], ECX
		mov jmpAddress, 0x486858
		jmp jmpAddress
	}
}

void __declspec(naked)patch_res_x7()
{
	_asm
	{
		mov esi, res_x
		MOV DWORD PTR DS : [0x787AD4], ESI
		mov jmpAddress, 0x48C13D
		jmp jmpAddress
	}
}

void __declspec(naked)patch_res_x8()
{
	_asm
	{
		mov eax, res_x
		MOV DWORD PTR DS : [0x787AEC], EAX
		mov jmpAddress, 0x48C27B
		jmp jmpAddress
	}
}

void __declspec(naked)patch_res_x9()
{
	_asm
	{
		mov eax, res_x
		MOV DWORD PTR DS : [0x787CB0], EAX
		mov jmpAddress, 0x48C15E
		jmp jmpAddress
	}
}

void __declspec(naked)patch_res_x10()
{
	_asm
	{
		mov eax, res_x
		MOV DWORD PTR DS : [EDI + 16Ch], EAX
		mov jmpAddress, 0x491691
		jmp jmpAddress
	}
}





void __declspec(naked)patch_res_y1()
{
	_asm
	{
		mov eax, res_y
		MOV DWORD PTR DS : [0x504CC4], EAX
		mov jmpAddress, 0x41500D
		jmp jmpAddress
	}
}

void __declspec(naked)patch_res_y2()
{
	_asm
	{
		mov eax, res_y
		MOV DWORD PTR DS : [0x5BFAB0], EAX
		mov jmpAddress, 0x43B7DD
		jmp jmpAddress
	}
}

void __declspec(naked)patch_res_y3()
{
	_asm
	{
		mov edi, res_y
		MOV DWORD PTR DS : [ECX + 0x74F16C], EDI
		mov jmpAddress, 0x464538
		jmp jmpAddress
	}
}

void __declspec(naked)patch_res_y4()
{
	_asm
	{
		mov edx, res_y
		MOV DWORD PTR DS : [0x785174], EDX
		mov jmpAddress, 0x486840
		jmp jmpAddress
	}
}

void __declspec(naked)patch_res_y5()
{
	_asm
	{
		mov edi, res_y
		MOV DWORD PTR DS : [0x787AD8], EDI
		mov jmpAddress, 0x48C143
		jmp jmpAddress
	}
}

void __declspec(naked)patch_res_y6()
{
	_asm
	{
		mov eax, res_y
		MOV DWORD PTR DS : [0x787AF0], EAX
		mov jmpAddress, 0x48C2B5
		jmp jmpAddress
	}
}

void __declspec(naked)patch_res_y7()
{
	_asm
	{
		mov edx, res_y
		MOV DWORD PTR DS : [0x4B48C0], EDX
		mov jmpAddress, 0x48AE8B
		jmp jmpAddress
	}
}

void __declspec(naked)patch_player_a()
{
	_asm
	{
		MOV BYTE PTR DS : [0x51029C], 0x06
		mov jmpAddress, 0x42B518
		jmp jmpAddress
	}
}

void __declspec(naked)patch_player_a2()
{
	_asm
	{
		MOV BYTE PTR DS : [0x51029C], 0x06
		mov jmpAddress, 0x427504
		jmp jmpAddress
	}
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Thread, NULL, 0, NULL);
	}
	return TRUE;
}

