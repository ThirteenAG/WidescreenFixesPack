#include "stdafx.h"
#include "GTA\common.h"
#include "GTA\global.h"

//#define _LOG
#ifdef _LOG
#include <fstream>
ofstream logfile;
#endif // _LOG

hook::pattern dwGameLoadStatePattern, DxInputNeedsExclusive, EmergencyVehiclesFixPattern, RadarScalingPattern;
hook::pattern MenuPattern, MenuPattern15625, RsSelectDevicePattern, CDarkelDrawMessagesPattern, CDarkelDrawMessagesPattern2, CParticleRenderPattern;
hook::pattern DrawHudHorScalePattern, DrawHudVerScalePattern, CSpecialFXRender2DFXsPattern, CSceneEditDrawPattern, sub61DEB0Pattern;
hook::pattern MenuPattern1, MenuPattern2, MenuPattern3, MenuPattern4, MenuPattern5, MenuPattern6, MenuPattern7, MenuPattern8, MenuPattern9, MenuPattern10, MenuPattern11, MenuPattern12;
hook::pattern ResolutionPattern0, ResolutionPattern1, ResolutionPattern2, ResolutionPattern3, ResolutionPattern4, ResolutionPattern5;
hook::pattern CRadarPattern, BordersPattern;
uint32_t* dwGameLoadState;
float* fCRadarRadarRange;
uint32_t funcCCameraAvoidTheGeometryJmp;
uint32_t* SetColorAddr;
uint8_t* bFontColorA;
uint8_t* bDropShadowPosition;
uint8_t* bFontDropColorA;
uint8_t* bBackgroundOn;
uint16_t* font94B924;
uint32_t* CMenuManager_m_PrefsLanguage;
void OverwriteResolution();
void* pRwRenderStateSet;

void GetPatterns()
{
    dwGameLoadStatePattern = hook::pattern("C7 05 ? ? ? ? 09 00 00 00"); //0x60041D
    DxInputNeedsExclusive = hook::pattern("8B 0D ? ? ? ? 83 EC 10 8D 04 24 51 50"); //0x601740
    EmergencyVehiclesFixPattern = hook::pattern("D9 9B F4 00 00 00 D9 83 F0 00 00 00"); //0x46DEB7
    RadarScalingPattern = hook::pattern("D9 1D ? ? ? ? EB 46 "); //0x4C5D0A
    //MenuPattern = hook::pattern("3D 80 02 00 00"); //0x48F482
    RsSelectDevicePattern = hook::pattern("C7 40 08 01 00 00 00 B8 01 00 00 00"); //0x600F97
    BordersPattern = hook::pattern("74 ? B9 ? ? ? ? E8"); //0x54A223 //0x4A61EE

    ResolutionPattern0 = hook::pattern("E8 ? ? ? ? 85 C0 74 0A B8 01 00 00 00 E9 C1 00 00 00"); //0x602F20
    ResolutionPattern1 = hook::pattern("8D 45 60 83 C4 0C 6A 01 50 53 E8 ? ? ? ? 8D 85 5B 01 00 00"); //0x490095 + 2
    ResolutionPattern2 = hook::pattern("83 3D ? ? ? ? 00 59 59 74 2B 81 3C 24");
    ResolutionPattern3 = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? 80 02 00 00");
    ResolutionPattern4 = hook::pattern("85 DB 75 0D E8 ? ? ? ? 39 05 ? ? ? ? 7C A4"); //0x600EBA
    ResolutionPattern5 = hook::pattern("A3 ? ? ? ? E9 84 00 00 00"); //0x600E57

    MenuPattern1 = hook::pattern("89 84 24 88 00 00 00 DB 84 24 88 00 00 00 D8 0D ? ? ? ? DD DA A1 ? ? ? ? 3D C0 01 00 00 75 08"); //0x68D29C
    MenuPattern2 = hook::pattern("89 44 24 48 DB 44 24 48 D8 0D"); //0x4912E3
    MenuPattern3 = hook::pattern("89 44 24 50 DB 44 24 50 D8 0D"); //0x492DAB
    MenuPattern4 = hook::pattern("89 14 24 DB 04 24 D8 0D"); //0x495886
    MenuPattern5 = hook::pattern("89 44 24 18 DB 44 24 18 D8 0D"); //0x68D5E8
    MenuPattern6 = hook::pattern("D8 0D ? ? ? ? 83 EC 50 DD D9"); //0x4C298B
    MenuPattern7 = hook::pattern("C6 83 40 01 00 00 01 C7 43 40 00 00 22 43 C7 43 44"); //0x4A3CF0
    MenuPattern8 = hook::pattern("89 04 24 DB 04 24 D8 0D"); //0x68C348
    MenuPattern9 = hook::pattern("89 44 24 08 DB 44 24 08 D8 0D"); //0x68D5C4
    MenuPattern10 = hook::pattern("89 44 24 10 DB 44 24 10 D8 0D"); //0x68D50C Loading game. Please wait...
    MenuPattern11 = hook::pattern("D8 05 ? ? ? ? DD DA D9 46 44"); //0x497AC1 Related to map scrolling position and something else
    MenuPattern12 = hook::pattern("89 ? 24 20 DB 44 24 20 D8 0D"); //0x4A3731 cursor
                                  
    auto dword_temp = *hook::pattern("89 04 24 DB 04 24 D8 0D").count(11).get(10).get<uint32_t*>(8);
    MenuPattern15625 = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(dword_temp))); //0x68C350

    dword_temp = *hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24").count(2).get(1).get<uint32_t*>(2);
    CDarkelDrawMessagesPattern = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(dword_temp))); //0x42A087

    dword_temp = *hook::pattern("50 D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 DB 05 ? ? ? ? 50").count(1).get(0).get<uint32_t*>(3);
    CDarkelDrawMessagesPattern2 = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(dword_temp))); //0x42A070

    dword_temp = *hook::pattern("D8 0D ? ? ? ? D8 ? ? ? ? ? D9 1C 24 E8").count(9).get(8).get<uint32_t*>(2);
    DrawHudHorScalePattern = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(dword_temp))); //0x55676E

    dword_temp = *hook::pattern("D8 0D ? ? ? ? D8 8D ? ? ? ? D8 0D").count(1).get(0).get<uint32_t*>(2);
    DrawHudVerScalePattern = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(dword_temp))); //0x556752

    dword_temp = *hook::pattern("D8 0D ? ? ? ? DD DA D9 43 3C D8 4C 24 34 D8 CA").count(1).get(0).get<uint32_t*>(2);
    CParticleRenderPattern = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(dword_temp))); //0x560C62

    dword_temp = *hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59").count(19).get(18).get<uint32_t*>(2);
    CSpecialFXRender2DFXsPattern = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(dword_temp))); //0x573F50

    dword_temp = *hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59").count(22).get(21).get<uint32_t*>(2);
    CSceneEditDrawPattern = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(dword_temp))); //0x605E20

    dword_temp = *hook::pattern("D8 0D ? ? ? ? DD D9 D9 05 ? ? ? ? D8 C9 DD DA D9 C1 D9 5C 24 24").count(1).get(0).get<uint32_t*>(2);
    sub61DEB0Pattern = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(dword_temp))); //0x61DF79

    dword_temp = *MenuPattern6.count(1).get(0).get<uint32_t*>(2);
    CRadarPattern = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(dword_temp))); //0x4C1B18
}

void GetMemoryAddresses()
{
        RsGlobal = *hook::pattern("8B 35 ? ? ? ? 6A 00 55 6A 00 6A 00 52 50").count(1).get(0).get<RsGlobalType *>(2); //0x9B48D8
        CDraw::pfScreenAspectRatio = *hook::pattern("FF 35 ? ? ? ? FF 74 24 10 6A 00 50").count(1).get(0).get<float*>(2); //0x94DD38
        CDraw::pfScreenFieldOfView = *hook::pattern("D8 35 ? ? ? ? D9 9B F0 00 00 00 D9 45 08 D8 1D").count(1).get(0).get<float*>(2); //0x696658
        CSprite2dDrawRect = (int(__cdecl *)(CRect const &, CRGBA const &)) hook::pattern("8B 44 24 04 53 8B 5C 24 0C 53 53 53 53 50").count(1).get(0).get<uint32_t>(0); //0x577B00
        funcCCameraAvoidTheGeometry = (void(__thiscall *)(void* _this, RwV3d const& a1, RwV3d const& a2, RwV3d& a3, float a4)) hook::pattern("53 56 57 55 81 EC 20 01 00 00 8B").count(1).get(0).get<uint32_t>(0); //0x473AA4
        funcCCameraAvoidTheGeometryJmp = (uint32_t)funcCCameraAvoidTheGeometry + 10;
        bWideScreen = *hook::pattern("80 3D ? ? ? ? 00 D9 C1 DD DA 74 00").count(1).get(0).get<bool*>(2);//0x869652
        BordersVar1 = *hook::pattern("A1 ? ? ? ? 85 C0 74 05 83 F8 02 75 1D").count(1).get(0).get<uint32_t*>(1); //0x7E4738
        BordersVar2 = *hook::pattern("DF E0 F6 C4 45 75 00 C7 05").count(1).get(0).get<uint32_t*>(9); //0x7E474C
        FindPlayerVehicle = (int(__cdecl *)()) hook::pattern("0F B6 05 ? ? ? ? 6B C0 2E 8B 0C C5 ? ? ? ? 85 C9 74 10").get(0).get<uint32_t>(0);//0x4BC1E0
        bIsInCutscene = *hook::pattern("80 3D ? ? ? ? ? 74 04 83 C4 38 C3").count(1).get(0).get<bool*>(2); //0x7E46F5
        dwGameLoadState = *dwGameLoadStatePattern.count(1).get(0).get<uint32_t*>(2);
        fCRadarRadarRange = *RadarScalingPattern.count(1).get(0).get<float*>(2);
        SetColorAddr = hook::pattern("53 83 EC 10 8B 5C 24 18 8A 13").count(2).get(1).get<uint32_t>(0);
        bFontColorA = *hook::pattern("C6 05 ? ? ? ? FF EB").count(1).get(0).get<uint8_t*>(2);
        bDropShadowPosition = *hook::pattern("8B 44 24 04 66 A3 ? ? ? ? C3").count(1).get(0).get<uint8_t*>(6); //0x97F860
        bFontDropColorA = *hook::pattern("A2 ? ? ? ? D9 EE D9 05 ? ? ? ? D8 15 ? ? ? ? DF E0").count(1).get(0).get<uint8_t*>(1); //0x97F865
        bBackgroundOn = *hook::pattern("C6 05 ? ? ? ? 01 C3").count(7).get(6).get<uint8_t*>(2); //0x97F83B
        font94B924 = *hook::pattern("66 83 0D ? ? ? ? FF C7 05 ? ? ? ? 00 00 00 00").count(1).get(0).get<uint16_t*>(3); //0x94B924
        CMenuManager_m_PrefsLanguage = *hook::pattern("A1 ? ? ? ? 83 C4 14 83 F8 03 74 0A 83 F8 01").count(1).get(0).get<uint32_t*>(1); //0x869680
}

void SilentPatchCompatibility()
{
    OverwriteResolution();
    auto pattern = hook::pattern("66 8B 2D ? ? ? ? 0F BF C5");
    if (bSmallerTextShadows && *(uint8_t*)pattern.count(1).get(0).get<uint8_t*>(10) != 0x89)
    {
        injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(10), 0x89, true); //0x5516FB
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(11), 0x04DB2404, true); //0x5516FC
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(15), 0xC81DD824, true); //0x5516FC + 4

        pattern = hook::pattern("89 44 24 0C D8 05 ? ? ? ? D9 1D ? ? ? ?");
        injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(16), 0xDB, true); //0x5517C4
        injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(16 + 3), 0x0C, true); //0x5517C7
        pattern = hook::pattern("0F BF C5 D9 1C 24 89 44 24 14 50");
        injector::WriteMemory<uint8_t>(pattern.count(2).get(0).get<uint32_t>(11), 0xDB, true); //0x5517DF
        injector::WriteMemory<uint8_t>(pattern.count(2).get(0).get<uint32_t>(11 + 3), 0x18, true); //0x5517E2
        injector::WriteMemory<uint8_t>(pattern.count(2).get(1).get<uint32_t>(11), 0xDB, true); //0x551848
        injector::WriteMemory<uint8_t>(pattern.count(2).get(1).get<uint32_t>(11 + 3), 0x18, true); //0x55184B
        pattern = hook::pattern("FF 74 24 34 89 44 24 04 53");
        injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(9), 0xDB, true); //0x551832
        injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(9 + 3), 0x08, true); //0x551835
    }
}

injector::hook_back<signed int(__cdecl*)(void)> hbRsSelectDevice;
signed int __cdecl RsSelectDeviceHook2()
{
    SilentPatchCompatibility();
    return hbRsSelectDevice.fun();
}

void OverwriteResolution()
{
    CIniReader iniReader("");
    ResX = iniReader.ReadInteger("MAIN", "ResX", -1);
    ResY = iniReader.ReadInteger("MAIN", "ResY", -1);
    //bSmallerTextShadows = iniReader.ReadInteger("MAIN", "SmallerTextShadows", 1) != 0;

    if (!ResX || !ResY)
        std::tie(ResX, ResY) = GetDesktopRes();
    else if (ResX == -1 || ResY == -1)
        return;

    if(!hbRsSelectDevice.fun)
        hbRsSelectDevice.fun = injector::MakeCALL(ResolutionPattern0.count(1).get(0).get<uint32_t>(0), RsSelectDeviceHook2).get();

    injector::WriteMemory<uint8_t>(ResolutionPattern1.count(1).get(0).get<uint32_t>(2), 22, true);

    injector::WriteMemory(ResolutionPattern2.count(1).get(0).get<uint32_t>(14), ResX, true); //0x600E7B + 0x3
    injector::WriteMemory(ResolutionPattern2.count(1).get(0).get<uint32_t>(24), ResY, true); //0x600E84 + 0x4
    injector::WriteMemory<uint8_t>(ResolutionPattern2.count(1).get(0).get<uint32_t>(34), 32, true); //0x600E8E + 0x4

    injector::WriteMemory(ResolutionPattern3.count(2).get(0).get<uint32_t>(16), ResX, true); //0x602D3A + 0x6
    injector::WriteMemory(ResolutionPattern3.count(2).get(0).get<uint32_t>(26), ResY, true); //0x602D44 + 0x6
    injector::WriteMemory(ResolutionPattern3.count(2).get(1).get<uint32_t>(16), ResX, true); //0x602D4E + 0x6
    injector::WriteMemory(ResolutionPattern3.count(2).get(1).get<uint32_t>(26), ResY, true); //0x602D58 + 0x6

    injector::WriteMemory<uint8_t>(ResolutionPattern4.count(1).get(0).get<uint32_t>(15), 0xEB, true); //jl      short loc_600E60 > jmp      short loc_600E60
    
    injector::MakeNOP(ResolutionPattern5.count(1).get(0).get<uint32_t>(5), 5, true);
    injector::WriteMemory<uint16_t>(ResolutionPattern5.count(1).get(0).get<uint32_t>(5), 0x07EB, true);
}

void FixAspectRatio()
{
    auto pattern = hook::pattern("80 3D ? ? ? ? 00 C7 05 ? ? ? ? 00 00 00 00 74 21"); //0x54A270
    injector::MakeJMP(pattern.count(1).get(0).get<uint32_t>(0), CDraw::CalculateAspectRatio, true);
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
        injector::MakeJMP(pattern.count(1).get(0).get<uint32_t>(0), CDraw::SetFOV, true);

        pattern = hook::pattern("E8 ? ? ? ? D9 EE D9 EE D9 44 24 38 D8 A3 7C 01 00 00 DD D9"); //0x480456 
        injector::MakeCALL(pattern.count(1).get(0).get<uint32_t>(0), funcCCameraAvoidTheGeometryHook, true);

        struct EmergencyVehiclesFix
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.ebx + 0xF4) = fEmergencyVehiclesFix;
            }
        }; injector::MakeInline<EmergencyVehiclesFix>(EmergencyVehiclesFixPattern.count(1).get(0).get<uint32_t>(0), EmergencyVehiclesFixPattern.count(1).get(0).get<uint32_t>(6));


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
        }; injector::MakeInline<RadarScaling>(RadarScalingPattern.count(1).get(0).get<uint32_t>(0), RadarScalingPattern.count(1).get(0).get<uint32_t>(6));
}

void FixMenu()
{
    float fMenuTextScale = 0.0009375000373f / (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
    injector::WriteMemory<float>(*MenuPattern1.count(2).get(1).get<uint32_t*>(16), fMenuTextScale, true);

    float fMenuControlsOptionsTextScale = 0.00062499999f / (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
    injector::WriteMemory<float>(*MenuPattern2.get(MenuPattern2.size() - 6).get<uint32_t*>(10), fMenuControlsOptionsTextScale, true); //MenuPattern2.size() is 9 for steam, 10 for 1.0

    float fMenuControlsOptionsTextScale2 = 0.00039062501f / (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
    injector::WriteMemory<float>(*MenuPattern2.get(MenuPattern2.size() - 8).get<uint32_t*>(10), fMenuControlsOptionsTextScale2, true);

    float fMenuBarsScale = 0.0046875002f / (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
    injector::WriteMemory<float>(*MenuPattern2.get(MenuPattern2.size() - 9).get<uint32_t*>(10), fMenuBarsScale, true);

    float fMenuHeadersSkinsScale = 0.0014062499f / (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
    injector::WriteMemory<float>(*MenuPattern3.count(36).get(35).get<uint32_t*>(10), fMenuHeadersSkinsScale, true);

    /*for (size_t i = 0; i < MenuPattern15625.size(); i++) // moved to fixhud()
    {
        if (i > 0 &&  i < 32)
        {
            uint32_t* p15625 = MenuPattern15625.get(i).get<uint32_t>(2);
            injector::WriteMemory(p15625, &fWideScreenWidthScaleDown, true); //MenuLotsOfStuffScale
        }
    }*/

    float fvcLogoScale = 0.2453125f / (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
    injector::WriteMemory<float>(*MenuPattern4.count(4).get(3).get<uint32_t*>(8), fvcLogoScale, true);
    
    float fMapLegendTextScale = 0.001015625f / (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
    injector::WriteMemory<float>(*MenuPattern5.count(11).get(4).get<uint32_t*>(10), fMapLegendTextScale, true);
    
    float fMapLegendIconsTextScale = 0.000859375f/ (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
    injector::WriteMemory<float>(*MenuPattern5.count(11).get(10).get<uint32_t*>(10), fMapLegendIconsTextScale, true);

    injector::WriteMemory<float>(*MenuPattern6.count(1).get(0).get<uint32_t*>(2), fWideScreenWidthScaleDown, true); //fRadarAndBlipsScale, replaces entire CRadarPattern addresses directly, so no need to use it in HudFix

    float fMapPos = (640.0f * (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f))) / 2.0f;
    injector::WriteMemory<float>(MenuPattern7.count(1).get(0).get<uint32_t>(17), fMapPos, true);
    injector::WriteMemory<float>(*MenuPattern11.count(1).get(0).get<uint32_t*>(2), fMapPos, true);

    float fMapBottomTextScale = 0.00046875002f / (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
    injector::WriteMemory<float>(*MenuPattern8.count(10).get(9).get<uint32_t*>(8), fMapBottomTextScale, true);

    float fSaveLoadListTextScale = 0.00075000001f / (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
    injector::WriteMemory<float>(*MenuPattern9.count(21).get(20).get<uint32_t*>(10), fSaveLoadListTextScale, true);

    float fLoadingGameTextScale = 0.00065625005f / (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
    injector::WriteMemory<float>(*MenuPattern10.count(12).get(11).get<uint32_t*>(10), fSaveLoadListTextScale, true);

    float fCursorScale = 0.0546875f / (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
    injector::WriteMemory<float>(*MenuPattern12.count(22).get(21).get<uint32_t*>(10), fCursorScale, true);

    float fCursorShadowScale = 0.0703125f / (*CDraw::pfScreenAspectRatio / (4.0f / 3.0f));
    injector::WriteMemory<float>(*MenuPattern12.count(22).get(17).get<uint32_t*>(10), fCursorShadowScale, true);
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
    }; injector::MakeInline<RSDH>(RsSelectDevicePattern.count(1).get(0).get<uint32_t>(0), RsSelectDevicePattern.count(1).get(0).get<uint32_t>(7));
}

void FixCoronas()
{
    auto pattern = hook::pattern("D8 0E D9 1E D9 05 ? ? ? ? D8 35 ? ? ? ? D8 0B D9 1B"); //0x57797A 
    injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(1), 0x0B, true);

    auto pfCAutoPreRender = (uint32_t)hook::pattern("FF 35 ? ? ? ? 50 8D 84 24 24 05 00").count(1).get(0).get<uint32_t>(0);
    auto pfCBikePreRender = (uint32_t)hook::pattern("D9 83 FC 03 00 00 D8 1D ? ? ? ? DF E0 F6 C4 45").count(1).get(0).get<uint32_t>(0);
    auto pfCBrightLightsRegisterOne = (uint32_t)hook::pattern("D9 EE D9 EE 83 EC 20 8B 44 24").count(1).get(0).get<uint32_t>(0);
    pattern = hook::range_pattern(pfCAutoPreRender, pfCAutoPreRender + 0x7E89, "E8 ? ? ? ?");
    for (size_t i = 0; i < pattern.size(); ++i)
    {
        auto addr = pattern.get(i).get<uint32_t>(0);
        auto dest = injector::GetBranchDestination(addr, true).as_int();
        if (dest == pfCBrightLightsRegisterOne)
            injector::MakeNOP(addr, 5, true); //CBrightLights::RegisterOne
    }

    pattern = hook::range_pattern(pfCBikePreRender, pfCBikePreRender + 0x2A54, "E8 ? ? ? ? ?");
    for (size_t i = 0; i < pattern.size(); ++i)
    {
        auto addr = pattern.get(i).get<uint32_t>(0);
        auto dest = injector::GetBranchDestination(addr, true).as_int();
        if (dest == pfCBrightLightsRegisterOne)
            injector::MakeNOP(addr, 5, true); //CBrightLights::RegisterOne
    }
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
    injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(0), 12, true);
    pattern = hook::pattern("D9 5A 0C 83 C4 08 5B C2 04 00"); //0x46FE09
    injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(0), 3, true);

    hbDrawBorders.fun = injector::MakeCALL(BordersPattern.get(14).get<uint32_t>(7), DrawBordersForWideScreenHook).get();
    injector::MakeCALL(BordersPattern.get(21).get<uint32_t>(7), DrawBordersForWideScreenHook);
}

void FixHUD()
{
    for (size_t i = 0; i < CDarkelDrawMessagesPattern.size(); i++)
    {
        uint32_t* p15625 = CDarkelDrawMessagesPattern.get(i).get<uint32_t>(2);
        injector::WriteMemory(p15625, &fWideScreenWidthScaleDown, true);
    }

    auto pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24"); //0x42F2B0
    injector::WriteMemory(pattern.count(9).get(8).get<uint32_t>(2), &fWideScreenWidthScaleDown, true);

    pattern = hook::pattern("D8 0D ? ? ? ? 89 44 24 04 8D 44 24 54 50"); //0x4A6417
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &fWideScreenWidthScaleDown, true); //sniper scope border
    pattern = hook::pattern("D8 0D ? ? ? ? D1 F8 52 89 44 24 04"); //0x4A646B
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &fWideScreenWidthScaleDown, true);

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

    pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? 6A ? E8"); //0x5FA15B radio text
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &fWideScreenWidthScaleDown, true);
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(49), &fWideScreenWidthScaleDown, true);
    pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 83 C0 EC"); //0x620C45 replay
    injector::WriteMemory(pattern.count(3).get(2).get<uint32_t>(2), &fWideScreenWidthScaleDown, true);

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

    auto pattern = hook::pattern("D9 05 ? ? ? ? D8 25 ? ? ? ? 69 C0 CC 01 00 00"); //0x46F925
    injector::WriteMemory(pattern.count(2).get(1).get<uint32_t>(2), &fCrosshairPosFactor, true);
    
    pattern = hook::pattern("D8 0D ? ? ? ? D9 5C 24 24"); //0x5575FA
    injector::WriteMemory(pattern.count(8).get(7).get<uint32_t>(2), &fCrosshairPosFactor, true);

    pattern = hook::pattern("D8 0D ? ? ? ? D8 AC 24 AC 00 00 00 DD DB D9 C2 D8 C8 DE C1 D9 FA"); //0x606B25
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &fCrosshairPosFactor, true);
}

uint32_t originalColor, _eax;
uint32_t* jmpAddr;
void __declspec(naked)GetTextOriginalColor()
{
    __asm
    {
        fadd    dword ptr ds : [esp + 38h]
        fstp    dword ptr ds : [esp]
        mov _eax, eax
        mov eax, [esp + 24h]
        mov originalColor, eax
        mov eax, _eax
        jmp jmpAddr
    }
}

injector::hook_back<int (__cdecl*)(float, float, unsigned int, unsigned short *, unsigned short *, float)> hbPrintString;
int __cdecl PrintStringHook(float PosX, float PosY, unsigned int c, unsigned short *d, unsigned short *e, float f)
{
    if (*bBackgroundOn == 1)
        return 0;

    unsigned char originalColorA = (originalColor >> 24) & 0xFF;
    unsigned char originalColorB = (originalColor >> 16) & 0xFF;
    unsigned char originalColorG = (originalColor >> 8) & 0xFF;
    unsigned char originalColorR = originalColor & 0xFF;
    if (!originalColorR && !originalColorG && !originalColorB)
    {
        CRGBA rgba; rgba.colorInt = (uint32_t)0xFFFFFFFFF; rgba.alpha = originalColorA / 10;
        injector::cstd<void(CRGBA*)>::call(SetColorAddr, &rgba);
    }
    int result = hbPrintString.fun(PosX, PosY, c, d, e, f);

    PosX -= 1.0f; PosY -= 1.0f;

    hbPrintString.fun(PosX + 1.0f, PosY, c, d, e, f);
    hbPrintString.fun(PosX - 1.0f, PosY, c, d, e, f);
    hbPrintString.fun(PosX, PosY + 1.0f, c, d, e, f);
    hbPrintString.fun(PosX, PosY - 1.0f, c, d, e, f);
    hbPrintString.fun(PosX + 1.0f, PosY + 1.0f, c, d, e, f);
    hbPrintString.fun(PosX - 1.0f, PosY + 1.0f, c, d, e, f);
    hbPrintString.fun(PosX + 1.0f, PosY - 1.0f, c, d, e, f);
    hbPrintString.fun(PosX - 1.0f, PosY - 1.0f, c, d, e, f);

    *bFontColorA = originalColorA / 2;
    hbPrintString.fun(PosX - 1.0f - 1.0f, PosY, c, d, e, f);
    hbPrintString.fun(PosX, PosY + 1.0f + 1.0f, c, d, e, f);
    *bFontColorA = originalColorA;

    if (ReplaceTextShadowWithOutline > 1)
    {
        hbPrintString.fun(PosX + 2.0f, PosY, c, d, e, f);
        hbPrintString.fun(PosX - 2.0f, PosY, c, d, e, f);
        hbPrintString.fun(PosX, PosY + 2.0f, c, d, e, f);
        hbPrintString.fun(PosX, PosY - 2.0f, c, d, e, f);
        hbPrintString.fun(PosX + 2.0f, PosY + 2.0f, c, d, e, f);
        hbPrintString.fun(PosX - 2.0f, PosY + 2.0f, c, d, e, f);
        hbPrintString.fun(PosX + 2.0f, PosY - 2.0f, c, d, e, f);
        hbPrintString.fun(PosX - 2.0f, PosY - 2.0f, c, d, e, f);

        *bFontColorA = originalColorA / 2;
        hbPrintString.fun(PosX - 2.0f - 1.0f, PosY, c, d, e, f);
        hbPrintString.fun(PosX, PosY + 2.0f + 1.0f, c, d, e, f);
        *bFontColorA = originalColorA;
    }
    return result;
}

injector::hook_back<void(__cdecl*)(float, float, unsigned short*)> hbPrintString2;
void __cdecl PrintStringHook2(float PosX, float PosY, unsigned short* c)
{
    *bDropShadowPosition = 1;
    *bFontDropColorA = 0xFF;
    return hbPrintString2.fun(PosX, PosY, c);
}

void __cdecl SetDropShadowPosition(uint8_t Pos)
{
    *bDropShadowPosition = Pos;
    if (Pos > 1 && Pos < 0x77)
        *bDropShadowPosition = 1;
}

void ApplyIniOptions()
{
    CIniReader iniReader("");
    fHudWidthScale = iniReader.ReadFloat("MAIN", "HudWidthScale", 0.0f); fHudWidthScale == 0.0f ? fHudWidthScale = 0.8f : fHudWidthScale;
    fHudHeightScale = iniReader.ReadFloat("MAIN", "HudHeightScale", 0.0f); fHudHeightScale == 0.0f ? fHudHeightScale = 0.8f : fHudHeightScale;

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

        //moved to delayed changes
        //auto pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? 6A 01 E8"); //0x5FA15B radio text
        //injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &fCustomWideScreenWidthScaleDown, true);
        //injector::WriteMemory(pattern.get(0).get<uint32_t>(49), &fCustomWideScreenWidthScaleDown, true);
        //pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 83 C0 EC"); //0x620C45 replay
        //injector::WriteMemory(pattern.get(2).get<uint32_t>(2), &fCustomWideScreenWidthScaleDown, true);

        for (size_t i = 0; i < CDarkelDrawMessagesPattern.size(); i++)
        {
            uint32_t* p15625 = CDarkelDrawMessagesPattern.get(i).get<uint32_t>(2);
            injector::WriteMemory(p15625, &fCustomWideScreenWidthScaleDown, true);
        }

        for (size_t i = 0; i < CDarkelDrawMessagesPattern2.size(); i++)
        {
            uint32_t* p2232143 = CDarkelDrawMessagesPattern2.get(i).get<uint32_t>(2);
            injector::WriteMemory(p2232143, &fCustomWideScreenHeightScaleDown, true);
        }
    }

    bIVRadarScaling = iniReader.ReadInteger("MAIN", "IVRadarScaling", 0) != 0;
    fRadarWidthScale = iniReader.ReadFloat("MAIN", "RadarWidthScale", 0.0f); fRadarWidthScale == 0.0f ? fRadarWidthScale = 0.9f : fRadarWidthScale;
    if (fRadarWidthScale && !bIVRadarScaling)
    {
        for (size_t i = 0; i < CRadarPattern.size(); i++)
        {
            if ((i >= 0 && i < 6) || i == 19 || i == 33)
            {
                uint32_t* p15625 = CRadarPattern.get(i).get<uint32_t>(2);
                injector::WriteMemory(p15625, &fCustomRadarWidthScale, true);
            }
        }

        injector::WriteMemory(DrawHudHorScalePattern.get(66).get<uint32_t>(2), &fCustomRadarWidthScale, true);
        injector::WriteMemory(DrawHudHorScalePattern.get(67).get<uint32_t>(2), &fCustomRadarWidthScale, true);

        auto pattern = hook::pattern("D8 0D ? ? ? ? DD DA D9 05 ? ? ? ? D8 CA"); //0x4C4200 + 0xF4 + 0x2 + 0x6
        injector::WriteMemory(pattern.count(3).get(2).get<uint32_t>(2), &fPlayerMarkerPos, true);

        pattern = hook::pattern("D8 0D ? ? ? ? DD DA D9 C0 D8 CA"); //0x4C5170 + 0x307 + 0x2 + 0x6
        injector::WriteMemory(pattern.count(2).get(1).get<uint32_t>(2), &fPlayerMarkerPos, true);
    }

    fSubtitlesScale = iniReader.ReadFloat("MAIN", "SubtitlesScale", 0.0f); fSubtitlesScale == 0.0f ? fSubtitlesScale = 0.8f : fSubtitlesScale;
    if (fSubtitlesScale)
    {
        auto pattern = hook::pattern("D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59"); //0x556A02
        injector::WriteMemory<float>(*pattern.count(7).get(6).get<uint32_t*>(2), 1.2f * fSubtitlesScale, true);

        pattern = hook::pattern("D8 0D ? ? ? ? D9 1C 24 DB 05 ? ? ? ? 50"); //0x55B010
        injector::WriteMemory<float>(*pattern.count(23).get(22).get<uint32_t*>(2), 1.22f * fSubtitlesScale, true);

        pattern = hook::pattern("D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? A1 ? ? ? ? 59"); //0x55AF5E
        injector::WriteMemory<float>(*pattern.count(1).get(0).get<uint32_t*>(2), 0.57999998f * fSubtitlesScale, true);
    }

    bRestoreCutsceneFOV = iniReader.ReadInteger("MAIN", "RestoreCutsceneFOV", 1) != 0;
    fCarSpeedDependantFOV = iniReader.ReadFloat("MAIN", "CarSpeedDependantFOV", 0.0f);
    bDontTouchFOV = iniReader.ReadInteger("MAIN", "DontTouchFOV", 0) != 0;

    szForceAspectRatio = iniReader.ReadString("MAIN", "ForceAspectRatio", "auto");
    if (strncmp(szForceAspectRatio, "auto", 4) != 0)
    {
        AspectRatioWidth = std::stoi(szForceAspectRatio);
        AspectRatioHeight = std::stoi(strchr(szForceAspectRatio, ':') + 1);
        fCustomAspectRatioHor = static_cast<float>(AspectRatioWidth);
        fCustomAspectRatioVer = static_cast<float>(AspectRatioHeight);
    }

    bFOVControl = iniReader.ReadString("MAIN", "FOVControl", "1") != 0;
    auto pattern = hook::pattern("DE D9 DE D9 EB E9"); //0x576686 1.0
    if (bFOVControl && pattern.size() > 0)
    {
        FOVControl = pattern.count(1).get(0).get<uint32_t>(6);
        injector::WriteMemory<float>(FOVControl, 1.0f, true);
    }

    nHideAABug = iniReader.ReadInteger("MAIN", "HideAABug", 0);
    if (nHideAABug)
    {
        auto pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 50 E8"); //0x57FAA0
        injector::MakeJMP(injector::ReadRelativeOffset(pattern.count(16).get(15).get<uint32_t>(1), 4, true), Hide1pxAABug, true);
    }

    bSmartCutsceneBorders = iniReader.ReadInteger("MAIN", "SmartCutsceneBorders", 1) != 0;
    if (bSmartCutsceneBorders)
    {
        injector::MakeCALL(BordersPattern.get(14).get<uint32_t>(7), CCamera::DrawBordersForWideScreen); //0x4A61EE 0x54A223
        injector::MakeCALL(BordersPattern.get(21).get<uint32_t>(7), CCamera::DrawBordersForWideScreen);
    }

    ReplaceTextShadowWithOutline = iniReader.ReadInteger("MAIN", "ReplaceTextShadowWithOutline", 0);
    if (ReplaceTextShadowWithOutline)
    {
        auto pattern = hook::pattern("D8 44 24 38 D9 1C 24 E8 ? ? ? ? 83 C4 18"); //0x551850 
        hbPrintString.fun = injector::MakeCALL(pattern.count(1).get(0).get<uint32_t>(7), PrintStringHook).get();
        injector::MakeJMP(pattern.count(1).get(0).get<uint32_t>(0), GetTextOriginalColor, true);
        jmpAddr = pattern.count(1).get(0).get<uint32_t>(7);

        // #167 crashfix (spanish lang)
        pattern = hook::pattern("66 A3 ? ? ? ? A0 ? ? ? ? D9 05");
        struct CrashFix
        {
            void operator()(injector::reg_pack& regs)
            {
                if (*CMenuManager_m_PrefsLanguage == 0)
                    *font94B924 = *(uint16_t*)(regs.esi + 0x2C);
            }
        }; injector::MakeInline<CrashFix>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));

        //textbox
        //pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? 6A 00 E8 ? ? ? ? D9");
        //injector::WriteMemory(pattern.get(0).get<uint32_t>(1), 0xFFFF4B31, true); //background 0x55B59B
        //injector::WriteMemory(pattern.get(0).get<uint32_t>(6), 0xFFFF4AEC, true); //enable shadow 0x55B5A0
        //injector::WriteMemory<uint8_t>(pattern.get(0).get<uint32_t>(11), 0x77, true); //shadow size 0x55B5A5 disabled due to bugs

        pattern = hook::pattern("6A 32 6A 64 6A 64 6A 64 E8");
        injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(1), 0x00, true); // cursor shadow alpha 0x4A35A2 

        pattern = hook::pattern("8D 4C 24 0C 68 FF 00 00 00 6A 00 6A 00 6A 00");
        injector::WriteMemory(pattern.count(3).get(2).get<uint32_t>(5), 0x00000000, true); //radio shadow 0x5FA1A5

        pattern = hook::pattern("E8 ? ? ? ? 83 C4 0C E8 ? ? ? ? 83 C4 18"); //0x5FA28A radio text
        hbPrintString2.fun = injector::MakeCALL(pattern.count(1).get(0).get<uint32_t>(0), PrintStringHook2).get();

        pattern = hook::pattern("E8 ? ? ? ? 8B 85 08 01 00 00 83 C4 0C 3D FF 00 00 00");
        injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(0), 5, true); //menu title shadows 0x49E30E
    }
    pattern = hook::pattern("DD D8 E8 ? ? ? ? 83 C4 0C 6A 00"); //0x55B113 subtitles
    hbPrintString2.fun = injector::MakeCALL(pattern.count(1).get(0).get<uint32_t>(2), PrintStringHook2).get();

    pattern = hook::pattern("A1 ? ? ? ? 3B C3"); //0x65C321
    szForceAspectRatio = iniReader.ReadString("MAIN", "ForceMultisamplingLevel", "");
    if (strncmp(szForceAspectRatio, "max", 3) != 0)
    {
        SelectedMultisamplingLevels = iniReader.ReadInteger("MAIN", "ForceMultisamplingLevel", 0);
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(1), &SelectedMultisamplingLevels, true);
    }
    else
    {
        auto pattern2 = hook::pattern("C7 05 ? ? ? ? 01 00 00 00 C7 05 ? ? ? ? 01 00 00 00"); //0x6DDE1C
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(1), *pattern2.count(3).get(2).get<uint32_t*>(2), true);
    }

    if (iniReader.ReadInteger("MAIN", "FixVehicleLights", 1))
    {
        pattern = hook::pattern("D9 C2 DE CB D9 C2 DE CB D9 05");
        injector::WriteMemory<float>(*pattern.count(1).get(0).get<uint32_t*>(10), 2.0f, true); //car lights stretch 0x69590C
    }

    if (bIVRadarScaling)
    {
        fCustomRadarPosXIV = 40.0f + 31.0f;
        auto pattern = hook::pattern("D8 05 ? ? ? ? DE C1 D9 5C 24 28");
        injector::WriteMemory<float>(*pattern.count(1).get(0).get<uint32_t*>(2), fCustomRadarPosXIV, true); //0x68FD2C
        static float f40 = 40.0f;
        pattern = hook::pattern("83 EC 50 DD D9 D9 05 ? ? ? ? D8 C9");
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(7), &f40, true); //0x4C2996
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(60), &f40, true); //0x4C29CB

        fCustomRadarWidthIV = 94.0f - 5.5f;
        pattern = hook::pattern("D8 0D ? ? ? ? DD D9 D9 C2 D8 C9 D8 0D");
        injector::WriteMemory<float>(*pattern.count(1).get(0).get<uint32_t*>(2), fCustomRadarWidthIV, true); //0x68FD24

        fCustomRadarPosYIV = 116.0f - 7.5f;
        pattern = hook::pattern("D9 05 ? ? ? ? D8 CB DA 2C 24 DE C1");
        injector::WriteMemory<float>(*pattern.count(1).get(0).get<uint32_t*>(2), fCustomRadarPosYIV); //0x68FD34
        fCustomRadarHeightIV = 76.0f + 5.0f;
        pattern = hook::pattern("D9 05 ? ? ? ? D8 C9 DD DB D9 C1 D8 CB");
        injector::WriteMemory<float>(*pattern.count(1).get(0).get<uint32_t*>(2), fCustomRadarHeightIV, true); //0x68FD30

        fCustomRadarRingPosXIV = 34.0f + 31.0f;
        fCustomRadarRingPosXIV2 = fCustomRadarRingPosXIV + 6.0f;
        pattern = hook::pattern("C7 84 24 80 04 00 00 00 00 08 42");
        injector::WriteMemory<float>(pattern.count(1).get(0).get<uint32_t>(7), fCustomRadarRingPosXIV, true); //0x55A956
        pattern = hook::pattern("C7 84 24 A0 04 00 00 00 00 08 42");
        injector::WriteMemory<float>(pattern.count(1).get(0).get<uint32_t>(7), fCustomRadarRingPosXIV, true); //0x55AA94
        pattern = hook::pattern("D8 05 ? ? ? ? D8 05 ? ? ? ? D9 9C 24");
        injector::WriteMemory(pattern.count(2).get(0).get<uint32_t>(2), &fCustomRadarRingPosXIV2, true); //0x55A9AC + 0x2
        injector::WriteMemory(pattern.count(2).get(1).get<uint32_t>(2), &fCustomRadarRingPosXIV2, true); //0x55AAE5 + 0x2

        fCustomRadarRingWidthIV = 94.0f - 5.5f;
        pattern = hook::pattern("D8 0D ? ? ? ? D8 05 ? ? ? ? D8 05 ? ? ? ? D9 9C 24 98 04 00 00");
        injector::WriteMemory<float>(*pattern.count(1).get(0).get<uint32_t*>(2), fCustomRadarRingWidthIV, true); //0x697C1C

        fCustomRadarRingPosYIV = 116.0f - 7.5f;
        pattern = hook::pattern("D9 05 ? ? ? ? D8 CA DA 6C 24 74 51");
        injector::WriteMemory<float>(*pattern.count(1).get(0).get<uint32_t*>(2), fCustomRadarRingPosYIV, true); //0x697C18

        fCustomRadarRingHeightIV = 76.0f + 5.0f;
        pattern = hook::pattern("D9 05 ? ? ? ? D8 CA D8 C1");
        injector::WriteMemory<float>(*pattern.count(1).get(0).get<uint32_t*>(2), fCustomRadarRingHeightIV, true); //0x697C20
    }

    bSmallerTextShadows = iniReader.ReadInteger("MAIN", "SmallerTextShadows", 1) != 0;
    if (bSmallerTextShadows || ReplaceTextShadowWithOutline)
    {
        auto pattern = hook::pattern("8B 44 24 04 66 A3 ? ? ? ? C3"); //0x54FF20
        //injector::WriteMemory(pattern.get(0).get<uint32_t>(0), 0x0001B866, true); // mov ax, 0001
        injector::MakeJMP(pattern.count(1).get(0).get<uint32_t>(0), SetDropShadowPosition, true);
    }

    bAllowAltTabbingWithoutPausing = iniReader.ReadInteger("MAIN", "AllowAltTabbingWithoutPausing", 0) != 0;
    if (bAllowAltTabbingWithoutPausing)
    {
        auto pattern = hook::pattern("A0 ? ? ? ? B9 ? ? ? ? A2 ? ? ? ? A0 ? ? ? ?"); //0x4A4FD0
        injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(0), 0xC3, true); // ret
    }
}

injector::hook_back<void(__cdecl*)(CRect&, CRGBA const&, CRGBA const&, CRGBA const&, CRGBA const&)> hbSetVertices;
void __cdecl SetVerticesHook(CRect& a1, CRGBA const& a2, CRGBA const& a3, CRGBA const& a4, CRGBA const& a5)
{
    uint32_t pTexture = 0;
    _asm mov pTexture, ebx

    if (static_cast<int>(a1.m_fRight) == RsGlobal->MaximumWidth && static_cast<int>(a1.m_fBottom) == RsGlobal->MaximumHeight)
    {
        float fMiddleScrCoord = (float)RsGlobal->MaximumWidth / 2.0f;

        float w = 16.0f;
        float h = 9.0f;

        if (FrontendAspectRatioWidth && FrontendAspectRatioHeight)
        {
            w = (float)FrontendAspectRatioWidth;
            h = (float)FrontendAspectRatioHeight;
        }
        else
        {
            if (pTexture)
            {
                if (*(uint32_t*)pTexture)
                {
                    pTexture = **(uint32_t**)pTexture;
                    w = (float)(*(uint32_t*)(pTexture + 0x28));
                    h = (float)(*(uint32_t*)(pTexture + 0x2C));
                    if (w == h && w > 0 && h > 0)
                    {
                        w = 4.0f;
                        h = 3.0f;
                    }
                }
            }
        }

        a1.m_fTop = 0.0f;
        a1.m_fLeft = fMiddleScrCoord - ((((float)RsGlobal->MaximumHeight * (w / h))) / 2.0f);
        a1.m_fBottom = (float)RsGlobal->MaximumHeight;
        a1.m_fRight = fMiddleScrCoord + ((((float)RsGlobal->MaximumHeight * (w / h))) / 2.0f);

        CSprite2dDrawRect(CRect(-5.0f, a1.m_fBottom, a1.m_fLeft, -5.0f), CRGBA(0, 0, 0, a2.alpha));
        CSprite2dDrawRect(CRect((float)RsGlobal->MaximumWidth, a1.m_fBottom, a1.m_fRight, -5.0f), CRGBA(0, 0, 0, a2.alpha));

        CSprite2dDrawRect(CRect(-5.0f, (float)RsGlobal->MaximumHeight + 5.0f, (float)RsGlobal->MaximumWidth + 5.0f, -5.0f), CRGBA(0, 0, 0, a2.alpha));
        injector::cstd<void(int,int)>::call(pRwRenderStateSet, 8, 0);
    }

    return hbSetVertices.fun(a1, a2, a3, a4, a5);
}

void Fix2DSprites()
{
    CIniReader iniReader("");
    szForceAspectRatio = iniReader.ReadString("MAIN", "FrontendAspectRatio", "auto");
    if (strncmp(szForceAspectRatio, "auto", 4) != 0)
    {
        FrontendAspectRatioWidth = std::stoi(szForceAspectRatio);
        FrontendAspectRatioHeight = std::stoi(strchr(szForceAspectRatio, ':') + 1);
    }
    else
    {
        FrontendAspectRatioWidth = 0;
        FrontendAspectRatioHeight = 0;
    }

    auto pattern = hook::pattern("E8 ? ? ? ? 8B 0B 83 C4 14 85 C9"); //0x578720
    hbSetVertices.fun = injector::MakeCALL(pattern.count(2).get(1).get<uint32_t>(0), SetVerticesHook).get();
    injector::MakeCALL(pattern.count(2).get(0).get<uint32_t>(0), SetVerticesHook); //0x57865C

    pRwRenderStateSet = hook::get_pattern("A1 ? ? ? ? 83 EC 08 83 38 00"); //0x649BA0
}

DWORD WINAPI Init(LPVOID bDelay)
{
    #ifdef _LOG
    logfile.open("GTAVC.WidescreenFix.log");
    #endif // _LOG

    auto pattern = hook::pattern("6A 02 6A 00 6A 00 68 01 20 00 00");
    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    //Immediate changes
    GetPatterns();
    OverwriteResolution();
    DxInputNeedsExclusive.size() > 0 ? injector::WriteMemory<uint32_t>(DxInputNeedsExclusive.count(1).get(0).get<uint32_t>(0), 0xC3C030, true) : nullptr; //mouse fix
    GetMemoryAddresses();
    FixAspectRatio();
    FixFOV();
    RsSelectDeviceHook();
    FixCoronas();
    FixBorders();
    FixHUD();
    FixCrosshair();
    ApplyIniOptions();
    Fix2DSprites();

    //Delayed changes
    struct LoadState
    {
        void operator()(injector::reg_pack& regs)
        {
            static float* pMenuPattern6;
            if (*dwGameLoadState < 9)
            {
                SilentPatchCompatibility();

                //+ another SilentPatchCompatibility (issue #105)
                auto pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? 6A 01 E8"); //0x5FA15B radio text
                injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &fCustomWideScreenWidthScaleDown, true);
                injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(49), &fCustomWideScreenWidthScaleDown, true);
                pattern = hook::pattern("50 D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 DB 05"); //0x5FA145 radio text
                injector::WriteMemory(pattern.get(34).get<uint32_t>(3), &fCustomWideScreenHeightScaleDown, true);
                pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 83 C0 EC"); //0x620C45 replay
                injector::WriteMemory(pattern.count(3).get(2).get<uint32_t>(2), &fCustomWideScreenWidthScaleDown, true);

                //injector::WriteMemory<float>(*MenuPattern6.count(1).get(0).get<uint32_t*>(2), fWideScreenWidthScaleDown, true); //issues/84, copypaste from FixMenu()
                pMenuPattern6 = *MenuPattern6.count(1).get(0).get<float*>(2);

                //WIDESCREEN to BORDERS text
                pattern = hook::pattern("E8 ? ? ? ? DB 05 ? ? ? ? 50 89 C3 D8 0D");
                auto GetTextCall = pattern.count(1).get(0).get<uint32_t>(0);
                auto GetText = injector::GetBranchDestination(GetTextCall, true).as_int();
                auto pfGetText = (wchar_t *(__thiscall *)(int, char *))GetText;
                auto TheText = *pattern.count(1).get(0).get<uint32_t*>(-9);
                wchar_t* ptr = pfGetText((int)TheText, "FED_WIS");
                wcscpy(ptr, L"BORDERS");
            }

            *pMenuPattern6 = fWideScreenWidthScaleDown;

            fCrosshairPosFactor = ((0.52999997f - 0.5f) / ((*CDraw::pfScreenAspectRatio) / (16.0f / 9.0f))) + 0.5f;
            fCrosshairHeightScaleDown = fWideScreenWidthScaleDown * *CDraw::pfScreenAspectRatio;

            fWideScreenHeightScaleDown = 1.0f / 480.0f;
            fCustomWideScreenWidthScaleDown = fWideScreenWidthScaleDown * fHudWidthScale;
            fCustomWideScreenHeightScaleDown = fWideScreenHeightScaleDown * fHudHeightScale;

            fCustomRadarWidthScale = fWideScreenWidthScaleDown * fRadarWidthScale;
            fPlayerMarkerPos = 94.0f * fRadarWidthScale;
            if (bIVRadarScaling)
                fPlayerMarkerPos = (94.0f - 5.5f) * fRadarWidthScale;

            *dwGameLoadState = 9;
        }
    }; injector::MakeInline<LoadState>(dwGameLoadStatePattern.count(1).get(0).get<uint32_t>(0), dwGameLoadStatePattern.count(1).get(0).get<uint32_t>(10));

    if (bDelay)
    {
        pattern = hook::pattern("A1 ? ? ? ? 51 8B 38 50 FF 57 20 85 C0");
        static auto dword_94DDA8 = *pattern.count(1).get(0).get<uint32_t*>(1);
        struct SteamCompat
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *dword_94DDA8;
                CDraw::CalculateAspectRatio();
                FixMenu();
            }
        }; injector::MakeInline<SteamCompat>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(5));
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

