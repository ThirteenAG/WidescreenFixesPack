module;

#include <stdafx.h>

export module Echelon;

import ComVars;

namespace AEPlayerController
{
    SafetyHookInline shTick = {};
    void __fastcall Tick(void* playerController, void* edx, int a2, int a3)
    {
        if (playerController)
        {
            auto ptr = *((uintptr_t*)playerController + 3);
            if (ptr)
            {
                auto ptr2 = *(uintptr_t*)(ptr + 0x1C);
                if (ptr2)
                {
                    auto StateID = *(int*)(ptr2 + 0x20);

                    wchar_t buffer[256];
                    auto objName = std::wstring_view(UObject::GetFullName(playerController, edx, buffer));

                    size_t spacePos = objName.find(L' ');
                    std::wstring type = (spacePos != std::wstring::npos) ? std::wstring(objName.substr(0, spacePos)) : std::wstring(objName);

                    auto svStateName = std::wstring_view(UObject::GetFullName(UObject::FindState(playerController, edx, StateID), edx, buffer));
                    size_t lastDot = svStateName.rfind(L'.');
                    std::wstring stateName = (lastDot != std::wstring::npos) ? std::wstring(svStateName.substr(lastDot + 1)) : std::wstring(svStateName);
                    UObject::objectStates[type] = stateName;
                }
            }
        }

        return shTick.unsafe_fastcall(playerController, edx, a2, a3);
    }
}

namespace AETextureManager
{
    injector::hook_back<void(__fastcall*)(void*, void*, void*, int, float, float, float, float, float, float)> hbDrawTileFromManager = {};
    void __fastcall DrawTileFromManager(void* AETextureManager, void* edx, void* UECanvas, int a3, float width, float height, float x, float y, float a8, float a9)
    {
        if (Screen.nCutsceneBorders != 0 && (int)x == 0 && (int)width == 640 && (int)height == 60 && ((int)y == 0 || int(y + height) == 480))
        {
            if (UObject::GetState(L"EchelonMainHUD") == L"s_Cinematic")
            {
                if (Screen.nCutsceneBorders == 1)
                {
                    x = 0.0f;
                    y = 0.0f;
                    width = 0.0f;
                    height = 0.0f;
                }
                else if (Screen.nCutsceneBorders == 2)
                {
                    const float newBorderHeight = (480.0f - 270.0f * Screen.fAspectRatio) / 2.0f;
                    if (newBorderHeight <= 0)
                    {
                        x = 0.0f;
                        y = 0.0f;
                        width = 0.0f;
                        height = 0.0f;
                    }
                    else
                    {
                        if (y != 0)
                            y = 480.0f - newBorderHeight;
                        height = newBorderHeight;
                    }
                }
            }
        }

        return hbDrawTileFromManager.fun(AETextureManager, edx, UECanvas, a3, width, height, x, y, a8, a9);
    }
}

export void InitEchelon()
{
    // EPlayerController additional state cache
    AEPlayerController::shTick = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Echelon"), "?Tick@AEPlayerController@@UAEHMW4ELevelTick@@@Z"), AEPlayerController::Tick);

    // Letterboxing
    auto pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "D9 1C 24 51 50 8B CE E8 ? ? ? ? 5E", "FF B6 ? ? ? ? 57 E8 ? ? ? ? 5F");
    AETextureManager::hbDrawTileFromManager.fun = injector::MakeCALL(pattern.get_first(7), AETextureManager::DrawTileFromManager, true).get();

    // Hud Helpers
    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "85 C0 7C ? 8B 89 6C 03 00 00", "85 C0 78 ? F3 0F 10 45 24");
    static auto AETextureManagerDrawTileFromManagerHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        curDrawTileManagerTextureIndex = regs.eax;
    });

    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "85 C0 7C ? 8B 4C 24 08", "85 C0 78 ? F3 0F 10 45 FC");
    static auto AETextureManagerexecDrawTileFromManagerHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        curDrawTileManagerTextureIndex = regs.eax;
    });

    // set player speed to max on game start
    UIntOverrides::Register(L"IntProperty Echelon.EchelonGameInfo.m_defautSpeed", +[]() -> int
    {
        return 5;
    });

    // Camera acceleration
    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "0F 8A ? ? ? ? F3 0F 10 81 ? ? ? ? 0F 2E C1 9F F6 C4 ? 0F 8A ? ? ? ? F3 0F 10 89");
    if (!pattern.empty())
        injector::WriteMemory<uint16_t>(pattern.get_first(0), 0xE990, true); // jp -> jmp
    else
    {
        pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "7A ? D9 05 ? ? ? ? D9 81 ? ? ? ? DA E9 DF E0 F6 C4 ? 7A");
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); // jp -> jmp
    }

    // Aiming camera smoothing
    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "0F 84 ? ? ? ? 8B 86 ? ? ? ? DD D8", "0F 84 ? ? ? ? 8B 87 ? ? ? ? 83 EC ? F3 0F 10 80");
    injector::WriteMemory<uint16_t>(pattern.get_first(0), 0xE990, true); // jz -> jmp
}