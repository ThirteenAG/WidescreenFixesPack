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
	float fAspectRatio;
	float fHudScale;
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
	Screen.fHudScale = ((1.0f / Screen.fAspectRatio) * (4.0f / 3.0f));

	pattern = hook::pattern("0F BE 0D ? ? ? ? 8D 0C 89 8B"); //446B2A
	struct ResHook
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.edx = Screen.Width;
			regs.ecx = Screen.Height;
		}
	}; injector::MakeInline<ResHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(27));

	//pattern = hook::pattern("68 AB AA AA 3F");
	//for (size_t i = 0; i < pattern.size(); ++i)
	//{
	//	injector::WriteMemory<float>(pattern.get(i).get<uint32_t>(1), Screen.fAspectRatio, true);
	//}
	
	pattern = hook::pattern("D9 05 ? ? ? ? 89 4E 68 8B 50 04 D8 76 68"); //0x64AC8B
	injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &Screen.fHudScale, true);
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