#include "stdafx.h"
#include <assembly64.hpp>
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
    float hfovRad = InFOV * M_PI / 180.0f;
    float vfovRad = 2.0f * atan(tan(hfovRad * 0.5f) / (16.0f / 9.0f));
    float newfovRad = 2.0f * atan(newAspect * tan(vfovRad * 0.5f));
    float vfov = (vfovRad * 180.0f) / M_PI;
    float newfov = (newfovRad * 180.0f) / M_PI;

    return newfov;
}

void Init()
{
    CIniReader iniReader("");
    static float fOverrideFOV = iniReader.ReadFloat("MAIN", "OverrideFOV", -1.0f);
    static float fFOVScalar = iniReader.ReadFloat("MAIN", "FOVScalar", 1.0f);

    uintptr_t loc_140907F30 = reinterpret_cast<uintptr_t>(hook::pattern("F2 0F 10 41 08 F2 0F 11 03 48 83 C4 20 5B C3").get_first(0));
    struct BlackBarRemoveHook
    {
        void operator()(injector::reg_pack& regs)
        {
            // TODO: find a better way to manage this because this is very very bad...
            if (*(uint32_t*)(regs.rcx + 8) == 0x44F00000)
                *(uint64_t*)regs.rbx = 0;
            else
                *(double*)regs.rbx = *(double*)(regs.rcx + 8);
        }
    }; injector::MakeInline<BlackBarRemoveHook>(loc_140907F30, loc_140907F30 + 9);

    // use the "constrain aspect ratio" area to modify rcx and then jump back to unconstrain area
    uintptr_t loc_14125F3D8 = reinterpret_cast<uintptr_t>(hook::pattern("F3 0F 10 51 28 48 8D 55 A7 49 8B C8 49 81 C1 90 00 00 00").get_first(0));
    struct AspectRatioHook
    {
        void operator()(injector::reg_pack& regs)
        {
            // in order to make this thread-safe and persistent, we have to create an instance and free it later
            CameraInfo *cam = (CameraInfo*)malloc(sizeof(CameraInfo));
            memcpy(cam, (void*)(regs.rcx), sizeof(CameraInfo));

            int32_t Width = *(int32_t*)(regs.r9 + 0x98) - *(int32_t*)(regs.r9 + 0x90);
            int32_t Height = *(int32_t*)(regs.r9 + 0x9C) - *(int32_t*)(regs.r9 + 0x94);

            float oldFOV = cam->fFOV;
            if (fOverrideFOV > 0.0f)
                oldFOV = fOverrideFOV;

            float newFOV = RecalculateFOV_16by9(oldFOV, ((float)Width/(float)Height));
#ifdef _DEBUG
            sprintf(debugstr, "OldFOV: %.2f | NewFOV: %.2f", cam->fFOV, newFOV);
            OutputDebugStringA(debugstr);
#endif
            cam->fFOV = newFOV * fFOVScalar;
            regs.rcx = (uint64_t)cam;
        }
    }; injector::MakeInline<AspectRatioHook>(loc_14125F3D8, loc_14125F3D8 + 0x69);

    // force branch to AspectRatioHook
    uintptr_t lov_14125F3D6 = reinterpret_cast<uintptr_t>(hook::pattern("F6 41 2C 01 49 8B D9").get_first(0)) + 0x1F;
    injector::MakeNOP(lov_14125F3D6, 2);

    // continue to unconstrained area
    uintptr_t loc_14125F446 = reinterpret_cast<uintptr_t>(hook::pattern("F3 0F 10 51 18 F3 0F 5F 15 51 89 06 01 41 8B 81 98 00 00 00").get_first(0));
    injector::MakeJMP(loc_14125F3D8 + 0x69, loc_14125F446);


    uintptr_t loc_14125F453 = loc_14125F446 + 0xD;
    struct CamFreeHook
    {
        void operator()(injector::reg_pack& regs)
        {
            CameraInfo* cam = (CameraInfo*)regs.rcx;
            free(cam);

            int32_t Width = *(int32_t*)(regs.r9 + 0x98) - *(int32_t*)(regs.r9 + 0x90);
            int32_t Height = *(int32_t*)(regs.r9 + 0x9C) - *(int32_t*)(regs.r9 + 0x94);

            regs.rax = Width;
            regs.rcx = Height;
        }
    }; injector::MakeInline<CamFreeHook>(loc_14125F453, loc_14125F453 + 0x1C);
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("E8 F1 01 00 00 0F B7 D8"));
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
