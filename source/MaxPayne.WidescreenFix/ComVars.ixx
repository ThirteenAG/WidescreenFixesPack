module;

#include "stdafx.h"
#include "minidx8/d3d8.h"

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
    // real aspect ratio: Hor+ above 4:3, Vert+ below it (same idea as the mobile release's
    // AspectRatioXMult/AspectRatioYMult).
    float fAspectScaleX = 1.0f;
    float fAspectScaleY = 1.0f;
    // 2D stays inside the centered 4:3 area; this is the distance from it to a screen edge
    float fFullscreenExtension;  // in 640x480 virtual units
    float fHudOffsetReal;        // in pixels
    float fWidescreenHudOffset;  // virtual units the edge anchored HUD moves outwards
    std::optional<float> fHudAspectRatioConstraint;
    float fFOVFactor = 1.0f;
    uint32_t nGeneration;        // changes whenever the values above do
    bool bDrawBordersToFillGap;
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

    Screen.fFullscreenExtension = (Screen.fAspectScaleX - 1.0f) * 320.0f;
    Screen.fHudOffsetReal = std::max(0.0f, (Screen.fWidth - Screen.fHeight * fDefaultAspectRatio) / 2.0f);

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

export namespace MaxPayne_GameMode
{
    enum
    {
        LEVEL_SETTINGS = 0xF8, // X_SharedDB entry of the level being played, the same object every time it's played
        GLOBAL_CINEMATIC_SETTINGS = 0x1230,
        SNIPER_ZOOM_STATE = 0x124C, // 0 when not zooming
        CURRENT_HEIGHT_MULTIPLIER = 0x1260,
        LEVEL = 0x1080,
        PAUSED = 0x12CE, // set by MaxPayne_GameMode::pause, e.g. quickload/quicksave prompts
    };

    // Set every frame the game view renders
    uint8_t* pInstance = nullptr;

    // MaxPayne_GameMode::getPlayerCharacterInScene
    uint8_t* GetPlayerCharacter(uint8_t* pGameMode = pInstance)
    {
        if (!pGameMode)
            return nullptr;

        auto pLevel = *(uint8_t**)(pGameMode + LEVEL);
        return pLevel ? *(uint8_t**)(pLevel + 8) : nullptr;
    }

    bool IsSniperZooming()
    {
        return pInstance && *(int32_t*)(pInstance + SNIPER_ZOOM_STATE) != 0;
    }
}

export namespace X_Character
{
    // X_Character::accessCharacterProperties
    constexpr ptrdiff_t CHARACTER_PROPERTIES = 0x22E;

    // What adaptive difficulty rates the player's play of a level by, counted from the start of it
    constexpr ptrdiff_t DEATHS = 0x75C;
    constexpr ptrdiff_t HEALTH_SUM = 0x760;         // health and painkillers, added every second of play
    constexpr ptrdiff_t PLAY_TIME = 0x764;          // seconds, standing still for long isn't counted
    constexpr ptrdiff_t PLAY_TIME_FRACTION = 0x768; // of the second not added yet

    bool IsPlayerCharacterProperties(uint8_t* pCharacterProperties)
    {
        auto pPlayer = MaxPayne_GameMode::GetPlayerCharacter();
        return pPlayer && pCharacterProperties == pPlayer + CHARACTER_PROPERTIES;
    }
}

export namespace X_CharacterProperties
{
    enum
    {
        CURRENT_WEAPON = 0x7B, // the WEAPONID of the weapon in hand
    };

    // WEAPONIDs from database\weaponid.h. The rest are no weapon, the lead pipe and the baseball bat,
    // molotovs, grenades and painkillers.
    bool IsHoldingGun(uint8_t* _this)
    {
        enum
        {
            WEAPONID_BERETTA = 3,
            WEAPONID_JACKHAMMER = 11,
            WEAPONID_M79 = 14,
            WEAPONID_SNIPER = 15,
        };

        auto nWeaponID = *(int32_t*)(_this + CURRENT_WEAPON);
        return (nWeaponID >= WEAPONID_BERETTA && nWeaponID <= WEAPONID_JACKHAMMER) || nWeaponID == WEAPONID_M79 || nWeaponID == WEAPONID_SNIPER;
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
    void* ForwardBackward = nullptr;
    void* LeftRight = nullptr;
    void* Shoot = nullptr;
    void* Reload = nullptr;
    void* Jump = nullptr;
    void* Crouch = nullptr;
    void* DodgeLeft = nullptr;
    void* DodgeRight = nullptr;
    void* DodgeForward = nullptr;
    void* DodgeBackward = nullptr;
    void* DodgeModifier = nullptr;
    void* AimUpDown = nullptr;
    void* AimLeftRight = nullptr;
    void* Use = nullptr;
    void* Slot1 = nullptr;
    void* Slot2 = nullptr;
    void* Slot3 = nullptr;
    void* Slot4 = nullptr;
    void* Slot5 = nullptr;
    void* Slot6 = nullptr;
    void* BestWeapon = nullptr;
    void* NextPreviousWeapon = nullptr;
    void* Pause = nullptr;
    void* Painkiller = nullptr;
    void* SniperZoom = nullptr;
    void* SlowMotion = nullptr;
    void* BulletTime = nullptr;
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

    // Aim sensitivity multiplier MaxPayne_GameMode::update sets from the sniper zoom, 1 when not zooming
    float GetZoomSensitivity()
    {
        auto pMouse = X_Input::getMouse ? (uint8_t*)X_Input::getMouse() : nullptr;
        return pMouse ? *(float*)(pMouse + 0x87) : 1.0f;
    }
}

export namespace P_BaseObject
{
    enum
    {
        LOCAL_MATRIX = 0x2C, // M_Matrix4x3: the right, up and forward axes, then the position
        WORLD_MATRIX = 0x5C, // the same in world space, up to date after calculateObjectToWorldMatrix
    };

    void(__fastcall* invalidateMatrices)(void* _this, void* edx) = nullptr;
    void(__fastcall* calculateObjectToWorldMatrix)(void* _this, void* edx) = nullptr;

    // Virtual, in object space
    const float* getBoundingBoxMin(uint8_t* _this)
    {
        auto pVTable = *(void***)_this;
        return reinterpret_cast<const float* (__fastcall*)(void*, void*)>(pVTable[10])(_this, nullptr);
    }

    const float* getBoundingBoxMax(uint8_t* _this)
    {
        auto pVTable = *(void***)_this;
        return reinterpret_cast<const float* (__fastcall*)(void*, void*)>(pVTable[11])(_this, nullptr);
    }
}

export namespace P_VirtualObject
{
    enum
    {
        SORT_PRIORITY = 0x110, // the order 2D objects are drawn in
    };
}

export namespace KF_ObjectAnimation
{
    // Virtual
    uint32_t getTotalMeshes(uint8_t* _this)
    {
        auto pVTable = *(void***)_this;
        return reinterpret_cast<uint32_t(__fastcall*)(void*, void*)>(pVTable[24])(_this, nullptr);
    }

    uint8_t* getMesh(uint8_t* _this, uint32_t nIndex)
    {
        auto pVTable = *(void***)_this;
        return reinterpret_cast<uint8_t* (__fastcall*)(void*, void*, uint32_t)>(pVTable[25])(_this, nullptr, nIndex);
    }
}

export namespace P_Sprite
{
    enum
    {
        WIDTH = 0x144,
        HEIGHT = 0x148,
        REFERENCE_POINT = 0x160,
        SCREEN_POSITION_X = 0x188, // of the reference point, in 640x480 virtual units
        SCREEN_POSITION_Y = 0x18C,
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
    float fExtensionX = bWholeScreen ? Screen.fFullscreenExtension : 0.0f;
    float fExtensionY = bWholeScreen ? (Screen.fAspectScaleY - 1.0f) * 240.0f : 0.0f; // screens narrower than 4:3
    CursorBounds.fLeft = -fExtensionX;
    CursorBounds.fTop = -fExtensionY;
    CursorBounds.fRight = 640.0f + fExtensionX;
    CursorBounds.fBottom = 480.0f + fExtensionY;
}

// Graphic novels:
// - the cursor stays hidden until the mouse moves
// - outside original framing, the playback controls (indicators, the band behind them
//   and the tooltip of the indicator under the cursor), which would cover the bottom of the page,
//   only show up while the cursor is there
export namespace MaxPayne_GraphicNovelMode
{
    enum
    {
        CONTROLS_BACKGROUND = 0xB7, // X_AdvSprite*, the band
        CURSOR = 0xF0,              // X_AdvSprite*
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
