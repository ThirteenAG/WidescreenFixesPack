#include "stdafx.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    int32_t Width43;
    float fWidth43;
    float fHudOffset;
} Screen;

float __fastcall sub_4140E0Hook(int _this, float a2, float a3, float a4, float a5)
{
    if (a2 == 0.0f && a4 == Screen.fWidth)
    {
        a2 += Screen.fHudOffset;
        a4 -= Screen.fHudOffset * 2.0f;
    }

    float v5 = a2 + a4;
    float v7 = a3 + a5;

    *(float *)(_this + 0x00) = a2;
    *(float *)(_this + 0x04) = v5;
    *(float *)(_this + 0x08) = v5;
    *(float *)(_this + 0x0C) = a2;
    *(float *)(_this + 0x10) = a3;
    *(float *)(_this + 0x14) = a3;
    *(float *)(_this + 0x18) = v7;
    *(float *)(_this + 0x1C) = v7;

    *(uint8_t*)(_this + 0x59) = 1;
    return a3;
}

SafetyHookMid ReticleHorizontal[3], ReticleVertical[3];

inline float ReticleX(float x)
{
    return 0.5f + (x - 0.5f) * (Screen.fWidth - 2.0f * Screen.fHudOffset) / Screen.fWidth;
}

void AdjustReticleHorizontal(injector::reg_pack& regs)
{
    auto* args = reinterpret_cast<float*>(regs.esp);
    args[0] = ReticleX(args[0]);
    args[1] = ReticleX(args[1]);
}
void AdjustReticleVertical(injector::reg_pack& regs)
{
    auto& x = *reinterpret_cast<float*>(regs.esp);
    x = ReticleX(x);
}

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    float FOV = iniReader.ReadFloat("MAIN", "FOV", 75.f);

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.Width43);

    auto pattern = hook::pattern("C7 ? ? ? ? ? ? ? ? ? C7 ? ? ? ? ? ? ? ? ? C7 ? ? ? ? ? ? ? ? ? C7 ? ? ? ? ? ? ? ? ? 50 A1"); //622B17
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(6), Screen.Height, true);
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(16), Screen.Width, true);

    pattern = hook::pattern("8B ? ? ? ? ? 8B ? ? ? ? ? 8B ? 99 2B ? D1 ? 89"); //48B0D6
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = Screen.Width;
            regs.esi = Screen.Height;
        }
    }; injector::MakeInline<SetResHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(12));

    pattern = hook::pattern("8B ? ? ? 57 8B ? ? ? 50 6A"); //48B076
    struct SetResHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edi = Screen.Width;
            regs.esi = Screen.Height;
        }
    }; injector::MakeInline<SetResHook2>(pattern.count(1).get(0).get<uint32_t>(1), pattern.count(1).get(0).get<uint32_t>(9));
    injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(0), 0x57, true); //push edi

    pattern = hook::pattern("D9 ? ? ? 8B ? ? ? D9 ? ? ? ? ? A3 ? ? ? ? DA"); //4152C0
    struct SetScalingHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)&regs.eax = Screen.fAspectRatio;
        }
    }; injector::MakeInline<SetScalingHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(8));
    injector::WriteMemory(*pattern.count(1).get(0).get<uint32_t*>(10), Screen.fAspectRatio, true);
    injector::WriteMemory(*pattern.count(1).get(0).get<uint32_t*>(15), Screen.fAspectRatio, true);

    pattern = hook::pattern("D8 ? ? ? ? ? 83 ? ? 6A ? 68 ? ? ? ? 51 D9 ? ? E8 ? ? ? ? 83 ? ? 83"); //4A15F6
    injector::WriteMemory(*pattern.count(1).get(0).get<uint32_t*>(2), AdjustFOV(FOV, Screen.fAspectRatio), true);

    Screen.fHudOffset = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;

    pattern = hook::pattern("83 ? ? 56 8B ? 8B ? ? 85 ? 57 74 ? 8B ? 85"); //507C40
    uint8_t* drawScopeReticle = pattern.count(1).get(0).get<uint8_t>(0);
    static constexpr int32_t horizontalCalls[] = { 0x31F, 0x3BC, 0x3E9 };
    static constexpr int32_t verticalCalls[] = { 0x33C, 0x401, 0x416 };
    for (size_t i = 0; i < _countof(ReticleHorizontal); ++i)
        ReticleHorizontal[i] = safetyhook::create_mid(drawScopeReticle + horizontalCalls[i], AdjustReticleHorizontal);
    for (size_t i = 0; i < _countof(ReticleVertical); ++i)
        ReticleVertical[i] = safetyhook::create_mid(drawScopeReticle + verticalCalls[i], AdjustReticleVertical);

    pattern = hook::pattern("E8 ? ? ? ? 6A 01 B9 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 8A"); //4D5839
    injector::MakeCALL(pattern.count(1).get(0).get<uint32_t>(0), sub_4140E0Hook, true); //intro screen

    static float fHudScale2 = (0.0009765625f / Screen.fAspectRatio) * (4.0f / 3.0f);
    pattern = hook::pattern("D8 ? ? ? ? ? D9 ? ? D9 ? ? ? D8 ? ? ? ? ? D9 ? ? ? ? ? D9 ? ? ? D8"); //502232
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &fHudScale2, true); //text size and radar arrows

    pattern = hook::pattern("8B ? ? 89 ? ? 83 ? ? ? ? ? ? 75 ? A1"); //5021C2
    struct HudHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.esi + 0x28) += Screen.fHudOffset;
            *(float*)(regs.esi + 0x30) -= Screen.fHudOffset * 2.0f;

            regs.eax = *(uintptr_t*)(regs.edi + 0x4);
            *(uintptr_t*)(regs.esi + 0x44) = regs.eax;
        }
    }; injector::MakeInline<HudHook2>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));

    pattern = hook::pattern("DB ? ? ? ? ? D8 ? D9 ? DB ? ? ? ? ? D8 ? D9"); //40E4FD
    struct TextHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)regs.edi *= Screen.fWidth43;
            *(float*)regs.edi += Screen.fHudOffset;
        }
    }; injector::MakeInline<TextHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(10));

    static int n0 = 0;
    pattern = hook::pattern("DB 05 ? ? ? ? 53 55 57 D9 5C 24 14"); //4D2166
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &n0, true);
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init, hook::pattern("BF ? ? ? ? 8B ? E8 ? ? ? ? 89 ? ? 8B"));
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