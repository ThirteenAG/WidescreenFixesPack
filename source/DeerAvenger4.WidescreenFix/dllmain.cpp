#include "stdafx.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
} Screen;

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("83 EC 58 53 56 57 89 65 E8");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

    pattern = hook::pattern("D9 05 ? ? ? ? D8 73 68 D9 05 ? ? ? ? D8 73 6C D9 C9"); //0x4460E7
    struct SetScaleHook
    {
        void operator()(injector::reg_pack& regs)
        {
            static float fScaleValue = ((1.0f / Screen.fAspectRatio) * (4.0f / 3.0f));
            _asm fld    dword ptr[fScaleValue]
        }
    }; injector::MakeInline<SetScaleHook>(pattern.get_first(0), pattern.get_first(6));

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