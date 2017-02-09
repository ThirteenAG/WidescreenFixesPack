#include "stdafx.h"
#include "GTA\CFileMgr.h"
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
	char* DataFilePath = (char*)iniReader.GetIniPath().c_str();
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
	bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) == 1;
	bFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1) == 1;
	bDisableCutsceneBorders = iniReader.ReadInteger("MISC", "DisableCutsceneBorders", 1) == 1;
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
	
	auto pattern = hook::pattern("C7 00 80 02 00 00 C7 01 E0 01 00 00");
	if (!(pattern.size() > 0) && !bDelay)
	{
		bDelay = true;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
		return 0;
	}

	if (bDelay)
	{
		while (!(pattern.size() > 0))
			pattern = hook::pattern("C7 00 80 02 00 00 C7 01 E0 01 00 00");
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

	//HUD
	if (bFixHUD)
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

		if (bDisableCutsceneBorders)
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

	if (bFixFOV)
	{
		hor3DScale = 1.0f / ((1.0f * ((float)ResX / (float)ResY)) / (4.0f / 3.0f));
		ver3DScale = 0.75f;

		DWORD* dword_5C7FE8 = hook::pattern("D8 3D ? ? ? ? D9 5C 24 1C DB 44 24 2C D8 4C 24 28 D8 0D ? ? ? ? E8").get(0).get<DWORD>(2);
		injector::WriteMemory(dword_5C7FE8, &hor3DScale, true);

		DWORD* dword_5C7F56 = hook::pattern("DB 40 18 C7 44 24 20 00 00 80 3F DA 70 14").get(0).get<DWORD>(0);
		jmpAddr5 = (DWORD)dword_5C7F56 + 14;
		injector::MakeJMP(dword_5C7F56, FOVHook, true);

		// FOV being different in menus and in-game fix
		static float f06 = 0.6f;
		DWORD* dword_5C7FFA = hook::pattern("D8 0D ? ? ? ? E8 ? ? ? ? 8B D8 53").get(1).get<DWORD>(2);
		injector::WriteMemory(dword_5C7FFA, &f06, true);

		static float f1234 = 1.234f;
		DWORD* dword_5C801F = hook::pattern("D8 3D ? ? ? ? D9 5C 24 38 D9 44 24 24 D8 64 24 30 D8 7C 24 24 D9 5C 24 34").get(0).get<DWORD>(2);
		injector::WriteMemory(dword_5C801F, &f1234, true);

		if (bXbox360Scaling)
		{
			hor3DScale /= 1.0511562719f;
		}
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

	if (bCustomUsrDir)
	{
		char			moduleName[MAX_PATH];
		GetModuleFileName(NULL, moduleName, MAX_PATH);
		char* tempPointer = strrchr(moduleName, '\\');
		*(tempPointer + 1) = '\0';
		strcat(moduleName, szCustomUserFilesDirectoryInGameDir);
		strcpy(szCustomUserFilesDirectoryInGameDir, moduleName);

		auto pattern = hook::pattern("50 6A 00 6A 00 68 ? 80 00 00 6A 00");
		DWORD* dword_6CBF17 = pattern.get(0).get<DWORD>(12);
		injector::MakeCALL((DWORD)dword_6CBF17, SHGetFolderPathAHook, true);
		injector::MakeNOP((DWORD)dword_6CBF17 + 5, 1, true);
	}

	if (nImproveGamepadSupport)
	{
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

		static bool Zstate, Pstate, Tstate, Dstate, Qstate, Cstate, LBRACKETstate, RBRACKETstate;
		pattern = hook::pattern("5E 5D 5F 33 C0 5B 59 C2 04 00"); //5C8C5F
		injector::WriteMemory(pattern.get(1).get<uint32_t>(7 + 5), 0x900004C2, true);
		static int32_t* nGameState = (int32_t*)*hook::pattern("83 3D ? ? ? ? 06 ? ? A1").get(0).get<uint32_t*>(2);
		struct CatchPad
		{
			void operator()(injector::reg_pack& regs)
			{
				PadState* PadKeyPresses = *(PadState**)(regs.esp + 0x4);

				if (PadKeyPresses != nullptr && PadKeyPresses != (PadState*)0x1 && *nGameState == 3)
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
							keybd_event(VkKeyScan('P'), 0, KEYEVENTF_EXTENDEDKEY, 0);
							keybd_event(VkKeyScan('P'), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
						}
						Zstate = true;
					}
					else
						Zstate = false;
				}
			}
		}; injector::MakeInline<CatchPad>(pattern.get(1).get<uint32_t>(7));


		const char* MenuTexts[] = { "Quit Game", "Continue", "Back", "Reset Keys To Default", "Activate GPS", "Deactivate GPS", "Install Package", "Install Part", "Install Paint", "Install Decal", "Reset To Default", "Delete Tuned Car", "Logoff", "Cancel Changes", "Customize", "Host LAN Server", "Read Message", "Delete", "Test N2O Purge", "Accept", "Reset to default", "Select", "Open/Close Doors", "Open/Close Hood" }; // "Tip", 
		const char* MenuTextsPC[] = { "[Q] Quit Game", "[Enter] Continue", "[Esc] Back", "[P] Reset Keys To Default", "[C] Activate GPS", "[C] Deactivate GPS", "[Enter] Install Package", "[Enter] Install Part", "[Enter] Install Paint", "[Enter] Install Decal", "[C] Reset To Default", "[C] Delete Tuned Car", "[Esc] Logoff", "[C] Cancel Changes", "[C] Customize", "[C] Host LAN Server", "[Enter] Read Message", "[C] Delete", "[C] Test N2O Purge", "[Enter] Accept", "[C] Reset to default", "[Enter] Select", "[C] Open/Close Doors", "[C] Open/Close Hood" };
		const char* MenuTextsXBOX[] = { "(LS+RS) Quit Game", "(A) Continue", "(B) Back", "(Y) Reset Keys To Default", "(X) Activate GPS", "(X) Deactivate GPS", "(A) Install Package", "(A) Install Part", "(A) Install Paint", "(A) Install Decal", "(X) Reset To Default", "(X) Delete Tuned Car", "(B) Logoff", "(X) Cancel Changes", "(X) Customize", "(X) Host LAN Server", "(A) Read Message", "(X) Delete", "(X) Test N2O Purge", "(A) Accept", "(X) Reset to default", "(A) Select", "(X) Open/Close Doors", "(X) Open/Close Hood" };
		const char* MenuTextsPS[] = { "(L3+R3) Quit Game", "(Cross) Continue", "(Circle) Back", "(Triangle) Reset Keys To Default", "(Square) Activate GPS", "(Square) Deactivate GPS", "(Cross) Install Package", "(Cross) Install Part", "(Cross) Install Paint", "(Cross) Install Decal", "(Square) Reset To Default", "(Square) Delete Tuned Car", "(Circle) Logoff", "(Square) Cancel Changes", "(Square) Customize", "(Square) Host LAN Server", "(Cross) Read Message", "(Square) Delete", "(Square) Test N2O Purge", "(Cross) Accept", "(Square) Reset to default", "(Cross) Select", "(Square) Open/Close Doors", "(Square) Open/Close Hood" };

		static std::vector<std::string> vMenuStrings(MenuTexts, std::end(MenuTexts));
		static std::vector<std::string> vMenuStringsPC(MenuTextsPC, std::end(MenuTextsPC));
		static std::vector<std::string> vMenuStringsXBOX(MenuTextsXBOX, std::end(MenuTextsXBOX));
		static std::vector<std::string> vMenuStringsPS(MenuTextsPS, std::end(MenuTextsPS));

		pattern = hook::pattern("8B 0D ? ? ? ? 85 C9 50 74 12 8B 44"); //0x5124DD (v1.1)
		static auto dword_8383DC = *pattern.get(0).get<uint32_t>(2);
		struct Buttons
		{
			void operator()(injector::reg_pack& regs)
			{
				regs.ecx = dword_8383DC;

				auto pszStr = (char*)regs.eax;
				auto it = std::find(vMenuStrings.begin(), vMenuStrings.end(), pszStr);
				auto i = std::distance(vMenuStrings.begin(), it);

				if (it != vMenuStrings.end())
				{
					if (nImproveGamepadSupport == 3)
						regs.eax = (uint32_t)vMenuStringsPC[i].c_str();
					else
						if (nImproveGamepadSupport != 2)
							regs.eax = (uint32_t)vMenuStringsXBOX[i].c_str();
						else
							regs.eax = (uint32_t)vMenuStringsPS[i].c_str();
				}
			}
		}; injector::MakeInline<Buttons>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(6));
	}

	const wchar_t* ControlsTexts[] = { L" 2", L" 3", L" 4", L" 5", L" 6", L" 7", L" 8", L" 9", L" 10", L" 1", L" Up", L" Down", L" Left", L" Right", L"X Rotation", L"Y Rotation", L"X Axis", L"Y Axis", L"Z Axis", L"Hat Switch" };
	const wchar_t* ControlsTextsXBOX[] = { L"B", L"X", L"Y", L"LB", L"RB", L"View (Select)", L"Menu (Start)", L"Left stick", L"Right stick", L"A", L"D-pad Up", L"D-pad Down", L"D-pad Left", L"D-pad Right", L"Right stick Left/Right", L"Right stick Up/Down", L"Left stick Left/Right", L"Left stick Up/Down", L"Left trigger / Right trigger", L"D-pad" };
	const wchar_t* ControlsTextsPS[] = { L"Circle", L"Square", L"Triangle", L"L1", L"R1", L"Select", L"Start", L"L3", L"R3", L"Cross", L"D-pad Up", L"D-pad Down", L"D-pad Left", L"D-pad Right", L"Right stick Left/Right", L"Right stick Up/Down", L"Left stick Left/Right", L"Left stick Up/Down", L"L2 / R2", L"D-pad" };

	static std::vector<std::wstring> Texts(ControlsTexts, std::end(ControlsTexts));
	static std::vector<std::wstring> TextsXBOX(ControlsTextsXBOX, std::end(ControlsTextsXBOX));
	static std::vector<std::wstring> TextsPS(ControlsTextsPS, std::end(ControlsTextsPS));

	pattern = hook::pattern("66 83 7C 24 08 00 5F 74 13 8D 4C 24 04 51"); //0x4ACF01
	injector::WriteMemory<uint8_t>(pattern.get(0).get<uint32_t>(0), 0x5F, true); //pop     edi
	static auto dword_4ACF1D = pattern.get(0).get<uint32_t>(28);
	struct Buttons
	{
		void operator()(injector::reg_pack& regs)
		{
			auto pszStr = (wchar_t*)(regs.esp + 0x4);
			if (wcslen(pszStr))
			{
				if (nImproveGamepadSupport)
				{
					auto it = std::find_if(Texts.begin(), Texts.end(), [&](const std::wstring& str) { std::wstring s(pszStr); return s.find(str) != std::wstring::npos; });
					auto i = std::distance(Texts.begin(), it);

					if (it != Texts.end())
					{
						if (nImproveGamepadSupport != 2)
							wcscpy(pszStr, TextsXBOX[i].c_str());
						else
							wcscpy(pszStr, TextsPS[i].c_str());
					}
				}
			}
			else
				*(uintptr_t*)(regs.esp - 4) = (uintptr_t)dword_4ACF1D;
		}
	}; injector::MakeInline<Buttons>(pattern.get(0).get<uint32_t>(1), pattern.get(0).get<uint32_t>(9));

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

