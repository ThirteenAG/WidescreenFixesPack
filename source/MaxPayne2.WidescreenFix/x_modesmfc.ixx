module;

#include <stdafx.h>

export module x_modesmfc;

import ComVars;

namespace X_ModeSwitch
{
    SafetyHookInline shSetModeSwitch = {};
    void __fastcall setModeSwitch(void* X_ModeSwitch, void* edx, void* a2)
    {
        CurrentGameMode = std::string_view((char*)a2 + 4);
        return shSetModeSwitch.unsafe_fastcall(X_ModeSwitch, edx, a2);
    }
}

export void InitX_ModesMFC()
{
    static CIniReader iniReader("");
    static int32_t nGraphicNovelModeKey = iniReader.ReadInteger("MAIN", "GraphicNovelModeKey", VK_F2);

    auto pattern = hook::module_pattern(GetModuleHandle(L"X_ModesMFC"), "56 8B F1 8B 46 ? 85 C0 0F 85");
    X_ModeSwitch::shSetModeSwitch = safetyhook::create_inline(pattern.get_first(0), X_ModeSwitch::setModeSwitch);

    //Graphic Novels Handler
    static bool bPatched;
    static uint16_t oldState = 0;
    static uint16_t curState = 0;

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

            if (CurrentGameMode == "graphicnovel")
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