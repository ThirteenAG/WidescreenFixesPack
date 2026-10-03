module;

#include "stdafx.h"

export module ComVars;

export struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    // Multipliers for the tangents P_Camera::validate derives from its FOV. The engine always
    // projects a 640x480 virtual screen, so these turn that 4:3 frustum into one that matches the
    // real aspect ratio: Hor+ above 4:3, Vert+ below it.
    float fAspectScaleX = 1.0f;
    float fAspectScaleY = 1.0f;
    // 2D stays inside the centered 4:3 area; these are the distances from it to the screen edges
    float fFullscreenExtensionX; // in 640x480 virtual units
    float fFullscreenExtensionY;
    float fHudOffsetReal;        // in pixels, left and right of the 4:3 area
    float fHudOffsetRealY;       // in pixels, above and below it
    float fWidescreenHudOffset;  // virtual units the edge anchored HUD moves outwards
    std::optional<float> fHudAspectRatioConstraint;
    float fFOVFactor = 1.0f;
    uint32_t nGeneration;        // changes whenever the values above do
    bool bDrawBordersForCameraOverlay;
    bool bGraphicNovelMode;      // true: original framing, false: whole pages as large as the screen allows
} Screen;

export void UpdateScreenResolution(int32_t nWidth, int32_t nHeight)
{
    if (nWidth <= 0 || nHeight <= 0 || (nWidth == Screen.nWidth && nHeight == Screen.nHeight))
        return;

    Screen.nWidth = nWidth;
    Screen.nHeight = nHeight;
    Screen.fWidth = static_cast<float>(nWidth);
    Screen.fHeight = static_cast<float>(nHeight);
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;

    constexpr float fDefaultAspectRatio = 4.0f / 3.0f;
    if (Screen.fAspectRatio >= fDefaultAspectRatio)
    {
        Screen.fAspectScaleX = Screen.fAspectRatio / fDefaultAspectRatio;
        Screen.fAspectScaleY = 1.0f;
    }
    else
    {
        Screen.fAspectScaleX = 1.0f;
        Screen.fAspectScaleY = fDefaultAspectRatio / Screen.fAspectRatio;
    }

    Screen.fFullscreenExtensionX = (Screen.fAspectScaleX - 1.0f) * 320.0f;
    Screen.fFullscreenExtensionY = (Screen.fAspectScaleY - 1.0f) * 240.0f;
    Screen.fHudOffsetReal = std::max(0.0f, (Screen.fWidth - Screen.fHeight * fDefaultAspectRatio) / 2.0f);
    Screen.fHudOffsetRealY = std::max(0.0f, (Screen.fHeight - Screen.fWidth / fDefaultAspectRatio) / 2.0f);

    Screen.fWidescreenHudOffset = std::max(0.0f, -CalculateWidescreenOffset(Screen.fWidth, Screen.fHeight, 640.0f, 480.0f));
    if (Screen.fHudAspectRatioConstraint.has_value())
    {
        float value = Screen.fHudAspectRatioConstraint.value();
        if (value < 0.0f || value > (32.0f / 9.0f))
            Screen.fWidescreenHudOffset = value;
        else
        {
            value = ClampHudAspectRatio(value, Screen.fAspectRatio);
            Screen.fWidescreenHudOffset = std::max(0.0f, -CalculateWidescreenOffset(Screen.fHeight * value, Screen.fHeight, 640.0f, 480.0f));
        }
    }

    ++Screen.nGeneration;
}

export SafetyHookInline shDllMainHook = {};
export safetyhook::MidHook BorderlessWindowedHook = {};

export namespace X_Crosshair
{
    GameRef<bool> sm_bCameraPathRunning;
}

export namespace MP_GameMode
{
    enum
    {
        PLAYER = 0x10A8,                    // holds the player X_Character at +4
        GLOBAL_CONFIG = 0x1220,             // X_GlobalConfig
        SNIPER_ZOOM_STATE = 0x1244,         // 0 when not zooming, 1 and 2 while the scope is up
        CURRENT_HEIGHT_MULTIPLIER = 0x1254, // letterbox transition, 1 when there are no bars
        PAUSED = 0x12CE,                    // quickload/quicksave prompts
    };

    // Set every frame the game view renders
    uint8_t* pInstance = nullptr;

    uint8_t* GetPlayerCharacter()
    {
        if (!pInstance)
            return nullptr;

        auto pPlayer = *(uint8_t**)(pInstance + PLAYER);
        return pPlayer ? *(uint8_t**)(pPlayer + 4) : nullptr;
    }

    bool IsSniperZooming()
    {
        return pInstance && *(int32_t*)(pInstance + SNIPER_ZOOM_STATE) != 0;
    }

    // Same test the game uses for X_Character::setSniperZoomOn, which brings up the scope overlay
    bool IsSniperScopeOn()
    {
        if (!pInstance)
            return false;

        auto nState = *(int32_t*)(pInstance + SNIPER_ZOOM_STATE);
        return nState == 1 || nState == 2;
    }

    // X_GlobalCinematicSettings::getWideScreenMultiplier(X_GlobalConfig::getCinematicSettings())
    float GetWideScreenMultiplier(uint8_t* pGameMode)
    {
        auto pConfig = *(uint8_t**)(pGameMode + GLOBAL_CONFIG);
        auto pCinematicSettings = pConfig ? *(uint8_t**)(pConfig + 0x11) : nullptr;
        return pCinematicSettings ? *(float*)(pCinematicSettings + 0x0C) : 1.0f;
    }
}

export namespace X_Character
{
    // X_Character::accessCharacterProperties
    constexpr ptrdiff_t CHARACTER_PROPERTIES = 0x1E2;

    bool IsPlayerCharacterProperties(uint8_t* pCharacterProperties)
    {
        auto pPlayer = MP_GameMode::GetPlayerCharacter();
        return pPlayer && pCharacterProperties == pPlayer + CHARACTER_PROPERTIES;
    }
}

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

        state.fWideScreenMultiplier = MP_GameMode::GetWideScreenMultiplier(pGameMode);
        if (state.fWideScreenMultiplier > 0.0f && state.fWideScreenMultiplier < 1.0f)
        {
            float fCurrent = *(float*)(pGameMode + MP_GameMode::CURRENT_HEIGHT_MULTIPLIER);
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

    // Called every frame the game view renders, before MP_HUDMode draws the letterbox bars
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

    // Replaces the current height multiplier getter in MP_HUDMode's render function, which draws
    // the letterbox bars and moves subtitles clear of them
    float __fastcall GetBordersHeightMultiplier(void* pGameInformation, void* edx)
    {
        return 1.0f - CurrentBorders.fLetterbox;
    }
}

// Set when MP_GameMode renders the game view, cleared at the end of each frame
export bool bGameViewRendered = false;

export namespace MaxPayne_HUDFadeLayer
{
    // The sprite HUD fades and mode switch fades are drawn with, it covers the 4:3 area
    uint8_t* pSprite = nullptr;
}

export namespace P_BaseObject
{
    enum
    {
        LOCAL_MATRIX = 0x38, // M_Matrix4x3: the right, up and forward axes, then the position
        WORLD_MATRIX = 0x68, // the same in world space, up to date after calculateObjectToWorldMatrix
    };

    void(__fastcall* invalidateMatrices)(void* _this, void* edx) = nullptr;
    void(__fastcall* calculateObjectToWorldMatrix)(void* _this, void* edx) = nullptr;

    // Virtual, in object space
    const float* getBoundingBoxMin(uint8_t* _this)
    {
        auto pVTable = *(void***)_this;
        return reinterpret_cast<const float* (__fastcall*)(void*, void*)>(pVTable[11])(_this, nullptr);
    }

    const float* getBoundingBoxMax(uint8_t* _this)
    {
        auto pVTable = *(void***)_this;
        return reinterpret_cast<const float* (__fastcall*)(void*, void*)>(pVTable[12])(_this, nullptr);
    }
}

export namespace P_Camera
{
    enum
    {
        VIEWPORT = 0x1D0, // left, right, top and bottom
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

export namespace KF_ObjectAnimation
{
    // Virtual
    uint32_t getTotalMeshes(uint8_t* _this)
    {
        auto pVTable = *(void***)_this;
        return reinterpret_cast<uint32_t(__fastcall*)(void*, void*)>(pVTable[25])(_this, nullptr);
    }

    uint8_t* getMesh(uint8_t* _this, uint32_t nIndex)
    {
        auto pVTable = *(void***)_this;
        return reinterpret_cast<uint8_t* (__fastcall*)(void*, void*, uint32_t)>(pVTable[26])(_this, nullptr, nIndex);
    }
}

// Graphic novel pages are 3D scenes, meshes and a camera to show them with, which the page copies to
// the camera of the graphic novel mode when it shows. Original framing keeps that camera: the page
// across the 4:3 width at the top, the playback controls below it. Otherwise the camera moves to the
// middle of the page and zooms in as far as the whole page, its black border included, still fits
// on the screen.
export namespace MaxPayne_GraphicNovelPage
{
    enum
    {
        OBJECT_ANIMATION = 0x2F, // KF2::KF_ObjectAnimation*, the meshes of the page
        CAMERA_MATRIX = 0x37,    // M_Matrix4x3
        FOV = 0x6F,
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

    // From the bounding boxes of the meshes, the page doesn't animate them
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

export std::string CurrentGameMode;

// Written by the game thread, read by Xidi's polling thread
export enum class eGamepadProfile : int32_t
{
    Menu,
    Main,
    Pause,
};
export std::atomic<eGamepadProfile> GamepadProfile = eGamepadProfile::Menu;

export namespace MaxPayne_ConfiguredInput
{
    uintptr_t* sm_control;

    enum eControls
    {
        RUN,
        WALKBACK,
        STRAFELEFT,
        STRAFERIGHT,
        SHOOT,
        ATTACK,
        RELOAD,
        JUMP,
        CROUCH,
        DODGELEFT,
        DODGERIGHT,
        DODGEFORWARD,
        DODGEBACKWARD,
        DODGEMODIFIER,
        USE,
        SLOT0,
        SLOT1,
        SLOT2,
        SLOT3,
        SLOT4,
        SLOT5,
        SLOT6,
        SLOT7,
        SLOT8,
        SLOT9,
        SLOT10,
        BESTWEAPON,
        NEXTWEAPON,
        PREVIOUSWEAPON,
        PAUSE,
        PAINKILLER,
        SNIPERZOOM,
        SLOWMOTION,
        BULLETTIME,
    };
}

export namespace X_Input
{
    void* (*getMouse)() = nullptr;
}

export namespace X_InputDeviceMouse
{
    enum
    {
        // Sums of the raw DirectInput movement, only moving the mouse changes them
        TOTAL_MOVEMENT_X = 0x6B,
        TOTAL_MOVEMENT_Y = 0x6F,
    };

    // X_InputDeviceMouse::getSensitivityMultiplier, set from the sniper zoom, 1 when not zooming
    float GetZoomSensitivity()
    {
        auto pMouse = X_Input::getMouse ? (uint8_t*)X_Input::getMouse() : nullptr;
        return pMouse ? *(float*)(pMouse + 0x87) : 1.0f;
    }
}

export namespace P_VirtualObject
{
    enum
    {
        SORT_PRIORITY = 0x11C, // the order 2D objects are drawn in
    };
}

export namespace P_Sprite
{
    enum
    {
        WIDTH = 0x15C,
        HEIGHT = 0x160,
        REFERENCE_POINT = 0x178,
        UV = 0x17C,                // u and v of the top left, top right, bottom right and bottom left corners
        SCREEN_POSITION_X = 0x1A0, // of the reference point, in 640x480 virtual units
        SCREEN_POSITION_Y = 0x1A4,
    };

    // P_BitmapInterface::ReferencePoint, the corner the screen position is of
    constexpr uint32_t REFERENCE_POINT_BOTTOM_LEFT = 0;
    constexpr uint32_t REFERENCE_POINT_TOP_LEFT = 2;
}

// Menus and graphic novels keep the mouse cursor inside these, the 640x480 area in the original game
export struct CursorBounds
{
    float fLeft = 0.0f;
    float fTop = 0.0f;
    float fRight = 640.0f;
    float fBottom = 480.0f;
} CursorBounds;

// Called every frame, before the active mode updates, with whether the cursor can move across the
// whole screen, which reaches past 640x480 on screens that aren't 4:3
export void UpdateCursorBounds(bool bWholeScreen)
{
    float fExtensionX = bWholeScreen ? Screen.fFullscreenExtensionX : 0.0f;
    float fExtensionY = bWholeScreen ? Screen.fFullscreenExtensionY : 0.0f;
    CursorBounds.fLeft = -fExtensionX;
    CursorBounds.fTop = -fExtensionY;
    CursorBounds.fRight = 640.0f + fExtensionX;
    CursorBounds.fBottom = 480.0f + fExtensionY;
}

// Stand-ins for the min and max calls menus and graphic novels keep the cursor inside 640x480 with
export float __stdcall ClampCursorRight(float, float fX) { return std::min(fX, CursorBounds.fRight); }
export float __stdcall ClampCursorLeft(float, float fX) { return std::max(fX, CursorBounds.fLeft); }
export float __stdcall ClampCursorBottom(float, float fY) { return std::min(fY, CursorBounds.fBottom); }
export float __stdcall ClampCursorTop(float, float fY) { return std::max(fY, CursorBounds.fTop); }

// Graphic novels:
// - the cursor stays hidden until the mouse moves
// - outside original framing, the playback controls (indicators, the band behind them
//   and the tooltip of the indicator under the cursor), which would cover the bottom of the page,
//   only show up while the cursor is there
export namespace MP_GraphicNovelMode
{
    enum
    {
        CONTROLS_BACKGROUND = 0xCE, // X_AdvSprite*, the band
        CURSOR = 0x107,             // X_AdvSprite*
    };

    // Sort priorities the game creates these with, no other 2D objects use them
    enum
    {
        PRIORITY_CONTROLS_BACKGROUND = 120,
        PRIORITY_TOOLTIP = 130,
        PRIORITY_INDICATOR = 150,
        PRIORITY_CURSOR = 200,
    };

    bool bHideCursor = false;
    bool bHideControls = false;

    // Called every frame, before the mode updates, with the active graphic novel mode, nullptr in other modes
    void Update(uint8_t* pMode)
    {
        bool bPageFillsScreen = pMode && !Screen.bGraphicNovelMode;

        // A graphic novel starts with the cursor and the controls hidden, even with the cursor left
        // over the controls, until the mouse moves. The game places the cursor itself then, so its
        // position can't tell.
        static uint8_t* pPrevMode = nullptr;
        static float fPrevMovementX = 0.0f;
        static float fPrevMovementY = 0.0f;
        static bool bMouseMoved = false;

        auto pMouse = X_Input::getMouse ? (uint8_t*)X_Input::getMouse() : nullptr;
        float fMovementX = pMouse ? *(float*)(pMouse + X_InputDeviceMouse::TOTAL_MOVEMENT_X) : 0.0f;
        float fMovementY = pMouse ? *(float*)(pMouse + X_InputDeviceMouse::TOTAL_MOVEMENT_Y) : 0.0f;
        if (pMode != pPrevMode)
            bMouseMoved = false;
        else if (fMovementX != fPrevMovementX || fMovementY != fPrevMovementY)
            bMouseMoved = true;
        pPrevMode = pMode;
        fPrevMovementX = fMovementX;
        fPrevMovementY = fMovementY;

        bHideCursor = pMode && !bMouseMoved;
        bHideControls = false;
        if (!bPageFillsScreen)
            return;

        auto pBackground = *(uint8_t**)(pMode + CONTROLS_BACKGROUND);
        auto pCursor = *(uint8_t**)(pMode + CURSOR);
        if (!pBackground || !pCursor)
            return;

        // Anywhere across the screen at the height of the band counts, the cursor reaches the sides
        // of the page too. The band is placed by its bottom left corner, the cursor by its tip.
        float fTop = *(float*)(pBackground + P_Sprite::SCREEN_POSITION_Y) - *(float*)(pBackground + P_Sprite::HEIGHT);
        bool bCursorAtControls = *(float*)(pCursor + P_Sprite::SCREEN_POSITION_Y) >= fTop;

        bHideControls = !bMouseMoved || !bCursorAtControls;
    }

    // For P_Sprite::executeAlways
    bool IsSpriteHidden(uint8_t* pSprite)
    {
        if (!bHideCursor && !bHideControls)
            return false;

        auto nPriority = *(int32_t*)(pSprite + P_VirtualObject::SORT_PRIORITY);
        auto nReferencePoint = *(uint32_t*)(pSprite + P_Sprite::REFERENCE_POINT);
        if (nReferencePoint == P_Sprite::REFERENCE_POINT_TOP_LEFT)
        {
            if (nPriority == PRIORITY_CURSOR)
                return bHideCursor;
            if (nPriority == PRIORITY_INDICATOR)
                return bHideControls;
        }
        else if (nReferencePoint == P_Sprite::REFERENCE_POINT_BOTTOM_LEFT && nPriority == PRIORITY_CONTROLS_BACKGROUND)
        {
            return bHideControls;
        }
        return false;
    }

    // For P_Text::executeAlways
    bool IsTextHidden(uint8_t* pText)
    {
        return bHideControls && *(int32_t*)(pText + P_VirtualObject::SORT_PRIORITY) == PRIORITY_TOOLTIP;
    }
}
