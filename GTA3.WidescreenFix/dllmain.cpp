#include "..\includes\stdafx.h"
#include "..\includes\GTA\common.h"
#include "..\includes\hooking\Hooking.Patterns.h"
#include "..\includes\GTA\global.h"

//#define _LOG
#ifdef _LOG
#include <fstream>
ofstream logfile;
#endif // _LOG

bool bDelay;
auto& gvm = injector::address_manager::singleton();

hook::pattern dwGameLoadStatePattern, DxInputNeedsExclusive, EmergencyVehiclesFixPattern, RadarScalingPattern;
hook::pattern MenuPattern, MenuPattern15625, RsSelectDevicePattern, CDarkelDrawMessagesPattern, CDarkelDrawMessagesPattern2, CParticleRenderPattern;
hook::pattern DrawHudHorScalePattern, DrawHudVerScalePattern, CSpecialFXRender2DFXsPattern, CSceneEditDrawPattern, sub61DEB0Pattern;
hook::pattern MenuPattern1, MenuPattern2, MenuPattern3, MenuPattern4, MenuPattern5, MenuPattern6, MenuPattern7, MenuPattern8, MenuPattern9, MenuPattern10;
hook::pattern ResolutionPattern0, ResolutionPattern1, ResolutionPattern2, ResolutionPattern3, ResolutionPattern4, ResolutionPattern5;
hook::pattern CRadarPattern, BordersPattern;
uint32_t* dwGameLoadState;
float* fCRadarRadarRange;
uint32_t funcCCameraAvoidTheGeometryJmp;
uint32_t* SetColorAddr;
uint8_t* bFontColorA;
uint8_t* bDropShadowPosition;
uint8_t* bFontDropColorA;
void OverwriteResolution();

void GetPatterns()
{

}

void GetMemoryAddresses()
{

}

void SilentPatchCompatibility()
{

}

injector::hook_back<signed int(__cdecl*)(void)> hbRsSelectDevice;
signed int __cdecl RsSelectDeviceHook2()
{

}

void OverwriteResolution()
{

}

void FixAspectRatio()
{

}

void __declspec(naked) funcCCameraAvoidTheGeometryHook()
{

}

void FixFOV()
{

}

void FixMenu()
{

}

void RsSelectDeviceHook()
{

}

void FixCoronas()
{

}

injector::hook_back<void(__fastcall*)(void*)> hbDrawBorders;
static void __fastcall DrawBordersForWideScreenHook(void* _this)
{

}

void FixBorders()
{

}

void FixHUD()
{
 
}

void FixCrosshair()
{

}

uint32_t originalColor, _eax;
uint32_t* jmpAddr;
void __declspec(naked)GetTextOriginalColor()
{

}

injector::hook_back<int (__cdecl*)(float, float, unsigned int, unsigned short *, unsigned short *, float)> hbPrintString;
int __cdecl PrintStringHook(float PosX, float PosY, unsigned int c, unsigned short *d, unsigned short *e, float f)
{
 
}

injector::hook_back<void(__cdecl*)(float, float, unsigned short*)> hbPrintString2;
void __cdecl PrintStringHook2(float PosX, float PosY, unsigned short* c)
{

}

void ApplyIniOptions()
{

}

DWORD WINAPI Init(LPVOID)
{
    #ifdef _LOG
    logfile.open("GTAVC.WidescreenFix.log");
    #endif // _LOG

    auto pattern = hook::pattern("");
    if (!(pattern.size() > 0) && !bDelay)
    {
        bDelay = true;
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
        return 0;
    }

    if (bDelay)
    {
        while (!(pattern.size() > 0))
            pattern = hook::pattern("");
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
            if (bIVRadarScaling)
                fPlayerMarkerPos = (94.0f - 5.5f) * fRadarWidthScale;
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

