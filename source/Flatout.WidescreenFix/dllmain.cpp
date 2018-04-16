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
    auto pattern = hook::pattern("8B 44 24 04 8B 4C 24 08 C7 00 00 00 80");
    injector::WriteMemory<float>(pattern.get_first<float>(10 + 0), Screen.fWidth, true);
    injector::WriteMemory<float>(pattern.get_first<float>(10 + 6), Screen.fHeight, true);

    //Hud Scale
    pattern = hook::pattern("D8 0D ? ? ? ? DB 40 0C C7 05 ? ? ? ? 00 80 F8 43");
    injector::WriteMemory<float>(*pattern.get_first<float*>(2), Screen.fHudScale, true);
    pattern = hook::pattern("D9 05 ? ? ? ? 56 8B F1 8B 86"); //4E23A0
    injector::WriteMemory(pattern.get_first(2), &Screen.fHudOffset, true);

    //pause menu centering
    auto TempPtr = *hook::pattern("D8 0D ? ? ? ? D9 5C 24 14 D9 44 24 10 D8 0D ? ? ? ? D9 5C 24 18").get_first<float*>(2); //667FA4
    injector::WriteMemory<float>(TempPtr, *(float*)TempPtr + Screen.fHudOffset, true); //background

    TempPtr = *hook::pattern("D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 8B 54 24 08 8B 44 24 0C 89 7E 04 5F").get_first<float*>(2); //667FAC
    injector::WriteMemory<float>(TempPtr, *(float*)TempPtr + Screen.fHudOffset, true); //PAUSED string

    TempPtr = *hook::pattern("D8 0D ? ? ? ? D9 1C 24 55 E8 ? ? ? ? 8B 4C 24 20 FF 56 44").get_first<float*>(2); //667F98
    injector::WriteMemory<float>(TempPtr, *(float*)TempPtr + Screen.fHudOffset, true); //menu text

    TempPtr = *hook::pattern("D8 0D ? ? ? ? D8 E1 D9 1C 24 57 DD D8").get_first<float*>(2); //6681A8
    injector::WriteMemory<float>(TempPtr, *(float*)TempPtr + Screen.fHudOffset, true); //press enter to start text

    TempPtr = *hook::pattern("D8 0D ? ? ? ? D9 1C 24 50 57").get_first<float*>(2); //6681A8
    injector::WriteMemory<float>(TempPtr, *(float*)TempPtr + Screen.fHudOffset, true); //sound options text

    TempPtr = *hook::pattern("D8 0D ? ? ? ? 83 EC 08 8B F7 D9 5C 24 20").get_first<float*>(2); //667FC0
    injector::WriteMemory<float>(TempPtr, *(float*)TempPtr + Screen.fHudOffset, true); //sound options sliders

    //wrong way text background (kinda ugly, let's remove it)
    TempPtr = *hook::pattern("D8 0D ? ? ? ? C7 05 ? ? ? ? 00 00 D0 42").get_first<float*>(2); //667FBC
    injector::WriteMemory<float>(TempPtr, 0.0f, true);
    injector::WriteMemory<float>(TempPtr + 1, 0.0f, true);

    //loading bar offset
    TempPtr = *hook::pattern("D8 0D ? ? ? ? 89 44 24 34 D9 5C 24 24").get_first<float*>(2); //4C07C6
    injector::WriteMemory<float>(TempPtr, *(float*)TempPtr + (Screen.fHudOffset / 2.18f), true);

    //main menu selectables
    pattern = hook::pattern("8B 06 8D 54 24 24 52 C7"); //0x4DEFAB
    struct CircleGlowHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.esi + 0x00);
            regs.edx = regs.esp + 0x24;
            *(float*)(regs.edx + 0x4) += Screen.fHudOffset;
        }
    }; injector::MakeInline<CircleGlowHook>(pattern.get_first(0), pattern.get_first(6));

    //FOV
    static float fFOVFactor = GetFOV2(0.017453292f, Screen.fAspectRatio) * fIniFOVFactor;
    auto pfSetFOV = hook::pattern("D8 0D ? ? ? ? 55 8B EF 2B DD C1 FB 02 8D 68 01 3B EB").get_first(2);
    injector::WriteMemory(pfSetFOV, &fFOVFactor, true);

    if (!bSkipIntro)
    {
        auto ShowIntro = (void(__cdecl *)()) pfShowIntro.get();
        ShowIntro();
    }
}

void Init()
{
    auto pfShowIntroCall = hook::get_pattern("E8 ? ? ? ? 8B 15 ? ? ? ? A1 ? ? ? ? 52 50 E8", 0);
    pfShowIntro = injector::GetBranchDestination(pfShowIntroCall, true);
    injector::MakeCALL(pfShowIntroCall, ShowIntroHook, true);
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("E8 ? ? ? ? 8B 15 ? ? ? ? A1 ? ? ? ? 52 50 E8"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {

    }
    return TRUE;
}