module;

#include <stdafx.h>
#include "rw.h"

export module Camera;

import Placeable;
import Skeleton;
import Entity;
import Physical;

#define NUMBER_OF_VECTORS_FOR_AVERAGE   2
#define CAM_NUM_TARGET_HISTORY          4

class CAutomobile;
class CPed;
class CGarage;

enum eEntityType : unsigned char
{
    ENTITY_TYPE_NOTHING,
    ENTITY_TYPE_BUILDING,
    ENTITY_TYPE_VEHICLE,
    ENTITY_TYPE_PED,
    ENTITY_TYPE_OBJECT,
    ENTITY_TYPE_DUMMY,
    ENTITY_TYPE_NOTINPOOLS
};

enum eCamMode : uint16_t
{
    MODE_NONE = 0,                              // 0x00
    MODE_TOPDOWN = 1,                           // 0x01
    MODE_GTACLASSIC = 2,                        // 0x02
    MODE_BEHINDCAR = 3,                         // 0x03
    MODE_FOLLOWPED = 4,                         // 0x04
    MODE_AIMING = 5,                            // 0x05
    MODE_DEBUG = 6,                             // 0x06
    MODE_SNIPER = 7,                            // 0x07
    MODE_ROCKETLAUNCHER = 8,                    // 0x08
    MODE_MODELVIEW = 9,                         // 0x09
    MODE_BILL = 10,                             // 0x0A
    MODE_SYPHON = 11,                           // 0x0B
    MODE_CIRCLE = 12,                           // 0x0C
    MODE_CHEESYZOOM = 13,                       // 0x0D
    MODE_WHEELCAM = 14,                         // 0x0E
    MODE_FIXED = 15,                            // 0x0F
    MODE_1STPERSON = 16,                        // 0x10
    MODE_FLYBY = 17,                            // 0x11
    MODE_CAM_ON_A_STRING = 18,                  // 0x12
    MODE_REACTION = 19,                         // 0x13
    MODE_FOLLOW_PED_WITH_BIND = 20,             // 0x14
    MODE_CHRIS = 21,                            // 0x15
    MODE_BEHINDBOAT = 22,                       // 0x16
    MODE_PLAYER_FALLEN_WATER = 23,              // 0x17
    MODE_CAM_ON_TRAIN_ROOF = 24,                // 0x18
    MODE_CAM_RUNNING_SIDE_TRAIN = 25,           // 0x19
    MODE_BLOOD_ON_THE_TRACKS = 26,              // 0x1A
    MODE_IM_THE_PASSENGER_WOOWOO = 27,          // 0x1B
    MODE_SYPHON_CRIM_IN_FRONT = 28,             // 0x1C
    MODE_PED_DEAD_BABY = 29,                    // 0x1D
    MODE_PILLOWS_PAPS = 30,                     // 0x1E
    MODE_LOOK_AT_CARS = 31,                     // 0x1F
    MODE_ARRESTCAM_ONE = 32,                    // 0x20
    MODE_ARRESTCAM_TWO = 33,                    // 0x21
    MODE_M16_1STPERSON = 34,                    // 0x22
    MODE_SPECIAL_FIXED_FOR_SYPHON = 35,         // 0x23
    MODE_FIGHT_CAM = 36,                        // 0x24
    MODE_TOP_DOWN_PED = 37,                     // 0x25
    MODE_LIGHTHOUSE = 38,                       // 0x26
    MODE_SNIPER_RUNABOUT = 39,                  // 0x27
    MODE_ROCKETLAUNCHER_RUNABOUT = 40,          // 0x28
    MODE_1STPERSON_RUNABOUT = 41,               // 0x29
    MODE_M16_1STPERSON_RUNABOUT = 42,           // 0x2A
    MODE_FIGHT_CAM_RUNABOUT = 43,               // 0x2B
    MODE_EDITOR = 44,                           // 0x2C
    MODE_HELICANNON_1STPERSON = 45,             // 0x2D
    MODE_CAMERA = 46,                           // 0x2E
    MODE_ATTACHCAM = 47,                        // 0x2F
    MODE_TWOPLAYER = 48,                        // 0x30
    MODE_TWOPLAYER_IN_CAR_AND_SHOOTING = 49,    // 0x31
    MODE_TWOPLAYER_SEPARATE_CARS = 50,          // 0x32
    MODE_ROCKETLAUNCHER_HS = 51,                // 0x33
    MODE_ROCKETLAUNCHER_RUNABOUT_HS = 52,       // 0x34
    MODE_AIMWEAPON = 53,                        // 0x35
    MODE_TWOPLAYER_SEPARATE_CARS_TOPDOWN = 54,  // 0x36
    MODE_AIMWEAPON_FROMCAR = 55,                // 0x37
    MODE_DW_HELI_CHASE = 56,                    // 0x38
    MODE_DW_CAM_MAN = 57,                       // 0x39
    MODE_DW_BIRDY = 58,                         // 0x3A
    MODE_DW_PLANE_SPOTTER = 59,                 // 0x3B
    MODE_DW_DOG_FIGHT = 60,                     // 0x3C
    MODE_DW_FISH = 61,                          // 0x3D
    MODE_DW_PLANECAM1 = 62,                     // 0x3E
    MODE_DW_PLANECAM2 = 63,                     // 0x3F
    MODE_DW_PLANECAM3 = 64,                     // 0x40
    MODE_AIMWEAPON_ATTACHED = 65                // 0x41
};

export enum class eMotionBlurType : uint32_t
{
    NONE = 0,
    SNIPER,
    LIGHT_SCENE,
    SECURITY_CAM,
    CUT_SCENE,
    INTRO,
    INTRO2,
    SNIPER_ZOOM,
    INTRO3,
    INTRO4,
};

struct CamTweak
{
    int32_t ModelID;
    float Dist;
    float Alt;
    float Angle;
};

class CQueuedMode
{
public:
    uint16_t m_nMode;
    float  m_fDuration;
    uint16_t m_nMinZoom;
    uint16_t m_nMaxZoom;
};

class CCamPathSplines
{
public:
    float* m_pArrPathData;
};

export class CCam
{
public:
    bool				bBelowMinDist;						// 0x0 //used for follow ped mode
    bool				bBehindPlayerDesired;				// 0x1 //used for follow ped mode
    bool				m_bCamLookingAtVector;				// 0x2
    bool				m_bCollisionChecksOn;				// 0x3
    bool				m_bFixingBeta;						// 0x4 //used for camera on a string
    bool				m_bTheHeightFixerVehicleIsATrain;	// 0x5
    bool				LookBehindCamWasInFront;			// 0x6
    bool				LookingBehind;						// 0x7
    bool				LookingLeft;						// 0x8
    bool				LookingRight;						// 0x9
    bool				ResetStatics;						// 0xA //for interpolation type stuff to work
    bool				Rotating;							// 0xB

    short				Mode;								// 0xC // CameraMode
    unsigned int		m_uiFinishTime;						// 0x10

    int					m_iDoCollisionChecksOnFrameNum;		// 0x14
    int					m_iDoCollisionCheckEveryNumOfFrames;	// 0x18
    int					m_iFrameNumWereAt;					// 0x1C
    int					m_iRunningVectorArrayPos;			// 0x20
    int					m_iRunningVectorCounter;			// 0x24
    int					DirectionWasLooking;				// 0x28

    float   			f_max_role_angle;					// 0x2C //=DEGTORAD(5.0f);
    float   			f_Roll;								// 0x30 //used for adding a slight roll to the camera in the
    float   			f_rollSpeed;						// 0x34 //camera on a string mode
    float   			m_fSyphonModeTargetZOffSet;			// 0x38
    float   			m_fAmountFractionObscured;			// 0x3C
    float   			m_fAlphaSpeedOverOneFrame;			// 0x40
    float   			m_fBetaSpeedOverOneFrame;			// 0x44
    float   			m_fBufferedTargetBeta;				// 0x48
    float   			m_fBufferedTargetOrientation;		// 0x4C
    float   			m_fBufferedTargetOrientationSpeed;	// 0x50
    float   			m_fCamBufferedHeight;				// 0x54
    float   			m_fCamBufferedHeightSpeed;			// 0x58
    float   			m_fCloseInPedHeightOffset;			// 0x5C
    float   			m_fCloseInPedHeightOffsetSpeed;		// 0x60
    float   			m_fCloseInCarHeightOffset;			// 0x64
    float   			m_fCloseInCarHeightOffsetSpeed;		// 0x68
    float   			m_fDimensionOfHighestNearCar;		// 0x6C
    float   			m_fDistanceBeforeChanges;			// 0x70
    float   			m_fFovSpeedOverOneFrame;				// 0x74
    float   			m_fMinDistAwayFromCamWhenInterPolating;	// 0x78
    float   			m_fPedBetweenCameraHeightOffset;		// 0x7C
    float   			m_fPlayerInFrontSyphonAngleOffSet;		// 0x80
    float   			m_fRadiusForDead;					// 0x84
    float   			m_fRealGroundDist;					// 0x88 //used for follow ped mode
    float   			m_fTargetBeta;						// 0x8C
    float   			m_fTimeElapsedFloat;				// 0x90
    float   			m_fTilt;							// 0x94
    float   			m_fTiltSpeed;						// 0x98

    float   			m_fTransitionBeta;					// 0x9C
    float   			m_fTrueBeta;						// 0xA0
    float   			m_fTrueAlpha;						// 0xA4
    float   			m_fInitialPlayerOrientation;		// 0xA8 //used for first person

    float   			Alpha;								// 0xAC
    float   			AlphaSpeed;							// 0xB0
    float   			FOV;								// 0xB4
    float   			FOVSpeed;							// 0xB8
    float   			Beta;								// 0xBC
    float   			BetaSpeed;							// 0xC0
    float   			Distance;							// 0xC4
    float   			DistanceSpeed;						// 0xC8
    float   			CA_MIN_DISTANCE;					// 0xCC
    float   			CA_MAX_DISTANCE;					// 0xD0
    float   			SpeedVar;							// 0xD4
    float				m_fCameraHeightMultiplier;			// 0xD8 //used by TwoPlayer_Separate_Cars_TopDown

    // ped onfoot zoom distance
    float				m_fTargetZoomGroundOne;				// 0xDC
    float				m_fTargetZoomGroundTwo;				// 0xE0
    float				m_fTargetZoomGroundThree;			// 0xE4
    // ped onfoot alpha angle offset
    float				m_fTargetZoomOneZExtra;				// 0xE8
    float				m_fTargetZoomTwoZExtra;				// 0xEC
    float				m_fTargetZoomTwoInteriorZExtra; 	// 0xF0 //extra one for interior
    float				m_fTargetZoomThreeZExtra;			// 0xF4

    float				m_fTargetZoomZCloseIn;				// 0xF8
    float				m_fMinRealGroundDist;				// 0xFC
    float				m_fTargetCloseInDist;				// 0x100

    // For targetting in cooperative mode.
    float				Beta_Targeting;						// 0x104
    float				X_Targetting, Y_Targetting;			// 0x108
    int					CarWeAreFocussingOn;				// 0x110 //which car is closer to the camera in coop mode with separate cars
    float				CarWeAreFocussingOnI;				// 0x114 //interpolated version

    float				m_fCamBumpedHorz;					// 0x118
    float				m_fCamBumpedVert;					// 0x11C
    int					m_nCamBumpedTime;					// 0x120
    static int			CAM_BUMPED_SWING_PERIOD;
    static int			CAM_BUMPED_END_TIME;
    static float		CAM_BUMPED_DAMP_RATE;
    static float		CAM_BUMPED_MOVE_MULT;

    CVector 			m_cvecSourceSpeedOverOneFrame;		// 0x124
    CVector 			m_cvecTargetSpeedOverOneFrame;		// 0x130
    CVector 			m_cvecUpOverOneFrame;				// 0x13C

    CVector 			m_cvecTargetCoorsForFudgeInter; 	// 0x148
    CVector 			m_cvecCamFixedModeVector;			// 0x154
    CVector 			m_cvecCamFixedModeSource;			// 0x160
    CVector				m_cvecCamFixedModeUpOffSet;			// 0x16C
    CVector				m_vecLastAboveWaterCamPosition;		// 0x178 //helper for when the player has gone under the water

    CVector				m_vecBufferedPlayerBodyOffset;		// 0x184

    // The three vectors that determine this camera for this frame
    CVector				Front;													// 0x190	//Direction of looking in
    CVector				Source;													// 0x19C	//Coors in world space
    CVector				SourceBeforeLookBehind;									// 0x1A8
    CVector				Up;                                                     // 0x1B4	//Just that
    CVector				m_arrPreviousVectors[NUMBER_OF_VECTORS_FOR_AVERAGE];	// 0x1C0	//used to average stuff

    CVector				m_aTargetHistoryPos[CAM_NUM_TARGET_HISTORY];			// 0x1D8
    DWORD				m_nTargetHistoryTime[CAM_NUM_TARGET_HISTORY];			// 0x208
    DWORD				m_nCurrentHistoryPoints;								// 0x218

    CEntity* CamTargetEntity;										// 0x21C
    float				m_fCameraDistance;										// 0x220
    float				m_fIdealAlpha;											// 0x224
    float				m_fPlayerVelocity;										// 0x228
    //CVector TempRight;
    CAutomobile* m_pLastCarEntered;										// 0x22C // So interpolation works
    CPed* m_pLastPedLookedAt;										// 0x230 // So interpolation works
    bool				m_bFirstPersonRunAboutActive;							// 0x234
};

enum class eFadeFlag : uint16_t
{
    FADE_IN,
    FADE_OUT
};

enum class eSwitchType : uint16_t
{
    NONE,
    INTERPOLATION,
    JUMPCUT
};

export class CCamera : public CPlaceable
{
public:
    bool            m_bAboveGroundTrainNodesLoaded;
    bool            m_bBelowGroundTrainNodesLoaded;
    bool            m_bCamDirectlyBehind;
    bool            m_bCamDirectlyInFront;
    bool            m_bCameraJustRestored;
    bool            m_bCutsceneFinished;
    bool            m_bCullZoneChecksOn;
    bool            m_bFirstPersonBeingUsed;
    bool            m_bJustJumpedOutOf1stPersonBecauseOfTarget;
    bool            m_bIdleOn;
    bool            m_bInATunnelAndABigVehicle;
    bool            m_bInitialNodeFound;
    bool            m_bInitialNoNodeStaticsSet;
    bool            m_bIgnoreFadingStuffForMusic;
    bool            m_bPlayerIsInGarage;
    bool            m_bPlayerWasOnBike;
    bool            m_bJustCameOutOfGarage;
    bool            m_bJustInitialized;
    bool            m_bJust_Switched;
    bool            m_bLookingAtPlayer;
    bool            m_bLookingAtVector;
    bool            m_bMoveCamToAvoidGeom;
    bool            m_bObbeCinematicPedCamOn;
    bool            m_bObbeCinematicCarCamOn;
    bool            m_bRestoreByJumpCut;
    bool            m_bUseNearClipScript;
    bool            m_bStartInterScript;
    bool            m_bStartingSpline;
    bool            m_bTargetJustBeenOnTrain;
    bool            m_bTargetJustCameOffTrain;
    bool            m_bUseSpecialFovTrain;
    bool            m_bUseTransitionBeta;
    bool            m_bUseScriptZoomValuePed;
    bool            m_bUseScriptZoomValueCar;
    bool            m_bWaitForInterpolToFinish;
    bool            m_bItsOkToLookJustAtThePlayer;
    bool            m_bWantsToSwitchWidescreenOff;
    bool            m_bWideScreenOn;
    bool            m_b1rstPersonRunCloseToAWall;
    bool            m_bHeadBob;
    bool            m_bVehicleSuspenHigh;
    bool            m_bEnable1rstPersonCamCntrlsScript;
    bool            m_bAllow1rstPersonWeaponsCamera;
    bool            m_bCooperativeCamMode;
    bool            m_bAllowShootingWith2PlayersInCar;
    bool            m_bDisableFirstPersonInCar;
    eCamMode        m_nModeForTwoPlayersSeparateCars;
    eCamMode        m_nModeForTwoPlayersSameCarShootingAllowed;
    eCamMode        m_nModeForTwoPlayersSameCarShootingNotAllowed;
    eCamMode        m_nModeForTwoPlayersNotBothInCar;
    bool            m_bGarageFixedCamPositionSet;
    bool            m_bDoingSpecialInterp;
    bool            m_bScriptParametersSetForInterp;
    bool            m_bFading;
    bool            m_bMusicFading;
    bool            m_bMusicFadedOut;
    bool            m_bFailedCullZoneTestPreviously;
    bool            m_bFadeTargetIsSplashScreen;
    bool            m_bWorldViewerBeingUsed;
    bool            m_bTransitionJUSTStarted;
    bool            m_bTransitionState;
    uint8_t           m_nActiveCam;
    uint32_t         m_nCamShakeStart;
    uint32_t         m_nFirstPersonCamLastInputTime;
    uint32_t         m_nLongestTimeInMill;
    uint32_t         m_nNumberOfTrainCamNodes;
    uint32_t         m_nTimeLastChange;
    uint32_t         m_nTimeWeLeftIdle_StillNoInput;
    uint32_t         m_nTimeWeEnteredIdle;
    uint32_t         m_nTimeTransitionStart;
    uint32_t         m_nTransitionDuration;
    uint32_t         m_nTransitionDurationTargetCoors;
    uint32_t         m_nBlurBlue;
    uint32_t         m_nBlurGreen;
    uint32_t         m_nBlurRed;
    eMotionBlurType m_nBlurType;
    uint32_t          m_nWorkOutSpeedThisNumFrames;
    uint32_t          m_nNumFramesSoFar;
    uint32_t          m_nCurrentTrainCamNode;
    uint32_t          m_nMotionBlur;
    uint32_t          m_nMotionBlurAddAlpha;
    uint32_t          m_nCheckCullZoneThisNumFrames;
    uint32_t          m_nZoneCullFrameNumWereAt;
    uint32_t          m_nWhoIsInControlOfTheCamera;
    uint32_t          m_nCarZoom;
    float           m_fCarZoomBase;
    float           m_fCarZoomTotal;
    float           m_fCarZoomSmoothed;
    float           m_fCarZoomValueScript;
    uint32_t          m_nPedZoom;
    float           m_fPedZoomBase;
    float           m_fPedZoomTotal;
    float           m_fPedZoomSmoothed;
    float           m_fPedZoomValueScript;
    float           m_fCamFrontXNorm;
    float           m_fCamFrontYNorm;
    float           m_fDistanceToWater;
    float           m_fHeightOfNearestWater;
    float           m_fFOVDuringInter;
    float           m_fLODDistMultiplier;
    float           m_fGenerationDistMultiplier;
    float           m_fAlphaSpeedAtStartInter;
    float           m_fAlphaWhenInterPol;
    float           m_fAlphaDuringInterPol;
    float           m_fBetaDuringInterPol;
    float           m_fBetaSpeedAtStartInter;
    float           m_fBetaWhenInterPol;
    float           m_fFOVWhenInterPol;
    float           m_fFOVSpeedAtStartInter;
    float           m_fStartingBetaForInterPol;
    float           m_fStartingAlphaForInterPol;
    float           m_fPedOrientForBehindOrInFront;
    float           m_fCameraAverageSpeed;
    float           m_fCameraSpeedSoFar;
    float           m_fCamShakeForce;
    float           m_fFovForTrain;
    float           m_fFOV_Wide_Screen;
    float           m_fNearClipScript;
    float           m_fOldBetaDiff;
    float           m_fPositionAlongSpline;
    float           m_fScreenReductionPercentage;
    float           m_fScreenReductionSpeed;
    float           m_fAlphaForPlayerAnim1rstPerson;
    float           m_fOrientation;
    float           m_fPlayerExhaustion;
    float           m_fSoundDistUp;
    float           m_fSoundDistUpAsRead;
    float           m_fSoundDistUpAsReadOld;
    float           m_fAvoidTheGeometryProbsTimer;
    uint16_t          m_nAvoidTheGeometryProbsDirn;
    float           m_fWideScreenReductionAmount;
    float           m_fStartingFOVForInterPol;
    std::array<CCam, 3> m_aCams;
    CGarage* m_pToGarageWeAreIn;
    CGarage* m_pToGarageWeAreInForHackAvoidFirstPerson;
    CQueuedMode     m_PlayerMode;
    CQueuedMode     m_PlayerWeaponMode;
    CVector         m_vecPreviousCameraPosition;
    CVector         m_vecRealPreviousCameraPosition;
    CVector         m_vecAimingTargetCoors;
    CVector         m_vecFixedModeVector;
    CVector         m_vecFixedModeSource;
    CVector         m_vecFixedModeUpOffSet;
    CVector         m_vecCutSceneOffset;
    CVector         m_vecStartingSourceForInterPol;
    CVector         m_vecStartingTargetForInterPol;
    CVector         m_vecStartingUpForInterPol;
    CVector         m_vecSourceSpeedAtStartInter;
    CVector         m_vecTargetSpeedAtStartInter;
    CVector         m_vecUpSpeedAtStartInter;
    CVector         m_vecSourceWhenInterPol;
    CVector         m_vecTargetWhenInterPol;
    CVector         m_vecUpWhenInterPol;
    CVector         m_vecClearGeometryVec;
    CVector         m_vecGameCamPos;
    CVector         m_vecSourceDuringInter;
    CVector         m_vecTargetDuringInter;
    CVector         m_vecUpDuringInter;
    CVector         m_vecAttachedCamOffset;
    CVector         m_vecAttachedCamLookAt;
    float           m_fAttachedCamAngle;
    RwCamera* m_pRwCamera;
    CEntity* m_pTargetEntity;
    CEntity* m_pAttachedEntity;
    std::array<CCamPathSplines, 4> m_aPathArray;
    bool            m_bMirrorActive;
    bool            m_bResetOldMatrix;
    CMatrix         m_mCameraMatrix;
    CMatrix         m_mCameraMatrixOld;
    CMatrix         m_mViewMatrix;
    CMatrix         m_mMatInverse;
    CMatrix         m_mMatMirrorInverse;
    CMatrix         m_mMatMirror;
    std::array<CVector, 4> m_avecFrustumNormals;
    std::array<CVector, 4> m_avecFrustumWorldNormals;
    std::array<CVector, 4> m_avecFrustumWorldNormals_Mirror;
    std::array<float, 4>   m_fFrustumPlaneOffsets;
    std::array<float, 4>   m_fFrustumPlaneOffsets_Mirror;
    CVector         m_vecRightFrustumNormal;
    CVector         m_vecBottomFrustumNormal;
    CVector         m_vecTopFrustumNormal;
    float           field_BF8;
    float           m_fFadeAlpha;
    float           m_fEffectsFaderScalingFactor;
    float           m_fFadeDuration;
    float           m_fTimeToFadeMusic;
    float           m_fTimeToWaitToFadeMusic;
    float           m_fFractionInterToStopMoving;
    float           m_fFractionInterToStopCatchUp;
    float           m_fFractionInterToStopMovingTarget;
    float           m_fFractionInterToStopCatchUpTarget;
    float           m_fGaitSwayBuffer;
    float           m_fScriptPercentageInterToStopMoving;
    float           m_fScriptPercentageInterToCatchUp;
    uint32_t          m_nScriptTimeForInterpolation;
    eFadeFlag       m_nFadeInOutFlag;
    int32_t           m_nModeObbeCamIsInForCar;
    eCamMode        m_nModeToGoTo;
    eFadeFlag       m_nMusicFadingDirection;
    eSwitchType     m_nTypeOfSwitch;
    char            _alignC40[2];
    uint32_t          m_nFadeStartTime;
    uint32_t          m_nFadeTimeStartedMusic;
    int32_t           m_nExtraEntitiesCount;
    CEntity* m_pExtraEntity[2];
    float           m_fDuckCamMotionFactor;
    float           m_fDuckAimCamMotionFactor;
    float           m_fTrackLinearStartTime;
    float           m_fTrackLinearEndTime;
    CVector         m_vecTrackLinearEndPoint;
    CVector         m_vecTrackLinearStartPoint;
    bool            m_bTrackLinearWithEase;
    CVector         m_vecTrackLinear;
    bool            m_bVecTrackLinearProcessed;
    float           m_fShakeIntensity;
    float           m_fStartShakeTime;
    float           m_fEndShakeTime;
    int32_t           field_C9C;
    int32_t           m_nShakeType;
    float           m_fStartZoomTime;
    float           m_fEndZoomTime;
    float           m_fZoomInFactor;
    float           m_fZoomOutFactor;
    uint8_t           m_nZoomMode;
    bool            m_bFOVLerpProcessed;
    float           m_fFOVNew;
    float           m_fMoveLinearStartTime;
    float           m_fMoveLinearEndTime;
    CVector         m_vecMoveLinearPosnStart;
    CVector         m_vecMoveLinearPosnEnd;
    bool            m_bMoveLinearWithEase;
    CVector         m_vecMoveLinear;
    bool            m_bVecMoveLinearProcessed;
    bool            m_bBlockZoom;
    bool            m_bCameraPersistPosition;
    bool            m_bCameraPersistTrack;
    bool            m_bCinemaCamera;
    CamTweak        m_aCamTweak[5];
    bool            m_bCameraVehicleTweaksInitialized;
    float           m_fCurrentTweakDistance;
    float           m_fCurrentTweakAltitude;
    float           m_fCurrentTweakAngle;
    int32_t           m_nCurrentTweakModelIndex;
    int32_t           field_D58;
    int32_t           field_D5C;
    int32_t           field_D60;
    int32_t           field_D64;
    int32_t           field_D68;
    int32_t           field_D6C;
    int32_t           field_D70;
    int32_t           field_D74;

    inline bool					GetFading() { return m_bFading; };

    void						GetScreenRect(CRect& rect);
    int							GetFadeStage();
    int							GetLookDirection();
    bool						IsPositionVisible(const CVector& vecPos, float fRadius);
    static void UpdatePlayerVehicleSpeedBlur(CCamera* camera);
};

export GameRef<CCamera> TheCamera([]() -> CCamera*
{
    auto pattern = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? ? ? ? BE ? ? ? ? ? ? ? ? ? ? DF E0");
    if (!pattern.empty())
        return *pattern.get_first<CCamera*>(1);
    return nullptr;
});

enum eVehicleType
{
    VEHICLE_INVALID = -1,
    VEHICLE_AUTOMOBILE,
    VEHICLE_MTRUCK,
    VEHICLE_QUAD,
    VEHICLE_HELI,
    VEHICLE_PLANE,
    VEHICLE_BOAT,
    VEHICLE_TRAIN,
    VEHICLE_FHELI,
    VEHICLE_FPLANE,
    VEHICLE_BIKE,
    VEHICLE_BMX,
    VEHICLE_TRAILER
};

bool IsHeli(int m_nVehicleType) { return m_nVehicleType == VEHICLE_HELI; }
bool IsPlane(int m_nVehicleType) { return m_nVehicleType == VEHICLE_PLANE; }
bool IsBoat(int m_nVehicleType) { return m_nVehicleType == VEHICLE_BOAT; }
bool IsTrain(int m_nVehicleType) { return m_nVehicleType == VEHICLE_TRAIN; }
bool IsFakeAircraft(int m_nVehicleType) { return m_nVehicleType == VEHICLE_FHELI || m_nVehicleType == VEHICLE_FPLANE; }
bool IsBike(int m_nVehicleType) { return m_nVehicleType == VEHICLE_BIKE; }
bool IsBMX(int m_nVehicleType) { return m_nVehicleType == VEHICLE_BMX; }
bool IsTrailer(int m_nVehicleType) { return m_nVehicleType == VEHICLE_TRAILER; }

void CCamera::UpdatePlayerVehicleSpeedBlur(CCamera* camera)
{
    if (camera->m_pTargetEntity->nType == ENTITY_TYPE_PED)
        return;

    if (camera->m_aCams[camera->m_nActiveCam].Mode != eCamMode::MODE_CAM_ON_A_STRING)
        return;

    CPhysical* playerVehicle = (CPhysical*)FindPlayerVehicle(0, true);

    if (!playerVehicle)
        return;

    float speed = playerVehicle->m_vecMoveSpeed.Magnitude();
    int m_nVehicleType = *(int*)((uintptr_t)playerVehicle + 0x590);

    if (speed <= 0.65f ||
        IsBoat(m_nVehicleType) ||
        IsHeli(m_nVehicleType) ||
        IsPlane(m_nVehicleType) ||
        IsTrain(m_nVehicleType) ||
        IsFakeAircraft(m_nVehicleType) ||
        IsBMX(m_nVehicleType) ||
        IsTrailer(m_nVehicleType) ||
        IsBike(m_nVehicleType))
        return;


    float speedMul = ((std::min(1.0f, speed) - 0.65f) / (1.0f - 0.65f)) * 75;
    speedMul = std::clamp(speedMul, 0.0f, 1.0f);

    speed = ((std::min(1.0f, speed) - 0.65f) / (1.0f - 0.65f)) / 200;

    int32_t r = CGeneral::GetRandomNumber();
    camera->GetMatrix()->GetPosition().x += speed * ((r & 0xF) - 7);
    camera->GetMatrix()->GetPosition().y += speed * (((r & 0xF0) >> 4) - 7);
    camera->GetMatrix()->GetPosition().z += speed * (((r & 0xF00) >> 8) - 7);
}