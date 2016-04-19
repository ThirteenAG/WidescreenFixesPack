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

bool bHudWidescreenMode;
int32_t nWidescreenHudOffset;
float fWidescreenHudOffset;
struct WidescreenHudOffset
{
	int32_t _int;
	float _float;
} WidescreenHudOffset;

//#define _LOG
#ifdef _LOG
#include <fstream>
ofstream logfile;
uint32_t logit;
#endif // _LOG

DWORD WINAPI Thread(LPVOID)
{
	auto pattern = hook::pattern("89 6B 58 89 7B 5C EB 0E");
	static auto dword_1120B6BC = *hook::pattern("D9 1D ? ? ? ? 0F BF").get(0).get<uint32_t*>(2);
	static auto dword_1120B6B0 = *hook::pattern("8B 0D ? ? ? ? 85 C9 74 ? 8B 54 24 04 8B 82").get(0).get<uint32_t*>(2);
	static auto dword_11223A7C = *hook::pattern("A1 ? ? ? ? 83 C4 04 85 C0 D8 3D ? ? ? ?").get(0).get<uint32_t*>(1);
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
			injector::WriteMemory(dword_1120B6BC, Screen.HUDScaleX, true);
			Screen.TextScaleX = ((4.0f / 3.0f) / Screen.fAspectRatio) * 2.0f;
			Screen.fHudOffset = Screen.TextScaleX / 2.0f;
			Screen.nHudOffsetReal = static_cast<int32_t>(((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f);

			if (Screen.fAspectRatio < (16.0f / 9.0f))
			{
				WidescreenHudOffset._float = fWidescreenHudOffset / (((16.0f / 9.0f) / (Screen.fAspectRatio)) * 1.5f);
				WidescreenHudOffset._int = static_cast<int32_t>(WidescreenHudOffset._float);
			}
			else
			{
				WidescreenHudOffset._int = nWidescreenHudOffset;
				WidescreenHudOffset._float = fWidescreenHudOffset;
			}

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
			injector::WriteMemory(dword_1120B6BC, Screen.HUDScaleX, true);
			regs.edx = *(uint16_t*)(regs.eax + 0x86);
		}
	}; injector::MakeInline<HudHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(7)); //<0x10ADB0BC>

	pattern = hook::pattern("D8 25 ? ? ? ? D9 05 ? ? ? ? D8 88");
	injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &Screen.fHudOffset, true); //0x10ADADBE + 0x2

	pattern = hook::pattern("89 35 ? ? ? ? DB 44 24 10 D8 0D ? ? ? ? D8 25");
	struct WSHud
	{
		void operator()(injector::reg_pack& regs)
		{
			*(uint32_t*)dword_1120B6B0 = regs.esi;

			int32_t offset1 = *(int32_t*)(regs.esp + 0x10);
			int32_t offset2 = *(int32_t*)(regs.esp + 0x14);
			int32_t offset3 = *(int32_t*)(regs.esp + 0x28);
			FColor Color; Color.RGBA = *(int32_t*)(regs.esp + 0x40);

#ifdef _LOG
			//if (logit /*&& offset3 > 45 && offset3 < 200*/)
			//	logfile << std::dec << offset1 << " " << offset2 << " " << offset3 << " " << std::hex << Color.RGBA << std::endl;
#endif // _LOG

			if (offset1 == 0 && offset2 == 480 && offset3 == 480) //mission failed fading
				*(int32_t*)(regs.esp + 0x10) += 10000;

			if (((offset1 == -2 && offset2 == 257) || (offset1 == -2 && offset2 == 258) || (offset1 == -61 && offset2 == 380))) //scopes image left
			{
				*(int32_t*)(regs.esp + 0x10) -= Screen.nHudOffsetReal;
			}
			else if (((offset1 == 319 && offset2 == 380) || (offset1 == 382 && offset2 == 258) || (offset1 == 383 && offset2 == 257) || (offset1 == 383 && offset2 == 258))) //scopes image right
			{
				*(int32_t*)(regs.esp + 0x10) += Screen.nHudOffsetReal;
			}

			if (bHudWidescreenMode)
			{
				if (
				((offset1 == 421 || offset1 == 424 || offset1 == 617 || offset1 == 622) && (offset2 == 20 || offset2 == 1) && (offset3 == 26 || offset3 == 45) && Color.RGBA == 0x32ffffff) || //health bar brackets
				((offset1 == 427|| offset1 == 618) && (offset2 == 1 || offset2 == 10) && (offset3 == 30 || offset3 == 40) && Color.RGBA == 0x59ffffff) ||                                      //health bar
				((offset1 == 428|| offset1 == 618) && (offset2 == 1 || offset2 == 10) && offset3 == 40 && (Color.RGBA == 0x59ffffff || Color.RGBA == 0x80ffffff)) ||                           //health bar
				((offset1 == 431|| offset1 == 618) && (offset2 == 3 || offset2 == 5) && (offset3 == 36 || offset3 == 37) && (Color.RGBA == 0x32ffffff || Color.RGBA == 0x96ffffff)) ||         //health bar
				((offset1 >= 478 && offset1 <= 610) && offset2 == 4 && offset3 == 420 && Color.RGBA == 0xffffffff) || //bottom right panel borders
				((offset1 == 510 || offset1 == 545 || offset1 == 580) && offset2 == 6 && offset3 == 405 && Color.RGBA == 0xffffffff) || //bottom right panel borders
				((offset1 == 467 || offset1 == 470 || offset1 == 477 || offset1 == 617 || offset1 == 622) && (offset2 == 1 || offset2 == 6 || offset2 == 75) && (offset3 == 391 || offset3 == 421 || offset3 == 465) && Color.RGBA == 0x32ffffff) || //brackets and sound meter (bottom right panel)
				((offset1 == 473 || offset1 == 618) && (offset2 == 1 || offset2 == 12 || offset2 == 32) && (offset3 == 396 || offset3 == 408 || offset3 == 409 || offset3 == 412 || offset3 == 424 || offset3 == 425 || offset3 == 428 || offset3 == 460 || offset3 == 461) && Color.RGBA == 0x59ffffff) || //green borders (bottom right panel)
				(offset1 == 474 && (offset2 == 11 || offset2 == 12 || offset2 == 31) && (offset3 == 424 || offset3 == 408 || offset3 == 460) && Color.RGBA == 0x80ffffff) || //backgrounds (bottom right panel)
				(((offset1 == 475 && offset2 == 20 && offset3 == 391) || (offset1 == 492 && offset2 == 32 && offset3 == 396)) && (Color.R == 0xb8 && Color.G == 0xf7 && Color.B == 0xc8)) || //target icon / notebook icon
				((offset1 == 476 || offset1 == 608) && offset2 == 16 && offset3 == 445 && Color.RGBA == 0x4bb8fac8) || //weapon name brackets
				((offset1 == 476 || offset1 == 557) && offset2 == 16 && offset3 == 461 && Color.RGBA == 0x40ffffff) || //ammo brackets
				((offset1 == 476 || offset1 == 527 || offset1 == 567 || offset1 == 568 || offset1 == 608) && offset2 == 16 && (offset3 == 445 || offset3 == 461) && Color.RGBA == 0x4bb8fac8) || //secondary ammo brackets
				((offset1 >= 476 && offset1 <= 620) && offset2 == 6 && offset3 == 405 && (Color.R == 0xff && Color.G == 0xff && Color.B == 0xff)) || //visibility slider
				((offset1 >= 476 && offset1 <= 620) && (offset2 == 1 || offset2 == 6 || offset2 == 32) && (offset3 == 397 || offset3 == 416 || offset3 == 421) && Color.RGBA == 0xc8ffffff) || //alarm icon
				((offset1 == 573) && (offset2 == 6 || offset2 == 8) && (offset3 == 455 || offset3 == 456) && (Color.RGBA == 0x80ffffff || Color.RGBA == 0xffffffff)) || //pistol jammer bar

				((offset1 == 421 || offset1 == 424 || offset1 == 617 || offset1 == 622) && (offset2 == 1 || offset2 == 5 || offset2 == 9 || offset2 == 16 || offset2 == 21) && (offset3 > 45 && offset3 < 200) && Color.RGBA == 0x32ffffff) || //interaction menu brackets
				((offset1 == 427 || offset1 == 428 || offset1 == 618) && (offset2 == 1 || offset2 == 5 || offset2 == 15 || offset2 == 16) && (offset3 > 45 && offset3 < 200) && Color.RGBA == 0x59ffffff) ||  //interaction menu 
				((offset1 == 428 || offset1 == 618) && (offset2 == 1 || offset2 == 4 || offset2 == 5 || offset2 == 14 || offset2 == 16) && (offset3 > 45 && offset3 < 200) && (Color.RGBA == 0x59ffffff || Color.RGBA == 0x99ffffff)) || //interaction menu 
				((offset1 == 429 || offset1 == 430 || offset1 == 434 || offset1 == 607 || offset1 == 608 || offset1 == 617 || offset1 == 618 || offset1 == 622) && (offset2 == 1 || offset2 == 9 || offset2 == 15 || offset2 == 16 || offset2 == 21) && (offset3 > 45 && offset3 < 200) && (Color.RGBA == 0x32ffffff || Color.RGBA == 0xc8ffffff)) //|| //interaction menu 
				) 
				{
					*(int32_t*)(regs.esp + 0x10) += WidescreenHudOffset._int;
				}
			}
		}
	}; injector::MakeInline<WSHud>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(6)); //0x10ADADAE, 0x10ADADAE + 6

	//TEXT
	pattern = hook::pattern("D8 3D ? ? ? ? D9 5C 24 68 DB");
	injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &Screen.TextScaleX, true); //0x10B149CE + 0x2
	pattern = hook::pattern("D8 25 ? ? ? ? D9 44 24 24 D8 4C 24");
	injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &Screen.fHudOffset, true); //0x10B14BAD + 0x2

	pattern = hook::pattern("A1 ? ? ? ? 83 C4 04 85 C0 D8 3D");
	struct WSText
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.eax = *(uint32_t*)dword_11223A7C;

			int32_t offset1 = *(int32_t*)(regs.esp + 0x4);
			int32_t offset2 = *(int32_t*)(regs.esp + 0xC);
			int32_t offset3 = static_cast<int32_t>(*(float*)(regs.esp + 0x1C));
			FColor Color; Color.RGBA = *(int32_t*)(regs.esp + 0x160);

#ifdef _LOG
			if (logit)
				logfile << std::dec << offset1 << " " << offset2 << " " << offset3 << " " << std::hex << Color.RGBA << std::endl;
#endif // _LOG*/

			if (
			((offset1 == 435 || offset1 == 436) && (offset2 >= 3  && offset2 <= 20) && (offset3 == 345 || offset3 == 361 || offset3 == 377 || offset3 == 393 || offset3 == 416) && ((Color.R == 0xff && Color.G == 0xff && Color.B == 0xff) || (Color.R == 0xb8 && Color.G == 0xfa && Color.B == 0xc8) || (Color.R == 0x66 && Color.G == 0x66 && Color.B == 0x66))) || // top corner
			((offset1 >= 489 && offset1 <= 598) && (offset2 == 1 || offset2 == 3 || (offset2 >= 7 && offset2 <= 20 )) && (offset3 == 23 || offset3 == 39 || offset3 == 93) && ((Color.R == 0xff && Color.G == 0xff && Color.B == 0xff) || (Color.R == 0xb8 && Color.G == 0xfa && Color.B == 0xc8))) || // bottom corner
			(offset1 == 598 && offset2 == 3 && offset3 == 93 && (Color.R == 0xb8 && Color.G == 0xf7 && Color.B == 0xc8)) //icons text
			)
			{
				*(float*)(regs.esp + 0x14) += WidescreenHudOffset._float;
			}

		}
	}; injector::MakeInline<WSText>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(5)); //0x10B149C4, 0x10B149C4 + 5

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
			bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) == 1;
			nWidescreenHudOffset = iniReader.ReadInteger("MAIN", "WidescreenHudOffset", 100);
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