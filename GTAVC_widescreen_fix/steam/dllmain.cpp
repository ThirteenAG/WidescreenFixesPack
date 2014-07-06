#include "stdafx.h"
#include "hooks.h"

#define gGameVersion (*(unsigned int *)0x600CB8)
#define GTA_VC_1_0     0x53FF1B8B
#define GTA_VC_STEAM     0xF04F883
DWORD WINAPI PatchSteamExe(LPVOID);

DWORD WINAPI MVLScopeFix(LPVOID)
{
	static int i;
	do
	{
		Sleep(100);
			i++;
			if (i > 100)
			break;

	} while (GetModuleHandle("vehmod_retail.dll") == NULL);

	if (GetModuleHandle("vehmod_retail.dll"))
		CPatch::SetPointer((DWORD)GetModuleHandle("vehmod_retail.dll") + 0x55401, &fMVLScopeFix);

	return 0;
}

void Init()
{
	if (gGameVersion == GTA_VC_1_0)
	{
		//Aspect Ratio fix
		CPatch::RedirectCall(0x4A5B33, CDraw__CalculateAspectRatio);
		CPatch::RedirectCall(0x4A5D65, CDraw__CalculateAspectRatio);
		CPatch::RedirectCall(0x4A5E65, CDraw__CalculateAspectRatio);
		CPatch::RedirectCall(0x4A5F68, CDraw__CalculateAspectRatio);
		CPatch::RedirectCall(0x4A7198, CDraw__CalculateAspectRatio);


		//FOV fix
		CPatch::RedirectCall(0x46DBA8, setScreenFieldOfView);
		CPatch::RedirectCall(0x4A476B, setScreenFieldOfView);
		CPatch::RedirectCall(0x4A4C07, setScreenFieldOfView);
		CPatch::RedirectCall(0x620806, setScreenFieldOfView);
		CPatch::RedirectCall(0x6279BC, setScreenFieldOfView);
		CPatch::RedirectCall(0x627A09, setScreenFieldOfView);
		CPatch::RedirectJump(0x46DD97, FiretruckAmblncFix);

		//CPatch::RedirectJump(0x, CameraSize_patch);


		//Menu Fix
		MenuFix();
		CPatch::RedirectJump(0x600C03, RsSelectDeviceHook);

		//Coronas fix
		CPatch::SetUChar(0x57786A, 0x0B);

		//CBrightLights::Render
		CPatch::Nop(0x57468D, 5);

		//CCamera::DrawBordersForWideScreen tweak
		CPatch::Nop(0x46FC97, 3);
		CPatch::Nop(0x46FCE9, 3);
		CPatch::SetUChar(0x55AD82, 0x02u);
		CPatch::Nop(0x557139, 5);
		CPatch::RedirectJump(0x46FAFC, DrawBordersForWideScreen_patch);

		//UI fix
		HudFix();

		//INI
		ApplyINIchanges();

		//MVL
		if (MVLScopeFix) { CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&MVLScopeFix, NULL, 0, NULL); }
	}
	else 
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&PatchSteamExe, NULL, 0, NULL);
	}
}

DWORD WINAPI PatchSteamExe(LPVOID)
{
	Sleep(3000);
	//if (gGameVersion == GTA_VC_STEAM)
	//{
	//Aspect Ratio fix
	CPatch::RedirectCall(0x4A5A03, CDraw__CalculateAspectRatio_steam);
	CPatch::RedirectCall(0x4A5C35, CDraw__CalculateAspectRatio_steam);
	CPatch::RedirectCall(0x4A5D35, CDraw__CalculateAspectRatio_steam);
	CPatch::RedirectCall(0x4A5E38, CDraw__CalculateAspectRatio_steam);
	CPatch::RedirectCall(0x4A7068, CDraw__CalculateAspectRatio_steam);


	//FOV fix
	CPatch::RedirectCall(0x46DA88, setScreenFieldOfView_steam);
	CPatch::RedirectCall(0x4A4627, setScreenFieldOfView_steam);
	CPatch::RedirectCall(0x4A4ACE, setScreenFieldOfView_steam);
	CPatch::RedirectCall(0x620446, setScreenFieldOfView_steam);
	CPatch::RedirectCall(0x62767C, setScreenFieldOfView_steam);
	CPatch::RedirectCall(0x6276C9, setScreenFieldOfView_steam);
	CPatch::RedirectJump(0x46DC77, FiretruckAmblncFix_steam);

	//CPatch::RedirectJump(0x, CameraSize_patch);


	//Menu Fix
	MenuFix_steam();
	CPatch::RedirectJump(0x600863, RsSelectDeviceHook);

	//Coronas fix
	CPatch::SetUChar(0x57775A, 0x0B);

	//CBrightLights::Render
	CPatch::Nop(0x57457D, 5);

	//CCamera::DrawBordersForWideScreen tweak
	CPatch::Nop(0x46FB77, 3);
	CPatch::Nop(0x46FBC9, 3);
	CPatch::SetUChar(0x55AC72, 0x02u);
	CPatch::Nop(0x557029, 5);
	CPatch::RedirectJump(0x46F9DC, DrawBordersForWideScreen_patch);

	//UI fix
	HudFix_steam();

	//INI
	//ApplyINIchanges_steam();
	//}

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

