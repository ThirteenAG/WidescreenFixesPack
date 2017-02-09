#include "stdafx.h"

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
	int Width43;
	float fWidth43;
	float fHudScale;
} Screen;


DWORD WINAPI Init(LPVOID)
{
	auto pattern = hook::pattern("BF 94 00 00 00 8B C7 E8");
	if (!(pattern.size() > 0) && !bDelay)
	{
		bDelay = true;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
		return 0;
	}

	if (bDelay)
	{
		while (!(pattern.size() > 0))
			pattern = hook::pattern("BF 94 00 00 00 8B C7 E8");
	}

	CIniReader iniReader("");
	Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
	Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);

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
	Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
	Screen.fWidth43 = static_cast<float>(Screen.Width43);

	pattern = hook::pattern("C7 00 ? ? ? ? C7 40 04 ? ? ? ? 88 58 08"); //45E939
	for (size_t i = 0; i < pattern.size(); ++i)
	{
		injector::WriteMemory(pattern.get(i).get<uint32_t>(2), Screen.Width, true);
		injector::WriteMemory(pattern.get(i).get<uint32_t>(9), Screen.Height, true);
	}

	pattern = hook::pattern("B9 ? ? ? ? 89 8E 28 03 00 00 B8 ? ? ? ? 89 86 2C 03 00 00"); //45EB3D
	injector::WriteMemory(pattern.get(0).get<uint32_t>(1), Screen.Width, true);
	injector::WriteMemory(pattern.get(0).get<uint32_t>(12), Screen.Height, true);

	//Scaling
	pattern = hook::pattern("F3 0F 59 15 ? ? ? ? F3 0F 5C D8 5F"); //41AA51
	injector::WriteMemory<float>(*pattern.get(0).get<float*>(4), Screen.fAspectRatio, true);

	pattern = hook::pattern("84 C0 F3 0F 10 05 ? ? ? ? 75 08 F3 0F 10 05"); //87E044
	injector::WriteMemory<float>(*pattern.get(0).get<float*>(6), Screen.fAspectRatio, true);

	//Text Scaling
	//static float fTextScale = 0.75f;
	//pattern = hook::pattern("F3 0F 10 05 ? ? ? ? 8B F8 C1 E7 1C C1 FF 1C F6 C4 10"); //8FEA7E
	//injector::WriteMemory(pattern.get(0).get<float*>(4), &fTextScale, true);
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