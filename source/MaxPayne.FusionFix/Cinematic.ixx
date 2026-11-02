module;

#include <stdafx.h>

export module Cinematic;

import ComVars;
import e2mfc;
import Misc;

// Set when MaxPayne_GameMode renders the game view, cleared at the end of each frame
bool bGameViewRendered = false;

export namespace Cinematic
{
    enum eCutsceneBorders
    {
        Off,
        Letterbox,
        Pillarbox,
        Both,
    };

    int32_t nCutsceneBorders = Both;
    bool bNoBorderAnimation = false;
    constexpr float fBorderAnimationTime = 0.35f;

    struct State
    {
        float fProgress = 0.0f;            // how far the game's own widescreen transition is, 0 = off, 1 = on
        float fWideScreenMultiplier = 1.0f; // X_GlobalCinematicSettings::WideScreenMultiplier, share of the 4:3 height left by the bars
    };

    State GetState(uint8_t* pGameMode)
    {
        State state;
        if (!pGameMode)
            return state;

        auto pSettings = *(uint8_t**)(pGameMode + MaxPayne_GameMode::GLOBAL_CINEMATIC_SETTINGS);
        if (pSettings)
            state.fWideScreenMultiplier = *(float*)(pSettings + 0x0C);

        if (state.fWideScreenMultiplier > 0.0f && state.fWideScreenMultiplier < 1.0f)
        {
            float fCurrent = *(float*)(pGameMode + MaxPayne_GameMode::CURRENT_HEIGHT_MULTIPLIER);
            state.fProgress = std::clamp((1.0f - fCurrent) / (1.0f - state.fWideScreenMultiplier), 0.0f, 1.0f);
            if (bNoBorderAnimation && state.fProgress > 0.0f)
                state.fProgress = 1.0f;
        }
        return state;
    }

    // The vanilla cutscene frame is the 4:3 view cut down to WideScreenMultiplier of its height, 16:9
    // for the default 0.75. Cutscenes always show exactly that frame, like in the GTA III widescreen
    // fix: across the full width on screens narrower than it (letterbox), across the full height on
    // wider ones (pillarbox). nCutsceneBorders only decides which of the bars around it get drawn.
    float GetFrameAspectRatio(float fWideScreenMultiplier)
    {
        return (4.0f / 3.0f) / fWideScreenMultiplier;
    }

    // Horizontal tangent multiplier, relative to the 4:3 FOV, that shows exactly the vanilla frame
    float GetCutsceneZoom(float fWideScreenMultiplier)
    {
        return std::max(1.0f, Screen.fAspectRatio / GetFrameAspectRatio(fWideScreenMultiplier));
    }

    struct Borders
    {
        float fLetterbox = 0.0f; // share of the screen height covered by the top and bottom bars
        float fPillarbox = 0.0f; // share of the screen width covered by the left and right bars
    };

    Borders GetBorders(float fWideScreenMultiplier)
    {
        Borders borders;
        float fScreenToFrame = Screen.fAspectRatio / GetFrameAspectRatio(fWideScreenMultiplier);
        if (fScreenToFrame < 1.0f)
        {
            if (nCutsceneBorders == Letterbox || nCutsceneBorders == Both)
                borders.fLetterbox = 1.0f - fScreenToFrame;
        }
        else if (nCutsceneBorders == Pillarbox || nCutsceneBorders == Both)
        {
            borders.fPillarbox = 1.0f - 1.0f / fScreenToFrame;
        }
        return borders;
    }

    // Borders slide in and out like in the GTA III Fusion Fix, following the game's own
    // transition when a script gives it a fade time.
    float fBordersShown = 0.0f;
    float fBordersWideScreenMultiplier = 0.75f; // of the last cutscene, kept while its borders slide out
    Borders CurrentBorders;

    // Called every frame the game view renders, before MaxPayne_HUDMode draws the letterbox bars
    void UpdateBorders(uint8_t* pGameMode)
    {
        auto state = GetState(pGameMode);
        if (state.fProgress > 0.0f)
            fBordersWideScreenMultiplier = state.fWideScreenMultiplier;

        using clock = std::chrono::steady_clock;
        static clock::time_point lastUpdate = clock::now();
        auto now = clock::now();
        float dt = std::min(std::chrono::duration<float>(now - lastUpdate).count(), 0.1f);
        lastUpdate = now;

        if (bNoBorderAnimation)
            fBordersShown = state.fProgress;
        else
        {
            float fStep = dt / fBorderAnimationTime;
            fBordersShown += std::clamp(state.fProgress - fBordersShown, -fStep, fStep);
        }

        auto borders = GetBorders(fBordersWideScreenMultiplier);
        CurrentBorders.fLetterbox = borders.fLetterbox * fBordersShown;
        CurrentBorders.fPillarbox = borders.fPillarbox * fBordersShown;
    }

    // Replaces MaxPayne_GameInformation::getCurrentHeightMultiplier in MaxPayne_HUDMode's render
    // function, which draws the letterbox bars and moves subtitles clear of them
    float __fastcall GetBordersHeightMultiplier(void* pGameInformation, void* edx)
    {
        return 1.0f - CurrentBorders.fLetterbox;
    }

    // At the end of the frame, like the bars the game draws, so they cover everything but the frame
    void DrawPillarboxBorders()
    {
        auto nWidth = static_cast<int32_t>(std::lround(Screen.fWidth * CurrentBorders.fPillarbox * 0.5f));
        if (nWidth > 0)
            DrawPillarboxBars(nWidth, nWidth);
    }

    // End of the frame, before P_Driver::endScene: the bars the fix adds, drawn with P_Driver::clearScreen the same
    // way the game draws its letterbox bars
    void DrawFrameBorders()
    {
        if (bGameViewRendered)
            DrawPillarboxBorders();

        if (Screen.bDrawBordersForCameraOverlay && CurrentGameMode != "graphicnovel")
        {
            auto nWidth = static_cast<int32_t>(Screen.fHudOffsetReal);
            constexpr int32_t nBadCamPosOffset = 10; // for motel camera gap https://i.imgur.com/JGNdm6y.jpg
            DrawPillarboxBars(nWidth, nWidth + nBadCamPosOffset);
        }

        if (Screen.bDrawBordersToFillGap)
        {
            // hiding top/left 1px gap
            ClearScreenRect(0, 0, Screen.nWidth, 1);
            ClearScreenRect(0, 0, 1, Screen.nHeight);
            Screen.bDrawBordersToFillGap = false;
        }

        bGameViewRendered = false;
    }
}

class CinematicBorders
{
public:
    CinematicBorders()
    {
        WFP::onInitEvent() += []()
        {
            auto pattern = hook::pattern("8B 8E 74 10 00 00 33 ED 89 6C 24"); // MaxPayne_GameMode::renderMode
            static auto MaxPayne_GameModerenderModeHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                MaxPayne_GameMode::pInstance = (uint8_t*)regs.esi;
                bGameViewRendered = true;
                Cinematic::UpdateBorders(MaxPayne_GameMode::pInstance);
                AdaptiveDifficulty::Track(MaxPayne_GameMode::pInstance);
            });

            // Cutscene borders: every height multiplier read in MaxPayne_HUDMode's render function, which moves
            // subtitles clear of the letterbox bars and then draws them
            pattern = hook::pattern("8B 8E 97 00 00 00 C7 44 24 ? 00 00 00 00 E8"); // subtitle offset check
            injector::MakeCALL(pattern.get_first(14), Cinematic::GetBordersHeightMultiplier, true); //0x4AF943
            pattern = hook::pattern("8B 8E 97 00 00 00 E8 ? ? ? ? D8 2D ? ? ? ? 8B 8E 94 01 00 00"); // subtitle offset
            injector::MakeCALL(pattern.get_first(6), Cinematic::GetBordersHeightMultiplier, true); //0x4AF95B
            pattern = hook::pattern("8B 8E 97 00 00 00 E8 ? ? ? ? D8 1D ? ? ? ? DF E0 F6 C4 01 74"); // subtitle offset restore check
            injector::MakeCALL(pattern.get_first(6), Cinematic::GetBordersHeightMultiplier, true); //0x4AF9F0
            pattern = hook::pattern("8B 8E 97 00 00 00 E8 ? ? ? ? D8 1D ? ? ? ? 5F DF E0"); // bars check
            injector::MakeCALL(pattern.get_first(6), Cinematic::GetBordersHeightMultiplier, true); //0x4AFA4A
            pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? ? ? ? ? 8B F0 89 5C 24"); // bar height
            injector::MakeCALL(pattern.get_first(), Cinematic::GetBordersHeightMultiplier, true); //0x4AFAA2

            pattern = hook::pattern("C6 87 ? ? ? ? ? E8 ? ? ? ? 8B 4D F4");
            struct CameraOverlayHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    Screen.bDrawBordersForCameraOverlay = false;
                    *(uint8_t*)(regs.edi + 0x14E) = 1;

                    auto a1 = *(uint32_t*)(regs.esp + 0x10);
                    auto a2 = *(uint32_t*)(regs.esp + 0x14);
                    auto a3 = *(uint32_t*)(regs.esp + 0x18);
                    auto a4 = *(uint32_t*)(regs.esp + 0x1C);

                    //what happens here is check for some camera coordinates or angles
                    if ((a1 == 0x3FE842CF && a4 == 0x3FE842CF) ||                                           //1.81 https://i.imgur.com/A7wRrgk.gifv
                        (a1 == 0x3FC00000 && a2 == 0x4096BEF4 && a3 == 0xC003936E && a4 == 0x3FC00000) ||   //1.5 https://i.imgur.com/ouRpysL.jpg
                        (a1 == 0xBFAAE30E && a2 == 0xBFC2B1AA && a3 == 0x3EC2E382 && a4 == 0xBFAAE30E) ||   //-1.33505 https://i.imgur.com/JGNdm6y.jpg
                        (a1 == 0x403F7470 && a2 == 0xC067ED50 && a3 == 0x40424DE0 && a4 == 0x403F7470)      // 2.99148  https://i.imgur.com/hj5FsXp.png
                        )
                    {
                        Screen.bDrawBordersForCameraOverlay = true;
                    }
                }
            }; injector::MakeInline<CameraOverlayHook>(pattern.get_first(0), pattern.get_first(7)); // 0x672EB1
        };
    }
} CinematicBorders;
