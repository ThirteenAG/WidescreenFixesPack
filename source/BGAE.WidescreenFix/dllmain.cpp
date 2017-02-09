#include "stdafx.h"

HWND hWnd;
bool bDelay;
bool bConfigTool;

struct Screen
{
	int Width;
	int Height;
	float fWidth;
	float fHeight;
	float fFieldOfView;
	float fAspectRatio;
	float fHudOffset;
	int Width43;
	float fWidth43;
} Screen;

uint8_t __stdcall GetInstallPath(void* _this)
{
	char path[MAX_PATH];
	GetModuleFileName(NULL, path, MAX_PATH);
	*(strrchr(path, '\\') + 1) = '\0';
	injector::thiscall<void(void *_this, const char *a2)>::call(0x404A10, _this, path);
	return 1;
}

void PatchInstallPath()
{
	auto pattern = hook::pattern("E8 ? ? ? ? B8 ? ? ? ? 8D 48 01 8D 9B 00 00 00 00"); //0x40994D in config tool
	if (pattern.size() > 0)
	{
		injector::MakeCALL(pattern.get(0).get<uintptr_t>(0), GetInstallPath, true);
		bConfigTool = true;
	}
}

DWORD WINAPI Init(LPVOID)
{
	auto pattern = hook::pattern("89 65 E8 8B F4 89 3E 56");
	if (!(pattern.size() > 0) && !bDelay)
	{
		bDelay = true;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
		return 0;
	}

	if (bDelay)
	{
		while (!(pattern.size() > 0))
			pattern = hook::pattern("89 65 E8 8B F4 89 3E 56");
	}

	PatchInstallPath();
	if (bConfigTool)
		return 0;

	CIniReader iniReader("");
	Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
	Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
	bool bDisableBorders = iniReader.ReadInteger("MAIN", "DisableBorders", 1) != 0;

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

	pattern = hook::pattern("8B 4C 24 1C 8B 44 24 18 8B 54 24 20 68"); //402564
	struct SetResHook
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.eax = Screen.Width;
			regs.ecx = Screen.Height;
		}
	}; injector::MakeInline<SetResHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(8));

	//B500D8 - 2D stretch
	//409D40 - fov related
	pattern = hook::pattern("A1 ? ? ? ? D9 18 8B 0D ? ? ? ? D9 44 24 10"); //409D88
	static auto dword_B8B77C = *pattern.get(0).get<float**>(1);
	struct ScalingHook
	{
		void operator()(injector::reg_pack& regs)
		{
			float temp = 0;
			_asm fstp    dword ptr[temp]
			//if (*(uint32_t*)&temp != 0x3FEA4D6C && *(uint32_t*)(regs.esp + 0x6C) != 0)
				**dword_B8B77C = (temp * ((4.0f / 3.0f) / (Screen.fAspectRatio)));
			//else
			//	**dword_B8B77C = temp;
		}
	}; injector::MakeInline<ScalingHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(7));

	static float fObjFix = (0.5f / ((4.0f / 3.0f) / (Screen.fAspectRatio)));
	pattern = hook::pattern("D8 0D ? ? ? ? D9 5C 24 08 8B 5C 24 08 53 E8"); //43C25B Objects at screen edges disappear, let's fix that
	injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &fObjFix, true);

	pattern = hook::pattern("D9 18 D9 EE D9 58 04 D9 EE D9 58 08 D9 EE"); //AF4363
	struct TextScalingHook
	{
		void operator()(injector::reg_pack& regs)
		{
			float temp = 0.0f;
			_asm fmul    st, st(1)
			_asm fstp    dword ptr[temp]
			_asm fldz
			*(float*)regs.eax = (temp * ((4.0f / 3.0f) / (Screen.fAspectRatio)));
		}
	}; injector::MakeInline<TextScalingHook>(pattern.get(0).get<uint32_t>(-2), pattern.get(0).get<uint32_t>(4));

	pattern = hook::pattern("D9 EE D9 58 2C D9 45 0C D8 45 10 D8 C9"); //AF43A5
	struct TextPosHook
	{
		void operator()(injector::reg_pack& regs)
		{
			float temp = 0.0f;
			_asm fldz
			_asm fstp    dword ptr[temp]
			*(float*)(regs.eax + 0x2C) = temp;
			*(float*)(regs.ebp + 0x10) = Screen.fWidth43;
		}
	}; injector::MakeInline<TextPosHook>(pattern.get(0).get<uint32_t>(0));

	if (bDisableBorders)
	{
		pattern = hook::pattern("0F 94 C1 A3 ? ? ? ? 89 0D"); //40241C
		injector::MakeNOP(pattern.get(0).get<uint32_t>(0), 3, true);
		pattern = hook::pattern("74 0A C7 05 ? ? ? ? 01 00 00 00 EB"); //ABA32A
		injector::WriteMemory(pattern.get(0).get<uint32_t>(8), 0, true);
		injector::WriteMemory(*pattern.get(0).get<uint32_t*>(4), 0, true);
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
