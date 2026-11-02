module;

#include "stdafx.h"
#include <xmmintrin.h>

export module ComVars;

export struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int32_t Width43;
    float fWidth43;
    float fHudOffset;
    std::optional<float> fHudAspectRatioConstraint;
    float fWidescreenHudOffset;
    float fWidescreenHudOffsetRadar;
} Screen;

// Game state
export GameRef<bool> bPause;
export GameRef<bool> bCutscene;
export GameRef<uint32_t> nLoading;

// The game's display settings (prefs.dat, startup dialog)
export GameRef<int> nResolutionWidth;
export GameRef<int> nResolutionHeight;
export GameRef<int> bWindowed;
export GameRef<int> bShowSettingsDialog;
export GameRef<bool> bAppInactive;
// The game's settings object: narrow cinematic bars at +0x1010
export GameRef<uint8_t*> pGameSettings;

// The game's Direct3D 8 device
export GameRef<IUnknown*> pD3DDevice;
// The device the movie player draws with
export GameRef<void*> pMovieDevice;

// The mapped input state of the pad: held and edge bits at +4, sticks at +8
export GameRef<uint8_t*> pMappedPad;

// World camera state
export GameRef<void*> pCameraWorld;
export GameRef<void*> pPlayer;
export GameRef<uint8_t> bCameraBlending;

// Mouse camera
export GameRef<float> fMouseSens;
export GameRef<uint8_t> byte_850D8A;
export GameRef<__m128> xmmword_778BF0;
export GameRef<__m128> xmmword_778BD0;
export GameRef<__m128> xmmword_778BA0;
export GameRef<float> float_70F390;
export GameRef<uint8_t*> dword_79430C;

// Post-processing: the render pass list and its current value
export struct ListNode
{
    struct ListNode* next;
    uint32_t _reserved;
    uint32_t data;
    int (__cdecl* cb)(uint32_t data, float* arg);
    float value;
};

export GameRef<ListNode*> g_listHead;
export GameRef<float> g_currentVal;

// The game's three DirectInput action formats (on foot, driving, menus)
export struct DIAction
{
    uintptr_t data;
    DWORD semantic;
    DWORD flags;
    uintptr_t name;
    GUID instance;
    DWORD object;
    DWORD how;
};
static_assert(sizeof(DIAction) == 40);

export struct DIActionFormat
{
    const char* name;
    DWORD genre;
    DWORD count;
    DIAction* actions;
};

export GameRef<std::array<DIActionFormat, 3>> aActionFormats;

export enum CameraMode
{
    CameraOnFoot = 0,
    CameraPrecisionAim = 6,
    CameraCrosshair = 11,
    CameraInVehicle = 13,
};

export int8_t CurrentCameraMode = -1;

export int CurrentCaseName = 0;
export int CurrentMissionName = 0;

export enum eCaseNames
{
    Mision0,
    Intro,
    BC01,
    BC02,
    BC03,
    BC04,
    EndGame,
    CIKingMission,
    CIMadamMission,
    CICabbieMission,
    FC,
    RedmanBonus,
    Police_Challenges,
    StreetRacing,
    SC03,
};

export enum eMissionNames_BC03
{
    BC3_Prologue,
    BC3_M0F,
    BC3_M1,
    BC3_M1F,
    BC3_M2,
    BC3_M2F,
    BC3_M4,
    BC3_M4F,
    BC3_M5,
    BC3_M5F,
    BC3_M6,
    BC3_M7,
    BC3_M8,
};
