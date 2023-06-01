#include "stdafx.h"

struct Screen
{
	int Width;
	int Width43;
	int WidthFMV;
	int OffsetX;
	int Height;
	int HeightFMV;
	int HeightHUD;
	int OffsetY;
	bool AspectRatioAffected;
	bool PreserveHorizontalPosition;
	int ContinuePositionChange;
	float fWidth;
	float fHeight;
	float fInvWidth;
	float fInvHeight;
	float fInvWidth43;
	float fAspectRatio;
	float fConditionalAspectRatio;
	float fZoomFactor;
	float fHudScale;
} Screen;

float iniZoomFactor = 1.0f;
bool bLensFlareFix = true;
float fLensFlareScalar;
bool bShadowFix = true;
float fShadowScale = 1.7f;
float fDustWidth = 40.0f;

bool bFixAdvertiseWindows = true;
bool bFixStaffRoll = true;
float WindowBezelSize = 1.0f / 32.0f;
float ButtonBezelOffset = 8.0f;
float AdvWindowButtonTextYOffset = 16.0f;
float AdvWindowButton2TextYOffset = 8.0f;
float AdvWindowButton2Offset = 40.0f;
float AdvWindowButton3Offset = 72.0f;
float AdvStaffRollLogoScale = 1.0f;

void updateValues(const float& newWidth, const float& newHeight)
{
	//Screen resolution
	Screen.Width = newWidth;
	Screen.Height = newHeight;
	Screen.fWidth = static_cast<float>(Screen.Width);
	Screen.fHeight = static_cast<float>(Screen.Height);
	Screen.fInvWidth = 1.0f / Screen.fWidth;
	Screen.fInvHeight = 1.0f / Screen.fInvHeight;
	Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
	Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
	Screen.fInvWidth43 = 1.0f / (Screen.fHeight * (4.0f * 3.0f));

	if (Screen.fAspectRatio < Screen.fConditionalAspectRatio)
	{
		Screen.fZoomFactor = iniZoomFactor * (Screen.fAspectRatio / Screen.fConditionalAspectRatio);
	}
	Screen.fHudScale = ((4.0f / 3.0f) / Screen.fAspectRatio) * Screen.fZoomFactor;
	Screen.HeightHUD = (int)(Screen.Height * Screen.fZoomFactor);

	Screen.OffsetX = (int)(Screen.Width - (Screen.Width * Screen.fHudScale)) / 2;
	Screen.OffsetY = (int)(Screen.Height - (Screen.Height * Screen.fZoomFactor)) / 2;

	Screen.WidthFMV = Screen.Width;
	Screen.HeightFMV = Screen.Height;

	if (Screen.fAspectRatio < (4.0f / 3.0f))
	{
		Screen.HeightFMV = (int)(Screen.HeightFMV * (Screen.fAspectRatio / (4.0f / 3.0f)));
	}
	else
	{
		Screen.WidthFMV = (int)(Screen.WidthFMV / (Screen.fAspectRatio / (4.0f / 3.0f)));
	}

	fDustWidth = 40.0f * Screen.fAspectRatio;

	// write resolution vars
	*(uint32_t*)0x00A7793C = Screen.Width;
	*(uint32_t*)0x00A77940 = Screen.Height;
	*(uint32_t*)0x007C931C = Screen.Width;
	*(uint32_t*)0x007C9320 = Screen.Height;

	injector::WriteMemory(0x004463E2 + 1, Screen.Width, true);
	injector::WriteMemory(0x004463E7 + 1, Screen.Height, true);

	*(float*)0x0078A08C = 1.0f / (480.0f * Screen.fAspectRatio);

	*(float*)0x00AA7140 = Screen.fWidth;
	*(float*)0x00AA7144 = Screen.fHeight;

	if (bLensFlareFix)
	{
		*(float*)0x0078A184 = Screen.fWidth;
		*(float*)0x0078A180 = Screen.fHeight;
		// lens flare size
		fLensFlareScalar = Screen.fHeight / 480.0f;
	}
	if (bFixAdvertiseWindows)
	{
		float scalar = (Screen.fHeight / 480.0f);
		if (Screen.Width < Screen.Height)
		{
			scalar = Screen.fWidth / 480.0f;
		}
		WindowBezelSize = 1.0f / (32.0f * scalar);
		ButtonBezelOffset = 8.0f * scalar;
		AdvWindowButtonTextYOffset = 16.0f * scalar;
		AdvWindowButton2TextYOffset = 8.0f * scalar;
		AdvWindowButton2Offset = 40.0f * scalar;
		AdvWindowButton3Offset = 72.0f * scalar;
	}
}

unsigned int GameWndProcAddr = 0;
LRESULT(WINAPI* GameWndProc)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WSFixWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
	{
		if (!WindowedModeWrapper::bEnableWindowResize) return TRUE;

		updateValues((float)LOWORD(lParam), (float)HIWORD(lParam));
		return GameWndProc(hWnd, msg, wParam, lParam);
	}

	case WM_SIZING:
	{
		if (!WindowedModeWrapper::bEnableWindowResize) return TRUE;
		return GameWndProc(hWnd, msg, wParam, lParam);
	}
	
	}

	return GameWndProc(hWnd, msg, wParam, lParam);
}

BOOL WINAPI GetClientRectHook(HWND hWnd, LPRECT lpRect)
{
	lpRect->top = 0;
	lpRect->left = 0;
	
	lpRect->right = Screen.Width;
	lpRect->bottom = Screen.Height;
	return TRUE;
}

bool bStretchOnBootFlag = false;
uintptr_t ptrWindowSizeX = 0x00AA5384;
uintptr_t loc_6567BF = 0x6567BF;
void __declspec(naked) StretchOnBoot()
{
	if (!bStretchOnBootFlag)
	{
		bStretchOnBootFlag = true;
		*(uint32_t*)ptrWindowSizeX = 640; // invalidate it on purpose to trigger the window to update & device reset
	}
	_asm
	{
		mov edx, [esp + 0x20]
		mov eax, ptrWindowSizeX
		mov eax, [eax]
		jmp loc_6567BF
	}
}

uintptr_t loc_48F41F = 0x48F41F;
void __declspec(naked) LensFlareScale()
{
	_asm
	{
		fmul[fLensFlareScalar]
		mov eax, [esi + 0x1C]
		fmul dword ptr[esi + 0x10]
		jmp loc_48F41F
	}
}


uintptr_t DemoRestoreExit1 = 0x00456990;
void __declspec(naked) RestoreDemos()
{
	_asm
	{
		inc ecx
		cmp cl, 06
		cmovge ecx, ebx
		//cmp ecx, ebx
		//jle thyjle
		//xor ecx, ecx
		//thyjle:
		jmp DemoRestoreExit1
	}
}

#pragma runtime_checks( "", off )
float TestFloat1 = 1.0f;
float TestFloat2 = 1.0f;
float TestFloat3 = 1.0f;
float TestFloat4 = 1.0f;

namespace AdvertiseWindowFix
{
	struct Vector2
	{
		float x;
		float y;
	};


	char* AdvWindowObj;
	void AdvWindowConstructorHook()
	{
		//printf("ADV WINDOW CONSTRUCTOR!!!\n");
		AdvWindowObj = (char*)malloc(0x848);
	}

	void __stdcall AdvWindowDestructorHook(char unk)
	{
		void* that;
		_asm mov that, ecx

		free(AdvWindowObj);

		return reinterpret_cast<void(__thiscall*)(void*, char)>(0x00456F90)(that, unk);
	}

	uintptr_t sub_456AA0 = 0x456AA0;
	void __declspec(naked) hkAdvWindowConstructor()
	{
		_asm
		{
			pushad
			call AdvWindowConstructorHook
			popad
			jmp sub_456AA0
		}
	}

	uintptr_t AdvertiseWindowDrawFunc2Addr = 0x004575C0;
	// this is a fastcall
	// arg0 = eax
	// arg1 = ecx
	void __stdcall AdvertiseWindowDrawFunc2(Vector2* a0, Vector2* a1, uintptr_t a2)
	{
		_asm
		{
			push a2
			mov ecx, a1
			mov eax, a0
			call AdvertiseWindowDrawFunc2Addr
		}
	}

	void __stdcall AdvertiseWindowDrawHook2(uintptr_t a2)
	{
		// EAX = pos
		// ECX = size
		Vector2* inPos, * inSize;
		_asm
		{
			mov inPos, eax
			mov inSize, ecx
		}


		Vector2 newPos, newSize;
		memcpy(&newPos, inPos, sizeof(Vector2));
		memcpy(&newSize, inSize, sizeof(Vector2));

		float Xscale = Screen.Width43 / 640.0f;
		float Yscale = Screen.fHeight / 480.0f;
		float Yscalesize = Yscale;

		if (Screen.Width < Screen.Height)
		{
			Xscale = Screen.fWidth / 640.0f;
			Yscalesize = Screen.fWidth / 480.0f;
		}

		newSize.x *= Yscalesize; // Xsize
		newSize.y *= Yscalesize; // Ysize

		newPos.x *= Xscale; // Xpos
		if (Screen.fAspectRatio != (4.0f / 3.0f))
		{
			newPos.x += static_cast<float>((Screen.Width - Screen.Width43) / 2.0f);
			if (Screen.fAspectRatio < (4.0f / 3.0f))
				if (newPos.x < 0) newPos.x = 0;
		}
		newPos.y *= Yscale; // Ypos

		return AdvertiseWindowDrawFunc2(&newPos, &newSize, a2);
	}

	uintptr_t AdvertiseWindowDrawFunc3Addr = 0x457710;
	// this is a fastcall
	// arg0 = eax
	// arg1 = ecx
	void __stdcall AdvertiseWindowDrawFunc3(Vector2* a0, Vector2* a1, uintptr_t a2)
	{
		_asm
		{
			push a2
			mov ecx, a1
			mov eax, a0
			call AdvertiseWindowDrawFunc3Addr
		}
	}

	void __stdcall AdvButtonDrawHook(uintptr_t a2)
	{
		// EAX = pos
		// ECX = size
		Vector2* inPos, * inSize;
		_asm
		{
			mov inPos, eax
			mov inSize, ecx
		}

		Vector2 newSize;
		memcpy(&newSize, inSize, sizeof(Vector2));

		float Yscalesize = Screen.fHeight / 480.0f;

		if (Screen.Width < Screen.Height)
			Yscalesize = Screen.fWidth / 480.0f;

		newSize.y *= Yscalesize;

		// since the button is shifted already to the right by the bezel size, we disregard the bezel size on the left, so it's half the bezel size!
		newSize.x -= ((32.0f * Yscalesize) / 2) - 16.0f;

		return AdvertiseWindowDrawFunc3(inPos, &newSize, a2);
	}

	void __stdcall AdvWindowDrawHook()
	{
		uintptr_t that;
		_asm mov that, ecx

		// make a copy of the object
		memcpy(AdvWindowObj, (void*)that, 0x848);

		float Xscale = Screen.Width43 / 640.0f;
		float Yscale = Screen.fHeight / 480.0f;
		float Yscalesize = Yscale;

		if (Screen.Width < Screen.Height)
		{
			Xscale = Screen.fWidth / 640.0f;
			Yscalesize = Screen.fWidth / 480.0f;
		}

		*(float*)(&AdvWindowObj[0x48]) *= Yscalesize; // Xsize
		*(float*)(&AdvWindowObj[0x4C]) *= Yscalesize; // Ysize
		*(float*)(&AdvWindowObj[0x3C]) *= Xscale; // Xpos
		if (Screen.fAspectRatio != (4.0f / 3.0f))
		{
			*(float*)(&AdvWindowObj[0x3C]) += static_cast<float>((Screen.Width - Screen.Width43) / 2.0f);
			if (Screen.fAspectRatio < (4.0f / 3.0f))
				if (*(float*)(&AdvWindowObj[0x3C]) < 0) *(float*)(&AdvWindowObj[0x3C]) = 0;
		}
		*(float*)(&AdvWindowObj[0x40]) *= Yscale; // Ypos

		reinterpret_cast<void(__thiscall*)(char*)>(0x004570D0)(AdvWindowObj);
	}
}

namespace AdvStaffRollFix
{
	struct typeStaffrollTextObj
	{
		uint32_t unk1;
		uint32_t unk2;
		float posX;
		float posY;
		float size;
		uint32_t unk4;
	};

	char* StaffrollObjCopy;
	typeStaffrollTextObj* scaledStaffRollObjs;
	void __stdcall AdvStaffrollConstructorHook(char* StaffrollObj)
	{
		StaffrollObjCopy = (char*)malloc(0xA4);

		int objCount = *(uint32_t*)(&StaffrollObj[0x88]);
		uint32_t TextObjSize = sizeof(typeStaffrollTextObj) * objCount;

		typeStaffrollTextObj* TextObjs = *(typeStaffrollTextObj**)(&StaffrollObj[0x84]);
		scaledStaffRollObjs = (typeStaffrollTextObj*)malloc(TextObjSize);
	}

	uintptr_t sub_454F10 = 0x454F10;
	void __declspec(naked) hkStaffrollConstructor()
	{
		_asm
		{
			call sub_454F10
			pushad
			push esi
			call AdvStaffrollConstructorHook
			popad
			retn
		}
	}

	void __stdcall AdvStaffrollDestructorHook(char unk)
	{
		void* that;
		_asm mov that, ecx

		free(StaffrollObjCopy);
		free(scaledStaffRollObjs);

		return reinterpret_cast<void(__thiscall*)(void*, char)>(0x4544B0)(that, unk);
	}

	void __stdcall AdvStaffrollDrawHook()
	{
		char* that;
		_asm mov that, ecx

		// make a copy of the object
		memcpy(StaffrollObjCopy, that, 0xA4);

		uint32_t objtype = *(uint32_t*)(&StaffrollObjCopy[0x28]);

		if (objtype == 5)
		{
			int objCount = *(uint32_t*)(&StaffrollObjCopy[0x88]);
			uint32_t TextObjSize = sizeof(typeStaffrollTextObj) * objCount;

			typeStaffrollTextObj* TextObjs = *(typeStaffrollTextObj**)(&StaffrollObjCopy[0x84]);
			memcpy(scaledStaffRollObjs, TextObjs, TextObjSize);

			for (int i = 0; i < objCount; i++)
			{
				float scalar = Screen.fHeight / 480.0f;
				float Xscale = Screen.Width / 640.0f;

				if (Screen.Width < Screen.Height)
				{
					scalar = Screen.fWidth / 480.0f;
					Xscale = Screen.fWidth / 640.0f;
				}

				scaledStaffRollObjs[i].posX *= Xscale;
				scaledStaffRollObjs[i].posY *= scalar;
			}

			*(typeStaffrollTextObj**)(&StaffrollObjCopy[0x84]) = scaledStaffRollObjs;
		}

		reinterpret_cast<void(__thiscall*)(char*)>(0x4545F0)(StaffrollObjCopy);
	}

	uintptr_t AdvStaffrollLogoFuncAddr = 0x00454FB0;
	// this is a fastcall
	// arg0 = esi
	void __stdcall AdvStaffrollLogoFunc(void* obj, float sizeX, float sizeY)
	{
		_asm
		{
			push sizeY
			push sizeX
			mov esi, obj
			call AdvStaffrollLogoFuncAddr
		}
	}

	void __stdcall AdvStaffrollLogoHook(float sizeX, float sizeY)
	{
		typeStaffrollTextObj* that;
		_asm mov that, esi

		float scalar = Screen.fHeight / 480.0f;
		float posscalar = (4.0f / 3.0f) / Screen.fAspectRatio;

		that->posX *= posscalar;

		if (Screen.Width < Screen.Height)
		{
			scalar = Screen.fWidth / 480.0f;
			that->posX = 0;
		}				

		return AdvStaffrollLogoFunc(that, sizeX * scalar, sizeY * scalar);
	}
}

uintptr_t TextDrawFunc2Addr = 0x00458200;
// this is a fastcall
// arg0 = eax
void __stdcall TextDrawFunc2(uintptr_t a0, uintptr_t a1, float posX, float posY, float sizeX, float sizeY, uintptr_t a3)
{
	_asm
	{
		push a3
		push sizeY
		push sizeX
		push posY
		push posX
		push a1
		mov eax, a0
		call TextDrawFunc2Addr
	}
}

void __stdcall TextDrawFunc2Hook(uintptr_t a1, float posX, float posY, float sizeX, float sizeY, uintptr_t a3)
{
	uintptr_t a0;
	_asm mov a0, eax

	float scalar = (Screen.fHeight / 480.0f);
	//float posXscalar = (Screen.fAspectRatio / (4.0f / 3.0f));

	if (Screen.fAspectRatio < (4.0f / 3.0f))
		scalar = (Screen.fWidth / 480.0f);

	float newSizeX = sizeX * scalar;
	float newSizeY = sizeY * scalar;
	//float newPosX = posX * TestFloat1;

	//printf("PosX: %.2f\tNewPosX: %.2f\n", posX, newPosX);

	return TextDrawFunc2(a0, a1, posX, posY, newSizeX, newSizeY, a3);
}

void __stdcall HookAConsole()
{
	AllocConsole();
	AttachConsole(ATTACH_PARENT_PROCESS);

	freopen("CON", "wb", stdout);
	freopen("CON", "wb", stderr);

	printf("TestFloat1: 0x%X\n", &TestFloat1);
	printf("TestFloat2: 0x%X\n", &TestFloat2);
	printf("TestFloat3: 0x%X\n", &TestFloat3);
	printf("TestFloat4: 0x%X\n", &TestFloat4);
}

#pragma runtime_checks( "", restore )

ATOM WINAPI RegisterClassHook(WNDCLASSEXA* wcex) 
{
	// TODO -- integrate icon into a resource
	wcex->hIcon = (HICON)LoadImage(NULL, L"icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);
	return RegisterClassExA(wcex);
}

static BOOL WINAPI UpdateWindowHook(HWND hWnd)
{
	// fix the window to open at the center of the screen...
	HMONITOR monitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
	MONITORINFOEX info = { sizeof(MONITORINFOEX) };
	GetMonitorInfo(monitor, &info);
	DEVMODE devmode = {};
	devmode.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devmode);
	DWORD DesktopX = devmode.dmPelsWidth;
	DWORD DesktopY = devmode.dmPelsHeight;

	RECT wndRect = { 0 };
	GetWindowRect(hWnd, &wndRect);

	int Width = wndRect.right - wndRect.left;
	int Height = wndRect.bottom - wndRect.top;

	int WindowPosX = (int)(((float)DesktopX / 2.0f) - ((float)Width / 2.0f));
	int WindowPosY = (int)(((float)DesktopY / 2.0f) - ((float)Height / 2.0f));

	SetWindowPos(hWnd, 0, WindowPosX, WindowPosY, 0, 0, SWP_NOSIZE | SWP_FRAMECHANGED);

	return UpdateWindow(hWnd);
}



enum SystemMode
{
	PalSelect,
	MainMenu,
	InGame,
	EasyMenu,
	Credits,
	EasyMenuMovie,
	MaxSysMode
};

void Init()
{
	CIniReader iniReader("");
	DWORD dummyoldprotect = 0;
	Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
	Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
	Screen.fConditionalAspectRatio = iniReader.ReadFloat("MAIN", "MinAspectRatio", (4.0f / 3.0f));
	Screen.fZoomFactor = iniReader.ReadFloat("MAIN", "FOVScale", 1.0f);
	iniZoomFactor = Screen.fZoomFactor;
	int nWindowedMode = iniReader.ReadInteger("MISC", "WindowedMode", 0);
	// bShadowFix = iniReader.ReadInteger("MISC", "ShadowFix", 1) != 0;
	fShadowScale = iniReader.ReadFloat("MISC", "ShadowScale", 1.7f);
	static uint32_t ShadowRes = iniReader.ReadInteger("MISC", "ShadowRes", 256);
	static float ClipRange = iniReader.ReadFloat("MISC", "ClipRange", 1.0f);
	bLensFlareFix = iniReader.ReadInteger("MISC", "LensFlareFix", 1) != 0;
	bFixAdvertiseWindows = iniReader.ReadInteger("MISC", "FixAdvertiseWindows", 1) != 0;
	bFixStaffRoll = iniReader.ReadInteger("MISC", "FixStaffRoll", 1) != 0;
	static bool bDisableMouseInput = iniReader.ReadInteger("MISC", "DisableMouseInput", 1) != 0;
	static bool bDisableFrameSkipping = iniReader.ReadInteger("MISC", "DisableFrameSkipping", 1) != 0;
	static bool bRestoreDemos = iniReader.ReadInteger("MISC", "RestoreDemos", 1) != 0;
	static bool bDisableCDCheck = iniReader.ReadInteger("MISC", "DisableCDCheck", 1) != 0;
	static bool bDisableQuitDialog = iniReader.ReadInteger("MISC", "DisableQuitDialog", 0) != 0;
	
	static auto szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MISC", "CustomUserFilesDirectoryInGameDir", "0");
	if (szCustomUserFilesDirectoryInGameDir.empty() || szCustomUserFilesDirectoryInGameDir == "0")
		szCustomUserFilesDirectoryInGameDir.clear();

	// SkipFE-like functions for Sonic Heroes
	// credit to: https://github.com/Sewer56/Heroes.Utils.DebugBoot.ReloadedII
	static bool bSkipFE = iniReader.ReadInteger("SkipFE", "Enabled", 0) != 0;
	static int32_t SysMode = iniReader.ReadInteger("SkipFE", "SystemMode", SystemMode::EasyMenu);

	if ((SysMode == SystemMode::InGame) && bSkipFE)
	{
		*(int32_t*)0x8D6720 = iniReader.ReadInteger("SkipFE", "Stage", 2);
		*(int32_t*)0x8D6920 = iniReader.ReadInteger("SkipFE", "Team1", 0);
		*(int32_t*)0x8D6924 = iniReader.ReadInteger("SkipFE", "Team2", -1);
		*(int32_t*)0x8D6928 = iniReader.ReadInteger("SkipFE", "Team3", -1);
		*(int32_t*)0x8D692C = iniReader.ReadInteger("SkipFE", "Team4", -1);
	}

	if (!Screen.Width || !Screen.Height)
		std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

	Screen.fWidth = static_cast<float>(Screen.Width);
	Screen.fHeight = static_cast<float>(Screen.Height);
	Screen.fInvWidth = 1.0f / Screen.fWidth;
	Screen.fInvHeight = 1.0f / Screen.fInvHeight;
	Screen.fInvWidth43 = 1.0f / (Screen.fHeight * (4.0f * 3.0f));
	Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
	Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));

	if (Screen.fAspectRatio < Screen.fConditionalAspectRatio)
	{
		Screen.fZoomFactor *= (Screen.fAspectRatio / Screen.fConditionalAspectRatio);
	}

	// TODO: it is kind of stupid to tie HUD scale with the FOV
	Screen.fHudScale = ((4.0f / 3.0f) / Screen.fAspectRatio) * Screen.fZoomFactor;

	Screen.OffsetX = (int)(Screen.Width - (Screen.Width * Screen.fHudScale)) / 2;
	Screen.OffsetY = (int)(Screen.Height - (Screen.Height * Screen.fZoomFactor)) / 2;

	Screen.HeightHUD = (int)(Screen.Height * Screen.fZoomFactor);

	Screen.WidthFMV = Screen.Width;
	Screen.HeightFMV = Screen.Height;
	

	if (Screen.fAspectRatio < (4.0f / 3.0f))
	{
		Screen.HeightFMV = (int)(Screen.HeightFMV * (Screen.fAspectRatio / (4.0f / 3.0f)));
	}
	else
	{
		Screen.WidthFMV = (int)(Screen.WidthFMV / (Screen.fAspectRatio / (4.0f / 3.0f)));
	}

	// disable writes to resolution vars
	injector::MakeNOP(0x427722, 10);
	injector::MakeNOP(0x00427735, 10);
	injector::MakeNOP(0x00444892, 5);
	injector::MakeNOP(0x00444897, 5);
	injector::MakeNOP(0x00629F5F, 5);

	injector::MakeJMP(0x446B06, 0x446B51);

	// write resolution vars
	*(uint32_t*)0x00A7793C = Screen.Width;
	*(uint32_t*)0x00A77940 = Screen.Height;
	*(uint32_t*)0x007C931C = Screen.Width;
	*(uint32_t*)0x007C9320 = Screen.Height;

	*(uint8_t*)0x008CAEE0 = 7; // Screen_Size_Selection -- force it to 7 (1280x960 32-bit)

	// fix for the split screen screen buffer -- force read the current res
	injector::MakeNOP(0x0061D418, 2);

	// fix "end the game" dialog to open at the center & have focus (disabling the minimize because minimized windows and their children lose focus!)
	injector::MakeNOP(0x00446C85, 6, true);
	injector::MakeCALL(0x00446C85, UpdateWindowHook);
	injector::MakeJMP(0x446F33, 0x446F38);

	if (bLensFlareFix)
	{
		injector::UnprotectMemory(0x0078A180, 2 * sizeof(float), dummyoldprotect);
		*(float*)0x0078A184 = Screen.fWidth;
		*(float*)0x0078A180 = Screen.fHeight;
		// lens flare size
		fLensFlareScalar = Screen.fHeight / 480.0f;

		injector::MakeJMP(0x0048F419, LensFlareScale, true);
	}

	auto pattern = hook::pattern("0F BF 4E ? 0F BF C0 89 44 ? ? DB 44 ? ? 89 4C ? ? 85 DB"); // 662C2D
	struct ResHook6
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.ecx = *(uint16_t*)(regs.esi + 0x1E);

			if (Screen.AspectRatioAffected)
			{
				if (Screen.PreserveHorizontalPosition == true)
				{
					regs.eax -= Screen.OffsetX;
					if (Screen.ContinuePositionChange != false)
					{
						Screen.PreserveHorizontalPosition = true;
					}
					else
					{
						Screen.PreserveHorizontalPosition = false;
					}
				}
				regs.eax += Screen.OffsetX;
				regs.ecx += Screen.OffsetY;
				if (Screen.ContinuePositionChange != false)
				{
					Screen.AspectRatioAffected = true;
					Screen.ContinuePositionChange -= 1;
				}
				else
				{
					Screen.AspectRatioAffected = false;
				}
			}
		}
	};

	injector::MakeInline<ResHook6>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(7));
	pattern = hook::pattern("66 8B ? ? 66 85 C0 75 29 66 39 ? ? 75 23"); // 662BFB
	injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<int8_t>(7), 0xEB, true);

	uintptr_t loc_64AC8B = reinterpret_cast<uintptr_t>(hook::pattern("D9 05 ? ? ? ? 89 4E 68 8B 50 04 D8 76 68").get_first(0));
	uintptr_t loc_64ACA5 = loc_64AC8B + 0x1A;
	static uintptr_t ShadowStuffAddr = 0x7476C4;

	// TODO - if any more things break thanks to this jank here, target function at 64AC80 in specific places instead
	struct ExceptionalHudScale1
	{
		void operator()(injector::reg_pack& regs)
		{
			float one = 1.0f;
			if (*(uint32_t*)(regs.esp + 8 + 4) == ShadowStuffAddr)
				_asm fld one
			else
				_asm fld Screen.fHudScale
		}
	}; injector::MakeInline<ExceptionalHudScale1>(loc_64AC8B, loc_64AC8B + 6);

	struct ExceptionalHudScale2
	{
		void operator()(injector::reg_pack& regs)
		{
			float one = 1.0f;
			if (*(uint32_t*)(regs.esp + 8 + 4) == ShadowStuffAddr)
				_asm fld one
			else
				_asm fld Screen.fZoomFactor
		}
	}; injector::MakeInline<ExceptionalHudScale2>(loc_64ACA5, loc_64ACA5 + 6);

	// 2P Interface
	pattern = hook::pattern("8B 48 60 DB 41 0C 83 C4 ? BF ? ? ? ? BE ? ? ? ? D9 5C"); // 0x422C2B
	struct HUDHor
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.ecx = *(int*)(regs.eax + 0x60);
			float ResolutionX = (float)(*(int*)(regs.ecx + 0x0C)) * Screen.fHudScale;
			_asm
			{
				fld dword ptr[ResolutionX]
			}
		}
	};

	struct HUDVert
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.ebp |= -1;
			float ResolutionY = (float)(*(int*)(regs.ecx + 0x10)) * Screen.fZoomFactor;
			_asm
			{
				fld dword ptr[ResolutionY]
			}
		}
	};

	injector::MakeInline<HUDHor>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
	injector::MakeInline<HUDVert>(pattern.count(1).get(0).get<uint32_t>(23), pattern.count(1).get(0).get<uint32_t>(29));

	pattern = hook::pattern("D9 44 ? ? D8 64 ? ? 6A 04 68 ? ? ? ? 6A 04 D9 15 ? ? ? ? D9 1D ? ? ? ? D9 44"); // 0x422A85

	struct HUDPos
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.AspectRatioAffected = true;
			float Temp = (*(float*)(regs.esp + 0x0C)) - (*(float*)(regs.esp + 0x14));
			_asm
			{
				fld dword ptr[Temp]
			}
		}
	};

	injector::MakeInline<HUDPos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(8));

	// Credits
	pattern = hook::pattern("DB 05 ? ? ? ? D8 0F D8 0D ? ? ? ? D9 9E ? ? ? ? 8B 57 04 89 96 ? ? ? ?"); // 0x4543FF
	injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &Screen.Width43, true);

	pattern = hook::pattern("DB 05 ? ? ? ? D9 5C ? ? DB 05 ? ? ? ? D9 5C ? ? E8 ? ? ? ? D9 05 ? ? ? ?"); // 0x4548B1

	struct CreditPicturePos
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.AspectRatioAffected = true;
			_asm
			{
				fild dword ptr[Screen.Width43]
			}
		}
	};

	injector::MakeInline<CreditPicturePos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
	injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(12), &Screen.HeightHUD, true);

	pattern = hook::pattern("8B D1 C1 EA 10 0F B6 C6 C1 E0 08 0F B6 F9 0B C7 89 4C ? ? C1 E0 08"); // 0x454FFD

	struct CreditPicturePos2
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.AspectRatioAffected = true;
			regs.edx = regs.ecx;
			regs.edx >>= 10;
		}
	};

	injector::MakeInline<CreditPicturePos2>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(5));

	pattern = hook::pattern("8B 44 ? ? EB 10 8B 1D ? ? ? ? 8B 2D ? ? ? ? 89 5C ? ?"); //0x6445C0

	injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(8), &Screen.WidthFMV, true);
	injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(14), &Screen.HeightFMV, true);

	pattern = hook::pattern("83 EC ? 56 57 E8 ? ? ? ? D9 05 ? ? ? ? D8 B0 ? ? ? ? 8B 84 ?"); // 0x6445C0
	struct CreditFMVPos
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.AspectRatioAffected = true;
			_asm
			{
				fld1
			}
		}
	};
	struct CreditFMVPos2
	{
		void operator()(injector::reg_pack& regs)
		{
			if (!Screen.AspectRatioAffected)
			{
				Screen.AspectRatioAffected = true;
			}
			regs.ecx = *(int*)(regs.esp + 8);
			_asm
			{
				fstp dword ptr[esi - 8]
			}
		}
	};
	injector::MakeInline<CreditFMVPos>(pattern.count(1).get(0).get<uint32_t>(10), pattern.count(1).get(0).get<uint32_t>(16));
	injector::MakeInline<CreditFMVPos2>(pattern.count(1).get(0).get<uint32_t>(332), pattern.count(1).get(0).get<uint32_t>(339));

	// Mission text

	struct MissionTextHor
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.AspectRatioAffected = true;
			float ResolutionX = (float)(*(int*)(regs.ecx + 0x0C)) * Screen.fHudScale;
			_asm
			{
				fld dword ptr[ResolutionX]
			}
			regs.esi = *(int*)(regs.esi + 0x98);
		}
	};

	struct MissionTextVert
	{
		void operator()(injector::reg_pack& regs)
		{
			float ResolutionY = (float)(*(int*)(regs.ecx + 0x10)) * Screen.fZoomFactor;
			_asm
			{
				fld dword ptr[ResolutionY]
			}
			*(float*)(regs.esp + 0x10) = ResolutionY;
		}
	};

	pattern = hook::pattern("DB 41 0C 8B B6 ? ? ? ? 83 C4 08 83 EE 02"); // 0x44296C
	injector::MakeInline<MissionTextHor>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(9));
	injector::MakeInline<MissionTextVert>(pattern.count(1).get(0).get<uint32_t>(19), pattern.count(1).get(0).get<uint32_t>(26));

	struct NowLoadingTextPos
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.AspectRatioAffected = true;
			float eax80 = *(float*)(regs.eax + 0x80);
			_asm
			{
				fdiv dword ptr[eax80]
			}
		}
	};
	pattern = hook::pattern("DB 05 ? ? ? ? D9 5C ? ? DB 47 ? D8 4C ? ? D8 0D ? ? ? ? D9 5C ? ? DB 05 ? ? ? ?");
	injector::MakeInline<NowLoadingTextPos>(pattern.count(1).get(0).get<uint32_t>(69), pattern.count(1).get(0).get<uint32_t>(75));

	// Powerup Icons -- TODO: broken, powerup icons are too close to each other!

	struct PowerupIcon1
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.AspectRatioAffected = true;
			regs.ecx = *(int*)(regs.eax + 0x60);
			float ResolutionX = (float)(*(int*)(regs.ecx + 0x0C)) * Screen.fHudScale;
			_asm
			{
				fld dword ptr[ResolutionX]
			}
		}
	};

	struct PowerupIcon2
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.ecx = *(int*)(regs.eax + 0x60);
			float ResolutionX = (float)(*(int*)(regs.ecx + 0x0C)) * Screen.fHudScale;
			_asm
			{
				fld dword ptr[ResolutionX]
			}
		}
	};

	struct PowerupIcon3
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.edx = *(int*)(regs.eax + 0x60);
			float ResolutionY = (float)(*(int*)(regs.edx + 0x10)) * Screen.fZoomFactor;
			_asm
			{
				fld dword ptr[ResolutionY]
			}
		}
	};

	struct PowerupIcon4
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.edi = 1;
			_asm
			{
				fmul dword ptr[Screen.fHudScale]
			}
		}
	};

	pattern = hook::pattern("8B 48 60 DB 41 0C 6A 04 68 ? ? ? ? 6A 04 D8 4C ? ?"); //0x479AEA
	injector::MakeInline<PowerupIcon1>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
	pattern = hook::pattern("8B 48 60 DB 41 0C D9 5C ? ? E8 ? ? ? ? 8B 50 60 DB 42 10 D9 5C ? ? E8 ? ? ? ? D9 05"); // 0x479EB1
	injector::MakeInline<PowerupIcon2>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
	injector::MakeInline<PowerupIcon3>(pattern.count(1).get(0).get<uint32_t>(15), pattern.count(1).get(0).get<uint32_t>(21));
	pattern = hook::pattern("DB 40 0C 8B 54 ? ? 8B 44 ? ? 8B 4D 34 D8 0D ? ? ? ? 4A 89 54 ? ? BF 01 00 00 00 3B C7"); // 0x479F26
	injector::MakeInline<PowerupIcon4>(pattern.count(1).get(0).get<uint32_t>(25), pattern.count(1).get(0).get<uint32_t>(30));

	// Results screen
	pattern = hook::pattern("8B 48 60 DB 41 0C E8 ? ? ? ? 8B 50 60 DB 42 10 D9 5C ? ? E8 ? ? ? ? D9 05 ? ? ? ? BE"); // 0x438CDD
	struct ResultsHor
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.ecx = *(int*)(regs.eax + 0x60);
			float ResolutionX = (float)(*(int*)(regs.ecx + 0x0C)) * Screen.fHudScale;
			_asm
			{
				fld dword ptr[ResolutionX]
			}
		}
	};

	struct ResultsVert
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.edx = *(int*)(regs.eax + 0x60);
			float ResolutionY = (float)(*(int*)(regs.edx + 0x10)) * Screen.fZoomFactor;
			_asm
			{
				fld dword ptr[ResolutionY]
			}
		}
	};

	struct ResultsPos
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.eax <<= 8;
			regs.eax |= regs.ecx;
			Screen.AspectRatioAffected = true;
		}
	};

	injector::MakeInline<ResultsHor>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
	injector::MakeInline<ResultsVert>(pattern.count(1).get(0).get<uint32_t>(11), pattern.count(1).get(0).get<uint32_t>(17));
	pattern = hook::pattern("8A 44 B4 ? C1 E0 08 0B C1 C1 E0 08 0B C2 A3"); //0x438E42
	injector::MakeInline<ResultsPos>(pattern.count(1).get(0).get<uint32_t>(4), pattern.count(1).get(0).get<uint32_t>(9));

	// Special Stage
	// TODO: distance bar at the bottom is incorrect
	fDustWidth = 40.0f * Screen.fAspectRatio;
	static float fDustHeight = 40.0f;
	injector::WriteMemory<float*>(0x0052C5FB + 2, &fDustWidth, true);
	injector::WriteMemory<float*>(0x0052C607 + 2, &fDustHeight, true);

	pattern = hook::pattern("DB 05 ? ? ? ? 8B 00 D9 84 ? ? ? ? ? 53 55 D8 C9 57 50 6A 01 D8 0D ? ? ? ? D9 9C"); // 0x45894A
	injector::MakeInline<CreditPicturePos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
	injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(38), &Screen.HeightHUD, true);

	pattern = hook::pattern("DB 05 ? ? ? ? A1 ? ? ? ? 83 C4 08 D9 54 ? ? D8 8C"); // 0x526F83
	injector::MakeInline<CreditPicturePos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
	injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(40), &Screen.HeightHUD, true);

	injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(236), &Screen.Width43, true);


	pattern = hook::pattern("E8 ? ? ? ? 83 C4 10 E8 ? ? ? ? 5F C3 CC CC CC CC CC CC 83 EC 0C 56 33 F6 89 74"); // 0x527470

	struct MeteorRightPos
	{
		void operator()(injector::reg_pack& regs)
		{
			if (regs.eax != 0)
			{
				Screen.PreserveHorizontalPosition = true;
				Screen.ContinuePositionChange = 3;
			}
		}
	};

	injector::MakeInline<MeteorRightPos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(5));
	injector::WriteMemory<uint16_t>(pattern.count(1).get(0).get<uint16_t>(5), 0xD1EB, true);
	injector::MakeNOP(pattern.count(1).get(0).get<uint16_t>(7), 1, true);

	pattern = hook::pattern("DB 05 ? ? ? ? A1 ? ? ? ? 0F B6 9C 24 ? ? ? ? D9 54 24 ? C1 E3 18"); // 0x5263DE

	injector::MakeInline<CreditPicturePos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
	injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(63), &Screen.HeightHUD, true);

	pattern = hook::pattern("DB 05 ? ? ? ? A1 ? ? ? ? 83 C4 08 D9 54 24 ? BF ? ? ? ? D8 8C 24 ? ? ? ? 8D 74 24 ?"); // 0x526297

	injector::MakeInline<CreditPicturePos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
	injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(49), &Screen.HeightHUD, true);

	pattern = hook::pattern("85 C0 74 0F 8B 46 28 85 C0 75 08 C7 44 24 04 00 80 FD 43 8A 42 1E 84 C0 74 0E"); // 0x527936

	struct LevelUpPos1
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.PreserveHorizontalPosition = false;
			regs.eax = *(int*)(regs.esi + 0x28);
			if (regs.eax == 0)
			{
				Screen.PreserveHorizontalPosition = true;
				*(float*)(regs.esp + 4) = 507.0f;
			}
		}
	};

	injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(0), 4, true);
	injector::MakeInline<LevelUpPos1>(pattern.count(1).get(0).get<uint32_t>(4), pattern.count(1).get(0).get<uint32_t>(19));

	struct LevelUpPos2
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.AspectRatioAffected = false;
			Screen.ContinuePositionChange = true;
			regs.eax = *(int*)(regs.esi + 0x2C);
			if (regs.eax < 95)
			{
				Screen.ContinuePositionChange = true;
			}
			regs.ecx = *(int*)(regs.esi + 0x48);
		}
	};

	injector::MakeInline<LevelUpPos2>(pattern.count(1).get(0).get<uint32_t>(40), pattern.count(1).get(0).get<uint32_t>(46));

	//pattern = hook::pattern("D9 C9 C1 E0 08 D9 5C ? ? 0B C1 C1 E0 08 D9 5C ? ? 0B C2 8D 4C ? ? BA"); //0x4578A3
	//
	//struct WindowTextPos
	//{
	//	void operator()(injector::reg_pack& regs)
	//	{
	//		_asm
	//		{
	//			fxch st(1)
	//		}
	//		Screen.AspectRatioAffected = true;
	//		regs.eax <<= 8;
	//	}
	//};
	//
	//injector::MakeInline<WindowTextPos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(5));
	//
	//pattern = hook::pattern("C1 E0 08 0B C2 D9 54 ? ? C1 E0 08 D9 41 ? 0F B6 D3 D8 05 ? ? ? ? 0B C2 8B 54"); //0x4583FB
	//
	//struct WindowTextPos2
	//{
	//	void operator()(injector::reg_pack& regs)
	//	{
	//		Screen.AspectRatioAffected = true;
	//		regs.eax <<= 8;
	//		regs.eax |= regs.edx;
	//	}
	//};
	//
	//injector::MakeInline<WindowTextPos2>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(5));
	//
	//pattern = hook::pattern("D9 00 8B 39 D8 02 8B 50 04 89 54 ? ? 8B 50 08 89 7C ? ? D9 54"); //0x4574F9
	//
	//struct WindowPos
	//{
	//	void operator()(injector::reg_pack& regs)
	//	{
	//		Screen.AspectRatioAffected = true;
	//		float Temp = *(float*)(regs.eax) + *(float*)(regs.edx);
	//		regs.edi = *(int*)(regs.ecx);
	//		_asm
	//		{
	//			fld dword ptr[Temp]
	//		}
	//	}
	//};
	//
	//injector::MakeInline<WindowPos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));

	// injector::WriteMemory<int*>(0x00456CFA + 2, &Screen.Width43, true);
	// injector::WriteMemory<int*>(0x00456D2B + 2, &Screen.Width43, true);
	// injector::WriteMemory<float*>(0x00456D03 + 2, &Screen.fInvWidth43, true);
	// injector::WriteMemory<float*>(0x00456D34 + 2, &Screen.fInvWidth43, true);

	//injector::WriteMemory<float*>(0x00456D2B + 2, &Screen.fInvHeight, true);


	// unprotect and set scaled X res divider for Advertise
	injector::UnprotectMemory(0x0078A08C, sizeof(float), dummyoldprotect);
	*(float*)0x0078A08C = 1.0f / (480.0f * Screen.fAspectRatio);

	if (bShadowFix)
	{
		struct ShadowFix2
		{
			void operator()(injector::reg_pack& regs)
			{
				float val = (5.0f * fShadowScale) - ((2.5f / fShadowScale) * (4 - regs.esi));
				float val2 = val * 0.025f;
	
				// 0.278645843267
				*(float*)(regs.esp + 0x3C) = val2;
				*(float*)(regs.esp + 0x40) = val2;
				*(float*)(regs.esp + 0x58) = val2;
				*(float*)(regs.esp + 0x78) = val2;
				// 0.72135412693
				*(float*)(regs.esp + 0x5C) = 1.0f - val2;
				*(float*)(regs.esp + 0x74) = 1.0f - val2;
				*(float*)(regs.esp + 0x90) = 1.0f - val2;
				*(float*)(regs.esp + 0x94) = 1.0f - val2;
		
				reinterpret_cast<void(__cdecl*)(uint32_t, uint32_t, uint32_t)>(0x64CA10)(4, regs.edx, 4);
				_asm fld [val]
			}
		}; injector::MakeInline<ShadowFix2>(0x0063B143, 0x0063B17C);
	}

	if (!szCustomUserFilesDirectoryInGameDir.empty())
	{
		szCustomUserFilesDirectoryInGameDir = GetExeModulePath<std::string>() + szCustomUserFilesDirectoryInGameDir;

		auto SHGetFolderPathAHook = [](HWND /*hwnd*/, int /*csidl*/, HANDLE /*hToken*/, DWORD /*dwFlags*/, LPSTR pszPath) -> HRESULT
		{
			CreateDirectoryA(szCustomUserFilesDirectoryInGameDir.c_str(), NULL);
			strcpy_s(pszPath, MAX_PATH, szCustomUserFilesDirectoryInGameDir.c_str());
			return S_OK;
		};

		auto PathAppendAHook = [](LPSTR pszPath, LPSTR) -> HRESULT
		{
			strcat_s(pszPath, MAX_PATH, "\\");
			return S_OK;
		};

		uintptr_t loc_629D17 = reinterpret_cast<uintptr_t>(hook::pattern("? ? ? 6A 1A ? FF 15").get_first(0)) + 6;
		uintptr_t loc_62A3FC = reinterpret_cast<uintptr_t>(hook::pattern("50 6A 00 6A 00 6A 1A 6A 00 FF 15").get_first(0)) + 9;
		uintptr_t loc_62DF99 = reinterpret_cast<uintptr_t>(hook::pattern("56 6A 00 6A 00 6A 1A 6A 00 FF 15").get_first(0)) + 9;
		uintptr_t loc_62DFAC = loc_62DF99 + 0x13;
		uintptr_t str_7468D0 = *reinterpret_cast<uintptr_t*>(loc_629D17 + 0xD);

		injector::MakeCALL(loc_629D17, static_cast<HRESULT(WINAPI*)(HWND, int, HANDLE, DWORD, LPSTR)>(SHGetFolderPathAHook), true);
		injector::MakeNOP(loc_629D17 + 5, 1, true);

		injector::MakeCALL(loc_62A3FC, static_cast<HRESULT(WINAPI*)(HWND, int, HANDLE, DWORD, LPSTR)>(SHGetFolderPathAHook), true);
		injector::MakeNOP(loc_62A3FC + 5, 1, true);

		injector::MakeCALL(loc_62DF99, static_cast<HRESULT(WINAPI*)(HWND, int, HANDLE, DWORD, LPSTR)>(SHGetFolderPathAHook), true);
		injector::MakeNOP(loc_62DF99 + 5, 1, true);
		
		injector::WriteMemory<uint8_t>(str_7468D0 + 2, 0, true);

		injector::MakeCALL(loc_62DFAC, static_cast<HRESULT(WINAPI*)(LPSTR, LPSTR)>(PathAppendAHook), true);
		injector::MakeNOP(loc_62DFAC + 5, 1, true);
	}

	// set Screen_Full & override game config
	injector::MakeNOP(0x00629F8E, 5);
	*(uint32_t*)0x008CAEDC = nWindowedMode == 0;

	if ((nWindowedMode == 4) || (nWindowedMode == 5) || !nWindowedMode)
	{
		uintptr_t addr = 0x004460A9;
		injector::MakeNOP(addr, 6, true);
		injector::MakeCALL(addr, GetClientRectHook, true);
		addr = 0x655794;
		injector::MakeNOP(addr, 6, true);
		injector::MakeCALL(addr, GetClientRectHook, true);
		addr = 0x656644;
		injector::MakeNOP(addr, 6, true);
		injector::MakeCALL(addr, GetClientRectHook, true);
		addr = 0x656787;
		injector::MakeNOP(addr, 6, true);
		injector::MakeCALL(addr, GetClientRectHook, true);
		addr = 0x657C9C;
		injector::MakeNOP(addr, 6, true);
		injector::MakeCALL(addr, GetClientRectHook, true);
		addr = 0x658172;
		injector::MakeNOP(addr, 6, true);
		injector::MakeCALL(addr, GetClientRectHook, true);
		addr = 0x65822F;
		injector::MakeNOP(addr, 6, true);
		injector::MakeCALL(addr, GetClientRectHook, true);
		addr = 0x65DBE7;
		injector::MakeNOP(addr, 6, true);
		injector::MakeCALL(addr, GetClientRectHook, true);
		addr = 0x65DC2F;
		injector::MakeNOP(addr, 6, true);
		injector::MakeCALL(addr, GetClientRectHook, true);
		addr = 0x65E152;
		injector::MakeNOP(addr, 6, true);
		injector::MakeCALL(addr, GetClientRectHook, true);
		addr = 0x65E783;
		injector::MakeNOP(addr, 6, true);
		injector::MakeCALL(addr, GetClientRectHook, true);
	}

	if (nWindowedMode)
	{
		injector::MakeJMP(0x446D87, 0x446DA5, true);
		injector::MakeJMP(0x446E11, 0x446E24, true);
		injector::MakeNOP(0x4462F8, 6, true);
		injector::MakeCALL(0x4462F8, WindowedModeWrapper::CreateWindowExA_Hook, true);
		injector::MakeNOP(0x4462B5, 6, true);
		injector::MakeCALL(0x4462B5, WindowedModeWrapper::AdjustWindowRect_Hook, true);
		injector::MakeNOP(0x446DD7, 6, true);
		injector::MakeCALL(0x446DD7, WindowedModeWrapper::AdjustWindowRect_Hook, true);

		// disable cursor centering on boot
		injector::MakeNOP(0x00446EA8, 5, true);

		// dereference the current WndProc from the game executable and write to the function pointer (to maximize compatibility)
		uint32_t* wndproc_addr = hook::pattern("C7 44 24 14 ? ? ? ? 89 74 24 18 89 74 24 1C").count(1).get(0).get<uint32_t>(4);
		GameWndProcAddr = *(unsigned int*)wndproc_addr;
		GameWndProc = (LRESULT(WINAPI*)(HWND, UINT, WPARAM, LPARAM))GameWndProcAddr;
		injector::WriteMemory<unsigned int>(wndproc_addr, (unsigned int)&WSFixWndProc, true);

		switch (nWindowedMode)
		{
		case 5:
			WindowedModeWrapper::bStretchWindow = true;
			injector::MakeJMP(0x6567B6, StretchOnBoot, true);
			break;
		case 4:
			WindowedModeWrapper::bScaleWindow = true;
			injector::MakeJMP(0x6567B6, StretchOnBoot, true);
			break;
		case 3:
			WindowedModeWrapper::bEnableWindowResize = true;
			injector::MakeJMP(0x00445B32, 0x00445B3A);
			injector::MakeJMP(0x00446EBF, 0x00446EC6);
		case 2:
			WindowedModeWrapper::bBorderlessWindowed = false;
			break;
		default:
			break;
		}
	}

	if (bDisableMouseInput)
		// corrupt GUID_SysMouse on purpose
		injector::WriteMemory<uint32_t>(0x00722FA0, 0, true);

	if (bDisableFrameSkipping)
		injector::MakeJMP(0x402CF5, 0x402D20);

	if (bRestoreDemos)
		injector::MakeJMP(0x456989, RestoreDemos, true);

	if (bSkipFE)
	{
		struct SysModeHook
		{
			void operator()(injector::reg_pack& regs)
			{
				*(uint32_t*)(regs.eax + 0x38) = SysMode;
				reinterpret_cast<void(*)()>(0x42A9F0)();
				*(uint32_t*)(regs.esi + 4) = regs.edi;
			}
		}; injector::MakeInline<SysModeHook>(0x42713E, 0x427149);
	}
	if (bDisableCDCheck)
		injector::MakeJMP(0x00629B72, 0x629C36, true);

	if (ShadowRes != 256)
	{
		injector::WriteMemory<uint32_t>(0x63BF6B + 1, ShadowRes, true);
		injector::WriteMemory<uint32_t>(0x63BF8F + 1, ShadowRes - 4, true);
		injector::WriteMemory<uint32_t>(0x7476CC, ShadowRes, true);

		injector::WriteMemory<float>(0x0063B077 + 4, (float)ShadowRes, true);
		injector::WriteMemory<float>(0x0063B07F + 4, (float)ShadowRes, true);
		injector::WriteMemory<float>(0x0063B08F + 4, (float)ShadowRes, true);
		injector::WriteMemory<float>(0x0063B097 + 4, (float)ShadowRes, true);

		injector::WriteMemory<float>(0x006B7237 + 1, (float)ShadowRes, true);
	}

	if (bDisableQuitDialog)
		injector::MakeJMP(0x446EFB, 0x446FFB);

	// custom clip range
	injector::WriteMemory<float>(0x007869B4, ClipRange, true);

	static float fInv640 = 1.0f / 640.0f;
	static int OrigWidth = 640;
	static int OrigHeight = 480;
	static int Width43_480 = 480.0f * Screen.fAspectRatio;
	// 2P fix -- ignore aspect change for screen texture
	injector::WriteMemory<float*>(0x0061D533 + 2, &fInv640, true);

	// ignore aspect change for special stage gauge
	injector::WriteMemory<float*>(0x005262B9 + 2, &fInv640, true);
	injector::WriteMemory<float*>(0x005262F9 + 2, &fInv640, true);
	injector::WriteMemory<float*>(0x0052640A + 2, &fInv640, true);
	injector::WriteMemory<float*>(0x00526459 + 2, &fInv640, true);

	// ignore aspect change for special stage link counter
	injector::WriteMemory<float*>(0x00526F9C + 2, &fInv640, true);
	injector::WriteMemory<float*>(0x00526FEA + 2, &fInv640, true);

	// ignore aspect change for results screen
	injector::WriteMemory<float*>(0x458961 + 2, &fInv640, true);
	injector::WriteMemory<float*>(0x00458993 + 2, &fInv640, true);

	// ignore aspect change for Advertise windows
	// position
	//injector::WriteMemory<int*>(0x00456CFA + 2, &Width43_480, true);
	injector::WriteMemory<int*>(0x00456CFA + 2, &OrigWidth, true);
	injector::WriteMemory<int*>(0x00456D0C + 2, &OrigHeight, true);
	injector::WriteMemory<float*>(0x00456D03 + 2, &fInv640, true);
	// size
	//injector::WriteMemory<int*>(0x00456D2B + 2, &Width43_480, true);
	injector::WriteMemory<int*>(0x00456D2B + 2, &OrigWidth, true);
	injector::WriteMemory<int*>(0x00456D3D + 2, &OrigHeight, true);
	injector::WriteMemory<float*>(0x00456D34 + 2, &fInv640, true);

	// staff roll
	injector::WriteMemory<int*>(0x004543FF + 2, &OrigWidth, true);
	injector::WriteMemory<float*>(0x00454407 + 2, &fInv640, true);
	//injector::WriteMemory<int*>(0x004546F5 + 2, &OrigHeight, true);

	// fix window icon
	injector::MakeNOP(0x00446229, 6);
	injector::MakeCALL(0x00446229, RegisterClassHook);
#ifdef _DEBUG
	injector::MakeRangedNOP(0x00446CC6, 0x00446CD3);
	injector::MakeCALL(0x00446CC8, HookAConsole);
#endif

	if (bFixAdvertiseWindows)
	{
		float scalar = (Screen.fHeight / 480.0f);
		if (Screen.Width < Screen.Height)
		{
			scalar = Screen.fWidth / 480.0f;
		}

		WindowBezelSize = 1.0f / (32.0f * scalar);
		injector::WriteMemory<float*>(0x0045764D + 2, &WindowBezelSize, true);
		injector::WriteMemory<float*>(0x00457658 + 2, &WindowBezelSize, true);

		struct AdvWindowHook1
		{
			void operator()(injector::reg_pack& regs)
			{
				float Yscalesize = Screen.fHeight / 480.0f;

				if (Screen.Width < Screen.Height)
				{
					//Xscale = Screen.fWidth / 640.0f;
					Yscalesize = Screen.fWidth / 480.0f;
				}
				*(float*)(regs.esp + 0x20) = 112.0f * Yscalesize; // Ysize
			}
		}; injector::MakeInline<AdvWindowHook1>(0x004571A0, 0x004571A8);


		struct AdvWindowHook2
		{
			void operator()(injector::reg_pack& regs)
			{
				float Yscalesize = Screen.fHeight / 480.0f;

				if (Screen.Width < Screen.Height)
				{
					Yscalesize = Screen.fWidth / 480.0f;
				}

				*(float*)(regs.esp + 0x18) = 80.0f * Yscalesize; // Ysize
			}
		}; injector::MakeInline<AdvWindowHook2>(0x004571C4, 0x004571CC);

		injector::MakeCALL(0x00457278, AdvertiseWindowFix::AdvButtonDrawHook);
		injector::MakeCALL(0x00457357, AdvertiseWindowFix::AdvButtonDrawHook);
		injector::MakeCALL(0x0045743C, AdvertiseWindowFix::AdvButtonDrawHook);

		ButtonBezelOffset = 8.0f * scalar;
		injector::WriteMemory<float*>(0x0045720A + 2, &ButtonBezelOffset, true);
		injector::WriteMemory<float*>(0x00457230 + 2, &ButtonBezelOffset, true);
		injector::WriteMemory<float*>(0x0045730F + 2, &ButtonBezelOffset, true);
		injector::WriteMemory<float*>(0x004573F4 + 2, &ButtonBezelOffset, true);

		AdvWindowButtonTextYOffset = 16.0f * scalar;
		AdvWindowButton2TextYOffset = 8.0f * scalar;
		AdvWindowButton2Offset = 40.0f * scalar;
		AdvWindowButton3Offset = 72.0f * scalar;

		injector::WriteMemory<float*>(0x00457286 + 2, &AdvWindowButtonTextYOffset, true);
		injector::WriteMemory<float*>(0x00457365 + 2, &AdvWindowButtonTextYOffset, true);
		injector::WriteMemory<float*>(0x0045744A + 2, &AdvWindowButtonTextYOffset, true);

		injector::WriteMemory<float*>(0x00457372 + 2, &AdvWindowButton2TextYOffset, true);
		injector::WriteMemory<float*>(0x00457457 + 2, &AdvWindowButton2TextYOffset, true);

		injector::WriteMemory<float*>(0x0045731F + 2, &AdvWindowButton2Offset, true);
		injector::WriteMemory<float*>(0x00457404 + 2, &AdvWindowButton3Offset, true);

		injector::WriteMemory<uintptr_t>(0x0075024C, (uintptr_t)&AdvertiseWindowFix::AdvWindowDrawHook, true);

		struct AdvButtonTextHook1
		{
			void operator()(injector::reg_pack& regs)
			{
				float Xsize = *(float*)(regs.esp + 0x24);
				float Yscalesize = Screen.fHeight / 480.0f;

				if (Screen.Width < Screen.Height)
					Yscalesize = Screen.fWidth / 480.0f;

				Xsize -= ((32.0f * Yscalesize) / 2) - 16.0f;
				Xsize *= 0.5;

				*(float*)(regs.esi + 0x114) *= Yscalesize;

				regs.edi = regs.esi + 0x520;
				_asm fld[Xsize]
			}
		}; injector::MakeInline<AdvButtonTextHook1>(0x0045729C, 0x004572AC);

		struct AdvButtonTextHook2
		{
			void operator()(injector::reg_pack& regs)
			{
				float Xsize = *(float*)(regs.esp + 0x24);
				float Yscalesize = Screen.fHeight / 480.0f;

				if (Screen.Width < Screen.Height)
					Yscalesize = Screen.fWidth / 480.0f;

				Xsize -= ((32.0f * Yscalesize) / 2) - 16.0f;
				Xsize *= 0.5f;

				*(float*)(regs.esi + 0x118) *= Yscalesize;

				_asm fld[Xsize]
			}
		}; injector::MakeInline<AdvButtonTextHook2>(0x0045738C, 0x00457396);


		struct AdvButtonTextHook3
		{
			void operator()(injector::reg_pack& regs)
			{
				float Xsize = *(float*)(regs.esp + 0x24);
				float Yscalesize = Screen.fHeight / 480.0f;

				if (Screen.Width < Screen.Height)
					Yscalesize = Screen.fWidth / 480.0f;

				Xsize -= ((32.0f * Yscalesize) / 2) - 16.0f;
				Xsize *= 0.5f;

				*(float*)(regs.esi + 0x11C) *= Yscalesize;

				_asm fld[Xsize]
			}
		}; injector::MakeInline<AdvButtonTextHook3>(0x00457471, 0x0045747B);

		injector::MakeCALL(0x00457F1B, TextDrawFunc2Hook);
		injector::MakeCALL(0x00457F3C, TextDrawFunc2Hook);

		injector::MakeCALL(0x00456CC7, AdvertiseWindowFix::hkAdvWindowConstructor);
		injector::WriteMemory<uintptr_t>(0x00750244, (uintptr_t)&AdvertiseWindowFix::AdvWindowDestructorHook, true);
	}

	// Staff roll text
	if (bFixStaffRoll)
	{
		injector::MakeCALL(0x00454744, TextDrawFunc2Hook);
		injector::WriteMemory<uintptr_t>(0x0074F8FC, (uintptr_t)&AdvStaffRollFix::AdvStaffrollDrawHook, true);
		injector::MakeCALL(0x0045448B, AdvStaffRollFix::hkStaffrollConstructor);
		injector::WriteMemory<uintptr_t>(0x0074F8F4, (uintptr_t)&AdvStaffRollFix::AdvStaffrollDestructorHook, true);

		injector::MakeCALL(0x0045475C, AdvStaffRollFix::AdvStaffrollLogoHook);

	}
}

CEXP void InitializeASI()
{
	std::call_once(CallbackHandler::flag, []()
		{
			CallbackHandler::RegisterCallback(Init, hook::pattern("0F BE 0D ? ? ? ? 8D 0C 89 8B"));
		});
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		if (!IsUALPresent()) { InitializeASI(); }
	}
	return TRUE;
}
