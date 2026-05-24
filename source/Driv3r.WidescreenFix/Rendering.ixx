module;

#include <stdafx.h>

export module Rendering;

import ComVars;
import Resolution;

constexpr float baseAspect = 4.0f / 3.0f;
std::optional<float> fHudAspectRatioConstraint;
float targetHudAspect = 16.0f / 9.0f;

ProtectedGameRef<float> fAspectRatio([]() -> float*
{
    auto pattern = find_pattern("D9 05 ? ? ? ? EB ? D9 05 ? ? ? ? 8B 75 0C");
    if (!pattern.empty())
        return *pattern.get_first<float*>(2);
    return nullptr;
});

ProtectedGameRef<float> fFieldOfView([]() -> float*
{
    auto pattern = find_pattern("D9 05 ? ? ? ? EB 06 D9 05 ? ? ? ? 33 C0");
    if (!pattern.empty())
        return *pattern.get_first<float*>(2);
    return nullptr;
});

enum ScreenModes
{
    LETTERBOX = 0,
    UNKNOWN = 1,
    WIDE16BY9 = 2
};

bool bHUD = false;
SafetyHookInline shsub_53BC50 = {};
void __fastcall sub_53BC50(void* _this, void* edx, int a2)
{
    bHUD = true;
    shsub_53BC50.unsafe_fastcall(_this, edx, a2);
    bHUD = false;
}

class Rendering
{
public:
    Rendering()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            static float fCustomFieldOfView = iniReader.ReadFloat("MAIN", "FOVFactor", 0.0f);
            fHudAspectRatioConstraint = ParseWidescreenHudOffset(iniReader.ReadString("MAIN", "HudAspectRatioConstraint", ""));

            onResChange() += [](int Width, int Height)
            {
                fAspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
                fFieldOfView = 2.0f * std::atan(std::tan(1.04f * 0.5f) * (fAspectRatio / baseAspect));
                if (fCustomFieldOfView)
                    fFieldOfView *= fCustomFieldOfView;

                targetHudAspect = fAspectRatio;
                if (fHudAspectRatioConstraint.has_value())
                {
                    const float value = fHudAspectRatioConstraint.value();
                    if (value >= 0.0f && value <= fAspectRatio)
                        targetHudAspect = ClampHudAspectRatio(value, fAspectRatio);
                }
            };

            auto pattern = hook::pattern("8B 44 24 04 89 41 ? B0 01 C2 04 00"); //0x580C00
            struct MenuAspectRatioSwitchHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint32_t*)(regs.ecx + 0x14) = WIDE16BY9;
                }
            }; injector::MakeInline<MenuAspectRatioSwitchHook>(pattern.get_first(0), pattern.get_first(7));

            // 2D — uniform hor+ correction; skip fullscreen fades and radar border/overlay
            pattern = hook::pattern("89 56 DC 83 C0 01 D9 5E D4 89 44 24 10"); //0x57E5BD
            static auto dword_53F06F = (uint32_t)hook::get_pattern("80 BE 40 02 00 00 00", 0);
            static auto dword_53F08A = (uint32_t)hook::get_pattern("8D 46 3C 53 E8 ? ? ? ? 5F", 9);
            static auto dword_540F2E = (uint32_t)hook::get_pattern("8D 86 9C 01 00 00 51 E8 ? ? ? ? C2", 12);
            static auto dword_53FD96 = (uint32_t)hook::get_pattern("83 C6 ? 83 ED ? 0F 85 ? ? ? ? 53", 0);
            struct HudHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    auto retAddr = *(uint32_t*)(regs.esp + 0x4C);

                    *(uint32_t*)(regs.esi - 0x24) = regs.edx;
                    regs.eax += 1;
                    float temp = 0.0f;
                    _asm { fstp dword ptr[temp] }
                    *(float*)(regs.esi - 0x2C) = temp;

                    // Skip radar border/overlay — position handled by RadarHook via sub_53D790
                    // Skip fullscreen fades — they cover the full widescreen viewport intentionally
                    if (retAddr == dword_53F06F || retAddr == dword_53F08A)
                        return;

                    if (bHUD)
                    {
                        const float hudTopThreshold = 0.35f;    // [0..1], only elements above this Y are eligible
                        const float hudLeftThreshold = 0.22f;   // [0..1], left-corner band width
                        const float hudRightThreshold = 0.22f;  // [0..1], right-corner band width

                        const float currentAspect = fAspectRatio;
                        const float constrainedAspect = std::clamp(currentAspect, baseAspect, targetHudAspect);
                        const float stretchScale = std::max(1.0f, currentAspect / baseAspect);
                        const float sideOffset = (1.0f - (baseAspect / constrainedAspect)) * targetHudAspect / currentAspect;

                        const uintptr_t src = *(uintptr_t*)(regs.edi + 8) + (regs.ebp - 0x30);
                        const float srcX1 = *(float*)(src + 0x10);
                        const float srcY1 = *(float*)(src + 0x14);
                        const float srcX2 = *(float*)(src + 0x18) + srcX1; // x + w
                        const float srcY2 = *(float*)(src + 0x1C) + srcY1; // y + h

                        // Only top row items can be moved.
                        const bool isTop = (srcY1 <= hudTopThreshold);
                        if (!isTop)
                            return;

                        // Only true corners can be moved.
                        const bool isLeftCorner = (srcX1 <= hudLeftThreshold);
                        const bool isRightCorner = (srcX2 >= (1.0f - hudRightThreshold));

                        // Skip center/edge-crossing items (neither or both).
                        if (isLeftCorner == isRightCorner)
                            return;

                        float* x0 = (float*)((regs.esi - 0x90) - 0x14);
                        float* x1v = (float*)((regs.esi - 0x90) + 0x04);
                        float* x2v = (float*)((regs.esi - 0x90) + 0x1C);
                        float* x3v = (float*)((regs.esi - 0x90) + 0x34);
                        float* x4 = (float*)((regs.esi - 0x44) + 0x00);
                        float* x5 = (float*)((regs.esi - 0x2C) + 0x00);

                        *x0 /= stretchScale; *x1v /= stretchScale; *x2v /= stretchScale;
                        *x3v /= stretchScale; *x4 /= stretchScale; *x5 /= stretchScale;

                        const float bias = isLeftCorner ? -sideOffset : sideOffset;
                        *x0 += bias; *x1v += bias; *x2v += bias;
                        *x3v += bias; *x4 += bias; *x5 += bias;
                        return;
                    }

                    auto x1 = *(float*)(*(uintptr_t*)(regs.edi + 8) + (regs.ebp - 0x30) + 0x10);
                    auto x2 = *(float*)(*(uintptr_t*)(regs.edi + 8) + (regs.ebp - 0x30) + 24)
                        + *(float*)((regs.ebp - 0x30) + *(uintptr_t*)(regs.edi + 8) + 16);

                    if (x1 == 0.0f && x2 == 1.0f && retAddr == dword_540F2E)
                        return;

                    const float scale = std::max(1.0f, fAspectRatio / baseAspect);
                    *(float*)((regs.esi - 0x90) - 0x14) /= scale; // vertex 0 X
                    *(float*)((regs.esi - 0x90) + 0x04) /= scale; // vertex 1 X
                    *(float*)((regs.esi - 0x90) + 0x1C) /= scale; // vertex 2 X
                    *(float*)((regs.esi - 0x90) + 0x34) /= scale; // vertex 3 X
                    *(float*)((regs.esi - 0x44) + 0x00) /= scale; // vertex 4 X
                    *(float*)((regs.esi - 0x2C) + 0x00) /= scale; // vertex 5 X
                }
            }; injector::MakeInline<HudHook>(pattern.get_first(0), pattern.get_first(9));

            // Radar
            pattern = hook::pattern("89 9E EC 01 00 00 89 9E F0 01 00 00"); //0x53DE73
            struct RadarHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint32_t*)(regs.esi + 0x1EC) = regs.ebx;
                    *(uint32_t*)(regs.esi + 0x1F0) = regs.ebx;

                    if (*(float*)(regs.esi + 0x0C) == 0.0f && *(float*)(regs.esi + 0x14) == 1.0f)
                    {
                        // Full-screen map view: shrink to 4:3 safe area
                        float fHudOffsetReal = ((float)BackbufferWidth - (float)BackbufferHeight * baseAspect) / 2.0f;
                        float offset = fHudOffsetReal / (float)BackbufferWidth;
                        *(float*)(regs.esi + 0x0C) = offset;
                        *(float*)(regs.esi + 0x14) = 1.0f - 2.0f * offset;
                    }
                    else
                    {
                        const float currentAspect = fAspectRatio;
                        const float constrainedAspect = std::clamp(currentAspect, baseAspect, targetHudAspect);

                        const float h = *(float*)(regs.esi + 0x18);
                        const float radarH_px = h * (float)BackbufferHeight;
                        const float marginR_px = 24.0f * ((float)BackbufferHeight / 480.0f);

                        // Make width equal to height in pixels
                        const float w = radarH_px / (float)BackbufferWidth;
                        const float marginR = marginR_px / (float)BackbufferWidth;

                        // Right edge of target area (16:9 max), clamped so narrow AR uses full screen
                        const float safeRight = 0.5f + 0.5f * std::min(1.0f, constrainedAspect / currentAspect);
                        const float x = safeRight - w - marginR;

                        *(float*)(regs.esi + 0x14) = w;
                        *(float*)(regs.esi + 0x0C) = x;
                    }
                }
            }; injector::MakeInline<RadarHook>(pattern.get_first(0), pattern.get_first(12));

            // Object disappearance fix
            pattern = hook::pattern("D9 83 80 01 00 00 ? ? ? 01 00 00");
            injector::WriteMemory<uint8_t>(pattern.count(2).get(0).get<void*>(2), 0x84i8, true); //0x57D4C2
            injector::WriteMemory<uint8_t>(pattern.count(2).get(1).get<void*>(2), 0x84i8, true); //0x57D505

            pattern = hook::pattern("83 EC ? 53 8B 5C 24 ? 57 8B F9");
            shsub_53BC50 = safetyhook::create_inline(pattern.get_first(), sub_53BC50);
        };
    }
} Rendering;