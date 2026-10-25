#include "stdafx.h"
#include "FixSettings.hpp"
#include <thread>

#pragma pack(push,1)
struct CameraInfo
{
    char unk[0x18];
    float fFOV;
};
#pragma pack(pop)

#ifdef _DEBUG
char debugstr[1024];
#endif

inline float RecalculateFOV_16by9(float InFOV, float newAspect)
{
    float hfovRad = static_cast<float>(InFOV * M_PI / 180.0f);
    float vfovRad = 2.0f * atan(tan(hfovRad * 0.5f) / (16.0f / 9.0f));
    float newfovRad = 2.0f * atan(newAspect * tan(vfovRad * 0.5f));
    float newfov = static_cast<float>((newfovRad * 180.0f) / M_PI);

    return newfov;
}

void Init()
{
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    static const auto& fOverrideFOV = settings.Float("MAIN.OverrideFOV");
    static const auto& fFOVScalar = settings.Float("MAIN.FOVFactor");

    auto blackBars = hook::pattern("F2 0F 10 41 08 F2 0F 11 03 48 83 C4 20 5B C3");
    struct BlackBarRemoveHook
    {
        void operator()(injector::reg_pack& regs)
        {
            // Match the authored side-bar width without changing other rectangles.
            if (*(uint32_t*)(regs.rcx + 8) == 0x44F00000)
                *(uint64_t*)regs.rbx = 0;
            else
                *(double*)regs.rbx = *(double*)(regs.rcx + 8);
        }
    };
    if (blackBars.size() == 1)
    {
        auto address = reinterpret_cast<uintptr_t>(blackBars.get_first());
        injector::MakeInline<BlackBarRemoveHook>(address, address + 9);
    }
    else
        OutputDebugStringA("Spyro Widescreen Fix: side-bar pattern unavailable; side-bar removal disabled.\n");

    // use the "constrain aspect ratio" area to modify rcx and then jump back to unconstrain area
    auto constrained = hook::pattern("F3 0F 10 51 28 48 8D 55 A7 49 8B C8 49 81 C1 90 00 00 00");
    auto branch = hook::pattern("F6 41 2C 01 49 8B D9");
    auto unconstrained = hook::pattern("F3 0F 10 51 18 F3 0F 5F 15 51 89 06 01 41 8B 81 98 00 00 00");
    if (constrained.size() != 1 || branch.size() != 1 || unconstrained.size() != 1)
    {
        OutputDebugStringA("Spyro Widescreen Fix: camera patterns unavailable; camera correction disabled.\n");
        return;
    }
    uintptr_t loc_14125F3D8 = reinterpret_cast<uintptr_t>(constrained.get_first());
    struct AspectRatioHook
    {
        void operator()(injector::reg_pack& regs)
        {
            // The following projection instructions consume only FOV before the continuation hook.
            static thread_local CameraInfo camera;
            CameraInfo* cam = &camera;
            memcpy(cam, (void*)(regs.rcx), sizeof(CameraInfo));

            int32_t Width = *(int32_t*)(regs.r9 + 0x98) - *(int32_t*)(regs.r9 + 0x90);
            int32_t Height = *(int32_t*)(regs.r9 + 0x9C) - *(int32_t*)(regs.r9 + 0x94);

            float oldFOV = cam->fFOV;
            if (fOverrideFOV > 0.0f)
                oldFOV = fOverrideFOV;

            float newFOV = oldFOV;
            if (Width > 0 && Height > 0 && std::isfinite(oldFOV) && oldFOV > 0.0f && oldFOV < 180.0f)
            {
                const float correctedFOV = RecalculateFOV_16by9(oldFOV, static_cast<float>(Width) / static_cast<float>(Height)) * (std::isfinite(fFOVScalar) ? std::clamp(fFOVScalar, 0.5f, 2.0f) : 1.0f);
                if (std::isfinite(correctedFOV) && correctedFOV > 0.0f)
                    newFOV = std::min(correctedFOV, 179.0f);
            }
#ifdef _DEBUG
            sprintf(debugstr, "OldFOV: %.2f | NewFOV: %.2f", cam->fFOV, newFOV);
            OutputDebugStringA(debugstr);
#endif
            cam->fFOV = newFOV;
            regs.rcx = (uint64_t)cam;
        }
    }; injector::MakeInline<AspectRatioHook>(loc_14125F3D8, loc_14125F3D8 + 0x69);

    // force branch to AspectRatioHook
    uintptr_t lov_14125F3D6 = reinterpret_cast<uintptr_t>(branch.get_first()) + 0x1F;
    injector::MakeNOP(lov_14125F3D6, 2);

    // continue to unconstrained area
    uintptr_t loc_14125F446 = reinterpret_cast<uintptr_t>(unconstrained.get_first());
    injector::MakeJMP(loc_14125F3D8 + 0x69, loc_14125F446);


    uintptr_t loc_14125F453 = loc_14125F446 + 0xD;
    struct CameraContinuationHook
    {
        void operator()(injector::reg_pack& regs)
        {
            int32_t Width = *(int32_t*)(regs.r9 + 0x98) - *(int32_t*)(regs.r9 + 0x90);
            int32_t Height = *(int32_t*)(regs.r9 + 0x9C) - *(int32_t*)(regs.r9 + 0x94);

            regs.rax = Width;
            regs.rcx = Height;
        }
    }; injector::MakeInline<CameraContinuationHook>(loc_14125F453, loc_14125F453 + 0x1C);
}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("E8 ? ? ? ? 0F B7 D8 E8 ? ? ? ? 4C 8B"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}
