module;

#include <stdafx.h>

export module Rendering;

import ComVars;
import Resolution;

constexpr float baseAspect = 16.0f / 9.0f;

ProtectedGameRef<float> fAspectRatio([]() -> float*
{
    auto pattern = find_pattern("D9 05 ? ? ? ? 53 8B 5D 08 8B 03 57 51");
    if (!pattern.empty())
        return *pattern.get_first<float*>(2);
    return nullptr;
});

ProtectedGameRef<float> fFieldOfView([]() -> float*
{
    auto pattern = find_pattern("D9 05 ? ? ? ? 8B 03 51 8B CB");
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

void __fastcall Process2DQuads(uintptr_t _this, uint32_t edx, uintptr_t a2, uintptr_t a3)
{
    size_t j = 0;
    float v4 = 1.0f / BackbufferWidth;
    float v5 = 1.0f / BackbufferHeight;
    uintptr_t v6 = *(uintptr_t*)(a2 + 8);
    if (*(uint32_t*)a2)
    {
        float scaleX = (baseAspect / fAspectRatio < 1.0f) ? (baseAspect / fAspectRatio) : 1.0f;
        float scaleY = (fAspectRatio / baseAspect < 1.0f) ? (fAspectRatio / baseAspect) : 1.0f;

        do
        {
            *(float*)(a3 + 00) = *(float*)(v6 + 16);
            *(float*)(a3 + 04) = *(float*)(v6 + 20) + *(float*)(v6 + 28);
            *(float*)(a3 + 16) = *(float*)(v6 + 32);
            *(float*)(a3 + 20) = *(float*)(v6 + 44);
            *(float*)(a3 + 24) = *(float*)(v6 + 16);
            *(float*)(a3 + 28) = *(float*)(v6 + 20);
            *(float*)(a3 + 40) = *(float*)(v6 + 32);
            *(float*)(a3 + 44) = *(float*)(v6 + 36);
            *(float*)(a3 + 48) = *(float*)(v6 + 24) + *(float*)(v6 + 16);
            *(float*)(a3 + 52) = *(float*)(v6 + 20) + *(float*)(v6 + 28);
            *(float*)(a3 + 64) = *(float*)(v6 + 40);
            *(float*)(a3 + 68) = *(float*)(v6 + 44);
            *(float*)(a3 + 72) = *(float*)(v6 + 24) + *(float*)(v6 + 16);
            *(float*)(a3 + 76) = *(float*)(v6 + 20);
            *(float*)(a3 + 88) = *(float*)(v6 + 40);
            *(float*)(a3 + 92) = *(float*)(v6 + 36);

            uintptr_t v8 = a3 + 4;

            struct flt
            {
                float f1;
                float f2;
            };

            flt floats[4];

            for (size_t i = 0; i < 4; i++)
            {
                floats[i].f1 = *(float*)(v8 - 4);
                floats[i].f2 = *(float*)(v8 - 0);
                *(float*)(v8 - 4) = (((*(float*)(v8 - 4) * 2.0f) - 1.0f) - v4);
                *(float*)(v8 + 0) = ((1.0f - (*(float*)v8 * 2.0f)) - (0.0f - v5));
                uint32_t v9 = (uint32_t)(*(float*)(v6 + 8) * 255.0f);
                uint32_t v10 = (uint32_t)(*(float*)(v6 + 4) * 255.0f) & 0xFF | ((((uint32_t)(*(float*)(v6 + 12) * 255.0f) << 8) | (uint32_t)(*(float*)v6 * 255.0) & 0xFF) << 8);
                *(uint32_t*)(v8 + 8) = (uint8_t)v9 | (v10 << 8);
                *(uint32_t*)(v8 + 4) = 0;
                v8 += 24;
            }

            uintptr_t v8a = a3 + 4;
            for (size_t i = 0; i < 4; i++)
            {
                bool isFullScreenQuad = (floats[0].f1 == 0.0f && floats[0].f2 == 1.0f) &&
                    (floats[1].f1 == 0.0f && floats[1].f2 == 0.0f) &&
                    (floats[2].f1 == 1.0f && floats[2].f2 == 1.0f) &&
                    (floats[3].f1 == 1.0f && floats[3].f2 == 0.0f);

                if (!isFullScreenQuad || pCurrentMovieStatus == 1)
                {
                    *(float*)(v8a - 4) *= scaleX;
                    *(float*)(v8a + 0) *= scaleY;
                }
                v8a += 24;
            }

            ++j;
            a3 += 96;
            v6 += 48;
        } while (j < *(uint32_t*)a2);
    }
}

float __fastcall sub_473431(void* _this, void* edx)
{
    return 0.0f;
}

float __fastcall sub_47345A(void* _this, void* edx)
{
    return 1.0f;
}

float __stdcall sub_4BAA31(float a1)
{
    float scaleY = (fAspectRatio / baseAspect < 1.0f) ? (fAspectRatio / baseAspect) : 1.0f;
    if (a1 <= 0.5f)
        return 0.5f - (0.5f - a1) * scaleY;
    return (a1 - 0.5f) * scaleY + 0.5f;
}

float __stdcall sub_4BAA87(float a1)
{
    float scaleY = (fAspectRatio / baseAspect < 1.0f) ? (fAspectRatio / baseAspect) : 1.0f;
    return a1 * scaleY;
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

            onResChange() += [](int Width, int Height)
            {
                fAspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
                fFieldOfView = 2.0f * std::atan(std::tan(1.308f * 0.5f) * (fAspectRatio / baseAspect));
                if (fCustomFieldOfView)
                    fFieldOfView *= fCustomFieldOfView;
            };

            auto pattern = hook::pattern("8B 44 24 04 89 41 10 B0 01 C2 04 00"); //0x57B954
            struct MenuAspectRatioSwitchHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint32_t*)(regs.ecx + 0x10) = WIDE16BY9;
                }
            }; injector::MakeInline<MenuAspectRatioSwitchHook>(pattern.get_first(0), pattern.get_first(7));

            // 2D
            pattern = hook::pattern("E8 ? ? ? ? 8B 45 08 8B 4D 0C 89 45 E8 8B"); //0x5F75AD
            injector::MakeCALL(pattern.get_first(0), Process2DQuads, true);

            // Radar
            pattern = hook::pattern("E8 ? ? ? ? 51 51 ? ? ? ? 8B CF ? ? ? ? ? FF 93 ? ? ? ? 8B 0D");
            static auto shsub_473431 = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), sub_473431);

            pattern = hook::pattern("E8 ? ? ? ? 51 51 ? ? ? ? 8B CB ? ? ? ? ? FF 57");
            static auto shsub_47345A = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), sub_47345A);

            pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? ? ? 8B 83 ? ? ? ? 51 ? ? ? ? ? ? F3 0F 10 40");
            static auto shsub_4BAA31 = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), sub_4BAA31);

            pattern = hook::pattern("E8 ? ? ? ? F3 0F 10 05 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 51");
            static auto shsub_4BAA87 = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), sub_4BAA87);

            static auto GetRadarPosition = [](injector::reg_pack& regs) -> float
            {
                float refAspect = (fAspectRatio < baseAspect) ? fAspectRatio : baseAspect;

                float fVar1 = 480.0f * refAspect;
                float fVar2 = 480.0f * fAspectRatio;
                float fOriginalPos = *(float*)(regs.eax + 0x10) + 0.00078125f;
                fOriginalPos *= fVar1;
                float offset = fOriginalPos - (fVar1 / 2.0f);
                offset += (fVar2 / 2.0f);
                offset /= fVar2;

                return offset;
            };

            static auto GetRadarScale = [](injector::reg_pack& regs) -> float
            {
                float base = (*(float*)(regs.eax + 0x18) - (1.0f / 640.0f / 2.0f));
                float radarFactor = (fAspectRatio > baseAspect) ? (baseAspect / fAspectRatio) : 1.0f;
                return base * radarFactor;
            };

            struct RadarPosHookEBX { void operator()(injector::reg_pack& regs) { *(float*)(regs.ebx + 0x744) = GetRadarPosition(regs); } };
            struct RadarPosHookESI { void operator()(injector::reg_pack& regs) { *(float*)(regs.esi + 0x744) = GetRadarPosition(regs); } };

            struct RadarScaleHookEBX { void operator()(injector::reg_pack& regs) { *(float*)(regs.ebx + 0x74C) = GetRadarScale(regs); } };
            struct RadarScaleHookESI { void operator()(injector::reg_pack& regs) { *(float*)(regs.esi + 0x74C) = GetRadarScale(regs); } };

            pattern = hook::pattern("F3 0F 11 ? 44 07 00 00").count(4);
            injector::MakeInline<RadarPosHookEBX>(pattern.get(0).get<uintptr_t>(0), pattern.get(0).get<uintptr_t>(8));
            injector::MakeInline<RadarPosHookESI>(pattern.get(2).get<uintptr_t>(0), pattern.get(2).get<uintptr_t>(8));
            injector::MakeInline<RadarPosHookEBX>(pattern.get(3).get<uintptr_t>(0), pattern.get(3).get<uintptr_t>(8));

            pattern = hook::pattern("F3 0F 11 ? 4C 07 00 00").count(3);
            injector::MakeInline<RadarScaleHookESI>(pattern.get(1).get<uintptr_t>(0), pattern.get(1).get<uintptr_t>(8));
            injector::MakeInline<RadarScaleHookEBX>(pattern.get(2).get<uintptr_t>(0), pattern.get(2).get<uintptr_t>(8));

            pattern = hook::pattern("89 8B 4C 07 00 00 D9 40 1C 51").count(1);
            injector::MakeInline<RadarScaleHookEBX>(pattern.get(0).get<uintptr_t>(0), pattern.get(0).get<uintptr_t>(6));
        };
    }
} Rendering;