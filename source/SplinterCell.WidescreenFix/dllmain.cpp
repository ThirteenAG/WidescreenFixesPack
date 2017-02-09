#include "stdafx.h"
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
	float fHudOffset;
	float fHudOffsetRight;
	float fFMVoffsetStartX;
	float fFMVoffsetEndX;
	float fFMVoffsetStartY;
	float fFMVoffsetEndY;
} Screen;

uint32_t nForceShadowBufferMode, nFMVWidescreenMode;
bool bHudWidescreenMode, bOpsatWidescreenMode;
float fWidescreenHudOffset;

HMODULE D3DDrv, WinDrv, Engine;

//#define _LOG
#ifdef _LOG
#include <fstream>
ofstream logfile;
#endif // _LOG

uint32_t logit;
bool isIngameText;
void WidescreenHud(float& offsetX1, float& offsetX2, float& offsetY1, float& offsetY2, FColor& Color)
{
	uint32_t n_offsetX1 = static_cast<uint32_t>((480.0f * (Screen.fWidth / Screen.fHeight)) / (Screen.fWidth / offsetX1));
	uint32_t n_offsetX2 = static_cast<uint32_t>((480.0f * (Screen.fWidth / Screen.fHeight)) / (Screen.fWidth / offsetX2));
	uint32_t n_offsetY1 = static_cast<uint32_t>((480.0f * (Screen.fWidth / Screen.fHeight)) / (Screen.fWidth / offsetY1));
	uint32_t n_offsetY2 = static_cast<uint32_t>((480.0f * (Screen.fWidth / Screen.fHeight)) / (Screen.fWidth / offsetY2));

#ifdef _LOG
	if (logit)
		logfile << n_offsetX1 << " " << n_offsetX2 << " " << n_offsetY1 << " " << n_offsetY2 << " " << Color.RGBA << std::endl;
#endif // _LOG

	if (
	(n_offsetX1 >= 599 && n_offsetX2 <= 616 && n_offsetY1 >= 39 && n_offsetY2 <= 233 /*&& (Color.RGBA == 4269834368 || Color.RGBA == 671088640)*/) || //health bar
	(n_offsetX1 >= 520 && n_offsetX2 <= 616 && n_offsetY1 == 304 && (n_offsetY2 == 331 || n_offsetY2 == 332) && (Color.RGBA == 4269834368))    || //hud icons
	(n_offsetX1 >= 494 && n_offsetX2 <= 616 && n_offsetY1 >= 334 && n_offsetY2 <= 410 && (Color.RGBA == 4264768307 || Color.RGBA == 1283489920 || Color.RGBA == 4269834368 || Color.RGBA == 4265623616 || Color.RGBA == 3237936894 || Color.RGBA == 1275068416 || (Color.RGBA == 2550136832 && n_offsetX1 != 573) || Color.RGBA == 2558558336 || (Color.RGBA == 4278124286 && (n_offsetX1 == 525 || n_offsetX1 == 499)))) || //stealth bar and weapon hud
	(n_offsetX1 >= 520 && n_offsetX2 <= 616 && n_offsetY1 >= 412 && n_offsetY2 <= 440 && Color.RGBA != 4264768307) //fire mode switch
	)
	{
		offsetX1 += fWidescreenHudOffset;
		offsetX2 += fWidescreenHudOffset;
	}
	//else
	//{
	//	if (
	//	(n_offsetX1 == 29 && n_offsetX2 == 356 && n_offsetY1 >= 40 && n_offsetY2 <= 160 && (Color.RGBA == 1275068416 || Color.RGBA == 4265623616 || Color.RGBA == 4278124286)) || //top dialogue menu background
	//	((n_offsetX1 == 23 || n_offsetX1 == 29 || n_offsetX1 == 32 || n_offsetX1 == 354 || n_offsetX1 == 356) && (n_offsetX2 == 29 || n_offsetX2 == 30 || n_offsetX2 == 32 || n_offsetX2 == 354 || n_offsetX2 == 356 || n_offsetX2 == 362) && n_offsetY1 >= 39 && n_offsetY2 <= 160 /*&& Color.RGBA == 4266682200*/) || //top dialogue menu background border
	//	(n_offsetX1 >= 29 && n_offsetX2 <= 356 && n_offsetY1 >= 40 && n_offsetY2 <= 160 && (Color.RGBA == 4265623616)) //|| //top dialogue menu icon
	//	//(n_offsetX1 >= 29 && n_offsetX2 <= 356 && (n_offsetY2 - n_offsetY1 == 15))
	//	)
	//	{
	//		if (n_offsetX1 >= 29 && n_offsetX2 <= 356 && n_offsetY1 >= 40 && n_offsetY2 <= 160 && (Color.RGBA == 4265623616))
	//			isIngameText = true;
	//		else
	//			isIngameText = false;
	//
	//		offsetX1 -= fWidescreenHudOffset;
	//		offsetX2 -= fWidescreenHudOffset;
	//	}
	//}

	/*if (n_offsetX1 >= 29 && n_offsetX2 <= 356 && n_offsetY1 >= 40 && n_offsetY2 <= 160 && (Color.RGBA != 4265623616) && isIngameText)
	{
		offsetX1 -= fWidescreenHudOffset;
		offsetX2 -= fWidescreenHudOffset;
	}*/

	if ((n_offsetX1 == 179 || n_offsetX1 == 180 || n_offsetX1 == 183) && (n_offsetX2 == 608 || n_offsetX2 == 611)) //inventory fix
	{
		offsetX2 += fWidescreenHudOffset;
	}
}

void __fastcall FCanvasUtilDrawTileHook(void* _this, uint32_t EDX, float X, float Y, float SizeX, float SizeY, float U, float V, float SizeU, float SizeV, float unk1, void* Texture, FColor Color)
{
	static auto DrawTile = (void(__fastcall *)(void* _this, uint32_t EDX, float, float, float, float, float, float, float, float, float, void*, FColor)) (uint32_t)GetProcAddress(Engine, "?DrawTile@FCanvasUtil@@QAEXMMMMMMMMMPAVUMaterial@@VFColor@@@Z"); //0x10305F01;
	static FColor ColBlack; ColBlack.RGBA = 0xFF000000;

	/*if (Color.RGBA == 0xfe000000 && X == 0.0f && static_cast<uint32_t>(SizeX) == static_cast<uint32_t>(Screen.fWidth - Screen.fHudOffset - Screen.fHudOffset) && //cutscene borders
	((Y == 0.0f && static_cast<uint32_t>(SizeY) <= static_cast<uint32_t>((Screen.fWidth * (3.0f / 4.0f)) / (480.0f / 60.0f))) ||
	(static_cast<uint32_t>(Y) <= static_cast<uint32_t>((Screen.fWidth * (3.0f / 4.0f)) / (480.0f / (421.0f))) && static_cast<uint32_t>(SizeY) == static_cast<uint32_t>(Screen.fHeight))))
	{
		return;
	}*/

	if (
	(X == 0.0f && static_cast<uint32_t>(SizeX) == static_cast<uint32_t>((Screen.fHeight * (4.0f / 3.0f)) / (640.0f / 188.0f))) || //zoom scope borders
	(X == 0.0f && static_cast<uint32_t>(SizeX) == static_cast<uint32_t>((Screen.fHeight * (4.0f / 3.0f)) / (640.0f / 43.0f ))) || //sticky camera borders
	(X == 0.0f && static_cast<uint32_t>(SizeX) == static_cast<uint32_t>((Screen.fHeight * (4.0f / 3.0f)) / (640.0f / 30.0f ))) || //optic cable
	(X == 0.0f && static_cast<uint32_t>(SizeX) == static_cast<uint32_t>((Screen.fHeight * (4.0f / 3.0f)) / (640.0f / 163.0f)))    //optic mic
	) 
	{
		DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, nullptr, ColBlack);
		DrawTile(_this, EDX, Screen.fWidth - Screen.fHudOffset, Y, Screen.fWidth, SizeY, U, V, SizeU, SizeV, unk1, nullptr, ColBlack);
	}
	
	if (X == 0.0f && static_cast<uint32_t>(SizeX) == static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f)))
	{
		if ((Color.R == 0x40 && Color.G == 0x00 && Color.B == 0x00) || (Color.R == 0x80 && Color.G == 0x80 && Color.B == 0x80)) //mission failed red screen
		{
			DrawTile(_this, EDX, X, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color);
			return;
		}
		else
		{
			DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack);
			DrawTile(_this, EDX, SizeX + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack);
		}
	}

	if (bHudWidescreenMode)
		WidescreenHud(X, SizeX, Y, SizeY, Color);

	X += Screen.fHudOffset;
	SizeX += Screen.fHudOffset;

	return DrawTile(_this, EDX, X, Y, SizeX, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color);
}

DWORD WINAPI Thread(LPVOID)
{
	while (true)
	{
		Sleep(0);
		D3DDrv = GetModuleHandle("D3DDrv");
		WinDrv = GetModuleHandle("WinDrv");
		Engine = GetModuleHandle("Engine");
		if (D3DDrv && WinDrv && Engine)
			break;
	}

	auto pattern = hook::module_pattern(D3DDrv, "C7 05 ? ? ? ? 01 00 00 00"); //0x1000CE5E
	static uint32_t* dword_1003DBA4 = *pattern.get(1).get<uint32_t*>(-4);
	static uint32_t* dword_1003DBA0 = dword_1003DBA4 - 1;
	static uint32_t* dword_1003DBC0 = *pattern.get(1).get<uint32_t*>(2);
	struct UD3DRenderDevice_SetRes_Hook
	{
		void operator()(injector::reg_pack&)
		{
			*dword_1003DBA0 = Screen.Width;
			*dword_1003DBA4 = Screen.Height;
			*dword_1003DBC0 = 1;
		}
	}; injector::MakeInline<UD3DRenderDevice_SetRes_Hook>(pattern.get(1).get<uint32_t>(0), pattern.get(1).get<uint32_t>(10));

	//HUD
	Screen.fHudOffset = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
	Screen.HUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
	pattern = hook::module_pattern(Engine, "D8 C9 D8 0D ? ? ? ?");
	injector::WriteMemory<float>(*pattern.get(0).get<uint32_t*>(4), Screen.HUDScaleX, true); //(DWORD)Engine + 0x1E9F78

	uint32_t pfDrawTile = injector::ReadMemory<uint32_t>((uint32_t)GetProcAddress(Engine, "?DrawTile@UCanvas@@UAEXPAVUMaterial@@MMMMMMMMMVFPlane@@1@Z") + 0x1, true) + (uint32_t)GetProcAddress(Engine, "?DrawTile@UCanvas@@UAEXPAVUMaterial@@MMMMMMMMMVFPlane@@1@Z") + 5;
	auto DrawTile = hook::range_pattern(pfDrawTile, pfDrawTile + 0x800, "8B ? 94 00 00 00 E8");
	pattern = hook::module_pattern(Engine, "8B ? 94 00 00 00 52 E8");

	injector::MakeCALL(DrawTile.get(0).get<uint32_t>(6), FCanvasUtilDrawTileHook, true); //(uint32_t)Engine + 0xC8ECE
	injector::MakeCALL(pattern.get(0).get<uint32_t>(7), FCanvasUtilDrawTileHook, true); //(uint32_t)Engine + 0xC9B7C
	injector::MakeCALL(pattern.get(1).get<uint32_t>(7), FCanvasUtilDrawTileHook, true); //(uint32_t)Engine + 0xC9DE1

	//FMV
	Screen.fFMVoffsetStartX = (Screen.fWidth - 640.0f) / 2.0f;
	Screen.fFMVoffsetStartY = (Screen.fHeight - 480.0f) / 2.0f;
	pattern = hook::module_pattern(D3DDrv, "DA ? ? 00 00 00 D8 0D");
	injector::MakeNOP(pattern.get(0).get<uint32_t>(0), 6, true);
	injector::MakeNOP(pattern.get(1).get<uint32_t>(0), 6, true);
	injector::WriteMemory<float>(*pattern.get(0).get<uint32_t*>(8), Screen.fFMVoffsetStartX * (1.0f / 640.0f), true); //(DWORD)D3DDrv + 0x31860
	injector::WriteMemory<float>(*pattern.get(1).get<uint32_t*>(8), Screen.fFMVoffsetStartY * (1.0f / 480.0f), true); //(DWORD)D3DDrv + 0x31864

	//FOV
	fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(fScreenFieldOfViewVStd * 0.5f)) * Screen.fAspectRatio)) * (1.0f / SCREEN_FOV_HORIZONTAL);
	//uint32_t pfDraw = injector::ReadMemory<uint32_t>((uint32_t)GetProcAddress(Engine, "?Draw@UGameEngine@@UAEXPAVUViewport@@HPAEPAH@Z") + 0x1, true) + (DWORD)GetProcAddress(Engine, "?Draw@UGameEngine@@UAEXPAVUViewport@@HPAEPAH@Z") + 5;

	pattern = hook::module_pattern(Engine, "8B 46 34 8B 88 B0 02 00 00");
	struct UGameEngine_Draw_Hook
	{
		void operator()(injector::reg_pack& regs)
		{
			*(float*)&regs.ecx = *(float*)(regs.eax + 0x2B0) * fDynamicScreenFieldOfViewScale;
		}
	}; injector::MakeInline<UGameEngine_Draw_Hook>(pattern.get(0).get<uint32_t>(3)/*pfDraw + 0x104*/, pattern.get(0).get<uint32_t>(3 + 6));

	fWidescreenHudOffset = ((Screen.fHeight * (4.0f / 3.0f)) / (640.0f / fWidescreenHudOffset));
	if (Screen.fAspectRatio < (16.0f / 9.0f))
	{
		fWidescreenHudOffset = fWidescreenHudOffset / (((16.0f / 9.0f) / (Screen.fAspectRatio)) * 1.5f);
	}

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
			nForceShadowBufferMode = iniReader.ReadInteger("MAIN", "ForceShadowBufferMode", 1);
			nFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 0);
			bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) == 1;
			bOpsatWidescreenMode = iniReader.ReadInteger("MAIN", "OpsatWidescreenMode", 1) == 1;
			fWidescreenHudOffset = iniReader.ReadFloat("MAIN", "WidescreenHudOffset", 120.0f);

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

			char UserIni[MAX_PATH];
			HMODULE hModule = GetModuleHandle(NULL);
			if (hModule != NULL)
			{
				GetModuleFileName(hModule, UserIni, (sizeof(UserIni)));
				char* pch = strrchr(UserIni, '\\');
				pch[0] = '\0';
				strcat(UserIni, "\\SplinterCellUser.ini");
			}

			CIniReader iniWriter(UserIni);
			char szRes[50];
			sprintf(szRes, "%dx%d", Screen.Width, Screen.Height);
			iniWriter.WriteString("Engine.EPCGameOptions", "Resolution", szRes);

			char* pch = strrchr(UserIni, '\\');
			pch[0] = '\0';
			strcat(UserIni, "\\SplinterCell.ini");
			CIniReader iniWriter2(UserIni);
			iniWriter2.WriteInteger("WinDrv.WindowsClient", "WindowedViewportX", Screen.Width);
			iniWriter2.WriteInteger("WinDrv.WindowsClient", "WindowedViewportY", Screen.Height);
			iniWriter2.WriteInteger("D3DDrv.D3DRenderDevice", "ForceShadowMode", nForceShadowBufferMode);
			
			// return to the original EP
			*(uint32_t*)ep = *(uint32_t*)&originalCode;
			*(uint8_t*)(ep + 4) = originalCode[4];
			*(uintptr_t*)(regs.esp) = (uintptr_t)ep;
		}
	}; injector::MakeInline<EP>(ep);

	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Thread, NULL, 0, NULL);
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
#ifdef _LOG
		logfile.open("SC.WidescreenFix.log");
#endif // _LOG
		Init();
	}
	return TRUE;
}