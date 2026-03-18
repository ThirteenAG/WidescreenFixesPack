module;

#include <stdafx.h>

export module x_modesmfc;

import ComVars;

namespace X_ModeSwitch
{
    std::string PendingGameMode;

    SafetyHookInline shSetModeSwitch = {};
    void __fastcall setModeSwitch(void* X_ModeSwitch, void* edx, void* a2)
    {
        PendingGameMode = std::string_view((char*)a2 + 4);
        return shSetModeSwitch.unsafe_fastcall(X_ModeSwitch, edx, a2);
    }
}

namespace MaxPayne_HUDFadeUpdate
{
    SafetyHookInline shupdate = {};
    void __fastcall update(float* MaxPayne_HUDFadeUpdate, void* edx, const void* X_TimeUpdate)
    {
        Screen.bIsFading = true;
        return shupdate.unsafe_fastcall(MaxPayne_HUDFadeUpdate, edx, X_TimeUpdate);
    }

    SafetyHookInline shdestruct = {};
    void __fastcall destruct(float* MaxPayne_HUDFadeUpdate, void* edx)
    {
        Screen.bIsFading = false;
        return shdestruct.unsafe_fastcall(MaxPayne_HUDFadeUpdate, edx);
    }
}

export void InitX_ModesMFC()
{
    static CIniReader iniReader("");
    static int32_t nGraphicNovelModeKey = iniReader.ReadInteger("MAIN", "GraphicNovelModeKey", VK_F2);

    auto pattern = hook::module_pattern(GetModuleHandle(L"X_ModesMFC"), "56 8B F1 8B 46 ? 85 C0 0F 85");
    X_ModeSwitch::shSetModeSwitch = safetyhook::create_inline(pattern.get_first(0), X_ModeSwitch::setModeSwitch);

    //Graphic Novels Handler
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
                    iniReader.WriteInteger("MAIN", "GraphicNovelMode", Screen.bGraphicNovelMode);
                }

                oldState = curState;

                if (Screen.bGraphicNovelMode)
                {
                    Screen.fNovelsScale = 0.003125f;
                    Screen.fNovelsOffset = -1.0f;
                    Screen.fViewPortSizeX = 480.0f * Screen.fAspectRatio;
                    Screen.fViewPortSizeY = 480.0f;
                }
                else
                {
                    Screen.fNovelsScale = 0.003125f;
                    Screen.fNovelsOffset = -1.0f;
                    Screen.fViewPortSizeX = (480.0f * Screen.fAspectRatio) / 1.17936117936f;
                    Screen.fViewPortSizeY = 480.0f / 1.17936117936f;
                }
            }
            else
            {
                Screen.fViewPortSizeX = 640.0f;
                Screen.fViewPortSizeY = 480.0f;
                Screen.fNovelsScale = Screen.fHudScale;
                Screen.fNovelsOffset = Screen.fHudOffset;
            }
        }
    }; injector::MakeInline<GraphicNovelPageUpdateHook>(GraphicNovelPageUpdate.get_first(0), GraphicNovelPageUpdate.get_first(6));

    pattern = hook::module_pattern(GetModuleHandle(L"X_ModesMFC"), "83 F8 ? 75 ? ? ? ? BF ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? DF E0 F6 C4 ? 74 ? 8B 7C 24 ? 8B 4E ? ? ? 57 FF 52 ? 8B 4E");
    static auto X_ModeSwitchupdateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        static void* prevMode = nullptr;
        auto mode = *(void**)(regs.esi + 0x19);

        if (mode != prevMode)
        {
            prevMode = mode;
            CurrentGameMode = X_ModeSwitch::PendingGameMode;
        }

        Screen.bIsFading = regs.eax == 2 || regs.eax == 3;
    });

    pattern = hook::pattern("83 EC ? 53 8B 1D");
    MaxPayne_HUDFadeUpdate::shupdate = safetyhook::create_inline(pattern.get_first(0), MaxPayne_HUDFadeUpdate::update);

    pattern = hook::pattern("E8 ? ? ? ? 8B 46 ? 8B 48");
    MaxPayne_HUDFadeUpdate::shdestruct = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first(0)).as_int(), MaxPayne_HUDFadeUpdate::destruct);

}