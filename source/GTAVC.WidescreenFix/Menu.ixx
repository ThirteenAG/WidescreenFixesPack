module;

#include <stdafx.h>
#include "common.h"

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

            pattern = hook::pattern("DB 05 ? ? ? ? D8 3D ? ? ? ? D8 D9 DF E0 80 E4 05 80 FC 01 0F 85");
            struct FrameLimiter
            {
                void operator()(injector::reg_pack& regs)
                {
                    float maxFPS = 30.0f;
                    auto pref = FrontendMenuManager->m_PrefsFrameLimiter;
                    switch (pref)
                    {
                        case FrameLimiterMode::e30: maxFPS = 30.0f; break;
                        case FrameLimiterMode::e40: maxFPS = 40.0f; break;
                        case FrameLimiterMode::e50: maxFPS = 50.0f; break;
                        case FrameLimiterMode::e60: maxFPS = 60.0f; break;
                        case FrameLimiterMode::e75: maxFPS = 75.0f; break;
                        case FrameLimiterMode::e100: maxFPS = 100.0f; break;
                        case FrameLimiterMode::e120: maxFPS = 120.0f; break;
                        case FrameLimiterMode::e144: maxFPS = 144.0f; break;
                        case FrameLimiterMode::e165: maxFPS = 165.0f; break;
                        case FrameLimiterMode::e200: maxFPS = 200.0f; break;
                        case FrameLimiterMode::e240: maxFPS = 240.0f; break;
                        default:
                            break;
                    }

                    _asm {fld dword ptr[maxFPS]}

                }
            }; injector::MakeInline<FrameLimiter>(pattern.get_first(0), pattern.get_first(6));
        };
    }
} Menu;