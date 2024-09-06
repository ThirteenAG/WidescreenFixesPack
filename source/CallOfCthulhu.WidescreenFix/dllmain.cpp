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

void Init()
{
    CIniReader iniReader("");
    static bool bStretchFullscreenImages = iniReader.ReadInteger("MAIN", "StretchFullscreenImages", 0) != 0;

    auto pattern = hook::pattern("8B 4C 24 18 BF 01 00 00 00 C7"); //0x406344
    struct ResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = *(int32_t*)(regs.esp + 0x18);
            regs.edi = 1;

            Screen.Width = regs.ebx;
            Screen.Height = regs.ecx;
            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
            Screen.fHudScale = ((1.0f / Screen.fAspectRatio) * (4.0f / 3.0f));
            Screen.fHudOffset = ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f);
            Screen.fTextOffset = -(Screen.fHudOffset / Screen.fHudScale);
            Screen.fFMVOffset = ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f);

            auto pattern = hook::pattern("68 A5 AC A9 3F"); //4352C4 435511 5D7EF6
            for (size_t i = 0; i < pattern.size(); ++i)
            {
                injector::WriteMemory<float>(pattern.get(i).get<float*>(1), Screen.fAspectRatio, true); // thanks to nemesis2000
            }

            // InventoryMenu - BlackBox (covers top-right line paper)
            int codePushZeroPush = 0x68006A; // 6A00 = push 0x00; 68 = push (next value)
            injector::WriteMemory<int>(0x422B1A, codePushZeroPush, true); // y = 0
            injector::WriteMemory<float>(0x422B1D, 1.0f, true); // x = 1
            injector::WriteMemory<float>(0x422B27, (4.0f / 3.0f) * Screen.fHudOffset / Screen.Width, true); // w
            injector::WriteMemory<float>(0x422B33, 1.0f, true); // h = 1

            // InventoryMenu - HeartMonitorLine (positioned on line paper)
            // NOTE: HeartMonitorLine ends at 103% ScreenWidth (i.e. 3% off-screen)
            float HeartMonitorLineEndX = 0.53f; // ReadMemory(0x62D630); // On-Screen Range: -0.5 to 0.5
            float HeartMonitorLineWidth = 0.000775f; // ReadMemory(0x62D794); // ScreenWidth is 0.002
            injector::WriteMemory<float>(0x62D630, (HeartMonitorLineEndX - 0.03f) * Screen.fHudScale, true);
            injector::WriteMemory<float>(0x62D794, (HeartMonitorLineWidth - 0.00006f) * Screen.fHudScale, true);

            // HealMenu - Fix Cursor Boxes
            // NOTE: Cursor is off by one small square even in the OG game (fixed)
            float HealMenuBoxWidth = 26.087f; // ReadMemory(0x62D5D4);
            float HealMenuCursorXMin = 0.35f; // ReadMemory(0x62D5D8);
            float HealMenuCursorXMax = 0.58f; // ReadMemory(0x62D5E4);
            float HealMenuCursorYMax = 0.87f; // ReadMemory(0x62D5DC);
            float HealMenuCursorYMin = 0.635f; // ReadMemory(0x62D5E0);
            float OneSquareWidth = (HealMenuCursorXMax - HealMenuCursorXMin) / 24.0f;
            injector::WriteMemory<float>(0x62D5D4, HealMenuBoxWidth / Screen.fHudScale, true);
            injector::WriteMemory<float>(0x62D5D8, ((HealMenuCursorXMin + OneSquareWidth - 0.5f) * Screen.fHudScale) + 0.5f, true);
            injector::WriteMemory<float>(0x62D5E4, ((HealMenuCursorXMax + OneSquareWidth - 0.5f) * Screen.fHudScale) + 0.5f, true);
            injector::WriteMemory<float>(0x62D5DC, HealMenuCursorYMax - OneSquareWidth, true);
            injector::WriteMemory<float>(0x62D5E0, HealMenuCursorYMin - OneSquareWidth, true);
        }
    }; injector::MakeInline<ResHook>(pattern.count(1).get(0).get<uintptr_t>(0), pattern.count(1).get(0).get<uintptr_t>(9));

    pattern = hook::pattern("D9 05 ? ? ? ? D8 F1 D9 5C 24 38 DD D8 DD D8"); //0x4E1486
    struct HudHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto ptr = *(uintptr_t*)(regs.esp + 0x60);
            float x = (float)(*(int32_t*)(regs.esp + 0x24));

            if (bStretchFullscreenImages && (*(float*)(ptr + 0x0) == 0.0f && *(float*)(ptr + 0x8) == Screen.fWidth))
            {
                *(float*)(regs.esp + 0x2C) = 1.0f / (10.0f * x);
                *(float*)(regs.esp + 0x34) = -0.5f / x;
            }
            else
            {
                *(float*)(regs.esp + 0x2C) = Screen.fHudScale / (10.0f * x);
                *(float*)(regs.esp + 0x34) = Screen.fHudOffset / x;
            }

            float fneg05 = -0.5f;
            _asm fld ds : fneg05
        }
    }; injector::MakeInline<HudHook>(pattern.count(1).get(0).get<uintptr_t>(0), pattern.count(1).get(0).get<uintptr_t>(6));

    pattern = hook::pattern("89 4C 24 08 8B 48 08 89 54 24 0C 8B 50 0C 8D 44 24 04"); //0x4239B1
    struct ClickableAreaHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.esp + 8) = (*(float*)&regs.ecx * Screen.fHudScale) + Screen.fHudOffset;
            *(float*)&regs.ecx = (*(float*)(regs.eax + 0x8) * Screen.fHudScale);
        }
    }; injector::MakeInline<ClickableAreaHook>(pattern.count(1).get(0).get<uintptr_t>(0), pattern.count(1).get(0).get<uintptr_t>(7));

    pattern = hook::pattern("D8 25 ? ? ? ? D8 F2 D9 5C 24 2C D9"); //0x4E0BC4
    injector::WriteMemory(pattern.count(1).get(0).get<uintptr_t>(2), &Screen.fTextOffset, true);

    pattern = hook::pattern("D8 F2 D9 5C 24 2C D9 44 24 44 D8 25"); //0x4E0BCA
    struct TextHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float temp = 0.0f;
            _asm {fdiv    st, st(2)}
            _asm {fstp    dword ptr[temp]}
            *(float*)(regs.esp + 0x2C) = temp;

            *(float*)(regs.esp + 0x24) *= Screen.fHudScale;
            *(float*)(regs.esp + 0x2C) *= Screen.fHudScale;
        }
    }; injector::MakeInline<TextHook>(pattern.count(1).get(0).get<uintptr_t>(0), pattern.count(1).get(0).get<uintptr_t>(6));

    pattern = hook::pattern("F3 0F 11 40 58 8B 15 ? ? ? ? 8B 02 8B 08 50"); //0x404C8E
    struct FMVHook
    {
        void operator()(injector::reg_pack& regs)
        {
            if (*(float*)(regs.eax + 0x18) == Screen.fWidth && *(float*)(regs.eax + 0x30) == Screen.fWidth)
            {
                *(float*)(regs.eax + 0x00) += Screen.fFMVOffset;
                *(float*)(regs.eax + 0x48) += Screen.fFMVOffset;
                *(float*)(regs.eax + 0x18) -= Screen.fFMVOffset;
                *(float*)(regs.eax + 0x30) -= Screen.fFMVOffset;
            }
            *(float*)(regs.eax + 0x58) = 0.0f;
        }
    }; injector::MakeInline<FMVHook>(pattern.count(1).get(0).get<uintptr_t>(0));
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init, hook::pattern("89 65 E8 8B F4 89 3E 56"));
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