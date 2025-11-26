module;

#include <stdafx.h>

export module x_gameobjectsmfc;

import ComVars;

export void InitX_GameObjectsMFC()
{
    //mirrors fix
    auto pattern = hook::module_pattern(GetModuleHandle(L"X_GameObjectsMFC"), "D8 3D ? ? ? ? D9 5C 24 0C D9");
    injector::WriteMemory(pattern.get_first(2), &Screen.fMirrorFactor, true); //0x10101F39

    //doors graphics fix
    static float fVisibilityFactor1 = 0.5f;
    static float fVisibilityFactor2 = 1.5f;
    pattern = hook::module_pattern(GetModuleHandle(L"X_GameObjectsMFC"), "D9 05 ? ? ? ? 89 44 24 1C"); //1000AD9E
    injector::WriteMemory(pattern.get_first(2), &fVisibilityFactor1, true);
    pattern = hook::module_pattern(GetModuleHandle(L"X_GameObjectsMFC"), "D9 05 ? ? ? ? 89 4C 24 18"); //1000AD55
    injector::WriteMemory(pattern.get_first(2), &fVisibilityFactor2, true);

    //FOV
    CIniReader iniReader("");
    static bool bRestoreCutsceneFOV = iniReader.ReadInteger("MAIN", "RestoreCutsceneFOV", 0) != 0;
    static float fFOVFactor = iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f);
    if (!fFOVFactor) { fFOVFactor = 1.0f; }

    pattern = hook::module_pattern(GetModuleHandle(L"X_GameObjectsMFC"), "A0 ? ? ? ? 84 C0 0F 85"); //byte_101A7AA0
    X_Crosshair::sm_bCameraPathRunning.SetAddress(*pattern.get_first<bool*>(1));

    pattern = hook::module_pattern(GetModuleHandle(L"X_GameObjectsMFC"), "8B 44 24 04 89 81 38 05 00 00"); //void __thiscall X_LevelRuntimeCamera::setFOV(X_LevelRuntimeCamera *this, float)
    struct X_LevelRuntimeCamerasetFOVHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)&regs.eax = *(float*)(regs.esp + 4);
            *(float*)(regs.ecx + 0x538) = *(float*)&regs.eax;

            if (bRestoreCutsceneFOV && X_Crosshair::sm_bCameraPathRunning && !Screen.bDrawBordersForCameraOverlay)
            {
                *(float*)(regs.ecx + 0x538) *= (4.0f / 3.0f) / Screen.fAspectRatio;
                return;
            }

            *(float*)(regs.ecx + 0x538) *= fFOVFactor;
        }
    }; injector::MakeInline<X_LevelRuntimeCamerasetFOVHook>(pattern.get_first(0), pattern.get_first(10));

    pattern = hook::module_pattern(GetModuleHandle(L"X_GameObjectsMFC"), "B1 01 88 46 65 E8 ? ? ? ? 5E C2 08 00");
    struct CameraOverlayHook
    {
        void operator()(injector::reg_pack& regs)
        {
            Screen.bDrawBordersForCameraOverlay = false;
            *((BYTE*)&(regs.ecx)) = 1;
            *(uint8_t*)(regs.esi + 0x65) = LOBYTE(regs.eax);

            auto x = *(uint32_t*)(regs.esi + 0x48);
            auto y = *(uint32_t*)(regs.esi + 0x4C);
            auto z = *(uint32_t*)(regs.esi + 0x50);

            //what happens here is check for some camera coordinates
            if ((x == 0x40d9d740 && y == 0x40d9d95a && z == 0xc24f706a) || (x == 0x405b016c && y == 0x40d69c24 && z == 0xc1a50336) || (x == 0xc0a3f326 && y == 0x40ee9c24 && z == 0xc2101fe9) || //https://i.imgur.com/Kn7lHIc.png
                (x == 0xc1564e4e && y == 0x406865f0 && z == 0xc253cb06) ||																														 // https://i.imgur.com/7Z0aaKz.png
                (x == 0xc01d03ff && y == 0x40e39c24 && z == 0x42ce75c2) || (x == 0xc10a916a && y == 0x412ee03e && z == 0x42cc4f35) || (x == 0x4117f993 && y == 0x418ee709 && z == 0x424c1fe9)    //https://i.imgur.com/7aw4nNh.png
                )
            {
                Screen.bDrawBordersForCameraOverlay = true;
            }
        }
    }; injector::MakeInline<CameraOverlayHook>(pattern.get_first(0)); // 100E19B7
}