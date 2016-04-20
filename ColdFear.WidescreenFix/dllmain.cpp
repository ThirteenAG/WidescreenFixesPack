#include "..\includes\stdafx.h"
#include "..\includes\hooking\Hooking.Patterns.h"
HWND hWnd;

#define _USE_MATH_DEFINES
#include "math.h"
#define DEGREE_TO_RADIAN(fAngle) \
	((fAngle)* (float)M_PI / 180.0f)
#define RADIAN_TO_DEGREE(fAngle) \
	((fAngle)* 180.0f / (float)M_PI)
#define SCREEN_AR_NARROW			(4.0f / 3.0f)	// 640.0f / 480.0f
#define SCREEN_FOV_HORIZONTAL		15.0f
#define SCREEN_FOV_VERTICAL			(2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / SCREEN_AR_NARROW)))	// Default is 75.0f.
float fScreenFieldOfViewVStd = SCREEN_FOV_VERTICAL;
float fDynamicScreenFieldOfViewScale;

struct Screen
{
	int Width;
	int Height;
	float fWidth;
	float fHeight;
	float fFieldOfView;
	float fAspectRatio;
	float fHudScaleX;
	float fHudOffset;
	float fHudOffsetRight;
	float fFMVoffsetStartX;
	float fFMVoffsetEndX;
	float fFMVoffsetStartY;
	float fFMVoffsetEndY;
} Screen;

union FColor
{
	uint32_t RGBA;
	struct
	{
		uint8_t B, G, R, A;
	};
};

DWORD WINAPI Thread(LPVOID)
{
	return 0;
}

uintptr_t sub_42B380_addr, sub_42B3A0_addr, sub_42B460_addr, sub_42B610_addr, sub_42B740_addr, sub_42BA20_addr;
uintptr_t sub_42BBA0_addr, sub_42BD11_addr, sub_42C880_addr, sub_42C8D0_addr, sub_42CA50_addr, sub_42B900_addr;

bool bDisableCutsceneBorders;
uint32_t nFMVWidescreenMode;

int __cdecl sub_42B460(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, int a9, int a10, int a11)
{
	auto _sub_42B460 = (int(__cdecl *)(float, float, float, float, float, float, float, float, int, int, int)) sub_42B460_addr;

	a1 = (a1 / Screen.fHudScaleX) + Screen.fHudOffset;
	a3 = (a3 / Screen.fHudScaleX) + Screen.fHudOffset;

	return _sub_42B460(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
}

void __cdecl sub_42B610(float a1, float a2, float a3, float a4, int a5, int a6, int a7)
{
	auto _sub_42B610 = (void(__cdecl *)(float, float, float, float, int, int, int)) sub_42B610_addr;
	
	if (a1 != 0.0f && a2 != 0.0f) //menu background rendered here
	{
		a1 = (a1 / Screen.fHudScaleX) + Screen.fHudOffset;
		a3 = (a3 / Screen.fHudScaleX);
	}

	return _sub_42B610(a1, a2, a3, a4, a5, a6, a7);
}

void __cdecl sub_42B740(float a1, float a2, float a3, float a4, int a5, int a6, int a7)
{
	auto _sub_42B740 = (void(__cdecl *)(float, float, float, float, int, int, int)) sub_42B740_addr;
	auto Color = *(FColor*)&a5;

	if (a1 == 0.0f && ((a2 >= 0.0f && a2 <= 69.5f) || (a2 >= 400.0f && a2 <= 480.0f)) /*&& (a4 == 69.5f || a4 == 78.5f)*/ && a3 == 640.0f && a5 == 0xff000000 && a6 == 0 && a7 == 0) //borders
	{
		if (bDisableCutsceneBorders)
		{
			return;
		}
		else
		{
			return _sub_42B740(a1, a2, a3, a4, a5, a6, a7);
		}
	}

	if (a1 == 0.0f && a2 == 0.0f && a3 == Screen.fWidth && a4 == Screen.fHeight && (Color.R == 0 && Color.G == 0 && Color.B == 0) && a6 == 0 && a7 == 0x00000001) //fading
		return _sub_42B740(a1, a2, a3, a4, a5, a6, a7);

	a1 = (a1 / Screen.fHudScaleX) + Screen.fHudOffset;
	a3 = (a3 / Screen.fHudScaleX);

	return _sub_42B740(a1, a2, a3, a4, a5, a6, a7);
}

void __cdecl sub_42BA20(float a1, float a2, int a3, int a4, int a5)
{
	auto _sub_42BA20 = (void(__cdecl *)(float, float, int, int, int)) sub_42BA20_addr;
	
	a1 = (a1 / Screen.fHudScaleX) + Screen.fHudOffset;
	//a3 = (a3 / Screen.fHudScaleX) + Screen.fHudOffset;

	return _sub_42BA20(a1, a2, a3, a4, a5);
}

int __cdecl sub_42BBA0(int a1, int a2, int a3, int a4, char a5) // all hud borders and other things
{
	auto _sub_42BBA0 = (int(__cdecl *)(int, int, int, int, char)) sub_42BBA0_addr;

	*(float *)a1 = (*(float *)a1 / Screen.fHudScaleX) + Screen.fHudOffset;
	*(float *)a2 = (*(float *)a2 / Screen.fHudScaleX) + Screen.fHudOffset;

	return _sub_42BBA0(a1, a2, a3, a4, a5);
}

int __cdecl sub_42C8D0(float a1, float a2, int a3, char a4, char a5, char a6)
{
	auto _sub_42C8D0 = (int(__cdecl *)(float, float, int, char, char, char)) sub_42C8D0_addr;

	a1 = (a1 / Screen.fHudScaleX) + Screen.fHudOffset;

	return _sub_42C8D0(a1, a2, a3, a4, a5, a6);
}

int __cdecl sub_42CA50(float a1, float a2, float a3, float a4, int a5, char a6, char a7, char a8)
{
	auto _sub_42CA50 = (int(__cdecl *)(float, float, float, float, int, char, char, char)) sub_42CA50_addr;

	a1 = (a1 / Screen.fHudScaleX) + Screen.fHudOffset;
	a3 = (a3 / Screen.fHudScaleX) + Screen.fHudOffset;

	return _sub_42CA50(a1, a2, a3, a4, a5, a6, a7, a8);
}

uint32_t FMV = 0;
void __cdecl sub_42B900(float a1, float a2, float a3, float a4, int a5) //fmv
{
	__asm mov FMV, esp
	FMV = *(uint32_t*)(FMV + 0x34);

	auto _sub_42B900 = (void(__cdecl *)(float, float, float, float, int)) sub_42B900_addr;
	auto _sub_42B740 = (void(__cdecl *)(float, float, float, float, int, int, int)) sub_42B740_addr;

	_sub_42B740(0.0f, 0.0f, Screen.fWidth, Screen.fHeight + 500.0f, 0xff000000, 0, 0);

	a1 = (a1 / Screen.fHudScaleX) + (Screen.fHudOffset * (Screen.fWidth / 640.f));
	a3 = (a3 / Screen.fHudScaleX);

	if (nFMVWidescreenMode && FMV == 0x5F564D46) //"FMV_"
	{
		a1 -= (720.0f / (640.0f / 280.0f) / 2.0f);
		a2 -= (720.0f / (640.0f / 280.0f) / 2.0f) / (4.0f / 3.0f);
		a3 += (720.0f / (640.0f / 280.0f) / 2.0f) * 2.0f;
		a4 += (720.0f / (640.0f / 280.0f) / 2.0f) / (4.0f / 3.0f) * 2.0f;
	}

	return _sub_42B900(a1, a2, a3, a4, a5);
}

void __cdecl sub_42C880(int a1)
{
	*(float *)a1 = *(float*)0x81C44C;
	*(float *)(a1 + 4) = *(float*)0x81C450;
}

int __cdecl sub_42BD11(float a1, float a2, int a3, char a4, unsigned __int8 a5)
{
	//?
}

void Init()
{
	CIniReader iniReader("");
	Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
	Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
	bDisableCutsceneBorders = iniReader.ReadInteger("MAIN", "DisableCutsceneBorders", 0) == 1;
	nFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 0);

	if (!Screen.Width || !Screen.Height) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		Screen.Width = info.rcMonitor.right - info.rcMonitor.left;
		Screen.Height = info.rcMonitor.bottom - info.rcMonitor.top;
	}

	Screen.fWidth = static_cast<float>(Screen.Width);
	Screen.fHeight = static_cast<float>(Screen.Height);
	Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
	Screen.fHudScaleX = (480.0f * Screen.fAspectRatio) / 640.0f;
	Screen.fHudOffset = ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f) / (Screen.fWidth / 640.f);
	fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(fScreenFieldOfViewVStd * 0.5f)) * Screen.fAspectRatio)) * (1.0f / SCREEN_FOV_HORIZONTAL);

	auto pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 08 89");
	struct SetResHook
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.esi = Screen.Width;
			regs.edi = Screen.Height;
		}
	}; injector::MakeInline<SetResHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(76)); //0x4021A3, 0x4021EF

	//Hud
	sub_42B380_addr = (uintptr_t)hook::pattern("55 8B EC 8B 45 08 D9 05 ? ? ? ? D8 08").get(0).get<uint32_t>(0);
	sub_42B3A0_addr = (uintptr_t)hook::pattern("55 8B EC 8A 45 14 56 84 C0 74 0D D9 45 0C E8 ? ? ? ? D9 45 10").get(0).get<uint32_t>(0);
	sub_42B460_addr = (uintptr_t)hook::pattern("55 8B EC A0 ? ? ? ? 56 84 C0 0F 84 ? ? 00 00").get(0).get<uint32_t>(0);
	sub_42B610_addr = (uintptr_t)hook::pattern("55 8B EC A0 ? ? ? ? 57 84 C0 0F 84 ? ? 00 00").get(0).get<uint32_t>(0);
	sub_42B740_addr = (uintptr_t)hook::pattern("55 8B EC A0 ? ? ? ? 57 84 C0 0F 84 ? ? 00 00").get(1).get<uint32_t>(0);
	sub_42BA20_addr = (uintptr_t)hook::pattern("55 8B EC 56 57 8B 3D ? ? ? ? 85 FF 0F 84 C8 00 00 00").get(0).get<uint32_t>(0);
	sub_42BBA0_addr = (uintptr_t)hook::pattern("55 8B EC 51 A1 ? ? ? ? 8B 0D ? ? ? ? 56").get(0).get<uint32_t>(0);
	//sub_42BD11_addr = hook::pattern("").get(0).get<uint32_t>(0); ?
	sub_42C880_addr = (uintptr_t)hook::pattern("55 8B EC 8B 45 08 8B 0D ? ? ? ? 8B 15 ? ? ? ? 89 08").get(0).get<uint32_t>(0);
	sub_42C8D0_addr = (uintptr_t)hook::pattern("55 8B EC 83 EC 0C 8A 55 14 53 56 33 F6 84 D2").get(0).get<uint32_t>(0);
	sub_42CA50_addr = (uintptr_t)hook::pattern("55 8B EC 83 EC 0C 8A 55 1C 53 56 33 F6 84 D2").get(0).get<uint32_t>(0);
	sub_42B900_addr = (uintptr_t)hook::pattern("55 8B EC A1 ? ? ? ? 8B 0D ? ? ? ? 56 8B 35 ? ? ? ? 57 03 C8").get(0).get<uint32_t>(0);

	pattern = hook::pattern("E8 ? ? ? ?");
	for (size_t i = 0; i < pattern.size(); ++i)
	{
		auto addr = pattern.get(i).get<uint32_t>(0);
		auto dest = injector::GetBranchDestination(addr, true).as_int();
		
		if (dest == sub_42B460_addr)
			injector::MakeCALL(addr, sub_42B460, true);
		
		if (dest == sub_42B610_addr)
			injector::MakeCALL(addr, sub_42B610, true);
		
		if (dest == sub_42B740_addr)
			injector::MakeCALL(addr, sub_42B740, true);
		
		if (dest == sub_42BA20_addr)
			injector::MakeCALL(addr, sub_42BA20, true);
				
		if (dest == sub_42C8D0_addr)
			injector::MakeCALL(addr, sub_42C8D0, true);
		
		if (dest == sub_42CA50_addr)
			injector::MakeCALL(addr, sub_42CA50, true);

		///if (dest == sub_42B380_addr)
		///	injector::MakeCALL(addr, sub_42B380, true); ?

		///if (dest == sub_42B3A0_addr)
		///	injector::MakeCALL(addr, sub_42B3A0, true); ? 

		//if (dest == sub_42BBA0_addr)
		//	injector::MakeCALL(addr, sub_42BBA0, true); ??

		//if (dest == sub_42BD11_addr)
		//	injector::MakeCALL(addr, sub_42BD11, true); ?

		//if (dest == sub_42C880_addr)
		//	injector::MakeCALL(addr, sub_42C880, true); ?

	}

	pattern = hook::pattern("89 45 18 8B 5D FC DB 45 18 D8 0D");
	struct sub_42BBA0Hook
	{
		void operator()(injector::reg_pack& regs)
		{
			*(uintptr_t*)(regs.ebp + 0x18) = regs.eax;
			regs.ebx = *(uintptr_t*)(regs.ebp - 0x4);

			*(float*)(regs.esi + 0x10) = ((*(float*)(regs.esi + 0x10)) / Screen.fHudScaleX) + (Screen.fHudOffset * (Screen.fWidth / 640.f));
			*(float*)(regs.esi + 0x18) = ((*(float*)(regs.esi + 0x18)) / Screen.fHudScaleX) + (Screen.fHudOffset * (Screen.fWidth / 640.f));
		}
	}; injector::MakeInline<sub_42BBA0Hook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(6)); //0x42BC7B, 0x42BC7B + 6

	//FMV
	pattern = hook::pattern("D9 5C 24 04 DB 45 10 D9 1C 24 6A 00 6A 00 E8 ? ? ? ? 83 C4 14");
	injector::MakeCALL(pattern.get(0).get<uint32_t>(14), sub_42B900, true); //0x486EC9

	//FOV
	pattern = hook::pattern("89 45 18 8B 5D FC DB 45 18 D8 0D");
	struct FOVHook
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.ecx = regs.ebp - 0x64;
			regs.edx = regs.ebp - 0x24;

			*(float *)(regs.esi + 0x40) *= fDynamicScreenFieldOfViewScale; //CAMERA_FOLLOW_MINDIST 
			*(float *)(regs.esi + 0x44) *= fDynamicScreenFieldOfViewScale; //CAMERA_FOLLOW_MAXDIST 
			*(float *)(regs.esi + 0xBC) *= fDynamicScreenFieldOfViewScale; //CAMERA_AIM_FOV
			*(float *)(regs.esi + 0xC4) *= fDynamicScreenFieldOfViewScale; //CAMERA_CONSTRAINT_FOV

		}
	}; injector::MakeInline<FOVHook>(0x4AF5EE, 0x4AF5EE + 6); ///*pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(6)*/
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
		//injector::MakeNOP(0x402232, 2, true); //debug menu
		//CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Thread, NULL, 0, NULL);
	}
	return TRUE;
}
