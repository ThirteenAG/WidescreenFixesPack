#include "stdafx.h"
HWND hWnd;
bool bDelay;

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
		uint8_t R, G, B, A;
	};
};

uintptr_t sub_42B380_addr, sub_42B3A0_addr, sub_42B460_addr, sub_42B610_addr, sub_42B740_addr, sub_42BA20_addr;
uintptr_t sub_42BBA0_addr, sub_42BD11_addr, sub_42C880_addr, sub_42C8D0_addr, sub_42CA50_addr, sub_42B900_addr;

bool bDisableCutsceneBorders;
uint32_t nFMVWidescreenMode;
bool bHudWidescreenMode;
int32_t nWidescreenHudOffset;
float fWidescreenHudOffset;

//#define _LOG
#ifdef _LOG
#include <fstream>
ofstream logfile;
uint32_t logit;
#endif // _LOG

int __cdecl sub_42B460(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, int a9, int a10, int a11)
{
	auto _sub_42B460 = (int(__cdecl *)(float, float, float, float, float, float, float, float, int, int, int)) sub_42B460_addr;

	uint32_t n_offsetX1 = static_cast<uint32_t>((a1));
	uint32_t n_offsetY1 = static_cast<uint32_t>((a2));
	uint32_t n_offsetX2 = static_cast<uint32_t>((a3));
	uint32_t n_offsetY2 = static_cast<uint32_t>((a4));
	auto Color = *(FColor*)&a9;

#ifdef _LOG
	if (logit)
		logfile << std::dec << n_offsetX1 << " " << n_offsetX2 << " " << n_offsetY1 << " " << n_offsetY2 << " " << std::hex << a9 << " " << a10 << " " << a11 << std::endl;
#endif // _LOG

	a1 = (a1 / Screen.fHudScaleX) + Screen.fHudOffset;
	a3 = (a3 / Screen.fHudScaleX) + Screen.fHudOffset;

	if (bHudWidescreenMode)
	{
		if ((n_offsetX1 >= 433 && n_offsetX1 <= 606) && (n_offsetX2 >= 441 && n_offsetX2 <= 616) && (n_offsetY1 >= 315 && n_offsetY1 <= 448) && ((Color.R == 0xff && Color.G == 0xff && Color.B == 0xff) || (Color.R == 0xfe && Color.G == 0xfe && Color.B == 0xfe) || (Color.A == 0xff && Color.R == 0xff)))
		{
			a1 += fWidescreenHudOffset;
			a3 += fWidescreenHudOffset;
		}

		if (
		(n_offsetX1 == 24) && (n_offsetX2 == 136) && (n_offsetY1 >= 315 && n_offsetY1 <= 448) && (Color.R == 0xff && Color.G == 0xff && Color.B == 0xff) || //text (RESISTANCE | LIFE)
		(n_offsetX1 == 92) && (n_offsetX2 == 204) && (n_offsetY1 >= 315 && n_offsetY1 <= 448) && (Color.R == 0xff && Color.G == 0xff && Color.B == 0xff) //text (INTERACTION)
		)
		{
			a1 -= fWidescreenHudOffset;
			a3 -= fWidescreenHudOffset;
		}
	}

	return _sub_42B460(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
}

void __cdecl sub_42B610(float a1, float a2, float a3, float a4, int a5, int a6, int a7)
{
	auto _sub_42B610 = (void(__cdecl *)(float, float, float, float, int, int, int)) sub_42B610_addr;
	
	uint32_t n_offsetX1 = static_cast<uint32_t>((a1));
	uint32_t n_offsetY1 = static_cast<uint32_t>((a2));
	uint32_t n_offsetX2 = static_cast<uint32_t>((a3));
	uint32_t n_offsetY2 = static_cast<uint32_t>((a4));

/*#ifdef _LOG
	if (logit)
		logfile << std::dec << n_offsetX1 << " " << n_offsetX2 << " " << n_offsetY1 << " " << n_offsetY2 << " " << std::hex << a5 << " " << a6 << " " << a7 << std::endl;
#endif // _LOG*/

	if (a1 != 0.0f && a2 != 0.0f) //menu background rendered here
	{
		a1 = (a1 / Screen.fHudScaleX) + Screen.fHudOffset;
		a3 = (a3 / Screen.fHudScaleX);
	}

	if (bHudWidescreenMode)
	{
		if (((n_offsetX1 == 21) && (n_offsetX2 >= 0 && n_offsetX2 <= 166) && (n_offsetY1 >= 315 && n_offsetY1 <= 448) && (a6 == 0 && a7 == 0))) //health, resistance bar etc
		{
			a1 -= fWidescreenHudOffset;
			//a3 -= fWidescreenHudOffset;
		}
	}

	return _sub_42B610(a1, a2, a3, a4, a5, a6, a7);
}

void __cdecl sub_42B740(float a1, float a2, float a3, float a4, int a5, int a6, int a7) //health bar border, damage red bar etc
{
	auto _sub_42B740 = (void(__cdecl *)(float, float, float, float, int, int, int)) sub_42B740_addr;
	auto Color = *(FColor*)&a5;

	uint32_t n_offsetX1 = static_cast<uint32_t>((a1));
	uint32_t n_offsetY1 = static_cast<uint32_t>((a2));
	uint32_t n_offsetX2 = static_cast<uint32_t>((a3));
	uint32_t n_offsetY2 = static_cast<uint32_t>((a4));

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

	if (a1 == 0.0f && a2 == 0.0f && a3 == Screen.fWidth && a4 == Screen.fHeight && ((Color.R == 0 && Color.G == 0 && Color.B == 0) || (Color.R == 0xff && Color.G == 0xff && Color.B == 0xff)) && a6 == 0 && a7 == 0x00000001) //fading
		return _sub_42B740(a1, a2, a3, a4, a5, a6, a7);

	a1 = (a1 / Screen.fHudScaleX) + Screen.fHudOffset;
	a3 = (a3 / Screen.fHudScaleX);

	if (bHudWidescreenMode)
	{
		if (((n_offsetX1 == 20 || n_offsetX1 == 164) && (n_offsetX2 == 1 || n_offsetX2 == 144 || n_offsetX2 == 145) && (n_offsetY1 >= 315 && n_offsetY1 <= 448) && (Color.R == 0 && Color.G == 0 && Color.B == 0) && (a6 == 0))//health bar border etc
		|| ((n_offsetX1 >= 20 && n_offsetX1 <= 166) && (Color.R == 0xff && Color.G == 0x24 && Color.B == 0x08))	)
		{
			a1 -= fWidescreenHudOffset;
			//a3 -= fWidescreenHudOffset;
		}
	}

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

	a1 = (a1 / Screen.fHudScaleX) + (Screen.fHudOffset * (Screen.fWidth / 640.0f));
	a3 = (a3 / Screen.fHudScaleX);

	if (nFMVWidescreenMode && FMV == 0x5F564D46) //"FMV_"
	{
		a1 -= (Screen.fHeight / (640.0f / 280.0f) / 2.0f);
		a2 -= (Screen.fHeight / (640.0f / 280.0f) / 2.0f) / (4.0f / 3.0f);
		a3 += (Screen.fHeight / (640.0f / 280.0f) / 2.0f) * 2.0f;
		a4 += (Screen.fHeight / (640.0f / 280.0f) / 2.0f) / (4.0f / 3.0f) * 2.0f;
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
	return 0; //?
}

DWORD WINAPI Init(LPVOID)
{
	auto pattern = hook::pattern("FF 74 24 10 FF 74 24 10 FF 74 24 10 FF 74 24 10 E8 ? ? ? ? C2 10 00");
	if (!(pattern.size() > 0) && !bDelay)
	{
		bDelay = true;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
		return 0;
	}

	if (bDelay)
	{
		while (!(pattern.size() > 0))
			pattern = hook::pattern("FF 74 24 10 FF 74 24 10 FF 74 24 10 FF 74 24 10 E8 ? ? ? ? C2 10 00");
	}

	CIniReader iniReader("");
	Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
	Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
	bDisableCutsceneBorders = iniReader.ReadInteger("MAIN", "DisableCutsceneBorders", 0) == 1;
	nFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 0);
	bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) == 1;
	nWidescreenHudOffset = iniReader.ReadInteger("MAIN", "WidescreenHudOffset", 75);
	fWidescreenHudOffset = static_cast<float>(nWidescreenHudOffset);

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
	Screen.fHudOffset = ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f) / (Screen.fWidth / 640.0f);
	fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(fScreenFieldOfViewVStd * 0.5f)) * Screen.fAspectRatio)) * (1.0f / SCREEN_FOV_HORIZONTAL);
	if (Screen.fAspectRatio < (16.0f / 9.0f))
	{
		fWidescreenHudOffset = fWidescreenHudOffset / (((16.0f / 9.0f) / (Screen.fAspectRatio)) * 1.5f);
	}

	pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 08 89");
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
		///	injector::MakeCALL(addr, sub_42B380, true);

		///if (dest == sub_42B3A0_addr)
		///	injector::MakeCALL(addr, sub_42B3A0, true);

		//if (dest == sub_42BBA0_addr)
		//	injector::MakeCALL(addr, sub_42BBA0, true);

		//if (dest == sub_42BD11_addr)
		//	injector::MakeCALL(addr, sub_42BD11, true);

		//if (dest == sub_42C880_addr)
		//	injector::MakeCALL(addr, sub_42C880, true);

	}

	pattern = hook::pattern("89 45 18 8B 5D FC DB 45 18 D8 0D");
	struct sub_42BBA0Hook
	{
		void operator()(injector::reg_pack& regs)
		{
			*(uintptr_t*)(regs.ebp + 0x18) = regs.eax;
			regs.ebx = *(uintptr_t*)(regs.ebp - 0x4);

			*(float*)(regs.esi + 0x10) = ((*(float*)(regs.esi + 0x10)) / Screen.fHudScaleX) + (Screen.fHudOffset * (Screen.fWidth / 640.0f));
			*(float*)(regs.esi + 0x18) = ((*(float*)(regs.esi + 0x18)) / Screen.fHudScaleX) + (Screen.fHudOffset * (Screen.fWidth / 640.0f));
		}
	}; injector::MakeInline<sub_42BBA0Hook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(6)); //0x42BC7B, 0x42BC7B + 6

	//FMV
	pattern = hook::pattern("D9 5C 24 04 DB 45 10 D9 1C 24 6A 00 6A 00 E8 ? ? ? ? 83 C4 14");
	injector::MakeCALL(pattern.get(0).get<uint32_t>(14), sub_42B900, true); //0x486EC9

	//FOV
	pattern = hook::pattern("8D 4D 9C 8D 55 DC 51 52 8B CE");
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
	}; injector::MakeInline<FOVHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(6)); //0x4AF5EE, 0x4AF5EE + 6

	return 0;
	//injector::MakeNOP(0x402232, 2, true); //debug menu
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
#ifdef _LOG
		logfile.open("CF.WidescreenFix.log");
#endif // _LOG
		Init(NULL);
	}
	return TRUE;
}
