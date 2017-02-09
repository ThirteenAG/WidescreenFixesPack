#include "stdafx.h"

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
	auto pattern = hook::pattern("0F BE 0D ? ? ? ? 8D 0C 89 8B");
	if (!(pattern.size() > 0) && !bDelay)
	{
		bDelay = true;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
		return 0;
	}

	if (bDelay)
	{
		while (!(pattern.size() > 0))
			pattern = hook::pattern("0F BE 0D ? ? ? ? 8D 0C 89 8B");
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

	injector::WriteMemory<uint8_t>(0x446B06, 0xEB, true);

	pattern = hook::pattern("A3 ? ? ? ? 8B 42 08 A3 ? ? ? ? 8B"); //655123
	static auto pResY = *pattern.get(0).get<uint32_t*>(1);
	struct ResHook2
	{
		void operator()(injector::reg_pack& regs)
		{
			*(pResY - 1) = Screen.Width;
			*(pResY - 0) = Screen.Height;
		}
	}; injector::MakeInline<ResHook2>(pattern.get(0).get<uint32_t>(0));
	
	pattern = hook::pattern("A3 ? ? ? ? A1 ? ? ? ? 68 ? ? ? ? 50 8B 08"); //6557A8
	struct ResHook2A
	{
		void operator()(injector::reg_pack& regs)
		{
			*(pResY - 1) = Screen.Width;
			*(pResY - 0) = Screen.Height;
		}
	}; injector::MakeInline<ResHook2A>(pattern.get(0).get<uint32_t>(0));
	
	pattern = hook::pattern("89 0D ? ? ? ? 8B 6C 24 7C 8B 75 60 80 7E 20 05"); //657047
	struct ResHook3
	{
		void operator()(injector::reg_pack& regs)
		{
			*(pResY - 1) = Screen.Width;
			*(pResY - 0) = Screen.Height;
		}
	}; injector::MakeInline<ResHook3>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(6));
	
	pattern = hook::pattern("89 15 ? ? ? ? 8B 51 08 89 15 ? ? ? ? 8B 49 0C"); //
	struct ResHook4
	{
		void operator()(injector::reg_pack& regs)
		{
			*(pResY - 1) = Screen.Width;
			*(pResY - 0) = Screen.Height;
		}
	}; injector::MakeInline<ResHook4>(pattern.get(2).get<uint32_t>(0), pattern.get(2).get<uint32_t>(6));
	
	pattern = hook::pattern("89 0D ? ? ? ? 5F 5E 5D B8 01 00 00 00"); //657CAF
	struct ResHook5
	{
		void operator()(injector::reg_pack& regs)
		{
			*(pResY - 1) = Screen.Width;
			*(pResY - 0) = Screen.Height;
		}
	}; injector::MakeInline<ResHook5>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(6));

	pattern = hook::pattern("8D B2 94 00 00 00 D9"); //0x64AFA5
	struct CutOffAreaHook
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.esi = regs.edx + 0x94;
			*(float*)(regs.edx + 0x68) /= Screen.fHudScale;
		}
	}; injector::MakeInline<CutOffAreaHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(6));
	
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