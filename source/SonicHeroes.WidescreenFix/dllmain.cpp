#include "stdafx.h"
#include "../Shared/PatchPreflight.hpp"
#include "FixSettings.hpp"
#include "Cursor.hpp"

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
	float fHalfWidth;
	float fHalfHeight;
	float fAspectRatio;
	float fConditionalAspectRatio;
	float fZoomFactor;
	float fCameraScaleX;
	float fHudZoomFactor;
	float fHudScale;
} Screen;

float iniZoomFactor = 1.0f;
bool bFixLensFlare = true;
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

uintptr_t ptrResX = 0;
uintptr_t ptrResY = 0;

uintptr_t ptrResX2 = 0;
uintptr_t ptrResY2 = 0;

uintptr_t ptrMaestroResX = 0;
uintptr_t ptrMaestroResY = 0;

uintptr_t ptrOneDiv640 = 0x78A08C;

void UpdateViewScales()
{
	Screen.fHudZoomFactor = Screen.fAspectRatio < Screen.fConditionalAspectRatio ? Screen.fAspectRatio / Screen.fConditionalAspectRatio : 1.0f;
	Screen.fZoomFactor = iniZoomFactor * Screen.fHudZoomFactor;
	Screen.fCameraScaleX = ((4.0f / 3.0f) / Screen.fAspectRatio) * Screen.fZoomFactor;
	Screen.fHudScale = ((4.0f / 3.0f) / Screen.fAspectRatio) * Screen.fHudZoomFactor;
	Screen.HeightHUD = static_cast<int>(Screen.Height * Screen.fHudZoomFactor);
	Screen.OffsetX = static_cast<int>(Screen.Width - Screen.Width * Screen.fHudScale) / 2;
	Screen.OffsetY = static_cast<int>(Screen.Height - Screen.Height * Screen.fHudZoomFactor) / 2;
}

void updateValues(const float& newWidth, const float& newHeight)
{
	if (!std::isfinite(newWidth) || !std::isfinite(newHeight) || newWidth <= 0.0f || newHeight <= 0.0f)
		return;
	Screen.fZoomFactor = iniZoomFactor;
	//Screen resolution
	Screen.Width = static_cast<int>(newWidth);
	Screen.Height = static_cast<int>(newHeight);
	Screen.fWidth = static_cast<float>(Screen.Width);
	Screen.fHeight = static_cast<float>(Screen.Height);
	Screen.fHalfWidth = Screen.fWidth * 0.5f;
	Screen.fHalfHeight = Screen.fHeight * 0.5f;
	Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
	Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));

	UpdateViewScales();

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
	ProtectedGameRef<uint32_t>{reinterpret_cast<uint32_t*>(ptrResX)} = Screen.Width;
	ProtectedGameRef<uint32_t>{reinterpret_cast<uint32_t*>(ptrResY)} = Screen.Height;
	ProtectedGameRef<uint32_t>{reinterpret_cast<uint32_t*>(ptrResX2)} = Screen.Width;
	ProtectedGameRef<uint32_t>{reinterpret_cast<uint32_t*>(ptrResY2)} = Screen.Height;

	ProtectedGameRef<float>{reinterpret_cast<float*>(ptrOneDiv640)} = 1.0f / (480.0f * Screen.fAspectRatio);

	ProtectedGameRef<float>{reinterpret_cast<float*>(ptrMaestroResX)} = Screen.fWidth;
	ProtectedGameRef<float>{reinterpret_cast<float*>(ptrMaestroResY)} = Screen.fHeight;

	if (bFixLensFlare)
	{
		// lens flare size
		fLensFlareScalar = Screen.fHeight / 480.0f;
		if (Screen.Width < Screen.Height)
			fLensFlareScalar = Screen.fWidth / 480.0f;
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
		if (WindowedModeWrapper::bScaleWindow || WindowedModeWrapper::bStretchWindow) return TRUE;

		updateValues((float)LOWORD(lParam), (float)HIWORD(lParam));
		return GameWndProc(hWnd, msg, wParam, lParam);
	}

	case WM_SIZING:
	{
		if (WindowedModeWrapper::bScaleWindow || WindowedModeWrapper::bStretchWindow) return TRUE;
		return GameWndProc(hWnd, msg, wParam, lParam);
	}

	case WM_ACTIVATE:
	{
		return TRUE;
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
void InvalidateWindowSize()
{
	ProtectedGameRef<uint32_t>{reinterpret_cast<uint32_t*>(ptrWindowSizeX)} = 640;
}

void __declspec(naked) StretchOnBoot()
{
	if (!bStretchOnBootFlag)
	{
		bStretchOnBootFlag = true;
		// Invalidate it to trigger the window update and device reset.
		_asm
		{
			pushad
			call InvalidateWindowSize
			popad
		}
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

	uintptr_t ptrAdvWindowDestructorFunc = 0x00456F90;
	void __stdcall AdvWindowDestructorHook(char unk)
	{
		void* that;
		_asm mov that, ecx

		free(AdvWindowObj);

		return reinterpret_cast<void(__thiscall*)(void*, char)>(ptrAdvWindowDestructorFunc)(that, unk);
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

	uintptr_t ptrAdvWindowDrawFunc = 0x004570D0;
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

		reinterpret_cast<void(__thiscall*)(char*)>(ptrAdvWindowDrawFunc)(AdvWindowObj);
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

	uintptr_t ptrAdvStaffrollDestructorFunc = 0x4544B0;
	void __stdcall AdvStaffrollDestructorHook(char unk)
	{
		void* that;
		_asm mov that, ecx

		free(StaffrollObjCopy);
		free(scaledStaffRollObjs);

		return reinterpret_cast<void(__thiscall*)(void*, char)>(ptrAdvStaffrollDestructorFunc)(that, unk);
	}

	uintptr_t ptrAdvStaffrollDrawFunc = 0x4545F0;
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

		reinterpret_cast<void(__thiscall*)(char*)>(ptrAdvStaffrollDrawFunc)(StaffrollObjCopy);
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

namespace BetterSync
{
	float fFPSLimit = 60.0f;

	// code from: https://github.com/ThirteenAG/d3d8-wrapper/blob/c2509f26d22c0813bb6eced277defe67628acd90/source/dllmain.cpp#L55
	class FrameLimiter
	{
	private:
		static inline double TIME_Frequency = 0.0;
		static inline double TIME_Ticks = 0.0;
		static inline double TIME_Frametime = 0.0;

	public:
		enum FPSLimitMode { FPS_NONE, FPS_REALTIME, FPS_ACCURATE };
		static void Init(FPSLimitMode mode)
		{
			LARGE_INTEGER frequency;

			QueryPerformanceFrequency(&frequency);
			static constexpr auto TICKS_PER_FRAME = 1;
			auto TICKS_PER_SECOND = (TICKS_PER_FRAME * fFPSLimit);
			if (mode == FPS_ACCURATE)
			{
				TIME_Frametime = 1000.0 / (double)fFPSLimit;
				TIME_Frequency = (double)frequency.QuadPart / 1000.0; // ticks are milliseconds
			}
			else // FPS_REALTIME
			{
				TIME_Frequency = (double)frequency.QuadPart / (double)TICKS_PER_SECOND; // ticks are 1/n frames (n = fFPSLimit)
			}
			Ticks();
		}
		static DWORD Sync_RT()
		{
			DWORD lastTicks, currentTicks;
			LARGE_INTEGER counter;

			QueryPerformanceCounter(&counter);
			lastTicks = (DWORD)TIME_Ticks;
			TIME_Ticks = (double)counter.QuadPart / TIME_Frequency;
			currentTicks = (DWORD)TIME_Ticks;

			return (currentTicks > lastTicks) ? currentTicks - lastTicks : 0;
		}
		static DWORD Sync_SLP()
		{
			LARGE_INTEGER counter;
			QueryPerformanceCounter(&counter);
			double millis_current = (double)counter.QuadPart / TIME_Frequency;
			double millis_delta = millis_current - TIME_Ticks;
			if (TIME_Frametime <= millis_delta)
			{
				TIME_Ticks = millis_current;
				return 1;
			}
			else if (TIME_Frametime - millis_delta > 2.0) // > 2ms
				Sleep(1); // Sleep for ~1ms
			else
				Sleep(0); // yield thread's time-slice (does not actually sleep)

			return 0;
		}

	private:
		static void Ticks()
		{
			LARGE_INTEGER counter;
			QueryPerformanceCounter(&counter);
			TIME_Ticks = (double)counter.QuadPart / TIME_Frequency;
		}
	};

	FrameLimiter::FPSLimitMode mFPSLimitMode = FrameLimiter::FPSLimitMode::FPS_ACCURATE;

	uintptr_t sub_6C4FC0 = 0x6C4FC0;
	uintptr_t MovieIDPtr = 0x8DB5B0;

	void CustomSyncFunc()
	{
		if ((*(int32_t*)MovieIDPtr > 0))
			return reinterpret_cast<void(*)()>(sub_6C4FC0)();

		if (mFPSLimitMode == FrameLimiter::FPSLimitMode::FPS_REALTIME)
			while (!FrameLimiter::Sync_RT());
		else if (mFPSLimitMode == FrameLimiter::FPSLimitMode::FPS_ACCURATE)
			while (!FrameLimiter::Sync_SLP());
	}
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

bool SupportedExecutable()
{
    if (reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)) != 0x400000)
        return false;
    const auto matches = [](uintptr_t address, std::initializer_list<uint8_t> expected)
    {
        uint8_t actual[8]{};
        SIZE_T read{};
        return expected.size() <= sizeof(actual) &&
            ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address), actual, expected.size(), &read) &&
            read == expected.size() && std::equal(expected.begin(), expected.end(), actual);
    };
    return
        matches(0x78a08c, { 0xCD, 0xCC, 0xCC, 0x3A }) &&
        matches(0x6567bf, { 0x3B, 0xD0, 0x75, 0x12, 0x8B, 0x44, 0x24, 0x24 }) &&
        matches(0x48f41f, { 0x8B, 0x48, 0x30, 0x8B, 0x56, 0x18, 0x8B, 0x42 }) &&
        matches(0x456990, { 0x88, 0x58, 0x27, 0x88, 0x58, 0x1E, 0x88, 0x48 }) &&
        matches(0x456f90, { 0x56, 0x8B, 0xF1, 0xE8, 0x28, 0x00, 0x00, 0x00 }) &&
        matches(0x456aa0, { 0x33, 0xC9, 0x89, 0x08, 0x89, 0x48, 0x04, 0x89 }) &&
        matches(0x457710, { 0x83, 0xEC, 0x5C, 0x8B, 0x15, 0x4C, 0x0A, 0x8E }) &&
        matches(0x4570d0, { 0x83, 0xEC, 0x28, 0x53, 0x32, 0xC0, 0x56, 0x8B }) &&
        matches(0x454f10, { 0x8B, 0x87, 0x88, 0x00, 0x00, 0x00, 0x53, 0x33 }) &&
        matches(0x4544b0, { 0x56, 0x8B, 0xF1, 0x56, 0xE8, 0x27, 0x00, 0x00 }) &&
        matches(0x4545f0, { 0x53, 0x8B, 0xD9, 0x56, 0x8B, 0x73, 0x38, 0x85 }) &&
        matches(0x454fb0, { 0x83, 0xEC, 0x7C, 0x57, 0x8D, 0x44, 0x24, 0x04 }) &&
        matches(0x458200, { 0x83, 0xEC, 0x0C, 0x85, 0xC0, 0x0F, 0x84, 0xB8 }) &&
        matches(0x6c4fc0, { 0xA1, 0x04, 0x21, 0x8E, 0x00, 0x83, 0xEC, 0x0C });
}

void Init()
{
    if (!SupportedExecutable())
    {
        OutputDebugStringA("Sonic Heroes: unsupported executable; game patches disabled.\n");
        return;
    }
    if (!wfp::PatchPreflight("Sonic Heroes display: signatures unavailable; feature disabled.\n",
    {
        { "? ? ? 6A 1A ? FF 15", 1 },
        { "A1 ? ? ? ? 85 C0 75 0A C7 05 ? ? ? ? 80 02 00 00", 1 },
        { "C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? 01 00 00 00 C7 05 ? ? ? ? 02 00 00 00 C7 05 ? ? ? ? ? ? ? ? B8 01 00 00 00", 1 },
        { "B8 01 00 00 00 83 C4 08 84 C8 74 22", 1 },
        { "A1 ? ? ? ? 8B 0D ? ? ? ? 53 68 00 00 C8 00", 1 },
        { "E8 ? ? ? ? 8B D8 33 ED 3B DE 7E 5A", 1 },
        { "C7 44 24 00 00 00 80 3F EB 0A 8B 0D ? ? ? ? 89 4C 24 00 8B 15 ? ? ? ? A1", 1 },
        { "50 FF 92 30 01 00 00 8A 44 24 1C 83 C4 0C A2 ? ? ? ? E9 ? ? ? ?", 1 },
        { "BD 80 02 00 00 C7 44 24 18 E0 01 00 00", 1 },
        { "FF 15 ? ? ? ? 8B 54 24 14 8B 44 24 10 52 50 68 10 01 00 00", 1 },
        { "6A 06 50 FF D5 A1 ? ? ? ? 3B C3 C6 05 ? ? ? ? 01", 1 },
        { "0F BF 4E ? 0F BF C0 89 44 ? ? DB 44 ? ? 89 4C ? ? 85 DB", 1 },
        { "66 8B ? ? 66 85 C0 75 29 66 39 ? ? 75 23", 1 },
        { "D9 42 68 D8 08 D9 42 68 D8 48 04 D9 42 68 D8 48 08", 1 },
        { "D9 05 ? ? ? ? 89 4E 68 8B 50 04 D8 76 68", 1 },
        { "83 EC 14 56 57 E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ?", 1 },
        { "8B 48 60 DB 41 0C 83 C4 ? BF ? ? ? ? BE ? ? ? ? D9 5C", 1 },
        { "D9 44 ? ? D8 64 ? ? 6A 04 68 ? ? ? ? 6A 04 D9 15 ? ? ? ? D9 1D ? ? ? ? D9 44", 1 },
        { "DB 05 ? ? ? ? D8 0F D8 0D ? ? ? ? D9 9E ? ? ? ? 8B 57 04 89 96 ? ? ? ?", 1 },
        { "DB 05 ? ? ? ? D9 5C ? ? DB 05 ? ? ? ? D9 5C ? ? E8 ? ? ? ? D9 05 ? ? ? ?", 1 },
        { "8B D1 C1 EA 10 0F B6 C6 C1 E0 08 0F B6 F9 0B C7 89 4C ? ? C1 E0 08", 1 },
        { "8B 44 ? ? EB 10 8B 1D ? ? ? ? 8B 2D ? ? ? ? 89 5C ? ?", 1 },
        { "DB 41 0C 8B B6 ? ? ? ? 83 C4 08 83 EE 02", 1 },
        { "DB 05 ? ? ? ? D9 5C ? ? DB 47 ? D8 4C ? ? D8 0D ? ? ? ? D9 5C ? ? DB 05 ? ? ? ?", 1 },
        { "8B 48 60 DB 41 0C 6A 04 68 ? ? ? ? 6A 04 D8 4C ? ?", 1 },
        { "8B 48 60 DB 41 0C D9 5C ? ? E8 ? ? ? ? 8B 50 60 DB 42 10 D9 5C ? ? E8 ? ? ? ? D9 05", 1 },
        { "DB 40 0C 8B 54 ? ? 8B 44 ? ? 8B 4D 34 D8 0D ? ? ? ? 4A 89 54 ? ? BF 01 00 00 00 3B C7", 1 },
        { "8B 48 60 DB 41 0C E8 ? ? ? ? 8B 50 60 DB 42 10 D9 5C ? ? E8 ? ? ? ? D9 05 ? ? ? ? BE", 1 },
        { "8A 44 B4 ? C1 E0 08 0B C1 C1 E0 08 0B C2 A3", 1 },
        { "66 C7 46 1E 54 00 DB 05", 1 },
        { "DB 05 ? ? ? ? 8B 00 D9 84 ? ? ? ? ? 53 55 D8 C9 57 50 6A 01 D8 0D ? ? ? ? D9 9C", 1 },
        { "DB 05 ? ? ? ? A1 ? ? ? ? 83 C4 08 D9 54 ? ? D8 8C", 1 },
        { "E8 ? ? ? ? 83 C4 10 E8 ? ? ? ? 5F C3 CC CC CC CC CC CC 83 EC 0C 56 33 F6 89 74", 1 },
        { "DB 05 ? ? ? ? A1 ? ? ? ? 0F B6 9C 24 ? ? ? ? D9 54 24 ? C1 E3 18", 1 },
        { "DB 05 ? ? ? ? A1 ? ? ? ? 83 C4 08 D9 54 24 ? BF ? ? ? ? D8 8C 24 ? ? ? ? 8D 74 24 ?", 1 },
        { "85 C0 74 0F 8B 46 28 85 C0 75 08 C7 44 24 04 00 80 FD 43 8A 42 1E 84 C0 74 0E", 1 },
        { "D8 0D ? ? ? ? D9 5B 3C DB 05 ? ? ? ? BF 01 00 00 00", 1 },
        { "8B 15 ? ? ? ? 8D 4C 24 10 51 68 ? ? ? ? 50 FF 92 30 01 00 00 8B 4C 24 1C 33 C0 83 C4 0C 3B CD 0F 95 C0 A3 ? ? ? ? E9", 9 },
        { "B9 00 01 00 00 33 C0 8B FE F3 AB A1", 1 },
        { "83 C4 20 83 F9 02", 1 },
        { "83 EC 78 85 C0 75 04 83 C4 78 C3 8B 00", 1 },
        { "81 EC 98 00 00 00 85 C0 75 07 81 C4 98 00 00 00 C3 8B 00 53", 1 },
        { "81 EC A0 00 00 00 53 33 DB 3B C3 75 0C 33 C0 5B 81 C4 A0 00 00 00", 1 },
        { "8A 51 08 81 EC 98 00 00 00 84 D2", 1 },
        { "8B 40 60 DB 40 0C 8B 54 24 10 8B 44 24", 1 },
        { "68 00 7F 00 00 56 C7 44 24 0C 30 00 00 00", 1 },
        { "D9 C9 C1 E0 08 D9 5C ? ? 0B C1 C1 E0 08 D9 5C ? ? 0B C2 8D 4C ? ? BA", 1 },
        { "C1 E0 08 0B C2 D9 54 ? ? C1 E0 08 D9 41 ? 0F B6 D3 D8 05 ? ? ? ? 0B C2 8B 54", 1 },
        { "D9 00 8B 39 D8 02 8B 50 04 89 54 ? ? 8B 50 08 89 7C ? ? D9 54", 1 },
    }))
        return;

	auto& settings = FixSettings();
	asi_settings::LiveLink::Start();
	SonicCursor::Initialize();
	DWORD dummyoldprotect = 0;
	Screen.Width = settings.Int("MAIN.ResX");
	Screen.Height = settings.Int("MAIN.ResY");
	Screen.fConditionalAspectRatio = settings.Float("MAIN.MinAspectRatio");
	const float factor = settings.Float("MAIN.FOVFactor");
	iniZoomFactor = std::isfinite(factor) ? std::clamp(factor, 0.5f, 2.0f) : 1.0f;
	settings.OnChange("MAIN.FOVFactor", []()
	{
		const float value = FixSettings().Float("MAIN.FOVFactor");
		iniZoomFactor = std::isfinite(value) ? std::clamp(value, 0.5f, 2.0f) : 1.0f;
		UpdateViewScales();
	});
	const int windowMode = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2);
	const int borderlessScaling = std::clamp(settings.Int("MAIN.BorderlessScaling"), 0, 2);
	const int nWindowedMode = windowMode == 0 ? 0 : (windowMode == 1 ? (settings.Bool("MAIN.WindowResize") ? 3 : 2) : (borderlessScaling == 1 ? 4 : (borderlessScaling == 2 ? 1 : 5)));
	WindowedModeWrapper::EnableDpiAwareness();
	fShadowScale = settings.Float("MISC.ShadowScale");
	static uint32_t ShadowRes = settings.Int("GRAPHICS.ShadowResolution");
	static float ClipRange = settings.Float("MISC.ClipRange");
	bFixLensFlare = settings.Bool("MISC.FixLensFlare");
	bFixAdvertiseWindows = settings.Bool("MISC.FixAdvertiseWindows");
	bFixStaffRoll = settings.Bool("MISC.FixStaffRoll");
	static uint32_t WarpSwitchEffectMode = settings.Int("MISC.WarpSwitchEffectMode");
	static bool bDisableMouseInput = settings.Bool("MISC.DisableMouseInput");
	static bool bDisableFrameSkipping = settings.Bool("MISC.DisableFrameSkipping");
	static bool bRestoreDemos = settings.Bool("MISC.RestoreDemos");
	static bool bDisableCDCheck = settings.Bool("MISC.DisableCDCheck");
	static bool bDisableQuitDialog = settings.Bool("MISC.DisableQuitDialog");
	static bool bDisableLoadingTimer = settings.Bool("MISC.DisableLoadingTimer");
	static bool bDisableSubtitles = settings.Bool("MISC.DisableSubtitles");
	static bool bIncreaseObjectDistance = settings.Bool("MISC.IncreaseObjectDistance");
	static uint8_t MinObjDistance = settings.Int("MISC.MinObjDistance") & 0xFF;
	static float fObjDistanceScale = settings.Float("GRAPHICS.DrawDistance");
	static bool bBetterFrameSync = settings.Bool("MISC.BetterFrameSync");
	BetterSync::mFPSLimitMode = (BetterSync::FrameLimiter::FPSLimitMode)settings.Int("MISC.FPSLimitMode");
	BetterSync::fFPSLimit = settings.Float("MAIN.FPSLimit");
	
	static std::filesystem::path CustomUserDir;
	static auto szCustomUserFilesDirectoryInGameDir = settings.String("MISC.CustomUserFilesDirectoryInGameDir");
	if (szCustomUserFilesDirectoryInGameDir.empty() || szCustomUserFilesDirectoryInGameDir == "0")
		szCustomUserFilesDirectoryInGameDir.clear();

	// demo/trial version detect -- the string for the user directory is different in these builds
	static bool bDemoVersion = false;
	uintptr_t loc_629D23 = reinterpret_cast<uintptr_t>(hook::pattern("? ? ? 6A 1A ? FF 15").get_first(0)) + 0x12;
	char* userPath = *(char**)(loc_629D23 + 1);
	if (strstr(userPath, "TRIAL") || strstr(userPath, "DEMO"))
	{
		bDemoVersion = true;
	}

	// SkipFE-like functions for Sonic Heroes
	// credit to: https://github.com/Sewer56/Heroes.Utils.DebugBoot.ReloadedII
	static bool bSkipFE = settings.Bool("SkipFE.Enabled");
	static int32_t SysMode = settings.Int("SkipFE.SystemMode");

	if ((SysMode == SystemMode::InGame) && bSkipFE)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "8B 04 85 ? ? ? ? 83 F8 ? 7C 0B", 1 },
            { "83 F8 05 7D 05 A3 ? ? ? ? A1", 1 },
        }))
        {

            uintptr_t StageIDPtr = *(uintptr_t*)(reinterpret_cast<uintptr_t>(hook::pattern("8B 04 85 ? ? ? ? 83 F8 ? 7C 0B").get_first(0)) + 3);
            uintptr_t loc_41A6FC = reinterpret_cast<uintptr_t>(hook::pattern("83 F8 05 7D 05 A3 ? ? ? ? A1").get_first(0)) + 5;
            uintptr_t loc_41A70B = loc_41A6FC + 0xF;
            uintptr_t loc_41A71B = loc_41A6FC + 0x1F;
            uintptr_t loc_41A72C = loc_41A6FC + 0x30;

            uintptr_t Team1Ptr = *reinterpret_cast<uintptr_t*>(loc_41A6FC + 1);
            uintptr_t Team2Ptr = *reinterpret_cast<uintptr_t*>(loc_41A70B + 1);
            uintptr_t Team3Ptr = *reinterpret_cast<uintptr_t*>(loc_41A71B + 2);
            uintptr_t Team4Ptr = *reinterpret_cast<uintptr_t*>(loc_41A72C + 2);


            ProtectedGameRef<int32_t>{reinterpret_cast<int32_t*>(StageIDPtr)} = settings.Int("SkipFE.Stage");
            ProtectedGameRef<int32_t>{reinterpret_cast<int32_t*>(Team1Ptr)} = settings.Int("SkipFE.Team1");
            ProtectedGameRef<int32_t>{reinterpret_cast<int32_t*>(Team2Ptr)} = settings.Int("SkipFE.Team2");
            ProtectedGameRef<int32_t>{reinterpret_cast<int32_t*>(Team3Ptr)} = settings.Int("SkipFE.Team3");
            ProtectedGameRef<int32_t>{reinterpret_cast<int32_t*>(Team4Ptr)} = settings.Int("SkipFE.Team4");
		
        }
	}

	if ((SysMode == SystemMode::EasyMenuMovie) && bSkipFE)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "C7 40 3C 05 00 00 00 5D C7 05 ? ? ? ? FF FF FF FF", 1 },
        }))
        {

            uintptr_t MovieIDPtr = *(uintptr_t*)(reinterpret_cast<uintptr_t>(hook::pattern("C7 40 3C 05 00 00 00 5D C7 05 ? ? ? ? FF FF FF FF").get_first(0)) + 0xA);
            ProtectedGameRef<int32_t>{reinterpret_cast<int32_t*>(MovieIDPtr)} = settings.Int("SkipFE.Movie");
        }
	}
	if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
		std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
	if (Screen.Width <= 0 || Screen.Height <= 0)
		std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

	Screen.fWidth = static_cast<float>(Screen.Width);
	Screen.fHeight = static_cast<float>(Screen.Height);
	Screen.fHalfWidth = Screen.fWidth * 0.5f;
	Screen.fHalfHeight = Screen.fHeight * 0.5f;
	Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
	Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));

	UpdateViewScales();

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
	uintptr_t loc_427719 = reinterpret_cast<uintptr_t>(hook::pattern("A1 ? ? ? ? 85 C0 75 0A C7 05 ? ? ? ? 80 02 00 00").get_first(0));
	injector::MakeJMP(loc_427719, loc_427719 + 0x26);
	uintptr_t sub_444890 = reinterpret_cast<uintptr_t>(hook::pattern("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? 01 00 00 00 C7 05 ? ? ? ? 02 00 00 00 C7 05 ? ? ? ? ? ? ? ? B8 01 00 00 00").get_first(0)) - 0x48;
	injector::MakeNOP(sub_444890 + 2, 5);
	injector::MakeNOP(sub_444890 + 7, 5);
	uintptr_t loc_446B04 = reinterpret_cast<uintptr_t>(hook::pattern("B8 01 00 00 00 83 C4 08 84 C8 74 22").get_first(0)) + 8;
	injector::MakeJMP(loc_446B04, loc_446B04 + 0x4D);

	// write resolution vars
	uintptr_t loc_446DB1 = reinterpret_cast<uintptr_t>(hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 53 68 00 00 C8 00").get_first(0));
	ptrResX = *reinterpret_cast<uintptr_t*>(loc_446DB1 + 1);
	ptrResY = *reinterpret_cast<uintptr_t*>(loc_446DB1 + 7);
	uintptr_t loc_446369 = reinterpret_cast<uintptr_t>(hook::pattern("E8 ? ? ? ? 8B D8 33 ED 3B DE 7E 5A").get_first(0)) - 0x10;
	ptrResX2 = *reinterpret_cast<uintptr_t*>(loc_446369) + 4;
	ptrResY2 = *reinterpret_cast<uintptr_t*>(loc_446369) + 8;

	uintptr_t loc_6A9C7D = reinterpret_cast<uintptr_t>(hook::pattern("C7 44 24 00 00 00 80 3F EB 0A 8B 0D ? ? ? ? 89 4C 24 00 8B 15 ? ? ? ? A1").get_first(0)) + 0x1F;
	uintptr_t loc_6A9C87 = loc_6A9C7D + 0xA;
	ptrMaestroResX = *reinterpret_cast<uintptr_t*>(loc_6A9C7D + 2);
	ptrMaestroResY = *reinterpret_cast<uintptr_t*>(loc_6A9C87 + 2);

	ProtectedGameRef<uint32_t>{reinterpret_cast<uint32_t*>(ptrResX)} = Screen.Width;
	ProtectedGameRef<uint32_t>{reinterpret_cast<uint32_t*>(ptrResY)} = Screen.Height;
	ProtectedGameRef<uint32_t>{reinterpret_cast<uint32_t*>(ptrResX2)} = Screen.Width;
	ProtectedGameRef<uint32_t>{reinterpret_cast<uint32_t*>(ptrResY2)} = Screen.Height;
	ProtectedGameRef<float>{reinterpret_cast<float*>(ptrMaestroResX)} = Screen.fWidth;
	ProtectedGameRef<float>{reinterpret_cast<float*>(ptrMaestroResY)} = Screen.fHeight;

	uintptr_t loc_629F5F = reinterpret_cast<uintptr_t>(hook::pattern("50 FF 92 30 01 00 00 8A 44 24 1C 83 C4 0C A2 ? ? ? ? E9 ? ? ? ?").get_first(0)) + 0xE;
	uintptr_t Screen_Size_Selection = *reinterpret_cast<uintptr_t*>(loc_629F5F + 1);
	ProtectedGameRef<uint8_t>{reinterpret_cast<uint8_t*>(Screen_Size_Selection)} = 7; // Screen_Size_Selection -- force it to 7 (1280x960 32-bit)
	// disable the write
	injector::MakeNOP(loc_629F5F, 5);

	// fix for the split screen screen buffer -- force read the current res
	uintptr_t loc_61D422 = reinterpret_cast<uintptr_t>(hook::pattern("BD 80 02 00 00 C7 44 24 18 E0 01 00 00").get_first(0));
	injector::MakeJMP(loc_61D422, loc_61D422 - 8);

	// fix "end the game" dialog to open at the center & have focus (disabling the minimize because minimized windows and their children lose focus!)
	uintptr_t loc_446C85 = reinterpret_cast<uintptr_t>(hook::pattern("FF 15 ? ? ? ? 8B 54 24 14 8B 44 24 10 52 50 68 10 01 00 00").get_first(0));
	injector::MakeNOP(loc_446C85, 6);
	injector::MakeCALL(loc_446C85, UpdateWindowHook);
	uintptr_t loc_446F33 = reinterpret_cast<uintptr_t>(hook::pattern("6A 06 50 FF D5 A1 ? ? ? ? 3B C3 C6 05 ? ? ? ? 01").get_first(0));
	injector::MakeJMP(loc_446F33, loc_446F33 + 5);

	if (bFixLensFlare)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "BF 00 00 00 3F D9 44 24 18 D8 C9", 1 },
            { "C7 44 24 28 0A D7 23 3C", 1 },
        }))
        {

            uintptr_t loc_48EF26 = reinterpret_cast<uintptr_t>(hook::pattern("BF 00 00 00 3F D9 44 24 18 D8 C9").get_first(0));
            uintptr_t loc_48EF31 = loc_48EF26 + 0xB;
            uintptr_t loc_48EF3F = loc_48EF26 + 0x19;

            uintptr_t loc_48EFE6 = loc_48EF26 + 0xC0;
            uintptr_t loc_48EFF4 = loc_48EF26 + 0xCE;

            uintptr_t loc_48F497 = reinterpret_cast<uintptr_t>(hook::pattern("C7 44 24 28 0A D7 23 3C").get_first(0));
            uintptr_t loc_48F454 = loc_48F497 - 0x43;
            uintptr_t loc_48F47C = loc_48F497 - 0x1B;
            uintptr_t loc_48F419 = loc_48F497 - 0x7E;

            loc_48F41F = loc_48F497 - 0x78;

            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_48EF31 + 2)} = &Screen.fWidth;
            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_48EF3F + 2)} = &Screen.fHeight;

            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_48EFE6 + 2)} = &Screen.fHalfWidth;
            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_48EFF4 + 2)} = &Screen.fHalfHeight;

            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_48F454 + 2)} = &Screen.fWidth;
            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_48F47C + 2)} = &Screen.fHeight;

            // lens flare size
            fLensFlareScalar = Screen.fHeight / 480.0f;
            if (Screen.Width < Screen.Height)
                fLensFlareScalar = Screen.fWidth / 480.0f;

            injector::MakeJMP(loc_48F419, LensFlareScale, true);
        }
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

	injector::MakeInline<ResHook6>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(7));
	pattern = hook::pattern("66 8B ? ? 66 85 C0 75 29 66 39 ? ? 75 23"); // 662BFB
	ProtectedGameRef<uint8_t>{reinterpret_cast<uint8_t*>(pattern.count_hint(1).get(0).get<int8_t>(7))} = 0xEB;

	pattern = hook::pattern("D9 42 68 D8 08 D9 42 68 D8 48 04 D9 42 68 D8 48 08"); // 0x64AFDC
	struct CutOffAreaHook
	{
		void operator()(injector::reg_pack& regs)
		{
			float edx68 = *(float*)(regs.edx + 0x68) / Screen.fCameraScaleX;
			float eax00 = *(float*)(regs.eax + 0);
			float eax04 = *(float*)(regs.eax + 4);
			float eax08 = *(float*)(regs.eax + 8);
			_asm
			{
				fld     dword ptr[edx68]
				fmul    dword ptr[eax00]
				fld     dword ptr[edx68]
				fmul    dword ptr[eax04]
				fld     dword ptr[edx68]
				fmul    dword ptr[eax08]
			}
		}
	};

	struct CutOffAreaHookY
	{
		void operator()(injector::reg_pack& regs)
		{
			float yScale = *(float*)(regs.edx + 0x6C) / Screen.fZoomFactor * *(float*)(regs.eax + 0x10);
		}
	};

	struct CutOffAreaHookY2
	{
		void operator()(injector::reg_pack& regs)
		{
			float yScale = *(float*)(regs.edx + 0x6C) / Screen.fZoomFactor * *(float*)(regs.eax + 0x14);
		}
	};

	struct CutOffAreaHookY3
	{
		void operator()(injector::reg_pack& regs)
		{
			float yScale = *(float*)(regs.edx + 0x6C) / Screen.fZoomFactor * *(float*)(regs.eax + 0x18);
		}
	};

	injector::MakeInline<CutOffAreaHook>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(17));
	injector::MakeInline<CutOffAreaHookY>(pattern.count_hint(1).get(0).get<uint32_t>(42), pattern.count_hint(1).get(0).get<uint32_t>(48));
	injector::MakeInline<CutOffAreaHookY2>(pattern.count_hint(1).get(0).get<uint32_t>(54), pattern.count_hint(1).get(0).get<uint32_t>(60));
	injector::MakeInline<CutOffAreaHookY3>(pattern.count_hint(1).get(0).get<uint32_t>(64), pattern.count_hint(1).get(0).get<uint32_t>(70));

	uintptr_t loc_64AC8B = reinterpret_cast<uintptr_t>(hook::pattern("D9 05 ? ? ? ? 89 4E 68 8B 50 04 D8 76 68").get_first(0));
	uintptr_t loc_64ACA5 = loc_64AC8B + 0x1A;

	uintptr_t loc_63AA3B = reinterpret_cast<uintptr_t>(hook::pattern("83 EC 14 56 57 E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ?").get_first(0)) + 0xB;
	static uintptr_t ShadowStuffAddr = *reinterpret_cast<uintptr_t*>(loc_63AA3B) + 0xC;

	// TODO - if any more things break thanks to this jank here, target function at 64AC80 in specific places instead
	struct ExceptionalHudScale1
	{
		void operator()(injector::reg_pack& regs)
		{
			if (*(uint32_t*)(regs.esp + 8 + 4) == ShadowStuffAddr)
				_asm fld1
			else
				_asm fld Screen.fHudScale
		}
	}; injector::MakeInline<ExceptionalHudScale1>(loc_64AC8B, loc_64AC8B + 6);

	struct ExceptionalHudScale2
	{
		void operator()(injector::reg_pack& regs)
		{
			if (*(uint32_t*)(regs.esp + 8 + 4) == ShadowStuffAddr)
				_asm fld1
			else
				_asm fld Screen.fHudZoomFactor
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
			float ResolutionY = (float)(*(int*)(regs.ecx + 0x10)) * Screen.fHudZoomFactor;
			_asm
			{
				fld dword ptr[ResolutionY]
			}
		}
	};

	injector::MakeInline<HUDHor>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(6));
	injector::MakeInline<HUDVert>(pattern.count_hint(1).get(0).get<uint32_t>(23), pattern.count_hint(1).get(0).get<uint32_t>(29));

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

	injector::MakeInline<HUDPos>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(8));

	pattern = hook::pattern("DB 05 ? ? ? ? D8 0F D8 0D ? ? ? ? D9 9E ? ? ? ? 8B 57 04 89 96 ? ? ? ?"); // 0x4543FF
	ProtectedGameRef<decltype(&Screen.Width43)>{reinterpret_cast<decltype(&Screen.Width43)*>(pattern.count_hint(1).get(0).get<uint32_t>(2))} = &Screen.Width43;

	pattern = hook::pattern("DB 05 ? ? ? ? D9 5C ? ? DB 05 ? ? ? ? D9 5C ? ? E8 ? ? ? ? D9 05 ? ? ? ?"); // 0x4548B1

	struct UIHook
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

	injector::MakeInline<UIHook>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(6));
	ProtectedGameRef<decltype(&Screen.HeightHUD)>{reinterpret_cast<decltype(&Screen.HeightHUD)*>(pattern.count_hint(1).get(0).get<uint32_t>(12))} = &Screen.HeightHUD;

	pattern = hook::pattern("8B D1 C1 EA 10 0F B6 C6 C1 E0 08 0F B6 F9 0B C7 89 4C ? ? C1 E0 08"); // 0x454FFD

	struct UIHook2
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.AspectRatioAffected = true;
			regs.edx = regs.ecx;
			regs.edx >>= 10;
		}
	};

	injector::MakeInline<UIHook2>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(5));

	pattern = hook::pattern("8B 44 ? ? EB 10 8B 1D ? ? ? ? 8B 2D ? ? ? ? 89 5C ? ?"); //0x6445C0

	ProtectedGameRef<decltype(&Screen.WidthFMV)>{reinterpret_cast<decltype(&Screen.WidthFMV)*>(pattern.count_hint(1).get(0).get<uint32_t>(8))} = &Screen.WidthFMV;
	ProtectedGameRef<decltype(&Screen.HeightFMV)>{reinterpret_cast<decltype(&Screen.HeightFMV)*>(pattern.count_hint(1).get(0).get<uint32_t>(14))} = &Screen.HeightFMV;
	
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
			float ResolutionY = (float)(*(int*)(regs.ecx + 0x10)) * Screen.fHudZoomFactor;
			_asm
			{
				fld dword ptr[ResolutionY]
			}
			*(float*)(regs.esp + 0x10) = ResolutionY;
		}
	};

	pattern = hook::pattern("DB 41 0C 8B B6 ? ? ? ? 83 C4 08 83 EE 02"); // 0x44296C
	injector::MakeInline<MissionTextHor>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(9));
	injector::MakeInline<MissionTextVert>(pattern.count_hint(1).get(0).get<uint32_t>(19), pattern.count_hint(1).get(0).get<uint32_t>(26));

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
	injector::MakeInline<NowLoadingTextPos>(pattern.count_hint(1).get(0).get<uint32_t>(69), pattern.count_hint(1).get(0).get<uint32_t>(75));

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
			float ResolutionY = (float)(*(int*)(regs.edx + 0x10)) * Screen.fHudZoomFactor;
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
	injector::MakeInline<PowerupIcon1>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(6));
	pattern = hook::pattern("8B 48 60 DB 41 0C D9 5C ? ? E8 ? ? ? ? 8B 50 60 DB 42 10 D9 5C ? ? E8 ? ? ? ? D9 05"); // 0x479EB1
	injector::MakeInline<PowerupIcon2>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(6));
	injector::MakeInline<PowerupIcon3>(pattern.count_hint(1).get(0).get<uint32_t>(15), pattern.count_hint(1).get(0).get<uint32_t>(21));
	pattern = hook::pattern("DB 40 0C 8B 54 ? ? 8B 44 ? ? 8B 4D 34 D8 0D ? ? ? ? 4A 89 54 ? ? BF 01 00 00 00 3B C7"); // 0x479F26
	injector::MakeInline<PowerupIcon4>(pattern.count_hint(1).get(0).get<uint32_t>(25), pattern.count_hint(1).get(0).get<uint32_t>(30));

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
			float ResolutionY = (float)(*(int*)(regs.edx + 0x10)) * Screen.fHudZoomFactor;
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

	injector::MakeInline<ResultsHor>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(6));
	injector::MakeInline<ResultsVert>(pattern.count_hint(1).get(0).get<uint32_t>(11), pattern.count_hint(1).get(0).get<uint32_t>(17));
	pattern = hook::pattern("8A 44 B4 ? C1 E0 08 0B C1 C1 E0 08 0B C2 A3"); //0x438E42
	injector::MakeInline<ResultsPos>(pattern.count_hint(1).get(0).get<uint32_t>(4), pattern.count_hint(1).get(0).get<uint32_t>(9));

	// Special Stage
	fDustWidth = 40.0f * Screen.fAspectRatio;
	static float fDustHeight = 40.0f;

	uintptr_t loc_52C5FB = reinterpret_cast<uintptr_t>(hook::pattern("66 C7 46 1E 54 00 DB 05").get_first(0)) + 0xF3;
	uintptr_t loc_52C607 = loc_52C5FB + 0xC;

	ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_52C5FB + 2)} = &fDustWidth;
	ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_52C607 + 2)} = &fDustHeight;

	pattern = hook::pattern("DB 05 ? ? ? ? 8B 00 D9 84 ? ? ? ? ? 53 55 D8 C9 57 50 6A 01 D8 0D ? ? ? ? D9 9C"); // 0x45894A
	injector::MakeInline<UIHook>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(6));
	ProtectedGameRef<decltype(&Screen.HeightHUD)>{reinterpret_cast<decltype(&Screen.HeightHUD)*>(pattern.count_hint(1).get(0).get<uint32_t>(38))} = &Screen.HeightHUD;

	pattern = hook::pattern("DB 05 ? ? ? ? A1 ? ? ? ? 83 C4 08 D9 54 ? ? D8 8C"); // 0x526F83
	injector::MakeInline<UIHook>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(6));
	ProtectedGameRef<decltype(&Screen.HeightHUD)>{reinterpret_cast<decltype(&Screen.HeightHUD)*>(pattern.count_hint(1).get(0).get<uint32_t>(40))} = &Screen.HeightHUD;

	ProtectedGameRef<decltype(&Screen.Width43)>{reinterpret_cast<decltype(&Screen.Width43)*>(pattern.count_hint(1).get(0).get<uint32_t>(236))} = &Screen.Width43;


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

	injector::MakeInline<MeteorRightPos>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(5));
	ProtectedGameRef<uint16_t>{reinterpret_cast<uint16_t*>(pattern.count_hint(1).get(0).get<uint16_t>(5))} = 0xD1EB;
	injector::MakeNOP(pattern.count_hint(1).get(0).get<uint16_t>(7), 1, true);

	pattern = hook::pattern("DB 05 ? ? ? ? A1 ? ? ? ? 0F B6 9C 24 ? ? ? ? D9 54 24 ? C1 E3 18"); // 0x5263DE

	injector::MakeInline<UIHook>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(6));
	ProtectedGameRef<decltype(&Screen.HeightHUD)>{reinterpret_cast<decltype(&Screen.HeightHUD)*>(pattern.count_hint(1).get(0).get<uint32_t>(63))} = &Screen.HeightHUD;

	pattern = hook::pattern("DB 05 ? ? ? ? A1 ? ? ? ? 83 C4 08 D9 54 24 ? BF ? ? ? ? D8 8C 24 ? ? ? ? 8D 74 24 ?"); // 0x526297

	injector::MakeInline<UIHook>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(6));
	ProtectedGameRef<decltype(&Screen.HeightHUD)>{reinterpret_cast<decltype(&Screen.HeightHUD)*>(pattern.count_hint(1).get(0).get<uint32_t>(49))} = &Screen.HeightHUD;

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

	injector::MakeNOP(pattern.count_hint(1).get(0).get<uint32_t>(0), 4, true);
	injector::MakeInline<LevelUpPos1>(pattern.count_hint(1).get(0).get<uint32_t>(4), pattern.count_hint(1).get(0).get<uint32_t>(19));

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

	injector::MakeInline<LevelUpPos2>(pattern.count_hint(1).get(0).get<uint32_t>(40), pattern.count_hint(1).get(0).get<uint32_t>(46));

	// unprotect and set scaled X res divider for Advertise
	uintptr_t loc_456D03 = reinterpret_cast<uintptr_t>(hook::pattern("D8 0D ? ? ? ? D9 5B 3C DB 05 ? ? ? ? BF 01 00 00 00").get_first(0));
	ptrOneDiv640 = *reinterpret_cast<uintptr_t*>(loc_456D03 + 2);

	injector::UnprotectMemory(ptrOneDiv640, sizeof(float), dummyoldprotect);
	ProtectedGameRef<float>{reinterpret_cast<float*>(ptrOneDiv640)} = 1.0f / (480.0f * Screen.fAspectRatio);

	if (bShadowFix)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "D9 54 24 3C D9 54 24 40 D9 54 24 58", 1 },
        }))
        {

            uintptr_t loc_63B143 = reinterpret_cast<uintptr_t>(hook::pattern("D9 54 24 3C D9 54 24 40 D9 54 24 58").get_first(0));
            static uintptr_t sub_64CA10 = static_cast<uintptr_t>(injector::GetBranchDestination(loc_63B143 + 0x30));

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
		
                    reinterpret_cast<void(__cdecl*)(uint32_t, uint32_t, uint32_t)>(sub_64CA10)(4, regs.edx, 4);
                    _asm fld [val]
                }
            }; injector::MakeInline<ShadowFix2>(loc_63B143, loc_63B143 + 0x39);
        }
	}

	if (!szCustomUserFilesDirectoryInGameDir.empty())
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "? ? ? 6A 1A ? FF 15", 1 },
            { "50 6A 00 6A 00 6A 1A 6A 00 FF 15", 1 },
            { "56 6A 00 6A 00 6A 1A 6A 00 FF 15", 1 },
        }))
        {

            CustomUserDir = GetExeModulePath<std::filesystem::path>();
            CustomUserDir.append(szCustomUserFilesDirectoryInGameDir);

            auto SHGetFolderPathAHook = [](HWND /*hwnd*/, int /*csidl*/, HANDLE /*hToken*/, DWORD /*dwFlags*/, LPSTR pszPath) -> HRESULT
            {
                const auto path = CustomUserDir.u8string();
                if (path.size() >= MAX_PATH)
                    return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                CreateDirectoryW((LPCWSTR)(CustomUserDir.u16string().c_str()), NULL);
                memcpy(pszPath, path.data(), path.size() + 1);

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
		
            ProtectedGameRef<uint8_t>{reinterpret_cast<uint8_t*>(str_7468D0 + 2)} = 0;

            injector::MakeCALL(loc_62DFAC, static_cast<HRESULT(WINAPI*)(LPSTR, LPSTR)>(PathAppendAHook), true);
            injector::MakeNOP(loc_62DFAC + 5, 1, true);
        }
	}

	// set Screen_Full & override game config
	pattern = hook::pattern("8B 15 ? ? ? ? 8D 4C 24 10 51 68 ? ? ? ? 50 FF 92 30 01 00 00 8B 4C 24 1C 33 C0 83 C4 0C 3B CD 0F 95 C0 A3 ? ? ? ? E9").count_hint(9);
	uintptr_t loc_629F8E = reinterpret_cast<uintptr_t>(pattern.get(3).get<uint32_t>(0)) + 0x25;
	uintptr_t Screen_Full = *reinterpret_cast<uintptr_t*>(loc_629F8E + 1);

	ProtectedGameRef<uint32_t>{ reinterpret_cast<uint32_t*>(Screen_Full) } = nWindowedMode == 0;
	injector::MakeNOP(loc_629F8E, 5);	

	if ((nWindowedMode == 4) || (nWindowedMode == 5) || !nWindowedMode)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "C6 43 08 00 8B 0D ? ? ? ? 51 FF 15", 1 },
        }))
        {

            // hook GetClientRect via IAT
            uintptr_t loc_65E783 = reinterpret_cast<uintptr_t>(hook::pattern("C6 43 08 00 8B 0D ? ? ? ? 51 FF 15").get_first(0)) + 0xB;
            uintptr_t GetClientRectIAT = *reinterpret_cast<uintptr_t*>(loc_65E783 + 2);
            ProtectedGameRef<decltype(&GetClientRectHook)>{reinterpret_cast<decltype(&GetClientRectHook)*>(GetClientRectIAT)} = &GetClientRectHook;
        }
	}

	if (nWindowedMode)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "68 00 00 C8 00 6A F0 56 FF 15", 1 },
            { "68 00 00 C8 00 8D 4C 24 08 51 C7 44 24 10 00 00 00 00", 1 },
            { "C7 44 24 14 ? ? ? ? 89 74 24 18 89 74 24 1C", 1 },
            { "8B 54 24 20 A1 ? ? ? ? 3B D0 75 12 8B 44 24 24", 1 },
            { "85 C0 7E 12 48 85 C0 A3", 1 },
            { "68 AB AA AA 3F 68 00 00 00 3F 8D 44 24 28 E8 ? ? ? ? 83", 1 },
        }))
        {

            uintptr_t loc_446D87 = reinterpret_cast<uintptr_t>(hook::pattern("68 00 00 C8 00 6A F0 56 FF 15").get_first(0));
            uintptr_t loc_446E11 = loc_446D87 + 0x8A;
            uintptr_t loc_446DD7 = loc_446D87 + 0x50;
            uintptr_t loc_446EA8 = loc_446D87 + 0x121;
            uintptr_t loc_4462B5 = reinterpret_cast<uintptr_t>(hook::pattern("68 00 00 C8 00 8D 4C 24 08 51 C7 44 24 10 00 00 00 00").get_first(0)) + 0x22;
            uintptr_t loc_4462F8 = loc_4462B5 + 0x43;

            injector::MakeJMP(loc_446D87, loc_446D87 + 0x1E, true);
            injector::MakeJMP(loc_446E11, loc_446E11 + 0x13, true);

            injector::MakeNOP(loc_446DD7, 6, true);
            injector::MakeCALL(loc_446DD7, WindowedModeWrapper::AdjustWindowRect_Hook, true);

            injector::MakeNOP(loc_4462B5, 6, true);
            injector::MakeCALL(loc_4462B5, WindowedModeWrapper::AdjustWindowRect_Hook, true);

            injector::MakeNOP(loc_4462F8, 6, true);
            injector::MakeCALL(loc_4462F8, WindowedModeWrapper::CreateWindowExA_Hook, true);

            // disable cursor centering on boot
            injector::MakeNOP(loc_446EA8, 5, true);

            // dereference the current WndProc from the game executable and write to the function pointer (to maximize compatibility)
            uint32_t* wndproc_addr = hook::pattern("C7 44 24 14 ? ? ? ? 89 74 24 18 89 74 24 1C").count_hint(1).get(0).get<uint32_t>(4);
            GameWndProcAddr = *(uint32_t*)wndproc_addr;
            GameWndProc = (LRESULT(WINAPI*)(HWND, UINT, WPARAM, LPARAM))GameWndProcAddr;
            ProtectedGameRef<uint32_t>{reinterpret_cast<uint32_t*>(wndproc_addr)} = (uint32_t)&WSFixWndProc;

            uintptr_t loc_6567B6 = reinterpret_cast<uintptr_t>(hook::pattern("8B 54 24 20 A1 ? ? ? ? 3B D0 75 12 8B 44 24 24").get_first(0));
            loc_6567BF = loc_6567B6 + 9;
            ptrWindowSizeX = *reinterpret_cast<uintptr_t*>(loc_6567B6 + 5);

            uintptr_t loc_445B32 = reinterpret_cast<uintptr_t>(hook::pattern("85 C0 7E 12 48 85 C0 A3").get_first(0)) + 0xE;
            uintptr_t loc_446EBF = reinterpret_cast<uintptr_t>(hook::pattern("68 AB AA AA 3F 68 00 00 00 3F 8D 44 24 28 E8 ? ? ? ? 83").get_first(0)) + 0x75;

            switch (nWindowedMode)
            {
            case 5:
                WindowedModeWrapper::bStretchWindow = true;
                injector::MakeJMP(loc_6567B6, StretchOnBoot, true);
                break;
            case 4:
                WindowedModeWrapper::bScaleWindow = true;
                injector::MakeJMP(loc_6567B6, StretchOnBoot, true);
                break;
            case 3:
                WindowedModeWrapper::bEnableWindowResize = true;
                injector::MakeJMP(loc_445B32, loc_445B32 + 8);
                injector::MakeJMP(loc_446EBF, loc_446EBF + 7);
            case 2:
                WindowedModeWrapper::bBorderlessWindowed = false;
                break;
            default:
                break;
            }
        }
	}

	if (bDisableMouseInput)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "8D 54 24 14 52 6A 02 C7 44 24 1C 14 00 00 00 C7 44 24 20 10", 1 },
        }))
        {

            uintptr_t loc_444E32 = reinterpret_cast<uintptr_t>(hook::pattern("8D 54 24 14 52 6A 02 C7 44 24 1C 14 00 00 00 C7 44 24 20 10").get_first(0)) - 0x8A;
            uintptr_t ptr_722FA0 = *reinterpret_cast<uintptr_t*>(loc_444E32 + 1);

            // corrupt GUID_SysMouse on purpose
            ProtectedGameRef<uint32_t>{reinterpret_cast<uint32_t*>(ptr_722FA0)} = 0;
        }
	}

	if (WarpSwitchEffectMode)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "83 7F 50 04 75 0D 6A 02 6A 0A E8 ? ? ? ? 6A 06 EB 0B 6A 05 6A 0A E8 ? ? ? ? 6A 02 6A 0B", 1 },
        }))
        {

            uintptr_t loc_41BAD5 = reinterpret_cast<uintptr_t>(hook::pattern("83 7F 50 04 75 0D 6A 02 6A 0A E8 ? ? ? ? 6A 06 EB 0B 6A 05 6A 0A E8 ? ? ? ? 6A 02 6A 0B").get_first(0)) + 0x10;

            if (WarpSwitchEffectMode > 2)
                WarpSwitchEffectMode = 2;
		
            ProtectedGameRef<uint8_t>{reinterpret_cast<uint8_t*>(loc_41BAD5)} = WarpSwitchEffectMode;
        }
	}

	if (bDisableFrameSkipping)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "8A 48 1E 84 C9 74 24 A1", 1 },
        }))
        {

            uintptr_t loc_402CF5 = reinterpret_cast<uintptr_t>(hook::pattern("8A 48 1E 84 C9 74 24 A1").get_first(0));
            injector::MakeJMP(loc_402CF5, loc_402CF5 + 0x2B);
        }
	}

	// does not exist in demo!
	if (bRestoreDemos && !bDemoVersion)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "C7 05 ? ? ? ? 06 00 00 00 EB 14 C7 05 ? ? ? ? 03 00 00 00 C7 05 ? ? ? ? 0A 00 00 00", 1 },
        }))
        {

            uintptr_t loc_456989 = reinterpret_cast<uintptr_t>(hook::pattern("C7 05 ? ? ? ? 06 00 00 00 EB 14 C7 05 ? ? ? ? 03 00 00 00 C7 05 ? ? ? ? 0A 00 00 00").get_first(0)) + 0x23;
            DemoRestoreExit1 = loc_456989 + 7;
            injector::MakeJMP(loc_456989, RestoreDemos, true);
        }
	}

	if (bSkipFE)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "8B 46 04 53 55 33 ED 3B C5 57 89 2E BF 01 00 00 00 75 16", 1 },
        }))
        {

            uintptr_t loc_42713E = reinterpret_cast<uintptr_t>(hook::pattern("8B 46 04 53 55 33 ED 3B C5 57 89 2E BF 01 00 00 00 75 16").get_first(0)) + 0x1E;
            static uintptr_t sub_42A9F0 = static_cast<uintptr_t>(injector::GetBranchDestination(loc_42713E + 3));

            struct SysModeHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint32_t*)(regs.eax + 0x38) = SysMode;
                    reinterpret_cast<void(*)()>(sub_42A9F0)();
                    *(uint32_t*)(regs.esi + 4) = regs.edi;
                }
            }; injector::MakeInline<SysModeHook>(loc_42713E, loc_42713E + 0xB);
        }
	}

	// does not exist in demo!
	if (bDisableCDCheck && !bDemoVersion)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "55 8B EC 83 E4 F8 83 EC 50 53 55 56 57 68 00 00 40 00 6A 00 6A 00", 1 },
        }))
        {

            uintptr_t loc_629B72 = reinterpret_cast<uintptr_t>(hook::pattern("55 8B EC 83 E4 F8 83 EC 50 53 55 56 57 68 00 00 40 00 6A 00 6A 00").get_first(0)) + 0x42;
            injector::MakeJMP(loc_629B72, loc_629B72 + 0xC4, true);
        }
	}

	if (ShadowRes != 256)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "83 EC 10 85 F6 74 76 85 FF 74 72 68 00 01 00 00", 1 },
            { "C7 44 24 40 00 00 80 43", 1 },
            { "68 00 00 80 43 56", 1 },
        }))
        {

            uintptr_t loc_63BF6B = reinterpret_cast<uintptr_t>(hook::pattern("83 EC 10 85 F6 74 76 85 FF 74 72 68 00 01 00 00").get_first(0)) + 0xB;
            uintptr_t loc_63BF8F = loc_63BF6B + 0x24;

            uintptr_t loc_63B077 = reinterpret_cast<uintptr_t>(hook::pattern("C7 44 24 40 00 00 80 43").get_first(0));
            uintptr_t loc_63B07F = loc_63B077 + 8;
            uintptr_t loc_63B08F = loc_63B077 + 0x18;
            uintptr_t loc_63B097 = loc_63B077 + 0x20;

            ProtectedGameRef<uint32_t>{reinterpret_cast<uint32_t*>(loc_63BF6B + 1)} = ShadowRes;
            ProtectedGameRef<uint32_t>{reinterpret_cast<uint32_t*>(loc_63BF8F + 1)} = ShadowRes - 4;
            ProtectedGameRef<uint32_t>{reinterpret_cast<uint32_t*>(ShadowStuffAddr + 8)} = ShadowRes;

            ProtectedGameRef<float>{reinterpret_cast<float*>(loc_63B077 + 4)} = (float)ShadowRes;
            ProtectedGameRef<float>{reinterpret_cast<float*>(loc_63B07F + 4)} = (float)ShadowRes;
            ProtectedGameRef<float>{reinterpret_cast<float*>(loc_63B08F + 4)} = (float)ShadowRes;
            ProtectedGameRef<float>{reinterpret_cast<float*>(loc_63B097 + 4)} = (float)ShadowRes;

            uintptr_t loc_6B7237 = reinterpret_cast<uintptr_t>(hook::pattern("68 00 00 80 43 56").get_first(0));
            ProtectedGameRef<float>{reinterpret_cast<float*>(loc_6B7237 + 1)} = (float)ShadowRes;
        }
	}

	if (bDisableQuitDialog)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "85 C0 0F 84 ? ? ? ? 83 7C 24 34 12 0F 85 ? ? ? ?", 1 },
        }))
        {

            uintptr_t loc_446EFB = reinterpret_cast<uintptr_t>(hook::pattern("85 C0 0F 84 ? ? ? ? 83 7C 24 34 12 0F 85 ? ? ? ?").get_first(0)) + 0x13;
            injector::MakeJMP(loc_446EFB, loc_446EFB + 0x100);
        }
	}

	// custom clip range
	uintptr_t loc_4EE3CA = reinterpret_cast<uintptr_t>(hook::pattern("B9 00 01 00 00 33 C0 8B FE F3 AB A1").get_first(0)) + 0x16;
	uintptr_t ptr_7869B4 = *reinterpret_cast<uintptr_t*>(loc_4EE3CA + 3) + 0xC;
	ProtectedGameRef<float>{reinterpret_cast<float*>(ptr_7869B4)} = ClipRange;

	static float fInv640 = 1.0f / 640.0f;
	static int OrigWidth = 640;
	static int OrigHeight = 480;
	static int Width43_480 = static_cast<int>(480.0f * Screen.fAspectRatio);

	// 2P fix -- ignore aspect change for screen texture -- TODO: dynamic resizing of screen texture
	uintptr_t loc_61D533 = reinterpret_cast<uintptr_t>(hook::pattern("83 C4 20 83 F9 02").get_first(0)) + 6;
	ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_61D533 + 2)} = &fInv640;

	// ignore aspect change for special stage gauge
	uintptr_t loc_5262B9 = reinterpret_cast<uintptr_t>(hook::pattern("83 EC 78 85 C0 75 04 83 C4 78 C3 8B 00").get_first(0)) + 0x39;
	uintptr_t loc_5262F9 = loc_5262B9 + 0x40;
	uintptr_t loc_52640A = reinterpret_cast<uintptr_t>(hook::pattern("81 EC 98 00 00 00 85 C0 75 07 81 C4 98 00 00 00 C3 8B 00 53").get_first(0)) + 0x4A;
	uintptr_t loc_526459 = loc_52640A + 0x4F;

	ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_5262B9 + 2)} = &fInv640;
	ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_5262F9 + 2)} = &fInv640;
	ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_52640A + 2)} = &fInv640;
	ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_526459 + 2)} = &fInv640;

	// ignore aspect change for special stage link counter
	uintptr_t loc_526F9C = reinterpret_cast<uintptr_t>(hook::pattern("81 EC A0 00 00 00 53 33 DB 3B C3 75 0C 33 C0 5B 81 C4 A0 00 00 00").get_first(0)) + 0x3C;
	uintptr_t loc_526FEA = loc_526F9C + 0x4E;

	ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_526F9C + 2)} = &fInv640;
	ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_526FEA + 2)} = &fInv640;

	// ignore aspect change for results screen
	uintptr_t loc_458961 = reinterpret_cast<uintptr_t>(hook::pattern("8A 51 08 81 EC 98 00 00 00 84 D2").get_first(0)) + 0x3B;
	uintptr_t loc_458993 = loc_458961 + 0x32;

	ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_458961 + 2)} = &fInv640;
	ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_458993 + 2)} = &fInv640;

	if (bFixAdvertiseWindows)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "66 C7 43 1E 48 08", 1 },
        }))
        {

            // ignore aspect change for Advertise windows
            // position
            uintptr_t loc_456CFA = reinterpret_cast<uintptr_t>(hook::pattern("66 C7 43 1E 48 08").get_first(0)) + 0x11;
            uintptr_t loc_456D0C = loc_456CFA + 0x12;
            uintptr_t loc_456D03 = loc_456CFA + 9;

            uintptr_t loc_456D2B = loc_456CFA + 0x31;
            uintptr_t loc_456D3D = loc_456CFA + 0x43;
            uintptr_t loc_456D34 = loc_456CFA + 0x3A;

            ProtectedGameRef<int*>{reinterpret_cast<int**>(loc_456CFA + 2)} = &OrigWidth;
            ProtectedGameRef<int*>{reinterpret_cast<int**>(loc_456D0C + 2)} = &OrigHeight;
            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_456D03 + 2)} = &fInv640;
            // size
            ProtectedGameRef<int*>{reinterpret_cast<int**>(loc_456D2B + 2)} = &OrigWidth;
            ProtectedGameRef<int*>{reinterpret_cast<int**>(loc_456D3D + 2)} = &OrigHeight;
            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_456D34 + 2)} = &fInv640;
        }
	}

	if (bFixStaffRoll)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "66 C7 46 1E A4 00", 1 },
        }))
        {

            uintptr_t loc_4543FF = reinterpret_cast<uintptr_t>(hook::pattern("66 C7 46 1E A4 00").get_first(0)) + 0x28;
            uintptr_t loc_454407 = loc_4543FF + 8;

            // Advertise staff roll
            ProtectedGameRef<int*>{reinterpret_cast<int**>(loc_4543FF + 2)} = &OrigWidth;
            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_454407 + 2)} = &fInv640;
        }
	}

	// ignore aspect change for power up icons
	uintptr_t loc_479F34 = reinterpret_cast<uintptr_t>(hook::pattern("8B 40 60 DB 40 0C 8B 54 24 10 8B 44 24").get_first(0)) + 0x11;
	ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_479F34 + 2)} = &fInv640;

	// fix window icon
	uintptr_t loc_446229 = reinterpret_cast<uintptr_t>(hook::pattern("68 00 7F 00 00 56 C7 44 24 0C 30 00 00 00").get_first(0)) + 0x53;

	injector::MakeNOP(loc_446229, 6);
	injector::MakeCALL(loc_446229, RegisterClassHook);

	if (bFixAdvertiseWindows)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "D9 41 04 D8 0D ? ? ? ? D9 5C 24 10 D9 01", 1 },
            { "C7 44 24 20 00 00 E0 42", 1 },
            { "C7 44 24 18 00 00 A0 42 E8 ? ? ? ? 8B 15", 1 },
            { "E8 ? ? ? ? E8 ? ? ? ? D9 44 24 20 D8 05 ? ? ? ? 8B 46 70", 1 },
            { "66 C7 43 1E 48 08", 1 },
            { "8D 44 24 10 50 68 00 00 80 3F 68 00 00 80 3F 83 EC 08", 1 },
        }))
        {

            float scalar = (Screen.fHeight / 480.0f);
            if (Screen.Width < Screen.Height)
            {
                scalar = Screen.fWidth / 480.0f;
            }

            WindowBezelSize = 1.0f / (32.0f * scalar);
            uintptr_t loc_45764D = reinterpret_cast<uintptr_t>(hook::pattern("D9 41 04 D8 0D ? ? ? ? D9 5C 24 10 D9 01").get_first(0)) + 0xF;
            uintptr_t loc_457658 = loc_45764D + 0xB;

            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_45764D + 2)} = &WindowBezelSize;
            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_457658 + 2)} = &WindowBezelSize;

            uintptr_t loc_4571A0 = reinterpret_cast<uintptr_t>(hook::pattern("C7 44 24 20 00 00 E0 42").get_first(0));
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
            }; injector::MakeInline<AdvWindowHook1>(loc_4571A0, loc_4571A0 + 8);

            uintptr_t loc_4571C4 = reinterpret_cast<uintptr_t>(hook::pattern("C7 44 24 18 00 00 A0 42 E8 ? ? ? ? 8B 15").get_first(0));
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
            }; injector::MakeInline<AdvWindowHook2>(loc_4571C4, loc_4571C4 + 8);


            uintptr_t loc_457278 = reinterpret_cast<uintptr_t>(hook::pattern("E8 ? ? ? ? E8 ? ? ? ? D9 44 24 20 D8 05 ? ? ? ? 8B 46 70").get_first(0));
            uintptr_t loc_457357 = loc_457278 + 0xDF;
            uintptr_t loc_45743C = loc_457278 + 0x1C4;

            uintptr_t loc_45720A = loc_457278 - 0x6E;
            uintptr_t loc_457230 = loc_457278 - 0x48;
            uintptr_t loc_45730F = loc_457278 + 0x97;
            uintptr_t loc_4573F4 = loc_457278 + 0x17C;

            uintptr_t loc_457286 = loc_457278 + 0xE;
            uintptr_t loc_457365 = loc_457278 + 0xED;
            uintptr_t loc_45744A = loc_457278 + 0x1D2;

            uintptr_t loc_457372 = loc_457278 + 0xFA;
            uintptr_t loc_457457 = loc_457278 + 0x1DF;

            uintptr_t loc_45731F = loc_457278 + 0xA7;
            uintptr_t loc_457404 = loc_457278 + 0x18C;

            injector::MakeCALL(loc_457278, AdvertiseWindowFix::AdvButtonDrawHook);
            injector::MakeCALL(loc_457357, AdvertiseWindowFix::AdvButtonDrawHook);
            injector::MakeCALL(loc_45743C, AdvertiseWindowFix::AdvButtonDrawHook);

            ButtonBezelOffset = 8.0f * scalar;
            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_45720A + 2)} = &ButtonBezelOffset;
            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_457230 + 2)} = &ButtonBezelOffset;
            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_45730F + 2)} = &ButtonBezelOffset;
            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_4573F4 + 2)} = &ButtonBezelOffset;

            AdvWindowButtonTextYOffset = 16.0f * scalar;
            AdvWindowButton2TextYOffset = 8.0f * scalar;
            AdvWindowButton2Offset = 40.0f * scalar;
            AdvWindowButton3Offset = 72.0f * scalar;

            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_457286 + 2)} = &AdvWindowButtonTextYOffset;
            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_457365 + 2)} = &AdvWindowButtonTextYOffset;
            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_45744A + 2)} = &AdvWindowButtonTextYOffset;

            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_457372 + 2)} = &AdvWindowButton2TextYOffset;
            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_457457 + 2)} = &AdvWindowButton2TextYOffset;

            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_45731F + 2)} = &AdvWindowButton2Offset;
            ProtectedGameRef<float*>{reinterpret_cast<float**>(loc_457404 + 2)} = &AdvWindowButton3Offset;

            uintptr_t loc_456CC1 = reinterpret_cast<uintptr_t>(hook::pattern("66 C7 43 1E 48 08").get_first(0)) - 0x28;
            uintptr_t ptrAdvWindowVTable = *reinterpret_cast<uintptr_t*>(loc_456CC1 + 2);

            AdvertiseWindowFix::ptrAdvWindowDrawFunc = *reinterpret_cast<uintptr_t*>(ptrAdvWindowVTable + 8);
            ProtectedGameRef<uintptr_t>{reinterpret_cast<uintptr_t*>(ptrAdvWindowVTable + 8)} = (uintptr_t)&AdvertiseWindowFix::AdvWindowDrawHook;

            uintptr_t loc_45729C = loc_457278 + 0x24;
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
            }; injector::MakeInline<AdvButtonTextHook1>(loc_45729C, loc_45729C + 0x10);

            uintptr_t loc_45738C = loc_457278 + 0x114;
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
            }; injector::MakeInline<AdvButtonTextHook2>(loc_45738C, loc_45738C + 0xA);

            uintptr_t loc_457471 = loc_457278 + 0x1F9;
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
            }; injector::MakeInline<AdvButtonTextHook3>(loc_457471, loc_457471 + 0xA);

            uintptr_t loc_457F1B = reinterpret_cast<uintptr_t>(hook::pattern("8D 44 24 10 50 68 00 00 80 3F 68 00 00 80 3F 83 EC 08").get_first(0)) + 0x2B;
            uintptr_t loc_457F3C = loc_457F1B + 0x21;
            TextDrawFunc2Addr = static_cast<uintptr_t>(injector::GetBranchDestination(loc_457F1B));

            injector::MakeCALL(loc_457F1B, TextDrawFunc2Hook);
            injector::MakeCALL(loc_457F3C, TextDrawFunc2Hook);

            uintptr_t loc_456CC7 = loc_456CC1 + 6;
            AdvertiseWindowFix::sub_456AA0 = static_cast<uintptr_t>(injector::GetBranchDestination(loc_456CC7));
            injector::MakeCALL(loc_456CC7, AdvertiseWindowFix::hkAdvWindowConstructor);

            AdvertiseWindowFix::ptrAdvWindowDestructorFunc = *reinterpret_cast<uintptr_t*>(ptrAdvWindowVTable);
            ProtectedGameRef<uintptr_t>{reinterpret_cast<uintptr_t*>(ptrAdvWindowVTable)} = (uintptr_t)&AdvertiseWindowFix::AdvWindowDestructorHook;
        }
	}
	else
	{
		// old Advertise window fix
		pattern = hook::pattern("D9 C9 C1 E0 08 D9 5C ? ? 0B C1 C1 E0 08 D9 5C ? ? 0B C2 8D 4C ? ? BA"); //0x4578A3

		struct WindowTextPos
		{
			void operator()(injector::reg_pack& regs)
			{
				_asm
				{
					fxch st(1)
				}
				Screen.AspectRatioAffected = true;
				regs.eax <<= 8;
			}
		};

		injector::MakeInline<WindowTextPos>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(5));

		pattern = hook::pattern("C1 E0 08 0B C2 D9 54 ? ? C1 E0 08 D9 41 ? 0F B6 D3 D8 05 ? ? ? ? 0B C2 8B 54"); //0x4583FB

		struct WindowTextPos2
		{
			void operator()(injector::reg_pack& regs)
			{
				Screen.AspectRatioAffected = true;
				regs.eax <<= 8;
				regs.eax |= regs.edx;
			}
		};

		injector::MakeInline<WindowTextPos2>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(5));

		pattern = hook::pattern("D9 00 8B 39 D8 02 8B 50 04 89 54 ? ? 8B 50 08 89 7C ? ? D9 54"); //0x4574F9
		struct WindowPos
		{
			void operator()(injector::reg_pack& regs)
			{
				Screen.AspectRatioAffected = true;
				float Temp = *(float*)(regs.eax) + *(float*)(regs.edx);
				regs.edi = *(int*)(regs.ecx);
				_asm
				{
					fld dword ptr[Temp]
				}
			}
		};

		injector::MakeInline<WindowPos>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(6));
	}

	// Staff roll
	if (bFixStaffRoll)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "8B 56 0C 8B 46 08 56 68 00 00 80 3F 68 00 00 80 3F 52 50 8B 46 14", 1 },
            { "66 C7 46 1E A4 00", 1 },
            { "68 00 00 00 43 EB 05 68 00 00 80 43 68 00 00 80 43", 1 },
        }))
        {

            uintptr_t loc_454744 = reinterpret_cast<uintptr_t>(hook::pattern("8B 56 0C 8B 46 08 56 68 00 00 80 3F 68 00 00 80 3F 52 50 8B 46 14").get_first(0)) + 0x1B;
            TextDrawFunc2Addr = static_cast<uintptr_t>(injector::GetBranchDestination(loc_454744));

            uintptr_t loc_4543D1 = reinterpret_cast<uintptr_t>(hook::pattern("66 C7 46 1E A4 00").get_first(0)) - 6;
            uintptr_t loc_45448B = loc_4543D1 + 0xBA;
            uintptr_t ptrAdvStaffRollVTable = *reinterpret_cast<uintptr_t*>(loc_4543D1 + 2);

            uintptr_t loc_45475C = reinterpret_cast<uintptr_t>(hook::pattern("68 00 00 00 43 EB 05 68 00 00 80 43 68 00 00 80 43").get_first(0)) + 0x11;
            AdvStaffRollFix::AdvStaffrollLogoFuncAddr = static_cast<uintptr_t>(injector::GetBranchDestination(loc_45475C));

            AdvStaffRollFix::ptrAdvStaffrollDrawFunc = *reinterpret_cast<uintptr_t*>(ptrAdvStaffRollVTable + 8);
            AdvStaffRollFix::ptrAdvStaffrollDestructorFunc = *reinterpret_cast<uintptr_t*>(ptrAdvStaffRollVTable);

            injector::MakeCALL(loc_454744, TextDrawFunc2Hook);
            ProtectedGameRef<uintptr_t>{reinterpret_cast<uintptr_t*>(ptrAdvStaffRollVTable + 8)} = (uintptr_t)&AdvStaffRollFix::AdvStaffrollDrawHook;
            injector::MakeCALL(loc_45448B, AdvStaffRollFix::hkStaffrollConstructor);
            ProtectedGameRef<uintptr_t>{reinterpret_cast<uintptr_t*>(ptrAdvStaffRollVTable)} = (uintptr_t)&AdvStaffRollFix::AdvStaffrollDestructorHook;
            injector::MakeCALL(loc_45475C, AdvStaffRollFix::AdvStaffrollLogoHook);
        }
	}

	if (bDisableLoadingTimer)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "DC 05 ? ? ? ? DC 1D ? ? ? ? DF E0 F6 C4 05", 1 },
        }))
        {

            uintptr_t loc_419734 = reinterpret_cast<uintptr_t>(hook::pattern("DC 05 ? ? ? ? DC 1D ? ? ? ? DF E0 F6 C4 05").get_first(0)) - 0x44;
            injector::MakeJMP(loc_419734, loc_419734 + 0x188);
        }
	}
	if (bDisableSubtitles)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "D8 80 50 6B 7C 00 D9 5C 24 30", 1 },
        }))
        {

            uintptr_t loc_428560 = reinterpret_cast<uintptr_t>(hook::pattern("D8 80 50 6B 7C 00 D9 5C 24 30").get_first(0)) - 0x22D;
            injector::MakeRET(loc_428560, 0x1C);
        }
	}

	if (bIncreaseObjectDistance)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "BF 00 08 00 00 BB 0F 00 00 00 EB 06 8D 9B 00 00 00 00", 1 },
        }))
        {

            if (fObjDistanceScale < 0.0f)
                fObjDistanceScale = 0.0f;

            if (fObjDistanceScale > 255.0f)
                fObjDistanceScale = 255.0f;

            uintptr_t loc_43DF60 = reinterpret_cast<uintptr_t>(hook::pattern("BF 00 08 00 00 BB 0F 00 00 00 EB 06 8D 9B 00 00 00 00").get_first(0)) + 0x12;
            static uintptr_t Clip_Range = *reinterpret_cast<uintptr_t*>(loc_43DF60 + 1);
            struct ObjDrawDistanceHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    float fDistance = static_cast<float>(*(uint8_t*)(regs.ecx));
                    uint32_t iDistance;
                    fDistance *= fObjDistanceScale;

                    iDistance = static_cast<uint32_t>(fDistance);
                    if (iDistance > 255)
                        iDistance = 255;

                    if (iDistance < MinObjDistance)
                        iDistance = MinObjDistance;

                    *(uint8_t*)(regs.ecx) = iDistance & 0xFF;
                    regs.eax = *(uint32_t*)Clip_Range;
                    regs.ebp = 1;
                }
            }; injector::MakeInline<ObjDrawDistanceHook>(loc_43DF60, loc_43DF60 + 0xA);
        }
	}

	if (bBetterFrameSync)
	{
        if (wfp::PatchPreflight("Sonic Heroes optional patch: signatures unavailable; feature disabled.\n",
        {
            { "68 40 42 0F 00 51 50 E8 ? ? ? ? 8B 0D", 1 },
            { "C7 40 3C 05 00 00 00 5D C7 05 ? ? ? ? FF FF FF FF", 1 },
            { "83 C4 08 E8 ? ? ? ? E8 ? ? ? ? FF 0D ? ? ? ? E8", 1 },
        }))
        {

            BetterSync::FrameLimiter::Init(BetterSync::mFPSLimitMode);

            BetterSync::sub_6C4FC0 = reinterpret_cast<uintptr_t>(hook::pattern("68 40 42 0F 00 51 50 E8 ? ? ? ? 8B 0D").get_first(0)) - 0xE2;
            BetterSync::MovieIDPtr = *(uintptr_t*)(reinterpret_cast<uintptr_t>(hook::pattern("C7 40 3C 05 00 00 00 5D C7 05 ? ? ? ? FF FF FF FF").get_first(0)) + 0xA);
            uintptr_t loc_443169 = reinterpret_cast<uintptr_t>(hook::pattern("83 C4 08 E8 ? ? ? ? E8 ? ? ? ? FF 0D ? ? ? ? E8").get_first(0)) + 0x13;
		
            injector::MakeCALL(loc_443169, BetterSync::CustomSyncFunc);
        }
	}
}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
	std::call_once(CallbackHandler::flag, []()
		{
			CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("0F BE 0D ? ? ? ? 8D 0C 89 8B"));
		});
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		if (!IsUALPresent()) { InitializeASI(); }
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		WindowCursor::Shutdown();
	}
	return TRUE;
}
