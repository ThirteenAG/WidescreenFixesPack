module;

#include <stdafx.h>

export module Rendering;

import ComVars;
import Resolution;

constexpr float baseAspect = 4.0f / 3.0f;

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

class Rendering
{
public:
    Rendering()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            static float fCustomFieldOfView = iniReader.ReadFloat("MAIN", "FOVFactor", 0.0f);

            onResChange() += [](int Width, int Height)
            {
                fAspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
                fFieldOfView = 2.0f * std::atan(std::tan(1.308f * 0.5f) * (fAspectRatio / baseAspect));
                if (fCustomFieldOfView)
                    fFieldOfView *= fCustomFieldOfView;
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

                    auto x1 = *(float*)(*(DWORD*)(regs.edi + 8) + (regs.ebp - 0x30) + 0x10);
                    auto x2 = *(float*)(*(DWORD*)(regs.edi + 8) + (regs.ebp - 0x30) + 24)
                        + *(float*)((regs.ebp - 0x30) + *(DWORD*)(regs.edi + 8) + 16);
                    if (x1 == 0.0f && x2 == 1.0f && retAddr == dword_540F2E)
                        return;

                    const float scale = fAspectRatio / baseAspect;
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
                        // Radar widget: maintain 4:3 physical size, anchored bottom-right
                        //const float scaleH = (float)BackbufferHeight / 480.0f;
                        //const float radarW_px = 152.0f * scaleH;
                        //const float marginR_px = 24.0f * scaleH;
                        //const float w = radarW_px / (float)BackbufferWidth;
                        //const float x = 1.0f - w - marginR_px / (float)BackbufferWidth;
                        //*(float*)(regs.esi + 0x14) = w;
                        //*(float*)(regs.esi + 0x0C) = x;

                        // Radar widget: maintain 4:3 physical size, anchored to bottom-right of 4:3 safe area
                        const float scaleH = (float)BackbufferHeight / 480.0f;
                        const float radarW_px = 152.0f * scaleH;
                        const float marginR_px = 24.0f * scaleH;
                        const float w = radarW_px / (float)BackbufferWidth;
                        const float marginR = marginR_px / (float)BackbufferWidth;
                        const float safeRight = 0.5f + 0.5f * (baseAspect / fAspectRatio);
                        const float x = safeRight - w - marginR;
                        *(float*)(regs.esi + 0x14) = w;
                        *(float*)(regs.esi + 0x0C) = x;
                    }
                }
            }; injector::MakeInline<RadarHook>(pattern.get_first(0), pattern.get_first(12));

            // Map game widget (disable)
            //pattern = hook::pattern("83 EC ? 53 8B 5C 24 ? 57 8B F9");
            //injector::MakeRET(pattern.get_first(), 4, true);

            // Object disappearance fix
            pattern = hook::pattern("D9 83 80 01 00 00 ? ? ? 01 00 00");
            injector::WriteMemory<uint8_t>(pattern.count(2).get(0).get<void*>(2), 0x84i8, true); //0x57D4C2
            injector::WriteMemory<uint8_t>(pattern.count(2).get(1).get<void*>(2), 0x84i8, true); //0x57D505
        };
    }
} Rendering;