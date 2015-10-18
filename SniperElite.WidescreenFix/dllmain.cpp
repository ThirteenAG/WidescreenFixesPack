#include "..\includes\stdafx.h"
#include "..\includes\CPatch.h"

#define _USE_MATH_DEFINES
#include "math.h"
HWND hWnd;

int res_x;
int res_y;
int res_x43;
float fScale, FOV;
float fres_x, fres_y;
float fDynamicScreenFieldOfViewScale;
float fAspectRatio;

#define DEGREE_TO_RADIAN(fAngle) \
	((fAngle)* (float)M_PI / 180.0f)
#define RADIAN_TO_DEGREE(fAngle) \
	((fAngle)* 180.0f / (float)M_PI)
#define SCREEN_FOV_HORIZONTAL		75.0f
#define SCREEN_FOV_VERTICAL			(2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / (4.0f / 3.0f))))

DWORD jmpAddr;
void _declspec(naked) asm_446B08()
{
	_asm
	{
		MOV ECX, res_x
		MOV EDX, res_y
		mov jmpAddr, 0x446B10
		jmp jmpAddr
	}
}

void _declspec(naked) asm_487D6D()
{
	_asm
	{
		MOV EAX, res_x
		MOV EDX, res_y
		mov jmpAddr, 0x487D75
		jmp jmpAddr
	}
}



//void Init()
DWORD WINAPI Init()
{
	CIniReader iniReader("");
	res_x = iniReader.ReadInteger("MAIN", "ResX", 0);
	res_y = iniReader.ReadInteger("MAIN", "ResY", 0);
	FOV = iniReader.ReadFloat("MAIN", "FixFOV", 0.0f);

	if (!res_x || !res_y) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		res_x = info.rcMonitor.right - info.rcMonitor.left;
		res_y = info.rcMonitor.bottom - info.rcMonitor.top;
	}
	//MessageBox(0, "0", "0", 0);
	//CPatch::RedirectJump(0x446B08, asm_446B08);
	CPatch::RedirectJump(0x487D6D, asm_487D6D);

	CPatch::Nop(0x48B10E, 6);
	CPatch::Nop(0x48B108, 6);

	CPatch::Nop(0x48B0BC, 6);
	CPatch::Nop(0x48B0B6, 6);

	
	CPatch::SetUInt(0x487C40 + 0x6, res_x);
	CPatch::SetUInt(0x487C4A + 0x6, res_y);

	CPatch::SetUInt(0x622B21 + 0x6, res_x);
	CPatch::SetUInt(0x622B17 + 0x6, res_y);

	CPatch::SetUInt(0x487C40 + 0x6, res_x);
	CPatch::SetUInt(0x487C4A + 0x6, res_y);

	CPatch::SetUInt(0x7716C8, res_x);
	CPatch::SetUInt(0x7716C4, res_y);

	CPatch::SetUInt(0x4D5599, res_x);
	CPatch::SetUInt(0x4D5594, res_y);


	CPatch::SetUInt(0x7A33C8, res_x);
	CPatch::SetUInt(0x7ACFB8, res_x);
	CPatch::SetUInt(0x7BB890, res_x);
	CPatch::SetUInt(0x7BCF1C, res_x);
	CPatch::SetUInt(0x7C5650, res_x);
	CPatch::SetUInt(0x7C6CCC, res_x);
	CPatch::SetUInt(0x7C8348, res_x);

	CPatch::SetUInt(0x7A33C8 - 0x4, res_y);
	CPatch::SetUInt(0x7ACFB8 - 0x4, res_y);
	CPatch::SetUInt(0x7BB890 - 0x4, res_y);
	CPatch::SetUInt(0x7BCF1C - 0x4, res_y);
	CPatch::SetUInt(0x7C5650 - 0x4, res_y);
	CPatch::SetUInt(0x7C6CCC - 0x4, res_y);
	CPatch::SetUInt(0x7C8348 - 0x4, res_y);
	

	//stretching
	CPatch::Nop(0x4152CE, 5);
	CPatch::SetFloat(0x756E50, (static_cast<float>(res_x) / static_cast<float>(res_y)));

	//FOV
	/*fScale = (0.5f *(static_cast<float>(res_x) / static_cast<float>(res_y))) / (4.0f / 3.0f);
	fScale > 1.0f ? fScale = 1.0f : fScale;
	CPatch::SetPointer(0x60121A + 0x2, &fScale);*/

	fres_x = static_cast<float>(res_x);
	fres_y = static_cast<float>(res_y);

	fAspectRatio = fres_x / fres_y;
	fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * fAspectRatio));
	
	if (!FOV)
	{
		CPatch::SetFloat(0x6FFF08, fDynamicScreenFieldOfViewScale);
	} else {
		CPatch::SetFloat(0x6FFF08, FOV);
	}

	//HUD
	res_x43 = static_cast<int>(res_x / (4.0f / 3.0f));

	if (iniReader.ReadInteger("MAIN", "FixHUD", 0))
	{
		while (*(DWORD *)0x77166C == 0)
			Sleep(1000);

		//485246 48580A 48AFA3 cuts viewport
		CPatch::SetPointer(0x400000 + 0xE47D, &res_x43);
		CPatch::SetPointer(0x400000 + 0xE4D1, &res_x43);
		CPatch::SetPointer(0x400000 + 0xE4DB, &res_x43);
		CPatch::SetPointer(0x400000 + 0xE4FF, &res_x43);
		CPatch::SetPointer(0x400000 + 0xE52E, &res_x43);
		CPatch::SetPointer(0x400000 + 0xE53F, &res_x43);
		CPatch::SetPointer(0x400000 + 0xE54E, &res_x43);
		CPatch::SetPointer(0x400000 + 0xE55C, &res_x43);
		CPatch::SetPointer(0x400000 + 0xE5B2, &res_x43);
		CPatch::SetPointer(0x400000 + 0xE9AF, &res_x43);
		CPatch::SetPointer(0x400000 + 0xFABE, &res_x43);
		CPatch::SetPointer(0x400000 + 0xFAFA, &res_x43);
		CPatch::SetPointer(0x400000 + 0x156C0, &res_x43);
		CPatch::SetPointer(0x400000 + 0x24CF4, &res_x43);
		CPatch::SetPointer(0x400000 + 0x6BCF5, &res_x43);
		CPatch::SetPointer(0x400000 + 0x6BE92, &res_x43);
		CPatch::SetPointer(0x400000 + 0x6BED2, &res_x43);
		CPatch::SetPointer(0x400000 + 0x6BF15, &res_x43);
		CPatch::SetPointer(0x400000 + 0x6BF66, &res_x43);
		CPatch::SetPointer(0x400000 + 0x6C003, &res_x43);
		CPatch::SetPointer(0x400000 + 0x6C075, &res_x43);
		CPatch::SetPointer(0x400000 + 0x6DF1C, &res_x43);
		CPatch::SetPointer(0x400000 + 0x6E26B, &res_x43);
		CPatch::SetPointer(0x400000 + 0x6E4CB, &res_x43);
		CPatch::SetPointer(0x400000 + 0x72744, &res_x43);
		CPatch::SetPointer(0x400000 + 0x733C4, &res_x43);
		CPatch::SetPointer(0x400000 + 0x73677, &res_x43);
		CPatch::SetPointer(0x400000 + 0x73D62, &res_x43);
		CPatch::SetPointer(0x400000 + 0x798C8, &res_x43);
		CPatch::SetPointer(0x400000 + 0x7E009, &res_x43);
		CPatch::SetPointer(0x400000 + 0x7E0F0, &res_x43);
		CPatch::SetPointer(0x400000 + 0x7E4B6, &res_x43);
		CPatch::SetPointer(0x400000 + 0x7E4CC, &res_x43);
		CPatch::SetPointer(0x400000 + 0x7E7BD, &res_x43);
		CPatch::SetPointer(0x400000 + 0x7E8E8, &res_x43);
		CPatch::SetPointer(0x400000 + 0x7E921, &res_x43);
		CPatch::SetPointer(0x400000 + 0x7EA63, &res_x43);
		CPatch::SetPointer(0x400000 + 0x7EABF, &res_x43);
		CPatch::SetPointer(0x400000 + 0x7EF35, &res_x43);
		CPatch::SetPointer(0x400000 + 0x7F39E, &res_x43);
		CPatch::SetPointer(0x400000 + 0x80375, &res_x43);
		CPatch::SetPointer(0x400000 + 0x8067E, &res_x43);
		CPatch::SetPointer(0x400000 + 0x81B62, &res_x43);
		CPatch::SetPointer(0x400000 + 0x8239D, &res_x43);
		CPatch::SetPointer(0x400000 + 0x824A4, &res_x43);
		CPatch::SetPointer(0x400000 + 0x824D5, &res_x43);
		CPatch::SetPointer(0x400000 + 0x84E40, &res_x43);
		CPatch::SetPointer(0x400000 + 0x85208, &res_x43);
		CPatch::SetPointer(0x400000 + 0x879AE, &res_x43);
		CPatch::SetPointer(0x400000 + 0x8913E, &res_x43);
		CPatch::SetPointer(0x400000 + 0x90951, &res_x43);
		CPatch::SetPointer(0x400000 + 0x90B01, &res_x43);
		CPatch::SetPointer(0x400000 + 0x90C92, &res_x43);
		CPatch::SetPointer(0x400000 + 0x9B1DD, &res_x43);
		CPatch::SetPointer(0x400000 + 0xA08A3, &res_x43);
		CPatch::SetPointer(0x400000 + 0xA09CA, &res_x43);
		CPatch::SetPointer(0x400000 + 0xA45E5, &res_x43);
		CPatch::SetPointer(0x400000 + 0xA45EF, &res_x43);
		CPatch::SetPointer(0x400000 + 0xAC31A, &res_x43);
		CPatch::SetPointer(0x400000 + 0xAC551, &res_x43);
		CPatch::SetPointer(0x400000 + 0xAC5D3, &res_x43);
		CPatch::SetPointer(0x400000 + 0xAC620, &res_x43);
		CPatch::SetPointer(0x400000 + 0xAC6AF, &res_x43);
		CPatch::SetPointer(0x400000 + 0xAC954, &res_x43);
		CPatch::SetPointer(0x400000 + 0xACEBE, &res_x43);
		CPatch::SetPointer(0x400000 + 0xAD080, &res_x43);
		CPatch::SetPointer(0x400000 + 0xADC05, &res_x43);
		CPatch::SetPointer(0x400000 + 0xADE83, &res_x43);
		CPatch::SetPointer(0x400000 + 0xADFE2, &res_x43);
		CPatch::SetPointer(0x400000 + 0xAE205, &res_x43);
		CPatch::SetPointer(0x400000 + 0xAFBC1, &res_x43);
		CPatch::SetPointer(0x400000 + 0xB13C7, &res_x43);
		CPatch::SetPointer(0x400000 + 0xB1F16, &res_x43);
		CPatch::SetPointer(0x400000 + 0xBECB5, &res_x43);
		CPatch::SetPointer(0x400000 + 0xBEEF5, &res_x43);
		CPatch::SetPointer(0x400000 + 0xC1820, &res_x43);
		CPatch::SetPointer(0x400000 + 0xC3ADD, &res_x43);
		CPatch::SetPointer(0x400000 + 0xC7787, &res_x43);
		CPatch::SetPointer(0x400000 + 0xC7DDE, &res_x43);
		CPatch::SetPointer(0x400000 + 0xC8278, &res_x43);
		CPatch::SetPointer(0x400000 + 0xC8301, &res_x43);
		CPatch::SetPointer(0x400000 + 0xC8361, &res_x43);
		CPatch::SetPointer(0x400000 + 0xC850B, &res_x43);
		CPatch::SetPointer(0x400000 + 0xC89FB, &res_x43);
		CPatch::SetPointer(0x400000 + 0xC8DB4, &res_x43);
		CPatch::SetPointer(0x400000 + 0xC9138, &res_x43);
		CPatch::SetPointer(0x400000 + 0xC93A8, &res_x43);
		CPatch::SetPointer(0x400000 + 0xC9747, &res_x43);
		CPatch::SetPointer(0x400000 + 0xC97D0, &res_x43);
		CPatch::SetPointer(0x400000 + 0xC9C47, &res_x43);
		CPatch::SetPointer(0x400000 + 0xC9D0D, &res_x43);
		CPatch::SetPointer(0x400000 + 0xC9F68, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCA359, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCA45C, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCA4D5, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCA551, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCA618, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCB357, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCBD25, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCCFC2, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCCFEF, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCD16D, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCD1B1, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCD3C6, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCD3F2, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCD6EF, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCD71F, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCD9FD, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCDB3D, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCDF45, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCE249, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCE783, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCEB41, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCEFC7, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCF050, &res_x43);
		CPatch::SetPointer(0x400000 + 0xCFA67, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD1357, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD1F36, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD209B, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD2168, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD2E98, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD38C5, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD4055, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD4342, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD4775, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD4845, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD4898, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD4935, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD4A2A, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD4AF6, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD4B8E, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD4CAB, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD4CF5, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD4DEA, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD5588, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD570E, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD582E, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD59BF, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD5AFC, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD5EBF, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD6202, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD65F4, &res_x43);
		CPatch::SetPointer(0x400000 + 0xD748C, &res_x43);
		CPatch::SetPointer(0x400000 + 0xDA8EE, &res_x43);
		CPatch::SetPointer(0x400000 + 0xDA913, &res_x43);
		CPatch::SetPointer(0x400000 + 0xDABAC, &res_x43);
		CPatch::SetPointer(0x400000 + 0xDABD1, &res_x43);
		CPatch::SetPointer(0x400000 + 0xDAEDF, &res_x43);
		CPatch::SetPointer(0x400000 + 0xE16FF, &res_x43);
		CPatch::SetPointer(0x400000 + 0xEBCE7, &res_x43);
		CPatch::SetPointer(0x400000 + 0xEF53B, &res_x43);
		CPatch::SetPointer(0x400000 + 0xEF78B, &res_x43);
		CPatch::SetPointer(0x400000 + 0xEF9D7, &res_x43);
		CPatch::SetPointer(0x400000 + 0xEFBDD, &res_x43);
		CPatch::SetPointer(0x400000 + 0xEFDE7, &res_x43);
		CPatch::SetPointer(0x400000 + 0xF004A, &res_x43);
		CPatch::SetPointer(0x400000 + 0xF02DC, &res_x43);
		CPatch::SetPointer(0x400000 + 0xF153D, &res_x43);
		CPatch::SetPointer(0x400000 + 0xF175F, &res_x43);
		CPatch::SetPointer(0x400000 + 0xF196B, &res_x43);
		CPatch::SetPointer(0x400000 + 0xF1B93, &res_x43);
		CPatch::SetPointer(0x400000 + 0xF2DA5, &res_x43);
		CPatch::SetPointer(0x400000 + 0xF62CB, &res_x43);
		CPatch::SetPointer(0x400000 + 0xF8070, &res_x43);
		CPatch::SetPointer(0x400000 + 0xF8C84, &res_x43);
		CPatch::SetPointer(0x400000 + 0xFD6A8, &res_x43);
		CPatch::SetPointer(0x400000 + 0xFE7C1, &res_x43);
		CPatch::SetPointer(0x400000 + 0xFE836, &res_x43);
		CPatch::SetPointer(0x400000 + 0xFED75, &res_x43);
		CPatch::SetPointer(0x400000 + 0xFEE4C, &res_x43);
		//CPatch::SetPointer(0x400000 + 0x102167, &res_x43); Scope and black tint
		CPatch::SetPointer(0x400000 + 0x1063DC, &res_x43);
		CPatch::SetPointer(0x400000 + 0x106C96, &res_x43);
		CPatch::SetPointer(0x400000 + 0x106ED5, &res_x43);
		CPatch::SetPointer(0x400000 + 0x108F6E, &res_x43);
		CPatch::SetPointer(0x400000 + 0x10AEEB, &res_x43);
		CPatch::SetPointer(0x400000 + 0x10B2C8, &res_x43);
		CPatch::SetPointer(0x400000 + 0x10C2ED, &res_x43);
		CPatch::SetPointer(0x400000 + 0x16360C, &res_x43);
		CPatch::SetPointer(0x400000 + 0x1671D9, &res_x43);
		CPatch::SetPointer(0x400000 + 0x1672B9, &res_x43);
		CPatch::SetPointer(0x400000 + 0x167407, &res_x43);
		CPatch::SetPointer(0x400000 + 0x1674DF, &res_x43);
		CPatch::SetPointer(0x400000 + 0x17600B, &res_x43);
		CPatch::SetPointer(0x400000 + 0x1806CC, &res_x43);
		CPatch::SetPointer(0x400000 + 0x1DB9F3, &res_x43);
		CPatch::SetPointer(0x400000 + 0x1E87AC, &res_x43);
		CPatch::SetPointer(0x400000 + 0x200586, &res_x43);
		CPatch::SetPointer(0x400000 + 0x220B27, &res_x43);
		CPatch::SetPointer(0x400000 + 0x220D8E, &res_x43);
		CPatch::SetPointer(0x400000 + 0x222B23, &res_x43);
		CPatch::SetPointer(0x400000 + 0x226971, &res_x43);
		CPatch::SetPointer(0x400000 + 0x22699D, &res_x43);
		CPatch::SetPointer(0x400000 + 0x226A55, &res_x43);
		CPatch::SetPointer(0x400000 + 0x226AE8, &res_x43);
		CPatch::SetPointer(0x400000 + 0x2282C3, &res_x43);
		CPatch::SetPointer(0x400000 + 0x22A6D3, &res_x43);

	}
	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
		//Init();
	}
	return TRUE;
}