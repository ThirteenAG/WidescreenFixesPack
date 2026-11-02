module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include <atomic>
#include <unordered_set>

export module Frontend;

import ComVars;
import Hud;


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
        auto Height = static_cast<float>(ResY);
        auto Width = Height * ScreenAspectRatio();

        cFEng::fWidescreenHudOffset = -CalculateWidescreenOffset(static_cast<float>(Width), static_cast<float>(Height), 640.0f, 480.0f);
        if (cFEng::fHudAspectRatioConstraint.has_value() && std::isfinite(*cFEng::fHudAspectRatioConstraint))
        {
            float value = cFEng::fHudAspectRatioConstraint.value();
            value = std::min(std::clamp(value, 4.0f / 3.0f, 32.0f / 9.0f), static_cast<float>(Width) / static_cast<float>(Height));
            cFEng::fWidescreenHudOffset = -CalculateWidescreenOffset(static_cast<float>(Height) * value, static_cast<float>(Height), 640.0f, 480.0f);
        }
    };

    NFSPSHud::Positions HudPositions;
    std::unordered_set<std::string> HudPackages;
    std::atomic_bool HudAspectRatioPending = false;
    static void* s_currentLeftGroup = nullptr;
    static void* s_currentRightGroup = nullptr;
    SafetyHookInline shDestroyGroup;

    void UpdateDialogBackdrop()
    {
        if (!FE::Object::FindObject || !mInstance || !*mInstance || !*static_cast<void**>(*mInstance))
            return;
        // The dialog's dimmed backdrop is made for 16:9: three centered layers 1200 wide and a darker one (x 78, 887
        // wide) that runs off the right side. On wider screens they grow to the screen's edges, the darker one keeps
        // its left edge.
        float halfScreen = 240.0f * ScreenAspectRatio() + 40.0f;
        float width = std::max(1200.0f, 2.0f * halfScreen);
        constexpr float DarkX = 78.0f, DarkWidth = 887.0f, DarkLeft = DarkX - DarkWidth / 2.0f;
        float darkWidth = std::max(DarkWidth, halfScreen - DarkLeft);
        struct Layer { uint32_t hash; float width; std::optional<float> x; };
        for (auto& layer : { Layer{ 0xC6AFDD7Eu, width }, Layer{ 0xE03FF6FBu, width }, Layer{ 0xF019AE2Bu, width },
            Layer{ 0xF019AE2Cu, darkWidth, DarkLeft + darkWidth / 2.0f } })
        {
            auto object = static_cast<FEObject*>(FE::Object::FindObject("FeDialog.fng", layer.hash));
            if (!object || !object->pData)
                continue;
            auto& x = *reinterpret_cast<float*>(object->pData + 28);    // position
            auto& size = *reinterpret_cast<float*>(object->pData + 56);
            if (size != layer.width || (layer.x && x != *layer.x))
            {
                size = layer.width;
                if (layer.x)
                    x = *layer.x;
                object->Flags |= 0x400000;
            }
        }
    }

    // Options pages park their value list (x 1145 in the page) and slider row (x 1307) right of a 16:9 screen until
    // they're used; on screens wider than about 3:1 they showed at the right edge. They're parked further out there
    // (the game puts them at the row when opened and back at the parked spot when closed).
    void UpdateParkedOptions()
    {
        if (!FE::Object::FindObject || !mInstance || !*mInstance || !*static_cast<void**>(*mInstance))
            return;
        float extra = std::max(0.0f, 240.0f * ScreenAspectRatio() - 708.0f);
        struct Parked { uint32_t hash; float x; };
        constexpr float None = std::numeric_limits<float>::quiet_NaN();
        static float written[2][2] = { { None, None }, { None, None } };
        constexpr const char* Packages[] = { "FeMainMenu.fng", "FePauseMain.fng" };
        constexpr Parked Groups[] = { { 0xDF4DFD48u, 1145.0f }, { 0x4322CEB6u, 1307.0f } };
        for (size_t p = 0; p < std::size(Packages); ++p)
        {
            for (size_t g = 0; g < std::size(Groups); ++g)
            {
                auto object = static_cast<FEObject*>(FE::Object::FindObject(Packages[p], Groups[g].hash));
                if (!object || !object->pData)
                    continue;
                auto& x = *reinterpret_cast<float*>(object->pData + 28);    // position
                float parked = Groups[g].x + extra;
                if (x != parked && (x == Groups[g].x || x == written[p][g]))
                {
                    x = parked;
                    written[p][g] = parked;
                    object->Flags |= 0x400000;
                }
            }
        }
    }

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
        UpdateDialogBackdrop();
        UpdateParkedOptions();
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
            cFEng::mInstance.SetAddress(*queue.get_first<void**>(-9));
            cFEng::hbQueuePackageMessage.fun = injector::MakeCALL(queue.get_first(), cFEng::QueuePackageMessage, true).get();
            injector::MakeCALL(hudMessage.get_first(6), cFEng::QueuePackageMessage, true);
        };
    }
} Frontend;
