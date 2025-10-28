module;

#include <stdafx.h>

export module Engine;

import ComVars;
import WidescreenHUD;

namespace UGameEngine
{
    SafetyHookInline shPressStartToContinue = {};
    void __fastcall PressStartToContinue(void* UGameEngine, void* edx, int a2, float a3)
    {
        bPressStartToContinue = true;
        shPressStartToContinue.unsafe_fastcall(UGameEngine, edx, a2, a3);
        bPressStartToContinue = false;
    }
}

namespace FCanvasUtil
{
    void(__fastcall* DrawTile)(void* _this, uint32_t EDX, float, float, float, float, float, float, float, float, float, void*, FColor, uint32_t, uint32_t); //0x103D2DA0;
}

void __fastcall FCanvasUtilDrawTileHook(void* _this, uint32_t EDX, float X, float Y, float SizeX, float SizeY, float U, float V, float SizeU, float SizeV, float unk1, void* Texture, FColor Color, uint32_t unk3, uint32_t unk4)
{
    FColor ColBlack; ColBlack.RGBA = 0xFF000000;
    auto n_X = static_cast<uint32_t>(X);
    auto n_Y = static_cast<uint32_t>(Y);
    auto n_SizeX = static_cast<uint32_t>(SizeX);
    auto n_SizeY = static_cast<uint32_t>(SizeY);

    if (n_X == 0 && n_SizeX == 188) //zoom scope 1
    {
        FCanvasUtil::DrawTile(_this, EDX, X, Y, SizeX, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
        return;
    }

    if (n_X == (640 - 188) && n_SizeX == 640) //zoom scope 2
    {
        FCanvasUtil::DrawTile(_this, EDX, SizeX + Screen.fHudOffset + Screen.fHudOffset - 188.0f, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
        return;
    }

    if ((n_X == 0 || n_X == 256 || n_X == 384) && n_Y == 448 && (n_SizeX == 256 || n_SizeX == 384 || n_SizeX == 640) && n_SizeY == 479) //hiding menu background
    {
        //FCanvasUtil::DrawTile(_this, EDX, 0.0f, 0.0f, 640.0f + Screen.fHudOffset + Screen.fHudOffset, Y, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4); hides all menu overlay graphics
        FCanvasUtil::DrawTile(_this, EDX, 0.0f, 0.0f, 640.0f + Screen.fHudOffset + Screen.fHudOffset, 30.0f, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
        FCanvasUtil::DrawTile(_this, EDX, 0.0f, 447.0f, 640.0f + Screen.fHudOffset + Screen.fHudOffset, 480.0f, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
        return;
    }

    if (n_X == 0 && n_SizeX == 64) //sony ericsson overlay
    {
        if (Screen.bOpsatWidescreenMode)
        {
            FCanvasUtil::DrawTile(_this, EDX, X, Y, SizeX, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
            return;
        }
        else
        {
            FCanvasUtil::DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset + 64.0f, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
        }
    }
    if (n_X == 64 && n_SizeX == 320) //sony ericsson overlay
    {
        if (Screen.bOpsatWidescreenMode)
        {
            FCanvasUtil::DrawTile(_this, EDX, X, Y, SizeX + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
            return;
        }
    }
    if (n_X == 320 && n_SizeX == 576) //sony ericsson overlay
    {
        if (Screen.bOpsatWidescreenMode)
        {
            FCanvasUtil::DrawTile(_this, EDX, X + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
            return;
        }
    }

    if (n_X == 576 && n_SizeX == 640) //sony ericsson overlay
    {
        if (Screen.bOpsatWidescreenMode)
        {
            FCanvasUtil::DrawTile(_this, EDX, X + Screen.fHudOffset + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
            return;
        }
        else
        {
            FCanvasUtil::DrawTile(_this, EDX, 576.0f + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
        }
    }

    if (n_X == 0 && (n_SizeX == 640 || n_SizeX == Screen.Width))
    {
        if ((Color.R == 0xFE && Color.G == 0xFE && Color.B == 0xFE) || (Color.R == 0xFF && Color.G == 0xFF && Color.B == 0xFF)) //flashbang grenade flash
        {
            FCanvasUtil::DrawTile(_this, EDX, X, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
            return;
        }
        else
        {
            FCanvasUtil::DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
            FCanvasUtil::DrawTile(_this, EDX, SizeX + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack, unk3, unk4);
        }
    }

    if (Screen.bHudWidescreenMode)
        WidescreenHud(X, SizeX, Y, SizeY, Color);

    X += Screen.fHudOffset;
    SizeX += Screen.fHudOffset;

    return FCanvasUtil::DrawTile(_this, EDX, X, Y, SizeX, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color, unk3, unk4);
}

#if _DEBUG
SafetyHookInline shFindAxisName = {};
float* __fastcall FindAxisName(void* UInput, void* edx, void* AActor, const wchar_t* a3)
{
    auto ret = shFindAxisName.unsafe_fastcall<float*>(UInput, edx, AActor, a3);

    if (std::wstring_view(a3) == L"aMouseX")
    {
        return ret;
    }
    else if (std::wstring_view(a3) == L"aMouseY")
    {
        return ret;
    }

    return ret;
}
#endif

namespace UInput
{
    uint8_t(__fastcall* GetKey)(void* uInput, void* edx, const wchar_t* a2, int a3) = nullptr;

    void* gUInput = nullptr;
    SafetyHookInline shInit = {};
    void __fastcall Init(void* uInput, void* edx, void* uViewport)
    {
        gUInput = uInput;
        return shInit.unsafe_fastcall(uInput, edx, uViewport);
    }
}

namespace UEngine
{
    SafetyHookInline shInputEvent = {};
    int __fastcall InputEvent(void* _this, void* edx, int a2, int inputID, int a4, int value)
    {
        if (inputID == 202) // A on gamepad
        {
            auto EchelonMainHUDState = UObject::GetState(L"EchelonMainHUD");
            if (EchelonMainHUDState == L"MainHUD" || EchelonMainHUDState == L"s_Slavery")
            {
                auto EPlayerControllerState = UObject::GetState(L"EPlayerControllerState");

                if (EPlayerControllerState == L"s_FirstPersonTargeting" || EPlayerControllerState == L"s_RappellingTargeting" ||
                    EPlayerControllerState == L"s_PlayerBTWTargeting" || EPlayerControllerState == L"s_PlayerSniping" || EPlayerControllerState == L"s_HOHFUTargeting")
                {
                    if (UObject::GetState(L"EGameInteractionState") != L"s_GameInteractionMenu")
                    {
                        shInputEvent.unsafe_fastcall<int>(_this, edx, a2, inputID, a4, value);
                        return shInputEvent.unsafe_fastcall<int>(_this, edx, a2, UInput::GetKey(UInput::gUInput, edx, L"ReloadGun", 0), a4, value);
                    }
                }
            }
        }
        return shInputEvent.unsafe_fastcall<int>(_this, edx, a2, inputID, a4, value);
    }
}

export void InitEngine()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B ? 94 00 00 00 E8");
    FCanvasUtil::DrawTile = (decltype(FCanvasUtil::DrawTile))injector::GetBranchDestination(pattern.count(3).get(0).get<uintptr_t>(6), true).as_int();

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "DC 0D ? ? ? ? DB 43 18 DC 0D ? ? ? ? D9 5D E4 75 12 D9 45 10");
    if (!pattern.empty())
        injector::WriteMemory(pattern.get_first(2), &Screen.dHUDScaleX, true);
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F2 0F 5E 05 ? ? ? ? 66 0F 5A D0");
        injector::WriteMemory(pattern.get_first(4), &Screen.dHUDScaleXInv, true);
    }

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 4D F4 5F 5E 64 89 0D 00 00 00 00 5B 8B E5 5D C2 4C 00");
    if (!pattern.empty())
        injector::MakeCALL(pattern.count(1).get(0).get<uint32_t>(-5), FCanvasUtilDrawTileHook, true); //pfFUCanvasDrawTile + 0x219
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 11 0C 24 E8 ? ? ? ? 5F 5E 5B");
        injector::MakeCALL(pattern.get_first(5), FCanvasUtilDrawTileHook, true);
    }

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 4C 24 04 8B 51 44 83");
    if (!pattern.empty())
    {
        uint32_t pfsub_103762F0 = (uint32_t)pattern.get_first(0);
        auto rpattern = hook::range_pattern(pfsub_103762F0, pfsub_103762F0 + 0x800, "E8 ? ? ? ? 8B ?");
        injector::MakeCALL(rpattern.get(3).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x36E
        injector::MakeCALL(rpattern.get(5).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x43D
        injector::MakeCALL(rpattern.get(7).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x4DA
        injector::MakeCALL(rpattern.get(9).get<uint32_t>(0), FCanvasUtilDrawTileHook, true); //pfsub_103762F0 + 0x564
    }
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "E8 ? ? ? ? 83 7D ? ? 8B 4D");
        injector::MakeCALL(pattern.get_first(0), FCanvasUtilDrawTileHook, true);
    }

    //FOV
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "81 EC 84 06 00 00 A1 ? ? ? ? 53 56 57");
    if (!pattern.empty())
    {
        uint32_t pfDraw = (uint32_t)pattern.get_first();
        auto rpattern = hook::range_pattern(pfDraw, pfDraw + 0x1036, "8B ? ? 03 00 00");
        struct UGameEngine_Draw_Hook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)&regs.ecx = AdjustFOV(*(float*)(regs.eax + 0x374), Screen.fAspectRatio);
            }
        }; injector::MakeInline<UGameEngine_Draw_Hook>(rpattern.get(0).get<uint32_t>(0), rpattern.get(0).get<uint32_t>(0 + 6));

        struct UGameEngine_Draw_Hook2 //1038AA8F
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)&regs.eax = AdjustFOV(*(float*)(regs.edx + 0x374), Screen.fAspectRatio);
            }
        };
        injector::MakeInline<UGameEngine_Draw_Hook2>(rpattern.get(2).get<uint32_t>(0), rpattern.get(2).get<uint32_t>(0 + 6));
        injector::MakeInline<UGameEngine_Draw_Hook2>(rpattern.get(3).get<uint32_t>(0), rpattern.get(3).get<uint32_t>(0 + 6));
    }
    else
    {
        struct UGameEngine_Draw_Hook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.xmm0.f32[0] = AdjustFOV(*(float*)(regs.eax + 0x374), Screen.fAspectRatio);
            }
        };

        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 8D ? ? ? ? F3 0F 10 80");
        injector::MakeInline<UGameEngine_Draw_Hook>(pattern.get_first(6), pattern.get_first(6 + 8));

        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 41 ? F3 0F 10 80 ? ? ? ? 8B C4");
        injector::MakeInline<UGameEngine_Draw_Hook>(pattern.get_first(3), pattern.get_first(3 + 8));

        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "C7 86 ? ? ? ? ? ? ? ? F3 0F 10 80");
        injector::MakeInline<UGameEngine_Draw_Hook>(pattern.get_first(10), pattern.get_first(10 + 8));
    }

    if (Screen.nPostProcessFixedScale)
    {
        if (Screen.nPostProcessFixedScale == 1)
            Screen.nPostProcessFixedScale = Screen.Width;

        auto pattern2 = hook::module_pattern(GetModuleHandle(L"Engine"), "68 00 02 00 00 68 00 02 00 00");
        for (size_t i = 0; i < pattern2.size(); i++)
        {
            injector::WriteMemory(pattern2.get(i).get<uint32_t>(1), Screen.nPostProcessFixedScale, true);
            injector::WriteMemory(pattern2.get(i).get<uint32_t>(6), Screen.nPostProcessFixedScale, true);
        }
    }

    if (gColor.RGBA)
    {
        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "B0 7F 88 45 24 88 45 25 88 45 26 C6 45 27 FF 8B 45 24 50 8B CB FF 52 40"); //104070CF
        if (!pattern.empty())
        {
            struct USkeletalMeshInstanceRenderHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint8_t*)(regs.ebp + 0x24) = gColor.B;
                    *(uint8_t*)(regs.ebp + 0x25) = gColor.G;
                    *(uint8_t*)(regs.ebp + 0x26) = gColor.R;
                    *(uint8_t*)(regs.ebp + 0x27) = 0xFF;
                }
            }; injector::MakeInline<USkeletalMeshInstanceRenderHook>(pattern.get_first(0), pattern.get_first(15));
        }
        else
        {
            pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "C7 85 ? ? ? ? ? ? ? ? FF B5 ? ? ? ? FF 50");
            struct USkeletalMeshInstanceRenderHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint8_t*)(regs.ebp - 0xA8) = gColor.B;
                    *(uint8_t*)(regs.ebp - 0xA7) = gColor.G;
                    *(uint8_t*)(regs.ebp - 0xA6) = gColor.R;
                    *(uint8_t*)(regs.ebp - 0xA5) = 0xFF;
                }
            }; injector::MakeInline<USkeletalMeshInstanceRenderHook>(pattern.get_first(0), pattern.get_first(10));
        }
    }

    #if _DEBUG
    pattern = find_module_pattern<1>(GetModuleHandle(L"Engine"), "55 8B EC 83 EC ? 53 56 57 6A ? FF 75 ? 8D 4D ? FF 15 ? ? ? ? 83 7D ? ? 0F 84 ? ? ? ? 8B 75");
    if (!pattern.empty())
        shFindAxisName = safetyhook::create_inline(pattern.get_first(), FindAxisName);
    #endif

    // LOD
    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "0F 84 ? ? ? ? FF 15 ? ? ? ? 8B 8C 24", "0F 84 ? ? ? ? 8B 41 ? F3 0F 10 81");
    injector::WriteMemory<uint16_t>(pattern.get_first(0), 0xE990, true); // jz -> jmp

    if (bSkipIntro)
    {
        pattern = find_module_pattern(GetModuleHandle(L"Engine"), "6A ? 6A ? 68 ? ? ? ? 53 E8 ? ? ? ? 83 C4 ? 68", "6A ? FF D6 85 C0");
        auto dest = find_module_pattern(GetModuleHandle(L"Engine"), "8B 83 ? ? ? ? 8B 48 ? 8B 01 83 A0", "8B 87 ? ? ? ? 8D 8D ? ? ? ? 8B 40");
        injector::MakeJMP(pattern.get_first(0), dest.get_first(0), true);
    }

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "E8 ? ? ? ? 83 C4 ? 85 C0 75 ? 8B 4F", "83 7B ? ? BE");
    static auto VideoPlaybackStartHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bPlayingVideo = true;
    });

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "8B 0D ? ? ? ? 8B 54 24 ? 89 11", "89 02 8B 13");
    static auto VideoPlaybackEndHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bPlayingVideo = false;
    });

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "55 8B EC 6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 83 EC ? A1 ? ? ? ? 53 56 8B F1 33 C9", "55 8B EC 83 EC ? 83 3D ? ? ? ? ? 56");
    if (!pattern.empty())
        UGameEngine::shPressStartToContinue = safetyhook::create_inline(pattern.get_first(), UGameEngine::PressStartToContinue);

    if (bSkipPressStartToContinue)
    {
        pattern = find_module_pattern(GetModuleHandle(L"Engine"), "0F 84 ? ? ? ? F3 0F 10 45 ? F3 0F 59 05");
        if (!pattern.empty())
            injector::WriteMemory<uint16_t>(pattern.get_first(0), 0xE990, true); // jz -> jmp
        else
        {
            pattern = find_module_pattern(GetModuleHandle(L"Engine"), "74 ? D9 45 ? D8 0D ? ? ? ? E8");
            if (!pattern.empty())
                injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); // jz -> jmp
        }
    }

    UInput::GetKey = (decltype(UInput::GetKey))FindProcAddress(GetModuleHandle(L"Engine"), "?GetKey@UInput@@UAEEPB_WH@Z", "?GetKey@UInput@@UAEEPBGH@Z");
    UEngine::shInputEvent = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?InputEvent@UEngine@@UAEHPAVUViewport@@W4EInputKey@@W4EInputAction@@M@Z"), UEngine::InputEvent);
    UInput::shInit = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?Init@UInput@@UAEXPAVUViewport@@@Z"), UInput::Init);
}