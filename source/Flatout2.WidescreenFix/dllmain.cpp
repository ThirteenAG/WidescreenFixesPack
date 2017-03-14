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

injector::hook_back<void(__fastcall*)(int _this, int edx, float a2, float a3, float a4, float a5, int a6)> hbDrawDerbyTriangles;
void __fastcall DrawDerbyTrianglesHook(int _this, int edx, float a2, float a3, float a4, float a5, int a6)
{
    a2 += Screen.fHudOffset; //??
    a5 *= 2.0f; //??
    return hbDrawDerbyTriangles.fun(_this,edx, a2, a3, a4, a5, a6);
}

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
    pattern = hook::pattern("D9 05 ? ? ? ? 56 8B F1 8B 86"); //53BBF3
    //injector::WriteMemory(pattern.get_first(2), &Screen.fHudOffset, true);
    pattern = hook::pattern("50 D8 0D ? ? ? ? D9 5C 24 20 DB 47 0C"); //4783C5
    //injector::WriteMemory<float>(*pattern.get_first<float*>(3), Screen.fHudScale, true);
    auto pf0015625 = *pattern.get_first<float*>(3);

    pattern = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(pf0015625)));
    for (size_t i = 0; i < pattern.size(); ++i) //http://pastebin.com/hrmrmkNq
    {
        if (i != 14 && i != 15 && i != 16 && i != 18 && i != 39 && i != 40 && i != 44 && i != 45 && i != 46 && i != 47 && i != 48)
        {
            injector::WriteMemory(pattern.get(i).get<uint32_t>(2), &Screen.fHudScale, true);
        }
    }

    pattern = hook::pattern(pattern_str(0xD9, 0x05, to_bytes(pf0015625)));
    injector::WriteMemory(pattern.count(2).get(0).get<uint32_t>(2), &Screen.fHudScale, true); // 0x00460217
    injector::WriteMemory(pattern.count(2).get(1).get<uint32_t>(2), &Screen.fHudScale, true); // 0x004B4C6C derby triangles etc

    injector::WriteMemory<float>(0x4BB53E + 1, 0.0f, true); //hide text backround stripe (top right corner)

    hbDrawDerbyTriangles.fun = injector::MakeCALL(0x4C0AE8, DrawDerbyTrianglesHook).get();

    //pattern = hook::pattern("8B 06 8D 54 24 24 52 C7"); //0x4DEFAB
    //struct DerbyTrianglesHook
    //{
    //    void operator()(injector::reg_pack& regs)
    //    {
    //        regs.eax = *(uint32_t*)(regs.esp + 0x00) << 24;
    //    }
    //}; injector::MakeInline<DerbyTrianglesHook>(0x4B4C8D, 0x4B4C8D+6/*pattern.get_first(0), pattern.get_first(6)*/);

    //pattern = hook::pattern("8B 06 8D 54 24 24 52 C7"); //0x4DEFAB
    //struct DerbyTrianglesHook
    //{
    //    void operator()(injector::reg_pack& regs)
    //    {
    //        *(float*)(regs.esp + 0x10) += Screen.fHudOffset;
    //        *(float*)(regs.esp + 0x18) += Screen.fHudOffset;
    //
    //        regs.eax = *(uint32_t*)(regs.ecx);
    //        regs.edx = regs.esp + 0x10;
    //    }
    //}; injector::MakeInline<DerbyTrianglesHook>(0x4B4D71, 0x4B4D71+6/*pattern.get_first(0), pattern.get_first(6)*/);

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
