#include "stdafx.h"

import ComVars;
import e2mfc;
import e2_d3d8_driver_mfc;
import input;
import xidi;
import PostFX;

SafetyHookInline shsub_40D040 = {};
int __fastcall sub_40D040(int* CWnd, void* edx, char a2)
{
    // border size to 0
    CWnd[22] = 0;
    CWnd[23] = 0;

    HMONITOR monitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
    MONITORINFOEX info = { sizeof(MONITORINFOEX) };
    GetMonitorInfo(monitor, &info);
    DEVMODE devmode = {};
    devmode.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devmode);
    DWORD DesktopX = devmode.dmPelsWidth;
    DWORD DesktopY = devmode.dmPelsHeight;

    // center window position
    CWnd[24] = (int)(((float)DesktopX / 2.0f) - ((float)CWnd[20] / 2.0f));
    CWnd[25] = (int)(((float)DesktopY / 2.0f) - ((float)CWnd[21] / 2.0f));

    return shsub_40D040.unsafe_fastcall<int>(CWnd, edx, a2);
}

namespace P_Camera
{
    enum
    {
        VIEWPORT = 0x1C4, // left, right, top and bottom
    };

    void(__fastcall* setFOV)(void* _this, void* edx, float fFOV) = nullptr;

    // What P_Camera::validate turns the horizontal FOV tangent into the vertical one with
    float GetViewportRatio(uint8_t* _this)
    {
        auto pViewport = (float*)(_this + VIEWPORT);
        float fRatio = (pViewport[3] - pViewport[2]) / (pViewport[1] - pViewport[0]);
        return std::isfinite(fRatio) && fRatio > 0.0f ? fRatio : 0.75f;
    }
}

// Set when MaxPayne_GameMode renders the game view, cleared at the end of each frame
bool bGameViewRendered = false;

namespace Cinematic
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

    // Borders slide in and out like in the GTA III widescreen fix, following the game's own
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
}

namespace X_LevelRuntimeCamera
{
    // The FOV set on the level camera is the 4:3 one, P_Camera::validate widens it for the screen.
    // FOVFactor and the cutscene framing are applied here, so the skybox camera and the portal code,
    // which both read this camera's FOV, stay in sync.
    float CalculateFOV(float fFOV)
    {
        float fTan = tanf(fFOV * 0.5f);
        if (fTan <= 0.0f)
            return fFOV;

        float fZoom = Screen.fAspectScaleX;
        if (Screen.fFOVFactor != 1.0f)
        {
            float fWideFOV = std::clamp(2.0f * atanf(fTan * Screen.fAspectScaleX) * Screen.fFOVFactor, 0.01f, 3.1f);
            fZoom = tanf(fWideFOV * 0.5f) / fTan;
        }

        // Cutscenes move to the vanilla framing along with the game's widescreen transition. Camera
        // overlays are 4:3 images with the gameplay view behind them.
        if (!Screen.bDrawBordersForCameraOverlay)
        {
            auto state = Cinematic::GetState(MaxPayne_GameMode::pInstance);
            if (state.fProgress > 0.0f)
                fZoom += (Cinematic::GetCutsceneZoom(state.fWideScreenMultiplier) - fZoom) * state.fProgress;
        }

        return 2.0f * atanf(fTan * fZoom / Screen.fAspectScaleX);
    }

    void __fastcall setFOV(void* pCamera, void* edx, float fFOV)
    {
        P_Camera::setFOV(pCamera, edx, CalculateFOV(fFOV));
    }

    // Portal culling in getFirstSceneToRender and getSceneToRender derives the frustum from the
    // camera FOV, so it needs the same scaling as P_Camera::validate (the mobile release does this
    // too). Without it rooms seen through portals get clipped to the 4:3 part of the screen.
    void getFirstSceneToRenderHook(SafetyHookContext& regs)
    {
        // [ebp-10h] is tan(fov / 2), st(0) is it multiplied by the viewport height
        *(float*)(regs.ebp - 0x10) *= Screen.fAspectScaleX;
        float fScaleY = Screen.fAspectScaleY;
        _asm fmul dword ptr[fScaleY]
    }

    void getSceneToRenderHook(SafetyHookContext& regs)
    {
        // st(1) is tan(fov / 2), st(0) is it multiplied by the viewport height
        float fScaleX = Screen.fAspectScaleX;
        float fScaleY = Screen.fAspectScaleY;
        _asm
        {
            fmul    dword ptr[fScaleY]
            fxch    st(1)
            fmul    dword ptr[fScaleX]
            fxch    st(1)
        }
    }
}

// Graphic novel pages are 3D scenes, meshes and a camera to show them with, which the page copies to
// the camera of the graphic novel mode when it shows. Original framing keeps that camera: the page
// across the 4:3 width at the top, the playback controls below it. Otherwise the camera moves to the
// middle of the page and zooms in as far as the whole page, its black border included, still fits
// on the screen.
namespace MaxPayne_GraphicNovelPage
{
    enum
    {
        OBJECT_ANIMATION = 0x2B, // KF2::KF_ObjectAnimation*, the meshes of the page
        CAMERA_MATRIX = 0x33,    // M_Matrix4x3
        FOV = 0x6B,
    };

    // The page shown and the camera showing it, cleared when the graphic novel mode ends
    uint8_t* pPage = nullptr;
    uint8_t* pCamera = nullptr;

    // The page as seen with its own camera
    struct Extents
    {
        float fCenterX; // how far the camera moves along its right and up axes to face the middle of the page
        float fCenterY;
        float fTanX;    // tangents of half the page width and height from there
        float fTanY;
    };
    std::optional<Extents> PageExtents;

    uint32_t nGeneration = 0;

    // From the bounding boxes of the meshes, the page only animates how opaque they are
    std::optional<Extents> Measure(uint8_t* page, uint8_t* camera)
    {
        auto pAnimation = *(uint8_t**)(page + OBJECT_ANIMATION);
        if (!pAnimation)
            return std::nullopt;

        P_BaseObject::calculateObjectToWorldMatrix(camera, nullptr);
        auto pView = (const float*)(camera + P_BaseObject::WORLD_MATRIX);

        struct Point { float x, y, z; };
        std::vector<Point> Points;
        for (uint32_t i = 0, nMeshes = KF_ObjectAnimation::getTotalMeshes(pAnimation); i < nMeshes; ++i)
        {
            auto pMesh = KF_ObjectAnimation::getMesh(pAnimation, i);
            if (!pMesh)
                continue;

            auto pMin = P_BaseObject::getBoundingBoxMin(pMesh);
            auto pMax = P_BaseObject::getBoundingBoxMax(pMesh);
            if (!(pMin[0] <= pMax[0] && pMin[1] <= pMax[1] && pMin[2] <= pMax[2]))
                continue;

            P_BaseObject::calculateObjectToWorldMatrix(pMesh, nullptr);
            auto pWorld = (const float*)(pMesh + P_BaseObject::WORLD_MATRIX);
            for (int nCorner = 0; nCorner < 8; ++nCorner)
            {
                float local[3] = { (nCorner & 1) ? pMax[0] : pMin[0], (nCorner & 2) ? pMax[1] : pMin[1], (nCorner & 4) ? pMax[2] : pMin[2] };
                float relative[3];
                for (int k = 0; k < 3; ++k)
                    relative[k] = local[0] * pWorld[k] + local[1] * pWorld[3 + k] + local[2] * pWorld[6 + k] + pWorld[9 + k] - pView[9 + k];

                auto Dot = [&](const float* pAxis) { return relative[0] * pAxis[0] + relative[1] * pAxis[1] + relative[2] * pAxis[2]; };
                Point point = { Dot(pView), Dot(pView + 3), Dot(pView + 6) };
                if (!(point.z > 0.0f))
                    return std::nullopt;
                Points.push_back(point);
            }
        }

        if (Points.empty())
            return std::nullopt;

        constexpr float fInfinity = std::numeric_limits<float>::infinity();
        float fMinX = fInfinity, fMaxX = -fInfinity, fMinY = fInfinity, fMaxY = -fInfinity, fDepth = 0.0f;
        for (auto& point : Points)
        {
            fMinX = std::min(fMinX, point.x / point.z);
            fMaxX = std::max(fMaxX, point.x / point.z);
            fMinY = std::min(fMinY, point.y / point.z);
            fMaxY = std::max(fMaxY, point.y / point.z);
            fDepth += point.z;
        }
        fDepth /= Points.size();

        // The page is flat and faces the camera, the depths only differ by its layers
        Extents extents = { (fMinX + fMaxX) * 0.5f * fDepth, (fMinY + fMaxY) * 0.5f * fDepth, 0.0f, 0.0f };
        for (auto& point : Points)
        {
            extents.fTanX = std::max(extents.fTanX, std::abs(point.x - extents.fCenterX) / point.z);
            extents.fTanY = std::max(extents.fTanY, std::abs(point.y - extents.fCenterY) / point.z);
        }

        if (!std::isfinite(extents.fCenterX) || !std::isfinite(extents.fCenterY) || !std::isfinite(extents.fTanX) || !std::isfinite(extents.fTanY) || extents.fTanX <= 0.0f || extents.fTanY <= 0.0f)
            return std::nullopt;
        return extents;
    }

    void Apply()
    {
        if (!pPage || !pCamera)
            return;

        nGeneration = Screen.nGeneration;

        // the page's camera, as MaxPayne_GraphicNovelPage::show sets it
        auto pMatrix = (float*)(pCamera + P_BaseObject::LOCAL_MATRIX);
        std::memcpy(pMatrix, pPage + CAMERA_MATRIX, 12 * sizeof(float));
        float fPageFOV = *(float*)(pPage + FOV);
        float fFOV = fPageFOV;
        if (!Screen.bGraphicNovelMode && PageExtents)
        {
            for (int k = 0; k < 3; ++k)
                pMatrix[9 + k] += pMatrix[k] * PageExtents->fCenterX + pMatrix[3 + k] * PageExtents->fCenterY;

            // P_Camera::validate widens the tangents for the screen
            float fTan = std::max(PageExtents->fTanX / Screen.fAspectScaleX, PageExtents->fTanY / (P_Camera::GetViewportRatio(pCamera) * Screen.fAspectScaleY));
            fFOV = 2.0f * atanf(fTan);
        }

        P_BaseObject::invalidateMatrices(pCamera, nullptr);
        P_Camera::setFOV(pCamera, nullptr, fFOV);
    }

    SafetyHookInline shShow = {};
    void __fastcall show(uint8_t* _this, void* edx, uint8_t* camera)
    {
        shShow.unsafe_fastcall(_this, edx, camera);
        if (!P_BaseObject::invalidateMatrices || !P_BaseObject::calculateObjectToWorldMatrix || !P_Camera::setFOV)
            return;

        pPage = _this;
        pCamera = camera;
        PageExtents = Measure(_this, camera);
        Apply();
    }

    // For the key toggling original framing
    void Refresh()
    {
        Apply();
    }

    // Called every frame in the graphic novel mode
    void Update()
    {
        if (pPage && nGeneration != Screen.nGeneration)
            Apply();
    }

    void Reset()
    {
        pPage = nullptr;
        pCamera = nullptr;
        PageExtents.reset();
    }
}

// Fugitive's adaptive difficulty rates each level by the player's deaths, average health and play
// time in it, counted on the player character, and moves the difficulty a step up or down when the
// level ends. Loading a save creates the player character anew with these at 0, and dying always
// ends in loading one, so deaths were never counted and health and time only covered the stretch
// since the last load: the difficulty went up after nearly every level and stayed at the hardest
// step. The counters now carry over loads within the same level.
namespace AdaptiveDifficulty
{
    struct LevelPerformance
    {
        uint8_t* pLevel = nullptr; // MaxPayne_GameMode::LEVEL_SETTINGS of the level the counters are from
        int32_t nDeaths = 0;
        float fHealthSum = 0.0f;
        float fPlayTime = 0.0f;
        float fPlayTimeFraction = 0.0f;
    } Performance;

    // Every frame the game view renders, a death included
    void Track(uint8_t* pGameMode)
    {
        auto pPlayer = MaxPayne_GameMode::GetPlayerCharacter(pGameMode);
        auto pLevel = *(uint8_t**)(pGameMode + MaxPayne_GameMode::LEVEL_SETTINGS);
        if (!pPlayer || !pLevel)
            return;

        Performance.pLevel = pLevel;
        Performance.nDeaths = *(int32_t*)(pPlayer + X_Character::DEATHS);
        Performance.fHealthSum = *(float*)(pPlayer + X_Character::HEALTH_SUM);
        Performance.fPlayTime = *(float*)(pPlayer + X_Character::PLAY_TIME);
        Performance.fPlayTimeFraction = *(float*)(pPlayer + X_Character::PLAY_TIME_FRACTION);
    }

    SafetyHookInline shLoad = {};
    bool __fastcall load(uint8_t* pGameMode, void* edx, void* pMemoryFile)
    {
        auto performance = Performance; // before loading tears the level down
        bool bResult = shLoad.unsafe_fastcall<bool>(pGameMode, edx, pMemoryFile);

        auto pPlayer = MaxPayne_GameMode::GetPlayerCharacter(pGameMode);
        if (pPlayer && performance.pLevel && performance.pLevel == *(uint8_t**)(pGameMode + MaxPayne_GameMode::LEVEL_SETTINGS))
        {
            *(int32_t*)(pPlayer + X_Character::DEATHS) = performance.nDeaths;
            *(float*)(pPlayer + X_Character::HEALTH_SUM) = performance.fHealthSum;
            *(float*)(pPlayer + X_Character::PLAY_TIME) = performance.fPlayTime;
            *(float*)(pPlayer + X_Character::PLAY_TIME_FRACTION) = performance.fPlayTimeFraction;
        }

        return bResult;
    }
}

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

// WriteSettingsToFile: the game keeps its settings in the registry through CWinApp's profile
// functions, which keep them in the INI file m_pszProfileName names instead while there's no
// registry key. Instead of setting one, the file next to the savegames gets used, starting out with
// what the registry has the first time.
namespace CWinApp
{
    enum
    {
        APP_NAME = 0x7C,     // char* m_pszAppName
        PROFILE_NAME = 0x94, // char* m_pszProfileName
    };

    std::filesystem::path SettingsPath;

    // of the CRT mfc42.dll frees m_pszProfileName with
    char* (__cdecl* crtStrdup)(const char* szString) = nullptr;
    void(__cdecl* crtFree)(void* pBlock) = nullptr;

    // As CWinApp::WriteProfileInt and WriteProfileString would have written the values
    void CopyRegistrySettings(const std::string& szKey)
    {
        HKEY hApp = nullptr;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, szKey.c_str(), 0, KEY_READ, &hApp) != ERROR_SUCCESS)
            return;

        auto szFile = SettingsPath.string();
        char szSection[256];
        for (DWORD i = 0; ; ++i)
        {
            DWORD nSectionLength = sizeof(szSection);
            if (RegEnumKeyExA(hApp, i, szSection, &nSectionLength, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
                break;

            HKEY hSection = nullptr;
            if (RegOpenKeyExA(hApp, szSection, 0, KEY_READ, &hSection) != ERROR_SUCCESS)
                continue;

            char szName[256];
            char data[4096];
            for (DWORD j = 0; ; ++j)
            {
                DWORD nNameLength = sizeof(szName);
                DWORD nDataSize = sizeof(data) - 1;
                DWORD nType = REG_NONE;
                auto status = RegEnumValueA(hSection, j, szName, &nNameLength, nullptr, &nType, (BYTE*)data, &nDataSize);
                if (status == ERROR_NO_MORE_ITEMS)
                    break;
                if (status != ERROR_SUCCESS)
                    continue;

                if (nType == REG_DWORD && nDataSize == sizeof(DWORD))
                    WritePrivateProfileStringA(szSection, szName, std::to_string(*(int32_t*)data).c_str(), szFile.c_str());
                else if (nType == REG_SZ)
                {
                    data[nDataSize] = '\0';
                    WritePrivateProfileStringA(szSection, szName, data, szFile.c_str());
                }
            }
            RegCloseKey(hSection);
        }
        RegCloseKey(hApp);
    }

    SafetyHookInline shSetRegistryKey = {};
    void __fastcall SetRegistryKey(uint8_t* _this, void* edx, const char* szRegistryKey)
    {
        std::error_code ec;
        if (!std::filesystem::exists(SettingsPath, ec))
            CopyRegistrySettings(std::string("Software\\") + szRegistryKey + "\\" + *(const char**)(_this + APP_NAME));

        auto& szProfileName = *(char**)(_this + PROFILE_NAME);
        crtFree(szProfileName);
        szProfileName = crtStrdup(SettingsPath.string().c_str());
    }
}

void InitSettingsFile()
{
    CIniReader iniReader("");
    if (iniReader.ReadInteger("MISC", "WriteSettingsToFile", 1) == 0)
        return;

    char szDocuments[MAX_PATH];
    if (iniReader.ReadInteger("MISC", "UseGameFolderForSavegames", 0) != 0)
        CWinApp::SettingsPath = GetExeModulePath() / "savegames";
    else if (SHGetSpecialFolderPathA(nullptr, szDocuments, CSIDL_PERSONAL, FALSE))
        CWinApp::SettingsPath = std::filesystem::path(szDocuments) / "Max Payne Savegames";
    else
        return;

    std::error_code ec;
    std::filesystem::create_directories(CWinApp::SettingsPath, ec);
    CWinApp::SettingsPath /= "settings.ini";

    auto crt = GetModuleHandleA("msvcrt.dll");
    CWinApp::crtStrdup = (decltype(CWinApp::crtStrdup))GetProcAddress(crt, "_strdup");
    CWinApp::crtFree = (decltype(CWinApp::crtFree))GetProcAddress(crt, "free");
    if (!CWinApp::crtStrdup || !CWinApp::crtFree)
        return;

    CWinApp::shSetRegistryKey = safetyhook::create_inline(GetProcAddress(GetModuleHandleA("mfc42.dll"), MAKEINTRESOURCEA(6117)), CWinApp::SetRegistryKey); // CWinApp::SetRegistryKey(LPCTSTR)
}

void ReadSettings()
{
    CIniReader iniReader("");
    Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(iniReader.ReadString("MAIN", "HudAspectRatioConstraint", ""));
    Screen.fFOVFactor = iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f);
    if (Screen.fFOVFactor <= 0.0f) { Screen.fFOVFactor = 1.0f; }
    Screen.bGraphicNovelMode = iniReader.ReadInteger("MAIN", "GraphicNovelMode", 1) != 0;
    Cinematic::nCutsceneBorders = std::clamp(iniReader.ReadInteger("MAIN", "CutsceneBorders", Cinematic::Both), (int32_t)Cinematic::Off, (int32_t)Cinematic::Both);
    Cinematic::bNoBorderAnimation = iniReader.ReadInteger("MAIN", "NoCutsceneBorderAnimation", 0) != 0;
}

void Init()
{
    CIniReader iniReader("");
    static bool bUseGameFolderForSavegames = iniReader.ReadInteger("MISC", "UseGameFolderForSavegames", 0) != 0;
    if (bUseGameFolderForSavegames)
    {
        auto pattern = hook::pattern("0F 84 ? ? ? ? E8 ? ? ? ? 8B 48 04 68 ? ? ? ? 56 89");
        injector::WriteMemory<uint8_t>(pattern.get_first(1), 0x85, true); //0x40FCAB
    }

    bool bAltTab = iniReader.ReadInteger("MISC", "AllowAltTabbingWithoutPausing", 0) != 0;
    if (bAltTab)
    {
        auto pattern = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 5E C2 08 00");
        injector::MakeNOP(pattern.count(2).get(1).get<uintptr_t>(0), 5, true); //0x40D29B
    }

    static int32_t nLoadSaveSlot = iniReader.ReadInteger("MISC", "LoadSaveSlot", -1);
    if (nLoadSaveSlot == -2 || nLoadSaveSlot == -3 || (nLoadSaveSlot >= 0 && nLoadSaveSlot <= 999))
    {
        static auto unk_8A6490 = *hook::get_pattern<void*>("BF ? ? ? ? F3 A5 8B C8 83 E1 03 F3 A4 5F 5E C3", 1);
        if (*(uint8_t*)unk_8A6490 == 0)
        {
            auto pattern = hook::pattern("E8 ? ? ? ? 8B 40 04 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 8D 55 10 52");
            static auto AfxGetModuleState = injector::GetBranchDestination(pattern.get_first(0), true); //0x76F77E
            static auto GetProfileStringA = injector::GetBranchDestination(pattern.get_first(29), true); //0x76F7B4
            static auto aLastSavedGameF = *pattern.get_first<char*>(14); // 8680EC
            static auto aSaveGame = *pattern.get_first<char*>(19); // 866FDC
            static auto aSavegames = *hook::get_pattern<char*>("BF ? ? ? ? F2 AE F7 D1 2B F9 8B D1 83 C9 FF 8B F7 8B FB F2 AE 8B 45 08 8B CA C1 E9 02 4F", 1);
            static auto MaxPayne2Saveg = *hook::get_pattern<char*>("BF ? ? ? ? 75 05 BF ? ? ? ? 83 C9 FF 33 C0 F2 AE F7 D1 2B F9 8B D1", 1);
            pattern = hook::pattern("89 87 ? ? ? ? E8 ? ? ? ? A2 ? ? ? ? E8");
            struct SaveGameHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint32_t*)(regs.edi + 0xCC) = regs.eax;

                    if (nLoadSaveSlot == -2)
                    {
                        void* pStr = nullptr;
                        auto _this = injector::stdcall<void* ()>::call(AfxGetModuleState);
                        injector::thiscall<void(void* _this, void* out, char const* a1, char const* a2, char const* a3)>::call(GetProfileStringA, *(void**)((uint32_t)_this + 4), &pStr, (char const*)aSaveGame, (char const*)aLastSavedGameF, "");
                        std::string_view LastSavedGameFilename{ (char*)pStr };
                        if (!LastSavedGameFilename.empty())
                            injector::WriteMemoryRaw(unk_8A6490, (void*)LastSavedGameFilename.data(), LastSavedGameFilename.size(), true);
                    }
                    else
                    {
                        char buffer[MAX_PATH];
                        if (bUseGameFolderForSavegames)
                        {
                            GetModuleFileNameA(NULL, buffer, MAX_PATH);
                            *strrchr(buffer, '\\') = '\0';
                            strcat_s(buffer, "\\");
                            strcat_s(buffer, aSavegames);
                            strcat_s(buffer, "\\");
                        }
                        else
                        {
                            SHGetSpecialFolderPathA(0, buffer, 5, false);
                            strcat_s(buffer, "\\");
                            strcat_s(buffer, MaxPayne2Saveg);
                            strcat_s(buffer, "\\");
                        }

                        auto nSaveNum = -1;
                        std::string SFPath(buffer);

                        WIN32_FIND_DATAA fd;
                        HANDLE File = FindFirstFileA(std::string(SFPath + "*.mps").c_str(), &fd);
                        FILETIME LastWriteTime = fd.ftLastWriteTime;

                        if (File != INVALID_HANDLE_VALUE)
                        {
                            do
                            {
                                std::string str(fd.cFileName);
                                auto n = str.find_first_of("0123456789");

                                if (nLoadSaveSlot >= 0)
                                {
                                    if (n != std::string::npos)
                                    {
                                        nSaveNum = std::atoi(&str[n]);
                                        if (nSaveNum == nLoadSaveSlot)
                                        {
                                            SFPath += str;
                                            injector::WriteMemoryRaw(unk_8A6490, SFPath.data(), SFPath.size(), true);
                                            return;
                                        }
                                    }
                                }
                                else
                                {
                                    if (CompareFileTime(&fd.ftLastWriteTime, &LastWriteTime) >= 0)
                                    {
                                        LastWriteTime = fd.ftLastWriteTime;
                                        std::string str(fd.cFileName);
                                        if (n != std::string::npos)
                                        {
                                            nSaveNum = std::atoi(&str[n]);
                                        }
                                    }
                                }
                            } while (FindNextFileA(File, &fd));
                            FindClose(File);
                        }

                        if (nSaveNum >= 0 && nLoadSaveSlot < 0)
                        {
                            char buffer[5]; sprintf(buffer, "%03d", nSaveNum);
                            SFPath += fd.cFileName;
                            auto offset = SFPath.end() - std::strlen("000.mps");
                            SFPath.replace(offset, offset + 3, buffer);
                            injector::WriteMemoryRaw(unk_8A6490, SFPath.data(), SFPath.size(), true);
                        }
                    }
                }
            }; injector::MakeInline<SaveGameHook>(pattern.get_first(0), pattern.get_first(6)); //0x415EBD
        }
    }

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

    // Mouse smoothing: each frame applies half of the movement not applied yet and carries the rest
    // over to the next one. Without it the movement applies in the frame it's made.
    if (iniReader.ReadInteger("MISC", "MouseSmoothing", 1) == 0)
    {
        pattern = hook::pattern("8D 4D DC E8 ? ? ? ? D9 45 DC D8 46 5B"); // X_InputDeviceMouse::update
        injector::MakeNOP(pattern.get_first(3), 5, true); //0x5191CC
    }

    pattern = hook::pattern("A0 ? ? ? ? 84 C0 0F 85 ? ? ? ? 8B 86");
    X_Crosshair::sm_bCameraPathRunning.SetAddress(*pattern.get_first<bool*>(1));

    // Graphic novels: key toggles between the original framing and the whole page as large as the screen allows
    static int32_t nGraphicNovelModeKey = iniReader.ReadInteger("MAIN", "GraphicNovelModeKey", VK_F2);
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

            bool bPressed = (GetAsyncKeyState(nGraphicNovelModeKey) & 0x8000) != 0;
            if (!bPressed && bWasPressed)
            {
                Screen.bGraphicNovelMode = !Screen.bGraphicNovelMode;
                CIniReader iniReader("");
                iniReader.WriteInteger("MAIN", "GraphicNovelMode", Screen.bGraphicNovelMode);
                MaxPayne_GraphicNovelPage::Refresh();
            }
            bWasPressed = bPressed;
        }
    }; injector::MakeInline<GraphicNovelPageUpdateHook>(pattern.get_first(0), pattern.get_first(6));

    // FOV
    pattern = hook::pattern("8B 97 D0 05 00 00 52 8B CE FF 15"); // X_LevelRuntimeCamera camera setup
    P_Camera::setFOV = **pattern.get_first<decltype(P_Camera::setFOV)*>(11);
    injector::MakeCALL(pattern.get_first(9), X_LevelRuntimeCamera::setFOV, true);
    injector::MakeNOP(pattern.get_first(14), 1, true);

    pattern = hook::pattern("8B 8E 74 10 00 00 33 ED 89 6C 24"); // MaxPayne_GameMode::renderMode
    static auto MaxPayne_GameModerenderModeHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        MaxPayne_GameMode::pInstance = (uint8_t*)regs.esi;
        bGameViewRendered = true;
        Cinematic::UpdateBorders(MaxPayne_GameMode::pInstance);
        AdaptiveDifficulty::Track(MaxPayne_GameMode::pInstance);
    });

    InitPostFX();

    pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 81 EC 14 02 00 00 53 55 33 DB 56 57 8B E9"); // MaxPayne_GameMode::load
    AdaptiveDifficulty::shLoad = safetyhook::create_inline(pattern.get_first(), AdaptiveDifficulty::load); //0x454230

    pattern = hook::pattern("55 8B EC 83 EC 0C 53 56 8B F1 57 89 75 F4 E8 ? ? ? ? 8B 45 08 83 C6 33 83 C0 2C"); // MaxPayne_GraphicNovelPage::show
    MaxPayne_GraphicNovelPage::shShow = safetyhook::create_inline(pattern.get_first(), MaxPayne_GraphicNovelPage::show); //0x4A8860

    // Menu and graphic novel cursor bounds, see UpdateCursorBounds. X_MenuModeBase::update and
    // MaxPayne_GraphicNovelMode::update clamp the cursor with the same inlined code, which loads the
    // bounds from constants shared with other code, so its loads are pointed at the fix's own.
    pattern = hook::pattern("D9 05 ? ? ? ? D8 5D C4 DF E0 F6 C4 01 74 08 D9 05 ? ? ? ? EB 1B D9 45 C4 D9 05 ? ? ? ? D8 5D C4 DF E0 F6 C4 41 75 08 DD D8 D9 05 ? ? ? ? D9 55 C4 D9 05 ? ? ? ? D8 5D C8 DF E0 F6 C4 01 74 08 D9 05 ? ? ? ? EB 1B D9 45 C8 D9 05 ? ? ? ? D8 5D C8 DF E0 F6 C4 41 75 08 DD D8 D9 05");
    pattern.count(2).for_each_result([](hook::pattern_match match) //0x631681, 0x49BBA2
    {
        injector::WriteMemory(match.get<void>(2), &CursorBounds.fRight, true);
        injector::WriteMemory(match.get<void>(18), &CursorBounds.fRight, true);
        injector::WriteMemory(match.get<void>(29), &CursorBounds.fLeft, true);
        injector::WriteMemory(match.get<void>(47), &CursorBounds.fLeft, true);
        injector::WriteMemory(match.get<void>(56), &CursorBounds.fBottom, true);
        injector::WriteMemory(match.get<void>(72), &CursorBounds.fBottom, true);
        injector::WriteMemory(match.get<void>(83), &CursorBounds.fTop, true);
        injector::WriteMemory(match.get<void>(101), &CursorBounds.fTop, true);
    });

    pattern = hook::pattern("D9 87 FA 02 00 00 D8 A7 F6 02 00 00 C7 05");
    static auto getFirstSceneToRenderHook = safetyhook::create_mid(pattern.get_first(), X_LevelRuntimeCamera::getFirstSceneToRenderHook);

    pattern = hook::pattern("D9 86 FA 02 00 00 D8 A6 F6 02 00 00 DE F9 74");
    static auto getSceneToRenderHook = safetyhook::create_mid(pattern.get_first(), X_LevelRuntimeCamera::getSceneToRenderHook);

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

    // End of the frame, before P_Driver::endScene: the bars the widescreen fix adds are drawn here with
    // P_Driver::clearScreen, the same way the game draws its letterbox bars
    pattern = hook::pattern("56 8B F1 8B 4E 0C FF 15 ? ? ? ? 8A 46 28 84 C0 C6 46 37 00");
    static auto FrameEndHook = safetyhook::create_mid(pattern.get_first(3), [](SafetyHookContext& regs)
    {
        if (bGameViewRendered)
            Cinematic::DrawPillarboxBorders();

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
    });

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

    // Loading screens cover the 4:3 area and leave the previous frame on the sides
    pattern = hook::pattern("E8 ? ? ? ? 8B 0D ? ? ? ? 8B 09 FF 15 ? ? ? ? 8B 15 ? ? ? ? 8B 0A FF 15"); // X_ProgressBar::updateProgressBar, before P_Driver::endScene
    static auto X_ProgressBarupdateProgressBarHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
    {
        RefreshScreenResolution();
        auto nWidth = static_cast<int32_t>(Screen.fHudOffsetReal);
        DrawPillarboxBars(nWidth, nWidth);
        UpdateVibration();
    }); //582A18

    //screenshots aspect ratio
    static float fScreenShotHeight = 0.0f;
    pattern = hook::pattern("89 93 ? ? ? ? D9 05 ? ? ? ? D9 05 ? ? ? ? D9 9B");
    static auto off_6301B6 = pattern.get_first(32);
    struct SaveScrHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ebx + 0x10E) = regs.edx;
            if (!fScreenShotHeight && Screen.fAspectRatio)
            {
                fScreenShotHeight = **(float**)off_6301B6 * ((4.0f / 3.0f) / Screen.fAspectRatio);
                injector::WriteMemory(off_6301B6, &fScreenShotHeight, true);
            }
        }
    }; injector::MakeInline<SaveScrHook>(pattern.get_first(0), pattern.get_first(6)); //630196

    //savegame date format
    static auto fmt = iniReader.ReadString("MISC", "SaveStringFormat", "%a, %b %d %Y, %H:%M");
    pattern = hook::pattern("68 ? ? ? ? 8D 54 24 24 68 ? ? ? ? 52"); //411091
    injector::WriteMemory(pattern.get_first(1), fmt.data(), true);

    bool BorderlessWindowedMode = iniReader.ReadInteger("MAIN", "BorderlessWindowedMode", 1) != 0;
    if (BorderlessWindowedMode)
    {
        pattern = hook::pattern("83 EC 08 53 55 56 57 8B F1 E8 ? ? ? ? 8B 78");
        shsub_40D040 = safetyhook::create_inline(pattern.get_first(), sub_40D040);
    }

    // Intro video, gamepad buttons skip it too
    static auto pVideoPeekMessageA = &VideoPeekMessageA;
    pattern = hook::pattern("8B 35 ? ? ? ? 8B 3D ? ? ? ? 38 5D 0B 0F 85"); // mov esi, ds:PeekMessageA
    injector::WriteMemory(pattern.get_first(2), &pVideoPeekMessageA, true); //0x409E77

    InitInput();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        ReadSettings();
        InitSettingsFile();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("0F 84 ? ? ? ? E8 ? ? ? ? 8B 48 04 68 ? ? ? ? 56 89"));
        CallbackHandler::RegisterCallback(L"E2MFC.dll", InitE2MFC);
        CallbackHandler::RegisterCallback(L"E2_D3D8_DRIVER_MFC.dll", []() { InitE2_D3D8_DRIVER_MFC(); InitPostFXDriver(); });
        CallbackHandler::RegisterModuleUnloadCallback(L"E2_D3D8_DRIVER_MFC.dll", []() { BorderlessWindowedHook.reset(); shDllMainHook.reset(); ShutdownPostFXDriver(); });
        CallbackHandler::RegisterCallback(L"Xidi.32.dll", InitXidi);
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}
