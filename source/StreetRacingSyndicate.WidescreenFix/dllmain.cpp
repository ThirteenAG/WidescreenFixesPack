#include "stdafx.h"
#define _USE_MATH_DEFINES
#include "math.h"

HWND hWnd;
bool bDelay;

struct Screen
{
	int Width;
	int Height;
	float fWidth;
	float fHeight;
	float fFieldOfView;
	float fAspectRatio;
	int Width43;
	float fWidth43;
	float fHudScale;
} Screen;

//#define _LOG
#ifdef _LOG
#include <fstream>
std::ofstream logfile;
int logit;
#endif // _LOG

DWORD WINAPI Init(LPVOID)
{
	auto pattern = hook::pattern("6A FF 51 FF D6");
	if (!(pattern.size() > 0) && !bDelay)
	{
		bDelay = true;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
		return 0;
	}

	if (bDelay)
	{
		while (!(pattern.size() > 0))
			pattern = hook::pattern("6A FF 51 FF D6");
	}

	CIniReader iniReader("");
	Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
	Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
	bool bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) != 0;
	bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;

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
	Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
	Screen.fWidth43 = static_cast<float>(Screen.Width43);
	
	pattern = hook::pattern("C7 47 14 ? ? ? ? C7 47 18 ? ? ? ?"); //0x4C81A3 0x4C81AA 
	injector::WriteMemory<float>(pattern.get(0).get<uint32_t>(3), Screen.fWidth, true);
	injector::WriteMemory<float>(pattern.get(0).get<uint32_t>(10), Screen.fHeight, true);

	pattern = hook::pattern("89 4F 14 D8 0D ? ? ? ? D9 5F 18"); //0x4C8235
	struct SetResHook
	{
		void operator()(injector::reg_pack& regs)
		{
			float temp = 0.0f;
			_asm fstp    dword ptr[temp]

			*(float*)(regs.edi + 0x14) = Screen.fWidth;
			*(float*)(regs.edi + 0x18) = Screen.fHeight;
		}
	}; injector::MakeInline<SetResHook>(pattern.get(0).get<uint32_t>(3), pattern.get(0).get<uint32_t>(12));

	pattern = hook::pattern("84 DB 74 09 DB 87 ? ? ? ? D9 5F 14"); //0x4C8241
	injector::MakeNOP(pattern.get(0).get<uint32_t>(0), 13, true);

	//3D
	static float f3DScale = 1.0f / (480.0f * Screen.fAspectRatio);
	pattern = hook::pattern("D8 0D ? ? ? ? D9 5C 24 04 D9 46"); //0x4BC66B
	injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &f3DScale, true);

	pattern = hook::pattern("C7 05 ? ? ? ? 00 00 20 44"); //0x553DFC //mov 
	for (size_t i = 0; i < pattern.size(); ++i)
	{
		injector::WriteMemory<float>(pattern.get(i).get<uint32_t>(6), (480.0f * Screen.fAspectRatio), true);
	}

	pattern = hook::pattern("68 00 00 F0 43 68 00 00 20 44"); //0x4CA7E6 //push
	for (size_t i = 0; i < pattern.size(); ++i)
	{
		injector::WriteMemory<float>(pattern.get(i).get<uint32_t>(6), (480.0f * Screen.fAspectRatio), true);
	}


	if (bFixHUD)
	{
		Screen.fHudScale = Screen.fAspectRatio / (4.0f / 3.0f);
		pattern = hook::pattern("8B 81 ? ? ? ? D8 40 6C 5E D9 58 6C"); //0x4CB3C5
		struct HudScaleHook
		{
			void operator()(injector::reg_pack& regs)
			{
				#ifdef _LOG
				if (logit)
					logfile << *(float*)(*(uintptr_t*)(regs.ecx + 0x0A859C) + 0x00) << " "
					<< *(uintptr_t*)(*(uintptr_t*)(regs.ecx + 0x0A859C) + 0x10) << " "
					<< *(uintptr_t*)(*(uintptr_t*)(regs.ecx + 0x0A859C) + 0x14) << " "
					<< *(uintptr_t*)(*(uintptr_t*)(regs.ecx + 0x0A859C) + 0x18) << " "
					<< *(uintptr_t*)(*(uintptr_t*)(regs.ecx + 0x0A859C) + 0x1C) << std::endl;
				#endif // _LOG

				if (*(float*)(*(uintptr_t*)(regs.ecx + 0x0A859C) + 0x08) != 90.0f && *(uintptr_t*)(*(uintptr_t*)(regs.ecx + 0x0A859C) + 0x18) != 235587392) //fading check
				{
					*(float*)(*(uintptr_t*)(regs.ecx + 0x0A859C) + 0x00) /= Screen.fHudScale;
				}

				regs.eax = *(uintptr_t*)(regs.ecx + 0x0C3B40);
			}
		}; injector::MakeInline<HudScaleHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(6));

		pattern = hook::pattern("DB 44 24 08 8D 86 A8 00 00 00 8D"); //0x5288D2
		struct RearviewMirrorHook
		{
			void operator()(injector::reg_pack& regs)
			{
				uint32_t fMirrorOffset = *(uint32_t*)(regs.esp + 0x8) + (uint32_t)(((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f);
				_asm fild    dword ptr[fMirrorOffset]
				regs.eax = regs.esi + 0xA8;
			}
		}; injector::MakeInline<RearviewMirrorHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(10));

	}


	if (bFixFOV)
	{
		#define DEGREE_TO_RADIAN(fAngle) \
		((fAngle)* (float)M_PI / 180.0f)
		#define RADIAN_TO_DEGREE(fAngle) \
		((fAngle)* 180.0f / (float)M_PI)
		#define SCREEN_AR_NARROW			(4.0f / 3.0f)	// 640.0f / 480.0f
		#define SCREEN_FOV_HORIZONTAL		48.0f
		#define SCREEN_FOV_VERTICAL			(2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / SCREEN_AR_NARROW)))	// Default is 75.0f.
		float fScreenFieldOfViewVStd = SCREEN_FOV_VERTICAL;
		float fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(fScreenFieldOfViewVStd * 0.5f)) * Screen.fAspectRatio)) * (1.0f / SCREEN_FOV_HORIZONTAL);
		static float fFOVFactor = fDynamicScreenFieldOfViewScale * 0.5f;
		pattern = hook::pattern("D8 0D ? ? ? ? 8D B2 ? ? ? ? 33 C0 8B FE D9 F2"); //0x4BC4BB
		injector::WriteMemory(pattern.get(0).get<uintptr_t>(2), &fFOVFactor, true);
	}

	return 0;
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		#ifdef _LOG
		logfile.open("WidescreenFix.log");
		#endif // _LOG
		Init(NULL);
	}
	return TRUE;
}