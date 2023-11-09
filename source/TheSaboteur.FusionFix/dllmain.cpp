#include "stdafx.h"

struct PblVector4
{
    float x;
    float y;
    float z;
    float w;
};

struct PblMatrix4x4
{
    PblVector4 _m[4];
};

struct OdinCamera
{
    bool m_bOrthographic;
    PblMatrix4x4 m_ViewMatrix;
    PblMatrix4x4 m_ViewMatrixInverse;
    PblMatrix4x4 m_ProjMatrix;
    PblMatrix4x4 m_ProjMatrixInverse;
    unsigned int m_Dirty;
    float m_NearPlane;
    float m_FarPlane;
    float m_TanHalfFOVWidth;
    float m_TanHalfFOVHeight;

    static void __fastcall SetFOV(OdinCamera* OdinCamera, void* edx, float a_FovX, float a_AspectRatio)
    {
        // too high fov breaks rendering (sky and objects start to disappear)
        auto fAspectRatioDiff = std::clamp((1.0f / a_AspectRatio) / (4.0f / 3.0f), 0.0f, 2.1f); // 2.1 seems safe before that happens
        auto a_FovXa = a_FovX * 0.5f;
        auto a_FovXb = tan(a_FovXa);
        OdinCamera->m_Dirty |= 3u;
        OdinCamera->m_bOrthographic = 0;
        OdinCamera->m_TanHalfFOVWidth = a_FovXb * fAspectRatioDiff;
        OdinCamera->m_TanHalfFOVHeight = a_FovXb * a_AspectRatio * fAspectRatioDiff;
    }

    static void __fastcall SetPerspective(OdinCamera* OdinCamera, void* edx, float a_NearPlane, float a_FarPlane, float a_FovX, float a_AspectRatio)
    {
        auto a_FovXa = a_FovX * 0.5f;
        auto a_FovXb = tan(a_FovXa);
        OdinCamera->m_Dirty |= 3u;
        OdinCamera->m_bOrthographic = 0;
        OdinCamera->m_NearPlane = a_NearPlane;
        OdinCamera->m_FarPlane = a_FarPlane;
        OdinCamera->m_TanHalfFOVWidth = a_FovXb;
        OdinCamera->m_TanHalfFOVHeight = a_FovXb * a_AspectRatio;
    }

    static float __fastcall GetAspectRatio(OdinCamera* OdinCamera, void* edx)
    {
        return OdinCamera->m_TanHalfFOVHeight / OdinCamera->m_TanHalfFOVWidth;
    }
};

void Init()
{
    WFP::onInitEventAsync() += []()
    {
        CIniReader iniReader("");
        auto bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) != 0;
        auto bBorderlessWindowed = iniReader.ReadInteger("MAIN", "BorderlessWindowed", 1) != 0;

        if (bFixFOV)
        {
            auto pattern = hook::pattern("D9 44 24 04 56 DC 0D ? ? ? ? 8B F1 D9 5C 24 08");
            injector::MakeJMP(pattern.get_first(0), OdinCamera::SetFOV, true);

            //auto pattern = hook::pattern("D9 44 24 0C 56 DC 0D ? ? ? ? 8B F1 D9 5C 24 10 D9 44 24 10 E8 ? ? ? ? D9 5C 24 10 D9 44 24 10 83 8E");
            //injector::MakeJMP(pattern.get_first(0), OdinCamera::SetPerspective, true);

            //pattern = hook::pattern("51 D9 81 ? ? ? ? D8 B1");
            //injector::MakeJMP(pattern.get_first(0), OdinCamera::GetAspectRatio, true);
        }

        if (bBorderlessWindowed)
        {
            IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
                std::forward_as_tuple("CreateWindowExA", WindowedModeWrapper::CreateWindowExA_Hook),
                std::forward_as_tuple("CreateWindowExW", WindowedModeWrapper::CreateWindowExW_Hook),
                std::forward_as_tuple("SetWindowLongA", WindowedModeWrapper::SetWindowLongA_Hook),
                std::forward_as_tuple("SetWindowLongW", WindowedModeWrapper::SetWindowLongW_Hook),
                std::forward_as_tuple("AdjustWindowRect", WindowedModeWrapper::AdjustWindowRect_Hook),
                std::forward_as_tuple("SetWindowPos", WindowedModeWrapper::SetWindowPos_Hook)
            );
        }
    };

    static auto futures = WFP::onInitEventAsync().executeAllAsync();

    WFP::onGameInitEvent() += []() //todo: add onGameInitEvent hook
    {
        for (auto& f : futures.get())
            f.wait();
        futures.get().clear();
    };

    WFP::onInitEvent().executeAll();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("56 68 ? ? ? ? 68 ? ? ? ? 8B F1 6A 19"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    else if (reason == DLL_PROCESS_DETACH)
    {

    }
    return TRUE;
}
