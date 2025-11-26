module;

#include <stdafx.h>

export module x_modesmfc;

import ComVars;

export void InitX_ModesMFC()
{
    static CIniReader iniReader("");
    static int32_t nGraphicNovelModeKey = iniReader.ReadInteger("MAIN", "GraphicNovelModeKey", VK_F2);

    //Graphic Novels Handler
    static bool bPatched;
    static uint16_t oldState = 0;
    static uint16_t curState = 0;
    static uint32_t callAddr;

    auto pattern = hook::module_pattern(GetModuleHandle(L"X_ModesMFC"), "8B 5C 24 18 8B 01 53 FF 50 38");
    struct GraphicNovelXRefHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ebx = *(uint32_t*)(regs.esp + 0x18);
            regs.eax = *(uint32_t*)(regs.ecx);
            callAddr = *(uint32_t*)(regs.eax + 0x38);
        }
    }; injector::MakeInline<GraphicNovelXRefHook>(pattern.get_first(0), pattern.get_first(6)); //10001A6A

    static auto sub_484AE0 = (uint32_t)hook::get_pattern("8B 44 24 04 83 EC 34 53 55 56 57 50 8B F1"); //MaxPayne_GraphicNovelMode::update
    auto GraphicNovelPageUpdate = hook::module_pattern(GetModuleHandle(L"X_ModesMFC"), "8B 16 8B CE 33 FF FF 52 10"); //10001A7A 
    struct GraphicNovelPageUpdateHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edx = *(uint32_t*)(regs.esi);
            regs.ecx = regs.esi;
            regs.edi = 0;

            if (!X_Crosshair::sm_bCameraPathRunning)
                Screen.bDrawBordersForCameraOverlay = false;

            Screen.bIsInGraphicNovel = (callAddr == sub_484AE0);
            callAddr = 0;

            if (Screen.bIsInGraphicNovel)
            {
                curState = GetAsyncKeyState(nGraphicNovelModeKey);

                if (!curState && oldState)
                {
                    Screen.bGraphicNovelMode = !Screen.bGraphicNovelMode;
                    bPatched = !bPatched;
                    iniReader.WriteInteger("MAIN", "GraphicNovelMode", Screen.bGraphicNovelMode);
                }

                oldState = curState;

                if (Screen.bGraphicNovelMode)
                {
                    if (!bPatched)
                    {
                        Screen.fNovelsScale = 0.003125f;
                        Screen.fNovelsOffset = -1.0f;
                        Screen.fViewPortSizeX = 480.0f * Screen.fAspectRatio;
                        Screen.fViewPortSizeY = 480.0f;
                        bPatched = true;
                    }
                }
                else
                {
                    if (!bPatched)
                    {
                        Screen.fNovelsScale = 0.003125f;
                        Screen.fNovelsOffset = -1.0f;
                        Screen.fViewPortSizeX = (480.0f * Screen.fAspectRatio) / 1.17936117936f;
                        Screen.fViewPortSizeY = 480.0f / 1.17936117936f;
                        bPatched = true;
                    }
                }
            }
            else
            {
                if (bPatched)
                {
                    Screen.fViewPortSizeX = 640.0f;
                    Screen.fViewPortSizeY = 480.0f;
                    Screen.fNovelsScale = Screen.fHudScale;
                    Screen.fNovelsOffset = Screen.fHudOffset;
                    bPatched = false;
                }
            }
        }
    }; injector::MakeInline<GraphicNovelPageUpdateHook>(GraphicNovelPageUpdate.get_first(0), GraphicNovelPageUpdate.get_first(6));
}