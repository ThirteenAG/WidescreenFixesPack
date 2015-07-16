#include "stdafx.h"
#include "..\includes\GTA\common.h"

void GetMemoryAddresses()
{
	if (!injector::address_manager::singleton().IsSteam())
	{
		RsGlobal = (RsGlobalType *)0x9B48D8;
		CDraw::pfScreenAspectRatio = (float*)0x94DD38;
		CDraw::pfScreenFieldOfView = (float*)0x696658;
		CSprite2dDrawRect = (int(__cdecl *)(CRect const &, CRGBA const &)) 0x577B00;
		bWideScreen = 0x869652; BordersVar1 = 0x7E4738; BordersVar2 = 0x7E474C;
		FindPlayerVehicle = (int(__cdecl *)()) 0x4BC1E0;
		IsInCutscene = 0x7E46F5;
	}
	else
	{
		RsGlobal = (RsGlobalType *)0x9B38E0;
		CDraw::pfScreenAspectRatio = (float*)0x94CD40;
		CDraw::pfScreenFieldOfView = (float*)0x695660;
		CSprite2dDrawRect = (int(__cdecl *)(CRect const &, CRGBA const &)) 0x5779F0;
		bWideScreen = 0x86865A; BordersVar1 = 0x7E3740; BordersVar2 = 0x7E3754;
		FindPlayerVehicle = (int(__cdecl *)()) 0x4BC0B0;
		IsInCutscene = 0x7E36FD;
	}
}

void OverwriteResolution()
{
	CIniReader iniReader("");
	ResX = iniReader.ReadInteger("MAIN", "ResX", -1);
	ResY = iniReader.ReadInteger("MAIN", "ResY", -1);

	if (!ResX || !ResY) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		ResX = info.rcMonitor.right - info.rcMonitor.left;
		ResY = info.rcMonitor.bottom - info.rcMonitor.top;
	}
	else if (ResX == -1 || ResY == -1)
	{
		return;
	}

		if (!injector::address_manager::singleton().IsSteam())
		{
			injector::WriteMemory<uchar>(0x490095 + 0x2, 22, true);
			injector::WriteMemory<int>(0x600E7B + 0x3, ResX, true);
			injector::WriteMemory<int>(0x600E84 + 0x4, ResY, true);
			injector::WriteMemory<int>(0x584C4A + 0x6, ResX, true);
			injector::WriteMemory<int>(0x584C54 + 0x6, ResY, true);
			injector::WriteMemory<int>(0x584C5E + 0x6, ResX, true);
			injector::WriteMemory<int>(0x584C68 + 0x6, ResY, true);
			injector::WriteMemory<uchar>(0x600E8E + 0x4, 32, true);
			injector::WriteMemory<uchar>(0x600EBA, 0xEB, true); //jl      short loc_600E60 > jmp      short loc_600E60
			injector::MakeNOP(0x600E57, 5, true);
		}
		else
		{
			injector::WriteMemory<char>(0x48FFA5 + 0x2, 22, true);
			injector::WriteMemory<int>(0x600ADB + 0x3, ResX, true);
			injector::WriteMemory<int>(0x600AE4 + 0x4, ResY, true);
			injector::WriteMemory<char>(0x600AEE + 0x4, 32, true);
			injector::WriteMemory<char>(0x600B1A, 0xEBu, true); //jl      short loc_600E60 > jmp      short loc_600E60
			injector::MakeNOP(0x600AB7, 5, true);
		}
}

void FixAspectRatio()
{
	if (!injector::address_manager::singleton().IsSteam())
	{
		injector::MakeCALL(0x4A5C63, CDraw::CalculateAspectRatio, true);
		injector::MakeCALL(0x4A5E95, CDraw::CalculateAspectRatio, true);
		injector::MakeCALL(0x4A5F95, CDraw::CalculateAspectRatio, true);
		injector::MakeCALL(0x4A6098, CDraw::CalculateAspectRatio, true);
		injector::MakeCALL(0x4A72C8, CDraw::CalculateAspectRatio, true);
	}
	else
	{
		injector::MakeCALL(0x4A5B33, CDraw::CalculateAspectRatio, true);
		injector::MakeCALL(0x4A5D65, CDraw::CalculateAspectRatio, true);
		injector::MakeCALL(0x4A5E65, CDraw::CalculateAspectRatio, true);
		injector::MakeCALL(0x4A5F68, CDraw::CalculateAspectRatio, true);
		injector::MakeCALL(0x4A7198, CDraw::CalculateAspectRatio, true);
	}
}

template<uintptr_t addr>
void ProcessCamOnAStringAvoidTheGeometryHook()
{
	using func_hook = injector::function_hooker_thiscall<addr, void(void*, RwV3d const&, RwV3d const&, RwV3d&, float)>;
	injector::make_static_hook<func_hook>([](func_hook::func_type funcAvoidTheGeometry, void* _this, RwV3d const& a1, RwV3d const& a2, RwV3d& a3, float a4)
	{
		if (*CDraw::pfScreenAspectRatio < 2.0f)
		funcAvoidTheGeometry(_this, a1, a2, a3, a4);
	});
}

void FixFOV()
{
	if (FOVControl)
	{
		fFOVControlValue = *(float*)FOVControl;
	}
	else
	{
		fFOVControlValue = 1.0f;
	}

	if (!injector::address_manager::singleton().IsSteam())
	{
		ProcessCamOnAStringAvoidTheGeometryHook<0x480456>();
		injector::MakeCALL(0x46DCC8, CDraw::SetFOV, true);
		injector::MakeCALL(0x4A48AB, CDraw::SetFOV, true);
		injector::MakeCALL(0x4A4D3A, CDraw::SetFOV, true);
		injector::MakeCALL(0x620BC6, CDraw::SetFOV, true);
		injector::MakeCALL(0x627CFC, CDraw::SetFOV, true);
		injector::MakeCALL(0x627D49, CDraw::SetFOV, true);
		injector::MakeInline<0x46DEB7, 0x46DEBD>([](injector::reg_pack& regs)
		{
			*(float*)(regs.ebx + 0xF4) = fEmergencyVehiclesFix;
		});
		injector::MakeInline<0x4C5D0A, 0x4C5D10>([](injector::reg_pack&)
		{
			_asm
			{
				//fstp    ds : 0x974BEC
				fstp    ds : fRadarScaling
				mov edx, fRadarScaling
				mov ds : [0x974BEC], edx
			}
			fRadarScaling -= 180.0f;
		});
	}
	else
	{
		ProcessCamOnAStringAvoidTheGeometryHook<0x480336>();
		injector::MakeCALL(0x46DBA8, CDraw::SetFOV, true);
		injector::MakeCALL(0x4A476B, CDraw::SetFOV, true);
		injector::MakeCALL(0x4A4C07, CDraw::SetFOV, true);
		injector::MakeCALL(0x620806, CDraw::SetFOV, true);
		injector::MakeCALL(0x6279BC, CDraw::SetFOV, true);
		injector::MakeCALL(0x627A09, CDraw::SetFOV, true);
		injector::MakeInline<0x46DD97, 0x46DD9D>([](injector::reg_pack& regs)
		{
			*(float*)(regs.ebx + 0xF4) = fEmergencyVehiclesFix;
		});
		injector::MakeInline<0x4C5BCA, 0x4C5BD0>([](injector::reg_pack&)
		{
			_asm
			{
				//fstp    ds : 0x974BEC
				fstp    ds : fRadarScaling
				mov edx, fRadarScaling
				mov ds : [0x973BF4], edx
			}
			fRadarScaling -= 180.0f;
		});
	}
}

void FixMenu()
{
	if (!injector::address_manager::singleton().IsSteam())
	{
	#pragma region MenuFunctions
		injector::WriteMemory(0x1 + 0x48F482, RsGlobal->MaximumWidth, true); // Map help text scaling
		//injector::WriteMemory(0x1 + 0x48F52A, RsGlobal->MaximumWidth, true); // Map help text alignment
		injector::WriteMemory(0x1 + 0x49E1E6, RsGlobal->MaximumWidth, true); // Text at right top corner scaling
		injector::WriteMemory(0x1 + 0x49E824, RsGlobal->MaximumWidth, true); // Left aligned text scaling 
		//injector::WriteMemory(0x400000+0x1+0x9FC3D, RsGlobal->MaximumWidth, true); // Game menu text alignment
		injector::WriteMemory(0x1 + 0x49FD6D, RsGlobal->MaximumWidth, true); // Right aligned text scaling 

		injector::WriteMemory(0x1 + 0x490712, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x490DA1, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4912D0, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x491354, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4913E4, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49154C, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4919CE, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x491A79, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x491B25, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x491B91, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x491C55, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x491CC1, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49222C, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4923B0, RsGlobal->MaximumWidth, true);
		//injector::WriteMemory(0x1 + 0x4925BA, RsGlobal->MaximumWidth, true); //green selection line
		injector::WriteMemory(0x1 + 0x492724, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49276E, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49294C, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4929A6, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x492C03, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x492CDE, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x492D98, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4933BA, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x493454, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x493506, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x493633, RsGlobal->MaximumWidth, true);
		//injector::WriteMemory(0x1 + 0x493697, RsGlobal->MaximumWidth, true); //skins page
		injector::WriteMemory(0x1 + 0x49372E, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49377B, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4937C5, RsGlobal->MaximumWidth, true);
		//injector::WriteMemory(0x1 + 0x493825, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49389E, RsGlobal->MaximumWidth, true);
		//injector::WriteMemory(0x1 + 0x493C64, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4948B0, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4948FD, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49494A, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x494997, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4949E1, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x494A7B, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x494AEF, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x494B4D, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4954D9, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4955B4, RsGlobal->MaximumWidth, true);
		//injector::WriteMemory(0x1 + 0x4956C2, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4973DE, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4973FD, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x497520, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49753F, RsGlobal->MaximumWidth, true);
		//injector::WriteMemory(0x1 + 0x498315, RsGlobal->MaximumWidth, true);
		//injector::WriteMemory(0x1 + 0x49834E, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49878B, RsGlobal->MaximumWidth, true); injector::WriteMemory<float>(0x68D508, 0.42000002f * 2.0f, true); //Loading game text
		injector::WriteMemory(0x1 + 0x4987E9, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49A3B1, RsGlobal->MaximumWidth, true);
		//injector::WriteMemory(0x1 + 0x49A40F, RsGlobal->MaximumWidth, true); // BRIEF text
		injector::WriteMemory(0x1 + 0x49A4E9, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49A535, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49B40F, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49B55B, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49B5A6, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49B60C, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49B694, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49B6DE, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49B7C2, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49B833, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49BB9C, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49BBE6, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49C1CF, RsGlobal->MaximumWidth, true);
		//injector::WriteMemory(0x1 + 0x49C22D, RsGlobal->MaximumWidth, true); //criminal rating (stats)
		//injector::WriteMemory(0x1 + 0x49C2D5, RsGlobal->MaximumWidth, true); //criminal rating (stats)
		//injector::WriteMemory(0x1 + 0x49C380, RsGlobal->MaximumWidth, true); //criminal rating(stats)
		//injector::WriteMemory(0x1 + 0x49C418, RsGlobal->MaximumWidth, true); //criminal rating (stats)

		/*injector::WriteMemory(0x1 + 0x49C4F6, RsGlobal->MaximumWidth, true); // draw radio logos slider in menu
		injector::WriteMemory(0x1 + 0x49C547, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49C586, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49C5C5, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49C904, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49C951, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49C9EB, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CA38, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CAE8, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CB35, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CBD3, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CC20, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CCB3, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CD00, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CDBD, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CE0A, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CEA8, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CEF5, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CF93, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CFE0, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D073, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D0C0, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D17D, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D1CA, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D260, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D2AD, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D35E, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D3AD, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D61F, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D670, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D6AF, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D6EE, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D79E, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D7EF, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D82E, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D86D, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D91A, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D96B, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D9AA, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D9E9, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DA9C, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DAED, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DB2C, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DB6B, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DC18, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DC69, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DCA8, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DCE7, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DD9A, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DDEB, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DE2A, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DE69, RsGlobal->MaximumWidth, true);*/

		injector::WriteMemory(0x1 + 0x49E357, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49E432, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49E7A5, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49F1EF, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49FD0B, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4A0F98, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4A14D4, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4A15ED, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4A168A, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4A16AF, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4A1A34, RsGlobal->MaximumWidth, true);

		//injector::WriteMemory(0x1 + 0x4A1B1F, RsGlobal->MaximumWidth, true);
		//injector::WriteMemory(0x1 + 0x4A1BBC, RsGlobal->MaximumWidth, true); // mouse sensitivity
		//injector::WriteMemory(0x1 + 0x4A1BE1, RsGlobal->MaximumWidth, true);
		/*injector::WriteMemory(0x1 + 0x4A1F56, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4A3A2F, RsGlobal->MaximumWidth, true);*/

		float fMenuTextScale = 0.6f*(RsGlobal->MaximumHeight / SCREEN_RESOLUTION_HEIGHT);
		float fMenuTextScale2 = 0.48f*(RsGlobal->MaximumHeight / SCREEN_RESOLUTION_HEIGHT);
		injector::WriteMemory<float>(0x68D0C0, fMenuTextScale, true);
		injector::WriteMemory<float>(0x68D5C0, fMenuTextScale2, true);

		injector::WriteMemory(0x495792 + 0xFA + 0x2, &fvcLogoScale, true);
		injector::WriteMemory(0x4A212D + 0x126C + 0x2, &fvcLogoScale, true);
		injector::WriteMemory(0x4A212D + 0x1353 + 0x2, &fvcLogoScale, true);


		injector::WriteMemory<float>(0x68D0B8, 0.5f, true); // controls page text scaling
		injector::WriteMemory<float>(0x68D100, 0.8f, true); // controls page text scaling

		injector::WriteMemory<float>(0x68D094, 2.0f, true); // menu header scaling
		injector::WriteMemory<float>(0x68D118, 1.8f, true); // controls page (in car, on foot text)

		injector::WriteMemory<float>(0x68C34C, 1.2f, true); //Map help text
		injector::WriteMemory<float>(0x68C344, 0.5f, true);
		injector::MakeNOP(0x48F4A8, 5, true);

		injector::WriteMemory<float>(0x68D680, 0.86f, true); //Stats text width scale
		injector::WriteMemory(0x49C2E0 + 0x2, 0x68D094, true);
	#pragma endregion MenuFunctions
	}
	else
	{
	#pragma region MenuFunctionsSteam
		injector::WriteMemory(0x1 + 0x48F392, RsGlobal->MaximumWidth, true); // Map help text scaling
		//injector::WriteMemory(0x1 + 0x48F43A, RsGlobal->MaximumWidth, true); // Map help text alignment
		injector::WriteMemory(0x1 + 0x49E0A5, RsGlobal->MaximumWidth, true); // Text at right top corner scaling
		injector::WriteMemory(0x1 + 0x49E6E3, RsGlobal->MaximumWidth, true); // Left aligned text scaling 
		//CPatch::SetUInt(0x400000+0x1+0x9FC3D, RsGlobal->MaximumWidth, true); // Game menu text alignment
		injector::WriteMemory(0x1 + 0x49FC2C, RsGlobal->MaximumWidth, true); // Right aligned text scaling 

		injector::WriteMemory(0x1 + 0x490622, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x490CB1, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4911E0, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x491264, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4912F4, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49145C, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4918DE, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x491989, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x491A35, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x491AA1, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x491B65, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x491BD1, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49213C, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4922C0, RsGlobal->MaximumWidth, true);
		//injector::WriteMemory(0x1 + 0x4924CA, RsGlobal->MaximumWidth, true); //green selection line
		injector::WriteMemory(0x1 + 0x492634, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49267E, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49285C, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4928B6, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x492B13, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x492BEE, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x492CA8, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4932CA, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x493364, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x493416, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x493543, RsGlobal->MaximumWidth, true);
		//injector::WriteMemory(0x1 + 0x4935A7, RsGlobal->MaximumWidth, true); //skins page
		injector::WriteMemory(0x1 + 0x49363E, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49368B, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4936D5, RsGlobal->MaximumWidth, true);
		//injector::WriteMemory(0x1 + 0x493735, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4937AE, RsGlobal->MaximumWidth, true);
		//injector::WriteMemory(0x1 + 0x493B74, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4947C0, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49480D, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49485A, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4948A7, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4948F1, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49498B, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4949FF, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x494A5D, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4953E9, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4954C4, RsGlobal->MaximumWidth, true);
		//injector::WriteMemory(0x1 + 0x4955D2, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4972EE, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49730D, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x497430, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49744F, RsGlobal->MaximumWidth, true);
		//injector::WriteMemory(0x1 + 0x498225, RsGlobal->MaximumWidth, true);
		//injector::WriteMemory(0x1 + 0x49825E, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49869B, RsGlobal->MaximumWidth, true); injector::WriteMemory<float>(0x68C508, 0.42000002f * 2.0f, true); //Loading game text
		injector::WriteMemory(0x1 + 0x4986F9, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49A2D2, RsGlobal->MaximumWidth, true);
		//injector::WriteMemory(0x1 + 0x49A330, RsGlobal->MaximumWidth, true); // BRIEF text
		injector::WriteMemory(0x1 + 0x49A40A, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49A456, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49B330, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49B47C, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49B4C7, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49B52D, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49B5B5, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49B5FF, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49B6E3, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49B754, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49BA6B, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49BAB5, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49C08E, RsGlobal->MaximumWidth, true);
		//injector::WriteMemory(0x1 + 0x49C0EC, RsGlobal->MaximumWidth, true); //criminal rating (stats)
		//injector::WriteMemory(0x1 + 0x49C194, RsGlobal->MaximumWidth, true); //criminal rating (stats)
		//injector::WriteMemory(0x1 + 0x49C23F, RsGlobal->MaximumWidth, true); //criminal rating(stats)
		//injector::WriteMemory(0x1 + 0x49C2D7, RsGlobal->MaximumWidth, true); //criminal rating (stats)

		/*injector::WriteMemory(0x1 + 0x49C3B5, RsGlobal->MaximumWidth, true); // draw radio logos slider in menu
		injector::WriteMemory(0x1 + 0x49C406, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49C445, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49C484, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49C7C3, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49C810, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49C8AA, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49C8F7, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49C9A7, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49C9F4, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CA92, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CADF, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CB72, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CBBF, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CC7C, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CCC9, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CD67, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CDB4, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CE52, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CE9F, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CF32, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49CF7F, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D03C, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D089, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D11F, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D16C, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D21D, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D26C, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D4DE, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D52F, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D56E, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D5AD, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D65D, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D6AE, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D6ED, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D72C, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D7D9, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D82A, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D869, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D8A8, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D95B, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D9AC, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49D9EB, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DA2A, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DAD7, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DB28, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DB67, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DBA6, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DC59, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DCAA, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DCE9, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49DD28, RsGlobal->MaximumWidth, true);*/

		injector::WriteMemory(0x1 + 0x49E216, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49E2F1, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49E664, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49F0AE, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x49FBCA, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4A0E57, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4A1393, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4A14AC, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4A1549, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4A156E, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4A18F3, RsGlobal->MaximumWidth, true);

		//injector::WriteMemory(0x1 + 0x4A19DE, RsGlobal->MaximumWidth, true);
		//injector::WriteMemory(0x1 + 0x4A1A7B, RsGlobal->MaximumWidth, true); // mouse sensitivity
		//injector::WriteMemory(0x1 + 0x4A1AA0, RsGlobal->MaximumWidth, true);
		/*injector::WriteMemory(0x1 + 0x4A1E15, RsGlobal->MaximumWidth, true);
		injector::WriteMemory(0x1 + 0x4A38EE, RsGlobal->MaximumWidth, true);*/

		float fMenuTextScale = 0.6f*(RsGlobal ->MaximumHeight / SCREEN_RESOLUTION_HEIGHT);
		float fMenuTextScale2 = 0.48f*(RsGlobal ->MaximumHeight / SCREEN_RESOLUTION_HEIGHT);
		injector::WriteMemory<float>(0x68C0C0, fMenuTextScale, true);
		injector::WriteMemory<float>(0x68C5C0, fMenuTextScale2, true);

		injector::WriteMemory(0x4956A2 + 0xFA + 0x2, &fvcLogoScale, true);
		injector::WriteMemory(0x4A1FEC + 0x126C + 0x2, &fvcLogoScale, true);
		injector::WriteMemory(0x4A1FEC + 0x1353 + 0x2, &fvcLogoScale, true);


		injector::WriteMemory<float>(0x68C0B8, 0.5f, true); // controls page text scaling
		injector::WriteMemory<float>(0x68C100, 0.8f, true); // controls page text scaling

		injector::WriteMemory<float>(0x68C094, 2.0f, true); // menu header scaling
		injector::WriteMemory<float>(0x68C118, 1.8f, true); // controls page (in car, on foot text)

		injector::WriteMemory<float>(0x68B34C, 1.2f, true); //Map help text
		injector::WriteMemory<float>(0x68B344, 0.5f, true);
		injector::MakeNOP(0x48F3B8, 5, true);

		injector::WriteMemory<float>(0x68C680, 0.86f, true); //Stats text width scale
		injector::WriteMemory(0x49C19F + 0x2, 0x68C094, true);
	#pragma endregion MenuFunctionsSteam
	}
}

void RsSelectDeviceHook()
{
	FixMenu();
	if (!injector::address_manager::singleton().IsSteam())
	{
		injector::MakeInline<0x600FA9, 0x600FA9 + 5>([](injector::reg_pack&)
		{
			injector::WriteMemory<uchar>(0x600FA9 + 5, 0xC3, true);
			FixMenu();
			if (!injector::address_manager::singleton().IsSteam() && IVRadarScaling)
			{
				IVRadar();
			}
		});
	}
	else
	{
		injector::MakeInline<0x600C09, 0x600C09 + 5>([](injector::reg_pack&)
		{
			injector::WriteMemory<uchar>(0x600C09 + 5, 0xC3, true);
			FixMenu();
		});
	}
}

void FixCoronas()
{
	if (!injector::address_manager::singleton().IsSteam())
	{
		injector::WriteMemory<uint8_t>(0x57797A, 0x0B, true);
		injector::MakeNOP(0x57479D, 5, true); //CBrightLights::Render
	}
	else
	{
		injector::WriteMemory<uint8_t>(0x57786A, 0x0B, true);
		injector::MakeNOP(0x57468D, 5, true); //CBrightLights::Render
	}
}

void FixBorders()
{
	if (!injector::address_manager::singleton().IsSteam())
	{
		injector::MakeNOP(0x46FDA9, 12, true); //CPatch::Nop(0x46FDB7, 3); //'All Hands On Deck' crash fix
		injector::MakeNOP(0x46FE09, 3, true);
		DrawBordersForWideScreenHook<(0x4A61EE)>();
		DrawBordersForWideScreenHook<(0x54A223)>();
	}
	else
	{
		injector::MakeNOP(0x46FC89, 12, true); //CPatch::Nop(0x46FDB7, 3); //'All Hands On Deck' crash fix
		injector::MakeNOP(0x46FCE9, 3, true);
		DrawBordersForWideScreenHook<(0x4A6060 + 0x5E)>();
		DrawBordersForWideScreenHook<(0x544687 + 0x5A8C)>();
	}
}

void FixHUD()
{
	if (!injector::address_manager::singleton().IsSteam())
	{
		//crosshair fix
		injector::WriteMemory(0x46F927, &fCrosshairPosFactor, true);
		injector::WriteMemory(0x5575FA, &fCrosshairPosFactor, true);
		injector::WriteMemory(0x606B25, &fCrosshairPosFactor, true);

	#pragma region HUDFunctions
	#define	 	CDarkel__DrawMessages	 	0x429FE0
	#define	 	CGarages__PrintMessages	 	0x42F2B0
	#define	 	CMenuManager__PrintMap	 	0x49A5B7
	#define	 	CMenuManager__DrawContollerScreenExtraText	 	0x49150B
	#define	 	CRadar__DrawRadarMask	 	0x4C1A20
	#define	 	CRadar__DrawRadarSection	 	0x4C1D60
	#define	 	CRadar__DrawYouAreHereSprite	 	0x4C2980
	#define	 	CRadar__DrawRadarSprite	 	0x4C2D00
	#define	 	CRadar__DrawRotatingRadarSprite	 	0x4C2E70
	#define	 	CRadar__ShowRadarTraceWithHeight	 	0x4C32F0
	#define	 	CRadar__DrawBlips	 	0x4C4200
	#define	 	CRadar__DrawLegend	 	0x4C4A10
	#define	 	CRadar__DrawEntityBlip	 	0x4C5170
	#define	 	CRadar__DrawCoordBlip	 	0x4C5680
	#define	 	CClouds__Render	 	0x53F380
	#define	 	CHud__DrawAfterFade	 	0x5566E0
	#define	 	CHud__Draw	 	0x557320
	#define	 	CMBlur__AddRenderFx	 	0x55D160
	#define	 	CParticle__Render	 	0x5608C0
	#define	 	CSpecialFX__Render2DFXs	 	0x573F20
	#define	 	cMusicManager__DisplayRadioStationName	 	0x5F9EE0
	#define	 	CSceneEdit__Draw	 	0x605DB0
	#define	 	CRunningScript__ProcessCommands1100To1199	 	0x606730
	#define	 	CReplay__Display	 	0x620BE0
	#define	 	Render2DStuff 0x4A6190
	#define	 	LoadingScreen 0x4A69D0
	#define	 	debug_internal_something 0x491997
	#define	 	CMenuManager_PrintStringMenu 0x4985DD

		injector::WriteMemory(CDarkel__DrawMessages + 0xA7 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CDarkel__DrawMessages + 0x168 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CDarkel__DrawMessages + 0x269 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CDarkel__DrawMessages + 0x369 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CDarkel__DrawMessages + 0x3D6 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CDarkel__DrawMessages + 0x475 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CDarkel__DrawMessages + 0x4DE + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CDarkel__DrawMessages + 0x569 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(CDarkel__DrawMessages + 0x91 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CDarkel__DrawMessages + 0x152 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CDarkel__DrawMessages + 0x254 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CDarkel__DrawMessages + 0x347 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CDarkel__DrawMessages + 0x3B4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CDarkel__DrawMessages + 0x453 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CDarkel__DrawMessages + 0x4BD + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CDarkel__DrawMessages + 0x553 + 0x2, &fWideScreenHeightScaleDown, true);




		injector::WriteMemory(CGarages__PrintMessages + 0x45 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(CGarages__PrintMessages + 0x2F + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CGarages__PrintMessages + 0x126 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CGarages__PrintMessages + 0x1B4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CGarages__PrintMessages + 0x208 + 0x2, &fWideScreenHeightScaleDown, true);




		injector::WriteMemory(0x48F429 + 0x94 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(debug_internal_something + 0xFD + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x492D82 + 0x653 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager_PrintStringMenu + 0x124 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x4D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x12B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x178 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x1D2 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x29F + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x2E8 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x340 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x412 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x45A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x4B3 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x581 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x5CA + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x622 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x6F3 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x73D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x79B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x86B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x8B5 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x91E + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x9F4 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0xA3C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0xAA8 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0xB78 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0xBC2 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0xC2C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0xCFA + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0xD44 + 0x2, &fWideScreenWidthScaleDown, true);

		/*injector::WriteMemory(0x49DF40 + 0x2C7 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x140C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x146A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x14C4 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x1524 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x18CB + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x1929 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x1983 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x19E3 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x1AB4 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x1B12 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x1B6C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x1BCC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x1D0C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x24A8 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x29D1 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x2F0D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x3449 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x39A9 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x3ECF + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(0x4A212D + 0x648 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A212D + 0x699 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A212D + 0x744 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A212D + 0x794 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A212D + 0x83E + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A212D + 0x890 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A212D + 0x947 + 0x2, &fWideScreenWidthScaleDown, true); Menu Background and Text
		injector::WriteMemory(0x4A212D + 0x999 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A212D + 0xBD0 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A212D + 0xC21 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A212D + 0xCCC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A212D + 0xD1C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A212D + 0xDC9 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A212D + 0xE1B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A212D + 0xED8 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A212D + 0xF2A + 0x2, &fWideScreenWidthScaleDown, true);*/

		injector::WriteMemory(0x49066B + 0x4A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49066B + 0x4CC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49066B + 0x4F6 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49066B + 0x67E + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49066B + 0x6A8 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49066B + 0xE33 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__DrawContollerScreenExtraText + 0x116 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__DrawContollerScreenExtraText + 0x1A1 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__DrawContollerScreenExtraText + 0x44E + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(debug_internal_something + 0xB68 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(debug_internal_something + 0xBBD + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(debug_internal_something + 0xE91 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(debug_internal_something + 0xFF0 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(debug_internal_something + 0x12AB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492D82 + 0xC6B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492D82 + 0xCBC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492D82 + 0xF35 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492D82 + 0xFF6 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492D82 + 0x11F7 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492D82 + 0x124D + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492D82 + 0x1332 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492D82 + 0x138E + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492D82 + 0x1DAC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492D82 + 0x23B8 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492D82 + 0x24A0 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492D82 + 0x25C9 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492D82 + 0x2608 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492D82 + 0x2796 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49598B + 0xA10 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49598B + 0xA6B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4965BA + 0x14A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4965BA + 0x193 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49A36D + 0x83 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49A36D + 0x211 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x89 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0xFA + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x154 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x207 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x274 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x2C2 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x375 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x3E4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x434 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x4E9 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x556 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x5A5 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x658 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x6C8 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x717 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x7D1 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x841 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x88F + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x954 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0x9C9 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0xA18 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0xADE + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0xB4D + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0xB9C + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0xC62 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0xCD1 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMenuManager__PrintMap + 0xD1E + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49B971 + 0x421 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49B971 + 0x481 + 0x2, &fWideScreenHeightScaleDown, true);
		/*injector::WriteMemory(0x49DF40 + 0x8B0 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x92F + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x12FA + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x143E + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x1496 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x14F4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x154C + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x18FD + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x1955 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x19B3 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x1A0B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x1AE6 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x1B3E + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x1B9C + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x1BF4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x1D49 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x1E16 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x1E78 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DF40 + 0x1EF2 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A212D + 0x674 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A212D + 0x6B9 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A212D + 0x76E + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A212D + 0x7B3 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A212D + 0x86A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A212D + 0x8B0 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A212D + 0x973 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A212D + 0x9B9 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A212D + 0xBFC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A212D + 0xC41 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A212D + 0xCF6 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A212D + 0xD3B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A212D + 0xDF5 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A212D + 0xE3B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A212D + 0xF04 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A212D + 0xF4A + 0x2, &fWideScreenHeightScaleDown, true);*/




		injector::WriteMemory(Render2DStuff + 0x287 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(Render2DStuff + 0x2DB + 0x2, &fWideScreenWidthScaleDown, true);
		if (!NoLoadingBarFix)
		{
			injector::WriteMemory(LoadingScreen + 0x118 + 0x2, &fWideScreenWidthScaleDown, true);
			injector::WriteMemory(LoadingScreen + 0x1DD + 0x2, &fWideScreenWidthScaleDown, true);
			injector::WriteMemory(LoadingScreen + 0x290 + 0x2, &fWideScreenWidthScaleDown, true);
			injector::WriteMemory(LoadingScreen + 0x328 + 0x2, &fWideScreenWidthScaleDown, true);
		}

		injector::WriteMemory(Render2DStuff + 0x11A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(Render2DStuff + 0x164 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(Render2DStuff + 0x1D3 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(Render2DStuff + 0x21D + 0x2, &fWideScreenHeightScaleDown, true);
		if (!NoLoadingBarFix)
		{
			injector::WriteMemory(LoadingScreen + 0x147 + 0x2, &fWideScreenHeightScaleDown, true);
			injector::WriteMemory(LoadingScreen + 0x206 + 0x2, &fWideScreenHeightScaleDown, true);
			injector::WriteMemory(LoadingScreen + 0x2B9 + 0x2, &fWideScreenHeightScaleDown, true);
			injector::WriteMemory(LoadingScreen + 0x312 + 0x2, &fWideScreenHeightScaleDown, true);
		}



		injector::WriteMemory(CRadar__DrawRadarMask + 0xF8 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawRadarMask + 0x13E + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawRadarMask + 0x23D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawRadarMask + 0x282 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawRadarSection + 0x382 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawRadarSection + 0x3C1 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawYouAreHereSprite + 0xB + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawYouAreHereSprite + 0x308 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawRadarSprite + 0x9 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawRotatingRadarSprite + 0x1A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__ShowRadarTraceWithHeight + 0xD2 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__ShowRadarTraceWithHeight + 0x1A3 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__ShowRadarTraceWithHeight + 0x255 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__ShowRadarTraceWithHeight + 0x330 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__ShowRadarTraceWithHeight + 0x3B6 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__ShowRadarTraceWithHeight + 0x43D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawBlips + 0xAB + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawBlips + 0xF4 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawBlips + 0x2E6 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawBlips + 0x32F + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawBlips + 0x71D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawBlips + 0x762 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawLegend + 0x6A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawLegend + 0x1C2 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawLegend + 0x279 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawLegend + 0x348 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawLegend + 0x3F3 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawLegend + 0x497 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawLegend + 0x51D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawLegend + 0x733 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawEntityBlip + 0x2BF + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawEntityBlip + 0x307 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawCoordBlip + 0x25F + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CRadar__DrawCoordBlip + 0x2A7 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(CRadar__DrawRadarMask + 0x124 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawRadarMask + 0x170 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawRadarMask + 0x269 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawRadarMask + 0x2B7 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawRadarSection + 0x3A1 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawRadarSection + 0x3FC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawYouAreHereSprite + 0x45 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawYouAreHereSprite + 0x2F2 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawRadarSprite + 0x47 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawRotatingRadarSprite + 0x4D + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__ShowRadarTraceWithHeight + 0x72 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__ShowRadarTraceWithHeight + 0x14B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__ShowRadarTraceWithHeight + 0x217 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__ShowRadarTraceWithHeight + 0x2F2 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawBlips + 0xDB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawBlips + 0x12C + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawBlips + 0x310 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawBlips + 0x361 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawBlips + 0x749 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawBlips + 0x796 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawLegend + 0x19C + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawLegend + 0x251 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawLegend + 0x322 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawLegend + 0x3CB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawLegend + 0x4C2 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawLegend + 0x549 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawLegend + 0x715 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawEntityBlip + 0x2ED + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawEntityBlip + 0x33B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawCoordBlip + 0x28D + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CRadar__DrawCoordBlip + 0x2DB + 0x2, &fWideScreenHeightScaleDown, true);






		//injector::WriteMemory(CHud__DrawAfterFade + 0x8E + 0x2, &fWideScreenWidthScaleDown, true);
		//injector::WriteMemory(CHud__DrawAfterFade + 0x10C + 0x2, &fWideScreenWidthScaleDown, true);
		//injector::WriteMemory(CHud__DrawAfterFade + 0x128 + 0x2, &fWideScreenWidthScaleDown, true);
		//injector::WriteMemory(CHud__DrawAfterFade + 0x1EC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x31C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x343 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x433 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x45A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x716 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x740 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x870 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x89C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x908 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x944 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0xBF3 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x358 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x43C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x559 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x67D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x87A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x91E + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x9C2 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xD90 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xECB + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xFDC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x12A1 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x138B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1431 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1458 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x15F2 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1691 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x18DD + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x193A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x19B9 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1A1B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1B4D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1BAA + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1C29 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1CAE + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1D1A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1E98 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2011 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2108 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x219A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x24EF + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x251F + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x256D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x268F + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x29C1 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x29F1 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2A3F + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2B5B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2BBB + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2CBE + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2DB6 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2E39 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2EAA + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2F68 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3050 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x308B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3159 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3218 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x32D8 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3371 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3429 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x34E8 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3680 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x37B9 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x387E + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x38FC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3918 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x39DD + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3C02 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3C38 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3D00 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3D2D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3DB6 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x41BF + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x41EF + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x4247 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x433C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x43FF + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x442C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x4454 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x484C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x487C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x497F + 0x2, &fWideScreenWidthScaleDown, true);

		//injector::WriteMemory(CHud__DrawAfterFade + 0x72 + 0x2, &fWideScreenHeightScaleDown, true);
		//injector::WriteMemory(CHud__DrawAfterFade + 0x1BF + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x306 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x3B7 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x41D + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x4CE + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x6F8 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x7B4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x85B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x887 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x8EA + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x926 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0xBCD + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3B6 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x49A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x5B1 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x6E2 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x849 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x932 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x9D6 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xD6F + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xEB5 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xFBB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1254 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x13BC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x141B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x15D1 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x167B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x18BC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1998 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1A05 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1B2C + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1C08 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1C98 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1E73 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1FFB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x20E7 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2178 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x24DA + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x250A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2548 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x266A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x29AC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x29DC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2A1A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2B36 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2BA5 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2C9D + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2DA0 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2E23 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2E89 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2F46 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x303A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3114 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x319B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3413 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x34A7 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x363A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3778 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3862 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x39AF + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3C22 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3C64 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3CEA + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3D81 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x41A4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x41DA + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x4311 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x43EA + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x4417 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x46EB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x4837 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x4867 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x4959 + 0x2, &fWideScreenHeightScaleDown, true);



		injector::WriteMemory(CMBlur__AddRenderFx + 0x14A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CMBlur__AddRenderFx + 0x330 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(CMBlur__AddRenderFx + 0x120 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CMBlur__AddRenderFx + 0x306 + 0x2, &fWideScreenHeightScaleDown, true);




		injector::WriteMemory(CParticle__Render + 0x3A2 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CParticle__Render + 0x523 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CParticle__Render + 0x648 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CParticle__Render + 0x773 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CParticle__Render + 0x80C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CParticle__Render + 0x8A6 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CParticle__Render + 0xDA8 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CParticle__Render + 0xE1C + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(CParticle__Render + 0x3E0 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CParticle__Render + 0x561 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CParticle__Render + 0x68A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CParticle__Render + 0x7B9 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CParticle__Render + 0x84C + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CParticle__Render + 0x8E6 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CParticle__Render + 0xE9F + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CParticle__Render + 0xF16 + 0x2, &fWideScreenHeightScaleDown, true);



		injector::WriteMemory(CSpecialFX__Render2DFXs + 0x30 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CSpecialFX__Render2DFXs + 0x3B8 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(CSpecialFX__Render2DFXs + 0x1A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CSpecialFX__Render2DFXs + 0x3A2 + 0x2, &fWideScreenHeightScaleDown, true);



		injector::WriteMemory(cMusicManager__DisplayRadioStationName + 0x27C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(cMusicManager__DisplayRadioStationName + 0x2AB + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory<float>(0x6CD9B8, 0.875f, true);
		injector::WriteMemory<float>(0x6CD9C0, 0.518f, true);
		injector::WriteMemory<float>(0x6CD9CC, 1.0f, true);

		injector::WriteMemory(cMusicManager__DisplayRadioStationName + 0x266 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(cMusicManager__DisplayRadioStationName + 0x2E7 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(cMusicManager__DisplayRadioStationName + 0x380 + 0x2, &fWideScreenHeightScaleDown, true);




		injector::WriteMemory(CSceneEdit__Draw + 0x71 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CSceneEdit__Draw + 0x134 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CSceneEdit__Draw + 0x1AF + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CSceneEdit__Draw + 0x24B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CSceneEdit__Draw + 0x2C5 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CSceneEdit__Draw + 0x3D2 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CSceneEdit__Draw + 0x487 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(CSceneEdit__Draw + 0x5B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CSceneEdit__Draw + 0x114 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CSceneEdit__Draw + 0x195 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CSceneEdit__Draw + 0x22B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CSceneEdit__Draw + 0x2AB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CSceneEdit__Draw + 0x3AA + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CSceneEdit__Draw + 0x465 + 0x2, &fWideScreenHeightScaleDown, true);




		//injector::WriteMemory(CRunningScript__ProcessCommands1100To1199 + 0x423 + 0x2, &fWideScreenWidthScaleDown, true);


		//injector::WriteMemory(CClouds__Render + 0x66B + 0x2, &fWideScreenWidthScaleDown, true);



		injector::WriteMemory(0x61DEB0 + 0xC9 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x61DEB0 + 0x158 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x61DEB0 + 0x202 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x61DEB0 + 0x28C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x61DEB0 + 0x2C1 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x61DEB0 + 0x33E + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(0x61DEB0 + 0xEC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x61DEB0 + 0x17B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x61DEB0 + 0x225 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x61DEB0 + 0x270 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x61DEB0 + 0x328 + 0x2, &fWideScreenHeightScaleDown, true);


		injector::WriteMemory(CReplay__Display + 0x66 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(CReplay__Display + 0x50 + 0x2, &fWideScreenHeightScaleDown, true);


		injector::WriteMemory<double>(0x697AD8, (1.8 / 1.50), true); //h
		injector::WriteMemory<double>(0x697AE0, (1.7 / 1.50), true); //w
		injector::WriteMemory<double>(0x697AE8, (1.5 / 1.50), true); //w


		injector::WriteMemory(CHud__DrawAfterFade + 0x31C + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x343 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x433 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x45A + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x716 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x740 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x870 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x89C + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x908 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x944 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0xBF3 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x358 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x43C + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x559 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x67D + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x87A + 0x2, &fCustomWideScreenWidthScaleDown, true);
		//injector::WriteMemory(CHud__Draw + 0x91E + 0x2, &fCustomWideScreenWidthScaleDown, true); //sniper crosshair fix
		injector::WriteMemory(CHud__Draw + 0x9C2 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xD90 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xECB + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xFDC + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x12A1 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x138B + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1431 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1458 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x15F2 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1691 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x18DD + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x193A + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x19B9 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1A1B + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1B4D + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1BAA + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1C29 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1CAE + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1D1A + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1E98 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2011 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2108 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x219A + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x24EF + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x251F + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x256D + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x268F + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x29C1 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x29F1 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2A3F + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2B5B + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2BBB + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2CBE + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2DB6 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2E39 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2EAA + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2F68 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3050 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x308B + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3159 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3218 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x32D8 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3371 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3429 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x34E8 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		//injector::WriteMemory(CHud__Draw + 0x3680 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		//injector::WriteMemory(CHud__Draw + 0x37B9 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x387E + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x38FC + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3918 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x39DD + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3C02 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3C38 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3D00 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3D2D + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3DB6 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x41BF + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x41EF + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x4247 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x433C + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x43FF + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x442C + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x4454 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x484C + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x487C + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x497F + 0x2, &fCustomWideScreenWidthScaleDown, true);


		injector::WriteMemory(CHud__DrawAfterFade + 0x306 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x3B7 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x41D + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x4CE + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x6F8 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x7B4 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x85B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x887 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x8EA + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0x926 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__DrawAfterFade + 0xBCD + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3B6 + 0x2, &fCrosshairHeightScaleDown, true); // let's make crosshair proportional
		injector::WriteMemory(CHud__Draw + 0x49A + 0x2, &fCrosshairHeightScaleDown, true); // let's make crosshair proportional
		injector::WriteMemory(CHud__Draw + 0x5B1 + 0x2, &fCrosshairHeightScaleDown, true); // let's make crosshair proportional
		injector::WriteMemory(CHud__Draw + 0x6E2 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x849 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		//injector::WriteMemory(CHud__Draw + 0x932 + 0x2, &fCustomWideScreenHeightScaleDown, true); //sniper crosshair fix
		injector::WriteMemory(CHud__Draw + 0x9D6 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xD6F + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xEB5 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xFBB + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1254 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x13BC + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x141B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x15D1 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x167B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x18BC + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1998 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1A05 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1B2C + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1C08 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1C98 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1E73 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1FFB + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x20E7 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2178 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x24DA + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x250A + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2548 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x266A + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x29AC + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x29DC + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2A1A + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2B36 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2BA5 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2C9D + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2DA0 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2E23 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2E89 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2F46 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x303A + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3114 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x319B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3413 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x34A7 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		//injector::WriteMemory(CHud__Draw + 0x363A + 0x2, &fCustomWideScreenHeightScaleDown, true);
		//injector::WriteMemory(CHud__Draw + 0x3778 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3862 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x39AF + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3C22 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3C64 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3CEA + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3D81 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x41A4 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x41DA + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x4311 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x43EA + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x4417 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x46EB + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x4837 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x4867 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x4959 + 0x2, &fCustomWideScreenHeightScaleDown, true);
	#pragma endregion HUDFunctions
	}
	else
	{
		//crosshair fix
		injector::WriteMemory(0x46F807, &fCrosshairPosFactor, true);
		injector::WriteMemory(0x5574EA, &fCrosshairPosFactor, true);
		injector::WriteMemory(0x606745, &fCrosshairPosFactor, true);

	#pragma region HUDFunctionsSteam
		#define	 	steam_CDarkel__DrawMessages	 	0x429FB0
		#define	 	steam_CGarages__PrintMessages	 	0x42F280
		#define	 	steam_CMenuManager__PrintMap	 	0x49A4D8
		#define	 	steam_CMenuManager__DrawContollerScreenExtraText	 	0x49141B
		#define	 	steam_CRadar__DrawRadarMask	 	0x4C18F0
		#define	 	steam_CRadar__DrawRadarSection	 	0x4C1C30
		#define	 	steam_CRadar__DrawYouAreHereSprite	 	0x4C2850
		#define	 	steam_CRadar__DrawRadarSprite	 	0x4C2BD0
		#define	 	steam_CRadar__DrawRotatingRadarSprite	 	0x4C2D40
		#define	 	steam_CRadar__ShowRadarTraceWithHeight	 	0x4C31C0
		#define	 	steam_CRadar__DrawBlips	 	0x4C40D0
		#define	 	steam_CRadar__DrawLegend	 	0x4C48D0
		#define	 	steam_CRadar__DrawEntityBlip	 	0x4C5030
		#define	 	steam_CRadar__DrawCoordBlip	 	0x4C5540
		#define	 	steam_CClouds__Render	 	0x53F270
		#define	 	steam_steam_CHud__DrawAfterFade	 	0x5565D0
		#define	 	steam_CHud__Draw	 	0x557210
		#define	 	steam_CMBlur__AddRenderFx	 	0x55D050
		#define	 	steam_CParticle__Render	 	0x5607B0
		#define	 	steam_CSpecialFX__Render2DFXs	 	0x573E10
		#define	 	steam_cMusicManager__DisplayRadioStationName	 	0x5F9B40
		#define	 	steam_CSceneEdit__Draw	 	0x6059D0
		#define	 	steam_CRunningScript__ProcessCommands1100To1199	 	0x606350
		#define	 	steam_CReplay__Display	 	0x620820
		#define	 	steam_Render2DStuff 0x4A6060
		#define	 	steam_LoadingScreen 0x4A68A0
		#define	 	steam_debug_internal_something 0x4918A7
		#define	 	steam_CMenuManager_PrintStringMenu 0x4984ED

		injector::WriteMemory(steam_CDarkel__DrawMessages + 0xA7 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CDarkel__DrawMessages + 0x168 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CDarkel__DrawMessages + 0x269 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CDarkel__DrawMessages + 0x369 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CDarkel__DrawMessages + 0x3D6 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CDarkel__DrawMessages + 0x475 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CDarkel__DrawMessages + 0x4DE + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CDarkel__DrawMessages + 0x569 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(steam_CDarkel__DrawMessages + 0x91 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CDarkel__DrawMessages + 0x152 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CDarkel__DrawMessages + 0x254 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CDarkel__DrawMessages + 0x347 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CDarkel__DrawMessages + 0x3B4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CDarkel__DrawMessages + 0x453 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CDarkel__DrawMessages + 0x4BD + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CDarkel__DrawMessages + 0x553 + 0x2, &fWideScreenHeightScaleDown, true);




		injector::WriteMemory(steam_CGarages__PrintMessages + 0x45 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(steam_CGarages__PrintMessages + 0x2F + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CGarages__PrintMessages + 0x126 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CGarages__PrintMessages + 0x1B4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CGarages__PrintMessages + 0x208 + 0x2, &fWideScreenHeightScaleDown, true);




		injector::WriteMemory(0x48F339 + 0x94 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_debug_internal_something + 0xFD + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x492C92 + 0x653 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager_PrintStringMenu + 0x124 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x4D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x12B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x178 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x1D2 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x29F + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x2E8 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x340 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x412 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x45A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x4B3 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x581 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x5CA + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x622 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x6F3 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x73D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x79B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x86B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x8B5 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x91E + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x9F4 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0xA3C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0xAA8 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0xB78 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0xBC2 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0xC2C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0xCFA + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0xD44 + 0x2, &fWideScreenWidthScaleDown, true);

		/*injector::WriteMemory(0x49DDFF + 0x2C7 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x140C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x146A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x14C4 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x1524 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x18CB + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x1929 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x1983 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x19E3 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x1AB4 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x1B12 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x1B6C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x1BCC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x1D0C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x24A8 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x29D1 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x2F0D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x3449 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x39A9 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x3ECF + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(0x4A1FEC + 0x648 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0x699 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0x744 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0x794 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0x83E + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0x890 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0x947 + 0x2, &fWideScreenWidthScaleDown, true); Menu Background and Text
		injector::WriteMemory(0x4A1FEC + 0x999 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0xBD0 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0xC21 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0xCCC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0xD1C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0xDC9 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0xE1B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0xED8 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0xF2A + 0x2, &fWideScreenWidthScaleDown, true);*/

		injector::WriteMemory(0x49057B + 0x4A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49057B + 0x4CC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49057B + 0x4F6 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49057B + 0x67E + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49057B + 0x6A8 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49057B + 0xE33 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__DrawContollerScreenExtraText + 0x116 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__DrawContollerScreenExtraText + 0x1A1 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__DrawContollerScreenExtraText + 0x44E + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_debug_internal_something + 0xB68 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_debug_internal_something + 0xBBD + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_debug_internal_something + 0xE91 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_debug_internal_something + 0xFF0 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_debug_internal_something + 0x12AB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492C92 + 0xC6B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492C92 + 0xCBC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492C92 + 0xF35 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492C92 + 0xFF6 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492C92 + 0x11F7 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492C92 + 0x124D + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492C92 + 0x1332 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492C92 + 0x138E + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492C92 + 0x1DAC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492C92 + 0x23B8 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492C92 + 0x24A0 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492C92 + 0x25C9 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492C92 + 0x2608 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x492C92 + 0x2796 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49589B + 0xA10 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49589B + 0xA6B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4964CA + 0x14A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4964CA + 0x193 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49A28E + 0x83 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49A28E + 0x211 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x89 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0xFA + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x154 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x207 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x274 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x2C2 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x375 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x3E4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x434 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x4E9 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x556 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x5A5 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x658 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x6C8 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x717 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x7D1 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x841 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x88F + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x954 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0x9C9 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0xA18 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0xADE + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0xB4D + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0xB9C + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0xC62 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0xCD1 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMenuManager__PrintMap + 0xD1E + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49B892 + 0x3BF + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49B892 + 0x41F + 0x2, &fWideScreenHeightScaleDown, true);
		/*injector::WriteMemory(0x49DDFF + 0x8B0 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x92F + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x12FA + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x143E + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x1496 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x14F4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x154C + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x18FD + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x1955 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x19B3 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x1A0B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x1AE6 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x1B3E + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x1B9C + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x1BF4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x1D49 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x1E16 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x1E78 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x49DDFF + 0x1EF2 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0x674 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0x6B9 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0x76E + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0x7B3 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0x86A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0x8B0 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0x973 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0x9B9 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0xBFC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0xC41 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0xCF6 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0xD3B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0xDF5 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0xE3B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0xF04 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A1FEC + 0xF4A + 0x2, &fWideScreenHeightScaleDown, true);*/




		injector::WriteMemory(steam_Render2DStuff + 0x287 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_Render2DStuff + 0x2DB + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_LoadingScreen + 0x118 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_LoadingScreen + 0x1DD + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_LoadingScreen + 0x290 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_LoadingScreen + 0x328 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(steam_Render2DStuff + 0x11A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_Render2DStuff + 0x164 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_Render2DStuff + 0x1D3 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_Render2DStuff + 0x21D + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_LoadingScreen + 0x147 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_LoadingScreen + 0x206 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_LoadingScreen + 0x2B9 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_LoadingScreen + 0x312 + 0x2, &fWideScreenHeightScaleDown, true);




		injector::WriteMemory(steam_CRadar__DrawRadarMask + 0xF8 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawRadarMask + 0x13E + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawRadarMask + 0x23D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawRadarMask + 0x282 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawRadarSection + 0x382 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawRadarSection + 0x3C1 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawYouAreHereSprite + 0xB + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawYouAreHereSprite + 0x308 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawRadarSprite + 0x9 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawRotatingRadarSprite + 0x1A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__ShowRadarTraceWithHeight + 0xD2 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__ShowRadarTraceWithHeight + 0x1A3 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__ShowRadarTraceWithHeight + 0x255 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__ShowRadarTraceWithHeight + 0x330 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__ShowRadarTraceWithHeight + 0x3B6 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__ShowRadarTraceWithHeight + 0x43D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawBlips + 0x9B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawBlips + 0xE4 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawBlips + 0x2D6 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawBlips + 0x31F + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawBlips + 0x70D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawBlips + 0x752 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawLegend + 0x6A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawLegend + 0x1C2 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawLegend + 0x279 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawLegend + 0x348 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawLegend + 0x3F3 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawLegend + 0x497 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawLegend + 0x51D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawLegend + 0x733 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawEntityBlip + 0x2BF + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawEntityBlip + 0x307 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawCoordBlip + 0x25F + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawCoordBlip + 0x2A7 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(steam_CRadar__DrawRadarMask + 0x124 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawRadarMask + 0x170 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawRadarMask + 0x269 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawRadarMask + 0x2B7 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawRadarSection + 0x3A1 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawRadarSection + 0x3FC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawYouAreHereSprite + 0x45 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawYouAreHereSprite + 0x2F2 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawRadarSprite + 0x47 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawRotatingRadarSprite + 0x4D + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__ShowRadarTraceWithHeight + 0x72 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__ShowRadarTraceWithHeight + 0x14B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__ShowRadarTraceWithHeight + 0x217 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__ShowRadarTraceWithHeight + 0x2F2 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawBlips + 0xCB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawBlips + 0x11C + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawBlips + 0x300 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawBlips + 0x351 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawBlips + 0x739 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawBlips + 0x786 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawLegend + 0x19C + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawLegend + 0x251 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawLegend + 0x322 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawLegend + 0x3CB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawLegend + 0x4C2 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawLegend + 0x549 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawLegend + 0x715 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawEntityBlip + 0x2ED + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawEntityBlip + 0x33B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawCoordBlip + 0x28D + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CRadar__DrawCoordBlip + 0x2DB + 0x2, &fWideScreenHeightScaleDown, true);






		//injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x8E + 0x2, &fWideScreenWidthScaleDown, true);
		//injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x10C + 0x2, &fWideScreenWidthScaleDown, true);
		//injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x128 + 0x2, &fWideScreenWidthScaleDown, true);
		//injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x1EC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x31C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x343 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x433 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x45A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x716 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x740 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x870 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x89C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x908 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x944 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0xBF3 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x358 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x43C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x559 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x67D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x87A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x91E + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x9C2 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0xD90 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0xECB + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0xFDC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x12A1 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x138B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1431 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1458 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x15F2 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1691 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x18DD + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x193A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x19B9 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1A1B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1B4D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1BAA + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1C29 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1CAE + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1D1A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1E98 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2011 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2108 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x219A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x24EF + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x251F + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x256D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x268F + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x29C1 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x29F1 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2A3F + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2B5B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2BBB + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2CBE + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2DB6 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2E39 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2EAA + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2F68 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3050 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x308B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3159 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3218 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x32D8 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3371 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3429 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x34E8 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3680 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x37B9 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x387E + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x38FC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3918 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x39DD + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3C02 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3C38 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3D00 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3D2D + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3DB6 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x41BF + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x41EF + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x4247 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x433C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x43FF + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x442C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x4454 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x484C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x487C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x497F + 0x2, &fWideScreenWidthScaleDown, true);

		//injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x72 + 0x2, &fWideScreenHeightScaleDown, true);
		//injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x1BF + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x306 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x3B7 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x41D + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x4CE + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x6F8 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x7B4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x85B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x887 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x8EA + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0x926 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_steam_CHud__DrawAfterFade + 0xBCD + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3B6 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x49A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x5B1 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x6E2 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x849 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x932 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x9D6 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0xD6F + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0xEB5 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0xFBB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1254 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x13BC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x141B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x15D1 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x167B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x18BC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1998 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1A05 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1B2C + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1C08 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1C98 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1E73 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1FFB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x20E7 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2178 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x24DA + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x250A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2548 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x266A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x29AC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x29DC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2A1A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2B36 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2BA5 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2C9D + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2DA0 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2E23 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2E89 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2F46 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x303A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3114 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x319B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3413 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x34A7 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x363A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3778 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3862 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x39AF + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3C22 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3C64 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3CEA + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3D81 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x41A4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x41DA + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x4311 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x43EA + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x4417 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x46EB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x4837 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x4867 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x4959 + 0x2, &fWideScreenHeightScaleDown, true);



		injector::WriteMemory(steam_CMBlur__AddRenderFx + 0x14A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CMBlur__AddRenderFx + 0x330 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(steam_CMBlur__AddRenderFx + 0x120 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CMBlur__AddRenderFx + 0x306 + 0x2, &fWideScreenHeightScaleDown, true);




		injector::WriteMemory(steam_CParticle__Render + 0x3A2 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CParticle__Render + 0x523 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CParticle__Render + 0x648 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CParticle__Render + 0x773 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CParticle__Render + 0x80C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CParticle__Render + 0x8A6 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CParticle__Render + 0xDA8 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CParticle__Render + 0xE1C + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(steam_CParticle__Render + 0x3E0 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CParticle__Render + 0x561 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CParticle__Render + 0x68A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CParticle__Render + 0x7B9 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CParticle__Render + 0x84C + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CParticle__Render + 0x8E6 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CParticle__Render + 0xE9F + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CParticle__Render + 0xF16 + 0x2, &fWideScreenHeightScaleDown, true);



		injector::WriteMemory(steam_CSpecialFX__Render2DFXs + 0x30 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CSpecialFX__Render2DFXs + 0x3B8 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(steam_CSpecialFX__Render2DFXs + 0x1A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CSpecialFX__Render2DFXs + 0x3A2 + 0x2, &fWideScreenHeightScaleDown, true);



		injector::WriteMemory(steam_cMusicManager__DisplayRadioStationName + 0x27C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_cMusicManager__DisplayRadioStationName + 0x2AB + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory<float>(0x6CC9B0, 0.875f, true);
		injector::WriteMemory<float>(0x6CC9B8, 0.518f, true);
		injector::WriteMemory<float>(0x6CC9C4, 1.0f, true);

		injector::WriteMemory(steam_cMusicManager__DisplayRadioStationName + 0x266 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_cMusicManager__DisplayRadioStationName + 0x2E7 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_cMusicManager__DisplayRadioStationName + 0x380 + 0x2, &fWideScreenHeightScaleDown, true);




		injector::WriteMemory(steam_CSceneEdit__Draw + 0x71 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CSceneEdit__Draw + 0x134 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CSceneEdit__Draw + 0x1AF + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CSceneEdit__Draw + 0x24B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CSceneEdit__Draw + 0x2C5 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CSceneEdit__Draw + 0x3D2 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CSceneEdit__Draw + 0x487 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(steam_CSceneEdit__Draw + 0x5B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CSceneEdit__Draw + 0x114 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CSceneEdit__Draw + 0x195 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CSceneEdit__Draw + 0x22B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CSceneEdit__Draw + 0x2AB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CSceneEdit__Draw + 0x3AA + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CSceneEdit__Draw + 0x465 + 0x2, &fWideScreenHeightScaleDown, true);




		//injector::WriteMemory(steam_CRunningScript__ProcessCommands1100To1199 + 0x423 + 0x2, &fWideScreenWidthScaleDown, true);


		//injector::WriteMemory(steam_CClouds__Render + 0x66B + 0x2, &fWideScreenWidthScaleDown, true);



		injector::WriteMemory(0x61DAF0 + 0xC9 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x61DAF0 + 0x158 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x61DAF0 + 0x202 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x61DAF0 + 0x28C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x61DAF0 + 0x2C1 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x61DAF0 + 0x33E + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(0x61DAF0 + 0xEC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x61DAF0 + 0x17B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x61DAF0 + 0x225 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x61DAF0 + 0x270 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x61DAF0 + 0x328 + 0x2, &fWideScreenHeightScaleDown, true);


		injector::WriteMemory(steam_CReplay__Display + 0x66 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(steam_CReplay__Display + 0x50 + 0x2, &fWideScreenHeightScaleDown, true);


		injector::WriteMemory<double>(0x696AE0, (1.8 / 1.50), true); //h
		injector::WriteMemory<double>(0x696AE8, (1.7 / 1.50), true); //w
		injector::WriteMemory<double>(0x696AF0, (1.5 / 1.50), true); //w

		injector::WriteMemory(steam_CHud__Draw + 0x358 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x43C + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x559 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x67D + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x87A + 0x2, &fCustomWideScreenWidthScaleDown, true);
		//injector::WriteMemory(steam_CHud__Draw + 0x91E + 0x2, &fCustomWideScreenWidthScaleDown, true); //sniper crosshair fix
		injector::WriteMemory(steam_CHud__Draw + 0x9C2 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0xD90 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0xECB + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0xFDC + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x12A1 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x138B + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1431 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1458 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x15F2 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1691 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x18DD + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x193A + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x19B9 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1A1B + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1B4D + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1BAA + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1C29 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1CAE + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1D1A + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1E98 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2011 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2108 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x219A + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x24EF + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x251F + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x256D + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x268F + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x29C1 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x29F1 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2A3F + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2B5B + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2BBB + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2CBE + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2DB6 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2E39 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2EAA + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2F68 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3050 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x308B + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3159 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3218 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x32D8 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3371 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3429 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x34E8 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		//injector::WriteMemory(steam_CHud__Draw + 0x3680 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		//injector::WriteMemory(steam_CHud__Draw + 0x37B9 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x387E + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x38FC + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3918 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x39DD + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3C02 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3C38 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3D00 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3D2D + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3DB6 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x41BF + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x41EF + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x4247 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x433C + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x43FF + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x442C + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x4454 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x484C + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x487C + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x497F + 0x2, &fCustomWideScreenWidthScaleDown, true);


		injector::WriteMemory(steam_CHud__Draw + 0x3B6 + 0x2, &fCrosshairHeightScaleDown, true); // let's make crosshair proportional
		injector::WriteMemory(steam_CHud__Draw + 0x49A + 0x2, &fCrosshairHeightScaleDown, true); // let's make crosshair proportional
		injector::WriteMemory(steam_CHud__Draw + 0x5B1 + 0x2, &fCrosshairHeightScaleDown, true); // let's make crosshair proportional
		injector::WriteMemory(steam_CHud__Draw + 0x6E2 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x849 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		//injector::WriteMemory(steam_CHud__Draw + 0x932 + 0x2, &fCustomWideScreenHeightScaleDown, true); //sniper crosshair fix
		injector::WriteMemory(steam_CHud__Draw + 0x9D6 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0xD6F + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0xEB5 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0xFBB + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1254 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x13BC + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x141B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x15D1 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x167B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x18BC + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1998 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1A05 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1B2C + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1C08 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1C98 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1E73 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x1FFB + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x20E7 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2178 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x24DA + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x250A + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2548 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x266A + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x29AC + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x29DC + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2A1A + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2B36 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2BA5 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2C9D + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2DA0 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2E23 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2E89 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x2F46 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x303A + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3114 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x319B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3413 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x34A7 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		//injector::WriteMemory(steam_CHud__Draw + 0x363A + 0x2, &fCustomWideScreenHeightScaleDown, true);
		//injector::WriteMemory(steam_CHud__Draw + 0x3778 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3862 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x39AF + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3C22 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3C64 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3CEA + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x3D81 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x41A4 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x41DA + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x4311 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x43EA + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x4417 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x46EB + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x4837 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x4867 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(steam_CHud__Draw + 0x4959 + 0x2, &fCustomWideScreenHeightScaleDown, true);
	#pragma endregion HUDFunctionsSteam
	}
}

void IVRadar()
{
	fCustomRadarWidthIV = 102.0f;
	fCustomRadarHeightIV = 79.0f;
	fCustomRadarPosXIV = 109.0f;
	fCustomRadarPosYIV = 107.0f;
	fCustomRadarRingWidthIV = 101.0f;
	fCustomRadarRingHeightIV = 83.0f;
	fCustomRadarRingPosXIV = 98.0f;
	fCustomRadarRingPosYIV = 109.5f;
	fCustomRadarRingPosXIV2 = fCustomRadarRingPosXIV - 19.0f;

	fCustomRadarPosXIV = 109.0f * ((float)RsGlobal->MaximumWidth * (1.0f / 1920.0f));
	fCustomRadarRingPosXIV = 98.0f * ((float)RsGlobal->MaximumWidth * (1.0f / 1920.0f));
	fCustomRadarRingPosXIV2 = 116.0f * ((float)RsGlobal->MaximumWidth * (1.0f / 1920.0f));

	fPlayerMarkerPos = fCustomRadarRingWidthIV * fRadarWidthScale;

	injector::WriteMemory<float>(0x68FD24, fCustomRadarWidthIV, true);
	injector::WriteMemory<float>(0x68FD30, fCustomRadarHeightIV, true);
	injector::WriteMemory<float>(0x68FD2C, fCustomRadarPosXIV); injector::WriteMemory(0x4C2996 + 0x2, 0x690238, true); //+draw you are here sprite fix
	injector::WriteMemory<float>(0x68FD34, fCustomRadarPosYIV); injector::WriteMemory(0x4C29CB + 0x2, 0x690238, true);

	injector::WriteMemory(0x55A9A6 + 0x2, &fCustomRadarRingWidthIV, true);
	injector::WriteMemory(0x55AADF + 0x2, &fCustomRadarRingWidthIV, true);
	injector::WriteMemory(0x55A9BF + 0x2, &fCustomRadarRingHeightIV, true);
	injector::WriteMemory(0x55AAF8 + 0x2, &fCustomRadarRingHeightIV, true);
	injector::WriteMemory<float>(0x55A956, fCustomRadarRingPosXIV, true);
	injector::WriteMemory(0x55A9AC + 0x2, &fCustomRadarRingPosXIV2, true);
	injector::WriteMemory<float>(0x55AA94, fCustomRadarRingPosXIV, true);
	injector::WriteMemory(0x55AAE5 + 0x2, &fCustomRadarRingPosXIV2, true);

	injector::WriteMemory<float>(0x697C18, fCustomRadarRingPosYIV, true);
}

template<uintptr_t addr>
void TextDrawOutlineHook()
{
	using func_hook = injector::function_hooker<addr, void(float, float, unsigned short*)>;
	injector::make_static_hook<func_hook>([](func_hook::func_type PrintString, float PosX, float PosY, unsigned short* c)
	{
		*(short*)0x97F860 = 1;
		*(short*)0x97F865 = 0xFF;
		PrintString(PosX, PosY, c);
		return;
	});
}

unsigned int originalColor, jmpAddr;
void __declspec(naked)GetTextOriginalColor()
{
	__asm
	{
		fadd    dword ptr ds : [esp + 38h]
		fstp    dword ptr ds : [esp]
		mov jmpAddr, eax
		mov eax, [esp + 24h]
		mov originalColor, eax
		mov eax, jmpAddr

		mov jmpAddr, 0x551853
		jmp jmpAddr
	}
}

template<uintptr_t addr>
void TextDrawOutlineHookShadow()
{
	using func_hook = injector::function_hooker<addr, void(float, float, unsigned int, unsigned short *, unsigned short *, float)>;
	injector::make_static_hook<func_hook>([](func_hook::func_type PrintString, float PosX, float PosY, unsigned int c, unsigned short *d, unsigned short *e, float f)
	{
		CRGBA rgba;
		unsigned char originalColorA = (originalColor >> 24) & 0xFF;
		unsigned char originalColorB = (originalColor >> 16) & 0xFF;
		unsigned char originalColorG = (originalColor >> 8) & 0xFF;
		unsigned char originalColorR = originalColor & 0xFF;
		if (!originalColorR && !originalColorG && !originalColorB)
		{
			injector::thiscall<void(CRGBA*, uint8_t, uint8_t, uint8_t, uint8_t)>::call<0x541570>(&rgba, 0xFF, 0xFF, 0xFF, originalColorA / 10);
			injector::cstd<void(CRGBA*)>::call<0x550170>(&rgba);
		}

		PosX -= 1.0f; PosY -= 1.0f;

		PrintString(PosX + 1.0f, PosY, c, d, e, f);
		PrintString(PosX - 1.0f, PosY, c, d, e, f);
		PrintString(PosX, PosY + 1.0f, c, d, e, f);
		PrintString(PosX, PosY - 1.0f, c, d, e, f);
		PrintString(PosX + 1.0f, PosY + 1.0f, c, d, e, f);
		PrintString(PosX - 1.0f, PosY + 1.0f, c, d, e, f);
		PrintString(PosX + 1.0f, PosY - 1.0f, c, d, e, f);
		PrintString(PosX - 1.0f, PosY - 1.0f, c, d, e, f);

		*(unsigned char*)0x97F823 = originalColorA / 2;
		PrintString(PosX - 1.0f - 1.0f, PosY, c, d, e, f);
		PrintString(PosX, PosY + 1.0f + 1.0f, c, d, e, f);
		*(unsigned char*)0x97F823 = originalColorA;

		if (ReplaceTextShadowWithOutline > 1)
		{
			PrintString(PosX + 2.0f, PosY, c, d, e, f);
			PrintString(PosX - 2.0f, PosY, c, d, e, f);
			PrintString(PosX, PosY + 2.0f, c, d, e, f);
			PrintString(PosX, PosY - 2.0f, c, d, e, f);
			PrintString(PosX + 2.0f, PosY + 2.0f, c, d, e, f);
			PrintString(PosX - 2.0f, PosY + 2.0f, c, d, e, f);
			PrintString(PosX + 2.0f, PosY - 2.0f, c, d, e, f);
			PrintString(PosX - 2.0f, PosY - 2.0f, c, d, e, f);

			*(unsigned char*)0x97F823 = originalColorA / 2;
			PrintString(PosX - 2.0f - 1.0f, PosY, c, d, e, f);
			PrintString(PosX, PosY + 2.0f + 1.0f, c, d, e, f);
			*(unsigned char*)0x97F823 = originalColorA;
		}
		return;
	});
}

template<uintptr_t addr>
void CSprite2dDrawHook()
{
	using func_hook = injector::function_hooker_thiscall<addr, void(void*, CRect const&, CRGBA const&)>;
	injector::make_static_hook<func_hook>([](func_hook::func_type CSprite2dDraw, void* _this, CRect const& rect, CRGBA const& rgba)
	{
		CSprite2dDraw(_this, rect, CRGBA(0xFF, 0xFF, 0xFF, 0x0));
	});
}

template<uintptr_t addr>
void Render2DStuffHook()
{
	using func_hook = injector::function_hooker<addr, void()>;
	injector::make_static_hook<func_hook>([](func_hook::func_type funcRender2DStuff)
	{
		if (*(bool*)0x869668 == 0) //bMenuVisible
			funcRender2DStuff();
		return;
	});
}

void ApplyIniOptions()
{
	CIniReader iniReader("");
	fHudWidthScale = iniReader.ReadFloat("MAIN", "HudWidthScale", 0.62221788786f);
	fHudHeightScale = iniReader.ReadFloat("MAIN", "HudHeightScale", 0.66666670937f);
	fRadarWidthScale = iniReader.ReadFloat("MAIN", "RadarWidthScale", 0.80354591724f);
	fSubtitlesScale = iniReader.ReadFloat("MAIN", "SubtitlesScale", 1.0f);
	fMVLScopeFix = iniReader.ReadFloat("MAIN", "MVLScopeFix", 0.0f);
	int SmallerTextShadows = iniReader.ReadInteger("MAIN", "SmallerTextShadows", 1);
	RestoreCutsceneFOV = iniReader.ReadInteger("MAIN", "RestoreCutsceneFOV", 1);
	fCarSpeedDependantFOV = iniReader.ReadFloat("MAIN", "CarSpeedDependantFOV", 0.0f);
	DontTouchFOV = iniReader.ReadInteger("MAIN", "DontTouchFOV", 0);

	szForceAspectRatio = iniReader.ReadString("MAIN", "ForceAspectRatio", "auto");
	if (strncmp(szForceAspectRatio, "auto", 4) != 0)
	{
		AspectRatioWidth = std::stoi(szForceAspectRatio);
		AspectRatioHeight = std::stoi(strchr(szForceAspectRatio, ':') + 1);
	}

	szFOVControl = iniReader.ReadString("MAIN", "FOVControl", ""); //0x40104A
	sscanf_s(szFOVControl, "%X", &FOVControl);
	injector::WriteMemory<float>(FOVControl, 1.0f, true);

	HideAABug = iniReader.ReadInteger("MAIN", "HideAABug", 1);
	SmartCutsceneBorders = iniReader.ReadInteger("MAIN", "SmartCutsceneBorders", 1);
	NoLoadingBarFix = iniReader.ReadInteger("LCS", "NoLoadingBarFix", 0);
	IVRadarScaling = iniReader.ReadInteger("MAIN", "IVRadarScaling", 0);
	ReplaceTextShadowWithOutline = iniReader.ReadInteger("MAIN", "ReplaceTextShadowWithOutline", 0);
	bool bTransparentMenu = iniReader.ReadInteger("MAIN", "TransparentMenu", 0) == 1;

	if (!fHudWidthScale || !fHudHeightScale) { fHudWidthScale = 0.62221788786f; fHudHeightScale = 0.66666670937f; }
	if (!fRadarWidthScale) { fRadarWidthScale = 0.80354591724f; }
	if (!fSubtitlesScale) { fSubtitlesScale = 1.0f; }

	fPlayerMarkerPos = 94.0f * fRadarWidthScale;

	if (!injector::address_manager::singleton().IsSteam())
	{
		szForceAspectRatio = iniReader.ReadString("MAIN", "ForceMultisamplingLevel", "");
		if (strncmp(szForceAspectRatio, "max", 3) != 0)
		{
			SelectedMultisamplingLevels = iniReader.ReadInteger("MAIN", "ForceMultisamplingLevel", 0);
			injector::WriteMemory(0x65C321 + 0x1, &SelectedMultisamplingLevels, true);
		}
		else
		{
			injector::WriteMemory(0x65C321 + 0x1, (void*)0x6DDE1C, true);
		}

		if (iniReader.ReadInteger("MAIN", "FixVehicleLights", 1))
		{
			injector::WriteMemory(0x00543B92, 0x0067E188, true); //lamp corona stretch
			injector::WriteMemory(0x00543A52, 0x00696AA0, true); //car lights stretch
			injector::WriteMemory<float>(0x69590C, 3.0f, true); //car lights stretch
		}

		if (SmallerTextShadows)
		{
			injector::MakeInline<0x54FF20, 0x54FF2A>([](injector::reg_pack& regs)
			{
				short Size = *(short*)(regs.esp + 4);
				if (Size > 1)
					*(short*)0x97F860 = Size - 1;
				else
					*(short*)0x97F860 = Size;
			});

			injector::WriteMemory<float>(0x6874E0, 0.8f, true);
			injector::WriteMemory<float>(0x6874E8, 0.93f, true);
			injector::WriteMemory<double>(0x6874F0, 0.5f, true);
		}

		if (fSubtitlesScale)
		{
			injector::WriteMemory<float>(0x697A80, 1.2f * fSubtitlesScale, true);
			injector::WriteMemory<float>(0x697C28, 0.57999998f * fSubtitlesScale, true);
		}

		if (fRadarWidthScale)
		{
			injector::WriteMemory(CRadar__DrawRadarMask + 0xF8 + 0x2, &fCustomRadarWidthScale, true);
			injector::WriteMemory(CRadar__DrawRadarMask + 0x13E + 0x2, &fCustomRadarWidthScale, true);
			injector::WriteMemory(CRadar__DrawRadarMask + 0x23D + 0x2, &fCustomRadarWidthScale, true);
			injector::WriteMemory(CRadar__DrawRadarMask + 0x282 + 0x2, &fCustomRadarWidthScale, true);
			injector::WriteMemory(CRadar__DrawRadarSection + 0x382 + 0x2, &fCustomRadarWidthScale, true);
			injector::WriteMemory(CRadar__DrawRadarSection + 0x3C1 + 0x2, &fCustomRadarWidthScale, true);

			injector::WriteMemory(CRadar__DrawRotatingRadarSprite + 0x1A + 0x2, &fCustomRadarWidthScale, true);

			injector::WriteMemory(CRadar__DrawBlips + 0xF4 + 0x2 + 0x6, &fPlayerMarkerPos, true);
			injector::WriteMemory(CRadar__DrawBlips + 0x2E6 + 0x2 + 0x6, &fPlayerMarkerPos, true);
			//injector::WriteMemory(CRadar__DrawBlips + 0x32F + 0x2 + 0x6, &fPlayerMarkerPos, true);

			injector::WriteMemory(CRadar__DrawBlips + 0x32F + 0x2, &fCustomRadarWidthScale, true);
			injector::WriteMemory(CRadar__DrawCoordBlip + 0x2A7 + 0x2, &fCustomRadarWidthScale, true);

			injector::WriteMemory(CRadar__DrawEntityBlip + 0x307 + 0x2 + 0x6, &fPlayerMarkerPos, true);

			injector::WriteMemory(CHud__Draw + 0x3680 + 0x2, &fCustomRadarWidthScale, true);
			injector::WriteMemory(CHud__Draw + 0x37B9 + 0x2, &fCustomRadarWidthScale, true);
		}

		if (IVRadarScaling)
		{
			IVRadar();
		}

		if (HideAABug)
		{
			injector::MakeJMP(0x57FAA0, Hide1pxAABug, true);
		}

		if (SmartCutsceneBorders)
		{
			injector::MakeCALL(0x4A61EE, CCamera::DrawBordersForWideScreen, true);
			injector::MakeCALL(0x54A223, CCamera::DrawBordersForWideScreen, true);
		}

		if (ReplaceTextShadowWithOutline)
		{
			injector::MakeInline<0x54FF20, 0x54FF2A>([](injector::reg_pack& )
			{
				*(short*)0x97F860 = 1;
			});
			injector::MakeJMP(0x55184C, GetTextOriginalColor, true);
			TextDrawOutlineHookShadow<(0x551853)>();

			//textbox
			injector::WriteMemory(0x55B59B, 0xFFFF4B31, true); //background 
			injector::WriteMemory(0x55B5A0, 0xFFFF4AEC, true); //enable shadow
			injector::WriteMemory<char>(0x55B5A5, 0x01, true); //shadow size

			injector::WriteMemory<char>(0x4A35A2 + 0x1, 0x00, true); // cursor shadow alpha

			injector::WriteMemory(0x5FA1A5, 0, true); //radio shadow
			TextDrawOutlineHook<(0x5FA28A)>(); // = 0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *) radio text

			injector::MakeNOP(0x49E30E, 5, true); //menu title shadows
		}
		TextDrawOutlineHook<(0x55B113)>(); // = 0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *) subtitles

		if (bTransparentMenu)
		{
			injector::MakeInline<0x600416, 0x60041D>([](injector::reg_pack&)
			{
				injector::WriteMemory<char>(0x86969C, 0, true); // bGameStarted

				injector::MakeNOP(0x4A73C9, 5, true);
				injector::MakeNOP(0x4A21E0, 5, true);

				injector::MakeNOP(0x4A5E27, 6, true);

				injector::MakeNOP(0x4A212D + 0x704, 5, true); //        CSprite2d::Draw2DPolygon(float, float, float, float, float, float, float, float, CRGBA const&)
				injector::MakeNOP(0x4A212D + 0x7FE, 5, true); //        CSprite2d::Draw2DPolygon(float, float, float, float, float, float, float, float, CRGBA const&)
				injector::MakeNOP(0x4A212D + 0x907, 5, true); //        CSprite2d::Draw2DPolygon(float, float, float, float, float, float, float, float, CRGBA const&)
				injector::MakeNOP(0x4A212D + 0xC8C, 5, true); //        CSprite2d::Draw2DPolygon(float, float, float, float, float, float, float, float, CRGBA const&)
				injector::MakeNOP(0x4A212D + 0xD86, 5, true); //        CSprite2d::Draw2DPolygon(float, float, float, float, float, float, float, float, CRGBA const&)
				injector::MakeNOP(0x4A212D + 0xE95, 5, true); //        CSprite2d::Draw2DPolygon(float, float, float, float, float, float, float, float, CRGBA const&)
				injector::MakeNOP(0x4A212D + 0xFA4, 5, true); //        CSprite2d::Draw2DPolygon(float, float, float, float, float, float, float, float, CRGBA const&)

				CSprite2dDrawHook<(0x4A212D + 0x5FD)>(); //                                      call    _ZN9CSprite2d4DrawERK5CRectRK5CRGBA; CSprite2d::Draw(CRect const&,CRGBA const&)
				CSprite2dDrawHook<(0x4A212D + 0xB85)>(); //                                      call    _ZN9CSprite2d4DrawERK5CRectRK5CRGBA; CSprite2d::Draw(CRect const&,CRGBA const&)
				CSprite2dDrawHook<(0x4A212D + 0x13D6)>(); //                                      call    _ZN9CSprite2d4DrawERK5CRectRK5CRGBA; CSprite2d::Draw(CRect const&,CRGBA const&)
				CSprite2dDrawHook<(0x4A212D + 0x1589)>(); //                                      call    _ZN9CSprite2d4DrawERK5CRectRK5CRGBA; CSprite2d::Draw(CRect const&,CRGBA const&)
				//CSprite2dDrawHook<(0x4A212D+0x1666)>(); //                                      call    _ZN9CSprite2d4DrawERK5CRectRK5CRGBA; CSprite2d::Draw(CRect const&,CRGBA const&)*/


				Render2DStuffHook<(0x4A608E)>();
			});
		}
	}
	else
	{
		#pragma region SteamINI
		szForceAspectRatio = iniReader.ReadString("MAIN", "ForceMultisamplingLevel", "");
		if (strncmp(szForceAspectRatio, "max", 3) != 0)
		{
			SelectedMultisamplingLevels = iniReader.ReadInteger("MAIN", "ForceMultisamplingLevel", 0);
			injector::WriteMemory(0x65B2D1 + 0x1, &SelectedMultisamplingLevels, true);
		}
		else
		{
			injector::WriteMemory(0x65B2D1 + 0x1, (void*)0x6DCDB4, true);
		}

		if (iniReader.ReadInteger("MAIN", "FixVehicleLights", 1))
		{
			injector::WriteMemory(0x543A82, 0x67D188, true); //lamp corona stretch
			injector::WriteMemory(0x543942, 0x695AA8, true); //car lights stretch
			injector::WriteMemory<float>(0x694914, 3.0f, true); //car lights stretch
		}
		
		if (SmallerTextShadows)
		{
			injector::MakeInline<0x54FE10, 0x54FE1A>([](injector::reg_pack& regs)
			{
				short Size = *(short*)(regs.esp + 4);
				if (Size > 1)
					*(short*)0x97E868 = Size - 1;
				else
					*(short*)0x97E868 = Size;
			});

			injector::WriteMemory<float>(0x6864E0, 0.8f, true);
			injector::WriteMemory<float>(0x6864E8, 0.93f, true);
			injector::WriteMemory<double>(0x6864F0, 0.5f, true);
		}
		
		if (fSubtitlesScale)
		{
			injector::WriteMemory<float>(0x696A88, 1.2f * fSubtitlesScale, true);
			injector::WriteMemory<float>(0x696C30, 0.57999998f * fSubtitlesScale, true);
		}
		
		if (fRadarWidthScale)
		{
			injector::WriteMemory(steam_CRadar__DrawRadarMask + 0xF8 + 0x2, &fCustomRadarWidthScale, true);
			injector::WriteMemory(steam_CRadar__DrawRadarMask + 0x13E + 0x2, &fCustomRadarWidthScale, true);
			injector::WriteMemory(steam_CRadar__DrawRadarMask + 0x23D + 0x2, &fCustomRadarWidthScale, true);
			injector::WriteMemory(steam_CRadar__DrawRadarMask + 0x282 + 0x2, &fCustomRadarWidthScale, true);
			injector::WriteMemory(steam_CRadar__DrawRadarSection + 0x382 + 0x2, &fCustomRadarWidthScale, true);
			injector::WriteMemory(steam_CRadar__DrawRadarSection + 0x3C1 + 0x2, &fCustomRadarWidthScale, true);

			injector::WriteMemory(steam_CRadar__DrawRotatingRadarSprite + 0x1A + 0x2, &fCustomRadarWidthScale, true);

			injector::WriteMemory(steam_CRadar__DrawBlips + 0xE4 + 0x2 + 0x6, &fPlayerMarkerPos, true);
			injector::WriteMemory(steam_CRadar__DrawBlips + 0x2D6 + 0x2 + 0x6, &fPlayerMarkerPos, true);
			//injector::WriteMemory(steam_CRadar__DrawBlips + 0x32F + 0x2 + 0x6, &fPlayerMarkerPos, true);

			injector::WriteMemory(steam_CRadar__DrawBlips + 0x31F + 0x2, &fCustomRadarWidthScale, true);
			injector::WriteMemory(steam_CRadar__DrawCoordBlip + 0x2A7 + 0x2, &fCustomRadarWidthScale, true);

			injector::WriteMemory(steam_CRadar__DrawEntityBlip + 0x307 + 0x2 + 0x6, &fPlayerMarkerPos, true);

			injector::WriteMemory(steam_CHud__Draw + 0x3680 + 0x2, &fCustomRadarWidthScale, true);
			injector::WriteMemory(steam_CHud__Draw + 0x37B9 + 0x2, &fCustomRadarWidthScale, true);
		}

		/*if (IVRadarScaling)
		{
			IVRadar();
		}*/
		
		if (HideAABug)
		{
			injector::MakeJMP(0x57F8D0, Hide1pxAABug, true);
		}

		if (SmartCutsceneBorders)
		{
			injector::MakeCALL(0x4A60BE, CCamera::DrawBordersForWideScreen, true);
			injector::MakeCALL(0x54A113, CCamera::DrawBordersForWideScreen, true);
		}

		/*if (ReplaceTextShadowWithOutline)
		{
			injector::MakeInline<0x54FE10, 0x54FE1A>([](injector::reg_pack&)
			{
				*(short*)0x97E868 = 1;
			});
			injector::MakeJMP(0x55184C, GetTextOriginalColor, true);
			TextDrawOutlineHookShadow<(0x551853)>();

			//textbox
			injector::WriteMemory(0x55B59B, 0xFFFF4B31, true); //background 
			injector::WriteMemory(0x55B5A0, 0xFFFF4AEC, true); //enable shadow
			injector::WriteMemory<char>(0x55B5A5, 0x01, true); //shadow size

			injector::WriteMemory(0x5FA1A5, 0, true); //radio shadow
			TextDrawOutlineHook<(0x5FA28A)>(); // = 0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *) radio text
		}
		TextDrawOutlineHook<(0x55B113)>(); // = 0x551040 + 0x0  -> call    _ZN5CFont11PrintStringEffPt; CFont::PrintString(float,float,ushort *) subtitles*/
		#pragma endregion SteamINI
	}
}

DWORD WINAPI CompatHandler(LPVOID)
{
	static int i;
	do
	{
		Sleep(100);
		i++;
		if (i > 100)
			break;

	} while (GetModuleHandle("SilentPatchVC.asi") == NULL);

	if (GetModuleHandle("SilentPatchVC.asi"))
	{
		OverwriteResolution();
		injector::WriteMemory<uint8_t>(0x5516FB, 0x89, true);
		injector::WriteMemory(0x5516FC, 0x04DB2404, true);
		injector::WriteMemory(0x5516FC + 4, 0xC81DD824, true);

		injector::WriteMemory<uint8_t>(0x5517C4, 0xDB, true);
		injector::WriteMemory<uint8_t>(0x5517DF, 0xDB, true);
		injector::WriteMemory<uint8_t>(0x551832, 0xDB, true);
		injector::WriteMemory<uint8_t>(0x551848, 0xDB, true);
		injector::WriteMemory<uint8_t>(0x5517E2, 0x18, true);
		injector::WriteMemory<uint8_t>(0x55184B, 0x18, true);
		injector::WriteMemory<uint8_t>(0x5517C7, 0x0C, true);
		injector::WriteMemory<uint8_t>(0x551835, 0x08, true);
	}

	i = 0;
	do
	{
		Sleep(100);
		i++;
		if (i > 100)
			break;

	} while (GetModuleHandle("vehmod_retail.dll") == NULL);

	if (GetModuleHandle("vehmod_retail.dll"))
		injector::WriteMemory((DWORD)GetModuleHandle("vehmod_retail.dll") + 0x55401, &fMVLScopeFix, true);

	return 0;
}

void Init()
{
	if (injector::address_manager::singleton().IsVC())
	{
		if ((injector::address_manager::singleton().GetMajorVersion() == 1 && injector::address_manager::singleton().GetMinorVersion() == 0) || injector::address_manager::singleton().IsSteam())
		{
			if (injector::address_manager::singleton().IsSteam())
			{
				while (true)
				{
					Sleep(0);
					if (*(DWORD*)0x666BA0 == 0x53E58955) break; //GTA_VC_STEAM
				}
			}

			GetMemoryAddresses();

			OverwriteResolution();
			FixAspectRatio();
			FixFOV();
			RsSelectDeviceHook();
			FixCoronas();
			FixBorders();
			FixHUD();
			ApplyIniOptions();

			if (!injector::address_manager::singleton().IsSteam())
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&CompatHandler, NULL, 0, NULL);
		}
	}
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		if (!injector::address_manager::singleton().IsSteam())
			Init();
		else
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
	}
	return TRUE;
}

