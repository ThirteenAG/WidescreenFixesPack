#include "stdafx.h"

struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int32_t nWidth43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fHudOffsetReal;
    float fTextScale;
} Screen;

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("8B 44 24 04 85 C0 75 ? 8B 44 24");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    CIniReader iniReader("");
    Screen.nWidth = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.nHeight = iniReader.ReadInteger("MAIN", "ResY", 0);

    if (!Screen.nWidth || !Screen.nHeight)
        std::tie(Screen.nWidth, Screen.nHeight) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.nWidth);
    Screen.fHeight = static_cast<float>(Screen.nHeight);
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
    Screen.nWidth43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
    Screen.fHudScale = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
    Screen.fTextScale = (1.0f / 640.0f) / Screen.fHudScale;

    //Resolution
    struct ResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(int32_t*)(regs.esp + 0x08) = Screen.nWidth;
            *(int32_t*)(regs.esp + 0x0C) = Screen.nHeight;
        }
    }; injector::MakeInline<ResHook>(pattern.get_first(6), pattern.get_first(28)); //0x53F6D6

    pattern = hook::pattern("89 BB 5C 03 00 00");
    struct ResHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *(int32_t*)(regs.ebx + 0x35C) = Screen.nWidth;
            *(int32_t*)(regs.ebx + 0x360) = Screen.nHeight;
        }
    }; injector::MakeInline<ResHook2>(pattern.get_first(0), pattern.get_first(6)); //0x53F6D6

    //Aspect Ratio
    pattern = hook::pattern("68 ? ? ? ? 8B CE E8 ? ? ? ? 51 D9 1C 24");
    injector::WriteMemory<float>(pattern.get_first(1), Screen.fAspectRatio, true);

    pattern = hook::pattern("F3 0F 10 05 ? ? ? ? 8B C7 99");
    injector::WriteMemory<float>(*pattern.get_first<void*>(4), Screen.fAspectRatio, true);

    //Text Scale
    pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 68");
    injector::WriteMemory<float>(*pattern.get_first<void*>(2), Screen.fTextScale, true); //0x5E0134

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