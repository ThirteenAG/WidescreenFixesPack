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
#define SCREEN_FOV_HORIZONTAL		75.0f
#define SCREEN_FOV_VERTICAL			(2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / SCREEN_AR_NARROW)))	// Default is 75.0f.
float fScreenFieldOfViewVStd = SCREEN_FOV_VERTICAL;
float fDynamicScreenFieldOfViewScale;

union FColor
{
	uint32_t RGBA;
	struct
	{
		uint8_t B, G, R, A;
	};
};

struct Screen
{
	int Width;
	int Height;
	float fWidth;
	float fHeight;
	float fFieldOfView;
	float fAspectRatio;
	float HUDScaleX;
	float TextScaleX;
	float fHudOffset;
	int32_t nHudOffsetReal;
	float fFMVoffsetStartX;
	float fFMVoffsetEndX;
	float fFMVoffsetStartY;
	float fFMVoffsetEndY;
} Screen;

uint32_t nFMVWidescreenMode;
bool bHudWidescreenMode, bOpsatWidescreenMode;
float fWidescreenHudOffset;

#define _LOG
#ifdef _LOG
#include <fstream>
ofstream logfile;
#endif // _LOG

uint32_t logit;
void WidescreenHud(float& offsetX1, float& offsetX2, float& offsetY1, float& offsetY2, FColor& Color)
{
#ifdef _LOG
	if (logit)
		logfile << offsetX1 << " " << offsetX2 << " " << offsetY1 << " " << offsetY2 << " " << Color.RGBA << std::endl;
#endif // _LOG
}


DWORD WINAPI Thread(LPVOID)
{
	auto pattern = hook::pattern("89 6B 58 89 7B 5C EB 0E");
	struct GetRes
	{
		void operator()(injector::reg_pack& regs)
		{
			*(uint32_t*)(regs.ebx + 0x58) = regs.ebp;
			*(uint32_t*)(regs.ebx + 0x5C) = regs.edi;
			Screen.Width = regs.ebp;
			Screen.Height = regs.edi;
			Screen.fWidth = static_cast<float>(Screen.Width);
			Screen.fHeight = static_cast<float>(Screen.Height);
			Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

			Screen.HUDScaleX = (1.0f / Screen.fWidth * (Screen.fHeight / 480.0f)) * 2.0f;
			injector::WriteMemory(0x1120B6BC, Screen.HUDScaleX, true);
			Screen.TextScaleX = ((4.0f / 3.0f) / Screen.fAspectRatio) * 2.0f;
			Screen.fHudOffset = Screen.TextScaleX / 2.0f;
			Screen.nHudOffsetReal = static_cast<int32_t>(((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f);

			fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(fScreenFieldOfViewVStd * 0.5f)) * Screen.fAspectRatio)) * (1.0f / SCREEN_FOV_HORIZONTAL);

			//FMV
			//Screen.fFMVoffsetStartX = ((Screen.fHeight * Screen.fAspectRatio) - (Screen.fHeight * (4.0f / 3.0f))) / 2.0f;
			//injector::WriteMemory(0x10C863D6 + 0x4, Screen.fFMVoffsetStartX, true);
			//actually it scales perfectly as is.
		}
	}; injector::MakeInline<GetRes>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(6)); //<0x10CC622C, 0x10CC6232>

	//HUD
	pattern = hook::pattern("0F BF 90 86 00 00 00");
	struct HudHook
	{
		void operator()(injector::reg_pack& regs)
		{
			injector::WriteMemory(0x1120B6BC, Screen.HUDScaleX, true);
			*(uint32_t*)(regs.esp + 0x8) = *(uint16_t*)(regs.eax + 0x86);
		}
	}; injector::MakeInline<HudHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(11)); //<0x10ADB0BC, 0x10ADB0C7>

	pattern = hook::pattern("D8 25 ? ? ? ? D9 05 ? ? ? ? D8 88");
	injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &Screen.fHudOffset, true); //0x10ADADBE + 0x2

	/////////////////////
	struct Test
	{
		void operator()(injector::reg_pack& regs)
		{
			*(uint32_t*)0x1120B6B0 = regs.esi;

			int32_t offset1 = *(int32_t*)(regs.esp + 0x10);
			int32_t offset2 = *(int32_t*)(regs.esp + 0x14);
			int32_t offset3 = *(int32_t*)(regs.esp + 0x28);
			FColor Color; Color.RGBA = *(int32_t*)(regs.esp + 0x40);

#ifdef _LOG
			if (logit && offset3 > 200)
				logfile << std::dec << offset1 << " " << offset2 << " " << offset3 << " " << std::hex << Color.RGBA << std::endl;
#endif // _LOG

			if (((offset1 == -2 && offset2 == 257) || (offset1 == -2 && offset2 == 258) || (offset1 == -61 && offset2 == 380))) //scopes image left
			{
				*(int32_t*)(regs.esp + 0x10) -= (Screen.nHudOffsetReal);
			}
			else if (((offset1 == 319 && offset2 == 380) || (offset1 == 382 && offset2 == 258) || (offset1 == 383 && offset2 == 257) || (offset1 == 383 && offset2 == 258))) //scopes image right
			{
				*(int32_t*)(regs.esp + 0x10) += (Screen.nHudOffsetReal);
			}

			if (true)
			{
				if (
				((offset1 == 421 || offset1 == 424 || offset1 == 617 || offset1 == 622) && (offset2 == 20 || offset2 == 1) && (offset3 == 26 || offset3 == 45) && Color.RGBA == 0x32ffffff) || //health bar brackets
				((offset1 == 427|| offset1 == 618) && (offset2 == 1 || offset2 == 10) && (offset3 == 30 || offset3 == 40) && Color.RGBA == 0x59ffffff) ||                                      //health bar
				((offset1 == 428|| offset1 == 618) && (offset2 == 1 || offset2 == 10) && offset3 == 40 && (Color.RGBA == 0x59ffffff || Color.RGBA == 0x80ffffff)) ||                           //health bar
				((offset1 == 431|| offset1 == 618) && (offset2 == 3 || offset2 == 5) && (offset3 == 36 || offset3 == 37) && (Color.RGBA == 0x32ffffff || Color.RGBA == 0x96ffffff)) ||         //health bar
				(true)
				) 
				{
					*(int32_t*)(regs.esp + 0x10) += 100;
				}

			}
		}
	}; injector::MakeInline<Test>(0x10ADADAE, 0x10ADADAE + 6);
	/////////////////////

	//TEXT
	pattern = hook::pattern("D8 3D ? ? ? ? D9 5C 24 68 DB");
	injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &Screen.TextScaleX, true); //0x10B149CE + 0x2
	pattern = hook::pattern("D8 25 ? ? ? ? D9 44 24 24 D8 4C 24");
	injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &Screen.fHudOffset, true); //0x10B14BAD + 0x2

	//FOV
	pattern = hook::pattern("8B 91 BC 02 00 00 52 8B 54 24 24");
	struct UGameEngine_Draw_Hook
	{
		void operator()(injector::reg_pack& regs)
		{
			*(float*)&regs.edx = *(float*)(regs.ecx + 0x2BC) * fDynamicScreenFieldOfViewScale;
		}
	}; injector::MakeInline<UGameEngine_Draw_Hook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(6)); //0x10A3E67F

	pattern = hook::pattern("8B 88 BC 02 00 00 8B 44 24 20 51");
	struct UGameEngine_Draw_Hook2
	{
		void operator()(injector::reg_pack& regs)
		{
			*(float*)&regs.ecx = *(float*)(regs.eax + 0x2BC) * fDynamicScreenFieldOfViewScale;
		}
	}; injector::MakeInline<UGameEngine_Draw_Hook2>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(6)); //0x10A3E8A0

	return 0;
}

void Init()
{
	HINSTANCE hExecutableInstance = GetModuleHandle(NULL);
	IMAGE_NT_HEADERS* ntHeader = (IMAGE_NT_HEADERS*)((uint32_t)hExecutableInstance + ((IMAGE_DOS_HEADER*)hExecutableInstance)->e_lfanew);
	static uint8_t* ep = (uint8_t*)((uint32_t)hExecutableInstance + ntHeader->OptionalHeader.AddressOfEntryPoint);
	static uint8_t originalCode[5];
	*(uint32_t*)&originalCode = *(uint32_t*)ep;
	originalCode[4] = *(ep + 4);

	struct EP
	{
		void operator()(injector::reg_pack& regs)
		{
			CIniReader iniReader("");
			Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
			Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
			nFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 0);
			bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) == 1;
			bOpsatWidescreenMode = iniReader.ReadInteger("MAIN", "OpsatWidescreenMode", 1) == 1;
			fWidescreenHudOffset = iniReader.ReadFloat("MAIN", "WidescreenHudOffset", 140.0f);

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

			auto pattern = hook::pattern("8D 84 24 34 04 00 00 68 ? ? ? ? 50 E8 ? ? ? ? 83 C4 14"); //0x10CD09C5
			struct SetRes_Hook
			{
				void operator()(injector::reg_pack& regs)
				{
					regs.eax = (regs.esp + 0x434); // lea eax, [esp+434h]

					char pszPath[MAX_PATH];
					wcstombs(pszPath, (const wchar_t*)regs.edi, wcslen((const wchar_t*)regs.edi));
					pszPath[wcslen((const wchar_t*)regs.edi)] = '\0';
					char ResX[20];
					char ResY[20];
					_snprintf(ResX, 20, "%d", Screen.Width);
					_snprintf(ResY, 20, "%d", Screen.Height);
					WritePrivateProfileString("WinDrv.WindowsClient", "WindowedViewportX", ResX, pszPath);
					WritePrivateProfileString("WinDrv.WindowsClient", "WindowedViewportY", ResY, pszPath);
					WritePrivateProfileString("WinDrv.WindowsClient", "FullscreenViewportX", ResX, pszPath);
					WritePrivateProfileString("WinDrv.WindowsClient", "FullscreenViewportY", ResY, pszPath);
				}
			}; injector::MakeInline<SetRes_Hook>(pattern.get(0).get<uintptr_t>(0), pattern.get(0).get<uintptr_t>(7));
			
			// return to the original EP
			*(uint32_t*)ep = *(uint32_t*)&originalCode;
			*(uint8_t*)(ep + 4) = originalCode[4];
			*(uintptr_t*)(regs.esp - 4) = (uintptr_t)ep;
		}
	}; injector::MakeInline<EP>(ep);

	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Thread, NULL, 0, NULL);
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
#ifdef _LOG
		logfile.open("SC2.WidescreenFix.log");
#endif // _LOG
		Init();
	}
	return TRUE;
}