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
uint32_t nForceShadowBufferSupport, nFMVWidescreenMode;

HMODULE D3DDrv, WinDrv, Engine;

uint32_t __esp, FCanvasUtilDrawTileHookJmp;
float offset1, offset2;
uint32_t Color;
void CenterHud()
{
	offset1 = *(float*)(__esp + 4);
	offset2 = *(float*)(__esp + 4 + 8);
	Color = *(uint32_t*)(__esp + 0x2C);

	offset1 += Screen.fHudOffset;
	offset2 += Screen.fHudOffset;

	if (Color == 0xFE000000)
	{
		offset2 = 0.0f; // hiding cutscene borders
		offset1 = 0.0f;
	}
}

void __declspec(naked) FCanvasUtil_DrawTile_Hook()
{
	_asm
	{
		mov  __esp, esp
		call CenterHud
		mov  eax, offset1
		mov  [esp+4], eax;
		mov  eax, offset2
		mov  [esp + 4 + 8], eax;
		jmp	 FCanvasUtilDrawTileHookJmp
	}
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

	uint32_t* dword_1110AB88 = hook::module_pattern(WinDrv, "83 E1 DF").get(0).get<uint32_t>(0); //0x1110AB88
	uint32_t dword_1110AB96 = 0;
	static uint8_t reg = 0;
	for (size_t i = 0; i < 40; i++)
	{
		if (*(uint8_t*)((uint32_t)dword_1110AB88 + i) == 0x80)
		{
			dword_1110AB96 = ((uint32_t)dword_1110AB88 + i - 2);
			reg = *(uint8_t*)(dword_1110AB96 + 1);
			break;
		}
	}

	struct UWindowsViewport_ResizeViewport_Hook
	{
		void operator()(injector::reg_pack& regs)
		{
			*(uint32_t*)((reg == 0x9D ? regs.ebp : regs.esi) + 0x7C) = Screen.Width;
			*(uint32_t*)((reg == 0x9D ? regs.ebp : regs.esi) + 0x80) = Screen.Height;
		}
	}; injector::MakeInline<UWindowsViewport_ResizeViewport_Hook>(dword_1110AB96, dword_1110AB96 + 6);

	//HUD
	Screen.fHudOffset = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
	Screen.HUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
	pattern = hook::module_pattern(Engine, "D8 C9 D8 0D ? ? ? ?");
	injector::WriteMemory<float>(*pattern.get(0).get<uint32_t*>(4), Screen.HUDScaleX, true); //(DWORD)Engine + 0x1E9F78

	pattern = hook::module_pattern(Engine, "8B ? 94 00 00 00 E8");
	uint32_t pfDrawTile = injector::ReadMemory<uint32_t>((uint32_t)GetProcAddress(Engine, "?DrawTile@UCanvas@@UAEXPAVUMaterial@@MMMMMMMMMVFPlane@@1@Z") + 0x1, true) + (uint32_t)GetProcAddress(Engine, "?DrawTile@UCanvas@@UAEXPAVUMaterial@@MMMMMMMMMVFPlane@@1@Z") + 5;
	static uint32_t* DrawTile = 0;
	for (size_t i = 0; i < pattern.size(); i++)
	{
		DrawTile = pattern.get(i).get<uint32_t>(0);
		if (DrawTile < (uint32_t*)pfDrawTile)
			continue;
		else
			break;
	}

	FCanvasUtilDrawTileHookJmp = injector::ReadMemory<uint32_t>((uint32_t)GetProcAddress(Engine, "?DrawTile@FCanvasUtil@@QAEXMMMMMMMMMPAVUMaterial@@VFColor@@@Z") + 0x1, true) + (uint32_t)GetProcAddress(Engine, "?DrawTile@FCanvasUtil@@QAEXMMMMMMMMMPAVUMaterial@@VFColor@@@Z") + 5; //(DWORD)Engine + 0x157200
	pattern = hook::module_pattern(Engine, "8B ? 94 00 00 00 52 E8");

	injector::MakeCALL((uint32_t)DrawTile + 6, FCanvasUtil_DrawTile_Hook, true); //(uint32_t)Engine + 0xC8ECE
	injector::MakeCALL(pattern.get(0).get<uint32_t>(7), FCanvasUtil_DrawTile_Hook, true); //(uint32_t)Engine + 0xC9B7C
	injector::MakeCALL(pattern.get(1).get<uint32_t>(7), FCanvasUtil_DrawTile_Hook, true); //(uint32_t)Engine + 0xC9DE1

	//FMV
	pattern = hook::module_pattern(D3DDrv, "DA ? ? 00 00 00 D8 0D");
	injector::WriteMemory<float>(*pattern.get(0).get<uint32_t*>(8), (1.0f / 640.0f) / ((Screen.fWidth / 640.0f)), true);  //X   //(DWORD)D3DDrv + 0x31860
	injector::WriteMemory<float>(*pattern.get(1).get<uint32_t*>(8), (1.0f / 480.0f) / ((Screen.fHeight / 480.0f)), true); //Y   //(DWORD)D3DDrv + 0x31864

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
	}; injector::MakeInline<UGameEngine_Draw_Hook>(pattern.get(0).get<uint32_t>(3)/*pfDraw + 0x104*/, pattern.get(0).get<uint32_t>(3+6));

	//Shadows
	if (nForceShadowBufferSupport)
	{
		uint32_t pfSupportsShadowBuffer = (uint32_t)GetProcAddress(D3DDrv, "?SupportsShadowBuffer@UD3DRenderDevice@@UAEHXZ");
		injector::WriteMemory<unsigned short>(pfSupportsShadowBuffer + 0x10, 0xE990, true);
		injector::WriteMemory(pfSupportsShadowBuffer + 0x113, &nForceShadowBufferSupport, true);
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
			nForceShadowBufferSupport = iniReader.ReadInteger("MAIN", "ForceShadowBufferSupport", 0);
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

			// return to the original EP
			*(uint32_t*)ep = *(uint32_t*)&originalCode;
			*(uint8_t*)(ep + 4) = originalCode[4];
			*(uintptr_t*)regs.esp = (uintptr_t)ep;
		}
	}; injector::MakeInline<EP>(ep);

	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Thread, NULL, 0, NULL);
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}