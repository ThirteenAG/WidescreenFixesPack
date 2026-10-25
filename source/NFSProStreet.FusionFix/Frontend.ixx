module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include <atomic>
#include <unordered_set>
#include "Hud.hpp"

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

    void updateWidescreenOffset()
    {
        auto Width = ResX;
        auto Height = ResY;

        cFEng::fWidescreenHudOffset = -CalculateWidescreenOffset(static_cast<float>(Width), static_cast<float>(Height), 640.0f, 480.0f);
        if (cFEng::fHudAspectRatioConstraint.has_value())
        {
            float value = cFEng::fHudAspectRatioConstraint.value();
            if (value < 0.0f || value > (32.0f / 9.0f))
                cFEng::fWidescreenHudOffset = value;
            else
            {
                value = ClampHudAspectRatio(value, static_cast<float>(Width) / static_cast<float>(Height));
                cFEng::fWidescreenHudOffset = -CalculateWidescreenOffset(static_cast<float>(Height) * value, static_cast<float>(Height), 640.0f, 480.0f);
            }
        }
    };

    NFSPSHud::Positions HudPositions;
    std::unordered_set<std::string> HudPackages;
    std::atomic_bool HudAspectRatioPending = false;
    static void* s_currentLeftGroup = nullptr;
    static void* s_currentRightGroup = nullptr;
    SafetyHookInline shDestroyGroup;

    int __fastcall DestroyGroup(void* object, void*)
    {
        HudPositions.Retire(object);
        if (s_currentLeftGroup == object)
            s_currentLeftGroup = nullptr;
        if (s_currentRightGroup == object)
            s_currentRightGroup = nullptr;
        return shDestroyGroup.unsafe_thiscall<int>(object);
    }

    bool ApplyHudPackage(const char* packageName)
    {
        if (!packageName || !*packageName || !FE::Object::FindObject || !shDestroyGroup)
            return false;
        bool found = false;
        for (bool right : { false, true })
        {
            auto object = static_cast<FEObject*>(FE::Object::FindObject(packageName, right ? 0x5D0101F1 : 0x1603009E));
            if (!object || object->Type != FE_Group || !object->pData)
                continue;
            found = true;
            auto& current = right ? s_currentRightGroup : s_currentLeftGroup;
            current = object;
            HudPositions.Apply(object, { object->pData, object->GUID }, right, fWidescreenHudOffset,
                [](void* group, float& x, float& y)
                {
                    FE::Object::GetCenter(group, &x, &y);
                    return true;
                }, [](void* group, float x, float y)
                {
                    FE::Object::SetCenter(group, x, y);
                });
        }
        return found;
    }

    void ServiceHud()
    {
        bool changed = HudAspectRatioPending.exchange(false);
        if (changed)
            fHudAspectRatioConstraint = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
        auto previous = fWidescreenHudOffset;
        updateWidescreenOffset();
        if (!changed && previous == fWidescreenHudOffset)
            return;
        for (auto package = HudPackages.begin(); package != HudPackages.end();)
        {
            if (ApplyHudPackage(package->c_str()))
                ++package;
            else
                package = HudPackages.erase(package);
        }
    }

    SafetyHookInline shHudFrame;
    void __cdecl HudFrame()
    {
        shHudFrame.unsafe_ccall();
        ServiceHud();
    }

    injector::hook_back<void(__fastcall*)(void*, void*, int, char const*, int)> hbQueuePackageMessage;
    void __fastcall QueuePackageMessage(void* instance, void* edx, int messageHash, const char* packageName, int argument)
    {
        hbQueuePackageMessage.fun(instance, edx, messageHash, packageName, argument);
        updateWidescreenOffset();
        if (ApplyHudPackage(packageName))
        {
            try
            {
                HudPackages.emplace(packageName);
            }
            catch (const std::bad_alloc&)
            {
            }
        }
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

            // A settings callback can run on the live-link thread; native HUD work stays on the game thread.
            FixSettings().OnChange("MAIN.HudAspectRatioConstraint", []()
            {
                cFEng::HudAspectRatioPending = true;
            });

            auto destroyGroup = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 51 56 A1 ? ? ? ? 33 C4 50 8D 44 24 0C 64 A3 00 00 00 00 8B F1 89 74 24 08 8D 4E 5C C7 44 24 14 00 00 00 00 C7 01 ? ? ? ? E8 ? ? ? ? 8B CE");
            auto hudFrame = hook::pattern("83 EC 0C 56 8B 35 ? ? ? ? 8D 44 24 08 50 FF 15 ? ? ? ? 8B 44 24 08 8B 54 24 0C 8B 0D ? ? ? ? E8 ? ? ? ? 50 56 E8 ? ? ? ? D9 1D ? ? ? ? 8B 0D ? ? ? ? 83 C4 08 83 F9 01");
            auto setCenter = hook::pattern("E8 ? ? ? ? 83 C4 ? 5E C2 ? ? 8B 51");
            auto getCenter = hook::pattern("E8 ? ? ? ? ? ? ? ? 83 C4 ? ? ? ? ? ? ? ? ? ? ? ? 56 E8 ? ? ? ? 83 C4 ? 5E C2");
            auto findObject = hook::pattern("E8 ? ? ? ? 83 C4 ? 50 57 68 ? ? ? ? 8B CD E8 ? ? ? ? ? ? 8B 50");
            auto queue = hook::pattern("E8 ? ? ? ? 5E 59 C3 8B 96");
            auto hudMessage = hook::pattern("52 68 ? ? ? ? E8 ? ? ? ? 5E 59");
            if (destroyGroup.size() != 1 || hudFrame.size() != 1 || setCenter.size() != 1 || getCenter.size() != 1 ||
                findObject.size() != 1 || queue.size() != 1 || hudMessage.size() != 1)
                return;
            FE::Object::SetCenter = reinterpret_cast<decltype(FE::Object::SetCenter)>(injector::GetBranchDestination(setCenter.get_first()).as_int());
            FE::Object::GetCenter = reinterpret_cast<decltype(FE::Object::GetCenter)>(injector::GetBranchDestination(getCenter.get_first()).as_int());
            FE::Object::FindObject = reinterpret_cast<decltype(FE::Object::FindObject)>(injector::GetBranchDestination(findObject.get_first()).as_int());
            cFEng::shDestroyGroup = safetyhook::create_inline(destroyGroup.get_first(), cFEng::DestroyGroup);
            if (!cFEng::shDestroyGroup)
                return;
            cFEng::shHudFrame = safetyhook::create_inline(hudFrame.get_first(), cFEng::HudFrame);
            if (!cFEng::shHudFrame)
            {
                cFEng::shDestroyGroup.reset();
                return;
            }
            cFEng::mInstance = *queue.get_first<void**>(-9);
            cFEng::hbQueuePackageMessage.fun = injector::MakeCALL(queue.get_first(), cFEng::QueuePackageMessage, true).get();
            injector::MakeCALL(hudMessage.get_first(6), cFEng::QueuePackageMessage, true);
        };
    }
} Frontend;