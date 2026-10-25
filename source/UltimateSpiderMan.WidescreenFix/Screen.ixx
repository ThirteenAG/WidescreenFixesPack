module;

#include "stdafx.h"
#include "../Shared/PatchPreflight.hpp"
#include "FixSettings.hpp"
#include <atomic>
#include "CameraState.hpp"

export module Screen;

export class Screen
{
public:
    Screen();
    int32_t Width = 640;
    int32_t Height = 480;
    float fAspectRatio = 4.0f / 3.0f;
    float fAspectRatioDiff = 1.0f;
    float fHudScaleX = 1.0f;
    float fHudScaleY = 1.0f;
    float fHudOffset = 0.0f;
    float fFOVFactor = 1.0f;
    std::optional<float> fHudAspectRatioConstraint;

    void Update(int32_t width, int32_t height)
    {
        if (width <= 0 || height <= 0)
            return;

        Width = width;
        Height = height;
        fAspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
        fAspectRatioDiff = fAspectRatio / (4.0f / 3.0f);
        fHudScaleX = std::min(1.0f, 1.0f / fAspectRatioDiff);
        fHudScaleY = std::min(1.0f, fAspectRatioDiff);

        auto constraint = fHudAspectRatioConstraint.value_or(fAspectRatio);
        if (!std::isfinite(constraint) || constraint <= 0.0f)
            constraint = 4.0f / 3.0f;
        constraint = std::max(4.0f / 3.0f, std::min(constraint, fAspectRatio));
        fHudOffset = 240.0f * (constraint - 4.0f / 3.0f);
    }

    float GameplayFOVFactor() const { return USMCamera::IsGameplay() ? fFOVFactor : 1.0f; }

    float HudX(float x) const { return 320.0f + (x - 320.0f) * fHudScaleX; }
    float HudY(float y) const { return 240.0f + (y - 240.0f) * fHudScaleY; }
    float UnscaleX(float x) const { return 320.0f + (x - 320.0f) / fHudScaleX; }
    float UnscaleY(float y) const { return 240.0f + (y - 240.0f) / fHudScaleY; }
} Screen;

export uintptr_t* pCurrentScene = nullptr;
int32_t* pScreenWidth = nullptr;

export bool IsBackBufferScene()
{
    auto scene = *pCurrentScene; // nglCurScene
    if (!scene)
        return false;
    auto target = *reinterpret_cast<uintptr_t*>(scene + 0x334);
    return target && (*reinterpret_cast<uint32_t*>(target + 0x34) & 4) != 0;
}

namespace
{
    std::atomic<bool> ScreenSettingsPending{ false };

    void ApplyScreenSettings()
    {
        auto& settings = FixSettings();
        const auto factor = settings.Float("MAIN.FOVFactor");
        Screen.fFOVFactor = std::isfinite(factor) ? std::clamp(factor, 0.5f, 2.0f) : 1.0f;
        auto constraint = ParseWidescreenHudOffset(settings.String("MAIN.HudAspectRatioConstraint"));
        if (constraint && (!std::isfinite(*constraint) || *constraint <= 0.0f))
            constraint.reset();
        if (constraint)
            *constraint = std::clamp(*constraint, 4.0f / 3.0f, 32.0f / 9.0f);
        Screen.fHudAspectRatioConstraint = constraint;
        Screen.Update(Screen.Width, Screen.Height);
    }

    void ApplyPendingScreenSettings()
    {
        if (ScreenSettingsPending.exchange(false))
            ApplyScreenSettings();
    }
}

Screen::Screen()
{
    WFP::onInitEvent() += []
    {
        if (!wfp::PatchPreflight("Ultimate Spider-Man screen: signatures unavailable; feature disabled.\n",
        {
            { "8B 0D ? ? ? ? 8A 81 ? ? ? ? 84 C0 74 ? E8 ? ? ? ? 84 C0 75 ? 6A", 1 },
            { "A3 ? ? ? ? FF D6 50", 1 },
            { "68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 8B 35", 1 },
            { "A3 ? ? ? ? ? ? ? ? ? ? 83 C4 ? 6A ? ? ? ? ? ? ? 68", 1 },
        }))
            return;

        ApplyScreenSettings();
        for (const auto id : { "MAIN.FOVFactor", "MAIN.HudAspectRatioConstraint" })
        {
            FixSettings().OnChange(id, []()
            {
                ScreenSettingsPending.store(true);
            });
        }
        WFP::onGameProcessEvent() += ApplyPendingScreenSettings;

        auto pattern = hook::pattern("8B 0D ? ? ? ? 8A 81 ? ? ? ? 84 C0 74 ? E8 ? ? ? ? 84 C0 75 ? 6A"); //0x406516 + 2
        pCurrentScene = *pattern.get_first<uintptr_t*>(2); // nglCurScene (0x971F00)

        pattern = hook::pattern("A3 ? ? ? ? FF D6 50"); //0x5AC3F9 + 1
        pScreenWidth = *pattern.get_first<int32_t*>(1);

        auto [width, height] = GetDesktopRes();
        static std::string defaultResolution = std::to_string(width) + "x" + std::to_string(height);
        // Saved menu settings take precedence over this startup fallback.
        pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 8B 35"); //0x5AC3C8 + 1
        WriteGameValue(pattern.get_first(1), defaultResolution.c_str());

        // Observe the parsed height in EAX; the original store still runs afterwards.
        pattern = hook::pattern("A3 ? ? ? ? ? ? ? ? ? ? 83 C4 ? 6A ? ? ? ? ? ? ? 68"); //0x5AC409
        static auto ResolutionHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            Screen.Update(*pScreenWidth, regs.eax);
        });
    };
}
