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

uint32_t __esp, FCanvasUtilDrawTileHookJmp, DisplayVideo_HookJmp;
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

void __declspec(naked) DisplayVideo_Hook()
{
	static uint32_t __ECX;
	_asm
	{
		fstp    dword ptr[esp]
		push    esi
		push    esi
		mov		__ECX, ecx
		mov ecx, nFMVWidescreenMode
		cmp ecx, 0
	jz label1
		mov ecx, Screen.fFMVoffsetStartY
		mov[esp + 4h], ecx;
		mov ecx, Screen.fFMVoffsetEndY
		mov[esp + 0xC], ecx;
	jmp label2
	label1:
		mov ecx, Screen.fFMVoffsetStartX
		mov[esp + 0h], ecx;
		mov ecx, Screen.fFMVoffsetEndX
		mov[esp + 8h], ecx;
	label2:
		mov	 ecx, __ECX
		jmp	 DisplayVideo_HookJmp
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

	//DWORD pfSetRes = injector::ReadMemory<DWORD>((DWORD)GetProcAddress(D3DDrv, "?SetRes@UD3DRenderDevice@@UAEHPAVUViewport@@HHH@Z") + 0x1, true) + (DWORD)GetProcAddress(D3DDrv, "?SetRes@UD3DRenderDevice@@UAEHPAVUViewport@@HHH@Z") + 5;
	/*dword_10173E5C = injector::ReadMemory<DWORD>(pfSetRes + 0x435 + 0x1, true);
	injector::MakeJMP(pfSetRes + 0x435, UD3DRenderDevice_SetRes_Hook, true);
	hookJmpAddr = pfSetRes + 0x435 + 0x5;

	DWORD pfResizeViewport = (DWORD)GetProcAddress(WinDrv, "?ResizeViewport@UWindowsViewport@@UAEHKHH@Z");
	injector::MakeJMP(pfResizeViewport + 0x469, UWindowsViewport_ResizeViewport_Hook, true); //crash on FMV
	hookJmpAddr2 = pfResizeViewport + 0x469 + 0x6;*/

	Screen.fHudOffset = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f / (Screen.fWidth / (640.0f * (4.0f / 3.0f)));
	Screen.HUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);

	uint32_t pfDrawTile = (uint32_t)GetProcAddress(Engine, "?DrawTile@FCanvasUtil@@QAEXMMMMMMMMMPAVUMaterial@@VFColor@@HH@Z");
	auto pattern = hook::range_pattern(pfDrawTile, pfDrawTile + 0x100, "DC 0D");
	injector::WriteMemory<double>(*pattern.get(0).get<uint32_t*>(2), Screen.HUDScaleX, true);

	uint32_t pfFUCanvasDrawTile = (uint32_t)GetProcAddress(Engine, "?DrawTile@UCanvas@@UAEXPAVUMaterial@@MMMMMMMMMVFPlane@@1H@Z");
	pattern = hook::range_pattern(pfFUCanvasDrawTile, pfFUCanvasDrawTile + 0x400, "E8 ? ? ? ? 8B");
	injector::MakeCALL(pattern.get(0).get<uint32_t>(0), FCanvasUtil_DrawTile_Hook, true); //pfFUCanvasDrawTile + 0x219

	uint32_t pfexecDrawTextClipped = (uint32_t)GetProcAddress(Engine, "?execDrawTextClipped@UCanvas@@QAEXAAUFFrame@@QAX@Z");
	pattern = hook::range_pattern(pfexecDrawTextClipped, pfexecDrawTextClipped + 0x400, "E8 ? ? ? ?");
	uint32_t pfsub_103762F0 = injector::ReadMemory<uint32_t>((uint32_t)pattern.get(3).get<uint32_t>(1), true) + (uint32_t)pattern.get(3).get<uint32_t>(5); //pfexecDrawTextClipped + 0xF4 + 0x1 / pfexecDrawTextClipped + 0xF4 + 5

	pattern = hook::range_pattern(pfsub_103762F0, pfsub_103762F0 + 0x800, "E8 ? ? ? ? 8B ?");
	injector::MakeCALL(pattern.get(3).get<uint32_t>(0), FCanvasUtil_DrawTile_Hook, true); //pfsub_103762F0 + 0x36E
	injector::MakeCALL(pattern.get(5).get<uint32_t>(0), FCanvasUtil_DrawTile_Hook, true); //pfsub_103762F0 + 0x43D
	injector::MakeCALL(pattern.get(7).get<uint32_t>(0), FCanvasUtil_DrawTile_Hook, true); //pfsub_103762F0 + 0x4DA
	injector::MakeCALL(pattern.get(9).get<uint32_t>(0), FCanvasUtil_DrawTile_Hook, true); //pfsub_103762F0 + 0x564
	FCanvasUtilDrawTileHookJmp = (uint32_t)GetProcAddress(Engine, "?DrawTile@FCanvasUtil@@QAEXMMMMMMMMMPAVUMaterial@@VFColor@@HH@Z");


	//FMV
	Screen.fFMVoffsetStartX = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
	Screen.fFMVoffsetEndX = Screen.fWidth - Screen.fFMVoffsetStartX;
	Screen.fFMVoffsetStartY = 0.0f - ((Screen.fHeight - ((Screen.fHeight / 1.5f) * ((16.0f / 9.0f) / Screen.fAspectRatio))) / 2.0f);
	Screen.fFMVoffsetEndY = Screen.fHeight - Screen.fFMVoffsetStartY;

	uint32_t pfOpenVideo = injector::ReadMemory<uint32_t>((uint32_t)GetProcAddress(D3DDrv, "?OpenVideo@UD3DRenderDevice@@UAEXPAVUCanvas@@PADHHH@Z") + 0x1, true) + (uint32_t)GetProcAddress(D3DDrv, "?OpenVideo@UD3DRenderDevice@@UAEXPAVUCanvas@@PADHHH@Z") + 5;
	uint32_t pfDisplayVideo = injector::ReadMemory<uint32_t>((uint32_t)GetProcAddress(D3DDrv, "?DisplayVideo@UD3DRenderDevice@@UAEXPAVUCanvas@@PAX@Z") + 0x1, true) + (uint32_t)GetProcAddress(D3DDrv, "?DisplayVideo@UD3DRenderDevice@@UAEXPAVUCanvas@@PAX@Z") + 5;
	//////injector::WriteMemory<float>(injector::ReadMemory<uint32_t>((uint32_t)GetProcAddress(D3DDrv, "?DisplayVideo@UD3DRenderDevice@@UAEXPAVUCanvas@@PAX@Z") + 0x55D, true), 0.0f, true); //Y 
	//////injector::WriteMemory<float>(injector::ReadMemory<uint32_t>(pfDisplayVideo + 0x332 + 0x2, true), 0.003125f, true); //X
	
	pattern = hook::range_pattern(pfOpenVideo, pfOpenVideo + 0x400, "C1 ? 02");
	struct OpenVideo_Hook
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.ecx = 0x4B000;
			regs.ecx >>= 2;
			regs.eax = 0;
		}
	}; injector::MakeInline<OpenVideo_Hook>(pattern.get(1).get<uint32_t>(0)); //pfOpenVideo + 0x2D4
	
	pattern = hook::range_pattern(pfDisplayVideo, pfDisplayVideo + 0x500, "D9 ? 24");
	injector::MakeJMP(pattern.get(5).get<uint32_t>(0), DisplayVideo_Hook, true);//pfDisplayVideo + 0x37E
	DisplayVideo_HookJmp = (uint32_t)pattern.get(5).get<uint32_t>(5);

	//FOV
	fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(fScreenFieldOfViewVStd * 0.5f)) * Screen.fAspectRatio)) * (1.0f / SCREEN_FOV_HORIZONTAL);
	
	uint32_t pfDraw = (uint32_t)GetProcAddress(Engine, "?Draw@UGameEngine@@UAEXPAVUViewport@@HPAEPAH@Z");
	pattern = hook::range_pattern(pfDraw, pfDraw + 0x300, "8B ? ? 03 00 00");
	struct UGameEngine_Draw_Hook
	{
		void operator()(injector::reg_pack& regs)
		{
			*(float*)&regs.ecx = *(float*)(regs.eax + 0x374) * fDynamicScreenFieldOfViewScale;
		}
	}; injector::MakeInline<UGameEngine_Draw_Hook>(pattern.get(0).get<uint32_t>(0)/*pfDraw + 0x167*/, pattern.get(0).get<uint32_t>(0 + 6));

	//Shadows
	if (nForceShadowBufferSupport)
	{
		uint32_t pfSupportsShadowBuffer = injector::ReadMemory<uint32_t>((uint32_t)GetProcAddress(D3DDrv, "?SupportsShadowBuffer@UD3DRenderDevice@@QAEHXZ") + 0x1, true) + (uint32_t)GetProcAddress(D3DDrv, "?SupportsShadowBuffer@UD3DRenderDevice@@QAEHXZ") + 5;
		pattern = hook::range_pattern(pfSupportsShadowBuffer, pfSupportsShadowBuffer + 0x90, "0F 84");
		injector::WriteMemory<unsigned short>(pattern.get(0).get<uint32_t>(0), 0xE990, true); //pfSupportsShadowBuffer + 0x10
		pattern = hook::range_pattern(pfSupportsShadowBuffer, pfSupportsShadowBuffer + 0x200, "A1 ? ? ? ? 5E");
		injector::WriteMemory(pattern.get(0).get<uint32_t>(1), &nForceShadowBufferSupport, true); //pfSupportsShadowBuffer + 0x113
		//injector::WriteMemory(pfSetRes + 0xA45 + 0x6, nForceShadowBufferSupport, true);
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
				strcat(UserIni, "\\SplinterCell2User.ini");
			}

			CIniReader iniWriter(UserIni);
			char szRes[50];
			sprintf(szRes, "%dx%d", Screen.Width, Screen.Height);
			iniWriter.WriteString("Engine.EPCGameOptions", "Resolution", szRes);

			char* pch = strrchr(UserIni, '\\');
			pch[0] = '\0';
			strcat(UserIni, "\\SplinterCell2.ini");
			CIniReader iniWriter2(UserIni);
			iniWriter2.WriteInteger("WinDrv.WindowsClient", "WindowedViewportX", Screen.Width);
			iniWriter2.WriteInteger("WinDrv.WindowsClient", "WindowedViewportY", Screen.Height);

			uint32_t appInit = (uint32_t)GetProcAddress(GetModuleHandle("Core"), "?appInit@@YAXPBG0PAVFMalloc@@PAVFOutputDevice@@PAVFOutputDeviceError@@PAVFFeedbackContext@@PAVFFileManager@@P6APAVFConfigCache@@XZH@Z");
			uint32_t pfappInit = injector::ReadMemory<uint32_t>(appInit + 0x1, true) + appInit + 5;

			auto pattern = hook::range_pattern(pfappInit, pfappInit + 0x900, "80 02 00 00");
			injector::WriteMemory<unsigned short>(pattern.get(0).get<uint32_t>(-6), 0x7EEB, true); //pfappInit + 0x5FC
			injector::WriteMemory(pattern.get(1).get<uint32_t>(0), Screen.Width, true);  //pfappInit + 0x67E + 0x1
			pattern = hook::range_pattern(pfappInit, pfappInit + 0x900, "E0 01 00 00");
			injector::WriteMemory(pattern.get(1).get<uint32_t>(0), Screen.Height, true); //pfappInit + 0x69F + 0x1
			
			// return to the original EP
			*(uint32_t*)ep = *(uint32_t*)&originalCode;
			*(uint8_t*)(ep + 4) = originalCode[4];
			//*(uintptr_t*)regs.esp = (uintptr_t)ep;
			__asm jmp ep //PT crashes for some reason 
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