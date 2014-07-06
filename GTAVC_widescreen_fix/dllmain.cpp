#include "stdafx.h"
#include "hooks.h"

#define gGameVersion (*(unsigned int *)0x601048)
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
		CPatch::RedirectCall(0x4A5C63, CDraw__CalculateAspectRatio);
		CPatch::RedirectCall(0x4A5E95, CDraw__CalculateAspectRatio);
		CPatch::RedirectCall(0x4A5F95, CDraw__CalculateAspectRatio);
		CPatch::RedirectCall(0x4A6098, CDraw__CalculateAspectRatio);
		CPatch::RedirectCall(0x4A72C8, CDraw__CalculateAspectRatio);


		//FOV fix
		CPatch::RedirectCall(0x46DCC8, setScreenFieldOfView);
		CPatch::RedirectCall(0x4A48AB, setScreenFieldOfView);
		CPatch::RedirectCall(0x4A4D3A, setScreenFieldOfView);
		CPatch::RedirectCall(0x620BC6, setScreenFieldOfView);
		CPatch::RedirectCall(0x627CFC, setScreenFieldOfView);
		CPatch::RedirectCall(0x627D49, setScreenFieldOfView);
		CPatch::RedirectJump(0x46DEB7, FiretruckAmblncFix);
		CPatch::RedirectJump(0x4C5D0A, RadarScaling_compat_patch);


		//Menu Fix
		MenuFix();
		CPatch::RedirectJump(0x600FA3, RsSelectDeviceHook);

		//Coronas fix
		CPatch::SetUChar(0x57797A, 0x0B);

		//CBrightLights::Render
		CPatch::Nop(0x57479D, 5);

		//CCamera::DrawBordersForWideScreen tweak
		CPatch::Nop(0x46FDA9, 12);//CPatch::Nop(0x46FDB7, 3); //'All Hands On Deck' crash fix
		//CPatch::Nop(0x0046FD9B, 6); //Disable fading?
		CPatch::Nop(0x46FE09, 3);
		CPatch::SetUChar(0x55AE92, 0x02u);
		CPatch::Nop(0x557249, 5);
		CPatch::RedirectJump(0x46FC1C, DrawBordersForWideScreen_patch);

		//UI fix
		HudFix();

		//INI
		ApplyINIchanges();

		//MVL
		if (MVLScopeFix) { CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&MVLScopeFix, NULL, 0, NULL); }

		//'Wide Screen' text replacement
		auto CText__Get = (int(__thiscall *)(void *, char  const*)) 0x584F30;
		int pFED_WIS = CText__Get((void *)0x94B220, "FED_WIS");
		const wchar_t* wide = L"Borders    ";
		CPatch::Patch2(pFED_WIS, (void*)wide, sizeof(wchar_t)* 11);
	}
	else 
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&PatchSteamExe, NULL, 0, NULL);
	}
}

DWORD WINAPI PatchSteamExe(LPVOID)
{
	while (true)
	{
		Sleep(0);
		if (*(DWORD*)0x666BA0 == 0x53E58955) break; //GTA_VC_STEAM
	}
	//Aspect Ratio fix
	CPatch::RedirectCall(0x4A5B33, CDraw__CalculateAspectRatio_steam);
	CPatch::RedirectCall(0x4A5D65, CDraw__CalculateAspectRatio_steam);
	CPatch::RedirectCall(0x4A5E65, CDraw__CalculateAspectRatio_steam);
	CPatch::RedirectCall(0x4A5F68, CDraw__CalculateAspectRatio_steam);
	CPatch::RedirectCall(0x4A7198, CDraw__CalculateAspectRatio_steam);


	//FOV fix
	CPatch::RedirectCall(0x46DBA8, setScreenFieldOfView_steam);
	CPatch::RedirectCall(0x4A476B, setScreenFieldOfView_steam);
	CPatch::RedirectCall(0x4A4C07, setScreenFieldOfView_steam);
	CPatch::RedirectCall(0x620806, setScreenFieldOfView_steam);
	CPatch::RedirectCall(0x6279BC, setScreenFieldOfView_steam);
	CPatch::RedirectCall(0x627A09, setScreenFieldOfView_steam);
	CPatch::RedirectJump(0x46DD97, FiretruckAmblncFix_steam);

	//CPatch::RedirectJump(0x, CameraSize_patch);

	while (true)
	{
		Sleep(0);
		if (*(DWORD*)0x9B38E4 != 0) break; //GTA_VC_STEAM
	}
	//Menu Fix
	MenuFix_steam();
	CPatch::RedirectJump(0x600C03, RsSelectDeviceHook_steam);

	//Coronas fix
	CPatch::SetUChar(0x57786A, 0x0B);

	//CBrightLights::Render
	CPatch::Nop(0x57468D, 5);

	//CCamera::DrawBordersForWideScreen tweak
	CPatch::Nop(0x46FC89, 12);
	CPatch::Nop(0x46FCE9, 3);
	CPatch::SetUChar(0x55AD82, 0x02u);
	CPatch::Nop(0x557139, 5);
	CPatch::RedirectJump(0x46FAFC, DrawBordersForWideScreen_patch_steam);

	//UI fix
	HudFix_steam();

	//INI
	ApplyINIchanges_steam();

	//'Wide Screen' text replacement
	Sleep(3000);
	auto CText__Get = (int(__thiscall *)(void *, char  const*)) 0x584D60;
	int pFED_WIS = CText__Get((void *)0x94A228, "FED_WIS");
	const wchar_t* wide = L"Borders    ";
	CPatch::Patch2(pFED_WIS, (void*)wide, sizeof(wchar_t)* 11);

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

