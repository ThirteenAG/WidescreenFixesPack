module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <optional>

export module Rendering;

import ComVars;
import Video;
import Resolution;


constexpr float baseAspect = 16.0f / 9.0f;
std::atomic<float> targetHudAspect{ baseAspect };
std::atomic<bool> hudSettingChanged{ false };
thread_local bool drawingHud = false;
SafetyHookInline hudHook, hudTextHook;
SafetyHookMid viewFovHook;
uintptr_t footFovOutput = 0, vehicleFovOutput = 0;
std::vector<uintptr_t> drivingFovOutputs;
std::optional<float> GameplayFovFactor(void* camera, float factor)
{
    auto* state = static_cast<const uint8_t*>(camera);
    if (!state || *reinterpret_cast<void* const*>(state + 200) || *reinterpret_cast<const uint32_t*>(state + 236)) return std::nullopt;
    const auto type = *reinterpret_cast<const uint32_t*>(state + 220);
    if (type != 74 && type != 77 && type != 79) return std::nullopt;
    const float viewScale = *reinterpret_cast<const float*>(state + 304);
    if (!std::isfinite(viewScale) || viewScale < 0.999f) return std::nullopt;
    auto* controller = *reinterpret_cast<uint8_t* const*>(state + 196);
    if (!controller) return std::nullopt;
    const auto output = (*reinterpret_cast<uintptr_t**>(controller))[3];
    factor = std::isfinite(factor) ? std::clamp(factor, 0.5f, 2.0f) : 1.0f;
    if (std::find(drivingFovOutputs.begin(), drivingFovOutputs.end(), output) != drivingFovOutputs.end()) return factor;
    size_t scaleOffset = output == footFovOutput ? 320 : output == vehicleFovOutput ? 304 : 0;
    if (!scaleOffset) return std::nullopt;
    const float scale = *reinterpret_cast<const float*>(controller + scaleOffset);
    if (!std::isfinite(scale)) return std::nullopt;
    // The native follow camera eases its aim zoom between 1.0 and 0.66.
    const float weight = std::clamp((scale - 0.66f) / 0.34f, 0.0f, 1.0f);
    factor = std::isfinite(factor) ? std::clamp(factor, 0.5f, 2.0f) : 1.0f;
    return 1.0f + (factor - 1.0f) * weight;
}
float BoundViewFov(float value)
{
    return std::isfinite(value) ? std::clamp(value, 0.01f, 2.617993878f) : 1.308f;
}
float HorGameplayFov(float value, float aspect)
{
    if (!std::isfinite(value) || !std::isfinite(aspect) || aspect <= 0) return 1.308f;
    value = std::clamp(value, 0.01f, 3.13f);
    return 2.0f * std::atan(std::tan(value * 0.5f) * (aspect / baseAspect));
}
thread_local bool drawingHudText = false;
thread_local int hudTextSide = 0;
int __fastcall DrawHudText(void* font, void*, int renderer)
{
    struct Scope
    {
        bool previousText = drawingHudText;
        int previousSide = hudTextSide;
        ~Scope() { drawingHudText = previousText; hudTextSide = previousSide; }
    } scope;
    drawingHudText = true;
    hudTextSide = 0;
    if (drawingHud)
    {
        auto* bytes = static_cast<const uint8_t*>(font);
        const int alignment = *reinterpret_cast<const int*>(bytes + 104);
        const float extent = *reinterpret_cast<const float*>(bytes + 128);
        const float width = *reinterpret_cast<const float*>(bytes + 8408);
        const float position = static_cast<float>(*reinterpret_cast<const int*>(bytes + 88));
        const float bound = static_cast<float>(*reinterpret_cast<const int*>(bytes + 96));
        if (alignment != 2 && std::isfinite(extent) && extent <= 0.5f && std::isfinite(width) && width > 0)
        {
            const float anchor = (position + (alignment == 1 ? bound : 0)) / width;
            hudTextSide = anchor < 0.35f ? -1 : anchor > 0.65f ? 1 : 0;
        }
    }
    return hudTextHook.thiscall<int>(font, renderer);
}
void UpdateRadar(void* manager);
void __fastcall DrawHud(void* self, void*, void* renderer)
{
    struct Scope
    {
        bool previous = drawingHud;
        Scope() { drawingHud = true; }
        ~Scope() { drawingHud = previous; }
    } scope;
    UpdateRadar(self);
    hudHook.thiscall<void>(self, renderer);
}

ProtectedGameRef<float> fAspectRatio;

float RadarLeft(float position, float aspect)
{
    const float extent = std::min(aspect, baseAspect);
    return ((position + 0.00078125f) * extent + (aspect + targetHudAspect - 2.0f * extent) * 0.5f) / aspect;
}
void UpdateRadar(void* manager)
{
    const auto map = *reinterpret_cast<uintptr_t*>(static_cast<uint8_t*>(manager) + 1108);
    if (!map || *reinterpret_cast<const uint8_t*>(map + 1909)) return;
    const auto geometry = *reinterpret_cast<uintptr_t*>(map + 1780);
    const float aspect = fAspectRatio;
    if (!geometry || !std::isfinite(aspect) || aspect <= 0) return;
    const float position = *reinterpret_cast<const float*>(geometry + 16);
    if (std::isfinite(position)) *reinterpret_cast<float*>(map + 1860) = RadarLeft(position, aspect);
}


export void InitRenderingRequiredRefs()
{
    fAspectRatio.SetAddress(*find_pattern("D9 05 ? ? ? ? 53 8B 5D 08 8B 03 57 51").get_first<float*>(2));
    fFieldOfView.SetAddress(*find_pattern("D9 05 ? ? ? ? 8B 03 51 8B CB").get_first<float*>(2));
}


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

                if (!DPLVideo::drawing && (!isFullScreenQuad || pCurrentMovieStatus == 1))
                {
                    *(float*)(v8a - 4) *= scaleX;
                    *(float*)(v8a + 0) *= scaleY;
                    if (drawingHud && !isFullScreenQuad)
                    {
                        const float left = floats[0].f1, right = floats[2].f1;
                        const bool leftSide = drawingHudText ? hudTextSide < 0 : left < 0.35f && right < 0.5f;
                        const bool rightSide = drawingHudText ? hudTextSide > 0 : left > 0.5f && right > 0.65f;
                        if (leftSide || rightSide)
                        {
                            const float extent = std::min(baseAspect, static_cast<float>(fAspectRatio));
                            const float offset = (targetHudAspect - extent) / fAspectRatio;
                            *(float*)(v8a - 4) += leftSide ? -offset : offset;
                        }
                    }
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
            DPLVideo::Install();
            auto viewFov = hook::pattern("D9 84 24 C0 00 00 00 8B 06 51 8B CE D9 1C 24 FF 90 8C 00 00 00 8B 06 8D 8C");
            auto footOutput = hook::pattern("55 8B EC 83 E4 F0 83 EC 48 8B 41 04 85 C0 56 57 74 05 83 C0 08 EB 02 33 C0 8B 7D 08 89 47 5C");
            auto vehicleOutput = hook::pattern("55 8B EC 83 E4 F0 81 EC 84 00 00 00 53 56 8B F1 8B 46 04 85 C0 57 74 05 83 C0 08 EB 02 33 C0");
            auto drivingOutput = hook::pattern("56 57 8B 7C 24 0C 8B F1 57 8D 8E C0 01 00 00 E8 ? ? ? ? 8D 4E 60 E8 ? ? ? ? D8 47 40 D9 5F 40 5F 5E C2 04 00");
            if (drivingOutput.empty())
            {
                drivingOutput.get_first();
                std::abort();
            }
            for (size_t i = 0; i < drivingOutput.size(); ++i)
                drivingFovOutputs.push_back(reinterpret_cast<uintptr_t>(drivingOutput.get(i).get<void>()));
            footFovOutput = reinterpret_cast<uintptr_t>(footOutput.get_first());
            vehicleFovOutput = reinterpret_cast<uintptr_t>(vehicleOutput.get_first());
            viewFovHook = safetyhook::create_mid(viewFov.get_first(15), [](SafetyHookContext& regs)
            {
                auto* value = reinterpret_cast<float*>(regs.esp);
                if (const auto factor = GameplayFovFactor(reinterpret_cast<void*>(regs.ebx), FixSettings().Float("MAIN.FOVFactor")))
                    *value = BoundViewFov(HorGameplayFov(*value, fAspectRatio) * *factor);
            });
            auto& settings = FixSettings();

            auto updateHud = []()
            {
                const float aspect = fAspectRatio;
                const auto constraint = ParseWidescreenHudOffset(FixSettings().Get("MAIN.HudAspectRatioConstraint"));
                float target = aspect;
                if (constraint && std::isfinite(*constraint) && *constraint >= 0)
                    target = ClampHudAspectRatio(*constraint, aspect);
                targetHudAspect.store(target);
            };
            settings.OnChange("MAIN.HudAspectRatioConstraint", []() { hudSettingChanged.store(true); });
            WFP::onGameProcessEvent() += [updateHud]()
            {
                if (hudSettingChanged.exchange(false))
                    updateHud();
            };
            auto hud = hook::pattern("55 8B EC 51 51 A1 ? ? ? ? 8A 80 D9 03 00 00 84 C0 53 56 57 8B F1 0F 85");
            auto text = hook::pattern("53 56 8B F1 8B 46 68 33 DB 48 57 0F 84 ? ? ? ? 48 74 46 39 9E 24 22 00 00");
            hudTextHook = safetyhook::create_inline(text.get_first(), DrawHudText);
            hudHook = safetyhook::create_inline(hud.get_first(), DrawHud);

            onResChange() += [updateHud](int Width, int Height)
            {
                fAspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
                updateHud();
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
                const float aspect = fAspectRatio;
                const float offset = RadarLeft(*(float*)(regs.eax + 0x10), aspect);

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

            auto radarPositions = hook::pattern("F3 0F 11 ? 44 07 00 00");
            auto radarScales = hook::pattern("F3 0F 11 ? 4C 07 00 00");
            if (radarPositions.empty())
            {
                radarPositions.get_first();
                std::abort();
            }
            if (radarScales.empty())
            {
                radarScales.get_first();
                std::abort();
            }
            if (radarPositions.size() != 4 || radarScales.size() != 3)
                std::abort();
            pattern = radarPositions;
            injector::MakeInline<RadarPosHookEBX>(pattern.get(0).get<uintptr_t>(0), pattern.get(0).get<uintptr_t>(8));
            injector::MakeInline<RadarPosHookESI>(pattern.get(2).get<uintptr_t>(0), pattern.get(2).get<uintptr_t>(8));
            injector::MakeInline<RadarPosHookEBX>(pattern.get(3).get<uintptr_t>(0), pattern.get(3).get<uintptr_t>(8));

            pattern = radarScales;
            injector::MakeInline<RadarScaleHookESI>(pattern.get(1).get<uintptr_t>(0), pattern.get(1).get<uintptr_t>(8));
            injector::MakeInline<RadarScaleHookEBX>(pattern.get(2).get<uintptr_t>(0), pattern.get(2).get<uintptr_t>(8));

            pattern = hook::pattern("89 8B 4C 07 00 00 D9 40 1C 51").count(1);
            injector::MakeInline<RadarScaleHookEBX>(pattern.get(0).get<uintptr_t>(0), pattern.get(0).get<uintptr_t>(6));
        };
    }
} Rendering;