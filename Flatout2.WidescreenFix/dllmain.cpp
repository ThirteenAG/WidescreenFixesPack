#include "..\includes\stdafx.h"
#include "..\includes\hooking\Hooking.Patterns.h"

DWORD* pfSetAR;

DWORD* nWidth;
DWORD* nHeight;
float* pfAspectRatioX;
float* pfAspectRatioY;
float* pfHUDScaleX;
float fHUDScaleX;
DWORD jmpAddr, jmpAddr2, jmpAddr3, jmpAddr4, jmpAddr5, _REG;
float fHudOffset, fHUDOffset1, fHUDOffset2;
DWORD* pfDrawHud;
DWORD* pfDrawText;
DWORD* pfSetFOV;
float fIniFOVFactor, fFOVFactor;

#define _USE_MATH_DEFINES
#include "math.h"
#define DEGREE_TO_RADIAN(fAngle) \
	((fAngle)* (float)M_PI / 180.0f)
#define RADIAN_TO_DEGREE(fAngle) \
	((fAngle)* 180.0f / (float)M_PI)
#define SCREEN_AR_NARROW			(4.0f / 3.0f)	// 640.0f / 480.0f
#define SCREEN_FOV_HORIZONTAL		90.0f
#define SCREEN_FOV_VERTICAL			(2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / SCREEN_AR_NARROW)))	// Default is 75.0f.
float fScreenFieldOfViewVStd = SCREEN_FOV_VERTICAL;
float fDynamicScreenFieldOfViewScale;

void __declspec(naked) CenterText()
{
	_asm
	{
		mov     esi, [ebp + 0x14C]
		fstp    dword ptr[esp + 20h]
		mov     _REG, eax
		mov     eax, dword ptr[esp + 0x20]
		mov		fHUDOffset1, eax
	}
	fHUDOffset1 += fHudOffset;
	_asm
	{
		mov     eax, fHUDOffset1
		mov     dword ptr[esp + 0x20], eax
		mov     eax, _REG
		jmp     jmpAddr2
	}
}

void __declspec(naked) CenterText2()
{
	_asm
	{
		fstp    dword ptr[esp + 0D8h]
		mov     ecx, [esp + 0D8h]
		mov		fHUDOffset1, ecx
	}
	fHUDOffset1 += fHudOffset;
	_asm
	{
		mov     ecx, fHUDOffset1
		mov     dword ptr[esp + 0D8h], ecx
		jmp     jmpAddr3
	}
}

void __declspec(naked) CenterText3()
{
	_asm
	{
		fstp    dword ptr[esp]
		push    eax
		mov     eax, [esp+4]
		mov		fHUDOffset1, eax
	}
	fHUDOffset1 += fHudOffset;
	_asm
	{
		mov     eax, fHUDOffset1
		mov     dword ptr[esp+4], eax
		mov     eax, [edi + 0x4C]
		jmp     jmpAddr4
	}
}

void __declspec(naked) CenterText4()
{
	_asm
	{
		fstp    dword ptr[esp + 28h]
		fld     dword ptr[esp + 6Ch]
		mov     esi, [esp + 28h]
		mov		fHUDOffset1, esi
	}
	fHUDOffset1 += fHudOffset;
	_asm
	{
		mov     esi, fHUDOffset1
		mov     dword ptr[esp + 28h], esi
		jmp     jmpAddr5
	}
}

void __declspec(naked) CenterHUD()
{
	_asm
	{
		fmul    dword ptr[esi + 4]
		fstp    dword ptr[esp + 14h]
		mov     _REG, ecx
		mov     ecx, dword ptr[esp + 14h]
		mov		fHUDOffset1, ecx
	}
	fHUDOffset1 += fHudOffset;
	_asm
	{
		
		mov		ecx, fHUDOffset1
		mov     dword ptr[esp + 14h], ecx
		mov     ecx, _REG
		jmp     jmpAddr
	}
}

void Init()
{
	CIniReader iniReader("");
	fIniFOVFactor = iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f);

	nWidth = *hook::pattern("C7 05 ? ? ? ? 00 04 00 00 C7 05 ? ? ? ? 00 03 00 00 C7 05 ? ? ? ? 10 00 00 00").get(0).get<DWORD*>(2);
	nHeight = nWidth + 1;
	
	injector::WriteMemory<float>(pfAspectRatioX, static_cast<float>(*nWidth), true);
	injector::WriteMemory<float>(pfAspectRatioY, static_cast<float>(*nHeight), true);

	pfHUDScaleX = *hook::pattern("50 D8 0D ? ? ? ? D9 5C 24 20 DB 47 0C").get(0).get<float*>(3); //0x
	fHUDScaleX = 1.0f / (480.0f * ((static_cast<float>(*nWidth) / static_cast<float>(*nHeight))));
	injector::WriteMemory<float>(pfHUDScaleX, fHUDScaleX, true);

	fHudOffset = (*nWidth - *nHeight * (4.0f / 3.0f)) / 2.0f;
	
	pfDrawHud = hook::pattern("D8 4E 04 D9 5C 24 14 D9 05 ? ? ? ? D8 4E 08 D9 5C 24 18 D9 05 ? ? ? ?").get(0).get<DWORD>(0); //0x533679
	jmpAddr = (DWORD)pfDrawHud + 7; //0x533679 + 7
	injector::MakeJMP(pfDrawHud, CenterHUD, true);
	
	pfDrawText = hook::pattern("8B B5 4C 01 00 00 D9 5C 24 20 52 D9 05 ? ? ? ? 83 EC 08").get(0).get<DWORD>(0); //0x53F714 
	jmpAddr2 = (DWORD)pfDrawText + 10; //0x53F714 + 10;
	injector::MakeJMP(pfDrawText, CenterText, true);

	pfDrawText = hook::pattern("D9 05 ? ? ? ? D8 4C 24 14 DE C1 D9 9C 24 D8 00 00 00").get(0).get<DWORD>(12); //0x52D585
	jmpAddr3 = (DWORD)pfDrawText + 7; //0x52D585 + 7
	injector::MakeJMP(pfDrawText, CenterText2, true);

	pfDrawText = hook::pattern("D9 1C 24 50 8B 47 4C E8 ? ? ? ?").get(0).get<DWORD>(0); //0x535939
	jmpAddr4 = (DWORD)pfDrawText + 7; //0x535939 + 7
	injector::MakeJMP(pfDrawText, CenterText3, true);
	
	//main menu fixes
	pfDrawText = hook::pattern("D9 5C 24 28 D9 44 24 6C 8B 74 24 28 D8 05 ? ? ? ? D8 4C 24 18").get(0).get<DWORD>(0); //4AA74F
	jmpAddr5 = (DWORD)pfDrawText + 8; //0x4AA74F+8
	injector::MakeJMP(pfDrawText, CenterText4, true);

	pfDrawText = hook::pattern("D9 05 ? ? ? ? D8 4E 0C D8 05 ? ? ? ? DD 1C 24 E8 ? ? ? ? D9 5C 24 2C").get(0).get<DWORD>(2); //533A90
	injector::WriteMemory(pfDrawText, &fHudOffset, true); // fade backround, fHudOffset instead of a random big number to stretch it

	fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(fScreenFieldOfViewVStd * 0.5f)) * (static_cast<float>(*nWidth) / static_cast<float>(*nHeight)))) * (1.0f / SCREEN_FOV_HORIZONTAL);
	fFOVFactor = fDynamicScreenFieldOfViewScale * 0.017453292f * fIniFOVFactor;
	pfSetFOV = hook::pattern("D8 0D ? ? ? ? 55 8B EF 2B DD C1 FB 02 8D 68 01 3B EB").get(0).get<DWORD>(2);
	injector::WriteMemory(pfSetFOV, &fFOVFactor, true);

	//to do: pause menu isn't centered, see refs 0x67DBE4

	//
	//CPatch::RedirectCall(0x4C0AE8, asm_patch); //derby triangles
	//CPatch::RedirectCall(0x4BB548, asm_patch); //black backround stripes
	//CPatch::SetInt(0x4BB500, 0x560010C2);
}

DWORD WINAPI PatchAR(LPVOID)
{
	Sleep(1000); // to do: a better way to delay?
	//pfSetAR = hook::pattern("8B 44 24 04 8B 4C 24 08 A3 ? ? ? ? 89 0D ? ? ? ? C2 08 00").get(0).get<DWORD>(0);
	pfAspectRatioX = *hook::pattern("8B 44 24 04 8B 4C 24 08 A3 ? ? ? ? 89 0D ? ? ? ? C2 08 00").get(0).get<float*>(9); //0x69BE18
	pfAspectRatioY = pfAspectRatioX + 1; //0x69BE1C
	//injector::MakeJMP(pfSetAR, Init, true);
	Init();
	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&PatchAR, NULL, 0, NULL);
	}
	return TRUE;
}
