module;

#include <stdafx.h>
#include "common.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib") // needed for timeBeginPeriod()/timeEndPeriod()

export module Menu;

import Skeleton;

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
    eOff, e30, e40, e50, e60, e75, e100, e120, e144, e165, e200, e240
};

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
    if (!pattern.empty())
        return *pattern.get_first<CMenuManager*>(1);
    return nullptr;
});

namespace CText
{
    void* TheText = nullptr;
    const wchar_t* (__fastcall* Get)(void*, void*, const char*) = nullptr;

    auto CUTSCENE_BORDERS = L"CUTSCENE BORDERS";

    const wchar_t* GetCutsceneBordersText()
    {
        switch (FrontendMenuManager->m_PrefsUseWideScreen)
        {
            case CutsceneBordersMode::Off:
                return Get(TheText, 0, "FEM_OFF");
            case CutsceneBordersMode::Letterbox:
                return L"LETTERBOX";
            case CutsceneBordersMode::Pillarbox:
                return L"PILLARBOX";
            case CutsceneBordersMode::Both:
                return L"LETTERBOX + PILLARBOX";
            default:
                break;
        };

        return Get(TheText, 0, "FEM_ON");
    }

    const wchar_t* GetFrameLimiterText()
    {
        switch (FrontendMenuManager->m_PrefsFrameLimiter)
        {
            case FrameLimiterMode::eOff:
                return Get(TheText, 0, "FEM_OFF");
            case FrameLimiterMode::e30:
                return L"30 FPS";
            case FrameLimiterMode::e40:
                return L"40 FPS";
            case FrameLimiterMode::e50:
                return L"50 FPS";
            case FrameLimiterMode::e60:
                return L"60 FPS";
            case FrameLimiterMode::e75:
                return L"75 FPS";
            case FrameLimiterMode::e100:
                return L"100 FPS";
            case FrameLimiterMode::e120:
                return L"120 FPS";
            case FrameLimiterMode::e144:
                return L"144 FPS";
            case FrameLimiterMode::e165:
                return L"165 FPS";
            case FrameLimiterMode::e200:
                return L"200 FPS";
            case FrameLimiterMode::e240:
                return L"240 FPS";
            default:
                break;
        };
        return Get(TheText, 0, "FEM_ON");
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
    switch (FrontendMenuManager->m_PrefsFrameLimiter)
    {
        case FrameLimiterMode::e30:  return 30.0f;
        case FrameLimiterMode::e40:  return 40.0f;
        case FrameLimiterMode::e50:  return 50.0f;
        case FrameLimiterMode::e60:  return 60.0f;
        case FrameLimiterMode::e75:  return 75.0f;
        case FrameLimiterMode::e100: return 100.0f;
        case FrameLimiterMode::e120: return 120.0f;
        case FrameLimiterMode::e144: return 144.0f;
        case FrameLimiterMode::e165: return 165.0f;
        case FrameLimiterMode::e200: return 200.0f;
        case FrameLimiterMode::e240: return 240.0f;
        case FrameLimiterMode::eOff:
        default:
            return 0.0f;
    }
}

injector::hook_back<bool(__cdecl*)(RsEvent event, void* data)> hbRsEventHandler;
bool __cdecl RsEventHandler(RsEvent event, void* data)
{
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

class Menu
{
public:
    Menu()
    {
        WFP::onGameInitEvent() += []()
        {
            auto pattern = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 89 44 24 ? 80 7D");
            CText::TheText = *pattern.get_first<void*>(1);
            CText::Get = (decltype(CText::Get))injector::GetBranchDestination(pattern.get_first(5)).as_int();

            pattern = hook::pattern("80 7D ? ? 75 ? 8B B5");
            static auto GetMenuOptionText = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto& ptr = *(wchar_t**)(regs.esp + 0x14);
                if (std::wstring_view(ptr) == CText::Get(CText::TheText, 0, "FED_WIS"))
                {
                    ptr = const_cast<wchar_t*>(CText::CUTSCENE_BORDERS);
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
                auto& pref = FrontendMenuManager->m_PrefsUseWideScreen;

                auto menuDirection = 1;
                if (*(int8_t*)(regs.esp + 0x8C) == -1)
                    menuDirection = -1;
                pref += menuDirection;

                if (pref > CutsceneBordersMode::Both)
                    pref = CutsceneBordersMode::Off;
                if (pref < CutsceneBordersMode::Off)
                    pref = CutsceneBordersMode::Both;
            });

            pattern = hook::pattern("31 C0 80 7B ? ? 75 ? 40 88 43 ? EB ? 31 C0 80 3D");
            injector::MakeNOP(pattern.get_first(), 12, true);
            static auto ProcessOnOffMenuOptionsHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto& pref = FrontendMenuManager->m_PrefsFrameLimiter;

                auto menuDirection = 1;
                if (*(int8_t*)(regs.esp + 0x8C) == -1)
                    menuDirection = -1;
                pref += menuDirection;

                if (pref > FrameLimiterMode::e240)
                    pref = FrameLimiterMode::eOff;
                if (pref < FrameLimiterMode::eOff)
                    pref = FrameLimiterMode::e240;
            });

            pattern = hook::pattern("74 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? DF E0 80 E4 ? 80 FC ? 0F 85 ? ? ? ? 6A");
            injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true);

            pattern = hook::pattern("E8 ? ? ? ? 59 59 EB ? 89 C0 8D 40 ? A1 ? ? ? ? 50");
            hbRsEventHandler.fun = injector::MakeCALL(pattern.get_first(0), RsEventHandler).get();
        };
    }
} Menu;