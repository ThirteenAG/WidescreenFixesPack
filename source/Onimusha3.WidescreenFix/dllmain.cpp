#include "stdafx.h"

struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    float fHudOffset1;
    float fHudOffset2;
    float fFMVOffset1;
    float fFMVOffset2;
    float fFMVOffset3;
    float fFMVOffset4;
    int Width43;
    float fWidth43;
    float f3DHorScale;
} Screen;

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bFMVFullscreenMode = iniReader.ReadInteger("MAIN", "FMVFullscreenMode", 1) != 0;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.Width43);

    auto pattern = hook::pattern("75 0C 68 ? ? ? ? 68 ? ? ? ? EB 0A"); //406584
    injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(0), 2, true);
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(3), Screen.Height, true);
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(8), Screen.Width, true);

    pattern = hook::pattern("8B 04 C5 ? ? ? ? 89 0D ? ? ? ? A3"); //4957DC
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = Screen.Width;
            regs.eax = Screen.Height;
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(-2), pattern.get_first(7));
    injector::WriteMemory<uint16_t>(pattern.get_first(3), 0xD13B, true);

    injector::WriteMemory(*pattern.count(1).get(0).get<uint32_t*>(9), Screen.Width, true);
    injector::WriteMemory(*pattern.count(1).get(0).get<uint32_t*>(14), Screen.Height, true);

    //clipping fix
    static float fNegResX = -Screen.fWidth;
    static float fNegResY = -Screen.fHeight;
    pattern = hook::pattern("F3 0F 10 05 ? ? ? ? 8D 54 24 10 8D 4C 24 70"); //479982
    injector::WriteMemory(pattern.get_first(4), &fNegResX, true);
    pattern = hook::pattern("F3 0F 10 05 ? ? ? ? 8D 54 24 10 8D 8C 24 90"); //479A0C
    injector::WriteMemory(pattern.get_first(4), &fNegResY, true);

    //static float f3DHorScale = 1.0f * (Screen.fAspectRatio / (4.0f / 3.0f));
    //pattern = hook::pattern("F3 0F 10 15 ? ? ? ? F3 0F 5E 54 24"); //40B504 
    //injector::WriteMemory(pattern.get(0).get<uint32_t>(4), &f3DHorScale, true);
    static float f2 = 2.0f;
    Screen.f3DHorScale = 2.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
    pattern = hook::pattern("F3 0F 10 0D ? ? ? ? 0F 28 54 24 30"); //40B504 
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(4), &Screen.f3DHorScale, true);

    pattern = hook::pattern("F3 0F 10 2D ? ? ? ? F3 0F 11 64 24 64 F3 0F 2A E1"); //41104C
    struct Set2DScale
    {
        void operator()(injector::reg_pack& regs)
        {
            if ((*(uint32_t*)(0x02ADB7B8 + 0x0) == 100 && (*(uint32_t*)(0x02ADB7B8 + 0x8) == 70) && (*(uint32_t*)(0x02ADB7B8 + 0x38) == 524288 || *(uint32_t*)(0x02ADB7B8 + 0x38) == 524296) && (*(uint32_t*)(0x02ADB7B8 + 0xB8) == 1)) //generic
                || (*(uint32_t*)(0x02ADB7B8 + 0x0) == 0 && *(uint32_t*)(0x02ADB7B8 + 0x4) == 0 && *(uint32_t*)(0x02ADB7B8 + 0x8) == 6 && (*(uint32_t*)(0x02ADB7B8 + 0x38) == 524288 || *(uint32_t*)(0x02ADB7B8 + 0x38) == 524296) && (*(uint32_t*)(0x02ADB7B8 + 0xB8) == 1 || *(uint32_t*)(0x02ADB7B8 + 0xB8) == 65536)) //cutscene borders
                || (*(uint32_t*)(0x02ADB7B8 + 0x0) == 100 && *(uint32_t*)(0x02ADB7B8 + 0x8) == 70 && (*(uint32_t*)(0x02ADB7B8 + 0x38) == 524288 || *(uint32_t*)(0x02ADB7B8 + 0x38) == 524296) && (*(uint32_t*)(0x02ADB7B8 + 0xB8) == 65536)) //cutscene fading
                || (*(uint32_t*)(0x02ADB7B8 + 0x0) == 68 && *(uint32_t*)(0x02ADB7B8 + 0x4) == 0 && *(uint32_t*)(0x02ADB7B8 + 0x8) == 6) //player shadow
                )
                _asm movss   xmm5, f2
            else
                _asm movss   xmm5, Screen.f3DHorScale
        }
    }; injector::MakeInline<Set2DScale>(pattern.get_first(0), pattern.get_first(8));

    Screen.fHudOffset1 = (Screen.fWidth - Screen.fWidth43) / 2.0f;
    Screen.fHudOffset2 = Screen.fHudOffset1 + Screen.fWidth43;
    pattern = hook::pattern("8B 08 8B 91 0C 01 00 00 6A 00 50"); //412FC7
    struct BackgroundsHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.esp + 0x0C) = Screen.fHudOffset1;
            *(float*)(regs.esp + 0x60) = Screen.fHudOffset1;
            *(float*)(regs.esp + 0x28) = Screen.fHudOffset2;
            *(float*)(regs.esp + 0x44) = Screen.fHudOffset2;

            regs.ecx = *(uintptr_t*)regs.eax;
            regs.edx = *(uintptr_t*)(regs.ecx + 0x10C);
        }
    }; injector::MakeInline<BackgroundsHook>(pattern.get_first(0), pattern.get_first(8));

    //movies
    if (bFMVFullscreenMode)
    {
        float fB = (Screen.fHeight - (Screen.fHeight / (640.0f / 480.0f))) / 2.0f;
        float fA = fB * (640.0f / 480.0f);
        Screen.fFMVOffset1 = Screen.fHudOffset1 - fA;
        Screen.fFMVOffset2 = Screen.fHudOffset2 + fA;
        Screen.fFMVOffset3 = 0.0f - fB;
        Screen.fFMVOffset4 = Screen.fHeight + fB;
    }
    else
    {
        Screen.fFMVOffset1 = Screen.fHudOffset1;
        Screen.fFMVOffset2 = Screen.fHudOffset2;
        Screen.fFMVOffset3 = 0.0f;
        Screen.fFMVOffset4 = Screen.fHeight;
    }
    pattern = hook::pattern("F3 0F 11 64 24 70 F3 0F 11 44 24 74 8B 08 6A 00"); //412E6F
    struct MoviesHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.esp + 0x08) = Screen.fFMVOffset1;
            *(float*)(regs.esp + 0x5C) = Screen.fFMVOffset1;
            *(float*)(regs.esp + 0x24) = Screen.fFMVOffset2;
            *(float*)(regs.esp + 0x40) = Screen.fFMVOffset2;

            *(float*)(regs.esp + 0x0C) = Screen.fFMVOffset3;
            *(float*)(regs.esp + 0x28) = Screen.fFMVOffset3;
            *(float*)(regs.esp + 0x60) = Screen.fFMVOffset4;
            *(float*)(regs.esp + 0x44) = Screen.fFMVOffset4;

            *(float*)(regs.esp + 0x70) = 0.0f;
            *(float*)(regs.esp + 0x74) = 1.0f;
        }
    }; injector::MakeInline<MoviesHook>(pattern.get_first(0), pattern.get_first(12));

    //disable "Effect" option overlay
    pattern = hook::pattern("0F 84 FC 00 00 00 8D 44 24 60 33 ED"); //5686E4
    injector::WriteMemory<uint8_t>(pattern.get_first(-1), 10, true);
    injector::WriteMemory<uint8_t>(pattern.get_first(1), 0x82, true); //jb
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("81 EC 04 01 00 00 53 55 56"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {

    }
    return TRUE;
}