module;

#include "stdafx.h"

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

Screen::Screen()
{
    WFP::onInitEvent() += []
    {
        CIniReader iniReader("");
        Screen.fFOVFactor = iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f);
        Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(iniReader.ReadString("MAIN", "HudAspectRatioConstraint", "4:3"));
        if (!std::isfinite(Screen.fFOVFactor) || Screen.fFOVFactor <= 0.0f)
            Screen.fFOVFactor = 1.0f;

        auto pattern = hook::pattern("8B 0D ? ? ? ? 8A 81 ? ? ? ? 84 C0 74 ? E8 ? ? ? ? 84 C0 75 ? 6A"); //0x406516 + 2
        pCurrentScene = *pattern.get_first<uintptr_t*>(2); // nglCurScene (0x971F00)

        pattern = hook::pattern("A3 ? ? ? ? FF D6 50"); //0x5AC3F9 + 1
        pScreenWidth = *pattern.get_first<int32_t*>(1);

        auto [width, height] = GetDesktopRes();
        static std::string defaultResolution = std::to_string(width) + "x" + std::to_string(height);
        // Saved menu settings take precedence over this startup fallback.
        pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 8B 35"); //0x5AC3C8 + 1
        injector::WriteMemory(pattern.get_first(1), defaultResolution.c_str(), true);

        // Observe the parsed height in EAX; the original store still runs afterwards.
        pattern = hook::pattern("A3 ? ? ? ? ? ? ? ? ? ? 83 C4 ? 6A ? ? ? ? ? ? ? 68"); //0x5AC409
        static auto ResolutionHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            Screen.Update(*pScreenWidth, regs.eax);
        });
    };
}
