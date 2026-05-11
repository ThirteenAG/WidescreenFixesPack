module;

#include <stdafx.h>
#include "common.h"

export module Menu;

import Skeleton;

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
};

export GameRef<CMenuManager> FrontendMenuManager([]() -> CMenuManager*
{
    auto pattern = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 6A 01 E8 ? ? ? ? 83 C4 04 E8 ? ? ? ? C6 46 ? ? A1 ? ? ? ? 83 C5 14 47 83 C3 2C");
    if (!pattern.empty())
        return *pattern.get_first<CMenuManager*>(1);
    return nullptr;
});

namespace CText
{
    void* TheText = nullptr;
    const char* (__fastcall* Get)(void*, void*, const char*) = nullptr;

    auto CUTSCENE_BORDERS = "CUTSCENE BORDERS";

    const char* GetCutsceneBordersText()
    {
        switch (FrontendMenuManager->m_bWidescreenOn)
        {
            case CutsceneBordersMode::Off:
                return Get(TheText, 0, "FEM_OFF");
            case CutsceneBordersMode::Letterbox:
                return "LETTERBOX";
            case CutsceneBordersMode::Pillarbox:
                return "PILLARBOX";
            case CutsceneBordersMode::Both:
                return "LETTERBOX + PILLARBOX";
            default:
                break;
        };

        return Get(TheText, 0, "FEM_ON");
    }

    const char* GetFrameLimiterText()
    {
        switch (FrontendMenuManager->m_bPrefsFrameLimiter)
        {
            case FrameLimiterMode::eOff:
                return Get(TheText, 0, "FEM_OFF");
            case FrameLimiterMode::e30:
                return "30 FPS";
            case FrameLimiterMode::e40:
                return "40 FPS";
            case FrameLimiterMode::e50:
                return "50 FPS";
            case FrameLimiterMode::e60:
                return "60 FPS";
            case FrameLimiterMode::e75:
                return "75 FPS";
            case FrameLimiterMode::e100:
                return "100 FPS";
            case FrameLimiterMode::e120:
                return "120 FPS";
            case FrameLimiterMode::e144:
                return "144 FPS";
            case FrameLimiterMode::e165:
                return "165 FPS";
            case FrameLimiterMode::e200:
                return "200 FPS";
            case FrameLimiterMode::e240:
                return "240 FPS";
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
                        *(char**)&regs.ebx = const_cast<char*>(CText::CUTSCENE_BORDERS);
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
                pref += menuDirection;

                if (pref > FrameLimiterMode::e240)
                    pref = FrameLimiterMode::eOff;
                if (pref < FrameLimiterMode::eOff)
                    pref = FrameLimiterMode::e240;
            });

            pattern = hook::pattern("DB 05 ? ? ? ? D8 3D ? ? ? ? D8 5C 24");
            struct FrameLimiter
            {
                void operator()(injector::reg_pack& regs)
                {
                    float maxFPS = 30.0f;
                    auto pref = FrontendMenuManager->m_bPrefsFrameLimiter;
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