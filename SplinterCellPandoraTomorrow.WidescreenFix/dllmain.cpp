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
	uint8_t R;
	uint8_t G;
	uint8_t B;
	uint8_t A;
	uint32_t RGBA;
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

uint32_t nForceShadowBufferMode, nFMVWidescreenMode, nPostProcessFixedScale;
bool bHudWidescreenMode, bOpsatWidescreenMode;
float fWidescreenHudOffset;

HMODULE D3DDrv, WinDrv, Engine;
uint32_t DisplayVideo_HookJmp;

//#define _LOG
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

	if (
	((offsetX2 - offsetX1 == 17) && (offsetY1 == 337.0f || offsetY1 == 360.0f) && (offsetY2 == 368.0f || offsetY2 == 345.0f) /*&& Color.RGBA == 4266682200*/) || //stealth meter line 
	((offsetX2 - offsetX1 == 109) && offsetY1 == 338.0f && offsetY2 == 368.0f && (Color.RGBA == 4266682200 || Color.RGBA == 4271286934)) || //stealth meter
	(offsetX1 == 491.0f && offsetX2 == 592.0f && offsetY1 == 373.0f && offsetY2 == 441.0f && Color.RGBA == 4266682200) || //weapon icon background
	((offsetX1 == 487.0f || offsetX1 == 503.0f || offsetX1 == 580.0f) && (offsetX2 == 503.0f || offsetX2 == 580.0f || offsetX2 == 596.0f) && (offsetY1 == 369.0f || offsetY1 == 385.0f || offsetY1 == 429.0f) && Color.RGBA == 2152752984) || //weapon icon background border
	(((offsetX2 - offsetX1 == 8) || (offsetX2 - offsetX1 == 11)) && offsetY1 == 316.0f && offsetY2 == 330.0f && Color.RGBA == 4265759816) || //alarm counter digits text
	(((offsetX2 - offsetX1 == 3) || (offsetX2 - offsetX1 == 7)) && offsetY1 == 315.0f && offsetY2 == 330.0f && Color.RGBA == 4265759816) || //alarm counter digits text (russian)
	((offsetX1 == 570.0f || offsetX1 == 586.0f) && (offsetX2 == 586.0f || offsetX2 == 602.0f) /*&& (offsetY1 == 125.0f || offsetY1 == 141.0f || offsetY1 == 269.0f)*/ /*&& (offsetY2 == 141.0f || offsetY2 == 269.0f || offsetY2 == 285.0f)*/ && Color.RGBA == 3360712536) || //health bar
	(offsetX1 == 492.0f && offsetX2 == 519.0f && offsetY1 == 314.0f && offsetY2 == 333.0f && Color.RGBA == 4266682200) || //envelope icon
	(offsetX1 == 521.0f && offsetX2 == 562.0f && offsetY1 == 306.0f && offsetY2 == 340.0f && (Color.RGBA == 4266682200 || Color.RGBA == 2152752984)) || //alarm icon
	((offsetX1 == 494.0f || offsetX1 == 515.0f) && offsetY1 == 377.0f /*&& offsetY2 == 423.0f*/ && Color.RGBA == 4266682200) || //weapon icon
	(offsetX1 >= 491.0f && /*offsetX2 == 519.0f &&*/ offsetY1 == 423.0f && offsetY2 == 437.0f /*&& Color.RGBA == 4265759816*/) || //weapon name text
	(offsetX1 >= 491.0f && /*offsetX2 == 519.0f &&*/ offsetY1 == 423.0f && offsetY2 == 438.0f /*&& Color.RGBA == 4265759816*/) || //weapon name text (russian)
	(offsetX1 >= 571.0f && /*offsetX2 == 519.0f &&*/ offsetY1 == 373.0f && offsetY2 == 385.0f && Color.RGBA == 4265759816) || //ammo text
	(offsetX1 >= 571.0f && /*offsetX2 == 519.0f &&*/ offsetY1 == 373.0f && offsetY2 == 388.0f && Color.RGBA == 4265759816) || //ammo text (russian)
	(offsetX1 == 581.0f && (offsetX2 == 588.0f || offsetX2 == 592.0f) /*&& (Color.RGBA == 4266682200 || Color.RGBA == 4265759816)*/) ||  //bullets image
	(offsetX1 == 568.0f /*&& (offsetX2 == 588.0f || offsetX2 == 592.0f)*/&& offsetY1 == 406.0f /*&& (Color.RGBA == 4266682200 /*|| Color.RGBA == 4265759816)*/) ||  //another bullet image
	(offsetX1 >= 536.0f && offsetX2 <= 576.0f && offsetY1 == 416.0f && offsetY2 == 421.0f /*&& Color.RGBA == 0xFE000000*/) || //rate of fire image
	(offsetX1 == 441.0f && offsetX2 == 481.0f && offsetY1 == 373.0f && offsetY2 == 413.0f && Color.RGBA == 4266682200) || //weapon addon icon background
	(offsetX1 == 439.0f && offsetX2 == 483.0f && offsetY1 == 380.0f && offsetY2 == 412.0f && Color.RGBA == 4266682200) || //weapon addon icon
	((offsetX1 == 437.0f || offsetX1 == 453.0f || offsetX1 == 469.0f) && (offsetX2 == 453.0f || offsetX2 == 469.0f || offsetX2 == 485.0f) && (offsetY1 == 369.0f || offsetY1 == 385.0f || offsetY1 == 401.0f) && Color.RGBA == 2152752984) || //weapon addon icon background border
	((offsetX1 == 456.0f || offsetX1 == 457.0f) && (offsetX2 == 467.0f || offsetX2 == 465.0f) && offsetY1 == 371.0f && offsetY2 == 383.0f && Color.RGBA == 4265759816) || //weapon addon ammo text
	((offsetX1 == 456.0f || offsetX1 == 457.0f) && (offsetX2 == 467.0f || offsetX2 == 464.0f) && offsetY1 == 371.0f && offsetY2 == 386.0f && Color.RGBA == 4265759816) || //weapon addon ammo text (russian)
	(offsetX1 == 362.0f && offsetX2 == 562.0f /*&& offsetY1 == 42.0f && offsetY2 == 66.0f*/ && (Color.RGBA == 4266682200 || Color.RGBA == 4269316740)) || //interaction menu background
	(offsetX1 == 443.0f && offsetX2 == 562.0f /*&& offsetY1 == 42.0f && offsetY2 == 66.0f*/ && (Color.RGBA == 4266682200 || Color.RGBA == 4269316740)) || //interaction menu background (russian)
	((offsetX1 == 359.0f || offsetX1 == 375.0f || offsetX1 == 549.0f || offsetX1 == 554.0f) && (offsetX2 == 375.0f || offsetX2 == 370.0f || offsetX2 == 549.0f || offsetX2 == 565.0f) && (Color.RGBA == 2152752984 || Color.RGBA == 4266682200)) || //interaction menu background border
	((offsetX1 == 440.0f || offsetX1 == 456.0f || offsetX1 == 537.0f || offsetX1 == 554.0f) && (offsetX2 == 456.0f || offsetX2 == 451.0f || offsetX2 == 549.0f || offsetX2 == 565.0f) && (Color.RGBA == 2152752984 || Color.RGBA == 4266682200)) || //interaction menu background border (russian)
	(offsetX1 >= 365.0f && offsetY1 >= 42.0f && offsetY2 <= 192.0f && (Color.RGBA == 4265759816 || Color.RGBA == 4269316740)) //interaction menu text
	)
	{
		offsetX1 += fWidescreenHudOffset;
		offsetX2 += fWidescreenHudOffset;
	}
	else
	{
		if (
		(offsetX1 == 48.0f && offsetX2 == 348.0f && offsetY1 >= 42.0f && offsetY2 <= 87.0f && Color.RGBA == 4266682200) || //top dialogue menu background
		(offsetX1 == 48.0f && offsetX2 == 348.0f && offsetY1 >= 42.0f && offsetY2 <= 90.0f && Color.RGBA == 4266682200) || //top dialogue menu background (russian)
		((offsetX1 == 45.0f || offsetX1 == 61.0f || offsetX1 == 335.0f ) && (offsetX2 == 61.0f || offsetX2 == 335.0f || offsetX2 == 351.0f) && Color.RGBA == 4266682200) || //top dialogue menu background border
		(offsetX1 >= 52.0f && offsetY1 >= 43.0f && offsetY2 <= 185.0f && Color.RGBA == 4265759816) //top dialogue menu text
		)
		{
			offsetX1 -= fWidescreenHudOffset;
			offsetX2 -= fWidescreenHudOffset;
		}
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

void __fastcall FCanvasUtilDrawTileHook(void* _this, uint32_t EDX, float X, float Y, float SizeX, float SizeY, float U, float V, float SizeU, float SizeV, float unk1, void* Texture, FColor Color, uint32_t unk3, uint32_t unk4)
{
	static auto DrawTile = (void(__fastcall *)(void* _this, uint32_t EDX, float, float, float, float, float, float, float, float, float, void*, FColor, uint32_t, uint32_t)) (uint32_t)GetProcAddress(Engine, "?DrawTile@FCanvasUtil@@QAEXMMMMMMMMMPAVUMaterial@@VFColor@@HH@Z"); //0x103D2DA0;
	FColor ColBlack; ColBlack.RGBA = 0xFF000000;

	if (X == 0.0f && SizeX == 188.0f) //zoom scope 1
	{
		DrawTile(_this, EDX, X, Y, SizeX, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
		return;
	}

	if (X == (640.0f - 188.0f) && SizeX == 640.0f) //zoom scope 2
	{
		DrawTile(_this, EDX, SizeX + Screen.fHudOffset + Screen.fHudOffset - 188.0f, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
		return;
	}

	if ((X == 0.0f || X == 256.0f || X == 384.0f) && Y == 448.0f && (SizeX == 256.0f || SizeX == 384.0f || SizeX == 640.0f) && SizeY == 479.0f) //hiding menu background
	{
		//DrawTile(_this, EDX, 0.0f, 0.0f, 640.0f + Screen.fHudOffset + Screen.fHudOffset, Y, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4); hides all menu overlay graphics
		DrawTile(_this, EDX, 0.0f, 0.0f, 640.0f + Screen.fHudOffset + Screen.fHudOffset, 30.0f, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
		DrawTile(_this, EDX, 0.0f, 447.0f, 640.0f + Screen.fHudOffset + Screen.fHudOffset, 480.0f, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
		return;
	}

	if (X == 0.0f && SizeX == 64.0f) //sony ericsson overlay
	{
		if (bOpsatWidescreenMode)
		{
			DrawTile(_this, EDX, X, Y, SizeX, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
			return;
		}
		else
		{
			DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset + 64.0f, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
		}
	}
	if (X == 64.0f && SizeX == 320.0f) //sony ericsson overlay
	{
		if (bOpsatWidescreenMode)
		{
			DrawTile(_this, EDX, X, Y, SizeX + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
			return;
		}
	}
	if (X == 320.0f && SizeX == 576.0f) //sony ericsson overlay
	{
		if (bOpsatWidescreenMode)
		{
			DrawTile(_this, EDX, X + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
			return;
		}
	}

	if (X == 576.0f && SizeX == 640.0f) //sony ericsson overlay
	{
		if (bOpsatWidescreenMode)
		{
			DrawTile(_this, EDX, X + Screen.fHudOffset + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
			return;
		}
		else
		{
			DrawTile(_this, EDX, 576.0f + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
		}
	}

	if (X == 0.0f && SizeX == 640.0f)
	{
		if (Color.R == 0xFE && Color.G == 0xFE && Color.B == 0xFE) //flashbang grenade flash
		{
			DrawTile(_this, EDX, X, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
			return;
		}
		else
		{
			DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
			DrawTile(_this, EDX, SizeX + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
		}
	}

	if (bHudWidescreenMode)
		WidescreenHud(X, SizeX, Y, SizeY, Color);

	X += Screen.fHudOffset;
	SizeX += Screen.fHudOffset;

	return DrawTile(_this, EDX, X, Y, SizeX, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
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
	injector::MakeCALL(pattern.get(0).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfFUCanvasDrawTile + 0x219

	uint32_t pfexecDrawTextClipped = (uint32_t)GetProcAddress(Engine, "?execDrawTextClipped@UCanvas@@QAEXAAUFFrame@@QAX@Z");
	pattern = hook::range_pattern(pfexecDrawTextClipped, pfexecDrawTextClipped + 0x400, "E8 ? ? ? ?");
	uint32_t pfsub_103762F0 = injector::ReadMemory<uint32_t>((uint32_t)pattern.get(3).get<uint32_t>(1), true) + (uint32_t)pattern.get(3).get<uint32_t>(5); //pfexecDrawTextClipped + 0xF4 + 0x1 / pfexecDrawTextClipped + 0xF4 + 5

	pattern = hook::range_pattern(pfsub_103762F0, pfsub_103762F0 + 0x800, "E8 ? ? ? ? 8B ?");
	injector::MakeCALL(pattern.get(3).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x36E
	injector::MakeCALL(pattern.get(5).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x43D
	injector::MakeCALL(pattern.get(7).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x4DA
	injector::MakeCALL(pattern.get(9).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x564

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

	if (nPostProcessFixedScale)
	{
		if (nPostProcessFixedScale == 1)
			nPostProcessFixedScale = Screen.Width;

		//uint32_t pfSetRes = injector::ReadMemory<uint32_t>((uint32_t)GetProcAddress(D3DDrv, "?SetRes@UD3DRenderDevice@@UAEHPAVUViewport@@HHH@Z") + 0x1, true) + (uint32_t)GetProcAddress(D3DDrv, "?SetRes@UD3DRenderDevice@@UAEHPAVUViewport@@HHH@Z") + 5;
		//auto pattern = hook::range_pattern(pfSetRes, pfSetRes + 0x1A8C, "68 00 02 00 00 68 00 02 00 00");
		auto pattern = hook::module_pattern(D3DDrv, "68 00 02 00 00 68 00 02 00 00");
		for (size_t i = 0; i < pattern.size(); i++)
		{
			injector::WriteMemory(pattern.get(i).get<uint32_t>(1), nPostProcessFixedScale, true);
			injector::WriteMemory(pattern.get(i).get<uint32_t>(6), nPostProcessFixedScale, true);
		}

		auto pattern2 = hook::module_pattern(Engine, "68 00 02 00 00 68 00 02 00 00 ?");
		for (size_t i = 0; i < pattern2.size(); i++)
		{
			injector::WriteMemory(pattern2.get(i).get<uint32_t>(1), nPostProcessFixedScale, true);
			injector::WriteMemory(pattern2.get(i).get<uint32_t>(6), nPostProcessFixedScale, true);
		}
	}

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
			fWidescreenHudOffset = iniReader.ReadFloat("MAIN", "WidescreenHudOffset", 140.0f);
			nPostProcessFixedScale = iniReader.ReadInteger("MAIN", "PostProcessFixedScale", 1);

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

			pch = strrchr(UserIni, '\\');
			pch[0] = '\0';
			strcat(UserIni, "\\SplinterCell.ini");
			CIniReader iniWriter3(UserIni);
			iniWriter3.WriteInteger("D3DDrv.D3DRenderDevice", "ForceShadowMode", nForceShadowBufferMode);
		
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