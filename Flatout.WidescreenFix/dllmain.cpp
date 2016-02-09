#include "..\includes\stdafx.h"
#include "..\includes\hooking\Hooking.Patterns.h"

DWORD* pfShowIntroCall;
injector::memory_pointer_raw pfShowIntro;

DWORD* nWidth;
DWORD* nHeight;
float* pfAspectRatioX;
float* pfAspectRatioY;
float* pfHUDScaleX;
float fHUDScaleX;
DWORD jmpAddr, jmpAddr2, jmpAddr3, _REG;
float fHudOffset, fHUDOffset1, fHudOffset2;
DWORD* pfDrawHud;
DWORD* pfDrawText;
DWORD* pfSetFOV;
float fIniFOVFactor, fFOVFactor;
bool bDelay;

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
		mov     dword ptr ds: [esp + 48h], 0
		fstp    dword ptr ds: [esp + 0x2C]
		mov     _REG, eax
		mov     eax, dword ptr[esp + 0x2C]
		mov		fHUDOffset1, eax
	}
	fHUDOffset1 += fHudOffset;
	_asm
	{
		mov     eax, fHUDOffset1
		mov     dword ptr[esp + 0x2C], eax
		mov     eax, _REG
		jmp     jmpAddr2
	}
}

void __declspec(naked) CenterText2()
{
	_asm
	{
		fstp    dword ptr[esp + 78h]
		mov     ecx, [esp + 78h]
		mov		fHUDOffset1, ecx
	}
	fHUDOffset1 += fHudOffset;
	_asm
	{
		mov     ecx, fHUDOffset1
		mov     dword ptr[esp + 78h], ecx
		mov     ecx, [esp + 78h]
		jmp     jmpAddr3
	}
}

void __declspec(naked) CenterHUD()
{
	_asm
	{
		fmul    dword ptr ds : [ebx + 10h]
		fstp    dword ptr ds : [esp + 24h]
		mov     _REG, ecx
		mov     ecx, dword ptr[esp + 18h]
		mov		fHUDOffset1, ecx
	}
	fHUDOffset1 += fHudOffset;
	_asm
	{
		
		mov		ecx, fHUDOffset1
		mov     dword ptr[esp + 18h], ecx
		mov     ecx, _REG
		jmp     jmpAddr
	}
}

void Init()
{
	CIniReader iniReader("");
	bool bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
	fIniFOVFactor = iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f);

	nWidth = *hook::pattern("C7 05 ? ? ? ? 00 04 00 00 C7 05 ? ? ? ? 00 03 00 00 C7 05 ? ? ? ? 10 00 00 00").get(0).get<DWORD*>(2);
	nHeight = nWidth + 1;

	pfAspectRatioX = hook::pattern("8B 44 24 04 8B 4C 24 08 C7 00 00 00 80").get(0).get<float>(10); //0x5069DA 
	pfAspectRatioY = (float*)((DWORD)pfAspectRatioX + 6); //0x5069E0
	
	injector::WriteMemory<float>(pfAspectRatioX, static_cast<float>(*nWidth), true);
	injector::WriteMemory<float>(pfAspectRatioY, static_cast<float>(*nHeight), true);

	pfHUDScaleX = *hook::pattern("D8 0D ? ? ? ? DB 40 0C C7 05 ? ? ? ? 00 80 F8 43").get(0).get<float*>(2); //0x667CE4
	fHUDScaleX = 1.0f / (480.0f * ((static_cast<float>(*nWidth) / static_cast<float>(*nHeight))));
	injector::WriteMemory<float>(pfHUDScaleX, fHUDScaleX, true);

	fHudOffset = (*nWidth - *nHeight * (4.0f / 3.0f)) / 2.0f;
	fHudOffset2 = ((480.0f * (static_cast<float>(*nWidth) / static_cast<float>(*nHeight))) - 640.0f) / 2.0f;

	pfDrawHud = hook::pattern("D8 4B 10 D9 5C 24 24 74 08").get(0).get<DWORD>(0); //0x4E2B41 
	jmpAddr = (DWORD)pfDrawHud + 7; //0x4E2B41 + 7
	injector::MakeJMP(pfDrawHud, CenterHUD, true);
	
	pfDrawText = hook::pattern("C7 44 24 48 00 00 00 00 D9 5C 24 2C D9 05 ? ? ? ?").get(0).get<DWORD>(0); //0x4E28C6 
	jmpAddr2 = (DWORD)pfDrawText + 12; //0x4E28C6 + 12;
	injector::MakeJMP(pfDrawText, CenterText, true);

	pfDrawText = hook::pattern("D9 5C 24 78 8B 4C 24 78 D9 05 ? ? ? ?").get(0).get<DWORD>(0); //0x4DA8BA
	jmpAddr3 = (DWORD)pfDrawText + 8; //0x4DA8BA + 8
	injector::MakeJMP(pfDrawText, CenterText2, true);

	//pause menu centering
	DWORD* TempPtr = *hook::pattern("D8 0D ? ? ? ? D9 5C 24 14 D9 44 24 10 D8 0D ? ? ? ? D9 5C 24 18").get(0).get<DWORD*>(2); //667FA4
	injector::WriteMemory<float>(TempPtr, *(float*)TempPtr + fHudOffset2, true); //background

	TempPtr = *hook::pattern("D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 8B 54 24 08 8B 44 24 0C 89 7E 04 5F").get(0).get<DWORD*>(2); //667FAC
	injector::WriteMemory<float>(TempPtr, *(float*)TempPtr + fHudOffset2, true); //PAUSED string

	TempPtr = *hook::pattern("D8 0D ? ? ? ? D9 1C 24 55 E8 ? ? ? ? 8B 4C 24 20 FF 56 44").get(0).get<DWORD*>(2); //667F98
	injector::WriteMemory<float>(TempPtr, *(float*)TempPtr + fHudOffset2, true); //menu text

	TempPtr = *hook::pattern("D8 0D ? ? ? ? D8 E1 D9 1C 24 57 DD D8").get(0).get<DWORD*>(2); //6681A8
	injector::WriteMemory<float>(TempPtr, *(float*)TempPtr + fHudOffset2, true); //press enter to start text

	TempPtr = *hook::pattern("D8 0D ? ? ? ? D9 1C 24 50 57").get(0).get<DWORD*>(2); //6681A8
	injector::WriteMemory<float>(TempPtr, *(float*)TempPtr + fHudOffset2, true); //sound options text

	TempPtr = *hook::pattern("D8 0D ? ? ? ? 83 EC 08 8B F7 D9 5C 24 20").get(0).get<DWORD*>(2); //667FC0
	injector::WriteMemory<float>(TempPtr, *(float*)TempPtr + fHudOffset2, true); //sound options sliders

	//wrong way text background (kinda ugly, let's remove it)
	TempPtr = *hook::pattern("D8 0D ? ? ? ? C7 05 ? ? ? ? 00 00 D0 42").get(0).get<DWORD*>(2); //667FBC
	injector::WriteMemory<float>(TempPtr, 0.0f, true);
	injector::WriteMemory<float>(TempPtr + 1, 0.0f, true);

	//loading bar offset
	TempPtr = *hook::pattern("D8 0D ? ? ? ? 89 44 24 34 D9 5C 24 24").get(0).get<DWORD*>(2); //4C07C6
	injector::WriteMemory<float>(TempPtr, *(float*)TempPtr + (fHudOffset2 / 2.18f), true);

	fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(fScreenFieldOfViewVStd * 0.5f)) * (static_cast<float>(*nWidth) / static_cast<float>(*nHeight)))) * (1.0f / SCREEN_FOV_HORIZONTAL);
	fFOVFactor = fDynamicScreenFieldOfViewScale * 0.017453292f * fIniFOVFactor;
	pfSetFOV = hook::pattern("D8 0D ? ? ? ? 55 8B EF 2B DD C1 FB 02 8D 68 01 3B EB").get(0).get<DWORD>(2);
	injector::WriteMemory(pfSetFOV, &fFOVFactor, true);

	if (!bSkipIntro)
	{
		static auto ShowIntro = (void(__cdecl *)()) pfShowIntro.get();
		ShowIntro();
	}
}

DWORD WINAPI PatchIntro(LPVOID)
{
	auto pattern = hook::pattern("C7 05 ? ? ? ? 00 04 00 00 C7 05 ? ? ? ? 00 03 00 00 C7 05 ? ? ? ? 10 00 00 00");
	if (!(pattern.size() > 0) && !bDelay)
	{
		bDelay = true;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&PatchIntro, NULL, 0, NULL);
		return 0;
	}

	pfShowIntroCall = hook::pattern("E8 ? ? ? ? 8B 15 ? ? ? ? A1 ? ? ? ? 52 50 E8 ? ? ? ?").get(0).get<DWORD>(0);
	pfShowIntro = injector::GetBranchDestination(pfShowIntroCall, true);
	injector::MakeCALL(pfShowIntroCall, Init, true);
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		PatchIntro(NULL);
	}
	return TRUE;
}
