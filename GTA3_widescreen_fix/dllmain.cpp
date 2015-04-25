#include "stdafx.h"
#include "..\includes\GTA\common.h"

void GetMemoryAddresses()
{
	if (!injector::address_manager::singleton().IsSteam())
	{
		RsGlobal = (RsGlobalType *)0x94300C;
		CDraw::pfScreenAspectRatio = (float*)0x5F53C0;
		CDraw::pfScreenFieldOfView = (float*)0x5FBC6C;
		CSprite2dDrawRect = (int(__cdecl *)(CRect const &, CRGBA const &)) 0x51F970;
		bWideScreen = 0x95CD23; BordersVar1 = 0x6FADA0; BordersVar2 = 0x6FADB8;
		FindPlayerVehicle = (int(__cdecl *)()) 0x4A10C0;
		IsInCutscene = 0x6FAD68;
	}
	else
	{
		RsGlobal = (RsGlobalType *)0x953304;
		CDraw::pfScreenAspectRatio = (float*)0x6022A8;
		CDraw::pfScreenFieldOfView = (float*)0x608A4C;
		CSprite2dDrawRect = (int(__cdecl *)(CRect const &, CRGBA const &)) 0x51FB30;
		bWideScreen = 0x96D01B; BordersVar1 = 0x70AEE0; BordersVar2 = 0x70AEF8;
		FindPlayerVehicle = (int(__cdecl *)()) 0x4A1140;
		IsInCutscene = 0x70AEA8;
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
			injector::MakeNOP(0x4890FA, 5, true);
			injector::WriteMemory(0x581E5B + 0x3, ResX, true);
			injector::WriteMemory(0x581E64 + 0x4, ResY, true);
			injector::WriteMemory(0x602D3A + 0x6, ResX, true);
			injector::WriteMemory(0x602D44 + 0x6, ResY, true);
			injector::WriteMemory(0x602D4E + 0x6, ResX, true);
			injector::WriteMemory(0x602D58 + 0x6, ResY, true);
			injector::WriteMemory<uchar>(0x581E6E + 0x4, 32, true);
			injector::WriteMemory<uchar>(0x581E9A, 0xEB, true); //jl      short loc_581E40 > jmp      short loc_581E40
			injector::MakeNOP(0x581E37, 5, true);
		}
		else
		{
			injector::MakeNOP(0x48920C, 5, true);
			injector::WriteMemory(0x58208B + 0x3, ResX, true);
			injector::WriteMemory(0x582094 + 0x4, ResY, true);
			injector::WriteMemory<uchar>(0x58209E + 0x4, 32, true);
			injector::WriteMemory<uchar>(0x5820CA, 0xEB, true); //jl      short loc_581E40 > jmp      short loc_581E40
			injector::MakeNOP(0x582067, 5, true);
		}
}

void FixAspectRatio()
{
	if (!injector::address_manager::singleton().IsSteam())
	{
		injector::MakeNOP(0x48D06B, 7, true);
		injector::MakeJMP(0x584B26, CDraw::CalculateAspectRatio, true);
		injector::MakeNOP(0x48D074, 2, true);
	}
	else
	{
		injector::MakeNOP(0x48D0FB, 7, true);
		injector::MakeJMP(0x584D56, CDraw::CalculateAspectRatio, true);
		injector::MakeNOP(0x48D104, 2, true);
	}
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
		injector::MakeCALL(0x46E92E, CDraw::SetFOV, true);
		injector::MakeCALL(0x46EACE, CDraw::SetFOV, true);
		injector::MakeCALL(0x48C0DA, CDraw::SetFOV, true);
		injector::MakeCALL(0x48C50B, CDraw::SetFOV, true);
		injector::MakeCALL(0x59BCA9, CDraw::SetFOV, true);
		injector::MakeCALL(0x59BCCC, CDraw::SetFOV, true);

		injector::MakeInline<0x46EC0D, 0x46EC13>([](injector::reg_pack& regs)
		{
			*(float*)(regs.ebx + 0xF0) = fEmergencyVehiclesFix;
		});
		injector::MakeInline<0x4A42A0, 0x4A42A6>([](injector::reg_pack&)
		{
			_asm
			{
				//fstp    ds : 0x974BEC
				fstp    ds : fRadarScaling
				mov edx, fRadarScaling
				mov ds : [0x8E281C], edx
			}
			fRadarScaling -= 120.0f;
		});
	}
	else
	{
		injector::MakeCALL(0x46D3D0 + 0x153E, CDraw::SetFOV, true);
		injector::MakeCALL(0x46D3D0 + 0x16DE, CDraw::SetFOV, true);
		injector::MakeCALL(0x48BF50 + 0x20A,  CDraw::SetFOV, true);
		injector::MakeCALL(0x48C540 + 0x5B,   CDraw::SetFOV, true);
		injector::MakeCALL(0x599020 + 0x39,   CDraw::SetFOV, true);
		injector::MakeCALL(0x599060 + 0x1C,   CDraw::SetFOV, true);
		injector::MakeInline<0x46EBED, 0x46EBF3>([](injector::reg_pack& regs)
		{
			*(float*)(regs.ebx + 0xF0) = fEmergencyVehiclesFix;
		});
		injector::MakeInline<0x4A4320, 0x4A4326>([](injector::reg_pack&)
		{
			_asm
			{
				//fstp    ds : 0x974BEC
				fstp    ds : fRadarScaling
				mov edx, fRadarScaling
				mov ds : [0x8F290C], edx
			}
			fRadarScaling -= 120.0f;
		});
	}
}

void FixMenu()
{
	if (!injector::address_manager::singleton().IsSteam())
	{
		if (nMenuFix)
		{
			injector::WriteMemory(0x47B174, nMenuAlignment, true);
			injector::WriteMemory(0x47B1E0, nMenuAlignment, true);
			injector::WriteMemory(0x47B33C, nMenuAlignment, true);
			injector::WriteMemory(0x47B3A8, nMenuAlignment, true);
			injector::WriteMemory(0x47B411, nMenuAlignment, true);

			injector::WriteMemory(0x48AC00 + 0x2, &fWideScreenWidthScaleDown, true);
			injector::WriteMemory(0x48B7E0 + 0x19B + 0x2, &fWideScreenWidthScaleDown, true);
	
			injector::WriteMemory(0x48AC40 + 0x2, &fWideScreenHeightScaleDown, true);

			injector::WriteMemory(0x47AA5F + 0x2, &gtaLogo128Coord1, true);
			injector::WriteMemory(0x47AA81 + 0x2, &gtaLogo128Coord2, true);
		}												  
	}													  
	else												  
	{													  
		if (nMenuFix) 
		{									  
			injector::WriteMemory(0x47B244, nMenuAlignment, true);
			injector::WriteMemory(0x47B2B0, nMenuAlignment, true);
			injector::WriteMemory(0x47B40C, nMenuAlignment, true);
			injector::WriteMemory(0x47B478, nMenuAlignment, true);
			injector::WriteMemory(0x47B4E1, nMenuAlignment, true);

			injector::WriteMemory(0x48AD10 + 0x2, &fWideScreenWidthScaleDown, true);
			injector::WriteMemory(0x48B860 + 0x19B + 0x2, &fWideScreenWidthScaleDown, true);

			injector::WriteMemory(0x48AD50 + 0x2, &fWideScreenHeightScaleDown, true);

			injector::WriteMemory(0x47AA4F + 0x2, &gtaLogo128Coord1, true);
			injector::WriteMemory(0x47AA71 + 0x2, &gtaLogo128Coord2, true);
		}
	}
}

template<uintptr_t addr>
void FixMenuHook()
{
	using func_hook = injector::function_hooker<addr, void()>;
	injector::make_static_hook<func_hook>([](func_hook::func_type nullsub)
	{
		nMenuAlignment = static_cast<int>(((*CDraw::pfScreenAspectRatio / (4.0f / 3.0f)) * 320.0f) - 30);// (CLASS_pclRsGlobal->m_iScreenWidth / 2) * fWideScreenWidthScaleDown;
		gtaLogo128Coord2 = static_cast<float>(nMenuAlignment - 95);
		gtaLogo128Coord1 = gtaLogo128Coord2 + 190.0f;
		FixMenu();
		if (!injector::address_manager::singleton().IsSteam() && IVRadarScaling)
		{
			IVRadar();
		}
		return;
	});
}

void RsSelectDeviceHook()
{
	FixMenu();
	if (!injector::address_manager::singleton().IsSteam())
	{
		injector::MakeInline<0x581F89, 0x581F89 + 5>([](injector::reg_pack&)
		{
			injector::WriteMemory<uchar>(0x581F89 + 5, 0xC3, true);
			FixMenu();
			if (!injector::address_manager::singleton().IsSteam() && IVRadarScaling)
			{
				IVRadar();
			}
		});
	}
	else
	{
		injector::MakeInline<0x5821B9, 0x5821B9 + 5>([](injector::reg_pack&)
		{
			injector::WriteMemory<uchar>(0x5821B9 + 5, 0xC3, true);
			FixMenu();
		});
	}
}

void FixCoronas()
{
	if (!injector::address_manager::singleton().IsSteam())
	{
		injector::WriteMemory<uint8_t>(0x51C46A, 0x0B, true);
		injector::MakeNOP(0x518DCA, 5, true); //CBrightLights::Render
	}
	else
	{
		injector::WriteMemory<uint8_t>(0x51C62A, 0x0B, true);
		injector::MakeNOP(0x518F8A, 5, true); //CBrightLights::Render
	}
}

void FixBorders()
{
	if (!injector::address_manager::singleton().IsSteam())
	{
		injector::MakeNOP(0x46086E, 2, true);
		injector::MakeNOP(0x46B5E8, 2, true);
		injector::MakeNOP(0x46EBB9, 2, true);
		injector::MakeNOP(0x48CF81, 2, true);
		injector::WriteMemory<char>(0x48D2D5, 1, true);
		DrawBordersForWideScreenHook<(0x48E13E)>();
		DrawBordersForWideScreenHook<(0x4FE5D0)>();
	}
	else
	{
		injector::MakeNOP(0x46086E, 2, true);
		injector::MakeNOP(0x46B5B7, 2, true);
		injector::MakeNOP(0x46EB99, 2, true);
		injector::MakeNOP(0x48D011, 2, true);
		injector::WriteMemory<char>(0x48D365, 1, true);
		DrawBordersForWideScreenHook<(0x48E18E)>();
		DrawBordersForWideScreenHook<(0x4FE640)>();
	}
}

void FixHUD()
{
	if (!injector::address_manager::singleton().IsSteam())
	{
	#pragma region HUDFunctions
	#define CDarkel__DrawMessages 0x420920
	#define LoadingScreen 0x48D770
	#define Render2dStuff 0x48E0E0
	#define CClouds__Render 0x4F6D90
	#define CHud__Draw 0x5052A0
	#define cMusicManager__DisplayRadioStationName 0x57E6D0

		injector::WriteMemory(CDarkel__DrawMessages + 0x289 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CDarkel__DrawMessages + 0x2F6 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CDarkel__DrawMessages + 0x395 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CDarkel__DrawMessages + 0x404 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(CDarkel__DrawMessages + 0x267 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CDarkel__DrawMessages + 0x2D4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CDarkel__DrawMessages + 0x373 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CDarkel__DrawMessages + 0x3E3 + 0x2, &fWideScreenHeightScaleDown, true);


		injector::WriteMemory(LoadingScreen + 0x16B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(LoadingScreen + 0x213 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(LoadingScreen + 0x279 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x48DA50 + 0x1B9 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x48DA50 + 0x28B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(Render2dStuff + 0x283 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(Render2dStuff + 0x2EC + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(LoadingScreen + 0x136 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(LoadingScreen + 0x1DE + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(LoadingScreen + 0x263 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x48DA50 + 0x1A3 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x48DA50 + 0x1E6 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x48DA50 + 0x275 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x48DA50 + 0x2AF + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(Render2dStuff + 0x102 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(Render2dStuff + 0x176 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(Render2dStuff + 0x1BC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(Render2dStuff + 0x22F + 0x2, &fWideScreenHeightScaleDown, true);




		injector::WriteMemory(0x4A5040 + 0x15 + 0x2, &fWideScreenWidthScaleDown, true); //CRadar__DrawRadarMask
		injector::WriteMemory(0x4A5D10 + 0x1A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A5EF0 + 0xF + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A6C20 + 0x43 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(0x4A5040 + 0x4B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A5D10 + 0x4D + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A5EF0 + 0x42 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A6C20 + 0x19 + 0x2, &fWideScreenHeightScaleDown, true);




		injector::WriteMemory(CClouds__Render + 0x5DC + 0x2, &fWideScreenWidthScaleDown, true);




		injector::WriteMemory(CHud__Draw + 0x2F4 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3CF + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x4F1 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x603 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x7BD + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x88B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x948 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xA60 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xB2A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xC30 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xCC9 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xD65 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xF3B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xFE5 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x100C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x10D5 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1112 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1321 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1382 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x145B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x14B6 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1570 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1692 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x16F3 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x17CC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1827 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x18BE + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x192A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1A5A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1D52 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1D82 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1E41 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1EEC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x21F0 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2220 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x22DF + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x238A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x23D1 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x249B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2517 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x25F7 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x268B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x26C9 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2737 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x27B7 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2805 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2884 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x295A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x29BD + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2A37 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2AB0 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2B49 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2C14 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2CC1 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2D2E + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2DAD + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2F6F + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3011 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3065 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x30AE + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x31B3 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x327E + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x32FC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3318 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x33DD + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3547 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3586 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3670 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x374E + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3775 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3B6B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3B9C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3C50 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3CF4 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x3A7 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x3DC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x429 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x505 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x56F + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x5EC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x608 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x6CC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x7E5 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x80C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x96E + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x995 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0xCC5 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0xCFA + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0xD94 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0xEA8 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0xED2 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x10F7 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x1199 + 0x2, &fWideScreenWidthScaleDown, true);


		injector::WriteMemory(CHud__Draw + 0x2D0 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3AB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x4BC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x5C8 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x792 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x86B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x96C + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xA11 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xAF9 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xC1A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xCA2 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xD44 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xF09 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xFCF + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x10B4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x10FC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x12FA + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x13C2 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x143A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x14F6 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x155A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x166B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1733 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x17AB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1867 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x18A8 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1974 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1A35 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1D3D + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1D6D + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1E16 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1EC7 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x21DB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x220B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x22B4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2365 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x23BB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2474 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x24F6 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x25E1 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2664 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x26B3 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2716 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x27A1 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x27DD + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2862 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2944 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2A10 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2A8F + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2B0B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2BD3 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2CAB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2D06 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2D8B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2F49 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3001 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3097 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3179 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3262 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x33AF + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3531 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3635 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3738 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x398B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3A37 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3B56 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3B87 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3C2B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3CCF + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0x38C + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0x3C7 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0x4DA + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0x553 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0x69F + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0x7CF + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0x958 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0xCA7 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0xD54 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0xDEB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0xE93 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0xEBD + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0x10C4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0x1174 + 0x2, &fWideScreenHeightScaleDown, true);





		injector::WriteMemory(cMusicManager__DisplayRadioStationName + 0x284 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(cMusicManager__DisplayRadioStationName + 0x2B3 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory<float>(0x60D43C, 0.875f, true);
		injector::WriteMemory<float>(0x60D444, 0.518f, true);
		injector::WriteMemory<float>(0x60D450, 1.0f, true);

		injector::WriteMemory(cMusicManager__DisplayRadioStationName + 0x26E + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(cMusicManager__DisplayRadioStationName + 0x2EF + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(cMusicManager__DisplayRadioStationName + 0x386 + 0x2, &fWideScreenHeightScaleDown, true);





		injector::WriteMemory(0x592880 + 0xE9 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x592880 + 0x17E + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x592880 + 0x231 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x592880 + 0x297 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x592880 + 0x2CC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x592880 + 0x349 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(0x592880 + 0xCF + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x592880 + 0x151 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x592880 + 0x208 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x592880 + 0x27B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x592880 + 0x333 + 0x2, &fWideScreenHeightScaleDown, true);




		injector::WriteMemory(0x595EE0 + 0x66 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x595EE0 + 0x50 + 0x2, &fWideScreenHeightScaleDown, true);



		injector::WriteMemory(CHud__Draw + 0x2F4 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3CF + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x4F1 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x603 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x7BD + 0x2, &fCustomWideScreenWidthScaleDown, true);
		//injector::WriteMemory(CHud__Draw + 0x88B + 0x2, &fCustomWideScreenWidthScaleDown, true);
		//injector::WriteMemory(CHud__Draw + 0x948 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		//injector::WriteMemory(CHud__Draw + 0xA60 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		//injector::WriteMemory(CHud__Draw + 0xB2A + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xC30 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xCC9 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xD65 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xF3B + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xFE5 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x100C + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x10D5 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1112 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1321 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1382 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x145B + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x14B6 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1570 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1692 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x16F3 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x17CC + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1827 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x18BE + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x192A + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1A5A + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1D52 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1D82 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1E41 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1EEC + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x21F0 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2220 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x22DF + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x238A + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x23D1 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x249B + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2517 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x25F7 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x268B + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x26C9 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2737 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x27B7 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2805 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2884 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x295A + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x29BD + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2A37 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2AB0 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2B49 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2C14 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2CC1 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2D2E + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2DAD + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2F6F + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3011 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3065 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x30AE + 0x2, &fCustomWideScreenWidthScaleDown, true);
		//injector::WriteMemory(CHud__Draw + 0x31B3 + 0x2, &fCustomWideScreenWidthScaleDown, true); //radar
		injector::WriteMemory(CHud__Draw + 0x327E + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x32FC + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3318 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x33DD + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3547 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3586 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3670 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x374E + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3775 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3B6B + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3B9C + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3C50 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3CF4 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x3A7 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x3DC + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x429 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x505 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x56F + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x5EC + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x608 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x6CC + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x7E5 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x80C + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x96E + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x995 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0xCC5 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0xCFA + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0xD94 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0xEA8 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0xED2 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x10F7 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x509030 + 0x1199 + 0x2, &fCustomWideScreenWidthScaleDown, true);


		injector::WriteMemory(CHud__Draw + 0x2D0 + 0x2, &fCrosshairHeightScaleDown, true); // let's make crosshair proportional
		injector::WriteMemory(CHud__Draw + 0x3AB + 0x2, &fCrosshairHeightScaleDown, true); // let's make crosshair proportional
		injector::WriteMemory(CHud__Draw + 0x4BC + 0x2, &fCrosshairHeightScaleDown, true); // let's make crosshair proportional
		injector::WriteMemory(CHud__Draw + 0x5C8 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x792 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		//injector::WriteMemory(CHud__Draw + 0x86B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		//injector::WriteMemory(CHud__Draw + 0x96C + 0x2, &fCustomWideScreenHeightScaleDown, true);
		//injector::WriteMemory(CHud__Draw + 0xA11 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		//injector::WriteMemory(CHud__Draw + 0xAF9 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xC1A + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xCA2 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xD44 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xF09 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0xFCF + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x10B4 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x10FC + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x12FA + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x13C2 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x143A + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x14F6 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x155A + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x166B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1733 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x17AB + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1867 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x18A8 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1974 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1A35 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1D3D + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1D6D + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1E16 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x1EC7 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x21DB + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x220B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x22B4 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2365 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x23BB + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2474 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x24F6 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x25E1 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2664 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x26B3 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2716 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x27A1 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x27DD + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2862 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2944 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2A10 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2A8F + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2B0B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2BD3 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2CAB + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2D06 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2D8B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x2F49 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3001 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3097 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		//injector::WriteMemory(CHud__Draw + 0x3179 + 0x2, &fCustomWideScreenHeightScaleDown, true); //radar
		injector::WriteMemory(CHud__Draw + 0x3262 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x33AF + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3531 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3635 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3738 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x398B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3A37 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3B56 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3B87 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3C2B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(CHud__Draw + 0x3CCF + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0x38C + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0x3C7 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0x4DA + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0x553 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0x69F + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0x7CF + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0x958 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0xCA7 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0xD54 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0xDEB + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0xE93 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0xEBD + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0x10C4 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x509030 + 0x1174 + 0x2, &fCustomWideScreenHeightScaleDown, true);
	#pragma endregion HUDFunctions
	}
	else
	{
	#pragma region HUDFunctionsSteam
		injector::WriteMemory(0x420920 + 0x289 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x420920 + 0x2F6 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x420920 + 0x395 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x420920 + 0x404 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(0x420920 + 0x267 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x420920 + 0x2D4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x420920 + 0x373 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x420920 + 0x3E3 + 0x2, &fWideScreenHeightScaleDown, true);



		injector::WriteMemory(0x48D800 + 0x16B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x48D800 + 0x213 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x48D800 + 0x279 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x48DAB0 + 0x1B9 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x48DAB0 + 0x28B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x48E130 + 0x283 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x48E130 + 0x2EC + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(0x48D800 + 0x136 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x48D800 + 0x1DE + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x48D800 + 0x263 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x48DAB0 + 0x1A3 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x48DAB0 + 0x1E6 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x48DAB0 + 0x275 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x48DAB0 + 0x2AF + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x48E130 + 0x102 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x48E130 + 0x176 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x48E130 + 0x1BC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x48E130 + 0x22F + 0x2, &fWideScreenHeightScaleDown, true);



		injector::WriteMemory(0x4A50C0 + 0x15 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A5D90 + 0x1A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A5F70 + 0xF + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x4A6CA0 + 0x43 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(0x4A50C0 + 0x4B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A5D90 + 0x4D + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A5F70 + 0x42 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x4A6CA0 + 0x19 + 0x2, &fWideScreenHeightScaleDown, true);



		injector::WriteMemory(0x4F6E00 + 0x5DC + 0x2, &fWideScreenWidthScaleDown, true);




		injector::WriteMemory(0x505310 + 0x2F4 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3CF + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x4F1 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x603 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x7BD + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x88B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x948 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0xA60 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0xB2A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0xC30 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0xCC9 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0xD65 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0xF3B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0xFE5 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x100C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x10D5 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1112 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1321 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1382 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x145B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x14B6 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1570 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1692 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x16F3 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x17CC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1827 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x18BE + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x192A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1A5A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1D52 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1D82 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1E41 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1EEC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x21F0 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2220 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x22DF + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x238A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x23D1 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x249B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2517 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x25F7 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x268B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x26C9 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2737 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x27B7 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2805 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2884 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x295A + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x29BD + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2A37 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2AB0 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2B49 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2C14 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2CC1 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2D2E + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2DAD + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2F6F + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3011 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3065 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x30AE + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x31B3 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x327E + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x32FC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3318 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x33DD + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3547 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3586 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3670 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x374E + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3775 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3B6B + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3B9C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3C50 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3CF4 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x3A7 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x3DC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x429 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x505 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x56F + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x5EC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x608 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x6CC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x7E5 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x80C + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x96E + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x995 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0xCC5 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0xCFA + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0xD94 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0xEA8 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0xED2 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x10F7 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x1199 + 0x2, &fWideScreenWidthScaleDown, true);




		injector::WriteMemory(0x505310 + 0x2D0 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3AB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x4BC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x5C8 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x792 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x86B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x96C + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0xA11 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0xAF9 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0xC1A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0xCA2 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0xD44 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0xF09 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0xFCF + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x10B4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x10FC + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x12FA + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x13C2 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x143A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x14F6 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x155A + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x166B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1733 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x17AB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1867 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x18A8 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1974 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1A35 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1D3D + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1D6D + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1E16 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1EC7 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x21DB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x220B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x22B4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2365 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x23BB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2474 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x24F6 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x25E1 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2664 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x26B3 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2716 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x27A1 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x27DD + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2862 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2944 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2A10 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2A8F + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2B0B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2BD3 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2CAB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2D06 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2D8B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2F49 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3001 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3097 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3179 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3262 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x33AF + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3531 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3635 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3738 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x398B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3A37 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3B56 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3B87 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3C2B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3CCF + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x38C + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x3C7 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x4DA + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x553 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x69F + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x7CF + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x958 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0xCA7 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0xD54 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0xDEB + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0xE93 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0xEBD + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x10C4 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x1174 + 0x2, &fWideScreenHeightScaleDown, true);





		injector::WriteMemory(0x57E920 + 0x284 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x57E920 + 0x2B3 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory<float>(0x619CFC, 0.875f, true);
		injector::WriteMemory<float>(0x619D04, 0.518f, true);
		injector::WriteMemory<float>(0x619D10, 1.0f, true);

		injector::WriteMemory(0x57E920 + 0x26E + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x57E920 + 0x2EF + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x57E920 + 0x386 + 0x2, &fWideScreenHeightScaleDown, true);



		injector::WriteMemory(0x592A20 + 0xE9 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x592A20 + 0x17E + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x592A20 + 0x231 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x592A20 + 0x297 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x592A20 + 0x2CC + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x592A20 + 0x349 + 0x2, &fWideScreenWidthScaleDown, true);

		injector::WriteMemory(0x592A20 + 0xCF + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x592A20 + 0x151 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x592A20 + 0x208 + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x592A20 + 0x27B + 0x2, &fWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x592A20 + 0x333 + 0x2, &fWideScreenHeightScaleDown, true);



		injector::WriteMemory(0x596080 + 0x66 + 0x2, &fWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x596080 + 0x50 + 0x2, &fWideScreenHeightScaleDown, true);




		injector::WriteMemory(0x505310 + 0x2F4 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3CF + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x4F1 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x603 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x7BD + 0x2, &fCustomWideScreenWidthScaleDown, true);
		//injector::WriteMemory(0x505310 + 0x88B + 0x2, &fCustomWideScreenWidthScaleDown, true);
		//injector::WriteMemory(0x505310 + 0x948 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		//injector::WriteMemory(0x505310 + 0xA60 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		//injector::WriteMemory(0x505310 + 0xB2A + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0xC30 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0xCC9 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0xD65 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0xF3B + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0xFE5 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x100C + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x10D5 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1112 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1321 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1382 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x145B + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x14B6 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1570 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1692 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x16F3 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x17CC + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1827 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x18BE + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x192A + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1A5A + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1D52 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1D82 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1E41 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1EEC + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x21F0 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2220 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x22DF + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x238A + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x23D1 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x249B + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2517 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x25F7 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x268B + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x26C9 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2737 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x27B7 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2805 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2884 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x295A + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x29BD + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2A37 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2AB0 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2B49 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2C14 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2CC1 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2D2E + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2DAD + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2F6F + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3011 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3065 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x30AE + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x31B3 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x327E + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x32FC + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3318 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x33DD + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3547 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3586 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3670 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x374E + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3775 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3B6B + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3B9C + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3C50 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3CF4 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x3A7 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x3DC + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x429 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x505 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x56F + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x5EC + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x608 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x6CC + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x7E5 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x80C + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x96E + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x995 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0xCC5 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0xCFA + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0xD94 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0xEA8 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0xED2 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x10F7 + 0x2, &fCustomWideScreenWidthScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x1199 + 0x2, &fCustomWideScreenWidthScaleDown, true);




		injector::WriteMemory(0x505310 + 0x2D0 + 0x2, &fCrosshairHeightScaleDown, true); // let's make crosshair proportional
		injector::WriteMemory(0x505310 + 0x3AB + 0x2, &fCrosshairHeightScaleDown, true); // let's make crosshair proportional
		injector::WriteMemory(0x505310 + 0x4BC + 0x2, &fCrosshairHeightScaleDown, true); // let's make crosshair proportional
		injector::WriteMemory(0x505310 + 0x5C8 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x792 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		//injector::WriteMemory(0x505310 + 0x86B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		//injector::WriteMemory(0x505310 + 0x96C + 0x2, &fCustomWideScreenHeightScaleDown, true);
		//injector::WriteMemory(0x505310 + 0xA11 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		//injector::WriteMemory(0x505310 + 0xAF9 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0xC1A + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0xCA2 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0xD44 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0xF09 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0xFCF + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x10B4 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x10FC + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x12FA + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x13C2 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x143A + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x14F6 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x155A + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x166B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1733 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x17AB + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1867 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x18A8 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1974 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1A35 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1D3D + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1D6D + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1E16 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x1EC7 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x21DB + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x220B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x22B4 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2365 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x23BB + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2474 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x24F6 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x25E1 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2664 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x26B3 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2716 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x27A1 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x27DD + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2862 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2944 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2A10 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2A8F + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2B0B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2BD3 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2CAB + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2D06 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2D8B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x2F49 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3001 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3097 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3179 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3262 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x33AF + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3531 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3635 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3738 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x398B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3A37 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3B56 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3B87 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3C2B + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x505310 + 0x3CCF + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x38C + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x3C7 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x4DA + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x553 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x69F + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x7CF + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x958 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0xCA7 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0xD54 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0xDEB + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0xE93 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0xEBD + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x10C4 + 0x2, &fCustomWideScreenHeightScaleDown, true);
		injector::WriteMemory(0x5090A0 + 0x1174 + 0x2, &fCustomWideScreenHeightScaleDown, true);
	#pragma endregion HUDFunctionsSteam
	}
}

void IVRadar()
{
	fCustomRadarWidthIV = 101.5f;
	fCustomRadarHeightIV = 78.0f;
	fCustomRadarPosXIV = 109.0f;
	fCustomRadarPosYIV = 107.0f;
	fCustomRadarRingWidthIV = 101.0f;
	fCustomRadarRingHeightIV = 84.0f;
	fCustomRadarRingPosXIV = 111.0f;
	fCustomRadarRingPosYIV = 109.5f;
	fCustomRadarRingPosXIV2 = fCustomRadarRingPosXIV - 13.0f;

	fCustomRadarPosXIV = 109.0f * ((float)RsGlobal->MaximumWidth * (1.0f / 1920.0f));
	fCustomRadarRingPosXIV = 111.0f * ((float)RsGlobal->MaximumWidth * (1.0f / 1920.0f));
	fCustomRadarRingPosXIV2 = 98.0f * ((float)RsGlobal->MaximumWidth * (1.0f / 1920.0f));

	injector::WriteMemory(0x4A505B + 0x2, &fCustomRadarWidthIV, true); //CRadar__DrawRadarMask
	injector::WriteMemory(0x4A5093 + 0x2, &fCustomRadarHeightIV, true); //CRadar__DrawRadarMask
	injector::WriteMemory(0x4A5075 + 0x2, &fCustomRadarPosXIV, true);
	injector::WriteMemory(0x4A50B0 + 0x2, &fCustomRadarPosYIV, true);

	injector::WriteMemory<float>(0x5FDC70, fCustomRadarRingWidthIV, true);
	injector::WriteMemory<float>(0x5FDC68, fCustomRadarRingHeightIV, true);

	injector::WriteMemory(0x50845F + 0x2, &fCustomRadarRingPosXIV, true);
	injector::WriteMemory(0x508481 + 0x2, &fCustomRadarRingPosXIV2, true);
	injector::WriteMemory(0x508433 + 0x2, &fCustomRadarRingPosYIV, true);
}

template<uintptr_t addr, bool bIsMenu = false, bool bBlackText = false, bool bIsSubtitles = false>
void TextDrawOutlineHook()
{
	using printstr_hook = injector::function_hooker<addr, void(float, float, unsigned short*)>;
	injector::make_static_hook<printstr_hook>([](printstr_hook::func_type PrintString, float PosX, float PosY, unsigned short* c)
	{
		CRGBA rgba;
		PrintString = injector::cstd<void(float, float, unsigned short*)>::call<0x500F50>;
		unsigned char originalColorR = *(unsigned char*)0x8F317C;
		unsigned char originalColorG = *(unsigned char*)0x8F317D;
		unsigned char originalColorB = *(unsigned char*)0x8F317E;
		unsigned char originalColorA = *(unsigned char*)0x8F317F;

		if (originalColorR != 0 && originalColorG != 0 && originalColorB != 0 || bBlackText)
		{
			if (!bBlackText)
				injector::thiscall<void(CRGBA*, uint8_t, uint8_t, uint8_t, uint8_t)>::call<0x4F8C20>(&rgba, 0, 0, 0, originalColorA);
			else
				injector::thiscall<void(CRGBA*, uint8_t, uint8_t, uint8_t, uint8_t)>::call<0x4F8C20>(&rgba, 0xFF, 0xFF, 0xFF, originalColorA / 10);
			injector::cstd<void(CRGBA*)>::call<0x501BD0>(&rgba);

			bIsSubtitles ? PosX -= 0.5f : 0.0f;

			PrintString(PosX + 1.0f, PosY, c);
			PrintString(PosX - 1.0f, PosY, c);
			PrintString(PosX, PosY + 1.0f, c);
			PrintString(PosX, PosY - 1.0f, c);
			PrintString(PosX + 1.0f, PosY + 1.0f, c);
			PrintString(PosX - 1.0f, PosY + 1.0f, c);
			PrintString(PosX + 1.0f, PosY - 1.0f, c);
			PrintString(PosX - 1.0f, PosY - 1.0f, c);

			if (bIsMenu)
			{
				*(unsigned char*)0x8F317F = originalColorA / 2;
				PrintString(PosX - 1.0f - 1.0f, PosY, c);
				PrintString(PosX, PosY + 1.0f + 1.0f, c);
				*(unsigned char*)0x8F317F = originalColorA;
			}

			if (ReplaceTextShadowWithOutline > 1)
			{
				PrintString(PosX + 2.0f, PosY, c);
				PrintString(PosX - 2.0f, PosY, c);
				PrintString(PosX, PosY + 2.0f, c);
				PrintString(PosX, PosY - 2.0f, c);
				PrintString(PosX + 2.0f, PosY + 2.0f, c);
				PrintString(PosX - 2.0f, PosY + 2.0f, c);
				PrintString(PosX + 2.0f, PosY - 2.0f, c);
				PrintString(PosX - 2.0f, PosY - 2.0f, c);

				if (bIsMenu)
				{
					*(unsigned char*)0x8F317F = originalColorA / 2;
					PrintString(PosX - 2.0f - 1.0f, PosY, c);
					PrintString(PosX, PosY + 2.0f + 1.0f, c);
					*(unsigned char*)0x8F317F = originalColorA;
				}
			}

			bIsSubtitles ? PosX += 0.5f : 0.0f;

			injector::thiscall<void(CRGBA*, uint8_t, uint8_t, uint8_t, uint8_t)>::call<0x4F8C20>(&rgba, originalColorR, originalColorG, originalColorB, originalColorA);
			injector::cstd<void(CRGBA*)>::call<0x501BD0>(&rgba);
			PrintString(PosX, PosY, c);
		}
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
	nMenuFix = iniReader.ReadInteger("MAIN", "MenuFix", 1);
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

	szFOVControl = iniReader.ReadString("MAIN", "FOVControl", ""); //0x4012F9
	sscanf_s(szFOVControl, "%X", &FOVControl);
	if (FOVControl)
	injector::WriteMemory<float>(FOVControl, 1.0f, true);

	HideAABug = iniReader.ReadInteger("MAIN", "HideAABug", 1);
	SmartCutsceneBorders = iniReader.ReadInteger("MAIN", "SmartCutsceneBorders", 1);
	NoLoadingBarFix = iniReader.ReadInteger("LCS", "NoLoadingBarFix", 0);
	IVRadarScaling = iniReader.ReadInteger("MAIN", "IVRadarScaling", 0);
	ReplaceTextShadowWithOutline = iniReader.ReadInteger("MAIN", "ReplaceTextShadowWithOutline", 0);

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
			injector::WriteMemory(0x5B7D75 + 0x1, &SelectedMultisamplingLevels, true);
		}
		else
		{
			injector::WriteMemory(0x5B7D75 + 0x1, (void*)0x619468);
		}
		
		if (iniReader.ReadInteger("MAIN", "FixVehicleLights", 1))
		{
			injector::WriteMemory(0x004F95E6, 0x005E51B0, true); //lamp corona stretch
			injector::WriteMemory(0x004F94A6, 0x005EC9BC, true); //car lights stretch
		}

		if (SmallerTextShadows)
		{
			fTextShadowsSize = 1.0f;
			injector::WriteMemory(0x505F7B, &fTextShadowsSize, true); injector::WriteMemory(0x505F50, &fTextShadowsSize, true);
			injector::WriteMemory(0x5065D3, &fTextShadowsSize, true); injector::WriteMemory(0x5065A8, &fTextShadowsSize, true);
			injector::WriteMemory(0x50668E, &fTextShadowsSize, true); injector::WriteMemory(0x506670, &fTextShadowsSize, true);
			injector::WriteMemory(0x506944, &fTextShadowsSize, true); injector::WriteMemory(0x506919, &fTextShadowsSize, true);
			injector::WriteMemory(0x5069FF, &fTextShadowsSize, true); injector::WriteMemory(0x5069E1, &fTextShadowsSize, true);
			injector::WriteMemory(0x506C2B, &fTextShadowsSize, true); injector::WriteMemory(0x506C22, &fTextShadowsSize, true);
			injector::WriteMemory(0x5070F3, &fTextShadowsSize, true); injector::WriteMemory(0x5070C8, &fTextShadowsSize, true);
			injector::WriteMemory(0x507591, &fTextShadowsSize, true); injector::WriteMemory(0x507566, &fTextShadowsSize, true);
			injector::WriteMemory(0x50774D, &fTextShadowsSize, true); injector::WriteMemory(0x507722, &fTextShadowsSize, true);
			injector::WriteMemory(0x50793D, &fTextShadowsSize, true); injector::WriteMemory(0x507912, &fTextShadowsSize, true);
			injector::WriteMemory(0x507A8B, &fTextShadowsSize, true); injector::WriteMemory(0x507CE9, &fTextShadowsSize, true);
			injector::WriteMemory(0x507CBE, &fTextShadowsSize, true); injector::WriteMemory(0x507FB4, &fTextShadowsSize, true);
			injector::WriteMemory(0x508C67, &fTextShadowsSize, true); injector::WriteMemory(0x508C46, &fTextShadowsSize, true);
			injector::WriteMemory(0x508F02, &fTextShadowsSize, true); injector::WriteMemory(0x42643F, &fTextShadowsSize, true);
			injector::WriteMemory(0x426418, &fTextShadowsSize, true); injector::WriteMemory(0x42657D, &fTextShadowsSize, true);
			injector::WriteMemory(0x426556, &fTextShadowsSize, true); injector::WriteMemory(0x426658, &fTextShadowsSize, true);
			injector::WriteMemory(0x426637, &fTextShadowsSize, true); injector::WriteMemory(0x509A5E, &fTextShadowsSize, true);
			injector::WriteMemory(0x509A3D, &fTextShadowsSize, true); injector::WriteMemory(0x509A5E, &fTextShadowsSize, true);
			injector::WriteMemory(0x509A3D, &fTextShadowsSize, true); injector::WriteMemory(0x50A139, &fTextShadowsSize, true);
			injector::WriteMemory(0x57E9EE, &fTextShadowsSize, true); injector::WriteMemory(0x57E9CD, &fTextShadowsSize, true);

			injector::WriteMemory<float>(0x5ECCE4, 0.8f, true);
			injector::WriteMemory<float>(0x5ECCE8, 0.93f, true);
			injector::WriteMemory<float>(0x5ECCEC, 0.5f, true);
			injector::WriteMemory<float>(0x5ECFD0, 0.93f, true);
			injector::WriteMemory<float>(0x5ECFD4, 0.8f, true);
		}

		
		if (fSubtitlesScale)
		{
			injector::WriteMemory<float>(0x5FDC7C, 1.12f * fSubtitlesScale, true);
			injector::WriteMemory<float>(0x5FDC80, 0.48f * fSubtitlesScale, true);
		}
		
		if (fRadarWidthScale)
		{
			injector::WriteMemory(CHud__Draw + 0x31B3 + 0x2, &fCustomRadarRingWidthScale, true);
			injector::WriteMemory(CHud__Draw + 0x3179 + 0x2, &fCustomRadarRingHeightScale, true);
			injector::WriteMemory(0x4A5040 + 0x15 + 0x2, &fCustomRadarWidthScale, true); //CRadar__DrawRadarMask
			//CPatch::SetPointer(0x4A5D10 + 0x1A + 0x2, &fCustomRadarWidthScale);
			//CPatch::SetPointer(0x4A5EF0 + 0xF + 0x2, &fCustomRadarWidthScale);
			//CPatch::SetPointer(0x4A6C20 + 0x43 + 0x2, &fCustomRadarWidthScale);
		}
	
		if (IVRadarScaling)
		{
			IVRadar();
		}
		
		if (HideAABug)
		{
			injector::MakeJMP(0x48E0DB, Hide1pxAABug, true);
		}

		if (SmartCutsceneBorders)
		{
			injector::MakeCALL(0x48E13E, CCamera::DrawBordersForWideScreen, true);
			injector::MakeCALL(0x4FE5D0, CCamera::DrawBordersForWideScreen, true);
		}

		if (ReplaceTextShadowWithOutline)
		{
			//SP compat
			injector::WriteMemory(0x500D27, 0xD8C3BF0F, true); injector::WriteMemory(0x500D27 + 4, 0xDA582444, true);
			injector::WriteMemory<WORD>(0x500D4C, 0x4489, true);
			injector::WriteMemory<WORD>(0x500D5F, 0x44DB, true);
			injector::WriteMemory<WORD>(0x500D6E, 0x2444, true);
			injector::WriteMemory(0x500D4E, 0xBF0F1824, true);
			injector::WriteMemory(0x500D70, 0x44DB501C, true);
			injector::WriteMemory(0x500D61, 0xBF0F1C24, true);
			injector::WriteMemory<DWORD>(0x500D53, 0x182444DA, true);
			injector::WriteMemory<BYTE>(0x500D52, 0xC3, true);
			injector::WriteMemory<BYTE>(0x500D65, 0xC3, true);
			injector::WriteMemory<BYTE>(0x500D6D, 0x89, true);
			injector::WriteMemory<WORD>(0x500D74, 0x2024, true);


			//
			TextDrawOutlineHook<(0x47C666), true>(); //(menu shadows and text)
			TextDrawOutlineHook<(0x47C74C), true>(); //(menu shadows and text)
			TextDrawOutlineHook<(0x47AF76), true, true>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(menu)
			TextDrawOutlineHook<(0x47F3E8), true, true>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(menu)

			//shadow
			injector::MakeNOP(0x420BBC, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x420C29, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x420CC8, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x426446, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x481010, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x48113C, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x4827AE, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x505F82, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x5065DA, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x50669B, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x50694B, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x506A0C, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x506C37, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x5070FA, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x507598, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x507754, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x507944, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x507AC8, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x507CF0, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x507FF1, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x508C6E, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x508F09, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x5098D6, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x509A65, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x509DDE, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x57E9F5, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x585786, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x5858E5, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)
			injector::MakeNOP(0x585AB7, 5, true);  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(shadow)

			//
			TextDrawOutlineHook<(0x509544), true>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))(text box) 
			//TextDrawOutlineHook<(0x508363)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *)) //pager
			//TextDrawOutlineHook<(0x592BD8)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *)) with backgr
			TextDrawOutlineHook<(0x508953), false, false, true>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *)) (subtitles)

			//menu 		
			//TextDrawOutlineHook<(0x4813DB), true, true>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *)) CLASSIC CONTROL CONFIGURATION
			//TextDrawOutlineHook<(0x48A019), true, true>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			//TextDrawOutlineHook<(0x48A0D2), true, true>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			//TextDrawOutlineHook<(0x48A241), true, true>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			//TextDrawOutlineHook<(0x48A318), true, true>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))



			//
#if(1)
			TextDrawOutlineHook<(0x420AAD)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x420E3E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4264CD)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x426584)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x42665F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4266EB)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x432723)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x47B132)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x47C74C)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x47F9F1)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x47FAE4)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x47FE87)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x47FF24)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4808A8)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4818BB)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4818FD)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48193F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x481D0A)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4824E1)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x482527)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4825AD)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x482620)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4826D0)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483BE0)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483C04)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483C28)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483C4C)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483C70)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483C94)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483CB8)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483CDC)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483D00)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483D24)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483D48)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483D6C)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483D90)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483DB4)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483DD8)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483E01)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483E25)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483E49)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483E6D)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483E91)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483EB5)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483ED9)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483EFD)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483F21)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483F45)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483F69)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483F8D)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483FB1)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483FD5)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x483FF9)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484022)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484046)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48406A)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48408E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4840B2)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4840D6)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4840FA)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48411E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484142)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484166)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48418A)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4841AE)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4841D2)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4841F6)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48421A)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484243)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484267)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48428B)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4842AF)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4842D3)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4842F7)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48431B)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48433F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484363)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484387)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4843AB)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4843CF)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4843F3)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484417)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48447B)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48449F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4844C3)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4844E7)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48450B)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48452F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484553)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484577)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48459B)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4845BF)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4845E3)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484607)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48462B)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48464F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484673)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484693)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4846BC)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4846E0)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484704)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484728)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48474C)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484770)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484794)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4847B8)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4847DC)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484800)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484824)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484848)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48486C)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484890)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4848B4)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4848D4)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4848FD)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484921)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484945)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484969)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48498D)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4849B1)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4849D5)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4849F9)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484A1D)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484A41)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484A65)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484A89)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484AAD)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484AD1)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484AF5)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484B36)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484B5A)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484B7E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484BA2)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484BC6)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484BEA)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484C0E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484C32)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484C56)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484C7A)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484C9E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484CC2)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484CE6)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484D0A)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484D2E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484D4E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x484F3A)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4850E5)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4893EF)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48945F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x489522)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x489610)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4896EF)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48A3DD)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48A4EA)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48A5FD)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48A741)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48A879)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48B7C4)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48BA25)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48DC67)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x48DD30)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x494413)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x4FE6B2)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x506018)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x50670E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x5067C9)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x506A7F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x506B34)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x506CE8)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x50719F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x50763D)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x5077CA)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x5079EA)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x507B47)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x507D63)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x508070)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x508698)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x508D0E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x508FA7)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x509717)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x509953)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x509AE2)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x509E51)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x50A142)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x50A1DC)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x51AF43)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x57EA80)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x584DF9)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x584E47)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x58581F)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x58597E)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x585B89)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
			TextDrawOutlineHook<(0x59601A)>();  //0x500F50 + 0x0  -> call    PrintString__5CFontFffPUs; CFont::PrintString((float,float,ushort *))
#endif
		}
	}
	else
	{
		//steam
		/*szForceAspectRatio = iniReader.ReadString("MAIN", "ForceMultisamplingLevel", "");
		if (strncmp(szForceAspectRatio, "max", 3) != 0)
		{
			SelectedMultisamplingLevels = iniReader.ReadInteger("MAIN", "ForceMultisamplingLevel", 0);
			injector::WriteMemory(0x5B7D75 + 0x1, &SelectedMultisamplingLevels, true);
		}
		else
		{
			injector::WriteMemory(0x5B7D75 + 0x1, (void*)0x619468);
		}*/
		
		if (iniReader.ReadInteger("MAIN", "FixVehicleLights", 1))
		{
			injector::WriteMemory(0x004F9656, 0x005F7030, true); //lamp corona stretch
			injector::WriteMemory(0x004F9516, 0x005F99BC, true); //car lights stretch
		}
		
		if (SmallerTextShadows)
		{
			fTextShadowsSize = 1.0f;
			injector::WriteMemory(0x505FEB, &fTextShadowsSize, true); injector::WriteMemory(0x505FC0, &fTextShadowsSize, true);
			injector::WriteMemory(0x506643, &fTextShadowsSize, true); injector::WriteMemory(0x506618, &fTextShadowsSize, true);
			injector::WriteMemory(0x5066FE, &fTextShadowsSize, true); injector::WriteMemory(0x5066E0, &fTextShadowsSize, true);
			injector::WriteMemory(0x5069B4, &fTextShadowsSize, true); injector::WriteMemory(0x506989, &fTextShadowsSize, true);
			injector::WriteMemory(0x506A6F, &fTextShadowsSize, true); injector::WriteMemory(0x506A51, &fTextShadowsSize, true);
			injector::WriteMemory(0x506C9B, &fTextShadowsSize, true); injector::WriteMemory(0x506C92, &fTextShadowsSize, true);
			injector::WriteMemory(0x507163, &fTextShadowsSize, true); injector::WriteMemory(0x507138, &fTextShadowsSize, true);
			injector::WriteMemory(0x507601, &fTextShadowsSize, true); injector::WriteMemory(0x5075D6, &fTextShadowsSize, true);
			injector::WriteMemory(0x5077BD, &fTextShadowsSize, true); injector::WriteMemory(0x507792, &fTextShadowsSize, true);
			injector::WriteMemory(0x5079AD, &fTextShadowsSize, true); injector::WriteMemory(0x507982, &fTextShadowsSize, true);
			injector::WriteMemory(0x507AFB, &fTextShadowsSize, true); injector::WriteMemory(0x507D59, &fTextShadowsSize, true);
			injector::WriteMemory(0x507D2E, &fTextShadowsSize, true); injector::WriteMemory(0x508024, &fTextShadowsSize, true);
			injector::WriteMemory(0x508CD7, &fTextShadowsSize, true); injector::WriteMemory(0x508CB6, &fTextShadowsSize, true);
			injector::WriteMemory(0x508F72, &fTextShadowsSize, true); injector::WriteMemory(0x42643F, &fTextShadowsSize, true);
			injector::WriteMemory(0x426418, &fTextShadowsSize, true); injector::WriteMemory(0x42657D, &fTextShadowsSize, true);
			injector::WriteMemory(0x426556, &fTextShadowsSize, true); injector::WriteMemory(0x426658, &fTextShadowsSize, true);
			injector::WriteMemory(0x426637, &fTextShadowsSize, true); injector::WriteMemory(0x509ACE, &fTextShadowsSize, true);
			injector::WriteMemory(0x509AAD, &fTextShadowsSize, true); injector::WriteMemory(0x509ACE, &fTextShadowsSize, true);
			injector::WriteMemory(0x509AAD, &fTextShadowsSize, true); injector::WriteMemory(0x50A1A9, &fTextShadowsSize, true);
			injector::WriteMemory(0x57EC3E, &fTextShadowsSize, true); injector::WriteMemory(0x57EC1D, &fTextShadowsSize, true);

			injector::WriteMemory<float>(0x5F9CE4, 0.8f, true);
			injector::WriteMemory<float>(0x5F9CE8, 0.93f, true);
			injector::WriteMemory<float>(0x5F9CEC, 0.5f, true);
			injector::WriteMemory<float>(0x5F9FD0, 0.93f, true);
			injector::WriteMemory<float>(0x5F9FD4, 0.8f, true);
		}

		
		if (fSubtitlesScale)
		{
			injector::WriteMemory<float>(0x60AA5C, 1.12f * fSubtitlesScale, true);
			injector::WriteMemory<float>(0x60AA60, 0.48f * fSubtitlesScale, true);
		}

		if (fRadarWidthScale)
		{
			fPlayerMarkerPos = 94.0f * fRadarWidthScale;

			injector::WriteMemory(0x505310 + 0x31B3 + 0x2, &fCustomRadarRingWidthScale, true);
			injector::WriteMemory(0x505310 + 0x3179 + 0x2, &fCustomRadarRingHeightScale, true);
			injector::WriteMemory(0x4A50C0 + 0x15 + 0x2, &fCustomRadarWidthScale, true); //CRadar__DrawRadarMask
			//CPatch::SetPointer(0x4A5D10 + 0x1A + 0x2, &fCustomRadarWidthScale);
			//CPatch::SetPointer(0x4A5EF0 + 0xF + 0x2, &fCustomRadarWidthScale);
			//CPatch::SetPointer(0x4A6C20 + 0x43 + 0x2, &fCustomRadarWidthScale);
		}

		/*if (IVRadarScaling)
		{
			IVRadar();
		}*/

		if (HideAABug)
		{
			injector::MakeJMP(0x48E12B, Hide1pxAABug, true);
		}

		if (SmartCutsceneBorders)
		{
			injector::MakeCALL(0x48E18E, CCamera::DrawBordersForWideScreen, true);
			injector::MakeCALL(0x4FE640, CCamera::DrawBordersForWideScreen, true);
		}
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

	} while (GetModuleHandle("SilentPatchIII.asi") == NULL);

	if (GetModuleHandle("SilentPatchIII.asi"))
	{
		ApplyIniOptions();
	}
	return 0;
}

void Init()
{
	if (injector::address_manager::singleton().IsIII())
	{
		if ((injector::address_manager::singleton().GetMajorVersion() == 1 && injector::address_manager::singleton().GetMinorVersion() == 0) || injector::address_manager::singleton().IsSteam())
		{
			if (injector::address_manager::singleton().IsSteam())
			{
				while (true)
				{
					Sleep(0);
					if (*(DWORD*)0x5C6FD0 == 0x53E58955) break; //GTA_III_STEAM
				}
			}

			GetMemoryAddresses();

			OverwriteResolution();
			FixAspectRatio();
			FixFOV();
			RsSelectDeviceHook();
			!injector::address_manager::singleton().IsSteam() ? FixMenuHook<(0x48BC90)>() : FixMenuHook<(0x48BD10)>();
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
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
	}
	return TRUE;
}

