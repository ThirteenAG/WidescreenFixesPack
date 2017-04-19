#include "stdafx.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    int32_t Width43;
    float fAspectRatio;
    float fAspectRatioDiff;
    float fFieldOfView;
    float fHUDScaleX;
    float fHudOffset;
    float fHudOffsetReal;
} Screen;

void _declspec(naked) asm_res_fix1()
{
    _asm
    {
        mov     eax, 800
        ret
    }
}

void _declspec(naked) asm_res_fix2()
{
    _asm
    {
        mov     eax, 600
        ret
    }
}

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("89 1D ? ? ? ? A3 ? ? ? ? 88 0D");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fAspectRatioDiff = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
    Screen.fHUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
  
    //Resolution 511CB2
    static int32_t* dword_913250 = *pattern.get_first<int32_t*>(2);
    static int32_t* dword_913254 = *pattern.get_first<int32_t*>(7);
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_913250 = Screen.Width;
            *dword_913254 = Screen.Height;
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(11));
        

    injector::MakeCALL(0x465360, asm_res_fix1, true);
    injector::MakeCALL(0x46536A, asm_res_fix2, true);
    injector::MakeCALL(0x477D37, asm_res_fix1, true);
    injector::MakeCALL(0x477D31, asm_res_fix2, true);
    
    injector::MakeNOP(0x4DD6CB, 2, true);
    injector::MakeNOP(0x4DD6ED, 2, true);
    injector::WriteMemory(0x4DD6CD + 0x3, Screen.Width, true);
    injector::WriteMemory(0x4DD6EF + 0x4, Screen.Height, true);

    //static int x = 800;
    //static int y = 600;
    //injector::WriteMemory(0x510A00 + 1, &x, true);
    //injector::WriteMemory(0x510A10 + 1, &y, true);

    injector::MakeNOP(0x4DD5B9, 5, true); // no intro



    auto retX = []() -> int
    {
        return 800;
    };

    auto retY = []() -> int
    {
        return 600;
    };

    pattern = hook::pattern("E8 ? ? ? ? ? E8");
    for (size_t i = 0; i < pattern.size(); ++i)
    {
        auto addr1 = pattern.get(i).get<uint32_t>(0);
        auto dest1 = injector::GetBranchDestination(addr1, true).as_int();
        auto addr2 = pattern.get(i).get<uint32_t>(6);
        auto dest2 = injector::GetBranchDestination(addr2, true).as_int();

        if (dest1 == 0x510A10 && dest2 == 0x510A00)
        {
            injector::MakeCALL(addr1, static_cast<int(*)()>(retY), true);
            injector::MakeCALL(addr2, static_cast<int(*)()>(retX), true);
        }
    }



    injector::MakeCALL(0x47CEC3, static_cast<int(*)()>(retX), true);
    injector::MakeCALL(0x47CEDA, static_cast<int(*)()>(retY), true);



    injector::MakeCALL(0x47D101, static_cast<int(*)()>(retX), true);
    injector::MakeCALL(0x47D118, static_cast<int(*)()>(retY), true);

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