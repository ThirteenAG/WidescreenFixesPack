module;

#include <stdafx.h>
#include <d3d9.h>

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

    std::unordered_map<void*, std::pair<float, float>> originalCenterCache;

    void** mInstance = nullptr;

    static void* s_currentLeftGroup = nullptr;
    static void* s_currentRightGroup = nullptr;

    injector::hook_back<void(__fastcall*)(void*, void*, int, char const*, int)> hbQueuePackageMessage;
    void __fastcall QueuePackageMessage(void* instance, void* edx, int MessageHash, char const* packageName, int a4)
    {
        updateWidescreenOffset();

        hbQueuePackageMessage.fun(instance, edx, MessageHash, packageName, a4);

        void* leftGroup = FE::Object::FindObject(packageName, 0x1603009E);
        void* rightGroup = FE::Object::FindObject(packageName, 0x5D0101F1);

        if (leftGroup && leftGroup != s_currentLeftGroup)
        {
            originalCenterCache.erase(s_currentLeftGroup);
            s_currentLeftGroup = leftGroup;
        }

        if (rightGroup && rightGroup != s_currentRightGroup)
        {
            originalCenterCache.erase(s_currentRightGroup);
            s_currentRightGroup = rightGroup;
        }

        if (leftGroup)
        {
            auto it = originalCenterCache.find(leftGroup);
            if (it == originalCenterCache.end())
            {
                float x = 0.0f, y = 0.0f;
                FE::Object::GetCenter(leftGroup, &x, &y);
                originalCenterCache[leftGroup] = { x, y };
                it = originalCenterCache.find(leftGroup);
            }

            const auto& [origX, origY] = it->second;
            FE::Object::SetCenter(leftGroup, origX - fWidescreenHudOffset, origY);
        }

        if (rightGroup)
        {
            auto it = originalCenterCache.find(rightGroup);
            if (it == originalCenterCache.end())
            {
                float x = 0.0f, y = 0.0f;
                FE::Object::GetCenter(rightGroup, &x, &y);
                originalCenterCache[rightGroup] = { x, y };
                it = originalCenterCache.find(rightGroup);
            }

            const auto& [origX, origY] = it->second;
            FE::Object::SetCenter(rightGroup, origX + fWidescreenHudOffset, origY);
        }
    }
}

namespace FEngHud
{
    SafetyHookInline shSetWideScreenMode = {};
    void __fastcall SetWideScreenMode(void* _this, void* edx)
    {
        static int prevResX = ResX;
        static int prevResY = ResY;

        if (ResX != prevResX || ResY != prevResY)
        {
            uint8_t* pThis = reinterpret_cast<uint8_t*>(_this);
            uint8_t* pCachedState = pThis + 882;
            bool isWidescreen = *pCachedState;

            constexpr int kEnableHash = 0x62ED04EC;
            constexpr int kDisableHash = 0x53EC068C;

            int messageHash = isWidescreen ? kEnableHash : kDisableHash;
            cFEng::QueuePackageMessage(*cFEng::mInstance, 0, messageHash, *reinterpret_cast<char**>(pThis + 44), 0);

            prevResX = ResX;
            prevResY = ResY;
        }

        return shSetWideScreenMode.unsafe_fastcall(_this, edx);
    }
}

class Frontend
{
public:
    Frontend()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            cFEng::fHudAspectRatioConstraint = ParseWidescreenHudOffset(iniReader.ReadString("MAIN", "HudAspectRatioConstraint", ""));

            auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 5E C2 ? ? 8B 51");
            FE::Object::SetCenter = (decltype(FE::Object::SetCenter))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? 83 C4 ? ? ? ? ? ? ? ? ? ? ? ? 56 E8 ? ? ? ? 83 C4 ? 5E C2");
            FE::Object::GetCenter = (decltype(FE::Object::GetCenter))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 50 57 68 ? ? ? ? 8B CD E8 ? ? ? ? ? ? 8B 50");
            FE::Object::FindObject = (decltype(FE::Object::FindObject))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = hook::pattern("E8 ? ? ? ? 5E 59 C3 8B 96");
            cFEng::mInstance = *pattern.get_first<void**>(-9);
            cFEng::hbQueuePackageMessage.fun = injector::MakeCALL(pattern.get_first(), cFEng::QueuePackageMessage, true).get();

            pattern = hook::pattern("52 68 ? ? ? ? E8 ? ? ? ? 5E 59");
            cFEng::hbQueuePackageMessage.fun = injector::MakeCALL(pattern.get_first(6), cFEng::QueuePackageMessage, true).get();

            //can't change res in game
            //pattern = hook::pattern("E8 ? ? ? ? 8B 86 ? ? ? ? 0B 86");
            //FEngHud::shSetWideScreenMode = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), FEngHud::SetWideScreenMode);

            static auto DrawPillarboxes = []()
            {
                float target_aspect = 2.25f;
                float video_width = ResY * target_aspect;
                float pillar = (ResX - video_width) / 2.0f;
                if (pillar > 0.0f)
                {
                    D3DRECT rects[2];
                    rects[0].x1 = 0;
                    rects[0].y1 = 0;
                    rects[0].x2 = static_cast<LONG>(pillar);
                    rects[0].y2 = static_cast<LONG>(ResY);
                    rects[1].x1 = static_cast<LONG>(ResX) - static_cast<LONG>(pillar);
                    rects[1].y1 = 0;
                    rects[1].x2 = static_cast<LONG>(ResX);
                    rects[1].y2 = static_cast<LONG>(ResY);
                    (**Direct3DDevice)->Clear(2, rects, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
                }
            };

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