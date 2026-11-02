module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module WidescreenHUD;

import ComVars;

// The HUD is laid out in fractions of the 4:3 screen and drawn in the centered 4:3 area. HUD groups are identified by
// their frame (the parent rectangle), single elements of the top frame by their position; those at the sides of the
// original screen move to the sides of the HUD box (the whole screen in Auto).
struct HUDIdentifier
{
    float x, y;

    bool Matches(float valueX, float valueY) const
    {
        return std::fabs(valueX - x) < 0.001f && std::fabs(valueY - y) < 0.001f;
    }
};

constexpr HUDIdentifier Bits(uint32_t x, uint32_t y)
{
    return { std::bit_cast<float>(x), std::bit_cast<float>(y) };
}

// Groups at the right: health silhouette and bars, badge meter, weapon icon and name, ammunition
constexpr HUDIdentifier RightGroups[] =
{
    Bits(0x3F57999A, 0x3F2ADB6E), Bits(0x3F660001, 0x3F32DB6F), Bits(0x3F473334, 0x3F364925),
    Bits(0x3ED80001, 0x3CB6DB6F), Bits(0x3F380001, 0x3D8DB6DC), Bits(0x3F640001, 0x3D8DB6DC),
};

// Groups at the left: phone
constexpr HUDIdentifier LeftGroups[] =
{
    Bits(0xBDCCCCCE, 0x3D924925),
};

// Elements of the top frame at the left: button prompt box and text, crime alert lines next to the radar
constexpr HUDIdentifier LeftElements[] =
{
    { 0.0594f, 0.0848f }, { 0.0750f, 0.1069f }, { 0.3281f, 0.8347f }, { 0.3281f, 0.8792f },
};

bool GroupFrame(const float* frame)
{
    return frame[3] == 1.0f && frame[4] == 1.0f && frame[5] == 1.0f && frame[6] == 1.0f;
}

template<size_t Count> bool Contains(const HUDIdentifier (&list)[Count], float x, float y)
{
    return std::any_of(std::begin(list), std::end(list), [&](const auto& item) { return item.Matches(x, y); });
}

void __cdecl WidescreenHUD(float* frame, float* element, int alignment, float* output)
{
    if (!frame || !element || !output)
        return;

    output[5] = frame[5] * element[5];
    output[6] = frame[6] * element[6];
    output[3] = element[3] * output[5];
    output[4] = element[4] * output[6];
    output[0] = frame[0] * element[0];

    float x;
    switch (alignment)
    {
        case 0: case 3: case 6:
            x = element[1] * output[5] + frame[1];
            break;
        case 1: case 4: case 7:
            x = (frame[3] - output[3]) * 0.5f + element[1] * output[5] + frame[1];
            break;
        case 2: case 5: case 8:
            x = frame[3] + frame[1] - output[3] + element[1] * output[5];
            break;
        default:
            return;
    }
    output[1] = x + Screen.fHudOffset;

    if (GroupFrame(frame))
    {
        const bool root = frame[1] == 0.0f && frame[2] == 0.0f;
        if (!root && Contains(RightGroups, frame[1], frame[2]))
            output[1] += x >= 1.0f ? Screen.fHudOffset : Screen.fWidescreenHudOffset;
        else if ((!root && Contains(LeftGroups, frame[1], frame[2])) || (root && Contains(LeftElements, element[1], element[2])))
        {
            // Parked outside the screen (the phone between calls) stays outside it
            output[1] -= x + output[3] <= 0.0f ? Screen.fHudOffset : Screen.fWidescreenHudOffset;
        }
    }

    switch (alignment)
    {
        case 0: case 1: case 2:
            output[2] = element[2] * output[6] + frame[2];
            break;
        case 3: case 4: case 5:
            output[2] = (frame[4] - output[4]) * 0.5f + element[2] * output[6] + frame[2];
            break;
        case 6: case 7: case 8:
            output[2] = frame[4] + frame[2] - output[4] + element[2] * output[6];
            break;
    }
}

SafetyHookInline shsub_654780 = {};
void __fastcall sub_654780(void* _this, void* edx, int a2)
{
    a2 = Screen.Width43;
    shsub_654780.unsafe_fastcall(_this, edx, a2);
}

void __stdcall TransformMapToScreen(__m128* a1, const __m128* a2)
{
    // Build matrix row 0: [0.21810906, 0, 0, 0]
    __m128 row0 = _mm_set_ps(0.0f, 0.0f, 0.0f, 0.21810906f);

    // Build matrix row 1: [0, 0, 0, 0]
    __m128 row1 = _mm_setzero_ps();

    // Build matrix row 2: [0, 0, -0.21810906, 0]
    __m128 row2 = _mm_set_ps(0.0f, -0.21810906f, 0.0f, 0.0f);

    // Build translation vector: [195.0, 0, 825.0, 0] with w = 1.0
    __m128 translation = _mm_set_ps(0.0f, 825.0f, 0.0f, (195.0f + ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f));
    // Shuffle to get: [1.0, 825.0, 0, 195.0] based on the assembly pattern
    __m128 temp = _mm_set_ss(1.0f);
    __m128 row3 = _mm_shuffle_ps(translation, _mm_shuffle_ps(temp, translation, 0xE0), 0x24);

    // Load input vector
    __m128 input = *a2;

    // Extract and broadcast x, y, z components
    __m128 xxxx = _mm_shuffle_ps(input, input, 0x00); // broadcast x
    __m128 yyyy_temp = _mm_shuffle_ps(input, input, 0x55); // broadcast y
    __m128 yyyy = _mm_shuffle_ps(yyyy_temp, yyyy_temp, 0x00);
    __m128 zzzz_temp = _mm_shuffle_ps(input, input, 0xAA); // broadcast z
    __m128 zzzz = _mm_shuffle_ps(zzzz_temp, zzzz_temp, 0x00);

    // Perform matrix multiplication: result = row0*x + row1*y + row2*z + translation
    __m128 result = _mm_mul_ps(row0, xxxx);
    result = _mm_add_ps(result, _mm_mul_ps(row1, yyyy));
    result = _mm_add_ps(result, _mm_mul_ps(row2, zzzz));
    result = _mm_add_ps(result, row3);

    // Store result
    *a1 = result;
}

// HUD offsets for the resolution and the HUD aspect ratio constraint; again when the constraint is changed
void UpdateWidescreenHudOffsets()
{
    if (Screen.Height <= 0)
        return;

    float fWidescreenHudOffset = CalculateWidescreenOffset(Screen.fWidth, Screen.fHeight, 640.0f, 480.0f, 0.0f, true);
    if (Screen.fHudAspectRatioConstraint.has_value())
    {
        const float value = ClampHudAspectRatio(Screen.fHudAspectRatioConstraint.value(), Screen.fAspectRatio);
        fWidescreenHudOffset = CalculateWidescreenOffset(Screen.fHeight * value, Screen.fHeight, 640.0f, 480.0f, 0.0f, true);
    }
    Screen.fWidescreenHudOffsetRadar = fWidescreenHudOffset;
    Screen.fWidescreenHudOffset = fWidescreenHudOffset / (Screen.fHeight * (4.0f / 3.0f));
}

std::atomic<bool> hudConstraintChanged{ false };

class Hud
{
public:
    Hud()
    {
        WFP::onInitEvent() += []()
        {
            Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(FixSettings().Get("MAIN.HudAspectRatioConstraint"));
            FixSettings().OnChange("MAIN.HudAspectRatioConstraint", [] { hudConstraintChanged.store(true); });
            WFP::onGameProcessEvent() += []
            {
                if (!hudConstraintChanged.exchange(false))
                    return;
                Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(FixSettings().Get("MAIN.HudAspectRatioConstraint"));
                UpdateWidescreenHudOffsets();
            };

            auto pattern = hook::pattern("89 55 00 89 5D 04 C7 45 08 15 00 00 00 89 7D 0C"); //0x649478
            struct ResHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    Screen.Width = regs.edx;
                    Screen.Height = regs.ebx;
                    *(uint32_t*)(regs.ebp + 0x00) = Screen.Width;
                    *(uint32_t*)(regs.ebp + 0x04) = Screen.Height;

                    Screen.fWidth = static_cast<float>(Screen.Width);
                    Screen.fHeight = static_cast<float>(Screen.Height);
                    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
                    Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
                    Screen.fWidth43 = static_cast<float>(Screen.Width43);
                    Screen.fHudOffset = (1.0f / (Screen.fHeight * (4.0f / 3.0f))) * ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f);

                    UpdateWidescreenHudOffsets();
                }
            }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(6));

            pattern = hook::pattern("8B 44 24 ? F3 0F 2A C0 0F 28 C8 F3 0F 59 0D ? ? ? ? F3 0F 11 89");
            shsub_654780 = safetyhook::create_inline(pattern.get_first(), sub_654780);

            pattern = hook::pattern("8B 4C 24 ? 85 C9 56");
            //static auto shsub_654D60 = safetyhook::create_inline(pattern.get_first(), sub_654D60);
            static auto shsub_654D60 = safetyhook::create_inline(pattern.get_first(), WidescreenHUD);

            //subtitles
            pattern = hook::pattern("F3 0F 59 05 ? ? ? ? F3 0F 11 4C 24");
            static auto SubtitlesPosHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                regs.xmm1.f32[0] *= Screen.fAspectRatio / (4.0f / 3.0f);
            });

            // 3D models (armoury etc)
            pattern = hook::pattern("F3 0F 10 80 ? ? ? ? 68 15 26 DB FB 8D 4C 24 ? F3 0F 11 44 24 ? E8 ? ? ? ? 8B 4E ? F3 0F 2A C0 F3 0F 59 44 24");
            static auto Move3DPos1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                *(float*)(regs.esi + 0x54) += (Screen.fWidth - (Screen.fHeight * (4.0f / 3.0f))) / 2.0f;
            });

            // Menu map
            pattern = hook::pattern("F3 0F 58 C8 F3 0F 2C E9");
            injector::MakeNOP(pattern.get_first(), 4, true);
            static auto MapHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                regs.xmm1.f32[0] += regs.xmm0.f32[0];
                regs.xmm1.f32[0] += (Screen.fWidth - (Screen.fHeight * (4.0f / 3.0f))) / 2.0f;
            });

            // Menu blips
            //injector::MakeCALL(0x4AB6ED, TransformMapToScreen, true);
            //injector::MakeCALL(0x4AB7C8, TransformMapToScreen, true);
            pattern = hook::pattern("E8 ? ? ? ? F3 0F 10 0D ? ? ? ? F3 0F 11 4C 24 ? 8D 54 24");
            injector::MakeCALL(pattern.get_first(), TransformMapToScreen, true);
            pattern = hook::pattern("E8 ? ? ? ? F3 0F 10 05 ? ? ? ? F3 0F 11 44 24 ? 8D 54 24 ? F3 0F 10 02 0F 28 C8 0F 28 44 24 ? 0F 28 D0 0F C6 D0 AA F3 0F 5C D1");
            injector::MakeCALL(pattern.get_first(), TransformMapToScreen, true);

            // Radar
            pattern = hook::pattern("F3 0F 58 C8 66 89 44 24 ? F3 0F 2C C1 F3 0F 2A C9 F3 0F 5C C8");
            injector::MakeNOP(pattern.get_first(), 4, true);
            static auto RadarPosHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                regs.xmm1.f32[0] += -15.0f + ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f) - Screen.fWidescreenHudOffsetRadar;
            });

            pattern = hook::pattern("F3 0F 10 0D ? ? ? ? F3 0F 2A C0 F3 0F 5E C8 F3 0F 11 4F");
            injector::MakeNOP(pattern.get_first(), 8, true);
            static auto RadarDiscPosHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                regs.xmm1.f32[0] = -15.0f + ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f) - Screen.fWidescreenHudOffsetRadar;
            });

            pattern = hook::pattern("89 64 24 ? 83 EC 08");
            static auto RadarClipArea = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                regs.eax = Screen.Width;
            });
        };
    }
} Hud;
