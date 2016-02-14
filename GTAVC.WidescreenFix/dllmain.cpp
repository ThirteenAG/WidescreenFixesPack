#include "..\includes\stdafx.h"
#include "..\includes\GTA\common.h"
#include "..\includes\hooking\Hooking.Patterns.h"

#define _LOG 1
#ifdef _LOG
#include <fstream>
ofstream logfile;
#endif // _LOG

bool bDelay;
auto& gvm = injector::address_manager::singleton();

hook::pattern dwGameLoadStatePattern, DxInputNeedsExclusive, EmergencyVehiclesFixPattern, RadarScalingPattern;
hook::pattern MenuPattern, MenuPattern15625, RsSelectDevicePattern, CDarkelDrawMessagesPattern, CParticleRenderPattern;
hook::pattern DrawHudHorScalePattern, DrawHudVerScalePattern, CSpecialFXRender2DFXsPattern, CSceneEditDrawPattern, sub61DEB0Pattern;
hook::pattern MenuPattern1, MenuPattern2, MenuPattern3, MenuPattern4, MenuPattern5, MenuPattern6, MenuPattern7, MenuPattern8;
hook::pattern ResolutionPattern0, ResolutionPattern1, ResolutionPattern2, ResolutionPattern3, ResolutionPattern4, ResolutionPattern5;
hook::pattern CRadarPattern;
uint32_t* dwGameLoadState;
float* fCRadarRadarRange;
uint32_t funcCCameraAvoidTheGeometryJmp;
void OverwriteResolution();

void GetPatterns()
{
    dwGameLoadStatePattern = hook::pattern("C7 05 ? ? ? ? 09 00 00 00"); //0x60041D
    DxInputNeedsExclusive = hook::pattern("8B 0D ? ? ? ? 83 EC 10 8D 04 24 51 50"); //0x601740
    EmergencyVehiclesFixPattern = hook::pattern("D9 9B F4 00 00 00 D9 83 F0 00 00 00"); //0x46DEB7
    RadarScalingPattern = hook::pattern("D9 1D ? ? ? ? EB 46 "); //0x4C5D0A
    MenuPattern = hook::pattern("3D 80 02 00 00"); //0x48F482
	RsSelectDevicePattern = hook::pattern("C7 40 08 01 00 00 00 B8 01 00 00 00"); //0x600F97

	ResolutionPattern0 = hook::pattern("E8 ? ? ? ? 85 C0 74 0A B8 01 00 00 00 E9 C1 00 00 00"); //0x602F20
	ResolutionPattern1 = hook::pattern("8D 45 60 83 C4 0C 6A 01 50 53 E8 ? ? ? ? 8D 85 5B 01 00 00"); //0x490095 + 2
	ResolutionPattern2 = hook::pattern("83 3D ? ? ? ? 00 59 59 74 2B 81 3C 24");
	ResolutionPattern3 = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? 80 02 00 00");
	ResolutionPattern4 = hook::pattern("85 DB 75 0D E8 ? ? ? ? 39 05 ? ? ? ? 7C A4"); //0x600EBA
	ResolutionPattern5 = hook::pattern("A3 ? ? ? ? E9 84 00 00 00"); //0x600E57

	MenuPattern1 = hook::pattern("89 84 24 88 00 00 00 DB 84 24 88 00 00 00 D8 0D ? ? ? ? DD DA A1 ? ? ? ? 3D C0 01 00 00 75 08"); //0x68D0C0
	MenuPattern2 = hook::pattern("89 44 24 48 DB 44 24 48 D8 0D"); //0x4912E3
	MenuPattern3 = hook::pattern("89 44 24 50 DB 44 24 50 D8 0D"); //0x492DAB
	MenuPattern4 = hook::pattern("89 14 24 DB 04 24 D8 0D"); //0x495886
	MenuPattern5 = hook::pattern("89 44 24 18 DB 44 24 18 D8 0D"); //0x68D5E8
	MenuPattern6 = hook::pattern("D8 0D ? ? ? ? 83 EC 50 DD D9"); //0x4C298B
	MenuPattern7 = hook::pattern("C6 83 40 01 00 00 01 C7 43 40 00 00 22 43 C7 43 44"); //0x4A3CF0
	MenuPattern8 = hook::pattern("89 04 24 DB 04 24 D8 0D"); //0x68C348

	char pattern_str[20];
	union {
		uint32_t* Int;
		unsigned char Byte[4];
	} dword_temp;

	dword_temp.Int = *hook::pattern("89 04 24 DB 04 24 D8 0D").get(10).get<uint32_t*>(8);
	sprintf(pattern_str, "%02X %02X %02X %02X %02X %02X", 0xD8, 0x0D, dword_temp.Byte[0], dword_temp.Byte[1], dword_temp.Byte[2], dword_temp.Byte[3]);
	MenuPattern15625 = hook::pattern(pattern_str); //0x68C350

	dword_temp.Int = *hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24").get(1).get<uint32_t*>(2);
	sprintf(pattern_str, "%02X %02X %02X %02X %02X %02X", 0xD8, 0x0D, dword_temp.Byte[0], dword_temp.Byte[1], dword_temp.Byte[2], dword_temp.Byte[3]);
	CDarkelDrawMessagesPattern = hook::pattern(pattern_str); //0x42A071

	dword_temp.Int = *hook::pattern("D8 0D ? ? ? ? D8 ? ? ? ? ? D9 1C 24 E8").get(8).get<uint32_t*>(2);
	sprintf(pattern_str, "%02X %02X %02X %02X %02X %02X", 0xD8, 0x0D, dword_temp.Byte[0], dword_temp.Byte[1], dword_temp.Byte[2], dword_temp.Byte[3]);
	DrawHudHorScalePattern = hook::pattern(pattern_str); //0x55676E

	dword_temp.Int = *hook::pattern("D8 0D ? ? ? ? D8 8D ? ? ? ? D8 0D").get(0).get<uint32_t*>(2);
	sprintf(pattern_str, "%02X %02X %02X %02X %02X %02X", 0xD8, 0x0D, dword_temp.Byte[0], dword_temp.Byte[1], dword_temp.Byte[2], dword_temp.Byte[3]);
	DrawHudVerScalePattern = hook::pattern(pattern_str); //0x556752

	dword_temp.Int = *hook::pattern("D8 0D ? ? ? ? DD DA D9 43 3C D8 4C 24 34 D8 CA").get(0).get<uint32_t*>(2);
	sprintf(pattern_str, "%02X %02X %02X %02X %02X %02X", 0xD8, 0x0D, dword_temp.Byte[0], dword_temp.Byte[1], dword_temp.Byte[2], dword_temp.Byte[3]);
	CParticleRenderPattern = hook::pattern(pattern_str); //0x560C62

	dword_temp.Int = *hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59").get(18).get<uint32_t*>(2);
	sprintf(pattern_str, "%02X %02X %02X %02X %02X %02X", 0xD8, 0x0D, dword_temp.Byte[0], dword_temp.Byte[1], dword_temp.Byte[2], dword_temp.Byte[3]);
	CSpecialFXRender2DFXsPattern = hook::pattern(pattern_str); //0x573F50

	dword_temp.Int = *hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59").get(21).get<uint32_t*>(2);
	sprintf(pattern_str, "%02X %02X %02X %02X %02X %02X", 0xD8, 0x0D, dword_temp.Byte[0], dword_temp.Byte[1], dword_temp.Byte[2], dword_temp.Byte[3]);
	CSceneEditDrawPattern = hook::pattern(pattern_str); //0x605E20

	dword_temp.Int = *hook::pattern("D8 0D ? ? ? ? DD D9 D9 05 ? ? ? ? D8 C9 DD DA D9 C1 D9 5C 24 24").get(0).get<uint32_t*>(2);
	sprintf(pattern_str, "%02X %02X %02X %02X %02X %02X", 0xD8, 0x0D, dword_temp.Byte[0], dword_temp.Byte[1], dword_temp.Byte[2], dword_temp.Byte[3]);
	sub61DEB0Pattern = hook::pattern(pattern_str); //0x61DF79

	dword_temp.Int = *MenuPattern6.get(0).get<uint32_t*>(2);
	sprintf(pattern_str, "%02X %02X %02X %02X %02X %02X", 0xD8, 0x0D, dword_temp.Byte[0], dword_temp.Byte[1], dword_temp.Byte[2], dword_temp.Byte[3]);
	CRadarPattern = hook::pattern(pattern_str); //0x4C1B18
	//logfile << pattern_str << std::endl;
}

void GetMemoryAddresses()
{
        RsGlobal = *hook::pattern("8B 35 ? ? ? ? 6A 00 55 6A 00 6A 00 52 50").get(0).get<RsGlobalType *>(2); //0x9B48D8
        CDraw::pfScreenAspectRatio = *hook::pattern("FF 35 ? ? ? ? FF 74 24 10 6A 00 50").get(0).get<float*>(2); //0x94DD38
        CDraw::pfScreenFieldOfView = *hook::pattern("D8 35 ? ? ? ? D9 9B F0 00 00 00 D9 45 08 D8 1D").get(0).get<float*>(2); //0x696658
        CSprite2dDrawRect = (int(__cdecl *)(CRect const &, CRGBA const &)) hook::pattern("8B 44 24 04 53 8B 5C 24 0C 53 53 53 53 50").get(0).get<uint32_t>(0); //0x577B00
        funcCCameraAvoidTheGeometry = (void(__thiscall *)(void* _this, RwV3d const& a1, RwV3d const& a2, RwV3d& a3, float a4)) hook::pattern("53 56 57 55 81 EC 20 01 00 00 8B").get(0).get<uint32_t>(0); //0x473AA4
        funcCCameraAvoidTheGeometryJmp = (uint32_t)funcCCameraAvoidTheGeometry + 10;
        bWideScreen = *hook::pattern("80 3D ? ? ? ? 00 D9 C1 DD DA 74 00").get(0).get<bool*>(2);//0x869652
        BordersVar1 = *hook::pattern("A1 ? ? ? ? 85 C0 74 05 83 F8 02 75 1D").get(0).get<uint32_t*>(1); //0x7E4738
        BordersVar2 = *hook::pattern("DF E0 F6 C4 45 75 00 C7 05").get(0).get<uint32_t*>(9); //0x7E474C
        FindPlayerVehicle = (int(__cdecl *)()) hook::pattern("0F B6 05 ? ? ? ? 6B C0 2E 8B 0C C5 ? ? ? ? 85 C9 74 10").get(0).get<uint32_t>(0);//0x4BC1E0
        bIsInCutscene = *hook::pattern("80 3D ? ? ? ? ? 74 04 83 C4 38 C3").get(0).get<bool*>(2); //0x7E46F5
        dwGameLoadState = *dwGameLoadStatePattern.get(0).get<uint32_t*>(2);
        fCRadarRadarRange = *RadarScalingPattern.get(0).get<float*>(2);
        //logfile << hex << (uint32_t)funcCCameraAvoidTheGeometry << std::endl;
}

injector::hook_back<signed int(__cdecl*)(void)> hbRsSelectDevice;
signed int __cdecl RsSelectDeviceHook2()
{
	OverwriteResolution(); //SilentPatch compat
	return hbRsSelectDevice.fun();
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

	if(!hbRsSelectDevice.fun)
		hbRsSelectDevice.fun = injector::MakeCALL(ResolutionPattern0.get(0).get<uint32_t>(0), RsSelectDeviceHook2).get();

    injector::WriteMemory<uchar>(ResolutionPattern1.get(0).get<uint32_t>(2), 22, true);

    injector::WriteMemory<int>(ResolutionPattern2.get(0).get<uint32_t>(14), ResX, true); //0x600E7B + 0x3
    injector::WriteMemory<int>(ResolutionPattern2.get(0).get<uint32_t>(24), ResY, true); //0x600E84 + 0x4
    injector::WriteMemory<uchar>(ResolutionPattern2.get(0).get<uint32_t>(34), 32, true); //0x600E8E + 0x4

    injector::WriteMemory(ResolutionPattern3.get(0).get<uint32_t>(16), ResX, true); //0x602D3A + 0x6
    injector::WriteMemory(ResolutionPattern3.get(0).get<uint32_t>(26), ResY, true); //0x602D44 + 0x6
    injector::WriteMemory(ResolutionPattern3.get(1).get<uint32_t>(16), ResX, true); //0x602D4E + 0x6
    injector::WriteMemory(ResolutionPattern3.get(1).get<uint32_t>(26), ResY, true); //0x602D58 + 0x6

    injector::WriteMemory<uchar>(ResolutionPattern4.get(0).get<uint32_t>(15), 0xEB, true); //jl      short loc_600E60 > jmp      short loc_600E60
	
    injector::MakeNOP(ResolutionPattern5.get(0).get<uint32_t>(5), 5, true);
}

void FixAspectRatio()
{
	auto pattern = hook::pattern("80 3D ? ? ? ? 00 C7 05 ? ? ? ? 00 00 00 00 74 21"); //0x54A270
	injector::MakeJMP(pattern.get(0).get<uint32_t>(0), CDraw::CalculateAspectRatio, true);
}

void __declspec(naked) funcCCameraAvoidTheGeometryHook()
{
    if (*CDraw::pfScreenAspectRatio < 2.0f)
    {
        _asm
        {
            push    ebx
            push    esi
            push    edi
            push    ebp
            sub     esp, 120h
            jmp     funcCCameraAvoidTheGeometryJmp
        }
    }
    else
        __asm ret 10h
}

void FixFOV()
{
        auto pattern = hook::pattern("D9 44 24 04 D9 1D ? ? ? ? C3"); //0x54A2E0 
        injector::MakeJMP(pattern.get(0).get<uint32_t>(0), CDraw::SetFOV, true);

        pattern = hook::pattern("E8 ? ? ? ? D9 EE D9 EE D9 44 24 38 D8 A3 7C 01 00 00 DD D9"); //0x480456 
        injector::MakeCALL(pattern.get(0).get<uint32_t>(0), funcCCameraAvoidTheGeometryHook, true);

        struct EmergencyVehiclesFix
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.ebx + 0xF4) = fEmergencyVehiclesFix;
            }
        }; injector::MakeInline<EmergencyVehiclesFix>(EmergencyVehiclesFixPattern.get(0).get<uint32_t>(0), EmergencyVehiclesFixPattern.get(0).get<uint32_t>(6));


        struct RadarScaling
        {
            void operator()(injector::reg_pack& regs)
            {
                _asm
                {
                    fstp    ds : fRadarScaling
                }
                *fCRadarRadarRange = fRadarScaling;
                fRadarScaling -= 180.0f;
            }
        }; injector::MakeInline<RadarScaling>(RadarScalingPattern.get(0).get<uint32_t>(0), RadarScalingPattern.get(0).get<uint32_t>(6));
}

void FixMenu()
{
    float fMenuTextScale = 0.0009375000373f / (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
    injector::WriteMemory<float>(*MenuPattern1.get(1).get<uint32_t*>(16), fMenuTextScale, true);

	float fMenuControlsOptionsTextScale = 0.00062499999f / (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
	injector::WriteMemory<float>(*MenuPattern2.get(4).get<uint32_t*>(10), fMenuControlsOptionsTextScale, true);

	float fMenuControlsOptionsTextScale2 = 0.00039062501f / (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
	injector::WriteMemory<float>(*MenuPattern2.get(2).get<uint32_t*>(10), fMenuControlsOptionsTextScale2, true);

	float fMenuBarsScale = 0.0046875002f / (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
	injector::WriteMemory<float>(*MenuPattern2.get(1).get<uint32_t*>(10), fMenuBarsScale, true);

	float fMenuHeadersSkinsScale = 0.0014062499f / (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
	injector::WriteMemory<float>(*MenuPattern3.get(35).get<uint32_t*>(10), fMenuHeadersSkinsScale, true);

	/*for (size_t i = 0; i < MenuPattern15625.size(); i++) // moved to fixhud()
	{
		if (i > 0 &&  i < 32)
		{
			uint32_t* p15625 = MenuPattern15625.get(i).get<uint32_t>(2);
			injector::WriteMemory(p15625, &fWideScreenWidthScaleDown, true); //MenuLotsOfStuffScale
		}
	}*/

	float fvcLogoScale = 0.2453125f / (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
	injector::WriteMemory<float>(*MenuPattern4.get(3).get<uint32_t*>(8), fvcLogoScale, true);
	
	float fMapLegendTextScale = 0.001015625f / (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
	injector::WriteMemory<float>(*MenuPattern5.get(4).get<uint32_t*>(10), fMapLegendTextScale, true);
	
	float fMapLegendIconsTextScale = 0.000859375f/ (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
	injector::WriteMemory<float>(*MenuPattern5.get(10).get<uint32_t*>(10), fMapLegendIconsTextScale, true);

	injector::WriteMemory<float>(*MenuPattern6.get(0).get<uint32_t*>(2), fWideScreenWidthScaleDown, true); //fRadarAndBlipsScale

	float fMapPos = (640.0f * (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f))) / 2.0f;
	injector::WriteMemory<float>(MenuPattern7.get(0).get<uint32_t>(17), fMapPos, true);

	float fMapBottomTextScale = 0.00046875002f / (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
	injector::WriteMemory<float>(*MenuPattern8.get(9).get<uint32_t*>(8), fMapBottomTextScale, true);
}

void RsSelectDeviceHook()
{
	struct RSDH
	{
		void operator()(injector::reg_pack& regs)
		{
			*(DWORD*)(regs.eax + 8) = 1;
			CDraw::CalculateAspectRatio();
			FixMenu();
		}
	}; injector::MakeInline<RSDH>(RsSelectDevicePattern.get(0).get<uint32_t>(0), RsSelectDevicePattern.get(0).get<uint32_t>(7));
}

void FixCoronas()
{
	auto pattern = hook::pattern("D8 0E D9 1E D9 05 ? ? ? ? D8 35 ? ? ? ? D8 0B D9 1B"); //0x57797A 
	injector::WriteMemory<uint8_t>(pattern.get(0).get<uint32_t>(1), 0x0B, true);
	pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? 31 DB 31 ED BE"); //0x57479D 
	injector::MakeNOP(pattern.get(1).get<uint32_t>(0), 5, true); //CBrightLights::Render
}

injector::hook_back<void(__fastcall*)(void*)> hbDrawBorders;
static void __fastcall DrawBordersForWideScreenHook(void* _this)
{
	if (!*bWideScreen == false)
		return hbDrawBorders.fun(_this);
}

void FixBorders()
{
	auto pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? DE E9 D9 5A 04"); //0x46FDA9 
	injector::MakeNOP(pattern.get(0).get<uint32_t>(0), 12, true);
	pattern = hook::pattern("D9 5A 0C 83 C4 08 5B C2 04 00"); //0x46FE09
	injector::MakeNOP(pattern.get(0).get<uint32_t>(0), 3, true);

	pattern = hook::pattern("80 3D F5 ? ? ? ? 74 0A B9 ? ? ? ? E8"); //0x54A223 //0x4A61EE
	hbDrawBorders.fun = injector::MakeCALL(pattern.get(0).get<uint32_t>(14), DrawBordersForWideScreenHook).get();
	injector::MakeCALL(pattern.get(1).get<uint32_t>(14), DrawBordersForWideScreenHook);
}

void FixHUD()
{
	for (size_t i = 0; i < CDarkelDrawMessagesPattern.size(); i++)
	{
		uint32_t* p15625 = CDarkelDrawMessagesPattern.get(i).get<uint32_t>(2);
		injector::WriteMemory(p15625, &fWideScreenWidthScaleDown, true);
	}

	auto pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24"); //0x42F2B0
	injector::WriteMemory(pattern.get(8).get<uint32_t>(2), &fWideScreenWidthScaleDown, true);

	pattern = hook::pattern("D8 0D ? ? ? ? 89 44 24 04 8D 44 24 54 50"); //0x4A6417
	injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &fWideScreenWidthScaleDown, true); //sniper scope border
	pattern = hook::pattern("D8 0D ? ? ? ? D1 F8 52 89 44 24 04"); //0x4A646B
	injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &fWideScreenWidthScaleDown, true);

	for (size_t i = 0; i < DrawHudHorScalePattern.size(); i++)
	{
		uint32_t* p15625 = DrawHudHorScalePattern.get(i).get<uint32_t>(2);
		injector::WriteMemory(p15625, &fWideScreenWidthScaleDown, true);
	}

	for (size_t i = 0; i < MenuPattern15625.size(); i++)
	{
		if (i > 0 && i < 32)
		{
			uint32_t* p15625 = MenuPattern15625.get(i).get<uint32_t>(2);
			injector::WriteMemory(p15625, &fWideScreenWidthScaleDown, true); //MenuLotsOfStuffScale
		}
	}

	for (size_t i = 0; i < CParticleRenderPattern.size(); i++)
	{
		uint32_t* p15625 = CParticleRenderPattern.get(i).get<uint32_t>(2);
		injector::WriteMemory(p15625, &fWideScreenWidthScaleDown, true);
	}

	for (size_t i = 0; i < CSpecialFXRender2DFXsPattern.size(); i++)
	{
		uint32_t* p15625 = CSpecialFXRender2DFXsPattern.get(i).get<uint32_t>(2);
		injector::WriteMemory(p15625, &fWideScreenWidthScaleDown, true);
	}

	pattern = hook::pattern("50 D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8"); //0x5FA15B
	injector::WriteMemory(pattern.get(31).get<uint32_t>(3), &fWideScreenWidthScaleDown, true);
	injector::WriteMemory(pattern.get(32).get<uint32_t>(3), &fWideScreenWidthScaleDown, true);
	injector::WriteMemory(pattern.get(38).get<uint32_t>(3), &fWideScreenWidthScaleDown, true);//0x620C45

	for (size_t i = 0; i < CSceneEditDrawPattern.size(); i++)
	{
		uint32_t* p15625 = CSceneEditDrawPattern.get(i).get<uint32_t>(2);
		injector::WriteMemory(p15625, &fWideScreenWidthScaleDown, true);
	}

	for (size_t i = 0; i < sub61DEB0Pattern.size(); i++)
	{
		uint32_t* p15625 = sub61DEB0Pattern.get(i).get<uint32_t>(2);
		injector::WriteMemory(p15625, &fWideScreenWidthScaleDown, true);
	}

	//pattern = hook::pattern(""); //0x620C46
	//injector::WriteMemory(pattern.get(31).get<uint32_t>(3), &fWideScreenWidthScaleDown, true);

}

void FixCrosshair()
{
	for (size_t i = 0; i < DrawHudVerScalePattern.size(); i++)
	{
		if (i >= 13 && i <= 15) //0x557320 + 0x3B6 + 0x2, 0x557320 + 0x49A + 0x2, 0x557320 + 0x5B1 + 0x2
		{
			uint32_t* pCustomScaleVer = DrawHudVerScalePattern.get(i).get<uint32_t>(2);
			injector::WriteMemory(pCustomScaleVer, &fCrosshairHeightScaleDown, true);
		}
	}

	injector::WriteMemory(0x46F927, &fCrosshairPosFactor, true);
	injector::WriteMemory(0x5575FA, &fCrosshairPosFactor, true);
	injector::WriteMemory(0x606B25, &fCrosshairPosFactor, true);
}

void ApplyIniOptions()
{
	CIniReader iniReader("");
	fHudWidthScale = iniReader.ReadFloat("MAIN", "HudWidthScale", 0.0f);
	fHudHeightScale = iniReader.ReadFloat("MAIN", "HudHeightScale", 0.0f);

	if (fHudWidthScale && fHudHeightScale) 
	{
		for (size_t i = 0; i < DrawHudHorScalePattern.size(); i++)
		{
			if (i > 3 && i != 20 && i != 66 && i != 67 && i != 15 && i != 16 && i != 17) //15 16 17 - crosshair
			{
				uint32_t* pCustomScaleHor = DrawHudHorScalePattern.get(i).get<uint32_t>(2);
				injector::WriteMemory(pCustomScaleHor, &fCustomWideScreenWidthScaleDown, true);
			}
		}

		for (size_t i = 0; i < DrawHudVerScalePattern.size(); i++)
		{
			if (i > 1 && i != 18 && i != 57 && i != 58 && i != 13 && i != 14 && i != 15) //13 14 15 - crosshair
			{
				uint32_t* pCustomScaleVer = DrawHudVerScalePattern.get(i).get<uint32_t>(2);
				injector::WriteMemory(pCustomScaleVer, &fCustomWideScreenHeightScaleDown, true);
			}
		}
	}

	fRadarWidthScale = iniReader.ReadFloat("MAIN", "RadarWidthScale", 0.0f);
	fRadarWidthScale = 0.80354591724f;
	if (fRadarWidthScale)
	{
		//injector::WriteMemory<float>(*MenuPattern6.get(0).get<uint32_t*>(2), fWideScreenWidthScaleDown, true); //fRadarAndBlipsScale

		for (size_t i = 0; i < CRadarPattern.size(); i++)
		{
			if ((i >= 0 && i < 6) || i == 19 || i == 33)
			{
				uint32_t* p15625 = CRadarPattern.get(i).get<uint32_t>(2);
				injector::WriteMemory(p15625, &fCustomRadarWidthScale, true); //MenuLotsOfStuffScale
			}
		}

		injector::WriteMemory(DrawHudHorScalePattern.get(66).get<uint32_t>(2), &fCustomRadarWidthScale, true);
		injector::WriteMemory(DrawHudHorScalePattern.get(67).get<uint32_t>(2), &fCustomRadarWidthScale, true);

		auto pattern = hook::pattern("D8 0D ? ? ? ? DD DA D9 05 ? ? ? ? D8 CA"); //0x4C4200 + 0xF4 + 0x2 + 0x6
		injector::WriteMemory(pattern.get(2).get<uint32_t>(2), &fPlayerMarkerPos, true);

		pattern = hook::pattern("D8 0D ? ? ? ? DD DA D9 C0 D8 CA"); //0x4C5170 + 0x307 + 0x2 + 0x6
		injector::WriteMemory(pattern.get(1).get<uint32_t>(2), &fPlayerMarkerPos, true);

		//injector::WriteMemory(0x4C4200 + 0x2E6 + 0x2 + 0x6, &fPlayerMarkerPos, true);
	}
}

DWORD WINAPI Init(LPVOID)
{
    #ifdef _LOG
    
    logfile.open("GTAVC.WidescreenFix.log");
    #endif // _LOG

    auto pattern = hook::pattern("6A 02 6A 00 6A 00 68 01 20 00 00");
    if (!(pattern.size() > 0) && !bDelay)
    {
        bDelay = true;
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
        return 0;
    }

    if (bDelay)
    {
        while (!(pattern.size() > 0))
            pattern = hook::pattern("6A 02 6A 00 6A 00 68 01 20 00 00");
    }
    
    GetPatterns();

    //Immediate changes
	DxInputNeedsExclusive.size() > 0 ? injector::WriteMemory<uint32_t>(DxInputNeedsExclusive.get(0).get<uint32_t>(0), 0xC3C030, true) : nullptr; //mouse fix
    GetMemoryAddresses();
    OverwriteResolution();
    FixAspectRatio();
    FixFOV();
	RsSelectDeviceHook();
	FixCoronas();
	FixBorders();
	FixHUD();
	FixCrosshair();
	ApplyIniOptions();

    //Delayed changes
    struct LoadState
    {
        void operator()(injector::reg_pack& regs) 
        {  
            *dwGameLoadState = 9;
			fCrosshairPosFactor = ((0.52999997f - 0.5f) / ((*CDraw::pfScreenAspectRatio) / (16.0f / 9.0f))) + 0.5f;
			fCrosshairHeightScaleDown = fWideScreenWidthScaleDown * *CDraw::pfScreenAspectRatio;

			fWideScreenHeightScaleDown = 1.0f / 480.0f;
			fCustomWideScreenWidthScaleDown = fWideScreenWidthScaleDown * fHudWidthScale;
			fCustomWideScreenHeightScaleDown = fWideScreenHeightScaleDown * fHudHeightScale;

			fCustomRadarWidthScale = fWideScreenWidthScaleDown * fRadarWidthScale;
			fPlayerMarkerPos = 94.0f * fRadarWidthScale;
            //IVRadar();
            //////////////////////////////////////////
			/*

			

			//Ini options
			fCustomWideScreenWidthScaleDown = fWideScreenWidthScaleDown * fHudWidthScale;
			fCustomWideScreenHeightScaleDown = fWideScreenHeightScaleDown * fHudHeightScale;
			fCustomRadarWidthScale = fWideScreenWidthScaleDown * fRadarWidthScale;
			fCustomRadarRingWidthScale = fCustomRadarWidthScale + 0.000019f;
			fCustomRadarRingHeightScale = fWideScreenHeightScaleDown + 0.000019f;

		injector::WriteMemory<float>(0x6CD9B8, 0.875f, true);
		injector::WriteMemory<float>(0x6CD9C0, 0.518f, true);
		injector::WriteMemory<float>(0x6CD9CC, 1.0f, true);




		injector::WriteMemory<double>(0x697AD8, (1.8 / 1.50), true); //h
		injector::WriteMemory<double>(0x697AE0, (1.7 / 1.50), true); //w
		injector::WriteMemory<double>(0x697AE8, (1.5 / 1.50), true); //w
			
			*/
        }
    }; injector::MakeInline<LoadState>(dwGameLoadStatePattern.get(0).get<uint32_t>(0), dwGameLoadStatePattern.get(0).get<uint32_t>(10));
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

