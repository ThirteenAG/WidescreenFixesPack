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

// EPlayerCam FPS fixes
namespace AEPlayerCam
{
    // Above 30 FPS, ftol() rounds speed * DeltaTime down to 0, so DampFloat() never settles
    constexpr float kStep     = 1.0f / 30.0f;
    constexpr int   kMaxSteps = 8;

    constexpr uintptr_t kHitRoll      = 0x1324;
    constexpr uintptr_t kHitFadeOut   = 0x1328;
    constexpr uintptr_t kShakeRoll    = 0x132c;
    constexpr uintptr_t kShakeTarget  = 0x1330;
    constexpr uintptr_t kShakeSpeed   = 0x1334;
    constexpr uintptr_t kShakeFadeOut = 0x1338;
    constexpr uintptr_t kTiltPitch    = 0x133c;
    constexpr uintptr_t kTiltTarget   = 0x1340;
    constexpr uintptr_t kTiltSpeed    = 0x1344;
    constexpr uintptr_t kTiltFadeOut  = 0x1348;

    constexpr uintptr_t kLevelOff     = 0x6c;
    constexpr uintptr_t kDeltaTimeOff = 0x55c;

    float accumulator = 0.0f;

    SafetyHookInline shUpdateView = {};
    void __fastcall UpdateView(void* cam, void* edx, int p1, int p2, int p3, int p4)
    {
        if (!cam)
            return shUpdateView.unsafe_fastcall(cam, edx, p1, p2, p3, p4);

        const auto base  = reinterpret_cast<uintptr_t>(cam);
        const auto level = *reinterpret_cast<uintptr_t*>(base + kLevelOff);
        if (!level)
            return shUpdateView.unsafe_fastcall(cam, edx, p1, p2, p3, p4);

        float& levelDt = *reinterpret_cast<float*>(level + kDeltaTimeOff);
        const float realDt = levelDt;
        if (!(realDt > 0.0f))
            return shUpdateView.unsafe_fastcall(cam, edx, p1, p2, p3, p4);

        accumulator += realDt;

        if (accumulator < kStep)
        {
            // Stop speed updates but allow camera rotation
            const int saved[10] = {
                *reinterpret_cast<int*>(base + kHitRoll),
                *reinterpret_cast<int*>(base + kHitFadeOut),
                *reinterpret_cast<int*>(base + kShakeRoll),
                *reinterpret_cast<int*>(base + kShakeTarget),
                *reinterpret_cast<int*>(base + kShakeSpeed),
                *reinterpret_cast<int*>(base + kShakeFadeOut),
                *reinterpret_cast<int*>(base + kTiltPitch),
                *reinterpret_cast<int*>(base + kTiltTarget),
                *reinterpret_cast<int*>(base + kTiltSpeed),
                *reinterpret_cast<int*>(base + kTiltFadeOut),
            };

            *reinterpret_cast<int*>(base + kHitFadeOut)   = 0;
            *reinterpret_cast<int*>(base + kShakeSpeed)   = 0;
            *reinterpret_cast<int*>(base + kShakeFadeOut) = 0;
            *reinterpret_cast<int*>(base + kTiltSpeed)    = 0;
            *reinterpret_cast<int*>(base + kTiltFadeOut)  = 0;

            shUpdateView.unsafe_fastcall(cam, edx, p1, p2, p3, p4);

            *reinterpret_cast<int*>(base + kHitRoll)      = saved[0];
            *reinterpret_cast<int*>(base + kHitFadeOut)   = saved[1];
            *reinterpret_cast<int*>(base + kShakeRoll)    = saved[2];
            *reinterpret_cast<int*>(base + kShakeTarget)  = saved[3];
            *reinterpret_cast<int*>(base + kShakeSpeed)   = saved[4];
            *reinterpret_cast<int*>(base + kShakeFadeOut) = saved[5];
            *reinterpret_cast<int*>(base + kTiltPitch)    = saved[6];
            *reinterpret_cast<int*>(base + kTiltTarget)   = saved[7];
            *reinterpret_cast<int*>(base + kTiltSpeed)    = saved[8];
            *reinterpret_cast<int*>(base + kTiltFadeOut)  = saved[9];
            return;
        }

        int steps = 0;
        while (accumulator >= kStep)
        {
            accumulator -= kStep;
            if (++steps >= kMaxSteps)
            {
                accumulator = 0.0f; // discard excess accumulated time to avoid stalls
                break;
            }
        }

        levelDt = kStep;
        for (int i = 0; i < steps; ++i)
            shUpdateView.unsafe_fastcall(cam, edx, p1, p2, p3, p4);
        levelDt = realDt;
    }
}

export void InitEchelon()
{
    //EPlayerController additional state cache
    AEPlayerController::shTick = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Echelon"), "?Tick@AEPlayerController@@UAEHMW4ELevelTick@@@Z"), AEPlayerController::Tick);

    // EPlayerCam FPS fixes
    AEPlayerCam::shUpdateView = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Echelon"), "?UpdateView@AEPlayerCam@@QAEXVFRotator@@H@Z"), AEPlayerCam::UpdateView);

    // Letterboxing
    auto pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "D9 1C 24 50 53 E8 ? ? ? ? 5F");
    AETextureManager::hbDrawTileFromManager.fun = injector::MakeCALL(pattern.get_first(5), AETextureManager::DrawTileFromManager, true).get();

    // Hud Helpers
    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "85 C0 7C ? 8B 89 A8 02 00 00");
    static auto AETextureManagerDrawTileFromManagerHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        curDrawTileManagerTextureIndex = regs.eax;
    });

    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "85 C0 7C ? D9 44 24 08");
    static auto AETextureManagerexecDrawTileFromManagerHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        curDrawTileManagerTextureIndex = regs.eax;
    });

    // Set player speed to max on game start
    if (!IsEnhanced())
    {
        UIntOverrides::Register(L"IntProperty Echelon.EchelonGameInfo.m_defautSpeed", +[]() -> int
        {
            return 5;
        });
    }

    // Camera acceleration
    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "7A ? D9 81 DC 03 00 00");
    injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); // jp -> jmp

    // Camera minYaw/maxYaw
    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "E8 ? ? ? ? 89 44 24 ? 8A 86");
    static auto AEPlayerCamUpdateRotationForSamHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto& minYaw = *(int32_t*)(regs.esp + 0x24);
        auto& maxYaw = *(int32_t*)(regs.esp + 0x28);

        enum ECamMode
        {
            ECM_Walking,
            ECM_WalkingCr,
            ECM_FirstPerson,
            ECM_FirstPersonCr,
            ECM_Grab,
            ECM_GrabFP,
            ECM_Carry,
            ECM_CarryCr,
            ECM_Throw,
            ECM_ThrowCr,
            ECM_Sniping,
            ECM_HSphere,
            ECM_FSphere,
            ECM_HOH,
            ECM_HOHFU,
            ECM_HOHFP,
            ECM_BTW,
            ECM_BTWPeakLeft,
            ECM_BTWPeakRight,
            ECM_BTWLeftFP,
            ECM_BTWRightFP,
            ECM_Rapel,
            ECM_RapelFP,
            ECM_DoorPeekRight,
            ECM_DoorPeekLeft,
            ECM_SplitJump,
            ECM_SplitJumpFP,
        };

        auto camMode = ECamMode(*(int8_t*)(regs.esi + 0x12B0));

        if (camMode == ECM_BTWPeakLeft || camMode == ECM_BTWPeakRight)
        {
            if (UObject::GetState(L"EPlayerController") == L"s_PlayerBTWThrow")
            {
                minYaw = -9000;
                maxYaw = 3000;
            }
        }
    });
}
