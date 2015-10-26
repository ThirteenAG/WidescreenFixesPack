#include "..\includes\stdafx.h"
#include "..\includes\CFileMgr.h"
#include "..\includes\hooking\Hooking.Patterns.h"
#include <algorithm>
HWND hWnd;

bool HudFix;
bool DisableCutsceneBorders;
int ResX;
int ResY;
float horFOV, verFOV;
bool bHudMode;
float fHudScaleX, fHudScaleY;
float fHudPosX;
float MinimapPosX, MinimapPosY;
float verFovCorrectionFactor;
bool bHudWidescreenMode;
bool bFMVWidescreenMode;

union HudPos
{
	DWORD dwPos;
	float fPos;
} HudPosX, HudPosY;

class CDatEntry
{
public:
	float fPosX;
	float fPosY;
	float fOffsetX;
	float fOffsetY;

	CDatEntry(float posx, float posy, float fOffsetX, float fOffsetY)
		: fPosX(posx), fPosY(posy), fOffsetX(fOffsetX), fOffsetY(fOffsetY)
	{}
};

std::vector<CDatEntry> HudCoords;

void LoadDatFile()
{
	CIniReader iniReader("");
	char* DataFilePath = iniReader.GetIniPath();
	char*			tempPointer;
	tempPointer = strrchr(DataFilePath, '.');
	*tempPointer = '\0';
	strcat(DataFilePath, ".dat");

	if (FILE* hFile = CFileMgr::OpenFile(DataFilePath, "r"))
	{
		while (const char* pLine = CFileMgr::LoadLine(hFile))
		{
			if (pLine[0] && pLine[0] != '#')
			{
				float PosX, PosY;
				float fOffsetX, fOffsetY;
				sscanf(pLine, "%*s %f %f %f %f", &PosX, &PosY, &fOffsetX, &fOffsetY);

				if (((float)ResX / (float)ResY) < (16.0f / 9.0f))
				{
					fOffsetX /= (((16.0f / 9.0f) / ((float)ResX / (float)ResY)) * 1.5f);
				}

				HudCoords.push_back(CDatEntry(PosX, PosY, fOffsetX, fOffsetY));
			}
		}
		CFileMgr::CloseFile(hFile);
	}
}

void WidescreenHud()
{
	__asm pushad
	__asm pushfd

	auto it = std::find_if(HudCoords.begin(), HudCoords.end(),
		[&cc = CDatEntry(std::floor(HudPosX.fPos), std::floor(HudPosY.fPos), 0.0f, 0.0f)]
	(const CDatEntry& cde) -> bool { return (cc.fPosX == cde.fPosX && cc.fPosY == cde.fPosY); });

	if (it != HudCoords.end())
	{
		HudPosX.fPos += it->fOffsetX;
		HudPosY.fPos += it->fOffsetY;
	}

	__asm popfd
	__asm popad
}

DWORD jmpAddr, jmpAddr2, jmpAddr3, jmpAddr4, _EDX;
void __declspec(naked) HudHook()
{
	_asm
	{
		mov		HudPosX.dwPos, ecx
		mov		HudPosY.dwPos, edx
		call	WidescreenHud
		mov		ecx, HudPosX.dwPos
		mov		edx, HudPosY.dwPos
		mov		[esp + 60h], ecx
		mov		[esp + 64h], edx
		jmp		jmpAddr
	}
}

void __declspec(naked) BlipsHook()
{
	_asm
	{
		mov     ecx, [ebx+1Ch]
		mov     edx, [ebx + 1Ch + 4h]
		mov		HudPosX.dwPos, ecx
		mov		HudPosY.dwPos, edx
		call	WidescreenHud
		mov		ecx, HudPosX.dwPos
		mov     edx, [esp + 18h]
		mov     [edx], ecx
		mov     eax, [ebx+20h]
		jmp		jmpAddr2
	}
}

void __declspec(naked) HudHook2()
{
	_asm
	{
		fst     dword ptr[esi + 48h]
		mov     ecx, [esi + 1Ch]
		mov     edx, [esi + 1Ch + 4h]
		mov		HudPosX.dwPos, ecx
		mov		HudPosY.dwPos, edx
		call	WidescreenHud
		mov		ecx, HudPosX.dwPos
		jmp		jmpAddr3
	}
}

void __declspec(naked) StopSignHook()
{
	_asm
	{
		mov		_EDX, edx
		mov     edx, [edx]
		mov		HudPosX.dwPos, edx
		mov     edx, _EDX
		mov     edx, [edx+4]
		mov		HudPosY.dwPos, edx
		call	WidescreenHud
		mov     edx, _EDX
		//mov		edx, HudPosX.dwPos
		fadd    dword ptr[HudPosX.dwPos]
		fstp    dword ptr[esi]
		fld     dword ptr[ecx + 4]
		jmp		jmpAddr4
	}
}

void Init()
{
	CIniReader iniReader("");
	ResX = iniReader.ReadInteger("MAIN", "ResX", 0);
	ResY = iniReader.ReadInteger("MAIN", "ResY", 0);
	HudFix = iniReader.ReadInteger("MAIN", "HudFix", 1) == 1;
	verFovCorrectionFactor = iniReader.ReadFloat("MAIN", "verFovCorrectionFactor", 0.04f);
	bHudMode = iniReader.ReadInteger("MAIN", "HudMode", 1) == 1;
	DisableCutsceneBorders = iniReader.ReadInteger("MAIN", "DisableCutsceneBorders", 1) == 1;
	bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) == 1;
	bFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1) == 1;

	if (!ResX || !ResY) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		ResX = info.rcMonitor.right - info.rcMonitor.left;
		ResY = info.rcMonitor.bottom - info.rcMonitor.top;
	}

	//game
	DWORD* dword_5BF62F = hook::pattern("C7 00 80 02 00 00 C7 01 E0 01 00 00").get(0).get<DWORD>(2);
	//injector::MakeNOP(dword_5BF62F - 10 - 13, 13, true);
	injector::WriteMemory(dword_5BF62F, ResX, true);
	DWORD* dword_5BF646 = hook::pattern("C7 02 20 03 00 00 C7 00 58 02 00 00").get(0).get<DWORD>(2);
	injector::WriteMemory(dword_5BF646, ResX, true);
	DWORD* dword_5BF65D = hook::pattern("C7 01 00 04 00 00 C7 02 00 03 00 00").get(0).get<DWORD>(2);
	injector::WriteMemory(dword_5BF65D, ResX, true);
	DWORD* dword_5BF674 = hook::pattern("C7 00 00 05 00 00 C7 01 C0 03 00 00").get(0).get<DWORD>(2);
	injector::WriteMemory(dword_5BF674, ResX, true);
	DWORD* dword_5BF68B = hook::pattern("C7 02 00 05 00 00 C7 00 00 04 00 00").get(0).get<DWORD>(2);
	injector::WriteMemory(dword_5BF68B, ResX, true);
	DWORD* dword_5BF6A2 = hook::pattern("C7 01 40 06 00 00 C7 02 B0 04 00 00").get(0).get<DWORD>(2);
	injector::WriteMemory(dword_5BF6A2, ResX, true);


	DWORD dword_5BF635 = (DWORD)dword_5BF62F + 6;
	injector::WriteMemory(dword_5BF635, ResY, true);
	DWORD dword_5BF64C = (DWORD)dword_5BF646 + 6;
	injector::WriteMemory(dword_5BF64C, ResY, true);
	DWORD dword_5BF663 = (DWORD)dword_5BF65D + 6;
	injector::WriteMemory(dword_5BF663, ResY, true);
	DWORD dword_5BF67A = (DWORD)dword_5BF674 + 6;
	injector::WriteMemory(dword_5BF67A, ResY, true);
	DWORD dword_5BF691 = (DWORD)dword_5BF68B + 6;
	injector::WriteMemory(dword_5BF691, ResY, true);
	DWORD dword_5BF6A8 = (DWORD)dword_5BF6A2 + 6;
	injector::WriteMemory(dword_5BF6A8, ResY, true);


	/*DWORD* dword_7FF77C = hook::pattern("").get(0).get<DWORD>(2);
	injector::WriteMemory(dword_7FF77C, ResX, true);
	DWORD* dword_86E77C = hook::pattern("").get(0).get<DWORD>(2);
	injector::WriteMemory(dword_86E77C, ResX, true);
	DWORD* dword_86E7B4 = hook::pattern("").get(0).get<DWORD>(2);
	injector::WriteMemory(dword_86E7B4, ResX, true);
	DWORD* dword_86F870 = hook::pattern("").get(0).get<DWORD>(2);
	injector::WriteMemory(dword_86F870, ResX, true);
	DWORD* dword_870980 = hook::pattern("").get(0).get<DWORD>(2);
	injector::WriteMemory(dword_870980, ResX, true);

	DWORD* dword_7FF780 = hook::pattern("").get(0).get<DWORD>(2);
	injector::WriteMemory(dword_7FF780, ResY, true);
	DWORD* dword_86E780 = hook::pattern("").get(0).get<DWORD>(2);
	injector::WriteMemory(dword_86E780, ResY, true);
	DWORD* dword_86E7B8 = hook::pattern("").get(0).get<DWORD>(2);
	injector::WriteMemory(dword_86E7B8, ResY, true);
	DWORD* dword_86F874 = hook::pattern("").get(0).get<DWORD>(2);
	injector::WriteMemory(dword_86F874, ResY, true);
	DWORD* dword_870984 = hook::pattern("").get(0).get<DWORD>(2);
	injector::WriteMemory(dword_870984, ResY, true);*/
	

	

	//HUD
	if (HudFix)
	{
		fHudScaleX = (1.0f / ResX * (ResY / 480.0f)) * 2.0f;
		fHudPosX = 640.0f / (640.0f * fHudScaleX);

		DWORD* dword_79AC10 = *hook::pattern("D9 05 ? ? ? ? 89 7C 24 28 C7 44 24 2C 00 00 00 3F D8 C9 C7 44 24 30 00 00 00 3F").get(0).get<DWORD*>(2);
		injector::WriteMemory<float>(dword_79AC10, fHudScaleX, true);

		//fHudScaleY = *(float*)0x79AC14;
		//CPatch::SetFloat(0x79AC14, fHudScaleY);

		DWORD* dword_51B3CB = hook::pattern("C7 84 24 A0 00 00 00 00 00 A0 43 C7 84 24 A4 00 00 00 00 00 70 43 C7 84 24 A8 00 00 00 00 00 00 00 0F B7 48 20").get(0).get<DWORD>(7);
		injector::WriteMemory<float>(dword_51B3CB, fHudPosX, true);
		DWORD* dword_5368C8 = hook::pattern("C7 44 24 74 00 00 A0 43 C7 44 24 78 00 00 70 43 C7 44 24 7C 00 00 00 00 E8 ? ? ? ? 8D 4C 24 70").get(0).get<DWORD>(4);
		injector::WriteMemory<float>(dword_5368C8, fHudPosX, true);
		DWORD* dword_536A99 = hook::pattern("C7 84 24 84 00 00 00 00 00 A0 43 C7 84 24 88 00 00 00 00 00 70 43 C7 84 24 8C 00 00 00 00 00 00 00 E8 ? ? ? ? 8D 8C 24 80 00 00 00").get(0).get<DWORD>(7);
	    injector::WriteMemory<float>(dword_536A99, fHudPosX, true); //minimap
		DWORD* dword_536CC9 = hook::pattern("C7 84 24 94 00 00 00 00 00 A0 43 C7 84 24 98 00 00 00 00 00 70 43 C7 84 24 9C 00 00 00 00 00 00 00 E8 ? ? ? ? 8D 8C 24 90 00 00 00").get(0).get<DWORD>(7);
		injector::WriteMemory<float>(dword_536CC9, fHudPosX, true);
		DWORD* dword_537011 = hook::pattern("C7 44 24 74 00 00 A0 43 C7 44 24 78 00 00 70 43 C7 44 24 7C 00 00 00 00 E8 ? ? ? ? 8D 54 24 70 52").get(0).get<DWORD>(4);
		injector::WriteMemory<float>(dword_537011, fHudPosX, true);
		DWORD* dword_48B640 = hook::pattern("C7 05 ? ? ? ? 00 00 A0 43 C7 05 ? ? ? ? 00 00 00 00 C7 05 ? ? ? ? 00 00 00 00 C6 05 ? ? ? ? 82").get(0).get<DWORD>(6);
		injector::WriteMemory<float>(dword_48B640, fHudPosX, true);
		DWORD* dword_50B4F5 = hook::pattern("C7 84 24 A0 01 00 00 00 00 A0 43 C7 84 24 A4 01 00 00 00 00 70 43 C7 84 24 A8 01 00 00 00 00 00 00 E8 ? ? ? ? 8D 8C 24 30 01 00 00 51 8B F0").get(0).get<DWORD>(7);
		injector::WriteMemory<float>(dword_50B4F5, fHudPosX, true);
		DWORD* dword_797D50 = *hook::pattern("D8 25 ? ? ? ? 8B 4C 24 28 6A 00 8D 54 24 48 D9 5C 24 48 89 4C 24 50 D9 44 24 28 8B 0B").get(0).get<DWORD*>(2);
		injector::WriteMemory<float>(dword_797D50, fHudPosX, true);
		injector::WriteMemory<float>(dword_797D50 + 2, fHudPosX * 2.0f, true);

		DWORD* dword_5CC109 = hook::pattern("C7 44 24 40 00 00 48 43 C7 44 24 44 00 00 70 41 C7 44 24 50 00 00 48 43 C7 44 24 54 00 00 BE 42 C7 44 24 60 00 00 DC 43").get(0).get<DWORD>(4); //rearview mirror
		DWORD dword_5CC119 = (DWORD)dword_5CC109 + 16;
		DWORD dword_5CC0F9 = (DWORD)dword_5CC109 - 16;
		DWORD dword_5CC129 = (DWORD)dword_5CC109 + 32;
		injector::WriteMemory<float>(dword_5CC109, (fHudPosX - 320.0f) + 200.0f, true);
		injector::WriteMemory<float>(dword_5CC119, (fHudPosX - 320.0f) + 200.0f, true);
		injector::WriteMemory<float>(dword_5CC0F9, (fHudPosX - 320.0f) + 440.0f, true);
		injector::WriteMemory<float>(dword_5CC129, (fHudPosX - 320.0f) + 440.0f, true);

		DWORD* dword_5CBEF5 = hook::pattern("C7 44 24 38 00 00 20 44 C7 44 24 40 00 00 00 00 C7 44 24 48 00 00").get(0).get<DWORD>(4);
		DWORD  dword_5CBF05 = (DWORD)dword_5CBEF5 + 16;
		DWORD* dword_5CBE89 = hook::pattern("C7 44 24 2C 00 00 20 44 C7 44 24 30 00 00 00 00 C7 44 24 34 00 00 00 00 C7 44 24 3C 00 00 20 44 C7 44 24 44 00 00 00 00").get(0).get<DWORD>(4);
		DWORD  dword_5CBEA1 = (DWORD)dword_5CBE89 + 24;
		injector::WriteMemory<float>(dword_5CBEF5, (float)ResX, true); // borders
		injector::WriteMemory<float>(dword_5CBF05, (float)ResX, true);
		injector::WriteMemory<float>(dword_5CBE89, (float)ResX, true);
		injector::WriteMemory<float>(dword_5CBEA1, (float)ResX, true);

		if (DisableCutsceneBorders)
		{
			injector::WriteMemory<float>(dword_5CBEF5, 0.0f, true); // borders
			injector::WriteMemory<float>(dword_5CBF05, 0.0f, true);
			injector::WriteMemory<float>(dword_5CBE89, 0.0f, true);
			injector::WriteMemory<float>(dword_5CBEA1, 0.0f, true);
		}

		DWORD* dword_5C726A = hook::pattern("C7 44 24 30 00 00 20 44 C7 44 24 34 00 00 00 00 C7 44 24 40 00 00 20 44 C7 44 24 44 00 00 F0 43 C7 44 24 50 00 00 00 00 C7 44 24 54 00 00 F0 43 7D 06").get(0).get<DWORD>(4);
		injector::WriteMemory<float>(dword_5C726A, (float)ResX * 2.0f, true); // radar mask
		DWORD  dword_5C727A = (DWORD)dword_5C726A + 16;
		injector::WriteMemory<float>(dword_5C727A, (float)ResX * 2.0f, true);
		DWORD  dword_5C7282 = (DWORD)dword_5C726A + 24;
		injector::WriteMemory<float>(dword_5C7282, (float)ResY * 2.0f, true);
		DWORD  dword_5C7292 = (DWORD)dword_5C726A + 40;
		injector::WriteMemory<float>(dword_5C7292, (float)ResY * 2.0f, true);
	}

	if (verFovCorrectionFactor)
	{
		DWORD* dword_5C7FEA = hook::pattern("D8 3D ? ? ? ? D9 5C 24 1C DB 44 24 2C D8 4C 24 28").get(0).get<DWORD>(2);
		injector::WriteMemory(dword_5C7FEA, &horFOV, true);
		DWORD* dword_5C8021 = hook::pattern("D8 3D ? ? ? ? D9 5C 24 38 D9 44 24 24 D8 64 24 30 D8 7C 24 24 D9 5C 24 34 E8 ? ? ? ?").get(0).get<DWORD>(2);
		injector::WriteMemory(dword_5C8021, &verFOV, true);

		horFOV = 1.0f / ((1.0f * ((float)ResX / (float)ResY)) / (4.0f / 3.0f));
		verFOV = horFOV - verFovCorrectionFactor;
	}

	if (bFMVWidescreenMode)
	{
		DWORD* dword_536A1F = hook::pattern("68 00 00 00 3F 68 00 00 00 3F 68 00 00 00 BF 68 00 00 00 BF 8B CB E8 ? ? ? ? 8B 44 24 18 8B CB").get(0).get<DWORD>(1);
		DWORD  dword_536A24 = (DWORD)dword_536A1F + 5;
		DWORD  dword_536A29 = (DWORD)dword_536A1F + 10;
		DWORD  dword_536A2E = (DWORD)dword_536A1F + 15;
		injector::WriteMemory<float>(dword_536A1F, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
		injector::WriteMemory<float>(dword_536A24, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
		injector::WriteMemory<float>(dword_536A29, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
		injector::WriteMemory<float>(dword_536A2E, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
	}

	if (bHudWidescreenMode)
	{
		LoadDatFile();

		DWORD* dword_51B190 = hook::pattern("89 4C 24 60 89 54 24 64 74 ? 8D 8C 24 F0 00 00 00 51").get(0).get<DWORD>(0);
		DWORD  dword_51B198 = (DWORD)dword_51B190 + 8;
		injector::MakeJMP(dword_51B190, HudHook, true);
		jmpAddr = dword_51B198;

		DWORD* dword_51D64E = hook::pattern("8B 4B 1C 8B 54 24 18 89 0A 8B 43 20").get(0).get<DWORD>(0);
		DWORD  dword_51D65A = (DWORD)dword_51D64E + 9;
		injector::MakeJMP(dword_51D64E, BlipsHook, true);
		jmpAddr2 = dword_51D65A;

		DWORD* dword_4C66B3 = hook::pattern("D9 56 48 8B 4E 1C D8 2D ? ? ? ? 89 4C 24 28 8B 56 20 89 54 24 2C 8B 46 24").get(0).get<DWORD>(0); //addresses from 1.1 exe
		DWORD  dword_4C66B9 = (DWORD)dword_4C66B3 + 6;
		injector::MakeJMP(dword_4C66B3, HudHook2, true);
		jmpAddr3 = dword_4C66B9;

		DWORD* dword_5050FB = hook::pattern("D8 02 D9 1E D9 41 04 D8 60 04 D8 4C 24 10 D8 42 04 D8 40 04 D9 5E 04 D9 41 08 D8 60 08").get(0).get<DWORD>(0); //addresses from 1.1 exe
		DWORD  dword_505102 = (DWORD)dword_5050FB + 7;
		injector::MakeJMP(dword_5050FB, StopSignHook, true);
		jmpAddr4 = dword_505102;
	}
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}

