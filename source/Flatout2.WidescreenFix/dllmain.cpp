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
    _asm {
        sub     esp, 0x34
        test    eax, eax
        mov     ecx, 0x8DA718
        mov     ecx, [ecx]
        fild    dword ptr[ecx + 0x8]
        mov     dword ptr[esp], 0
        fild    dword ptr[ecx + 0x0C]
        fstp    dword ptr[esp + 0x0C]
        fld     ds : flt_67DBE4
        jmp     dword_4B4C72
    }
}

void __declspec(naked) sub_4B4BF0()
{
    _asm {
        sub     esp, 8
        movzx   eax, byte ptr[ecx + 1Ch]
        fld     dword ptr[esp + 18h]
        mov     edx, [ecx + 4]
        mov     ecx, [ecx + 10h]
        lea     edx, [edx + eax * 8]
        push    esi
        push    edi
        fmul    dword ptr[edx]
        shl     eax, 4
        lea     esi, [eax + ecx]
        mov     eax, [esp + 18h]
        mov     ecx, [esp + 14h]
        fstp    dword ptr[esp + 8]
        fld     dword ptr[esp + 20h]
        lea     edi, [esp + 8]
        fmul    dword ptr[edx + 4]
        mov     edx, [esp + 1Ch]
        push    edx
        push    eax
        mov     eax, [esp + 2Ch]
        fstp    dword ptr[esp + 14h]
        lea     edx, [esi + 8]
        push    ecx
        call    sub_4B4C50
        pop     edi
        pop     esi
        add     esp, 8
        retn    14h
    }
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
    if (!pattern.empty())
    {
        injector::WriteMemory(pattern.get_first<float>(2), &Screen.fWidth, true);
        injector::WriteMemory(pattern.get_first<float>(18), &Screen.fHeight, true);
    }
    else
    {
        pattern = hook::pattern("8B 44 24 04 8B 4C 24 08 C7 00 00 00 80");
        if (!pattern.empty())
        {
            injector::WriteMemory<float>(pattern.get_first<float>(10 + 0), Screen.fWidth, true);
            injector::WriteMemory<float>(pattern.get_first<float>(10 + 6), Screen.fHeight, true);
            injector::WriteMemory<float>(pattern.get_first<float>(23 + 10 + 0), Screen.fWidth, true);
            injector::WriteMemory<float>(pattern.get_first<float>(23 + 10 + 6), Screen.fHeight, true);
        }
    }

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
    static float fFOVFactor = GetFOV2(0.017453292f, Screen.fAspectRatio) * fIniFOVFactor;
    auto pfSetFOV = hook::pattern("D8 0D ? ? ? ? 55 8B EF 2B DD C1 FB 02 8D 68 01 3B EB").get_first(2);
    injector::WriteMemory(pfSetFOV, &fFOVFactor, true);

    if (!bSkipIntro)
    {
        auto ShowIntro = pfShowIntro.as_int() + 6;
        _asm sub esp, 0x104
        _asm jmp ShowIntro
    }
}

void Init()
{
    pfShowIntro = hook::get_pattern("81 EC 04 01 00 00 8B 15", 0);
    injector::MakeJMP(pfShowIntro, ShowIntroHook, true);

    auto pattern = hook::pattern("6A 00 6A 00 E8 ? ? ? ? 83 C4 08 B8 01 00 00 00 5B");
    if (!pattern.empty())
        injector::MakeNOP(pattern.get_first(-7), 2, true);
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("81 EC 04 01 00 00 8B 15"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {

    }
    return TRUE;
}