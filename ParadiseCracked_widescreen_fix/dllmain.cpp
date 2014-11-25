#include "stdafx.h"
#include "..\includes\CPatch.h"

HANDLE HndThread;
HWND hWndTarget = NULL;

int view_stretch_address = 0x61F0B4;

long screen_height;
long screen_width;

float view_stretch;

BOOL CALLBACK EnumProc(HWND hWnd, LPARAM /*lParam*/) {
	char szWindowName[1024];
	GetWindowText(hWnd, szWindowName, 1024);

	char szClassName[1024];
	GetClassName(hWnd, szClassName, 1024);

	if (strcmp(szClassName, "Paradise Cracked") == 0 && strcmp(szWindowName, "Paradise Cracked") == 0) {
		hWndTarget = hWnd;
		SetLastError(ERROR_SUCCESS);
		return FALSE;
	}
	return TRUE;
}

bool FindGameWindow() {
	return (!EnumWindows(EnumProc, NULL) && (GetLastError() == ERROR_SUCCESS));
}

int Thread()
{
	RECT rcClient;

	while (!FindGameWindow()) {
		Sleep(0);
	}

	if (!(*(float *) view_stretch_address == 0.75f)) // Currently works only for 1.06.001
		return 0;

	while (TRUE) {
		Sleep(0);

		GetClientRect(hWndTarget, &rcClient);

		screen_width = rcClient.right - rcClient.left;
		screen_height = rcClient.bottom - rcClient.top;

		view_stretch = (screen_width == 0 ? 0.75f : (screen_height / (float) screen_width));

		CPatch::SetFloat(view_stretch_address, view_stretch);
	}

	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH) {
		HndThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE) &Thread, NULL, 0, NULL);
	}
	return TRUE;
}
