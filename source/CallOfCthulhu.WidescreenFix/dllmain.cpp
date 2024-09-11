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
    float fInvMenuHeartLineEnd; // On-Screen Range: -0.5 to 0.5
    float fInvMenuHeartLineWidth; // Full Screen Width is 0.002
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
            pattern = hook::pattern("68 D7 A3 30 3F 6A 00"); // 422B1A
            const uint32_t codePushZeroPush = 0x68006A; // 6A 00 = push 0x00; 68 = push (next value)
            const float blackBoxWidth = (4.0f / 3.0f) * Screen.fHudOffset / Screen.Width;
            injector::WriteMemory(pattern.get_first(0), codePushZeroPush, true); // y = 0
            injector::WriteMemory(pattern.get_first(3), 1.0f, true); // x = 1
            pattern = hook::pattern("68 E1 7A 14 3F ? ? ? ? ? ? ? 68 52 B8 9E 3E"); // 422B26
            injector::WriteMemory(pattern.get_first(1), blackBoxWidth, true); // w
            injector::WriteMemory(pattern.get_first(13), 1.0f, true); // h = 1

            // InventoryMenu - HeartMonitorLine (positioned on line paper)
            // NOTE: HeartMonitorLine ended at 103% ScreenWidth (i.e. 3% off-screen) in OG game.
            pattern = hook::pattern("D8 0D ? ? ? ? 85 C9 DB"); // 424FD8; [????] = 0.53
            Screen.fInvMenuHeartLineEnd = Screen.fHudScale * 0.5f; // 50% instead of 53% so line doesn't go past screen edge
            injector::WriteMemory(pattern.get_first(2), &Screen.fInvMenuHeartLineEnd, true);
            pattern = hook::pattern("D8 0D ? ? ? ? 8B 44 24 ? 85 C0"); // 425DC8; [????] = 0.000775
            Screen.fInvMenuHeartLineWidth = Screen.fHudScale * 0.000715f; // Subtracted 0.00006 (3% + 3%)
            injector::WriteMemory(pattern.get_first(2), &Screen.fInvMenuHeartLineWidth, true);

            // HealMenu - Fix Cursor Boxes
            // NOTE: Cursor was off by one small grid square in the OG game (fixed)
            pattern = hook::pattern("D8 0D ? ? ? ? 89 44 24 ? D9 6C 24 ? DB 5C 24 ? 8B 44 24 ? 50"); // 4247B5
            const float* ptr = injector::ReadMemory<float*>(pattern.get_first(2));
            const float HealMenuBoxWidth = ptr[0]; // 26.087f
            const float HealMenuCursorXMin = ptr[1]; // 0.35f
            const float HealMenuCursorYMax = ptr[2]; // 0.87f
            const float HealMenuCursorYMin = ptr[3]; // 0.635f
            const float HealMenuCursorXMax = ptr[4]; // 0.58f
            const float TinyBoxWidth = (HealMenuCursorXMax - HealMenuCursorXMin) / 24.0f; // 24 tiny boxes wide
            injector::WriteMemory((uint32_t)&ptr[0], HealMenuBoxWidth / Screen.fHudScale, true);
            injector::WriteMemory((uint32_t)&ptr[1], ((HealMenuCursorXMin + TinyBoxWidth - 0.5f) * Screen.fHudScale) + 0.5f, true);
            injector::WriteMemory((uint32_t)&ptr[2], HealMenuCursorYMax - TinyBoxWidth, true);
            injector::WriteMemory((uint32_t)&ptr[3], HealMenuCursorYMin - TinyBoxWidth, true);
            injector::WriteMemory((uint32_t)&ptr[4], ((HealMenuCursorXMax + TinyBoxWidth - 0.5f) * Screen.fHudScale) + 0.5f, true);
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