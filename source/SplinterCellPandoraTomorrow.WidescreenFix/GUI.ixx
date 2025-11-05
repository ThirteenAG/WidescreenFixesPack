module;

#include <stdafx.h>

export module GUI;

import ComVars;

// Helper to compute delta and bounds for X axis
inline void GetXAxisBounds(int32_t& outDelta, int32_t& outVmin, int32_t& outVmax)
{
    outDelta = static_cast<int32_t>(((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f);
    outVmin = static_cast<int32_t>(0 - outDelta);
    outVmax = static_cast<int32_t>(640 + outDelta);
}

// Helper to convert raw input to normalized [0, 1] range
inline float NormalizeRawInputX(int32_t rawX)
{
    return static_cast<float>(rawX) / static_cast<float>(Screen.Width);
}

inline float NormalizeRawInputY(int32_t rawY)
{
    return static_cast<float>(rawY) / static_cast<float>(Screen.Height);
}

// Helper to convert normalized coordinates to game space
inline int32_t NormalizedToGameX(float normalized, int32_t minGame, int32_t maxGame)
{
    return static_cast<int32_t>(normalized * (maxGame - minGame) + minGame);
}

inline int32_t NormalizedToGameY(float normalized, int32_t minGame, int32_t maxGame)
{
    return static_cast<int32_t>(normalized * (maxGame - minGame) + minGame);
}

// Helper to convert game coordinates back to raw input space
inline int32_t GameToRawInputX(int32_t gameX, int32_t vmin, int32_t vmax)
{
    float normalized = static_cast<float>(gameX - vmin) / static_cast<float>(vmax - vmin);
    return static_cast<int32_t>(normalized * Screen.Width);
}

inline int32_t GameToRawInputY(int32_t gameY, int32_t minGame, int32_t maxGame)
{
    float normalized = static_cast<float>(gameY) / static_cast<float>(maxGame);
    return static_cast<int32_t>(normalized * Screen.Height);
}

int32_t __cdecl ClampXAxisState(int32_t value, int32_t min, int32_t max)
{
    int32_t delta, vmin, vmax;
    GetXAxisBounds(delta, vmin, vmax);

    if (Screen.bRawInputMouseForMenu)
    {
        float normalizedX = NormalizeRawInputX(RawMouseCursorX);
        int32_t menuX = NormalizedToGameX(normalizedX, vmin, vmax);
        return std::clamp(menuX, vmin, vmax);
    }
    else
        return std::clamp(value, int32_t(0 - delta), int32_t(640 + delta));
}

int32_t __cdecl ClampYAxisState(int32_t value, int32_t min, int32_t max)
{
    max += 15;

    if (Screen.bRawInputMouseForMenu)
    {
        float normalizedY = NormalizeRawInputY(RawMouseCursorY);
        int32_t menuY = NormalizedToGameY(normalizedY, min, max);
        return std::clamp(menuY, min, max);
    }
    else
        return std::clamp(value, min, max);
}

export void InitGUI()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"GUI"), "85 C0 79 ? 33 DB EB 0A BB 80 02 00 00 3B C3 0F 4C D8");
    if (!pattern.empty())
    {
        struct UGUIControllerNativePostRenderHookX
        {
            void operator()(injector::reg_pack& regs)
            {
                *(int32_t*)&regs.ebx = ClampXAxisState(int32_t(regs.eax), int32_t(0), int32_t(640));
            }
        }; injector::MakeInline<UGUIControllerNativePostRenderHookX>(pattern.get_first(0), pattern.get_first(18));
    }
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"GUI"), "3B C5 7D ? 89 6C 24 38");
        struct UGUIControllerNativePostRenderHookX
        {
            void operator()(injector::reg_pack& regs)
            {
                *(int32_t*)(regs.esp + 0x38) = ClampXAxisState(int32_t(regs.eax), int32_t(0), int32_t(640));
            }
        }; injector::MakeInline<UGUIControllerNativePostRenderHookX>(pattern.get_first(0), pattern.get_first(29));
    }

    pattern = hook::module_pattern(GetModuleHandle(L"GUI"), "85 C9 79 ? C7 45 08 00 00 00 00");
    if (!pattern.empty())
    {
        struct UGUIControllerNativePostRenderHookY
        {
            void operator()(injector::reg_pack& regs)
            {
                *(int32_t*)(regs.ebp + 8) = ClampYAxisState(int32_t(regs.ecx), int32_t(0), int32_t(480));
            }
        }; injector::MakeInline<UGUIControllerNativePostRenderHookY>(pattern.get_first(0), pattern.get_first(26));
    }
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"GUI"), "3B C5 7C ? 3D E0 01 00 00");
        struct UGUIControllerNativePostRenderHookY
        {
            void operator()(injector::reg_pack& regs)
            {
                *(int32_t*)&regs.ebp = ClampYAxisState(int32_t(regs.eax), int32_t(0), int32_t(480));
            }
        }; injector::MakeInline<UGUIControllerNativePostRenderHookY>(pattern.get_first(0), pattern.get_first(18));
    }
}
