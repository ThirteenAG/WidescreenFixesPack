#include "stdafx.h"

struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int Width43;
    float fWidth43;
    float fHudScale;
} Screen;

void __declspec(naked) Ret4()
{
    __asm ret 4
}

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.Width43);

    //ini res crash fix
    auto pattern = hook::pattern("74 21 8B 4C 24 10 8B 54 24 0C 8B 74 24 08"); //0x62AC16
    injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(0), 0xEB, true);

    //display crashfix
    pattern = hook::pattern("E8 ? ? ? ? 8B 96 0C 0B 00 00 8D 8E 0C 0B 00 00 6A 00"); //0x58063B
    injector::MakeCALL(pattern.count(1).get(0).get<uint32_t>(0), Ret4, true);

    pattern = hook::pattern("89 96 2C 05 00 00 8B 41 18"); //0x62BC08 
    if (pattern.size() > 0)
    {
        struct SetResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uint32_t*)(regs.esi + 0x528) = Screen.Width;
                *(uint32_t*)(regs.esi + 0x52C) = Screen.Height;
            }
        }; injector::MakeInline<SetResHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
    }
    else
    {
        pattern = hook::pattern("89 91 2C 05 00 00 8B 46 18 8B 50 04"); //0x61D630
        if (pattern.size() > 0)
        {
            struct SetResHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint32_t*)(regs.ecx + 0x528) = Screen.Width;
                    *(uint32_t*)(regs.ecx + 0x52C) = Screen.Height;
                }
            }; injector::MakeInline<SetResHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
        }
    }
    pattern = hook::pattern("8D 99 28 05 00 00 8B 49 24 53"); //0x62C1F2
    struct SetResHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ebx = regs.ecx + 0x528;
            *(uint32_t*)(regs.ecx + 0x528) = Screen.Width;
            *(uint32_t*)(regs.ecx + 0x52C) = Screen.Height;
        }
    }; injector::MakeInline<SetResHook2>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));


    pattern = hook::pattern("89 0D ? ? ? ? 8B 56 04 89 15 ? ? ? ? 8B 4E 0C"); //0x5792C4

    injector::WriteMemory(*pattern.count(1).get(0).get<uint32_t*>(2), Screen.Width, true);
    injector::WriteMemory(*pattern.count(1).get(0).get<uint32_t*>(11), Screen.Height, true);

    injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(0), 6, true);
    injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(9), 6, true);

    pattern = hook::pattern("8B D8 53 8D ? 24 10 68 ? ? ? ? ? E8"); //0x62FCD5
    struct SetIniResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ebx = Screen.Width;
            regs.edx = regs.esp + 0x10;
        }
    }; injector::MakeInline<SetIniResHook>(pattern.count(3).get(1).get<uint32_t>(0), pattern.count(3).get(1).get<uint32_t>(7));
    injector::WriteMemory<uint8_t>(pattern.count(3).get(1).get<uint32_t>(5), 0x53, true); //push ebx

    struct SetIniResHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ebx = Screen.Height;
            regs.ecx = regs.esp + 0x10;
        }
    }; injector::MakeInline<SetIniResHook2>(pattern.count(3).get(2).get<uint32_t>(0), pattern.count(3).get(2).get<uint32_t>(7));
    injector::WriteMemory<uint8_t>(pattern.count(3).get(2).get<uint32_t>(5), 0x53, true); //push ebx
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init, hook::pattern("BF 94 00 00 00 8B C7"));
        });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}