module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module ModeSwitch;

import ComVars;
import GraphicNovel;
import Settings;

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

        if (szClassName == ".?AVMaxPayne_GameMode@@")
            return "game";
        if (szClassName == ".?AVMaxPayne_MenuMode@@")
            return "menu";
        if (szClassName == ".?AVMaxPayne_GraphicNovelMode@@")
            return "graphicnovel";
        if (szClassName == ".?AVMaxPayne_StatisticsMode@@")
            return "statistics";
        return szClassName;
    }

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
            profile = *(pActiveMode + MaxPayne_GameMode::PAUSED) ? eGamepadProfile::Pause : eGamepadProfile::Main;
        GamepadProfile.store(profile, std::memory_order_relaxed);

        if (CurrentGameMode == "graphicnovel")
            MaxPayne_GraphicNovelPage::Update();

        // graphic novels in their original framing keep the cursor on the 4:3 page
        UpdateCursorBounds(CurrentGameMode != "graphicnovel" || !Screen.bGraphicNovelMode);
        MaxPayne_GraphicNovelMode::Update(CurrentGameMode == "graphicnovel" ? pActiveMode : nullptr);
    }
}

// Mouse smoothing (X_InputDeviceMouse::update): each frame applies half of the movement not applied yet and carries the
// rest over to the next one. Switched off, the movement applies in the frame it's made.
void(__fastcall* SmoothMouseMovement)(float* pMovement, void* edx) = nullptr;
void __fastcall SmoothMouseMovementHook(float* pMovement, void* edx)
{
    static auto& nMouseSmoothing = FixSettings().Int("MAIN.MouseSmoothing");
    if (nMouseSmoothing)
        SmoothMouseMovement(pMovement, edx);
}

class ModeSwitch
{
public:
    ModeSwitch()
    {
        WFP::onInitEvent() += []()
        {
            auto pattern = hook::pattern("83 F8 ? C7 44 24");
            static auto X_ModeSwitchupdateHook = safetyhook::create_mid(pattern.get_first(), X_ModeSwitch::update);

            // Mouse: X_InputDeviceMouse turns the movement into a speed by dividing it by the frame time
            // X_Input::update gets, and aiming and the menu cursor turn that back into a distance with the
            // frame time the game runs on. The game runs on an average of the last frame times while
            // X_Input got the last frame time alone, so whenever frame times varied, part of the movement
            // was lost (slow frames) or exaggerated (fast frames after them). X_Input gets the averaged
            // time too now, X_TimeUpdate::getRelativeTime stored just before.
            pattern = hook::pattern("D9 5C 24 3C 8B CF E8 ? ? ? ? D9 5C 24 ? 8B 44 24 40 50 E8"); // X_ModeSwitch::update
            injector::WriteMemory<uint8_t>(pattern.get_first(18), 0x3C, true); // mov eax, [esp+40h] -> mov eax, [esp+3Ch] //0x60128B

            // Mouse smoothing, see SmoothMouseMovementHook
            pattern = hook::pattern("8D 4D DC E8 ? ? ? ? D9 45 DC D8 46 5B"); // X_InputDeviceMouse::update
            SmoothMouseMovement = (decltype(SmoothMouseMovement))injector::MakeCALL(pattern.get_first(3), SmoothMouseMovementHook, true).get(); //0x5191CC

            pattern = hook::pattern("A0 ? ? ? ? 84 C0 0F 85 ? ? ? ? 8B 86");
            X_Crosshair::sm_bCameraPathRunning.SetAddress(*pattern.get_first<bool*>(1));

            // Graphic novels: key toggles between the original framing and the whole page as large as the screen allows
            pattern = hook::pattern("8B 06 8B CE 33 FF FF 50 10"); //60146E
            struct GraphicNovelPageUpdateHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.eax = *(uint32_t*)(regs.esi);
                    regs.ecx = regs.esi;
                    regs.edi = 0;

                    if (!X_Crosshair::sm_bCameraPathRunning)
                        Screen.bDrawBordersForCameraOverlay = false;

                    static bool bWasPressed = false;
                    if (CurrentGameMode != "graphicnovel")
                    {
                        bWasPressed = false;
                        return;
                    }

                    bool bPressed = nGraphicNovelModeKey && (GetAsyncKeyState(nGraphicNovelModeKey) & 0x8000) != 0;
                    if (!bPressed && bWasPressed)
                    {
                        // Applied through the setting, which the menu and the settings file follow
                        FixSettings().Set("MAIN.GraphicNovelMode", Screen.bGraphicNovelMode ? "0" : "1");
                        MaxPayne_GraphicNovelPage::Refresh();
                    }
                    bWasPressed = bPressed;
                }
            }; injector::MakeInline<GraphicNovelPageUpdateHook>(pattern.get_first(0), pattern.get_first(6));

            // Menu and graphic novel cursor bounds, see UpdateCursorBounds. X_MenuModeBase::update and
            // MaxPayne_GraphicNovelMode::update clamp the cursor with the same inlined code, which loads the
            // bounds from constants shared with other code, so its loads are pointed at the fix's own.
            const std::string cursorPattern = "D9 05 ? ? ? ? D8 5D C4 DF E0 F6 C4 01 74 08 D9 05 ? ? ? ? EB 1B D9 45 C4 D9 05 ? ? ? ? D8 5D C4 DF E0 F6 C4 41 75 08 DD D8 D9 05 ? ? ? ? D9 55 C4 D9 05 ? ? ? ? D8 5D C8 DF E0 F6 C4 01 74 08 D9 05 ? ? ? ? EB 1B D9 45 C8 D9 05 ? ? ? ? D8 5D C8 DF E0 F6 C4 41 75 08 DD D8 D9 05";
            for (const auto* suffix : { " ? ? ? ? 8B 96 F0 00 00 00", " ? ? ? ? 8B 97 9E 00 00 00" })
            {
                auto match = hook::pattern(cursorPattern + suffix).get_first<uint8_t>();
                injector::WriteMemory(match + 2, &CursorBounds.fRight, true);
                injector::WriteMemory(match + 18, &CursorBounds.fRight, true);
                injector::WriteMemory(match + 29, &CursorBounds.fLeft, true);
                injector::WriteMemory(match + 47, &CursorBounds.fLeft, true);
                injector::WriteMemory(match + 56, &CursorBounds.fBottom, true);
                injector::WriteMemory(match + 72, &CursorBounds.fBottom, true);
                injector::WriteMemory(match + 83, &CursorBounds.fTop, true);
                injector::WriteMemory(match + 101, &CursorBounds.fTop, true);
            }
        };
    }
} ModeSwitch;
