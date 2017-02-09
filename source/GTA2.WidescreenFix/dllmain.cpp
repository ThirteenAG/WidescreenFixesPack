#include "stdafx.h"
#include "..\includes\CPatch.h"

HWND hWnd;
DWORD jmpAddress;
void asm_patch();
DWORD res_x;
DWORD res_y, res_y43;

DWORD WINAPI Thread(LPVOID)
{
	CIniReader iniReader("");
	res_x = iniReader.ReadInteger("MAIN", "ResX", 0);
	res_y = iniReader.ReadInteger("MAIN", "ResY", 0);

	if (!res_x || !res_y) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		res_x = info.rcMonitor.right - info.rcMonitor.left;
		res_y = info.rcMonitor.bottom - info.rcMonitor.top;
	}
	res_y43 = static_cast<DWORD>((res_x / (4.0f / 3.0f)) - res_y);

	CPatch::SetUInt(0x4C94A1, res_y43); //subs

	CPatch::SetPointer(0x4BAD1E, &res_x); // = 0x673578 + 0x0  -> mov     eax, window_width
	CPatch::SetPointer(0x4BADAF, &res_x); // = 0x673578 + 0x0  -> mov     eax, window_width
	CPatch::SetPointer(0x4BAE58, &res_x); // = 0x673578 + 0x0  -> mov     edx, window_width
	CPatch::SetPointer(0x4BAF13, &res_x); // = 0x673578 + 0x0  -> mov     eax, window_width
	CPatch::SetPointer(0x4CAEF3, &res_x); // = 0x673578 + 0x0  -> mov     eax, window_width
	CPatch::SetPointer(0x4CB166, &res_x); // = 0x673578 + 0x0  -> mov     eax, window_width
	CPatch::SetPointer(0x4CB182, &res_x); // = 0x673578 + 0x0  -> mov     eax, window_width
	CPatch::SetPointer(0x4CB294, &res_x); // = 0x673578 + 0x0  -> mov     eax, window_width
	//CPatch::SetPointer(0x4CB2D7, &res_x); // = 0x673578 + 0x0  -> mov     window_width, ecx
	//CPatch::SetPointer(0x4CB30C, &res_x); // = 0x673578 + 0x0  -> mov     window_width, eax
	CPatch::SetPointer(0x4CB3B7, &res_x); // = 0x673578 + 0x0  -> mov     eax, window_width
	CPatch::SetPointer(0x4CB7DE, &res_x); // = 0x673578 + 0x0  -> mov     eax, window_width
	CPatch::SetPointer(0x4CB8D4, &res_x); // = 0x673578 + 0x0  -> mov     ecx, window_width
	CPatch::SetPointer(0x4CC613, &res_x); // = 0x673578 + 0x0  -> mov     ecx, window_width
	//CPatch::SetPointer(0x4CC621, &res_x); // = 0x673578 + 0x0  -> mov     window_width, eax
	CPatch::SetPointer(0x4CC63A, &res_x); // = 0x673578 + 0x0  -> mov     ecx, window_width
	CPatch::SetPointer(0x4D135F, &res_x); // = 0x673578 + 0x0  -> mov     edi, window_width

	CPatch::SetPointer(0x4BAD48, &res_y); // = 0x6732E8 + 0x0  -> mov     ecx, window_height
	CPatch::SetPointer(0x4BADD9, &res_y); // = 0x6732E8 + 0x0  -> mov     ecx, window_height
	CPatch::SetPointer(0x4BAE81, &res_y); // = 0x6732E8 + 0x0  -> mov     eax, window_height
	CPatch::SetPointer(0x4BAF44, &res_y); // = 0x6732E8 + 0x0  -> mov     edx, window_height
	CPatch::SetPointer(0x4CAEF9, &res_y); // = 0x6732E8 + 0x0  -> mov     ecx, window_height
	CPatch::SetPointer(0x4CB176, &res_y); // = 0x6732E8 + 0x0  -> mov     edx, window_height
	CPatch::SetPointer(0x4CB29A, &res_y); // = 0x6732E8 + 0x0  -> mov     ecx, window_height
	//CPatch::SetPointer(0x4CB2DC, &res_y); // = 0x6732E8 + 0x0  -> mov     window_height, eax
	//CPatch::SetPointer(0x4CB325, &res_y); // = 0x6732E8 + 0x0  -> mov     window_height, eax
	CPatch::SetPointer(0x4CB3C5, &res_y); // = 0x6732E8 + 0x0  -> mov     ecx, window_height
	CPatch::SetPointer(0x4CB7BE, &res_y); // = 0x6732E8 + 0x0  -> mov     edx, window_height
	CPatch::SetPointer(0x4CB8DF, &res_y); // = 0x6732E8 + 0x0  -> mov     eax, window_height
	//CPatch::SetPointer(0x4CC627, &res_y); // = 0x6732E8 + 0x0  -> mov     window_height, 1E0h
	CPatch::SetPointer(0x4CC650, &res_y); // = 0x6732E8 + 0x0  -> mov     edx, window_height
	CPatch::SetPointer(0x4D138B, &res_y); // = 0x6732E8 + 0x0  -> mov     ecx, window_height


	CPatch::SetPointer(0x4CB2A1, &res_x); // = 0x6732E4 + 0x0  -> mov     ebx, full_width
	//CPatch::SetPointer(0x4CB2E2, &res_x); // = 0x6732E4 + 0x0  -> mov     full_width, ecx
	//CPatch::SetPointer(0x4CB33E, &res_x); // = 0x6732E4 + 0x0  -> mov     full_width, eax
	//CPatch::SetPointer(0x4CB3A0, &res_x); // = 0x6732E4 + 0x0  -> cmp     full_width, ebx
	CPatch::SetPointer(0x4CB577, &res_x); // = 0x6732E4 + 0x0  -> mov     ecx, full_width
	CPatch::SetPointer(0x4CB598, &res_x); // = 0x6732E4 + 0x0  -> mov     ecx, full_width
	//CPatch::SetPointer(0x4CB5AD, &res_x); // = 0x6732E4 + 0x0  -> mov     full_width, eax
	CPatch::SetPointer(0x4CB5D8, &res_x); // = 0x6732E4 + 0x0  -> mov     edx, full_width
	CPatch::SetPointer(0x4CB695, &res_x); // = 0x6732E4 + 0x0  -> mov     eax, full_width

	CPatch::SetPointer(0x4CB2B1, &res_y); // = 0x6732E0 + 0x0  -> mov     ebp, full_height
	//CPatch::SetPointer(0x4CB354, &res_y); // = 0x6732E0 + 0x0  -> mov     full_height, eax
	//CPatch::SetPointer(0x4CB3A8, &res_y); // = 0x6732E0 + 0x0  -> cmp     full_height, ebp
	CPatch::SetPointer(0x4CB571, &res_y); // = 0x6732E0 + 0x0  -> mov     eax, full_height
	//CPatch::SetPointer(0x4CB5BC, &res_y); // = 0x6732E0 + 0x0  -> mov     full_height, 1E0h
	CPatch::SetPointer(0x4CB5D2, &res_y); // = 0x6732E0 + 0x0  -> mov     ecx, full_height
	CPatch::SetPointer(0x4CB6AB, &res_y); // = 0x6732E0 + 0x0  -> mov     ecx, full_height

	//subs
	//CPatch::SetUInt(0x4C94A1, static_cast<unsigned int>(480 / (res_x / res_y)));
	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Thread, NULL, 0, NULL);
	}
	return TRUE;
}

