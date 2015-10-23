#include "..\includes\stdafx.h"
#include "..\includes\CFileMgr.h"
#include "..\includes\hooking\Hooking.Patterns.h"

HWND hWnd;

bool HudFix;
bool DisableCutsceneBorders;
int ResX;
int ResY;
float horFOV, verFOV;
float fHudScaleX, fHudScaleY;
float fHudPosX;
float MinimapPosX, MinimapPosY;
float verFovCorrectionFactor;
bool bAddHudOffset;

union HudPos
{
	DWORD dwPos;
	float fPos;
} HudPosX, HudPosY;

class CDatEntry
{
public:
	DWORD dwPosX;
	DWORD dwPosY;
	float fOffsetX;
	float fOffsetY;

	CDatEntry(DWORD posx, DWORD posy, float fOffsetX, float fOffsetY)
		: dwPosX(posx), dwPosY(posy), fOffsetX(fOffsetX), fOffsetY(fOffsetY)
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
				DWORD PosX, PosY;
				float fOffsetX, fOffsetY;
				sscanf(pLine, "%*s %x %x %f %f %*f %*f", &PosX, &PosY, &fOffsetX, &fOffsetY);

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

	for (auto it = HudCoords.cbegin(); it != HudCoords.cend(); ++it)
	{
		if (HudPosX.dwPos == it->dwPosX && HudPosY.dwPos == it->dwPosY)
		{
			HudPosX.fPos += it->fOffsetX;
			HudPosY.fPos += it->fOffsetY;

			if (bAddHudOffset)
			{
				HudPosX.fPos -= MinimapPosX;
				HudPosX.fPos += fHudPosX;
			}
			break;
		}
	}

	__asm popfd
	__asm popad
}

DWORD jmpAddr, jmpAddr2, jmpAddr3, jmpAddr4;
void __declspec(naked) HudHook()
{
	_asm
	{
		mov eax, [edi+1Ch]
		mov ecx, [edi+20h]
		mov HudPosX.dwPos, eax
		mov HudPosY.dwPos, ecx
		call WidescreenHud
		mov eax, HudPosX.dwPos
		mov ecx, HudPosY.dwPos
		jmp jmpAddr
	}
}

void __declspec(naked) MinimapHook1()
{
	_asm
	{
		mov     ecx, [ebp + 8]
		mov		edx, [ecx + 68h]
		mov		HudPosX.dwPos, edx
		mov		edx, [ecx + 6Ch]
		mov		HudPosY.dwPos, edx
		mov     bAddHudOffset, 1
		call	WidescreenHud
		mov     bAddHudOffset, 0
		mov		edx, HudPosX.dwPos
		mov		[ecx + 68h], edx
		mov		edx, HudPosY.dwPos
		mov		[ecx + 6Ch], edx
		
		fld     dword ptr [ecx+68h]
		jmp		jmpAddr2
	}
}

void __declspec(naked) MinimapHook2()
{
	_asm
	{
		mov		edx, [eax + 68h]
		mov		HudPosX.dwPos, edx
		mov		edx, [eax + 6Ch]
		mov		HudPosY.dwPos, edx
		mov     bAddHudOffset, 1
		call	WidescreenHud
		mov     bAddHudOffset, 0
		mov		edx, HudPosX.dwPos
		mov		[eax + 68h], edx
		mov		edx, HudPosY.dwPos
		mov		[eax + 6Ch], edx
		
		fld     dword ptr[eax + 68h]
		mov     ecx, [ebp + 0Ch]
		jmp		jmpAddr3
	}
}

void Init()
{
	CIniReader iniReader("");
	ResX = iniReader.ReadInteger("MAIN", "ResX", 0);
	ResY = iniReader.ReadInteger("MAIN", "ResY", 0);
	HudFix = iniReader.ReadInteger("MAIN", "HudFix", 1) == 1;
	verFovCorrectionFactor = iniReader.ReadFloat("MAIN", "verFovCorrectionFactor", 0.04f);
	DisableCutsceneBorders = iniReader.ReadInteger("MAIN", "DisableCutsceneBorders", 1) == 1;	
	bool bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) == 1;
	bool bFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1) == 1;

	if (!ResX || !ResY) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		ResX = info.rcMonitor.right - info.rcMonitor.left;
		ResY = info.rcMonitor.bottom - info.rcMonitor.top;
	}

	//menu
	DWORD* dword_408A25 = hook::pattern("68 20 03 00 00 BE 58 02 00 00").get(0).get<DWORD>(1);
	injector::WriteMemory(dword_408A25, ResX, true);
	DWORD dword_408A2A = (DWORD)dword_408A25 + 5;
	injector::WriteMemory(dword_408A2A, ResY, true);

	DWORD* dword_4494C5 = hook::pattern("68 20 03 00 00 BE 58 02 00 00").get(0).get<DWORD>(1);
	injector::WriteMemory(dword_4494C5, ResX, true);
	DWORD dword_4494CA = (DWORD)dword_4494C5 + 5;
	injector::WriteMemory(dword_4494CA, ResY, true);

	//game
	DWORD* dword_408783 = hook::pattern("B8 20 03 00 00 BE 58 02 00 00 77 75").get(0).get<DWORD>(1);
	injector::WriteMemory(dword_408783, ResX, true);
	DWORD dword_408788 = (DWORD)dword_408783 + 5;
	injector::WriteMemory(dword_408788, ResY, true);

	injector::MakeNOP(dword_408788 + 4, 9, true);

	DWORD* dword_408796 = hook::pattern("B8 80 02 00 00 BE E0 01 00 00 50").get(0).get<DWORD>(1);
	injector::WriteMemory(dword_408796, ResX, true);
	DWORD dword_40879B = (DWORD)dword_408796 + 5;
	injector::WriteMemory(dword_40879B, ResY, true);
	DWORD* dword_4087AF = hook::pattern("B8 20 03 00 00 BE 58 02 00 00 50").get(0).get<DWORD>(1);
	injector::WriteMemory(dword_4087AF, ResX, true);
	DWORD dword_4087B4 = (DWORD)dword_4087AF + 5;
	injector::WriteMemory(dword_4087B4, ResY, true);
	DWORD* dword_4087C8 = hook::pattern("B8 00 04 00 00 BE 00 03 00 00 50").get(0).get<DWORD>(1);
	injector::WriteMemory(dword_4087C8, ResX, true);
	DWORD dword_4087CD = (DWORD)dword_4087C8 + 5;
	injector::WriteMemory(dword_4087CD, ResY, true);
	DWORD* dword_4087E1 = hook::pattern("B8 00 05 00 00 BE C0 03 00 00 50").get(0).get<DWORD>(1);
	injector::WriteMemory(dword_4087E1, ResX, true);
	DWORD dword_4087E6 = (DWORD)dword_4087E1 + 5;
	injector::WriteMemory(dword_4087E6, ResY, true);

	//DWORD* dword_4087FA = hook::pattern("BE 00 04 00 00 B8 00 05 00 00 50").get(0).get<DWORD>(1);
	//injector::WriteMemory(dword_4087FA, ResX, true);
	//DWORD dword_4087FF = (DWORD)dword_4087E1 + 5;
	//injector::WriteMemory(dword_4087FF, ResY, true);


	//HUD
	if (HudFix)
	{
		fHudScaleX = (1.0f / ResX * (ResY / 480.0f)) * 2.0f;
		fHudPosX = 640.0f / (640.0f * fHudScaleX);

		DWORD* dword_6CC914 = *hook::pattern("D8 0D ? ? ? ? 57 0F B6 B8 81 00 00 00 D8 ? ? ? ? ?").get(0).get<DWORD*>(2);
		injector::WriteMemory<float>(dword_6CC914, fHudScaleX, true);

		//fHudScaleY = *(float*)0x6CCBA4;
		//CPatch::SetFloat(0x79AC14, fHudScaleY);

		for (size_t i = 0; i < 4; i++)
		{
			DWORD* dword_4F20A3 = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43").get(0).get<DWORD>(7);
			injector::WriteMemory<float>(dword_4F20A3, fHudPosX, true);
		}

		DWORD* dword_4F1E2D = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43").get(0).get<DWORD>(4);
		injector::WriteMemory<float>(dword_4F1E2D, fHudPosX, true);
		DWORD* dword_4F26E5 = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43").get(0).get<DWORD>(4);
		injector::WriteMemory<float>(dword_4F26E5, fHudPosX, true);
		
		DWORD* dword_6CC910 = *hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 41 75 0F D9 45 FC").get(0).get<DWORD*>(2);
		injector::WriteMemory<float>(dword_6CC910, fHudPosX, true);

		///injector::WriteMemory<float>(0x400000 + 0x14646B, fHudPosX, true);
	}

	if (verFovCorrectionFactor)
	{
		horFOV = 1.0f / ((1.0f * ((float)ResX / (float)ResY)) / (4.0f / 3.0f));
		verFOV = horFOV - verFovCorrectionFactor;

		DWORD* dword_40DEBB = hook::pattern("D8 3D ? ? ? ? D9 1F E8 ? ? ? ? D9 5C 24 24 8D 85 00 40 00 00").get(0).get<DWORD>(2);
		injector::WriteMemory(dword_40DEBB, &horFOV, true);
		DWORD* dword_40DEEC = hook::pattern("D8 3D ? ? ? ? D9 E0 D9 5E 54 D9 44 24 20 D8 4C 24 0C D9 E0 D9 5E 78").get(0).get<DWORD>(2);
		injector::WriteMemory(dword_40DEEC, &verFOV, true);
	}

	if (bFMVWidescreenMode)
	{
		DWORD* dword_4F1FC3 = hook::pattern("68 00 00 80 3F 68 00 00 00 3F 68 00 00 00 3F 68 00 00 00 BF 68 00 00 00 BF 50").get(0).get<DWORD>(6);
		injector::WriteMemory<float>(dword_4F1FC3 + 0, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
		injector::WriteMemory<float>((DWORD)dword_4F1FC3 + 5, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
		injector::WriteMemory<float>((DWORD)dword_4F1FC3 + 10, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
		injector::WriteMemory<float>((DWORD)dword_4F1FC3 + 15, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
	}

	if (bHudWidescreenMode && (((float)ResX / (float)ResY) > (4.0f / 3.0f)))
	{
		LoadDatFile();

		DWORD* dword_4F18CB = hook::pattern("8B 47 1C 8B 4F 20 89 84 24 10 01 00 00 8B 47 24 89 84 24 18 01 00 00 39 5C 24 04").get(0).get<DWORD>(0);
		DWORD  dword_4F18D1 = (DWORD)dword_4F18CB + 6;
		injector::MakeJMP(dword_4F18CB, HudHook, true);
		jmpAddr = dword_4F18D1;

		MinimapPosX = 320.0f;
		MinimapPosY = 240.0f;
		DWORD* dword_58E5BD = hook::pattern("D8 05 ? ? ? ? D9 5B 68 D8 05 ? ? ? ? D9 5B 6C D9 40 3C").get(0).get<DWORD>(2);
		injector::WriteMemory(dword_58E5BD, &MinimapPosX, true);
		DWORD dword_58E5C6 = (DWORD)dword_58E5BD + 9;
		injector::WriteMemory(dword_58E5C6, &MinimapPosY, true);

		DWORD* dword_590DDC = hook::pattern("8B 4D 08 D9 41 68 53 57 8B 5D 0C 8B F8").get(0).get<DWORD>(0);
		DWORD  dword_590DE2 = (DWORD)dword_590DDC + 6;
		injector::MakeJMP(dword_590DDC, MinimapHook1, true);
		jmpAddr2 = dword_590DE2;

		DWORD* dword_58FB1C = hook::pattern("D9 40 68 8B 4D 0C D8 60 70 53 56 8D 91 83 00 00 00 D9 54 24 34").get(0).get<DWORD>(0);
		DWORD  dword_58FB22 = (DWORD)dword_58FB1C + 6;
		injector::MakeJMP(dword_58FB1C, MinimapHook2, true);
		jmpAddr3 = dword_58FB22;
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

