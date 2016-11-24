#include "..\includes\stdafx.h"
#include "..\includes\hooking\Hooking.Patterns.h"

HWND hWnd;
bool bDelay;

struct Screen
{
	int Width;
	int Height;
	float fWidth;
	float fHeight;
	float fFieldOfView;
	float fAspectRatio;
} Screen;

DWORD WINAPI Init(LPVOID)
{
	auto pattern = hook::pattern("BF 94 00 00 00 8B C7");
	if (!(pattern.size() > 0) && !bDelay)
	{
		bDelay = true;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
		return 0;
	}

	if (bDelay)
	{
		while (!(pattern.size() > 0))
			pattern = hook::pattern("BF 94 00 00 00 8B C7");
	}

	CIniReader iniReader("");
	Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
	Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
	Screen.fFieldOfView = iniReader.ReadFloat("MAIN", "FOVFactor", 0.5f);

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

	pattern = hook::pattern("A3 ? ? ? ? E8 ? ? ? ? 50 E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? 83 C4 44"); //0x40F2CD
	static auto pResY = *pattern.get(0).get<uint32_t*>(1);
	struct ResHook
	{
		void operator()(injector::reg_pack& regs)
		{
			*(pResY - 1) = Screen.Width;
			*(pResY - 0) = Screen.Height;
		}
	}; injector::MakeInline<ResHook>(pattern.get(0).get<uint32_t>(0));

	static float fARFactor = 0.25f * Screen.fAspectRatio;
	pattern = hook::pattern("D8 0D ? ? ? ? 51 DB 05 ? ? ? ? 8B");
	injector::WriteMemory(pattern.get(1).get<uint32_t>(2), &fARFactor); //0x43D26B

	pattern = hook::pattern("D8 0D ? ? ? ? 33 C9 D9 44 24 14 89 48 04 D8 64 24 10");
	injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &Screen.fFieldOfView); //0x5E4B08

	pattern = hook::pattern("68 ? ? ? ? 52 E8 ? ? ? ? 8D 44 24 60 83");
	injector::WriteMemory<float>(pattern.get(0).get<uint32_t>(1), Screen.fAspectRatio, true); //0x55BEB0
	pattern = hook::pattern("D8 0D ? ? ? ? D9 9C 24 88 00 00 00 E8");
	injector::WriteMemory<float>(*pattern.get(0).get<uint32_t*>(2), Screen.fAspectRatio, true); //0x71C714

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