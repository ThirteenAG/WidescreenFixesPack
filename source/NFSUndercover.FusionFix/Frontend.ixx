module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include <atomic>
#include <unordered_set>

export module Frontend;

import ComVars;

namespace FE
{
    namespace Object
    {
        void  (__cdecl* SetCenter)(void* FEObject, float X, float Y) = nullptr;
        void  (__cdecl* GetCenter)(void* FEObject, float* X, float* Y) = nullptr;
        void* (__cdecl* FindObject)(const char* pkg_name, int obj_hash) = nullptr;
    }
}

namespace cFEng
{
    std::optional<float> fHudAspectRatioConstraint;
    float fWidescreenHudOffset = 120.0f;
    float fRadarOffset = 0.0f;

    // How far the side groups move out: at every width they keep their 16:9 distance from the edges (the HUD's units
    // are 1/720 of the screen height), so on narrower boxes they move in and long texts such as the cost to state
    // stay on the screen
    float WidescreenOffset(float aspect)
    {
        constexpr float wide = 16.0f / 9.0f;
        float offset = -CalculateWidescreenOffset(480.0f * wide, 480.0f, 480.0f, 480.0f);
        return offset + 360.0f * (aspect - wide);
    }

    void updateWidescreenOffset()
    {
        if (!Height || !*Height)
            return;
        float aspect = ScreenAspectRatio();
        if (cFEng::fHudAspectRatioConstraint.has_value() && std::isfinite(*cFEng::fHudAspectRatioConstraint))
            aspect = std::min(std::clamp(*cFEng::fHudAspectRatioConstraint, 4.0f / 3.0f, 32.0f / 9.0f), aspect);
        cFEng::fWidescreenHudOffset = WidescreenOffset(aspect);
    };

    void updateRadarOffset()
    {
        fRadarOffset = fWidescreenHudOffset - 20.0f;
    };

    // The radar map's x as the HUD left it and as moved, for each radar
    struct RadarPosition
    {
        float original;
        float moved;
    };
    std::unordered_map<uintptr_t, RadarPosition> RadarPositions;
    std::unordered_map<uintptr_t, uint32_t> HudRevisions;
    uint32_t HudRevision = 0;

    float RadarX(uintptr_t radar, float x)
    {
        updateRadarOffset();
        auto it = RadarPositions.find(radar);
        float original = (it != RadarPositions.end() && it->second.moved == x) ? it->second.original : x;
        float moved = original - fRadarOffset;
        RadarPositions[radar] = { original, moved };
        return moved;
    }

    struct Position
    {
        float originalX;
    };
    std::unordered_map<void*, Position> originalCenterCache;
    std::unordered_set<std::string> HudPackages;
    std::atomic_bool HudLayoutChanged = false;
    void** mInstance = nullptr;
    SafetyHookInline shDestroyGroup;

    int __fastcall DestroyGroup(void* object, void*)
    {
        originalCenterCache.erase(object);
        return shDestroyGroup.unsafe_thiscall<int>(object);
    }

    bool ApplyHudPackage(const char* packageName)
    {
        bool found = false;
        for (bool right : { false, true })
        {
            auto object = FE::Object::FindObject(packageName, right ? 0x5D0101F1 : 0x1603009E);
            if (!object)
                continue;
            found = true;
            float x{}, y{};
            FE::Object::GetCenter(object, &x, &y);
            // Group bounds can change after SetCenter as their children animate. Keep the
            // authored anchor for this object's lifetime instead of accumulating that change.
            auto& saved = originalCenterCache.try_emplace(object, Position{ x }).first->second;
            float target = saved.originalX + (right ? fWidescreenHudOffset : -fWidescreenHudOffset);
            if (std::abs(x - target) > 0.001f)
                FE::Object::SetCenter(object, target, y);
        }
        return found;
    }

    void ServiceHud()
    {
        bool changed = HudLayoutChanged.exchange(false);
        if (changed)
            fHudAspectRatioConstraint = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
        float previous = fWidescreenHudOffset;
        updateWidescreenOffset();
        if (!changed && previous == fWidescreenHudOffset)
            return;
        ++HudRevision;
        for (auto it = HudPackages.begin(); it != HudPackages.end();)
        {
            if (ApplyHudPackage(it->c_str()))
                ++it;
            else
                it = HudPackages.erase(it);
        }
    }

    injector::hook_back<void(__fastcall*)(void*, void*, int, char const*, int)> hbQueuePackageMessage;
    void __fastcall QueuePackageMessage(void* instance, void* edx, int messageHash, const char* packageName, int argument)
    {
        hbQueuePackageMessage.fun(instance, edx, messageHash, packageName, argument);
        updateWidescreenOffset();
        if (packageName && ApplyHudPackage(packageName))
            HudPackages.emplace(packageName);
    }

}

class Frontend
{
public:
    Frontend()
    {
        WFP::onInitEvent() += []()
        {
            cFEng::fHudAspectRatioConstraint = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));

            // Settings callbacks defer native layout work to the game thread.
            FixSettings().OnChange("MAIN.HudAspectRatioConstraint", []()
            {
                cFEng::HudLayoutChanged = true;
            });
            WFP::onGameProcessEvent() += cFEng::ServiceHud;

            // Some protected builds relocate the destructor's first call after this shared prologue.
            auto destroyGroup = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 51 56 A1 ? ? ? ? 33 C4 50 8D 44 24 0C 64 A3 00 00 00 00 8B F1 89 74 24 08 8D 4E 5C C7 44 24 14 00 00 00 00 C7 01 ? ? ? ?");
            cFEng::shDestroyGroup = safetyhook::create_inline(destroyGroup.get_first(), cFEng::DestroyGroup);

            auto pattern = hook::pattern("E8 ? ? ? ? 6A ? 57 E8 ? ? ? ? 83 C4 ? EB ? 8B 7C 24");
            FE::Object::SetCenter = (decltype(FE::Object::SetCenter))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? 83 C4 ? ? ? ? ? ? ? ? ? ? ? ? 57 E8 ? ? ? ? ? ? ? 8B 7E");
            FE::Object::GetCenter = (decltype(FE::Object::GetCenter))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? 6A ? EB ? 8B 7C 24");
            FE::Object::FindObject = (decltype(FE::Object::FindObject))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = find_pattern("E8 ? ? ? ? 85 F6 74 ? F3 0F 10 86 ? ? ? ? 0F 5A C0 F2 0F 5C 05", "E8 ? ? ? ? 85 F6 74 ? F3 0F 10 86 ? ? ? ? F3 0F 5C 05");
            cFEng::mInstance = *pattern.get_first<void**>(-11);
            cFEng::hbQueuePackageMessage.fun = injector::MakeCALL(pattern.get_first(), cFEng::QueuePackageMessage, true).get();

            pattern = find_pattern("E8 ? ? ? ? 85 F6 74 ? F3 0F 10 86 ? ? ? ? 0F 5A C0 F2 0F 58 05", "E8 ? ? ? ? 85 F6 74 ? F3 0F 10 86 ? ? ? ? F3 0F 58 05");
            injector::MakeCALL(pattern.get_first(), cFEng::QueuePackageMessage, true);

            pattern = find_pattern("74 ? 68 ? ? ? ? E8 ? ? ? ? 85 F6");
            injector::MakeNOP(pattern.get_first(), 2, true);

            // FEngHud::SetWideScreenMode moves the radar's map once per HUD (x at +0xBC, esi): by the offset that applies
            // now, from where the HUD put it, also when it runs again for a changed HUD box
            pattern = find_pattern("F2 0F 5C 05 ? ? ? ? EB ? 68");
            if (!pattern.empty())
            {
                injector::MakeNOP(pattern.get_first(0), 8);
                static auto SetWideScreenModeHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    regs.xmm0.f64[0] = cFEng::RadarX(regs.esi, static_cast<float>(regs.xmm0.f64[0]));
                });
            }
            else
            {
                pattern = find_pattern("F3 0F 5C 05 ? ? ? ? EB ? 68");
                injector::MakeNOP(pattern.get_first(0), 8);
                static auto SetWideScreenModeHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    regs.xmm0.f32[0] = cFEng::RadarX(regs.esi, regs.xmm0.f32[0]);
                });
            }

            // FEngHud's update calls SetWideScreenMode while its flag (byte at +0x371) is clear: cleared again once for
            // each HUD after the HUD box changed
            pattern = GetPattern("80 BE 71 03 00 00 00 75 ? 8B CE E8");
            static auto WideScreenModeCheckHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto& revision = cFEng::HudRevisions[regs.esi];
                if (revision != cFEng::HudRevision)
                {
                    revision = cFEng::HudRevision;
                    *reinterpret_cast<uint8_t*>(regs.esi + 0x371) = 0;
                }
            });

            //pattern = find_pattern("F2 0F 58 05 ? ? ? ? 8B 86");
            //if (!pattern.empty())
            //{
            //    injector::MakeNOP(pattern.get_first(0), 8);
            //    static auto SetWideScreenModeHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            //    {
            //        cFEng::updateRadarOffset();
            //        regs.xmm0.f64[0] += cFEng::fRadarOffset;
            //    });
            //}
            //else
            //{
            //    pattern = find_pattern("F3 0F 58 05 ? ? ? ? 8B 86");
            //    injector::MakeNOP(pattern.get_first(0), 8);
            //    static auto SetWideScreenModeHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            //    {
            //        cFEng::updateRadarOffset();
            //        regs.xmm0.f32[0] += cFEng::fRadarOffset;
            //    });
            //}

            //static auto DrawPillarboxes = []()
            //{
            //    float target_aspect = 2.25f;
            //    float video_width = ResY * target_aspect;
            //    float pillar = (ResX - video_width) / 2.0f;
            //    if (pillar > 0.0f)
            //    {
            //        D3DRECT rects[2];
            //        rects[0].x1 = 0;
            //        rects[0].y1 = 0;
            //        rects[0].x2 = static_cast<LONG>(pillar);
            //        rects[0].y2 = static_cast<LONG>(ResY);
            //        rects[1].x1 = static_cast<LONG>(ResX) - static_cast<LONG>(pillar);
            //        rects[1].y1 = 0;
            //        rects[1].x2 = static_cast<LONG>(ResX);
            //        rects[1].y2 = static_cast<LONG>(ResY);
            //        (*Direct3DDevice)->Clear(2, rects, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
            //    }
            //};

            //WFP::onEndScene() += []()
            //{
            //    if (TheGameFlowManager->CurrentGameFlowState == GAMEFLOW_STATE_IN_FRONTEND)
            //    {
            //        DrawPillarboxes();
            //    }
            //};
        };
    }
} Frontend;
