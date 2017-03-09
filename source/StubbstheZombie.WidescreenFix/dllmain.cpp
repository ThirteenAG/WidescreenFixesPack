#include "stdafx.h"
#include <string>
#include <set>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>

HWND hWnd;
bool bDelay;

#define _USE_MATH_DEFINES
#include "math.h"
#define DEGREE_TO_RADIAN(fAngle) \
    ((fAngle)* (float)M_PI / 180.0f)
#define RADIAN_TO_DEGREE(fAngle) \
    ((fAngle)* 180.0f / (float)M_PI)
#define SCREEN_AR_NARROW			(4.0f / 3.0f)	// 640.0f / 480.0f
#define SCREEN_FOV_HORIZONTAL		60.0f
#define SCREEN_FOV_VERTICAL			(2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / SCREEN_AR_NARROW)))

static char* GLExeStrings = "GL_ALWAYS GL_APPLE_transform_hint GL_ARB_fragment_program GL_ARB_multisample GL_ARB_multitexture GL_ARB_occlusion_query \
GL_ARB_texture_compression GL_ARB_texture_mirrored_repeat GL_ARB_vertex_buffer_object GL_ATI_array_rev_comps_in_4_bytes GL_EXT_compiled_vertex_array \
GL_EXT_texture_compression_s3tc GL_EXT_texture_env_add GL_EXT_texture_env_combine GL_EXT_texture_filter_anisotropic GL_EXT_texture_lod_bias GL_EXT_texture_rectangle \
GL_GEQUAL GL_GREATER GL_LEQUAL GL_LESS GL_NEVER GL_NOTEQUAL GL_NV_multisample_filter_hint GL_NV_register_combiners2 GL_NV_texgen_reflection GL_NV_texture_rectangle \
GL_NV_texture_shader GL_SGIS_texture_lod GL_TEXTURE_GEN_Q GL_TEXTURE_GEN_R GL_TEXTURE_GEN_S GL_TEXTURE_GEN_T";

struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    float fScaling;
    float fTextAlignment;
} Screen;

float f1 = 1.0f;
float f0031 = 0.0031298904f;
float fElementWidth1, fElementWidth2;
uintptr_t GUIHookJmp, _EAX;
void __declspec(naked) GUIHook()
{
    _asm mov _EAX, eax
    _asm mov eax, dword ptr[esp + 0x13C]
    _asm mov fElementWidth1, eax
    _asm mov eax, dword ptr[esp + 0x230]
    _asm mov fElementWidth2, eax
    
    if (fElementWidth1 == 640.0f || fElementWidth2 == 640.0f || fElementWidth1 == 641.0f || fElementWidth2 == 641.0f)
    {
        _asm mov eax, f0031
        _asm mov  dword ptr[esp + 0x20], eax
        _asm fadd f1
    }
    else
    {
        _asm mov eax, Screen.fScaling
        _asm mov dword ptr[esp + 0x20], eax
        _asm fadd Screen.fTextAlignment
    }
    _asm mov eax, _EAX
    _asm test al, al
    _asm jmp GUIHookJmp
}

DWORD WINAPI Init(LPVOID)
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);

    auto pattern = hook::pattern("BF 94 00 00 00 8B C7");
    if (!(pattern.size() > 0) && !bDelay)
    {
        bDelay = true;
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
        return 0;
    }

    if (!Screen.Width || !Screen.Height) {
        HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO info;
        info.cbSize = sizeof(MONITORINFO);
        GetMonitorInfo(monitor, &info);
        Screen.Width = info.rcMonitor.right - info.rcMonitor.left;
        Screen.Height = info.rcMonitor.bottom - info.rcMonitor.top;
    }

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

    pattern = hook::pattern("8D 94 24 A0 00 00 00 8D 49 00"); //0x00561986
    struct GLCrashFix
    {
        void operator()(injector::reg_pack& regs)
        {
            std::istringstream s1(GLExeStrings);
            std::istringstream s2((char*)regs.eax);
            std::set<std::string> ext1((std::istream_iterator<std::string>(s1)), std::istream_iterator<std::string>());
            std::set<std::string> ext2((std::istream_iterator<std::string>(s2)), std::istream_iterator<std::string>());

            static std::ostringstream is;
            std::set_intersection(ext1.begin(), ext1.end(),	ext2.begin(), ext2.end(), std::ostream_iterator<std::string>(is, " "));

            regs.eax = (uintptr_t)is.str().c_str();
            regs.edx = regs.esp + 0xA0;
        }
    }; injector::MakeInline<GLCrashFix>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(7));

    pattern = hook::pattern("66 8B 0D ? ? ? ? 66 89 8E 68 09 00 00 66 8B 15"); //0x54F0FE
    injector::WriteMemory<uint16_t>(*pattern.get(0).get<uint16_t*>(3), (uint16_t)Screen.Width, true);
    injector::WriteMemory<uint16_t>(*pattern.get(0).get<uint16_t*>(17), (uint16_t)Screen.Height, true);

    pattern = hook::pattern("66 C7 86 68 09 00 00 80 02 66 C7 86 6A 09 00 00 E0 01"); //0x54F150
    injector::WriteMemory<uint16_t>(pattern.get(0).get<uint16_t>(7), (uint16_t)Screen.Width, true);
    injector::WriteMemory<uint16_t>(pattern.get(0).get<uint16_t>(16), (uint16_t)Screen.Height, true);

    pattern = hook::pattern("0F BF 81 68 09 00 00 89 02 0F BF 81 6A 09 00 00"); //0x4ABE30
    struct PatchRes
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)regs.edx = (uint16_t)Screen.Width;
            regs.eax = (uint16_t)Screen.Height;
        }
    }; injector::MakeInline<PatchRes>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(16));

    //Text
    Screen.fScaling = ((1.0f / (480.0f * (Screen.fWidth / Screen.fHeight))) * 2.0f);
    Screen.fTextAlignment = Screen.fScaling * ((480.0f * (4.0f / 3.0f)) / 2.0f);

    pattern = hook::pattern("C7 44 24 10 CD CC 4C 3B C7 44 24 14 00 00 00 00"); //0x5755E1 + 0x4
    injector::WriteMemory<float>(pattern.get(0).get<uint32_t>(4), Screen.fScaling, true);
    pattern = hook::pattern("D8 05 ? ? ? ? D8 EA D9 5C 24 1C DB 44 24 0C"); //0x575671 + 0x2
    injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &Screen.fTextAlignment, true);

    //GUI
    pattern = hook::pattern("D8 05 ? ? ? ? D8 EA D9 5C 24 2C DB 44 24 10"); //0x565494
    injector::MakeJMP(pattern.get(0).get<uint32_t>(0), GUIHook, true); //makeinline doesn't work
    GUIHookJmp = (uintptr_t)pattern.get(0).get<uint32_t>(6); \

    //FOV
    Screen.fFieldOfView = 2.8f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * (Screen.fAspectRatio))) * (1.0f / SCREEN_FOV_HORIZONTAL);
    pattern = hook::pattern("D8 3D ? ? ? ? D9 C0 D8 8F ? ? ? ? D9 9F ? ? ? ? D9 C0"); //0x466AB0
    injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &Screen.fFieldOfView, true);
    injector::WriteMemory(pattern.get(1).get<uint32_t>(2), &Screen.fFieldOfView, true);
    pattern = hook::pattern("D9 05 ? ? ? ? D8 F1 D9 44 24 4C D8 C9"); //0x4669CD
    injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &Screen.fFieldOfView, true);
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