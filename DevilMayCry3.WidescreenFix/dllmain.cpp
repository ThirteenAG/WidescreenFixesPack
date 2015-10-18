#include "..\includes\stdafx.h"

HWND hWnd;
float fAspectRatio, fWidthScale, fHUDScaleX, fTextScaleX, fComboScale;

void Init()
{
	CIniReader iniReader("");
	int ResX = iniReader.ReadInteger("MAIN", "ResX", 0);
	int ResY = iniReader.ReadInteger("MAIN", "ResY", 0);
	bool FixHud = iniReader.ReadInteger("MAIN", "FixHud", 1) != 0;

	if (!ResX || !ResY) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		ResX = info.rcMonitor.right - info.rcMonitor.left;
		ResY = info.rcMonitor.bottom - info.rcMonitor.top;
	}

	//game
	injector::WriteMemory(0x4028DE + 0x2, ResX, true);
	injector::WriteMemory(0x4028E4 + 0x2, ResY, true);

	injector::WriteMemory<uchar>(0x4028C2, 0xEB, true);

	injector::WriteMemory(0x40A7E2 + 0x6, ResX, true);
	injector::WriteMemory(0x40A7EC + 0x6, ResY, true);

	injector::WriteMemory(0x781394, ResX, true);
	injector::WriteMemory(0x781398, ResY, true);

	injector::MakeInline<0x6CDB8E>([](injector::reg_pack& regs)
	{
		regs.eax = *(DWORD*)0x0252CAC8;
		*(float*)(regs.edi + 0x60) = fWidthScale;
	});

	injector::WriteMemory<uchar>(0x6CDBB9, 0x70, true);

	fAspectRatio = (float)ResX / (float)ResY;
	fWidthScale = ((1.0f / fAspectRatio) * (4.0f / 3.0f));
	fHUDScaleX = ((1.0f / (480.0f * (fAspectRatio))) * 10.0f) / 8.0f;
	fTextScaleX = ((1.0f / (480.0f * (fAspectRatio))) * 10.0f) / 4.0f;
	fComboScale = 64.0f / ((fAspectRatio) / (4.0f / 3.0f));

	//HUD
	if (FixHud)
	{
		injector::WriteMemory<float>(0x7491DC, fHUDScaleX, true);
		injector::WriteMemory(0x406E40 - 0x6B + 0x2, &fTextScaleX, true);
		injector::WriteMemory(0x406E40 - 0x45 + 0x2, &fTextScaleX, true);

		injector::WriteMemory(0x65374C + 0x1, static_cast<int>(320.0f + (200.0f / (0.001953125 / fHUDScaleX))), true);
		injector::WriteMemory(0x6536F7 + 0x2, &fComboScale, true);
	}
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}