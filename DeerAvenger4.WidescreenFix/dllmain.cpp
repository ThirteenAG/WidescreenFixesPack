#include "..\includes\stdafx.h"
#include "..\includes\hooking\Hooking.Patterns.h"

HWND hWnd;
bool bDelay;

struct Screen
{
	int32_t Width;
	int32_t Height;
	float fWidth;
	float fHeight;
	float fAspectRatio;
} Screen;

DWORD WINAPI Init(LPVOID)
{
	auto pattern = hook::pattern("83 EC 58 53 56 57 89 65 E8");
	if (!(pattern.size() > 0) && !bDelay)
	{
		bDelay = true;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
		return 0;
	}

	if (bDelay)
	{
		while (!(pattern.size() > 0))
			pattern = hook::pattern("83 EC 58 53 56 57 89 65 E8");
	}

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

	pattern = hook::pattern("D9 05 ? ? ? ? D8 73 68 D9 05 ? ? ? ? D8 73 6C D9 C9"); //0x4460E7
	struct SetScaleHook
	{
		void operator()(injector::reg_pack& regs)
		{
			static float fScale = ((1.0f / Screen.fAspectRatio) * (4.0f / 3.0f));
			_asm fld    dword ptr[fScale]
		}
	}; injector::MakeInline<SetScaleHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(6));

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