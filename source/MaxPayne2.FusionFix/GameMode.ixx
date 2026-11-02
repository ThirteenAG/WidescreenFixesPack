module;

#include <stdafx.h>

export module GameMode;

import ComVars;
import e2mfc;

class GameMode
{
public:
    GameMode()
    {
        WFP::onInitEvent() += []()
        {
            // MP_GameMode render function, where the game view gets rendered
            auto pattern = hook::pattern("C6 86 48 01 00 00 01 FF 15 ? ? ? ? 8B 8E A0 10 00 00 FF 15");
            static auto MP_GameModeRenderHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) //0x451B93
            {
                MP_GameMode::pInstance = (uint8_t*)regs.esi;
                bGameViewRendered = true;
                Cinematic::UpdateBorders(MP_GameMode::pInstance);
            });

            // MP_GameMode destructor. The game mode is deleted on quit, while the progress bar still draws.
            pattern = hook::pattern("C7 06 ? ? ? ? C7 46 19 ? ? ? ? C7 86 BE 00 00 00 ? ? ? ? C7 44 24 24 19 00 00 00");
            static auto MP_GameModeDestructorHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) //0x46A412
            {
                if ((uint8_t*)regs.esi == MP_GameMode::pInstance)
                    MP_GameMode::pInstance = nullptr;
            });

            // Cutscene borders: every height multiplier read in MP_HUDMode's render function, which moves
            // subtitles clear of the letterbox bars and then draws them
            pattern = hook::pattern("8B 8E AE 00 00 00 33 DB 89 5C 24 ? E8"); // subtitle offset check
            injector::MakeCALL(pattern.get_first(12), Cinematic::GetBordersHeightMultiplier, true); //0x488A01
            pattern = hook::pattern("8B 8E AE 00 00 00 E8 ? ? ? ? D8 2D ? ? ? ? 8B 8E 07 02 00 00"); // subtitle offset
            injector::MakeCALL(pattern.get_first(6), Cinematic::GetBordersHeightMultiplier, true); //0x488A1D
            pattern = hook::pattern("FF 52 0C 8B 8E AE 00 00 00 E8"); // subtitle offset restore check
            injector::MakeCALL(pattern.get_first(9), Cinematic::GetBordersHeightMultiplier, true); //0x488B23
            pattern = hook::pattern("8B 8E AE 00 00 00 E8 ? ? ? ? D8 1D ? ? ? ? DF E0 F6 C4 05 0F 8A ? ? ? ? 8B 15"); // bars check
            injector::MakeCALL(pattern.get_first(6), Cinematic::GetBordersHeightMultiplier, true); //0x488C05
            pattern = hook::pattern("8B 8E AE 00 00 00 D9 5C 24 10 E8"); // bar height
            injector::MakeCALL(pattern.get_first(10), Cinematic::GetBordersHeightMultiplier, true); //0x488C68

            // MP_HUDMode's render function updates the fade layer right before drawing it
            pattern = hook::pattern("8B 8E F0 02 00 00 D9 1C 24 E8");
            static auto MaxPayne_HUDFadeLayerHook = safetyhook::create_mid(pattern.get_first(9), [](SafetyHookContext& regs) //0x4889F0
            {
                if (regs.ecx)
                    MaxPayne_HUDFadeLayer::pSprite = *(uint8_t**)(regs.ecx + 0xAD);
            });

            // Graphic novels: key toggles between the original framing and the whole page as large as the screen allows
            pattern = hook::pattern("56 57 8B F1 E8 ? ? ? ? 8B 7C 24 0C 8D 46 37 50 8D 4F 38 E8"); // MaxPayne_GraphicNovelPage::show
            MaxPayne_GraphicNovelPage::shShow = safetyhook::create_inline(pattern.get_first(), MaxPayne_GraphicNovelPage::show); //0x4862D0

            // Graphic novel cursor bounds, see UpdateCursorBounds
            pattern = hook::pattern("52 68 00 00 20 44 E8 ? ? ? ? 51 D9 1C 24 6A 00 E8 ? ? ? ? D9 5C 24 ? 8B 44 24 ? 50 68 00 00 F0 43 E8 ? ? ? ? 51 D9 1C 24 6A 00 E8"); // MP_GraphicNovelMode::update
            injector::MakeCALL(pattern.get_first(6), ClampCursorRight, true); //0x484FB5
            injector::MakeCALL(pattern.get_first(17), ClampCursorLeft, true);
            injector::MakeCALL(pattern.get_first(36), ClampCursorBottom, true);
            injector::MakeCALL(pattern.get_first(47), ClampCursorTop, true);

            // Post-processing (pain and bullet time): the scene is warped with a grid covering the render target
            pattern = hook::pattern("8B 8E 9C 00 00 00 52 68 ? ? ? ? E8");
            static auto PostProcessWarpHook = safetyhook::create_mid(pattern.get_first(12), [](SafetyHookContext& regs) //0x47AB87
            {
                KeepOriginalProjection(*(void**)(regs.ecx + 0x10));
            });
        };
    }
} GameMode;
