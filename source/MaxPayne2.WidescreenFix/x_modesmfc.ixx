module;

#include <stdafx.h>

export module x_modesmfc;

import ComVars;
import e2mfc;

namespace X_ModeSwitch
{
    enum
    {
        ACTIVE_BASIC_MODE = 0x19,
    };

    // Name the mode by its class. The requested name can't be used: setModeSwitch ignores requests
    // while fading out, and a new request can already be accepted in the frame the switch completes.
    std::string_view GetModeName(uint8_t* pMode)
    {
        if (!pMode)
            return {};

        auto pVTable = *(uintptr_t**)pMode;
        auto pCompleteObjectLocator = (uint8_t*)pVTable[-1];
        auto pTypeDescriptor = *(uint8_t**)(pCompleteObjectLocator + 12);
        std::string_view szClassName = (const char*)(pTypeDescriptor + 8);

        if (szClassName == ".?AVMP_GameMode@@")
            return "game";
        if (szClassName == ".?AVMP_MenuMode@@")
            return "menu";
        if (szClassName == ".?AVMP_GraphicNovelMode@@")
            return "graphicnovel";
        if (szClassName == ".?AVMP_StatisticsMode@@")
            return "statistics";
        return szClassName;
    }

    int32_t nGraphicNovelModeKey = VK_F2;

    void update(SafetyHookContext& regs)
    {
        auto pModeSwitch = (uint8_t*)regs.esi;
        auto pActiveMode = *(uint8_t**)(pModeSwitch + ACTIVE_BASIC_MODE);

        static uint8_t* pPrevActiveMode = nullptr;
        if (pActiveMode != pPrevActiveMode)
        {
            pPrevActiveMode = pActiveMode;
            CurrentGameMode = GetModeName(pActiveMode);
            if (CurrentGameMode != "graphicnovel")
                MaxPayne_GraphicNovelPage::Reset();
        }

        auto profile = eGamepadProfile::Menu;
        if (pActiveMode && CurrentGameMode == "game")
            profile = *(pActiveMode + MP_GameMode::PAUSED) ? eGamepadProfile::Pause : eGamepadProfile::Main;
        GamepadProfile.store(profile, std::memory_order_relaxed);

        if (X_Crosshair::sm_bCameraPathRunning.is_initialized() && !X_Crosshair::sm_bCameraPathRunning)
            Screen.bDrawBordersForCameraOverlay = false;

        if (CurrentGameMode == "graphicnovel")
            MaxPayne_GraphicNovelPage::Update();

        // graphic novels in their original framing have borders around the 4:3 page
        UpdateCursorBounds(CurrentGameMode != "graphicnovel" || !Screen.bGraphicNovelMode);

        // Graphic novels: key toggles between the original framing and the whole page as large as the screen allows
        static bool bWasPressed = false;
        if (CurrentGameMode != "graphicnovel")
        {
            bWasPressed = false;
            MP_GraphicNovelMode::Update(nullptr);
            return;
        }

        bool bPressed = (GetAsyncKeyState(nGraphicNovelModeKey) & 0x8000) != 0;
        if (!bPressed && bWasPressed)
        {
            Screen.bGraphicNovelMode = !Screen.bGraphicNovelMode;
            CIniReader iniReader("");
            iniReader.WriteInteger("MAIN", "GraphicNovelMode", Screen.bGraphicNovelMode);
            MaxPayne_GraphicNovelPage::Refresh();
        }
        bWasPressed = bPressed;

        MP_GraphicNovelMode::Update(pActiveMode);
    }

    // Before the last X_VideoInterface::endScene of the frame, after the overlaid modes (HUD) are
    // drawn. The bars the widescreen fix adds are drawn here with P_Driver::clearScreen, the same
    // way the game draws its letterbox bars.
    void render(SafetyHookContext& regs)
    {
        if (bGameViewRendered)
        {
            auto nWidth = static_cast<int32_t>(std::lround(Screen.fWidth * Cinematic::CurrentBorders.fPillarbox * 0.5f));
            if (nWidth > 0)
                DrawPillarboxBars(nWidth, nWidth);

            // the scope overlay covers the 4:3 area, unless the widescreen HUD fills the screen around it
            if (MP_GameMode::IsSniperScopeOn() && !IsSniperScopeFilled())
                Draw4by3Borders(1);
        }

        if (CurrentGameMode == "graphicnovel")
        {
            // pages are 3D scenes, the widened view shows more than the original 4:3 frame
            if (Screen.bGraphicNovelMode)
                Draw4by3Borders();
        }
        else if (Screen.bDrawBordersForCameraOverlay)
        {
            Draw4by3Borders(1);
        }

        bGameViewRendered = false;
    }
}

export void InitX_ModesMFC()
{
    CIniReader iniReader("");
    X_ModeSwitch::nGraphicNovelModeKey = iniReader.ReadInteger("MAIN", "GraphicNovelModeKey", VK_F2);

    auto pattern = hook::module_pattern(GetModuleHandle(L"X_ModesMFC"), "8B 46 0D 83 F8 03 75 ? D9 46 11");
    static auto X_ModeSwitchUpdateHook = safetyhook::create_mid(pattern.get_first(), X_ModeSwitch::update); //0x10001914

    // Mouse: X_InputDeviceMouse turns the movement into a speed by dividing it by the frame time
    // X_Input::update gets, and aiming and the menu cursor turn that back into a distance with the
    // frame time the game runs on. The game runs on an average of the last frame times while
    // X_Input got the last frame time alone, so whenever frame times varied, part of the movement
    // was lost (slow frames) or exaggerated (fast frames after them). X_Input gets the averaged
    // time too now, X_TimeUpdate::getRelativeTime stored just before.
    pattern = hook::module_pattern(GetModuleHandle(L"X_ModesMFC"), "D9 5C 24 18 8B CF FF 15 ? ? ? ? D9 5C 24 10 8B 44 24 1C 50 FF 15"); // X_ModeSwitch::update
    injector::WriteMemory<uint8_t>(pattern.get_first(19), 0x18, true); // mov eax, [esp+1Ch] -> mov eax, [esp+18h] //0x10001906

    pattern = hook::module_pattern(GetModuleHandle(L"X_ModesMFC"), "8B 4E 09 8B 11 FF 52 10 8B 4E 09 8B 01 FF 50 14");
    static auto X_ModeSwitchRenderHook = safetyhook::create_mid(pattern.get_first(), X_ModeSwitch::render); //0x10008A7E
}
