module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/RefreshRatePicker.hpp"
#include "Shared/RWGTA/WindowRefreshRate.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib") // needed for timeBeginPeriod()/timeEndPeriod()

export module Menu;

import Skeleton;
import Options;

enum eMenuAction
{
    MENUACTION_NOTHING,
    MENUACTION_LABEL,
    MENUACTION_YES,
    MENUACTION_NO,
    MENUACTION_CHANGEMENU,
    MENUACTION_INVERTPADY,
    MENUACTION_CTRLDISPLAY,
    MENUACTION_FRAMESYNC,
    MENUACTION_FRAMELIMIT,
    MENUACTION_TRAILS,
    MENUACTION_SUBTITLES,
    MENUACTION_WIDESCREEN,
    MENUACTION_BRIGHTNESS,
    MENUACTION_MUSICVOLUME,
    MENUACTION_SFXVOLUME,
    MENUACTION_RADIO,
    MENUACTION_LANG_ENG,
    MENUACTION_LANG_FRE,
    MENUACTION_LANG_GER,
    MENUACTION_LANG_ITA,
    MENUACTION_LANG_SPA,
    MENUACTION_POPULATESLOTS_CHANGEMENU,
    MENUACTION_CHECKSAVE,
    MENUACTION_NEWGAME,
    MENUACTION_RESUME_FROM_SAVEZONE,
    MENUACTION_RELOADIDE,
    MENUACTION_SETDBGFLAG,
    MENUACTION_LOADRADIO,
    MENUACTION_SAVEGAME,
    MENUACTION_SWITCHBIGWHITEDEBUGLIGHT,
    MENUACTION_COLLISIONPOLYS,
    MENUACTION_LEGENDS,
    MENUACTION_RADARMODE,
    MENUACTION_HUD,
    MENUACTION_GOBACK,
    MENUACTION_KEYBOARDCTRLS,
    MENUACTION_GETKEY,
    MENUACTION_SHOWHEADBOB,
    MENUACTION_UNK38,
    MENUACTION_INVVERT,
    MENUACTION_CANCELGAME,
    MENUACTION_RESUME,
    MENUACTION_DONTCANCEL,
    MENUACTION_SCREENRES,
    MENUACTION_AUDIOHW,
    MENUACTION_SPEAKERCONF,
    MENUACTION_PLAYERSETUP,
    MENUACTION_RESTOREDEF,
    MENUACTION_CTRLMETHOD,
    MENUACTION_DYNAMICACOUSTIC,
    MENUACTION_MOUSESTEER,
    MENUACTION_DRAWDIST,
    MENUACTION_MOUSESENS,
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

// MAIN.FPSLimit: 0 unlimited, -1 the monitor's refresh rate, or frames per second; the native row's steps
constexpr int FrameLimitSteps[] = { 0, -1, 30, 40, 50, 60, 75, 90, 100, 120, 144, 165, 180, 200, 240 };

static int NextFrameLimit(int current, int direction)
{
    constexpr int count = int(std::size(FrameLimitSteps));
    auto it = std::find(std::begin(FrameLimitSteps), std::end(FrameLimitSteps), current);
    if (it != std::end(FrameLimitSteps))
        return FrameLimitSteps[(int(it - std::begin(FrameLimitSteps)) + count + direction) % count];
    // A value set elsewhere: the nearest step that way
    if (direction > 0)
    {
        for (int i = 2; i < count; ++i)
            if (FrameLimitSteps[i] > current)
                return FrameLimitSteps[i];
        return FrameLimitSteps[0];
    }
    for (int i = count - 1; i >= 2; --i)
        if (FrameLimitSteps[i] < current)
            return FrameLimitSteps[i];
    return FrameLimitSteps[count - 1];
}

static int cutsceneBorders = 3;
static int frameLimit = -1;

static void ReadGameplayPreferences()
{
    cutsceneBorders = std::clamp(ReadFixSetting("MAIN.CutsceneBorders", 3), 0, 3);
    frameLimit = ReadFixSetting("MAIN.FPSLimit", -1);
}

static int CutsceneBordersSetting() { return cutsceneBorders; }
static int FrameLimitSetting() { return frameLimit; }

struct tSkinInfo
{
    int32_t skinId;
    char skinNameDisplayed[256];
    char skinNameOriginal[256];
    char date[256];
    tSkinInfo* nextSkin;
};

enum
{
    MENUALIGN_LEFT = 1,
    MENUALIGN_RIGHT,
    MENUALIGN_CENTER,
};

enum eMenuSprites
{
    MENUSPRITE_BACKGROUND,
    MENUSPRITE_VCLOGO,
    MENUSPRITE_MOUSE,
    MENUSPRITE_MAPTOP01,
    MENUSPRITE_MAPTOP02,
    MENUSPRITE_MAPTOP03,
    MENUSPRITE_MAPMID01,
    MENUSPRITE_MAPMID02,
    MENUSPRITE_MAPMID03,
    MENUSPRITE_MAPBOT01,
    MENUSPRITE_MAPBOT02,
    MENUSPRITE_MAPBOT03,
    MENUSPRITE_WILDSTYLE,
    MENUSPRITE_FLASH,
    MENUSPRITE_KCHAT,
    MENUSPRITE_FEVER,
    MENUSPRITE_VROCK,
    MENUSPRITE_VCPR,
    MENUSPRITE_ESPANTOSO,
    MENUSPRITE_EMOTION,
    MENUSPRITE_WAVE,
    MENUSPRITE_MP3,
    MENUSPRITE_DOWNOFF,
    MENUSPRITE_DOWNON,
    MENUSPRITE_UPOFF,
    MENUSPRITE_UPON,

    NUM_MENU_SPRITES
};

class CSprite2d
{
public:
    RwTexture* m_pTexture;
};

export class CMenuManager
{
public:
    int8_t m_StatsScrollDirection;
    float m_StatsScrollSpeed;
    uint8_t field_8;
    bool m_PrefsUseVibration;
    bool m_PrefsShowHud;
    int32_t m_PrefsRadarMode;
    bool m_DisplayControllerOnFoot;
    bool m_bShutDownFrontEndRequested;
    bool m_bStartUpFrontEndRequested;
    int32_t m_KeyPressedCode;
    int32_t m_PrefsBrightness;
    float m_PrefsLOD;
    int8_t m_PrefsShowSubtitles;
    int8_t m_PrefsShowLegends;
    int8_t m_PrefsUseWideScreen;
    int8_t m_PrefsVsync;
    int8_t m_PrefsVsyncDisp;
    int8_t m_PrefsFrameLimiter;
    int8_t m_nPrefsAudio3DProviderIndex;
    int8_t m_PrefsSpeakers;
    int8_t m_PrefsDMA;
    int8_t m_PrefsSfxVolume;
    int8_t m_PrefsMusicVolume;
    int8_t m_PrefsRadioStation;
    uint8_t m_PrefsStereoMono; // unused except restore settings
    int32_t m_nCurrOption;
    bool m_bQuitGameNoCD;
    bool m_bMenuMapActive;
    bool m_AllowNavigation;
    uint8_t field_37;
    bool m_bMenuActive;
    bool m_bWantToRestart;
    bool m_bFirstTime;
    bool m_bActivateSaveMenu;
    bool m_bWantToLoad;
    float m_fMapSize;
    float m_fMapCenterX;
    float m_fMapCenterY;
    uint32_t OS_Language;
    int32_t m_PrefsLanguage;
    int32_t field_54;
    int8_t m_bLanguageLoaded;
    uint8_t m_PrefsAllowNastyGame;
    int8_t m_PrefsMP3BoostVolume;
    int8_t m_ControlMethod;
    int32_t m_nPrefsVideoMode;
    int32_t m_nDisplayVideoMode;
    int32_t m_nMouseTempPosX;
    int32_t m_nMouseTempPosY;
    bool m_bGameNotLoaded;
    int8_t m_lastWorking3DAudioProvider;
    bool m_bFrontEnd_ReloadObrTxtGxt;
    int32_t* pEditString;
    uint8_t field_74[4];
    int32_t* pControlEdit;
    bool m_OnlySaveMenu;
    int32_t m_firstStartCounter;
    CSprite2d m_aFrontEndSprites[NUM_MENU_SPRITES];
    bool m_bSpritesLoaded;
    int32_t m_LeftMostRadioX;
    int32_t m_ScrollRadioBy;
    int32_t m_nCurrScreen;
    int32_t m_nPrevScreen;
    int32_t m_nCurrSaveSlot;
    uint32_t m_LastScreenSwitch;
    int32_t m_nMenuFadeAlpha;
    int32_t m_nOptionHighlightTransitionBlend;
    bool bMenuChangeOngoing;
    int32_t MouseButtonJustClicked;
    int32_t JoyButtonJustClicked;
    bool DisplayComboButtonErrMsg;
    bool m_NoEmptyBinding;
    bool m_ShowEmptyBindingError;
    int32_t m_nHelperTextAlpha;
    bool m_bPressedPgUpOnList;
    bool m_bPressedPgDnOnList;
    bool m_bPressedUpOnList;
    bool m_bPressedDownOnList;
    bool m_bPressedScrollButton;
    uint8_t field_129;
    uint8_t field_12A;
    uint8_t field_12B;
    int32_t m_nMousePosX;
    int32_t m_nMousePosY;
    int32_t m_nMouseOldPosX;
    int32_t m_nMouseOldPosY;
    int32_t m_nHoverOption;
    bool m_bShowMouse;
    int32_t m_nOptionMouseHovering;
    bool m_bStartWaitingForKeyBind;
    bool m_bWaitingForNewKeyBind;
    bool m_bKeyChangeNotProcessed;
    int32_t m_CurrCntrlAction;
    uint8_t field_150;
    uint8_t field_151;
    uint8_t field_152;
    uint8_t field_153;
    int32_t m_nSelectedContSetupColumn;
    bool m_bKeyIsOK;
    bool field_159;
    uint8_t m_nCurrExLayer;
    char m_PrefsSkinFile[256];
    char m_aSkinName[256];
    uint8_t field_35B;
    int32_t m_nHelperTextMsgId;
    tSkinInfo m_pSkinListHead;
    tSkinInfo* m_pSelectedSkin;
    int32_t m_nFirstVisibleRowOnList;
    float m_nScrollbarTopMargin;
    int32_t m_nTotalListRow;
    int32_t m_nSkinsTotal;
    uint8_t field_67C[4];
    int32_t m_nSelectedListRow;
    bool m_bSkinsEnumerated;
    enum LANGUAGE
    {
        LANGUAGE_AMERICAN,
        LANGUAGE_FRENCH,
        LANGUAGE_GERMAN,
        LANGUAGE_ITALIAN,
        LANGUAGE_SPANISH,
    };
    bool GetIsMenuActive() { return !!m_bMenuActive; }
};

export GameRef<CMenuManager> FrontendMenuManager([]() -> CMenuManager*
{
    auto pattern = hook::pattern("B9 ? ? ? ? 6A 01 68 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? E8");
    return *pattern.get_first<CMenuManager*>(1);
});

namespace CText
{
    void* TheText = nullptr;
    const wchar_t* (__fastcall* Get)(void*, void*, const char*) = nullptr;

    const wchar_t* Translated(const char* key, const wchar_t* fallback)
    {
        auto text = FixOptions::GameText(key);
        return *text ? text : fallback;
    }

    const wchar_t* GetCutsceneBordersText()
    {
        switch (CutsceneBordersSetting())
        {
            case CutsceneBordersMode::Off:
                return Get(TheText, 0, "FEM_OFF");
            case CutsceneBordersMode::Letterbox:
                return Translated("cutsceneborders.letterbox", L"LETTERBOX");
            case CutsceneBordersMode::Pillarbox:
                return Translated("cutsceneborders.pillarbox", L"PILLARBOX");
            default:
                return Translated("cutsceneborders.both", L"LETTERBOX + PILLARBOX");
        }
    }

    const wchar_t* GetFrameLimiterText()
    {
        int limit = FrameLimitSetting();
        if (limit == 0)
            return Translated("fpslimit.unlimited", L"UNLIMITED");
        if (limit < 0)
            return Translated("fpslimit.monitor", L"MONITOR REFRESH RATE");
        static std::wstring value;
        value = Translated("fpslimit.value", L"{0} FPS");
        auto at = value.find(L"{0}");
        if (at == std::wstring::npos)
            value = std::to_wstring(limit) + L" " + value;
        else
            value.replace(at, 3, std::to_wstring(limit));
        return value.c_str();
    }
}

enum RsEvent
{
    rsCAMERASIZE,
    rsCOMMANDLINE,
    rsFILELOAD,
    rsINITDEBUG,
    rsINPUTDEVICEATTACH,
    rsLEFTBUTTONDOWN,
    rsLEFTBUTTONUP,
    rsMOUSEMOVE,
    rsMOUSEWHEELMOVE,
    rsPLUGINATTACH,
    rsREGISTERIMAGELOADER,
    rsRIGHTBUTTONDOWN,
    rsRIGHTBUTTONUP,
    _rs_13,
    _rs_14,
    _rs_15,
    _rs_16,
    _rs_17,
    _rs_18,
    _rs_19,
    _rs_20,
    rsRWINITIALIZE,
    rsRWTERMINATE,
    rsSELECTDEVICE,
    rsINITIALIZE,
    rsTERMINATE,
    rsIDLE,
    rsFRONTENDIDLE,
    rsKEYDOWN,
    rsKEYUP,
    rsQUITAPP,
    rsPADBUTTONDOWN,
    rsPADBUTTONUP,
    rsPADANALOGUELEFT,
    rsPADANALOGUELEFTRESET,
    rsPADANALOGUERIGHT,
    rsPADANALOGUERIGHTRESET,
    rsPREINITCOMMANDLINE,
    rsACTIVATE,
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
    int limit = FrameLimitSetting();
    if (limit < 0)
    {
        const auto window = RsGlobal->ps ? *reinterpret_cast<HWND*>(RsGlobal->ps) : nullptr;
        return float(RWGTA::WindowRefreshRate(window));
    }
    return float(limit);
}

// The game's own preferences follow the settings: the cutscene borders' mode, and the frame limiter on or off
static void MirrorPreferences()
{
    FrontendMenuManager->m_PrefsUseWideScreen = int8_t(CutsceneBordersSetting());
    FrontendMenuManager->m_PrefsFrameLimiter = int8_t(FrameLimitSetting() != 0 ? FrameLimiterMode::e30 : FrameLimiterMode::eOff);
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

class Menu
{
public:
    Menu()
    {
        WFP::onInitEvent() += []()
        {
            RWGTA::InstallRefreshRatePicker<false>("39 E8 73 07 83 F8 3C 72 02 89 C5");
        };

        WFP::onGameInitEvent() += []()
        {
            // The cutscene borders and the frame limit are the fix's settings, whatever the game's settings file had
            ReadGameplayPreferences();
            MirrorPreferences();
            OnSettingChange("MAIN.CutsceneBorders", ReadGameplayPreferences);
            OnSettingChange("MAIN.FPSLimit", ReadGameplayPreferences);

            // CMenuManager::SetDefaultPreferences of Display Setup (Restore Defaults): the two rows' settings too
            auto restore = hook::pattern("C7 45 18 00 01 00 00 C7 45 1C 9A 99 99 3F");
            static auto RestoreDisplayDefaultsHook = safetyhook::create_mid(restore.get_first(), [](SafetyHookContext&)
            {
                RWGTAMenu::RestoreDefaults(FixSettings(), { "MAIN.CutsceneBorders", "MAIN.FPSLimit" });
                ReadGameplayPreferences();
                MirrorPreferences();
            });
            WFP::onGameProcessEvent() += []()
            {
                MirrorPreferences();
            };

            menuFrameLimit = static_cast<float>(std::max(0, ReadFixSetting("MAIN.FPSLimitMenus", 60)));
            OnSettingChange("MAIN.FPSLimitMenus", []()
            {
                menuFrameLimit = static_cast<float>(std::max(0, ReadFixSetting("MAIN.FPSLimitMenus", 60)));
                menuFrameLimitChanged = true;
            });

            auto pattern = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 89 44 24 ? 80 7D");
            CText::TheText = *pattern.get_first<void*>(1);
            CText::Get = (decltype(CText::Get))injector::GetBranchDestination(pattern.get_first(5)).as_int();

            pattern = hook::pattern("80 7D ? ? 75 ? 8B B5");
            static auto GetMenuOptionText = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto& ptr = *(wchar_t**)(regs.esp + 0x14);
                if (std::wstring_view(ptr) == CText::Get(CText::TheText, 0, "FED_WIS"))
                {
                    ptr = const_cast<wchar_t*>(CText::Translated("cutsceneborders", L"CUTSCENE BORDERS"));
                }
            });

            static int cachedAction = -1;
            pattern = hook::pattern("83 E8 ? 0F 84 ? ? ? ? 83 E8 ? 0F 84 ? ? ? ? 83 E8 ? 0F 84 ? ? ? ? 83 E8 ? 0F 84 ? ? ? ? 83 E8 ? 0F 84 ? ? ? ? 83 E8 ? 83 F8");
            static auto GetCurrentAction = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                cachedAction = regs.eax;
            });

            pattern = hook::pattern("80 BC 24 ? ? ? ? ? 0F 84 ? ? ? ? 8B 44 24");
            static auto GetMenuOnOffText = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                // The values of the fix's own rows (Options.ixx)
                if (auto text = FixOptions::RightText(cachedAction))
                {
                    regs.ebx = reinterpret_cast<uintptr_t>(text);
                    return;
                }

                auto ptr = (wchar_t*)(regs.ebx);
                if (ptr)
                {
                    if (cachedAction == MENUACTION_FRAMELIMIT)
                    {
                        if (std::wstring_view(ptr) == CText::Get(CText::TheText, 0, "FEM_ON") || std::wstring_view(ptr) == CText::Get(CText::TheText, 0, "FEM_OFF"))
                        {
                            *(wchar_t**)&regs.ebx = const_cast<wchar_t*>(CText::GetFrameLimiterText());
                        }
                    }
                    else if (cachedAction == MENUACTION_WIDESCREEN)
                    {
                        if (std::wstring_view(ptr) == CText::Get(CText::TheText, 0, "FEM_ON") || std::wstring_view(ptr) == CText::Get(CText::TheText, 0, "FEM_OFF"))
                        {
                            *(wchar_t**)&regs.ebx = const_cast<wchar_t*>(CText::GetCutsceneBordersText());
                        }
                    }
                }
            });

            pattern = hook::pattern("31 C0 80 7B ? ? 75 ? 40 88 43 ? 89 D9");
            injector::MakeNOP(pattern.get_first(), 12, true);
            static auto ProcessOnOffMenuOptionsHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                int direction = *(int8_t*)(regs.esp + 0x8C) == -1 ? -1 : 1;
                int borders = (CutsceneBordersSetting() + direction + 4) % 4;
                FixSettings().Set("MAIN.CutsceneBorders", std::to_string(borders));
                ReadGameplayPreferences();
                MirrorPreferences();
            });

            pattern = hook::pattern("31 C0 80 7B ? ? 75 ? 40 88 43 ? EB ? 31 C0 80 3D");
            injector::MakeNOP(pattern.get_first(), 12, true);
            static auto ProcessOnOffMenuOptionsHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                int direction = *(int8_t*)(regs.esp + 0x8C) == -1 ? -1 : 1;
                FixSettings().Set("MAIN.FPSLimit", std::to_string(NextFrameLimit(FrameLimitSetting(), direction)));
                ReadGameplayPreferences();
                MirrorPreferences();
            });

            // A new screen resolution (CMenuManager::ProcessButtonPresses) centred the cursor and selected the sixth
            // row of Display Setup, the one under the centre, and the same accept then toggled that row (Map Legend,
            // or Cutscene Borders below an added Trails row). The selection stays on Screen Resolution, whose accept
            // changes nothing else, and the menu doesn't take the centred cursor or the resized window for a mouse
            // movement until the cursor really moves.
            static POINT resolutionCursor{};
            static bool resolutionCursorHeld = false;
            pattern = hook::pattern("C6 85 40 01 00 00 01 C7 45 30 05 00 00 00");
            injector::MakeNOP(pattern.get_first(7), 7, true);
            static auto ResolutionAppliedHook = safetyhook::create_mid(pattern.get_first(7), [](SafetyHookContext&)
            {
                resolutionCursorHeld = GetCursorPos(&resolutionCursor) != FALSE;
            });

            // CMenuManager::UserInput's mouse movement check (previous mouse position at +134h/+138h, current at
            // +12Ch/+130h, the window's mouse position at +64h/+68h)
            pattern = hook::pattern("80 BD 40 01 00 00 00 75 ? 83 BD F8 00 00 00 06 74");
            static auto MouseMovedHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (!resolutionCursorHeld)
                    return;
                POINT cursor{};
                if (!GetCursorPos(&cursor) || cursor.x != resolutionCursor.x || cursor.y != resolutionCursor.y)
                {
                    resolutionCursorHeld = false;
                    return;
                }
                auto manager = reinterpret_cast<uint8_t*>(regs.ebp);
                auto field = [manager](size_t offset) -> int32_t& { return *reinterpret_cast<int32_t*>(manager + offset); };
                const int32_t x = std::clamp(field(0x64), 0, std::max(0, RsGlobal->width));
                const int32_t y = std::clamp(field(0x68), 0, std::max(0, RsGlobal->height));
                field(0x12C) = field(0x134) = x;
                field(0x130) = field(0x138) = y;
            });

            pattern = hook::pattern("74 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? DF E0 80 E4 ? 80 FC ? 0F 85 ? ? ? ? 6A");
            injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true);

            pattern = hook::pattern("E8 ? ? ? ? 59 59 EB ? 89 C0 8D 40 ? A1 ? ? ? ? 50");
            hbRsEventHandler.fun = injector::MakeCALL(pattern.get_first(0), RsEventHandler).get();

            pattern = hook::pattern("E8 ? ? ? ? 59 E8 ? ? ? ? E8 ? ? ? ? 6A ? 6A");
            static auto DrawFrontEndHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&)
            {
                LimitMenuFrame();
            });
            DrawMenuHook = safetyhook::create_inline(pattern.get_first<uint8_t>(-9), DrawMenuWithIndicators);
        };
    }
} Menu;