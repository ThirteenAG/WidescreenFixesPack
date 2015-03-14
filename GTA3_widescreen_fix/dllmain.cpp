#include "stdafx.h"
#include "hooks.h"

#define gGameVersion (*(unsigned int *)0x601048)
#define GTA_3_1_0     0x3A83126F
#define GTA_3_1_1     0x3F8CCCCD
#define GTA_3_STEAM     0x47BDA5
DWORD WINAPI PatchSteamExe(LPVOID);

DWORD WINAPI SPHandler(LPVOID)
{
	static int i;
	do
	{
		Sleep(100);
		i++;
		if (i > 100)
			break;

	} while (GetModuleHandle("SilentPatchIII.asi") == NULL);

	if (GetModuleHandle("SilentPatchIII.asi"))
		ApplyINIchanges();

	return 0;
}

void Init()
{
	if (gGameVersion == GTA_3_1_0)
	{
		//Aspect Ratio fix
		CPatch::Nop(0x48D06B, 7);
		CPatch::RedirectJump(0x584B26, CDraw__CalculateAspectRatio);
		CPatch::Nop(0x48D074, 2);
		
		//FOV fix
		CPatch::RedirectCall(0x46E92E, setScreenFieldOfView);
		CPatch::RedirectCall(0x46EACE, setScreenFieldOfView);
		CPatch::RedirectCall(0x48C0DA, setScreenFieldOfView);
		CPatch::RedirectCall(0x48C50B, setScreenFieldOfView);
		CPatch::RedirectCall(0x59BCA9, setScreenFieldOfView);
		CPatch::RedirectCall(0x59BCCC, setScreenFieldOfView);
		CPatch::RedirectJump(0x46EC0D, FiretruckAmblncFix);
		CPatch::RedirectJump(0x4A42A0, RadarScaling_compat_patch);

		//Menu Fix
		CDraw__CalculateAspectRatio();
		//MenuFix();
		CPatch::RedirectJump(0x581F83, RsSelectDeviceHook);
		

		//Coronas fix
		CPatch::SetUChar(0x51C46A, 0x0B);

		//CBrightLights::Render
		CPatch::Nop(0x518DCA, 5);

		//CCamera::DrawBordersForWideScreen tweak
		CPatch::Nop(0x46086E, 2);
		CPatch::Nop(0x46B618, 2);
		CPatch::Nop(0x46EBD9, 2);
		//CPatch::Nop(0x47B7EF, 2);
		//CPatch::Nop(0x48B041, 2);
		CPatch::Nop(0x48CF81, 2);
		CPatch::SetChar(0x48D2D5, 1);
		CPatch::RedirectJump(0x46B430, DrawBordersForWideScreen_patch);

		//UI fix
		HudFix();

		//INI
		ApplyINIchanges();

		//SilenPatch
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&SPHandler, NULL, 0, NULL);

		//'Wide Screen' text replacement
		//auto CText__Get = (int(__stdcall *)(void *, wchar_t*)) 0x52C5A0;
		//int pFED_WIS = CText__Get((void *)0x941520, L"FED_TRA");
		//const wchar_t* wide = L"Borders    ";
		//CPatch::Patch2(pFED_WIS, (void*)wide, sizeof(wchar_t)* 11);
	}
	else 
	{
		if (gGameVersion == GTA_3_STEAM)
		{
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&PatchSteamExe, NULL, 0, NULL);
		}
	}
}

DWORD WINAPI PatchSteamExe(LPVOID)
{
	while (!(*(DWORD*)0x5C6FD0 == 0x53E58955))
	{
	Sleep(0);
	}
	//Aspect Ratio fix
	CPatch::Nop(0x48D0FB, 7);
	CPatch::RedirectJump(0x584D56, CDraw__CalculateAspectRatio_steam);
	CPatch::Nop(0x48D104, 2);

	//FOV fix
	CPatch::RedirectCall(0x46D3D0 + 0x153E, setScreenFieldOfView_steam);
	CPatch::RedirectCall(0x46D3D0 + 0x16DE, setScreenFieldOfView_steam);
	CPatch::RedirectCall(0x48BF50 + 0x20A, setScreenFieldOfView_steam);
	CPatch::RedirectCall(0x48C540 + 0x5B, setScreenFieldOfView_steam);
	CPatch::RedirectCall(0x599020 + 0x39, setScreenFieldOfView_steam);
	CPatch::RedirectCall(0x599060 + 0x1C, setScreenFieldOfView_steam);
	CPatch::RedirectJump(0x46EBED, FiretruckAmblncFix_steam);

	while (true)
	{
		Sleep(0);
		if (*(DWORD*)0x953308 != 0) break; //GTA_3_STEAM
	}
	//Menu Fix
	CDraw__CalculateAspectRatio_steam();
	MenuFix_steam();
	CPatch::RedirectJump(0x5821B3, RsSelectDeviceHook_steam);



	//Coronas fix
	CPatch::SetUChar(0x51C62A, 0x0B);

	//CBrightLights::Render
	CPatch::Nop(0x518F8A, 5);

	//CCamera::DrawBordersForWideScreen tweak
	CPatch::Nop(0x46086E, 2);
	CPatch::Nop(0x46B5E8, 2);
	CPatch::Nop(0x46EBB9, 2);
	//CPatch::Nop(0x47B8BF, 2);
	//CPatch::Nop(0x48B0C1, 2);
	CPatch::Nop(0x48D011, 2);
	CPatch::SetChar(0x48D365, 1);
	CPatch::RedirectJump(0x46B400, DrawBordersForWideScreen_patch_steam);

	//UI fix
	HudFix_steam();

	//INI
	ApplyINIchanges_steam();

	//'Wide Screen' text replacement
	//Sleep(5000);
	//auto CText__Get = (int(__stdcall *)(void *, char  const*)) 0x52C180;
	//int pFED_WIS = CText__Get((void *)0x951818, "FED_WIS");
	//const wchar_t* wide = L"Borders    ";
	//CPatch::Patch2(pFED_WIS, (void*)wide, sizeof(wchar_t)* 11);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}

