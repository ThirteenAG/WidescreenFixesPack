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

//injector::hook_back<void(__fastcall*)(int _this, int edx, float a2, float a3, float a4, float a5, int a6)> hbDrawDerbyTriangles;
//void __fastcall DrawDerbyTrianglesHook(int _this, int edx, float a2, float a3, float a4, float a5, int a6)
//{
//    return hbDrawDerbyTriangles.fun(_this,edx, a2, a3, a4, a5, a6);
//}

uintptr_t* dword_4B4C72;
uintptr_t* dword_8DA718;
void __declspec(naked) sub_4B4C50()
{
    const static float flt_67DBE4 = 0.0015625f;
    _asm sub     esp, 0x34
    _asm test    eax, eax
    _asm mov     ecx, 0x8DA718
    _asm mov     ecx, [ecx]
    _asm fild    dword ptr [ecx+0x8]
    _asm mov     dword ptr [esp], 0
    _asm fild    dword ptr [ecx+0x0C]
    _asm fstp    dword ptr [esp+0x0C]
    _asm fld     ds:flt_67DBE4
    _asm jmp     dword_4B4C72
}

void __declspec(naked) sub_4B4BF0()
{
    _asm sub     esp, 8
    _asm movzx   eax, byte ptr [ecx+1Ch]
    _asm fld     dword ptr [esp+18h]
    _asm mov     edx, [ecx+4]
    _asm mov     ecx, [ecx+10h]
    _asm lea     edx, [edx+eax*8]
    _asm push    esi
    _asm push    edi
    _asm fmul    dword ptr [edx]
    _asm shl     eax, 4
    _asm lea     esi, [eax+ecx]
    _asm mov     eax, [esp+18h]
    _asm mov     ecx, [esp+14h]
    _asm fstp    dword ptr [esp+8]
    _asm fld     dword ptr [esp+20h]
    _asm lea     edi, [esp+8]
    _asm fmul    dword ptr [edx+4]
    _asm mov     edx, [esp+1Ch]
    _asm push    edx
    _asm push    eax
    _asm mov     eax, [esp+2Ch]
    _asm fstp    dword ptr [esp+14h]
    _asm lea     edx, [esi+8]
    _asm push    ecx
    _asm call    sub_4B4C50
    _asm pop     edi
    _asm pop     esi
    _asm add     esp, 8
    _asm retn    14h
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
    injector::WriteMemory(pattern.get_first(2), &Screen.fHudOffset, true);
    pattern = hook::pattern("50 D8 0D ? ? ? ? D9 5C 24 20 DB 47 0C"); //4783C5
    //injector::WriteMemory<float>(*pattern.get_first<float*>(3), Screen.fHudScale, true);
    auto pf0015625 = *pattern.get_first<float*>(3);

    pattern = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(pf0015625)));
    for (size_t i = 0; i < pattern.size(); ++i) //http://pastebin.com/hrmrmkNq
    {
        if (i != 14 && i != 15 && i != 16 && i != 18 && i != 39 && i != 40 && i != 44 && i != 45 && i != 47 && i != 48)
        {
            injector::WriteMemory(pattern.get(i).get<uint32_t>(2), &Screen.fHudScale, true);
        }
    }

    pattern = hook::pattern(pattern_str(0xD9, 0x05, to_bytes(pf0015625)));
    injector::WriteMemory(pattern.count(2).get(0).get<uint32_t>(2), &Screen.fHudScale, true); // 0x00460217
    injector::WriteMemory(pattern.count(2).get(1).get<uint32_t>(2), &Screen.fHudScale, true); // 0x004B4C6C derby triangles etc

    pattern = hook::pattern("D8 C9 D9 44 24 0C D8 0D ? ? ? ? 75");
    dword_4B4C72 = pattern.get_first<uintptr_t>(0);
    pattern = hook::pattern("8B 0D ? ? ? ? 89 16 8B 01 6A 20");
    dword_8DA718 = *pattern.get_first<uintptr_t*>(2);

    pattern = hook::pattern("E8 ? ? ? ? EB 04 DD D8 DD D8"); // 0x4C0AE8
    injector::MakeCALL(pattern.get_first(0), sub_4B4BF0, true); //derby triangle restoration
    pattern = hook::pattern("E8 ? ? ? ? D9 44 24 24 0F B6 4E"); // 0x4BB548
    injector::MakeCALL(pattern.get_first(0), sub_4B4BF0, true);//text backround stripe (top right corner)

    //pattern = hook::pattern(""); //
    //struct CenterStuff
    //{
    //    void operator()(injector::reg_pack& regs)
    //    {
    //        if (*(float*)(regs.esp + 0x44) <= 640.0f)
    //            *(float*)(regs.esp + 0x44) += Screen.fHudOffset;
    //        regs.esi = regs.ecx;
    //        regs.eax = *(uint32_t*)(regs.esi + 0x60);
    //    }
    //}; injector::MakeInline<CenterStuff>(0x54F056);

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

    if (pattern.count_hint(1).empty() && !bDelay)
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
