module;

#include <stdafx.h>
#include <mutex>
#include <unordered_map>

export module Echelon;

import ComVars;

namespace AEPlayerController
{
    void* lastController = nullptr;
    void* lastState = nullptr;
    std::wstring lastType;

    void Reset()
    {
        if (lastController)
            UObject::objectStates.erase(lastType);
        lastController = nullptr;
        lastState = nullptr;
        lastType.clear();
    }

    SafetyHookInline shTick = {};
    void __fastcall Tick(void* playerController, void* edx, int a2, int a3)
    {
        if (playerController)
        {
            const auto object = static_cast<void**>(playerController);
            const auto frame = static_cast<void**>(object[3]);
            // An inactive state frame points at the object's class.
            void* state = frame && frame[7] != object[9] ? frame[7] : nullptr;
            if (playerController != lastController || state != lastState)
            {
                wchar_t buffer[256];
                if (playerController != lastController)
                {
                    Reset();
                    const std::wstring_view name(UObject::GetFullName(playerController, edx, buffer));
                    lastType = name.substr(0, name.find(L' '));
                    lastController = playerController;
                }
                lastState = state;
                if (state)
                {
                    const std::wstring_view name(UObject::GetFullName(state, edx, buffer));
                    const auto dot = name.rfind(L'.');
                    UObject::objectStates[lastType] = dot == name.npos ? name : name.substr(dot + 1);
                }
                else
                    UObject::objectStates.erase(lastType);
            }
        }
        return shTick.unsafe_fastcall(playerController, edx, a2, a3);
    }

    // Ledge prediction looks two frames ahead with the level frame time, too short to reach a ledge at high frame rates
    constexpr float kLedgeStep = 1.0f / 30.0f;
    constexpr uintptr_t kLevelOff = 0x78;
    constexpr uintptr_t kDeltaTimeOff = 0x75C;

    SafetyHookInline shCheckLedgeGrabArms = {};
    int __fastcall CheckLedgeGrabArms(void* pc, void* edx, void* p1, void* p2, void* p3, void* p4, void* p5)
    {
        if (pc)
        {
            const auto level = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(pc) + kLevelOff);
            if (level)
            {
                float& levelDt = *reinterpret_cast<float*>(level + kDeltaTimeOff);
                const float realDt = levelDt;
                if (realDt > 0.0f && realDt < kLedgeStep)
                {
                    levelDt = kLedgeStep;
                    const int r = shCheckLedgeGrabArms.unsafe_fastcall<int>(pc, edx, p1, p2, p3, p4, p5);
                    levelDt = realDt;
                    return r;
                }
            }
        }
        return shCheckLedgeGrabArms.unsafe_fastcall<int>(pc, edx, p1, p2, p3, p4, p5);
    }
}

// Camera hit, tilt and shake move in whole rotation units per frame; at high frame rates the steps round to zero
namespace AEPlayerCam
{
    constexpr float kStep = 1.0f / 30.0f;
    constexpr int kMaxSteps = 8;

    constexpr uintptr_t kHitRoll = 0x124C;
    constexpr uintptr_t kHitFadeOut = 0x1250;
    constexpr uintptr_t kTiltPitch = 0x1254;
    constexpr uintptr_t kTiltTarget = 0x1258;
    constexpr uintptr_t kTiltSpeed = 0x125C;
    constexpr uintptr_t kTiltFadeOut = 0x1260;
    constexpr uintptr_t kShakeRoll = 0x1264;
    constexpr uintptr_t kShakeTarget = 0x1268;
    constexpr uintptr_t kShakeSpeed = 0x126C;
    constexpr uintptr_t kShakeFadeOut = 0x1270;

    constexpr uintptr_t kLevelOff = 0x78;
    constexpr uintptr_t kDeltaTimeOff = 0x75C;

    std::mutex clockMutex;
    std::unordered_map<void*, float> elapsed;

    int EffectSteps(void* camera, float delta)
    {
        std::lock_guard lock(clockMutex);
        auto& accumulator = elapsed[camera];
        accumulator += delta;
        int steps = 0;
        while (accumulator >= kStep)
        {
            accumulator -= kStep;
            if (++steps == kMaxSteps)
            {
                accumulator = 0.0f;
                break;
            }
        }
        return steps;
    }

    SafetyHookInline shUpdateView = {};
    void __fastcall UpdateView(void* cam, void* edx, int pitch, int yaw, int roll, int bAdd)
    {
        if (!cam)
            return shUpdateView.unsafe_fastcall(cam, edx, pitch, yaw, roll, bAdd);

        const auto base = reinterpret_cast<uintptr_t>(cam);
        const auto level = *reinterpret_cast<uintptr_t*>(base + kLevelOff);
        if (!level)
            return shUpdateView.unsafe_fastcall(cam, edx, pitch, yaw, roll, bAdd);

        float& levelDt = *reinterpret_cast<float*>(level + kDeltaTimeOff);
        const float realDt = levelDt;
        if (!(realDt > 0.0f))
            return shUpdateView.unsafe_fastcall(cam, edx, pitch, yaw, roll, bAdd);

        const int steps = EffectSteps(cam, realDt);
        if (!steps)
        {
            // Apply the view without advancing the effects
            constexpr uintptr_t kEffects[] = { kHitRoll, kHitFadeOut, kTiltPitch, kTiltTarget, kTiltSpeed, kTiltFadeOut, kShakeRoll, kShakeTarget, kShakeSpeed, kShakeFadeOut };
            int saved[std::size(kEffects)];
            for (size_t i = 0; i < std::size(kEffects); ++i)
                saved[i] = *reinterpret_cast<int*>(base + kEffects[i]);

            *reinterpret_cast<int*>(base + kHitFadeOut) = 0;
            *reinterpret_cast<int*>(base + kShakeSpeed) = 0;
            *reinterpret_cast<int*>(base + kShakeFadeOut) = 0;
            *reinterpret_cast<int*>(base + kTiltSpeed) = 0;
            *reinterpret_cast<int*>(base + kTiltFadeOut) = 0;

            shUpdateView.unsafe_fastcall(cam, edx, pitch, yaw, roll, bAdd);

            for (size_t i = 0; i < std::size(kEffects); ++i)
                *reinterpret_cast<int*>(base + kEffects[i]) = saved[i];
            return;
        }

        levelDt = kStep;
        for (int i = 0; i < steps; ++i)
            shUpdateView.unsafe_fastcall(cam, edx, pitch, yaw, roll, bAdd);
        levelDt = realDt;
    }
}

export void ResetPlayerControllerState(void* object = nullptr)
{
    if (!object || object == AEPlayerController::lastController || object == AEPlayerController::lastState)
        AEPlayerController::Reset();
}

export void InitEchelon()
{
    // EPlayerController additional state cache
    auto pattern = hook::pattern("8B 88 ? ? ? ? 8B 6E");
    AEPlayerController::shTick = safetyhook::create_inline(pattern.get_first(-10), AEPlayerController::Tick);

    // Ledge grab at high frame rates
    pattern = hook::pattern("A0 ? ? ? ? 81 EC 88 01 00 00 A8 01 53 55 56 57 8B D9");
    AEPlayerController::shCheckLedgeGrabArms = safetyhook::create_inline(pattern.get_first(), AEPlayerController::CheckLedgeGrabArms);

    // Camera hit, tilt and shake at high frame rates
    pattern = hook::pattern("81 EC 64 01 00 00 53 55 8B D9 8B 83 4C 12 00 00");
    AEPlayerCam::shUpdateView = safetyhook::create_inline(pattern.get_first(), AEPlayerCam::UpdateView);

    // ConditionalDestroy sets the destroyed flag before dispatching the native virtual destructor path.
    pattern = hook::pattern("0D 00 00 80 00 89 41 1C 8B 01 FF 50 24 B8 01 00 00 00");
    static auto destroyHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        ResetPlayerControllerState(reinterpret_cast<void*>(regs.ecx));
        std::lock_guard lock(AEPlayerCam::clockMutex);
        AEPlayerCam::elapsed.erase(reinterpret_cast<void*>(regs.ecx));
    });

    // Aiming camera smoothing
    pattern = hook::pattern("6B D2 ? 8B 8C 32");
    injector::WriteMemory<uint8_t>(pattern.get_first(2), 0, true);
}
