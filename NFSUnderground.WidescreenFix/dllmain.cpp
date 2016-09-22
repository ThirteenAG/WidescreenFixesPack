#include "..\includes\stdafx.h"
#include "..\includes\CFileMgr.h"
#include "..\includes\hooking\Hooking.Patterns.h"
#include <algorithm>
HWND hWnd;
bool bDelay;

bool bFixHUD, bFixFOV;
bool bDisableCutsceneBorders;
int ResX;
int ResY;
float hor3DScale, ver3DScale;
float fHudScaleX, fHudScaleY;
float fHudPosX;
float MinimapPosX, MinimapPosY;
bool bHudWidescreenMode;
bool bFMVWidescreenMode;
bool bXbox360Scaling;
bool bAddHudOffset;
char* szCustomUserFilesDirectoryInGameDir;

HRESULT WINAPI SHGetFolderPathAHook(HWND /*hwnd*/, int /*csidl*/, HANDLE /*hToken*/, DWORD /*dwFlags*/, LPSTR pszPath)
{
	CreateDirectory(szCustomUserFilesDirectoryInGameDir, NULL);
	strcpy(pszPath, szCustomUserFilesDirectoryInGameDir);
	return S_OK;
}

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

	//if (HudPosX.fPos < -320.0f)
	//{
	//	HudPosX.fPos -= 10000.0f;
	//}
	//else
	//{
	//	if (HudPosX.fPos > 640.0f)
	//	{
	//		HudPosX.fPos += 10000.0f;
	//	}
	//}

	auto it = std::find_if(HudCoords.begin(), HudCoords.end(),
		[&cc = CDatEntry(HudPosX.dwPos, HudPosY.dwPos, 0.0f, 0.0f)]
	(const CDatEntry& cde) -> bool { return (cc.dwPosX == cde.dwPosX && cc.dwPosY == cde.dwPosY); });

	if (it != HudCoords.end())
	{
		HudPosX.fPos += it->fOffsetX;
		HudPosY.fPos += it->fOffsetY;

		if (bAddHudOffset)
		{
			HudPosX.fPos -= MinimapPosX;
			HudPosX.fPos += fHudPosX;
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

void __declspec(naked) LapsHook()
{
	_asm
	{
		mov		eax, [ebx + 1Ch]
		mov		edx, [ebx + 20h]
		mov		HudPosX.dwPos, eax
		mov		HudPosY.dwPos, edx
		call	WidescreenHud
		mov		eax, HudPosX.dwPos
		mov		edx, HudPosY.dwPos
		mov		[ebx + 1Ch], eax
		mov		[ebx + 20h], edx
		fadd	dword ptr [ebx + 1Ch]
		mov		edx, [esp + 18h]
		jmp		jmpAddr4
	}
}

DWORD jmpAddr5;
void __declspec(naked) FOVHook()
{
	_asm
	{
		mov dword ptr[esp + 20h], 3F800000h
		fld ds : ver3DScale
		jmp jmpAddr5
	}
}

DWORD WINAPI Init(LPVOID)
{
	CIniReader iniReader("");
	ResX = iniReader.ReadInteger("MAIN", "ResX", 0);
	ResY = iniReader.ReadInteger("MAIN", "ResY", 0);
	bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) == 1;
	bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) == 1;
	bXbox360Scaling = iniReader.ReadInteger("MAIN", "Xbox360Scaling", 1) == 1;
	bDisableCutsceneBorders = iniReader.ReadInteger("MAIN", "bDisableCutsceneBorders", 1) == 1;	
	bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) == 1;
	bFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1) == 1;
	szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MISC", "CustomUserFilesDirectoryInGameDir", "");
	static int nImproveGamepadSupport = iniReader.ReadInteger("MISC", "ImproveGamepadSupport", 0);
	bool bCustomUsrDir = false;
	if (strncmp(szCustomUserFilesDirectoryInGameDir, "0", 1) != 0)
		bCustomUsrDir = true;

	if (!ResX || !ResY) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		ResX = info.rcMonitor.right - info.rcMonitor.left;
		ResY = info.rcMonitor.bottom - info.rcMonitor.top;
	}

	auto pattern = hook::pattern("68 20 03 00 00 BE 58 02 00 00");
	if (!(pattern.size() > 0) && !bDelay)
	{
		bDelay = true;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
		return 0;
	}

	if (bDelay)
	{
		while (!(pattern.size() > 0))
			pattern = hook::pattern("68 20 03 00 00 BE 58 02 00 00");
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
	if (bFixHUD)
	{
		fHudScaleX = (1.0f / ResX * (ResY / 480.0f)) * 2.0f;
		fHudPosX = 640.0f / (640.0f * fHudScaleX);

		DWORD* dword_6CC914 = *hook::pattern("D8 0D ? ? ? ? 57 0F B6 B8 81 00 00 00 D8 ? ? ? ? ?").get(0).get<DWORD*>(2);
		injector::WriteMemory<float>(dword_6CC914, fHudScaleX, true);

		//fHudScaleY = *(float*)0x6CCBA4;
		//CPatch::SetFloat(0x79AC14, fHudScaleY);

		for (size_t i = 0; i < 4; i++)
		{
			auto pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43");
			if (pattern.size() > 0)
			{
				DWORD* dword_4F20A3 = pattern.get(0).get<DWORD>(7);
				injector::WriteMemory<float>(dword_4F20A3, fHudPosX, true);
			}
			else
			{
				pattern = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43");
				if (pattern.size() > 0)
				{
					DWORD* dword_4F2287 = pattern.get(0).get<DWORD>(4);
					injector::WriteMemory<float>(dword_4F2287, fHudPosX, true);
				}
			}
		}

		DWORD* dword_4F1E2D = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43").get(0).get<DWORD>(4);
		injector::WriteMemory<float>(dword_4F1E2D, fHudPosX, true);
		DWORD* dword_4F26E5 = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43").get(0).get<DWORD>(4);
		injector::WriteMemory<float>(dword_4F26E5, fHudPosX, true);
		
		DWORD* dword_6CC910 = *hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 41 75 0F D9 45 FC").get(0).get<DWORD*>(2);
		injector::WriteMemory<float>(dword_6CC910, fHudPosX, true);

		DWORD* dword_5CC109 = nullptr;
		DWORD dword_5CC119 = 0;
		DWORD dword_5CC0F9 = 0;
		DWORD dword_5CC129 = 0;
		auto pattern = hook::pattern("C7 44 24 7C 00 00 48 43 C7 84 24 80 00 00 00 00 00 70 41 C7 84 24 8C 00 00 00 00 00 48 43 C7 84 24 90 00 00 00 00 00 BE 42");
		if (pattern.size() > 0)
		{
			dword_5CC109 = pattern.get(0).get<DWORD>(4); //rearview mirror (nfsu2 addresses)
			dword_5CC119 = (DWORD)dword_5CC109 + 16 + 6;
			dword_5CC0F9 = (DWORD)dword_5CC109 - 16;
			dword_5CC129 = (DWORD)dword_5CC109 + 32 + 12;
		}
		else
		{
			pattern = hook::pattern("C7 84 24 8C 00 00 00 00 00 48 43 C7 84 24 90 00 00 00 00 00 70 41 C7 84 24 9C 00 00 00 00 00 48 43 C7 84 24 A0 00 00 00 00 00 BE 42");
			if (pattern.size() > 0)
			{
				dword_5CC109 = pattern.get(0).get<DWORD>(7);
				dword_5CC119 = (DWORD)dword_5CC109 + 16 + 6;
				dword_5CC0F9 = (DWORD)dword_5CC109 - 16 - 6;
				dword_5CC129 = (DWORD)dword_5CC109 + 44;
			}
		}
		injector::WriteMemory<float>(dword_5CC109, (fHudPosX - 320.0f) + 200.0f, true);
		injector::WriteMemory<float>(dword_5CC119, (fHudPosX - 320.0f) + 200.0f, true);
		injector::WriteMemory<float>(dword_5CC0F9, (fHudPosX - 320.0f) + 440.0f, true);
		injector::WriteMemory<float>(dword_5CC129, (fHudPosX - 320.0f) + 440.0f, true);
	}

	if (bFixFOV)
	{
		hor3DScale = 1.0f / ((1.0f * ((float)ResX / (float)ResY)) / (4.0f / 3.0f));
		ver3DScale = 0.75f;

		DWORD* dword_40DEBB = hook::pattern("D8 3D ? ? ? ? D9 1F E8 ? ? ? ? D9 5C 24 24 8D 85 00 40 00 00").get(0).get<DWORD>(2);
		injector::WriteMemory(dword_40DEBB, &hor3DScale, true);

		DWORD* dword_40DDD2 = hook::pattern("DB 40 18 C7 44 24 20 00 00 80 3F DA 70 14").get(0).get<DWORD>(0);
		jmpAddr5 = (DWORD)dword_40DDD2 + 14;
		injector::MakeJMP(dword_40DDD2, FOVHook, true);

		// FOV being different in menus and in-game fix
		static float f06 = 0.6f;
		DWORD* dword_40DE43 = hook::pattern("D8 0D ? ? ? ? 8B E8 E8 ? ? ? ? DD D8 D9 44 24 0C 8D 7E 40").get(0).get<DWORD>(2);
		injector::WriteMemory(dword_40DE43, &f06, true);

		static float f1234 = 1.234f;
		DWORD* dword_40DEEC = hook::pattern("D8 3D ? ? ? ? D9 E0 D9 5E 54 D9 44 24 20").get(0).get<DWORD>(2);
		injector::WriteMemory(dword_40DEEC, &f1234, true);

		if (bXbox360Scaling)
		{
			hor3DScale /= 1.0511562719f;
		}
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

		DWORD* dword_4F6DAB = hook::pattern("D8 43 1C 8B 54 24 18 8B 44 24 10 D9 1A 8B 4E 68 50 51 57").get(0).get<DWORD>(0);
		DWORD  dword_4F6DB2 = (DWORD)dword_4F6DAB + 7;
		injector::MakeJMP(dword_4F6DAB, LapsHook, true);
		jmpAddr4 = dword_4F6DB2;
	}

	if (bCustomUsrDir)
	{
		char			moduleName[MAX_PATH];
		GetModuleFileName(NULL, moduleName, MAX_PATH);
		char* tempPointer = strrchr(moduleName, '\\');
		*(tempPointer + 1) = '\0';
		strcat(moduleName, szCustomUserFilesDirectoryInGameDir);
		strcpy(szCustomUserFilesDirectoryInGameDir, moduleName);

		union {
			DWORD* Int;
			unsigned char Byte[4];
		} dword_41C481;

		dword_41C481.Int = *hook::pattern("68 ? ? ? ? 6A 00 6A 00 68 23 80 00 00 6A 00 FF 15").get(0).get<DWORD*>(16+2);
		char pattern_str[20];

		sprintf(pattern_str, "%02X %02X %02X %02X %02X %02X", 0xFF, 0x15, dword_41C481.Byte[0], dword_41C481.Byte[1], dword_41C481.Byte[2], dword_41C481.Byte[3]);
		
		auto pattern = hook::pattern(pattern_str);
		//MessageBox(0, pattern_str, pattern_str, 0);
		for (size_t i = 0; i < pattern.size(); i++)
		{
			DWORD* dword_6CBF17 = pattern.get(i).get<DWORD>(0);
			injector::MakeCALL((DWORD)dword_6CBF17, SHGetFolderPathAHook, true);
			injector::MakeNOP((DWORD)dword_6CBF17 + 5, 1, true);
		}
	}

	if (nImproveGamepadSupport)
	{
		static char* GLOBALB = "scripts\\XBOXGLOBALB.BUN";
		static char* GLOBALB2 = "scripts\\PSGLOBALB.BUN";
		static char* FrontB = "scripts\\XBOXFrontB.BUN";
		static char* FrontB2 = "scripts\\PSFrontB.BUN";

		pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 8B F0 83 C4 14 33 C9 33 C0"); //0x6B6E81 globalb
		if (nImproveGamepadSupport == 2)
			injector::WriteMemory(pattern.get(0).get<uint32_t>(1), GLOBALB2, true);
		else
			injector::WriteMemory(pattern.get(0).get<uint32_t>(1), GLOBALB, true);

		pattern = hook::pattern("68 ? ? ? ? C6 05 ? ? ? ? 01 C7 ? ? ? ? ? ? ? ? ? E8 ? ? ? ? 8B F0 83 C4 14 33 C9"); //0x6B6E81 frontb
		if (nImproveGamepadSupport == 2)
			injector::WriteMemory(pattern.get(0).get<uint32_t>(1), FrontB2, true);
		else
			injector::WriteMemory(pattern.get(0).get<uint32_t>(1), FrontB, true);

		struct PadState
		{
			int32_t LSAxis1;
			int32_t LSAxis2;
			int32_t LTRT;
			int32_t RSAxis1;
			int32_t RSAxis2;
			uint8_t unk[28];
			int8_t A;
			int8_t B;
			int8_t X;
			int8_t Y;
			int8_t LB;
			int8_t RB;
			int8_t Select;
			int8_t Start;
			int8_t LSClick;
			int8_t RSClick;
		};

		static bool Zstate, Pstate, Tstate, Dstate, Qstate, Cstate;
		pattern = hook::pattern("7C ? 5F 5D 5E 33 C0 5B C2 08 00"); //0x41989E
		injector::WriteMemory(pattern.get(0).get<uint32_t>(8 + 5), 0x900008C2, true);
		//static uintptr_t ButtonsState = (uintptr_t)*hook::pattern("").get(0).get<uint32_t*>(8); //0x
		static int32_t* nGameState = (int32_t*)*hook::pattern("83 3D ? ? ? ? 06 ? ? A1").get(0).get<uint32_t*>(2); //0x77A920
		struct CatchPad
		{
			void operator()(injector::reg_pack& regs)
			{	
				PadState* PadKeyPresses = *(PadState**)(regs.esp + 0x4);
				//Keyboard 
				//006F9358 backspace
				//006F931C enter
				if (*nGameState == 3)
				{
					if (PadKeyPresses->LSClick && PadKeyPresses->RSClick)
					{
						if (!Qstate)
						{
							keybd_event(VkKeyScan('Q'), 0, KEYEVENTF_EXTENDEDKEY, 0);
							keybd_event(VkKeyScan('Q'), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
						}
						Qstate = true;
					}
					else
						Qstate = false;

					if (PadKeyPresses->Y)
					{
						if (!Zstate)
						{
							keybd_event(VkKeyScan('Z'), 0, KEYEVENTF_EXTENDEDKEY, 0);
							keybd_event(VkKeyScan('Z'), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
						}
						Zstate = true;
					}
					else
						Zstate = false;

					if (PadKeyPresses->X)
					{
						if (!Pstate)
						{
							keybd_event(VkKeyScan('P'), 0, KEYEVENTF_EXTENDEDKEY, 0);
							keybd_event(VkKeyScan('P'), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
						}
						Pstate = true;
					}
					else
						Pstate = false;

					if (PadKeyPresses->LSClick && !PadKeyPresses->RSClick)
					{
						if (!Tstate)
						{
							keybd_event(VkKeyScan('T'), 0, KEYEVENTF_EXTENDEDKEY, 0);
							keybd_event(VkKeyScan('T'), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
						}
						Tstate = true;
					}
					else
						Tstate = false;

					if (PadKeyPresses->RSClick && !PadKeyPresses->LSClick)
					{
						if (!Dstate)
						{
							keybd_event(VkKeyScan('D'), 0, KEYEVENTF_EXTENDEDKEY, 0);
							keybd_event(VkKeyScan('D'), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
						}
						Dstate = true;
					}
					else
						Dstate = false;

					if (PadKeyPresses->Select)
					{
						if (!Cstate)
						{
							keybd_event(VkKeyScan('C'), 0, KEYEVENTF_EXTENDEDKEY, 0);
							keybd_event(VkKeyScan('C'), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
						}
						Cstate = true;
					}
					else
						Cstate = false;
				}
			}
		}; injector::MakeInline<CatchPad>(pattern.get(0).get<uint32_t>(8));
	}
	return 0;
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init(NULL);
	}
	return TRUE;
}

