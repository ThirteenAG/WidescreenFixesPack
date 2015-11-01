#include "..\includes\stdafx.h"

#define _USE_MATH_DEFINES
#include "math.h"
#define DEGREE_TO_RADIAN(fAngle) \
	((fAngle)* (float)M_PI / 180.0f)
#define RADIAN_TO_DEGREE(fAngle) \
	((fAngle)* 180.0f / (float)M_PI)
#define SCREEN_AR_NARROW			(4.0f / 3.0f)	// 640.0f / 480.0f
#define SCREEN_FOV_HORIZONTAL		65.0f
#define SCREEN_FOV_VERTICAL			(2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / SCREEN_AR_NARROW)))

struct Screen
{
	int nWidth;
	int nHeight;
	float fWidth;
	float fHeight;
	float fDynamicScreenFieldOfViewScale;
	float fFieldOfView;
	float fAspectRatio;
	float fHudScale;
	float fHudOffset;
	float fHudOffsetWide;
} Screen;

struct TextCoords
{
	float a;
	float b;
	float c;
	float d;
};

float f1_480 = 1.0f / 480.0f;

HMODULE e2mfc, e2_d3d8_driver_mfc;
float fFOVFactor;
bool bComicsMode, bFixHud, bWidescreenHud;
float fAspectRatio;
float fWidthScale, fHalfWidthScale, f1_fWidthScale;
float fDouble1_fWidthScale, fHalf1_fWidthScale;
uchar* IsInComicsMode;
DWORD GetScreenRectJmp;
DWORD jmpAddr, jmpAddr2, jmpAddr3, jmpAddr4;
DWORD nCounter;
float* pHudElementPosX; float* pHudElementPosY; TextCoords* pTextElementPosX;
int nCutsceneBorders;

int __fastcall PDriverGetWidth(int _this)
{
	if (*(DWORD *)(_this + 48))
		Screen.nWidth = *(DWORD *)(_this + 52);
	else
		Screen.nWidth = *(DWORD *)(_this + 4);

	return Screen.nWidth;
}

int __fastcall PDriverGetHeight(int _this)
{
	if (*(DWORD *)(_this + 48))
		Screen.nHeight = *(DWORD *)(_this + 56);
	else
		Screen.nHeight = *(DWORD *)(_this + 8);

	return Screen.nHeight;
}

float __fastcall FOVHook(int _this)
{
	Screen.fFieldOfView = *(float *)(_this + 88) * Screen.fDynamicScreenFieldOfViewScale;
	return Screen.fFieldOfView;
}

float __fastcall GetBordersSize(int _this)
{
	if (nCutsceneBorders > 1)
	{
		return *(float *)(_this + 12) * (1.0f / ((4.0f / 3.0f) / Screen.fAspectRatio));
	}
	else
	{
		return 1.0f;
	}
}

void __declspec(naked)PCameraGetScreenRectHook()
{
	__asm
	{
		inc nCounter
		mov ebx, 0C00h
		jmp GetScreenRectJmp
	}
}

void __declspec(naked) DisableSubViewport()
{
	__asm ret 10h
}

DWORD WINAPI ComicsHandler(LPVOID)
{
	CIniReader iniReader("");
	bComicsMode = iniReader.ReadInteger("MAIN", "ComicsMode", 1) != 0;
	DWORD nComicsModeKey = iniReader.ReadInteger("MAIN", "ComicsModeKey", VK_F2);
	bool bPatched;

	while (true)
	{
		Sleep(0);

		if ((GetAsyncKeyState(nComicsModeKey) & 1) && (*IsInComicsMode == 255))
		{
			bComicsMode = !bComicsMode;
			bPatched = !bPatched;
			iniReader.WriteInteger("MAIN", "ComicsMode", bComicsMode);
			while ((GetAsyncKeyState(nComicsModeKey) & 0x8000) > 0) { Sleep(0); }
		}

		if (bComicsMode)
		{
			if (*IsInComicsMode == 255 && *(uchar*)0x8A69E4) //+main menu check
			{
				if (!bPatched)
				{
					injector::WriteMemory<float>((DWORD)e2mfc + 0x49E00, ((1.0f / (480.0f * Screen.fAspectRatio)) / 2.0f), true);
					injector::WriteMemory<float>((DWORD)e2mfc + 0x49DFC, (1.0f / (640.0f / (4.0f / 3.0f)) / 2.0f), true);
					injector::WriteMemory<float>((DWORD)e2mfc + 0x15658 + 0x6, 0.0f, true);
					bPatched = true;
				}
			}
			else
			{
				if (bPatched)
				{
					injector::WriteMemory<float>((DWORD)e2mfc + 0x49E00, ((1.0f / 640.0f) / 2.0f), true);
					injector::WriteMemory<float>((DWORD)e2mfc + 0x49DFC, fHalf1_fWidthScale, true);
					injector::WriteMemory<float>((DWORD)e2mfc + 0x15658 + 0x6, 0.0f, true);
					bPatched = false;
				}
			}
		}
		else
		{
			if (*IsInComicsMode == 255 && *(uchar*)0x8A69E4) //+main menu check
			{
				if (!bPatched)
				{
					injector::WriteMemory<float>((DWORD)e2mfc + 0x49E00, ((1.0f / (480.0f * Screen.fAspectRatio)) / 2.0f) * 1.27f, true);
					injector::WriteMemory<float>((DWORD)e2mfc + 0x49DFC, (1.0f / (640.0f / (4.0f / 3.0f)) / 2.0f) * 1.27f, true);
					injector::WriteMemory<float>((DWORD)e2mfc + 0x15658 + 0x6, -0.39f, true);
					bPatched = true;
				}
			}
			else
			{
				if (bPatched)
				{
					injector::WriteMemory<float>((DWORD)e2mfc + 0x49E00, ((1.0f / 640.0f) / 2.0f), true);
					injector::WriteMemory<float>((DWORD)e2mfc + 0x49DFC, fHalf1_fWidthScale, true);
					injector::WriteMemory<float>((DWORD)e2mfc + 0x15658 + 0x6, 0.0f, true);
					bPatched = false;
				}
			}
		}
	}
	return 0;
}

float ElementPosX, ElementNewPosX1, ElementNewPosX2;
float ElementPosY, ElementNewPosY1, ElementNewPosY2;
DWORD _EAX;
float _EDX;
void __declspec(naked) P_HudPosHook()
{
	__asm mov _EAX, eax
	__asm mov _EDX, edx

	ElementPosX = *pHudElementPosX;
	ElementPosY = *pHudElementPosY;
	ElementNewPosX1 = ElementPosX;
	ElementNewPosY1 = ElementPosY;
	ElementNewPosX2 = ElementPosX;
	ElementNewPosY2 = ElementPosY;

	if (bWidescreenHud)
	{
		if (ElementPosX == 7.0f) // bullet time meter
		{
			ElementNewPosX1 = ElementPosX + Screen.fHudOffsetWide;
		}

		if (ElementPosX == 8.0f && _EAX != 8) // bullet time overlay()
		{
			ElementNewPosX1 = ElementPosX + Screen.fHudOffsetWide;
		}

		if (ElementPosX == 12.0f) // painkillers
		{
			ElementNewPosX1 = ElementPosX + Screen.fHudOffsetWide;
		}

		if (ElementPosX == 22.5f) //health bar and overlay
		{
			ElementNewPosX1 = ElementPosX + Screen.fHudOffsetWide;
		}

		if (ElementPosX == 95.0f) // other weapons name
		{
			ElementNewPosX1 = ElementPosX - Screen.fHudOffsetWide;
		}

		if (ElementPosX == 190.0f) //molotovs/grenades name pos
		{
			ElementNewPosX1 = ElementPosX - Screen.fHudOffsetWide;
		}
	}

	ElementNewPosX2 = ElementNewPosX1;

	if (ElementPosX == 0.0f && ElementPosY == 0.0f && _EAX == 2 && _EDX == 640.0f) // fading
	{
		ElementNewPosX1 = ElementPosX + 640.0f;
		ElementNewPosX2 = ElementPosX - 640.0f;					  
		//ElementNewPosY1 = ElementPosY + 48.0f;
		//ElementNewPosY2 = ElementPosY - 48.0f;
	}

	if (*IsInComicsMode == 255 && *(uchar*)0x8A69E4) //+main menu check
	{
		if (ElementPosX == 0.0f && ElementPosY == 100.0f /*&& _EAX == 2*/ /*&& _EDX == 80.0f*/) // comics controls and background
		{
			if (bComicsMode)
			{
				ElementNewPosY1 = ElementPosY - 90.0f;
				ElementNewPosY2 = ElementPosY - 90.0f;
			}
			else
			{
				ElementNewPosY1 = ElementPosY - 160.0f;
				ElementNewPosY2 = ElementPosY - 160.0f;
			}
		}

		if (ElementPosX == 0.0f && ElementPosY == 0.0f && _EAX == 2 && (_EDX == 80.0f || _EDX == 60.0f)) // comics controls and background
		{
			if (bComicsMode)
			{
				ElementNewPosY1 = ElementPosY - 60.0f;
				ElementNewPosY2 = ElementPosY - 60.0f;
			}
			else
			{
				ElementNewPosY1 = ElementPosY - 160.0f;
				ElementNewPosY2 = ElementPosY - 160.0f;
			}
		}
	}

	_asm
	{
		fld     dword ptr[ElementNewPosX1]
		fchs
		fld     dword ptr [ebp-4]
		fsub    dword ptr[ElementNewPosX2]
		fstp    dword ptr [ebp-4]
		fld     dword ptr[ElementNewPosY1]
		fchs
		fld     dword ptr [ebp-8]
		fsub    dword ptr[ElementNewPosY2]
		jmp	    jmpAddr
	}
}

float TextPosX, TextNewPosX, TextUnkVal;
void __declspec(naked) P_TextPosHook()
{

	TextPosX = pTextElementPosX->a;
	TextNewPosX = TextPosX;

	if ((pTextElementPosX->a == 0.0f || pTextElementPosX->a == -8.0f || pTextElementPosX->a == -16.0f || pTextElementPosX->a == -24.0f || pTextElementPosX->a == -32.0f) && pTextElementPosX->b == -10.5f && (pTextElementPosX->c == 8.0f || pTextElementPosX->c == 16.0f || pTextElementPosX->c == 24.0f || pTextElementPosX->c == 32.0f) && pTextElementPosX->d == 21) //ammo numbers(position depends on digits amount)
	{
		TextNewPosX = TextPosX + Screen.fHudOffsetWide;
	}

	__asm fld    dword ptr[TextNewPosX]
	__asm jmp    jmpAddr2
}

float TextPosX1, TextPosY1;
float TextPosX2;

void PainkillersText()
{
	if (TextPosX1 == (69.0f + Screen.fHudOffsetWide) && TextPosY1 == 457.0f) // painkillers amount number
	{
		TextPosX2 += (24.0f * Screen.fHudOffsetWide);
	}
}

void __declspec(naked) P_TextPosHook2()
{
	_asm
	{
		mov dword ptr[ebp - 30h], 0
		mov eax, [ebp - 28h]
		mov TextPosX1, eax
		mov eax, [ebp - 2Ch]
		mov TextPosY1, eax
		jmp jmpAddr3
	}
}

void __declspec(naked) P_TextPosHook3()
{
	_asm
	{
		mov  eax, [ebp - 1Ch]
		mov  TextPosX2, eax
		call PainkillersText
		mov  eax, TextPosX2
		mov  [ebp - 1Ch], eax
		mov  [ecx + 8], eax
		fld  dword ptr[ebp - 1Ch]
		jmp  jmpAddr4
	}
}

DWORD WINAPI Thread(LPVOID)
{
	CIniReader iniReader("");
	bool bUseGameFolderForSavegames = iniReader.ReadInteger("MAIN", "UseGameFolderForSavegames", 0) != 0;
	if (bUseGameFolderForSavegames)
		injector::WriteMemory<uchar>(0x40FCAB, 0x85, true);

	bool bAltTab = iniReader.ReadInteger("MAIN", "AllowAltTabbingWithoutPausing", 0) != 0;
	if (bAltTab)
		injector::MakeNOP(0x40D29B, 5, true);

	nCutsceneBorders = iniReader.ReadFloat("MAIN", "CutsceneBorders", 1);
	if (nCutsceneBorders)
		injector::MakeCALL(0x4565B8, GetBordersSize, true);

	fFOVFactor = iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f);
	DWORD nLoadingDelay = iniReader.ReadInteger("MAIN", "LoadingDelay", 300);
	bFixHud = iniReader.ReadInteger("MAIN", "FixHud", 1) != 0;
	bWidescreenHud = iniReader.ReadInteger("MAIN", "WidescreenHud", 1) != 0;
	float fWidescreenHudOffset = iniReader.ReadFloat("MAIN", "WidescreenHudOffset", 100.0f);
	
	if (!fFOVFactor) { fFOVFactor = 1.0f; }
	if (!fWidescreenHudOffset) { fWidescreenHudOffset = 100.0f; }

	do
	{
		Sleep(100);
		e2mfc = GetModuleHandle("e2mfc.dll");
	} while (e2mfc == NULL);

	injector::MakeCALL((DWORD)e2mfc + 0x15582, PDriverGetWidth, true);
	injector::MakeCALL((DWORD)e2mfc + 0x155AB, PDriverGetHeight, true);

	do
	{
		Sleep(0);
	} while (Screen.nWidth == 0 || Screen.nHeight == 0);

	Screen.fWidth = static_cast<float>(Screen.nWidth);
	Screen.fHeight = static_cast<float>(Screen.nHeight);
	Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
	Screen.fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * Screen.fAspectRatio)) * (1.0f / SCREEN_FOV_HORIZONTAL);
	Screen.fDynamicScreenFieldOfViewScale *= fFOVFactor;

	injector::MakeJMP(0x50B9E0, FOVHook, true);

	IsInComicsMode = (uchar*)((DWORD)GetModuleHandle("e2_d3d8_driver_mfc.dll") + 0x845E8);

	if (Screen.fAspectRatio > (4.0f / 3.0f))
	{
		fWidthScale = 640.0f / Screen.fAspectRatio;
		fHalfWidthScale = fWidthScale / 2.0f;
		f1_fWidthScale = 1.0f / fWidthScale;
		fDouble1_fWidthScale = f1_fWidthScale * 2.0f;
		fHalf1_fWidthScale = f1_fWidthScale / 2.0f;

		if (bFixHud)
		{
			Screen.fHudOffset = ((-1.0f / Screen.fAspectRatio) * (4.0f / 3.0f));
			Screen.fHudScale = (1.0f / (480.0f * Screen.fAspectRatio)) * 2.0f;
			injector::WriteMemory<float>((DWORD)e2mfc + 0x495C8, Screen.fHudOffset, true);
			injector::WriteMemory<float>((DWORD)e2mfc + 0x495D0, Screen.fHudScale, true);

			pHudElementPosX = (float*)((DWORD)e2mfc + 0x65190);
			pHudElementPosY = (float*)((DWORD)e2mfc + 0x65194);
			injector::MakeJMP((DWORD)e2mfc + 0x856C, P_HudPosHook, true);
			jmpAddr = (DWORD)e2mfc + 0x856C + 0x25;

			if (bWidescreenHud)
			{
				Screen.fHudOffsetWide = fWidescreenHudOffset;

				if (Screen.fAspectRatio < (16.0f / 9.0f))
				{
					Screen.fHudOffsetWide = fWidescreenHudOffset / (((16.0f / 9.0f) / (Screen.fAspectRatio)) * 1.5f);
				}
				
				pTextElementPosX = (TextCoords*)((DWORD)e2mfc + 0x647D0);
				injector::MakeJMP((DWORD)e2mfc + 0x453A, P_TextPosHook, true);
				jmpAddr2 = (DWORD)e2mfc + 0x453A + 0x6;

				injector::MakeJMP((DWORD)e2mfc + 0x45FC, P_TextPosHook2, true);
				jmpAddr3 = (DWORD)e2mfc + 0x45FC + 0x7;

				injector::MakeJMP((DWORD)e2mfc + 0x4693, P_TextPosHook3, true);
				jmpAddr4 = (DWORD)e2mfc + 0x4693 + 0x6;
			}
		}

		injector::MakeJMP((DWORD)e2mfc + 0x14786, PCameraGetScreenRectHook, true);
		GetScreenRectJmp = (DWORD)e2mfc + 0x1478B;
		injector::MakeJMP((DWORD)e2mfc + 0x13FB0, DisableSubViewport, true);

		//injector::WriteMemory((DWORD)e2mfc + 0x148ED + 0x2, &f1_480, true); //doors fix ???

		injector::WriteMemory<float>((DWORD)e2mfc + 0x12F6C, fWidthScale, true);
		injector::WriteMemory<float>((DWORD)e2mfc + 0x13B5F, fWidthScale, true);
		injector::WriteMemory<float>((DWORD)e2mfc + 0x15AC6, fWidthScale, true);
		injector::WriteMemory<float>((DWORD)e2mfc + 0x15AF0, fWidthScale, true);
		injector::WriteMemory<float>((DWORD)e2mfc + 0x16A6A, fWidthScale, true);
		injector::WriteMemory<float>((DWORD)e2mfc + 0x16BDD, fWidthScale, true);
		injector::WriteMemory<float>((DWORD)e2mfc + 0x49DDC, fWidthScale, true);


		injector::WriteMemory<float>((DWORD)e2mfc + 0x49DF0, fHalfWidthScale, true);


		//CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x49DE4, height_multipl); //D3DERR_INVALIDCALL

		injector::WriteMemory((DWORD)e2mfc + 0x13D1D + 0x2, &f1_fWidthScale, true);
		injector::WriteMemory((DWORD)e2mfc + 0x13D82 + 0x2, &f1_fWidthScale, true);
		injector::WriteMemory((DWORD)e2mfc + 0x14238 + 0x2, &f1_fWidthScale, true);
		injector::WriteMemory((DWORD)e2mfc + 0x1429D + 0x2, &f1_fWidthScale, true);
		injector::WriteMemory((DWORD)e2mfc + 0x146FA + 0x2, &f1_fWidthScale, true);
	  //injector::WriteMemory((DWORD)e2mfc + 0x14775 + 0x2, &f1_fWidthScale, true);
		injector::WriteMemory((DWORD)e2mfc + 0x148ED + 0x2, &f1_fWidthScale, true);
		injector::WriteMemory((DWORD)e2mfc + 0x14968 + 0x2, &f1_fWidthScale, true);
		injector::WriteMemory((DWORD)e2mfc + 0x1566C + 0x2, &f1_fWidthScale, true);

		injector::WriteMemory<float>((DWORD)e2mfc + 0x49DEC, fDouble1_fWidthScale, true);
		injector::WriteMemory<float>((DWORD)e2mfc + 0x49DFC, fHalf1_fWidthScale, true);


		do
		{
			Sleep(0);
		} while (nCounter < nLoadingDelay);

		injector::WriteMemory((DWORD)e2mfc + 0x14775 + 0x2, &f1_fWidthScale, true);
		injector::WriteMemory<uint>((DWORD)e2mfc + 0x14786, 0x000C00BB, true);
		injector::WriteMemory<uchar>((DWORD)e2mfc + 0x14786 + 4, 0x00, true);

		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&ComicsHandler, NULL, 0, NULL);
	}
	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Thread, NULL, 0, NULL);
	}
	return TRUE;
}
