module;

#include <stdafx.h>

export module Engine;

import ComVars;
import WidescreenHUD;

export void InitEngine()
{
    //HUD
    static uint8_t* bIsInMenu = nullptr;
    auto pattern = hook::pattern("88 81 39 4D 00 00 C2 08 00");
    struct MenuCheckHook
    {
        void operator()(injector::reg_pack& regs)
        {
            bIsInMenu = reinterpret_cast<uint8_t*>(regs.ecx + 0x4D39);
            *bIsInMenu = 0;
        }
    }; injector::MakeInline<MenuCheckHook>(pattern.get_first(0), pattern.get_first(6)); //0x10C7FC4C

    pattern = hook::pattern(pattern_str(0xD9, 0x05, to_bytes(dword_1120B6BC))); //fld
    for (size_t i = 0; i < pattern.size(); ++i)
        injector::WriteMemory(pattern.get(i).get<uint32_t>(2), &Screen.fHUDScaleXDyn, true);

    pattern = hook::pattern(pattern_str(0xA1, to_bytes(dword_1120B6BC))); //mov eax
    injector::WriteMemory(pattern.get_first(1), &Screen.fHUDScaleXDyn, true);

    static float fHUDScaleXDynXref = 0.0f;
    static float fHudOffsetDynXref = 0.0f;
    pattern = hook::pattern("D8 0D ? ? ? ? D8 25 ? ? ? ? D9 05");
    injector::WriteMemory(pattern.get_first(2), &fHUDScaleXDynXref, true);
    injector::WriteMemory(pattern.get_first(8), &fHudOffsetDynXref, true);
    injector::WriteMemory(pattern.get_first(14), &fHUDScaleXDynXref, true);

    static auto HudScaleRestorerHook = safetyhook::create_mid(pattern.get_first(-10), [](SafetyHookContext& regs)
    {
        fHUDScaleXDynXref = Screen.fHUDScaleXDyn;
        fHudOffsetDynXref = Screen.fHudOffsetDyn;
        if (UObject::GetState(L"EPlayerController") == L"s_KeyPadInteract" || bHackingGameplay)
        {
            fHUDScaleXDynXref = Screen.fHUDScaleXOriginal;
            fHudOffsetDynXref = Screen.fHudOffsetOriginal;
        }
    });

    pattern = hook::pattern("33 CA 89 48 68 D9 44 24 48 DA E9 DF E0");
    struct HudHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx ^= regs.edx;
            *reinterpret_cast<uint32_t*>(regs.eax + 0x68) = regs.ecx;

            Screen.fHUDScaleXDyn = Screen.fHUDScaleX;
            Screen.fHudOffsetDyn = Screen.fHudOffset;

            auto& fLeft = *reinterpret_cast<int16_t*>(regs.esp + 0x40);
            auto& fRight = *reinterpret_cast<int16_t*>(regs.esp + 0x42);
            auto& fTop = *reinterpret_cast<int16_t*>(regs.esp + 0x44);
            auto& fBottom = *reinterpret_cast<int16_t*>(regs.esp + 0x46);
            FColor Color{ *reinterpret_cast<uint32_t*>(regs.esp + 0x3C) };
            wchar_t buffer[256];
            std::wstring_view curTextureName(UObject::GetFullName(*(void**)(regs.esi + 0x14), 0, buffer));

            // Fullscreen images, including camera feed overlay at 0 512
            if ((fLeft == 0 && fRight == 640) ||
                (fLeft == -2 && fRight == 639 && fTop == -2 && fBottom == 479) ||
                (fLeft == -1 && fRight == 640 && fTop == -2 && fBottom == 479) ||
                (fTop == 0 && fBottom == 512))
            {
                Screen.fHUDScaleXDyn = Screen.fHUDScaleXOriginal;
                Screen.fHudOffsetDyn = Screen.fHudOffsetOriginal;
                return;
            }

            if (bIsInMenu && *bIsInMenu == 0)
            {
                WidescreenHudImage(fLeft, fRight, fTop, fBottom, Color, curTextureName);
            }
        }
    }; injector::MakeInline<HudHook>(pattern.get_first(0)); //0x10ADABFA

    pattern = hook::pattern("8B 8E A4 01 00 00 E8 ? ? ? ? 8B 86 A4 01 00 00 8B 88 30 61 00 00 89 88 E0 56 00 00 8B 90 1C 76 00 00 89 90 E8 56 00 00 8B 06 6A 03");
    struct ScopeHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = *reinterpret_cast<uint32_t*>(regs.esi + 0x1A4);
            *reinterpret_cast<int32_t*>(regs.esp + 0x68) = Screen.nScopeScale;
        }
    }; injector::MakeInline<ScopeHook>(pattern.get_first(0), pattern.get_first(6)); //0x10C9A646

    //TEXT
    pattern = hook::pattern("D8 3D ? ? ? ? D9 5C 24 68 DB");
    injector::WriteMemory(pattern.get_first(2), &Screen.fTextScaleX, true); //0x10B149CE + 0x2
    pattern = hook::pattern("D8 25 ? ? ? ? D9 44 24 24 D8 4C 24");
    injector::WriteMemory(pattern.get_first(2), &Screen.fHudOffset, true); //0x10B14BAD + 0x2

    if (bHudWidescreenMode)
    {
        pattern = hook::pattern("A1 ? ? ? ? 83 C4 04 85 C0 D8 3D");
        struct WSText
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *(uint32_t*)dword_11223A7C;

                int32_t offset1 = *reinterpret_cast<int32_t*>(regs.esp + 0x4);
                int32_t offset2 = *reinterpret_cast<int32_t*>(regs.esp + 0xC);
                int32_t offset3 = static_cast<int32_t>(*reinterpret_cast<float*>(regs.esp + 0x1C));
                FColor Color{ *reinterpret_cast<uint32_t*>(regs.esp + 0x160) };

                auto& textX = *reinterpret_cast<float*>(regs.esp + 0x14);

                //DBGONLY(KEYPRESS(VK_F2) { spd::log()->info("{:d} {:d} {:d} {:08x}", offset1, offset2, offset3, Color.RGBA); });

                if (bIsInMenu && *bIsInMenu == 0)
                {
                    WidescreenHudText(textX, offset1, offset2, offset3, Color);
                }
            }
        }; injector::MakeInline<WSText>(pattern.get_first(0), pattern.get_first(5)); //0x10B149C4, 0x10B149C4 + 5
    }

    //FOV
    pattern = hook::pattern("8B 91 BC 02 00 00 52 8B 54 24 24");
    struct UGameEngine_Draw_Hook
    {
        void operator()(injector::reg_pack& regs)
        {
            *reinterpret_cast<float*>(&regs.edx) = AdjustFOV(*reinterpret_cast<float*>(regs.ecx + 0x2BC), Screen.fAspectRatio);
        }
    }; injector::MakeInline<UGameEngine_Draw_Hook>(pattern.get_first(0), pattern.get_first(6)); //0x10A3E67F

    pattern = hook::pattern("8B 88 BC 02 00 00 8B 44 24 20 51");
    struct UGameEngine_Draw_Hook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *reinterpret_cast<float*>(&regs.ecx) = AdjustFOV(*reinterpret_cast<float*>(regs.eax + 0x2BC), Screen.fAspectRatio);
        }
    }; injector::MakeInline<UGameEngine_Draw_Hook2>(pattern.get_first(0), pattern.get_first(6)); //0x10A3E8A0

    pattern = hook::pattern("8B 96 BC 02 00 00 8B 4C 24 24");
    struct UGameEngine_Draw_Hook3
    {
        void operator()(injector::reg_pack& regs)
        {
            *reinterpret_cast<float*>(&regs.edx) = AdjustFOV(*reinterpret_cast<float*>(regs.esi + 0x2BC), Screen.fAspectRatio);
        }
    }; injector::MakeInline<UGameEngine_Draw_Hook3>(pattern.get_first(0), pattern.get_first(6)); //0x10A0D0C1

    //windowed alt-tab fix
    if (!bDisableAltTabFix)
    {
        pattern = hook::pattern("8B 84 24 ? ? ? ? 50 53 57 52 74 50 FF 15 ? ? ? ? 5F 5E 5D 5B 81 C4 ? ? ? ? C2 0C 00");
        struct WndProcHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *reinterpret_cast<uint32_t*>(regs.esp + 0x3A8);

                if (bIsWindowed)
                {
                    if (regs.edi == WM_WINDOWPOSCHANGED)
                    {
                        auto lpwp = reinterpret_cast<LPWINDOWPOS>(regs.eax);
                        auto hwnd = reinterpret_cast<HWND>(regs.edx);
                        if (lpwp->x < 0 && lpwp->y < 0 && IsIconic(hwnd))
                        {
                            ShowWindow(hwnd, SW_RESTORE);
                            SetForegroundWindow(hwnd);
                        }
                    }
                }
            }
        }; injector::MakeInline<WndProcHook>(pattern.get_first(0), pattern.get_first(7)); //0x10CC4EEA
    }

    pattern = hook::pattern("E8 ? ? ? ? 8B 0D ? ? ? ? 33 DB");
    static auto UGameEngineLoadGameHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        UObject::objectStates.clear();
        bHackingGameplay = false;
        IsMenuDisplayedCache.clear();
    });
}