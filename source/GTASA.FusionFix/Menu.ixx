module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/RefreshRatePicker.hpp"
#include <mmsystem.h>
#include <commctrl.h>
#pragma comment(lib, "winmm.lib") // needed for timeBeginPeriod()/timeEndPeriod()

export module Menu;

import Skeleton;
import Options;

export enum eMenuScreen : int8_t
{
    SCREEN_GO_BACK = -2,
    SCREEN_NONE = -1,
    SCREEN_STATS = 0,
    SCREEN_NOP = 0,
    SCREEN_START_GAME,                     //  1  New Game, Load Game, Delete Game
    SCREEN_BRIEF,                          //  2
    SCREEN_AUDIO_SETTINGS,                 //  3
    SCREEN_DISPLAY_SETTINGS,               //  4
    SCREEN_MAP,                            //  5
    SCREEN_NEW_GAME_ASK,                   //  6  Are you sure you want to start a new game? All current game progress will be lost. Proceed?
    SCREEN_SELECT_GAME,                    //  7  Please select which new game you wish to start:
    SCREEN_MISSION_PACK_LOADING_ASK,       //  8  Are you sure you want to load a San Andreas Mission Pack? All current game progress will be lost. Proceed?
    SCREEN_LOAD_GAME,                      //  9  Select save file to load:
    SCREEN_DELETE_GAME,                    // 10  Select save file to delete:
    SCREEN_LOAD_GAME_ASK,                  // 11  All unsaved progress in your current game will be lost. Proceed with loading?
    SCREEN_DELETE_GAME_ASK,                // 12  Are you sure you wish to delete this save file?
    SCREEN_LOAD_FIRST_SAVE,                // 13
    SCREEN_DELETE_FINISHED,                // 14
    SCREEN_DELETE_SUCCESSFUL,              // 15  Delete Successful. Select OK to continue.
    SCREEN_GAME_SAVE,                      // 16  Select file you wish to save to:
    SCREEN_SAVE_WRITE_ASK,                 // 17  Are you sure you wish to save?
    SCREEN_SAVE_DONE_1,                    // 18
    SCREEN_SAVE_DONE_2,                    // 19  Save Successful. Select OK to continue.
    SCREEN_GAME_SAVED,                     // 20  OK
    SCREEN_GAME_LOADED,                    // 21  OK
    SCREEN_GAME_WARNING_DONT_SAVE,         // 22  Warning! One or more cheats have been activated. This may affect your save game. It is recommended that you do not save this game.
    SCREEN_ASK_DISPLAY_DEFAULT_SETS,       // 23  Are you sure you want to reset your current settings to default?
    SCREEN_ASK_AUDIO_DEFAULT_SETS,         // 24  Are you sure you want to reset your current settings to default?
    SCREEN_CONTROLS_RESET,                 // 25  Are you sure you want to reset your current settings to default?
    SCREEN_USER_TRACKS_OPTIONS,            // 26
    SCREEN_DISPLAY_ADVANCED,               // 27  DRAW DISTANCE, ...
    SCREEN_LANGUAGE,                       // 28  English, ...
    SCREEN_SAVE_GAME_DONE,                 // 29  O.K
    SCREEN_SAVE_GAME_FAILED,               // 30  Save Unsuccessful., O.K.
    SCREEN_SAVE_WRITE_FAILED,              // 31  Save Unsuccessful.
    SCREEN_SAVE_FAILED_FILE_ERROR,         // 32  Load Unsuccessful. File Corrupted, Please delete.
    SCREEN_OPTIONS,                        // 33  Controller Setup, Audio Setup ...
    SCREEN_MAIN_MENU,                      // 34  Start Game, Options, Quit Game
    SCREEN_QUIT_GAME_ASK,                  // 35  Are you sure you want to quit? All progress since the last save game will be lost. Proceed?
    SCREEN_CONTROLLER_SETUP,               // 36  CONFIGURATION, Redefine Controls ...
    SCREEN_REDEFINE_CONTROLS,              // 37  Foot Controls, Vehicle Controls
    SCREEN_CONTROLS_DEFINITION,            // 38  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    SCREEN_MOUSE_SETTINGS,                 // 39
    SCREEN_JOYPAD_SETTINGS,                // 40
    SCREEN_PAUSE_MENU,                     // 41  Resume, START NEW GAME ...
    SCREEN_INITIAL,                        // 42  SCREEN_QUIT_GAME_2
    SCREEN_EMPTY,                          // 43

    SCREEN_COUNT,                          // Screen count
};

export enum CutsceneBordersMode : int8_t
{
    Off = 0,
    Letterbox = 1,
    Pillarbox = 2,
    Both = 3
};

export enum FrameLimiterMode : int8_t
{
    eOff, e30, e40, e50, e60, e75, e100, e120, e144, e165, e200, e240, e90, e180
};

// The Frame Limiter row's steps and their frame rates
static constexpr std::pair<int8_t, int> FrameLimits[] = { { FrameLimiterMode::eOff, 0 }, { FrameLimiterMode::e30, 30 },
    { FrameLimiterMode::e40, 40 }, { FrameLimiterMode::e50, 50 }, { FrameLimiterMode::e60, 60 }, { FrameLimiterMode::e75, 75 },
    { FrameLimiterMode::e90, 90 }, { FrameLimiterMode::e100, 100 }, { FrameLimiterMode::e120, 120 }, { FrameLimiterMode::e144, 144 },
    { FrameLimiterMode::e165, 165 }, { FrameLimiterMode::e180, 180 }, { FrameLimiterMode::e200, 200 }, { FrameLimiterMode::e240, 240 } };

static int FrameLimitOfStep(int8_t step)
{
    for (auto& [mode, fps] : FrameLimits)
    {
        if (mode == step)
            return fps;
    }
    return 0;
}

// The step of a frame rate: the nearest one (MAIN.FPSLimit may hold any rate)
static int8_t StepOfFrameLimit(int fps)
{
    if (fps <= 0)
        return FrameLimiterMode::eOff;
    int8_t step = FrameLimiterMode::e30;
    for (auto& [mode, rate] : FrameLimits)
    {
        if (rate && std::abs(rate - fps) < std::abs(FrameLimitOfStep(step) - fps))
            step = mode;
    }
    return step;
}

static int8_t NextFrameLimiter(int8_t current, int direction)
{
    static constexpr std::array<int8_t, 14> modes{ FrameLimiterMode::eOff, FrameLimiterMode::e30,
        FrameLimiterMode::e40, FrameLimiterMode::e50, FrameLimiterMode::e60, FrameLimiterMode::e75,
        FrameLimiterMode::e90, FrameLimiterMode::e100, FrameLimiterMode::e120, FrameLimiterMode::e144,
        FrameLimiterMode::e165, FrameLimiterMode::e180, FrameLimiterMode::e200, FrameLimiterMode::e240 };
    const auto it = std::find(modes.begin(), modes.end(), current);
    if (it == modes.end())
        return FrameLimiterMode::eOff;
    const auto index = static_cast<int>(it - modes.begin());
    return modes[(index + static_cast<int>(modes.size()) + direction) % static_cast<int>(modes.size())];
}

class CSprite2d
{
public:
    RwTexture* m_pTexture;
};

enum eRadarMode : int32_t
{
    RADAR_MODE_MAPS_AND_BLIPS = 0,
    RADAR_MODE_BLIPS_ONLY = 1,
    RADAR_MODE_OFF = 2,

    RADAR_MODE_COUNT
};

export class CMenuManager
{
    enum
    {
        MPACK_COUNT = 25,
        SPRITE_COUNT = 25,
    };

public:
    int8_t      m_nStatsScrollDirection;
    float     m_fStatsScrollSpeed;
    uint8_t     m_nSelectedRow;
    int32_t     m_CurrentGallerySelected;
    int32_t     m_CurrentMaxGalleryImages;
    int32_t     m_CurrentMaxRealPhotos;
    int32_t     m_CurrentSelectedRealPhoto;
    uint32_t    m_ReloadImageTime;
    bool      m_PrefsUseVibration;
    bool      m_bHudOn;
    eRadarMode m_nRadarMode;
    int32_t     field_28;
    int32_t     m_nTargetBlipIndex;
    int8_t      m_nSysMenu;
    bool      m_DisplayControllerOnFoot;
    bool      m_bDontDrawFrontEnd;
    bool      m_bActivateMenuNextFrame;
    bool      m_bMenuAccessWidescreen;
    bool      field_35;
    int32_t  m_KeyPressedCode;
    int32_t     m_PrefsBrightness;
    float     m_fDrawDistance;

    bool      m_bShowSubtitles;
    union
    {
        struct
        {
            bool m_ShowLocationsBlips;
            bool m_ShowContactsBlips;
            bool m_ShowMissionBlips;
            bool m_ShowOtherBlips;
            bool m_ShowGangAreaBlips;
        };
        bool m_abPrefsMapBlips[5];
    };
    bool      m_bMapLegend;
    int8_t      m_bWidescreenOn;
    int8_t      m_bPrefsFrameLimiter;
    bool      m_bRadioAutoSelect;
    bool      m_PrefsAudioOutputMode;
    int8_t      m_nSfxVolume;
    int8_t      m_nRadioVolume;
    bool      m_bRadioEq;

    int8_t  m_nRadioStation;
    bool      m_RecheckNumPhotos;
    int32_t     m_nCurrentScreenItem;
    bool      m_bQuitGameNoDVD;

    bool      m_bDrawingMap;
    bool      m_bStreamingDisabled;
    bool      m_bAllStreamingStuffLoaded;

    bool      m_bMenuActive;
    bool      m_bStartGameLoading;
    int8_t      m_nGameState;
    bool      m_bIsSaveDone;
    bool      m_bLoadingData;
    float     m_fMapZoom;
    CVector2D m_vMapOrigin;
    CVector2D m_vMousePos;
    bool      m_bMapLoaded;

    int32_t     m_nTitleLanguage;
    int32_t     m_nTextLanguage;
    uint8_t m_nPrefsLanguage;
    uint8_t m_nPreviousLanguage;
    int32_t     m_SystemLanguage;
    bool      m_LoadedLanguage;
    int32_t     m_ListSelection;
    int32_t     m_RenderScreenOnce;
    uint8_t* m_GalleryImgBuffer;
    RwRaster* m_GpJpgTex;
    bool      m_StartUpFrontEndRequestedForPads;
    int32_t     m_ScreenXOffset;
    int32_t     m_ScreenYOffset;
    uint32_t    m_UserTrackIndex;
    int8_t m_RadioMode;

    bool      m_bInvertPadX1;
    bool      m_bInvertPadY1;
    bool      m_bInvertPadX2;
    bool      m_bInvertPadY2;
    bool      m_bSwapPadAxis1;
    bool      m_bSwapPadAxis2;

    uint8_t   m_RedefiningControls;
    bool      m_DisplayTheMouse;
    int32_t     m_nMousePosX;
    int32_t     m_nMousePosY;
    bool      m_bPrefsMipMapping;
    bool      m_bTracksAutoScan;
    int32_t     m_nPrefsAntialiasing;
    int32_t     m_nDisplayAntialiasing;
    int8_t m_ControlMethod;
    int32_t     m_nPrefsVideoMode;
    int32_t     m_nDisplayVideoMode;
    int32_t     m_nCurrentRwSubsystem;

    int32_t     m_nMousePosWinX;
    int32_t     m_nMousePosWinY;

    bool      m_bSavePhotos;
    bool      m_bMainMenuSwitch;
    uint8_t     m_nPlayerNumber;
    bool      m_bLanguageChanged;
    int32_t     field_EC;
    int32_t* m_pPressedKey;
    bool      m_isPreInitialised;
    ///...

    eMenuScreen GetCurrentScreen() const
    {
        // The sprites following the partial layout end at the current screen.
        return *reinterpret_cast<const eMenuScreen*>(reinterpret_cast<const uint8_t*>(this) + 0x15D);
    }
};

export GameRef<CMenuManager> FrontendMenuManager([]() -> CMenuManager*
{
    auto pattern = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 6A 01 E8 ? ? ? ? 83 C4 04 E8 ? ? ? ? C6 46 ? ? A1 ? ? ? ? 83 C5 14 47 83 C3 2C");
    return *pattern.get_first<CMenuManager*>(1);
});

// The gameplay frame rate limit (MAIN.FPSLimit, 0: none)
static int frameLimit = 60;
static int FrameLimit() { return frameLimit; }

namespace CText
{
    void* TheText = nullptr;
    const char* (__fastcall* Get)(void*, void*, const char*) = nullptr;

    std::string BordersLabel, BordersValue, LimiterValue;

    const char* GetCutsceneBordersLabel()
    {
        BordersLabel = FixOptions::GameText("cutsceneborders");
        return BordersLabel.c_str();
    }

    const char* GetCutsceneBordersText()
    {
        const char* key = nullptr;
        switch (FrontendMenuManager->m_bWidescreenOn)
        {
            case CutsceneBordersMode::Off:
                return Get(TheText, 0, "FEM_OFF");
            case CutsceneBordersMode::Letterbox:
                key = "cutsceneborders.letterbox";
                break;
            case CutsceneBordersMode::Pillarbox:
                key = "cutsceneborders.pillarbox";
                break;
            case CutsceneBordersMode::Both:
                key = "cutsceneborders.both";
                break;
            default:
                return Get(TheText, 0, "FEM_ON");
        };
        BordersValue = FixOptions::GameText(key, true);
        return BordersValue.c_str();
    }

    const char* GetFrameLimiterText()
    {
        int fps = FrameLimit();
        if (!fps)
            return Get(TheText, 0, "FEM_OFF");
        LimiterValue = std::to_string(fps) + " FPS";
        return LimiterValue.c_str();
    }
}

// Cutscene Borders and the Frame Limiter are the game's rows of Display Setup and Advanced, saved in gta_sa.set, and
// the fix's MAIN.CutsceneBorders and MAIN.FPSLimit (FusionFixHub, presets): the .cfg decides at the start, and a change
// on either side is written to the other
namespace NativeRows
{
    int8_t SyncedBorders = -1;
    int8_t SyncedLimiter = -1;

    void ApplyBorders()
    {
        const int borders = ReadFixSetting("MAIN.CutsceneBorders", 3);
        FrontendMenuManager->m_bWidescreenOn = int8_t(std::clamp(borders, 0, int(CutsceneBordersMode::Both)));
        SyncedBorders = FrontendMenuManager->m_bWidescreenOn;
    }

    void ApplyLimiter()
    {
        frameLimit = std::max(0, ReadFixSetting("MAIN.FPSLimit", 60));
        FrontendMenuManager->m_bPrefsFrameLimiter = StepOfFrameLimit(FrameLimit());
        SyncedLimiter = FrontendMenuManager->m_bPrefsFrameLimiter;
    }

    // The game's rows changed them (or its Restore Defaults)
    void Update()
    {
        if (FrontendMenuManager->m_bWidescreenOn != SyncedBorders)
        {
            SyncedBorders = FrontendMenuManager->m_bWidescreenOn;
            FixSettings().Set("MAIN.CutsceneBorders", std::to_string(SyncedBorders));
        }
        if (FrontendMenuManager->m_bPrefsFrameLimiter != SyncedLimiter)
        {
            SyncedLimiter = FrontendMenuManager->m_bPrefsFrameLimiter;
            frameLimit = FrameLimitOfStep(SyncedLimiter);
            FixSettings().Set("MAIN.FPSLimit", std::to_string(frameLimit));
        }
    }
}

enum RsEvent
{
    rsCAMERASIZE = 0,
    rsCOMMANDLINE = 1,
    rsFILELOAD = 2,
    rsINITDEBUG = 3,
    rsINPUTDEVICEATTACH = 4,
    rsLEFTBUTTONDOWN = 5,
    rsLEFTBUTTONUP = 6,
    rsMOUSEMOVE = 7,
    rsMOUSEWHEELMOVE = 8,
    rsPLUGINATTACH = 9,
    rsREGISTERIMAGELOADER = 10,

    rsRIGHTBUTTONDOWN = 11,
    rsRIGHTBUTTONUP = 12,
    rsMIDDLEBUTTONDOWN = 13,
    rsMIDDLEBUTTONUP = 14,
    rsMOUSEWHEELMOVEDUP = 15,
    rsMOUSEWHEELMOVEDDOWN = 16,
    rsFIRST_XBUTTONUP = 17,
    rsSECOND_XBUTTONUP = 18,
    rsFIRST_XBUTTONDOWN = 19,
    rsSECOND_XBUTTONDOWN = 20,

    rsRWINITIALIZE = 21,
    rsRWTERMINATE = 22,
    rsSELECTDEVICE = 23,
    rsINITIALIZE = 24,
    rsTERMINATE = 25,
    rsIDLE = 26,
    rsFRONTENDIDLE = 27,
    rsKEYDOWN = 28,
    rsKEYUP = 29,
    rsQUITAPP = 30,

    rsPADBUTTONDOWN = 31,
    rsPADBUTTONUP = 32,
    rsPADANALOGUELEFT = 33,
    rsPADANALOGUELEFTRESET = 34,
    rsPADANALOGUERIGHT = 35,
    rsPADANALOGUERIGHTRESET = 36,
    rsPREINITCOMMANDLINE = 37,
    rsACTIVATE = 38,
    rsSETMEMORYFUNCS = 39,
};

class FrameLimiter
{
public:
    enum FPSLimitMode
    {
        FPS_NONE,
        FPS_REALTIME,
        FPS_ACCURATE
    };

    FPSLimitMode mFPSLimitMode = FPS_NONE;

private:
    double TIME_Frequency = 0.0;
    double TIME_Ticks = 0.0;
    double TIME_Frametime = 0.0;
    float fFPSLimit = 0.0f;
    bool bFpsLimitWasUpdated = false;

public:
    void Init(FPSLimitMode mode, float fps_limit)
    {
        bFpsLimitWasUpdated = true;
        mFPSLimitMode = mode;
        fFPSLimit = fps_limit;

        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);

        static constexpr auto TICKS_PER_FRAME = 1;
        auto TICKS_PER_SECOND = (TICKS_PER_FRAME * fFPSLimit);

        if (mFPSLimitMode == FPS_ACCURATE)
        {
            TIME_Frametime = 1000.0 / (double)fFPSLimit;
            TIME_Frequency = (double)frequency.QuadPart / 1000.0;
        }
        else
        {
            TIME_Frequency = (double)frequency.QuadPart / (double)TICKS_PER_SECOND;
        }

        Ticks();
    }

    DWORD Sync_RT()
    {
        if (bFpsLimitWasUpdated)
        {
            bFpsLimitWasUpdated = false;
            return 1;
        }

        DWORD lastTicks, currentTicks;
        LARGE_INTEGER counter;

        QueryPerformanceCounter(&counter);
        lastTicks = (DWORD)TIME_Ticks;
        TIME_Ticks = (double)counter.QuadPart / TIME_Frequency;
        currentTicks = (DWORD)TIME_Ticks;

        return (currentTicks > lastTicks) ? currentTicks - lastTicks : 0;
    }

    DWORD Sync_SLP()
    {
        if (bFpsLimitWasUpdated)
        {
            bFpsLimitWasUpdated = false;
            return 1;
        }

        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        double millis_current = (double)counter.QuadPart / TIME_Frequency;
        double millis_delta = millis_current - TIME_Ticks;

        if (TIME_Frametime <= millis_delta)
        {
            TIME_Ticks = millis_current;
            return 1;
        }
        else if (TIME_Frametime - millis_delta > 2.0)
            Sleep(1);
        else
            Sleep(0);

        return 0;
    }

    void Sync()
    {
        if (fFPSLimit <= 0.0f)
            return;

        if (mFPSLimitMode == FPS_REALTIME)
            while (!Sync_RT());
        else if (mFPSLimitMode == FPS_ACCURATE)
            while (!Sync_SLP());
    }

private:
    void Ticks()
    {
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        TIME_Ticks = (double)counter.QuadPart / TIME_Frequency;
    }
};

static float GetFrameLimiterValue()
{
    return float(FrameLimit());
}

// Keep menu pacing independent of the gameplay limiter's timestamps and setting.
static FrameLimiter menuFpsLimiter;
static float menuFrameLimit = 60.0f;
static bool menuFrameLimitChanged = false;

static void LimitMenuFrame()
{
    if (menuFrameLimit <= 0.0f)
        return;

    static bool initialized = false;
    if (!initialized || menuFrameLimitChanged)
    {
        if (!initialized)
            timeBeginPeriod(1);
        menuFpsLimiter.Init(FrameLimiter::FPS_ACCURATE, menuFrameLimit);
        initialized = true;
        menuFrameLimitChanged = false;
    }
    menuFpsLimiter.Sync();
}

injector::hook_back<bool(__cdecl*)(RsEvent event, void* data)> hbRsEventHandler;
bool __cdecl RsEventHandler(RsEvent event, void* data)
{
    if (FrontendMenuManager->m_bMenuActive)
        return hbRsEventHandler.fun(event, data);

    static FrameLimiter fpsLimiter;
    static constexpr auto mode = FrameLimiter::FPS_ACCURATE;
    static bool bTimerResolutionSet = false;
    static float fLastFpsLimit = -1.0f;

    float fFpsLimit = GetFrameLimiterValue();

    if (fFpsLimit != fLastFpsLimit)
    {
        fLastFpsLimit = fFpsLimit;

        if (!bTimerResolutionSet)
        {
            timeBeginPeriod(1);
            bTimerResolutionSet = true;
        }

        RsGlobal->maxFPS = static_cast<int32_t>(fFpsLimit);
        fpsLimiter.Init(mode, fFpsLimit);
    }

    fpsLimiter.Sync();

    return hbRsEventHandler.fun(event, data);
}

static SafetyHookInline DrawMenuHook;

static void __fastcall DrawMenuWithIndicators(uint8_t* manager, void*)
{
    FixOptions::BeginMenuDraw();
    DrawMenuHook.unsafe_thiscall<void>(manager);
    FixOptions::DrawScrollIndicators();
}

// MAIN.PauseOnFocusLoss: the game pauses through its own activation messages, which a window wrapper can take
// away. The game's menu opens on the first game tick after the window lost the foreground.
namespace PauseOnFocusLoss
{
    int32_t* GameState = nullptr;               // gGameState: 9 = playing
    bool Requested = false;

    LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR subclass, DWORD_PTR)
    {
        if ((message == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE) || (message == WM_ACTIVATEAPP && !wParam))
            Requested = true;
        else if (message == WM_NCDESTROY)
        {
            Requested = false;
            RemoveWindowSubclass(window, WindowProc, subclass);
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }

    void Install()
    {
        // The first member of RenderWare's Windows platform data is the game window
        const auto window = *static_cast<const HWND*>(RsGlobal->ps);
        if (!SetWindowSubclass(window, WindowProc, 0, 0))
            return;

        WFP::onGameProcessEvent() += [window]()
        {
            if (!std::exchange(Requested, false) || GetForegroundWindow() == GetAncestor(window, GA_ROOT) ||
                *GameState != 9 || FrontendMenuManager->m_bMenuActive || FrontendMenuManager->m_bActivateMenuNextFrame)
                return;

            FrontendMenuManager->m_bDontDrawFrontEnd = false;
            FrontendMenuManager->m_bActivateMenuNextFrame = true;
        };
    }
}

class Menu
{
public:
    Menu()
    {
        WFP::onInitEvent() += []()
        {
            RWGTA::InstallRefreshRatePicker<true>("3B C5 73 07 83 F8 3C 72 02 8B E8");
            PauseOnFocusLoss::GameState = *hook::pattern("A1 ? ? ? ? 83 F8 09 0F 87 ? ? ? ? FF 24 85 ? ? ? ? ? ? E8").get_first<int32_t*>(1);

            static auto fading = *hook::pattern("A0 ? ? ? ? 83 C4 08 84 C0 75 ? A0 ? ? ? ? 84 C0 0F 85 ? ? ? ? 8D 44 24 00").get_first<bool*>(1);
            auto renderSplash = hook::pattern("83 EC 18 E8 ? ? ? ? A1 ? ? ? ? 8B 0D ? ? ? ? 83 C0 05");
            static auto paceSplash = safetyhook::create_mid(renderSplash.get_first(), [](SafetyHookContext&)
            {
                if (*fading)
                {
                    static FrameLimiter limiter;
                    static std::once_flag initialized;
                    std::call_once(initialized, []() { limiter.Init(FrameLimiter::FPS_ACCURATE, 60.0f); });
                    limiter.Sync();
                }
            });
        };

        WFP::onGameInitEvent() += []()
        {
            // The game has read gta_sa.set: the .cfg's values replace its two
            NativeRows::ApplyBorders();
            NativeRows::ApplyLimiter();
            OnSettingChange("MAIN.CutsceneBorders", NativeRows::ApplyBorders);
            OnSettingChange("MAIN.FPSLimit", NativeRows::ApplyLimiter);
            WFP::onGameProcessEvent() += NativeRows::Update;
            if (FixSettings().Bool("MAIN.PauseOnFocusLoss"))
                PauseOnFocusLoss::Install();

            menuFrameLimit = static_cast<float>(std::max(0, ReadFixSetting("MAIN.FPSLimitMenus", 60)));
            OnSettingChange("MAIN.FPSLimitMenus", []()
            {
                menuFrameLimit = static_cast<float>(std::max(0, ReadFixSetting("MAIN.FPSLimitMenus", 60)));
                menuFrameLimitChanged = true;
            });

            auto pattern = hook::pattern("B9 ? ? ? ? ? ? ? E8 ? ? ? ? 50 E8 ? ? ? ? 83 C4 ? 8B 46");
            CText::TheText = *pattern.get_first<void*>(1);
            CText::Get = (decltype(CText::Get))injector::GetBranchDestination(pattern.get_first(8)).as_int();

            pattern = hook::pattern("8B 4C 24 ? 8B 54 24 ? 53 51 52 E8");
            static auto GetMenuOptionText = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto ptr = (char*)(regs.ebx);
                if (ptr)
                {
                    if (std::string_view(ptr) == CText::Get(CText::TheText, 0, "FED_WIS"))
                    {
                        *(char**)&regs.ebx = const_cast<char*>(CText::GetCutsceneBordersLabel());
                    }
                }
            });

            static int cachedAction = -1;
            pattern = hook::pattern("83 C0 E8 83 F8 29");
            static auto GetCurrentAction = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                cachedAction = regs.eax;
            });

            pattern = hook::pattern("85 DB BF C0 01 00 00");
            static auto GetMenuOnOffText = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                // The values of the fix's own rows (Options.ixx)
                if (auto text = FixOptions::RightText(cachedAction))
                {
                    regs.esi = reinterpret_cast<uintptr_t>(text);
                    return;
                }

                auto ptr = (char*)(regs.esi);
                if (ptr)
                {
                    if (cachedAction == 0x18)
                    {
                        if (std::string_view(ptr) == CText::Get(CText::TheText, 0, "FEM_ON") || std::string_view(ptr) == CText::Get(CText::TheText, 0, "FEM_OFF"))
                        {
                            *(char**)&regs.esi = const_cast<char*>(CText::GetFrameLimiterText());
                        }
                    }
                    else if (cachedAction == 0x1A)
                    {
                        if (std::string_view(ptr) == CText::Get(CText::TheText, 0, "FEM_ON") || std::string_view(ptr) == CText::Get(CText::TheText, 0, "FEM_OFF"))
                        {
                            *(char**)&regs.esi = const_cast<char*>(CText::GetCutsceneBordersText());
                        }
                    }
                }
            });

            pattern = hook::pattern("8A 46 ? 84 C0 0F 94 C2 8B CE");
            injector::MakeNOP(pattern.get_first(), 13, true);
            static auto ProcessOnOffMenuOptionsHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                regs.ecx = regs.esi;
                auto& pref = FrontendMenuManager->m_bWidescreenOn;

                auto menuDirection = 1;
                if (*(int8_t*)(regs.esp + 0x8) == -1)
                    menuDirection = -1;
                pref += menuDirection;

                if (pref > CutsceneBordersMode::Both)
                    pref = CutsceneBordersMode::Off;
                if (pref < CutsceneBordersMode::Off)
                    pref = CutsceneBordersMode::Both;
            });

            pattern = hook::pattern("8A 46 ? 84 C0 0F 94 C0 8B CE 88 46 ? E8 ? ? ? ? 5F 5E 8A C3");
            injector::MakeNOP(pattern.get_first(), 13, true);
            static auto ProcessOnOffMenuOptionsHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                regs.ecx = regs.esi;
                auto& pref = FrontendMenuManager->m_bPrefsFrameLimiter;

                auto menuDirection = 1;
                if (*(int8_t*)(regs.esp + 0x8) == -1)
                    menuDirection = -1;
                pref = NextFrameLimiter(pref, menuDirection);
            });

            // A new screen resolution (CMenuManager::ProcessPCMenuOptions) centred the cursor and selected the sixth
            // row of Advanced Display Setup (Mip Mapping). The selection stays on Resolution, and the menu doesn't
            // take the centred cursor or the resized window for a mouse movement until the cursor really moves.
            pattern = hook::pattern("88 9E B8 00 00 00 C7 46 54 05 00 00 00 E8");
            injector::MakeNOP(pattern.get_first(6), 7, true);
            static auto ResolutionAppliedHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext&)
            {
                FixOptions::RememberResolutionCursor();
            });

            pattern = hook::pattern("75 ? 80 3D ? ? ? ? ? 74 ? B9");
            injector::MakeNOP(pattern.get_first(), 2, true);

            pattern = hook::pattern("7A ? 57 6A ? E8");
            injector::MakeNOP(pattern.get_first(), 2, true);

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 39 1D ? ? ? ? 75 ? 89 3D");
            hbRsEventHandler.fun = injector::MakeCALL(pattern.get_first(0), RsEventHandler).get();

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 8B CE");
            static auto DrawFrontEndHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&)
            {
                LimitMenuFrame();
            });
            DrawMenuHook = safetyhook::create_inline(pattern.get_first<uint8_t>(-19), DrawMenuWithIndicators);
        };
    }
} Menu;
