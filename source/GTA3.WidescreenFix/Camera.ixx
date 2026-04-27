module;

#include <stdafx.h>
#include "common.h"

export module Camera;

import Placeable;
import Timer;
import Skeleton;
import Entity;
import Vehicle;

using CAutomobile = void;
using CPed = void;
using CGarage = void;

enum
{
    NUMBER_OF_VECTORS_FOR_AVERAGE = 2,
    MAX_NUM_OF_SPLINETYPES = 4,
    MAX_NUM_OF_NODES = 800 // for trains
};

enum
{
    CAMCONTROL_GAME,
    CAMCONTROL_SCRIPT,
    CAMCONTROL_OBBE
};

export enum
{
    MOTION_BLUR_NONE = 0,
    MOTION_BLUR_SNIPER,
    MOTION_BLUR_LIGHT_SCENE,
    MOTION_BLUR_SECURITY_CAM,
    MOTION_BLUR_CUT_SCENE,
    MOTION_BLUR_INTRO,
    MOTION_BLUR_INTRO2,
    MOTION_BLUR_SNIPER_ZOOM,
    MOTION_BLUR_INTRO3,
    MOTION_BLUR_INTRO4,
};

enum
{
    FADE_0, // faded in
    FADE_1, // mid fade
    FADE_2, // faded out

    // Direction
    FADE_OUT = 0,
    FADE_IN,
    FADE_NONE
};

const float DefaultFOV = 70.0f;

struct CQueuedMode
{
    int16_t Mode;
    float Duration;
    int16_t MinZoom;
    int16_t MaxZoom;
};

class CCamPathSplines
{
public:
    enum
    {
        MAXPATHLENGTH = 800
    };
    float m_arr_PathData[MAXPATHLENGTH];
    CCamPathSplines(void);
};

struct CTrainCamNode
{
    CVector m_cvecCamPosition;
    CVector m_cvecPointToLookAt;
    CVector m_cvecMinPointInRange;
    CVector m_cvecMaxPointInRange;
    float m_fDesiredFOV;
    float m_fNearClip;
};

export class CCam
{
public:
    enum CameraMode : int
    {
        MODE_NONE = 0,
        MODE_TOPDOWN,
        MODE_GTACLASSIC,
        MODE_BEHINDCAR,
        MODE_FOLLOWPED,
        MODE_AIMING,
        MODE_DEBUG,
        MODE_SNIPER,
        MODE_ROCKETLAUNCHER,
        MODE_MODELVIEW,
        MODE_BILL,
        MODE_SYPHON,
        MODE_CIRCLE,
        MODE_CHEESYZOOM,
        MODE_WHEELCAM,
        MODE_FIXED,
        MODE_1STPERSON,
        MODE_FLYBY,
        MODE_CAM_ON_A_STRING,
        MODE_REACTION,
        MODE_FOLLOW_PED_WITH_BIND,
        MODE_CHRIS,
        MODE_BEHINDBOAT,
        MODE_PLAYER_FALLEN_WATER,
        MODE_CAM_ON_TRAIN_ROOF,
        MODE_CAM_RUNNING_SIDE_TRAIN,
        MODE_BLOOD_ON_THE_TRACKS,
        MODE_IM_THE_PASSENGER_WOOWOO,
        MODE_SYPHON_CRIM_IN_FRONT,
        MODE_PED_DEAD_BABY,
        MODE_PILLOWS_PAPS,
        MODE_LOOK_AT_CARS,
        MODE_ARRESTCAM_ONE,
        MODE_ARRESTCAM_TWO,
        MODE_M16_1STPERSON,
        MODE_SPECIAL_FIXED_FOR_SYPHON,
        MODE_FIGHT_CAM,
        MODE_TOP_DOWN_PED,
        MODE_SNIPER_RUNABOUT,
        MODE_ROCKETLAUNCHER_RUNABOUT,
        MODE_1STPERSON_RUNABOUT,
        MODE_M16_1STPERSON_RUNABOUT,
        MODE_FIGHT_CAM_RUNABOUT,
        MODE_EDITOR,
        MODE_HELICANNON_1STPERSON,
    };

    // --- flags ---
    bool bBelowMinDist = false;
    bool bBehindPlayerDesired = false;
    bool m_bCamLookingAtVector = false;
    bool m_bCollisionChecksOn = false;
    bool m_bFixingBeta = false;
    bool m_bTheHeightFixerVehicleIsATrain = false;
    bool LookBehindCamWasInFront = false;
    bool LookingBehind = false;
    bool LookingLeft = false;
    bool LookingRight = false;
    bool ResetStatics = false;
    bool Rotating = false;

    std::int16_t  Mode = MODE_NONE;
    std::uint32_t m_uiFinishTime = 0;

    int m_iDoCollisionChecksOnFrameNum = 0;
    int m_iDoCollisionCheckEveryNumOfFrames = 0;
    int m_iFrameNumWereAt = 0;
    int m_iRunningVectorArrayPos = 0;
    int m_iRunningVectorCounter = 0;
    int DirectionWasLooking = 0;

    float f_max_role_angle = 0.f;
    float f_Roll = 0.f;
    float f_rollSpeed = 0.f;
    float m_fSyphonModeTargetZOffSet = 0.f;
    float m_fRoadOffSet = 0.f;
    float m_fAmountFractionObscured = 0.f;
    float m_fAlphaSpeedOverOneFrame = 0.f;
    float m_fBetaSpeedOverOneFrame = 0.f;
    float m_fBufferedTargetBeta = 0.f;
    float m_fBufferedTargetOrientation = 0.f;
    float m_fBufferedTargetOrientationSpeed = 0.f;
    float m_fCamBufferedHeight = 0.f;
    float m_fCamBufferedHeightSpeed = 0.f;
    float m_fCloseInPedHeightOffset = 0.f;
    float m_fCloseInPedHeightOffsetSpeed = 0.f;
    float m_fCloseInCarHeightOffset = 0.f;
    float m_fCloseInCarHeightOffsetSpeed = 0.f;
    float m_fDimensionOfHighestNearCar = 0.f;
    float m_fDistanceBeforeChanges = 0.f;
    float m_fFovSpeedOverOneFrame = 0.f;
    float m_fMinDistAwayFromCamWhenInterPolating = 0.f;
    float m_fPedBetweenCameraHeightOffset = 0.f;
    float m_fPlayerInFrontSyphonAngleOffSet = 0.f;
    float m_fRadiusForDead = 0.f;
    float m_fRealGroundDist = 0.f;
    float m_fTargetBeta = 0.f;
    float m_fTimeElapsedFloat = 0.f;
    float m_fTransitionBeta = 0.f;
    float m_fTrueBeta = 0.f;
    float m_fTrueAlpha = 0.f;
    float m_fInitialPlayerOrientation = 0.f;

    float Alpha = 0.f;
    float AlphaSpeed = 0.f;
    float FOV = DefaultFOV;
    float FOVSpeed = 0.f;
    float Beta = 0.f;
    float BetaSpeed = 0.f;
    float Distance = 0.f;
    float DistanceSpeed = 0.f;
    float CA_MIN_DISTANCE = 0.f;
    float CA_MAX_DISTANCE = 0.f;
    float SpeedVar = 0.f;

    CVector m_cvecSourceSpeedOverOneFrame{};
    CVector m_cvecTargetSpeedOverOneFrame{};
    CVector m_cvecUpOverOneFrame{};
    CVector m_cvecTargetCoorsForFudgeInter{};
    CVector m_cvecCamFixedModeVector{};
    CVector m_cvecCamFixedModeSource{};
    CVector m_cvecCamFixedModeUpOffSet{};
    CVector m_vecLastAboveWaterCamPosition{};
    CVector m_vecBufferedPlayerBodyOffset{};

    CVector Front{};
    CVector Source{};
    CVector SourceBeforeLookBehind{};
    CVector Up{};
    CVector m_arrPreviousVectors[NUMBER_OF_VECTORS_FOR_AVERAGE]{};

    CEntity* CamTargetEntity = nullptr;

    float m_fCameraDistance = 0.f;
    float m_fIdealAlpha = 0.f;
    float m_fPlayerVelocity = 0.f;
    CAutomobile* m_pLastCarEntered = nullptr;
    CPed* m_pLastPedLookedAt = nullptr;
    bool m_bFirstPersonRunAboutActive = false;
};

export class CCamera : public CPlaceable
{
public:
    bool m_bAboveGroundTrainNodesLoaded = false;
    bool m_bBelowGroundTrainNodesLoaded = false;
    bool m_bCamDirectlyBehind = false;
    bool m_bCamDirectlyInFront = false;
    bool m_bCameraJustRestored = false;
    bool m_bcutsceneFinished = false;
    bool m_bCullZoneChecksOn = false;
    bool m_bFirstPersonBeingUsed = false;
    bool m_bUnknown = false;
    bool m_bIdleOn = false;
    bool m_bInATunnelAndABigVehicle = false;
    bool m_bInitialNodeFound = false;
    bool m_bInitialNoNodeStaticsSet = false;
    bool m_bIgnoreFadingStuffForMusic = false;
    bool m_bPlayerIsInGarage = false;
    bool m_bJustCameOutOfGarage = false;
    bool m_bJustInitalised = false;
    bool m_bJust_Switched = false;
    bool m_bLookingAtPlayer = false;
    bool m_bLookingAtVector = false;
    bool m_bMoveCamToAvoidGeom = false;
    bool m_bObbeCinematicPedCamOn = false;
    bool m_bObbeCinematicCarCamOn = false;
    bool m_bRestoreByJumpCut = false;
    bool m_bUseNearClipScript = false;
    bool m_bStartInterScript = false;
    bool m_bStartingSpline = false;
    bool m_bTargetJustBeenOnTrain = false;
    bool m_bTargetJustCameOffTrain = false;
    bool m_bUseSpecialFovTrain = false;
    bool m_bUseTransitionBeta = false;
    bool m_bUseScriptZoomValuePed = false;
    bool m_bUseScriptZoomValueCar = false;
    bool m_bWaitForInterpolToFinish = false;
    bool m_bItsOkToLookJustAtThePlayer = false;
    bool m_bWantsToSwitchWidescreenOff = false;
    bool m_WideScreenOn = false;
    bool m_1rstPersonRunCloseToAWall = false;
    bool m_bHeadBob = false;
    bool m_bFailedCullZoneTestPreviously = false;
    bool m_FadeTargetIsSplashScreen = false;
    bool WorldViewerBeingUsed = false;

    std::uint8_t  ActiveCam = 0;
    std::uint32_t m_uiCamShakeStart = 0;
    std::uint32_t m_uiFirstPersonCamLastInputTime = 0;
    std::uint32_t m_uiLongestTimeInMill = 0;
    std::uint32_t m_uiNumberOfTrainCamNodes = 0;
    std::uint8_t  m_uiTransitionJUSTStarted = 0;
    std::uint8_t  m_uiTransitionState = 0;
    std::uint32_t m_uiTimeLastChange = 0;
    std::uint32_t m_uiTimeWeEnteredIdle = 0;
    std::uint32_t m_uiTimeTransitionStart = 0;
    std::uint32_t m_uiTransitionDuration = 0;

    int m_BlurBlue = 0;
    int m_BlurGreen = 0;
    int m_BlurRed = 0;
    int m_BlurType = MOTION_BLUR_NONE;

    std::uint32_t unknown = 0; // music counter
    int m_iWorkOutSpeedThisNumFrames = 0;
    int m_iNumFramesSoFar = 0;
    int m_iCurrentTrainCamNode = 0;
    int m_motionBlur = 0;
    int m_imotionBlurAddAlpha = 0;
    int m_iCheckCullZoneThisNumFrames = 0;
    int m_iZoneCullFrameNumWereAt = 0;
    int WhoIsInControlOfTheCamera = CAMCONTROL_GAME;

    float CamFrontXNorm = 0.f;
    float CamFrontYNorm = 0.f;
    std::int32_t CarZoomIndicator = 0;   // FIX_BUGS: int32 (was float)
    float CarZoomValue = 0.f;
    float CarZoomValueSmooth = 0.f;
    float DistanceToWater = 0.f;

    // PS2_CAM_TRANSITION=false — these are present
    float FOVDuringInter = 0.f;
    float LODDistMultiplier = 1.f;
    float GenerationDistMultiplier = 1.f;
    float m_fAlphaSpeedAtStartInter = 0.f;
    float m_fAlphaWhenInterPol = 0.f;
    float m_fAlphaDuringInterPol = 0.f;
    float m_fBetaDuringInterPol = 0.f;
    float m_fBetaSpeedAtStartInter = 0.f;
    float m_fBetaWhenInterPol = 0.f;
    float m_fFOVWhenInterPol = 0.f;
    float m_fFOVSpeedAtStartInter = 0.f;
    float m_fStartingBetaForInterPol = 0.f;
    float m_fStartingAlphaForInterPol = 0.f;
    float m_PedOrientForBehindOrInFront = 0.f;
    float m_CameraAverageSpeed = 0.f;
    float m_CameraSpeedSoFar = 0.f;
    float m_fCamShakeForce = 0.f;
    float m_fCarZoomValueScript = 0.f;
    float m_fFovForTrain = 0.f;
    float m_fFOV_Wide_Screen = 0.f;
    float m_fNearClipScript = 0.f;
    float m_fOldBetaDiff = 0.f;
    float m_fPedZoomValue = 0.f;
    float m_fPedZoomValueScript = 0.f;
    float m_fPedZoomValueSmooth = 0.f;
    float m_fPositionAlongSpline = 0.f;
    float m_ScreenReductionPercentage = 0.f;
    float m_ScreenReductionSpeed = 0.f;
    float m_AlphaForPlayerAnim1rstPerson = 0.f;
    float Orientation = 0.f;
    std::int32_t PedZoomIndicator = 0;   // FIX_BUGS: int32 (was float)
    float PlayerExhaustion = 0.f;

    float SoundDistUp = 0.f, SoundDistLeft = 0.f, SoundDistRight = 0.f;
    float SoundDistUpAsRead = 0.f, SoundDistLeftAsRead = 0.f, SoundDistRightAsRead = 0.f;
    float SoundDistUpAsReadOld = 0.f, SoundDistLeftAsReadOld = 0.f, SoundDistRightAsReadOld = 0.f;

    float m_fWideScreenReductionAmount = 0.f;
    float m_fStartingFOVForInterPol = 0.f;
    float m_fMouseAccelHorzntl = 0.f;
    float m_fMouseAccelVertical = 0.f;
    float m_f3rdPersonCHairMultX = 0.f;
    float m_f3rdPersonCHairMultY = 0.f;

    CCam          Cams[3];
    CGarage* pToGarageWeAreIn = nullptr;
    CGarage* pToGarageWeAreInForHackAvoidFirstPerson = nullptr;
    CQueuedMode   m_PlayerMode{};
    CQueuedMode   PlayerWeaponMode{};

    CVector m_PreviousCameraPosition{};
    CVector m_RealPreviousCameraPosition{};
    CVector m_cvecAimingTargetCoors{};
    CVector m_vecFixedModeVector{};
    CVector m_vecFixedModeSource{};
    CVector m_vecFixedModeUpOffSet{};
    CVector m_vecCutSceneOffset{};

    // PS2_CAM_TRANSITION=false
    CVector m_cvecStartingSourceForInterPol{};
    CVector m_cvecStartingTargetForInterPol{};
    CVector m_cvecStartingUpForInterPol{};
    CVector m_cvecSourceSpeedAtStartInter{};
    CVector m_cvecTargetSpeedAtStartInter{};
    CVector m_cvecUpSpeedAtStartInter{};
    CVector m_vecSourceWhenInterPol{};
    CVector m_vecTargetWhenInterPol{};
    CVector m_vecUpWhenInterPol{};
    CVector m_vecGameCamPos{};
    CVector SourceDuringInter{};
    CVector TargetDuringInter{};
    CVector UpDuringInter{};

    void* m_pRwCamera = nullptr; // opaque — was RwCamera*
    CEntity* pTargetEntity = nullptr;

    CCamPathSplines m_arrPathArray[MAX_NUM_OF_SPLINETYPES];
    CTrainCamNode   m_arrTrainCamNode[MAX_NUM_OF_NODES]{};

    CMatrix m_cameraMatrix{};

    bool m_bGarageFixedCamPositionSet = false;
    bool m_vecDoingSpecialInterPolation = false;
    bool m_bScriptParametersSetForInterPol = false;
    bool m_bFading = false;
    bool m_bMusicFading = false;

    CMatrix m_viewMatrix{};
    CVector m_vecFrustumNormals[4]{};
    CVector m_vecOldSourceForInter{};
    CVector m_vecOldFrontForInter{};
    CVector m_vecOldUpForInter{};

    float m_vecOldFOVForInter = 0.f;
    float m_fFLOATingFade = 0.f;
    float m_fFLOATingFadeMusic = 0.f;
    float m_fTimeToFadeOut = 0.f;
    float m_fTimeToFadeMusic = 0.f;
    float m_fFractionInterToStopMoving = 0.f;
    float m_fFractionInterToStopCatchUp = 0.f;
    float m_fGaitSwayBuffer = 0.f;
    float m_fScriptPercentageInterToStopMoving = 0.f;
    float m_fScriptPercentageInterToCatchUp = 0.f;
    std::uint32_t m_fScriptTimeForInterPolation = 0;

    std::int16_t m_iFadingDirection = FADE_NONE;
    int          m_iModeObbeCamIsInForCar = 0;
    std::int16_t m_iModeToGoTo = 0;
    std::int16_t m_iMusicFadingDirection = 0;
    std::int16_t m_iTypeOfSwitch = 0;

    std::uint32_t m_uiFadeTimeStarted = 0;
    std::uint32_t m_uiFadeTimeStartedMusic = 0;

    static bool m_bUseMouse3rdPerson;

    // --- interface (implementations call your renderer/world layer) ---
    CCamera();
    void Init();
    void Process();
    void CamControl();

    bool Get_Just_Switched_Status() const { return m_bJust_Switched; }
    float GetPositionAlongSpline()  const { return m_fPositionAlongSpline; }
    const CMatrix& GetCameraMatrix() { return m_cameraMatrix; }
    CVector& GetGameCamPosition() { return m_vecGameCamPos; }

    void CamShake(float strength, float x, float y, float z);
    void SetCamPositionForFixedMode(const CVector& source, const CVector& upOffset);

    // Widescreen
    void SetWideScreenOn();
    void SetWideScreenOff();
    void ProcessWideScreenOn();
    void DrawBordersForWideScreen(float screenW, float screenH, float scaleY8,
                                  std::function<void(float, float, float, float,
                                  std::uint8_t, std::uint8_t,
                                  std::uint8_t, std::uint8_t)> drawRect);

    // Motion blur
    void SetMotionBlur(int r, int g, int b, int a, int type);
    void SetMotionBlurAlpha(int a);
    void RenderMotionBlur();

    // Fading
    void Fade(float timeout, std::int16_t direction);
    void SetFadeColour(std::uint8_t r, std::uint8_t g, std::uint8_t b);
    bool GetFading()         const { return m_bFading; }
    int  GetFadingDirection()const { return m_iFadingDirection; }
    int  GetScreenFadeStatus();
    void ProcessFade();
    void ProcessMusicFade();

    // Derived values / frustum
    void CalculateDerivedValues(float scaledFOV, float aspectRatio);

    bool IsPointVisible (const CVector& center, const CMatrix* mat);
    bool IsSphereVisible(const CVector& center, float radius, const CMatrix* mat);
    bool IsSphereVisible(const CVector& center, float radius);
    bool IsBoxVisible   (const CVector* box, const CMatrix* mat); // was CVUVECTOR*

    // Script helpers
    void     SetNearClipScript(float v) { m_fNearClipScript = v; }
    void     SetZoomValueFollowPedScript(std::int16_t dist);
    void     SetZoomValueCamStringScript(std::int16_t dist);
    void     SetParametersForScriptInterpolation(float stopMoving, float catchUp, std::int32_t time);
    std::uint32_t GetCutSceneFinishTime();
    void     SetCamCutSceneOffSet(const CVector& pos) { m_vecCutSceneOffset = pos; }
    void     SetPercentAlongCutScene(float pct);
    void     FinishCutscene();

    static void UpdatePlayerVehicleSpeedBlur(CCamera* camera);
};

export GameRef<CCamera> TheCamera([]() -> CCamera*
{
    auto pattern = hook::pattern("B9 ? ? ? ? 53 E8 ? ? ? ? 53");
    if (!pattern.empty())
        return *pattern.get_first<CCamera*>(1);
    return nullptr;
});

void CCamera::UpdatePlayerVehicleSpeedBlur(CCamera* camera)
{
    if (camera->pTargetEntity->GetType() == ENTITY_TYPE_PED)
        return;

    if (camera->Cams[camera->ActiveCam].Mode == CCam::MODE_1STPERSON)
        return;

    if (!FindPlayerVehicle())
        return;

    float speed = FindPlayerVehicle()->GetMoveSpeed().Magnitude();

    if (speed <= 0.65f ||
        FindPlayerVehicle()->IsBoat() ||
        FindPlayerVehicle()->IsHeli() ||
        FindPlayerVehicle()->IsPlane() ||
        FindPlayerVehicle()->IsBike())
        return;


    float speedMul = ((std::min(1.0f, speed) - 0.65f) / (1.0f - 0.65f)) * 75;
    speedMul = std::clamp(speedMul, 0.0f, 1.0f);

    speed = ((std::min(1.0f, speed) - 0.65f) / (1.0f - 0.65f)) / 200;

    int32_t r = CGeneral::GetRandomNumber();
    camera->GetMatrix().GetPosition().x += speed * ((r & 0xF) - 7);
    camera->GetMatrix().GetPosition().y += speed * (((r & 0xF0) >> 4) - 7);
    camera->GetMatrix().GetPosition().z += speed * (((r & 0xF00) >> 8) - 7);
}