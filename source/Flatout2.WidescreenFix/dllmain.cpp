#include "stdafx.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHudScale;
    float fHudOffset;
    float fTextOffset;
    float fFMVOffset;
} Screen;

injector::memory_pointer_raw pfShowIntro;

void ShowIntroHook()
{
    CIniReader iniReader("");
    bool bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    float fIniFOVFactor = iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f);

    auto ptr = *hook::pattern("C7 05 ? ? ? ? 00 04 00 00 C7 05 ? ? ? ? 00 03 00 00 C7 05 ? ? ? ? 10 00 00 00").get_first<int32_t*>(2);
    Screen.Width = *ptr;
    Screen.Height = *(ptr + 1);

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
    Screen.fHudScale = 1.0f / (480.0f * Screen.fAspectRatio);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;

    //Aspect Ratio
    auto pattern = hook::pattern("D9 05 ? ? ? ? 8B 44 24 04 8B 4C 24");
    injector::WriteMemory(pattern.get_first<float>(2), &Screen.fWidth, true);
    injector::WriteMemory(pattern.get_first<float>(18), &Screen.fHeight, true);

    //Hud Scale
    pattern = hook::pattern("50 D8 0D ? ? ? ? D9 5C 24 20 DB 47 0C");
    injector::WriteMemory<float>(*pattern.get_first<float*>(3), Screen.fHudScale, true);
    pattern = hook::pattern("D9 05 ? ? ? ? 56 8B F1 8B 86"); //53BBF3
    injector::WriteMemory(pattern.get_first(2), &Screen.fHudOffset, true);

    //FOV
    float fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * Screen.fAspectRatio)) * (1.0f / SCREEN_FOV_HORIZONTAL);
    static float fFOVFactor = fDynamicScreenFieldOfViewScale * 0.017453292f * fIniFOVFactor;
    auto pfSetFOV = hook::pattern("D8 0D ? ? ? ? 55 8B EF 2B DD C1 FB 02 8D 68 01 3B EB").get_first(2);
    injector::WriteMemory(pfSetFOV, &fFOVFactor, true);

    if (!bSkipIntro)
    {
        auto ShowIntro = pfShowIntro.as_int() + 6;
        _asm sub esp, 0x104
        _asm jmp ShowIntro
    }
}

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("81 EC 04 01 00 00 8B 15");

    if (pattern.empty() && !bDelay)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    pfShowIntro = pattern.get_first(0);
    injector::MakeJMP(pfShowIntro, ShowIntroHook, true);

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
