module;
#include <stdafx.h>
#include <cstdint>
#include "common.h"

export module Vehicle;

import Placeable;
import Entity;
import Physical;

using CPed = void;

enum tVehicleType
{
    HANDLING_LANDSTAL,
    HANDLING_IDAHO,
    HANDLING_STINGER,
    HANDLING_LINERUN,
    HANDLING_PEREN,
    HANDLING_SENTINEL,
    HANDLING_PATRIOT,
    HANDLING_FIRETRUK,
    HANDLING_TRASH,
    HANDLING_STRETCH,
    HANDLING_MANANA,
    HANDLING_INFERNUS,
    HANDLING_PONY,
    HANDLING_MULE,
    HANDLING_CHEETAH,
    HANDLING_AMBULAN,
    HANDLING_FBICAR,
    HANDLING_MOONBEAM,
    HANDLING_ESPERANT,
    HANDLING_TAXI,
    HANDLING_KURUMA,
    HANDLING_BOBCAT,
    HANDLING_MRWHOOP,
    HANDLING_BFINJECT,
    HANDLING_POLICE,
    HANDLING_ENFORCER,
    HANDLING_SECURICA,
    HANDLING_BANSHEE,
    HANDLING_BUS,
    HANDLING_RHINO,
    HANDLING_BARRACKS,
    HANDLING_TRAIN,
    HANDLING_HELI,
    HANDLING_DODO,
    HANDLING_COACH,
    HANDLING_CABBIE,
    HANDLING_STALLION,
    HANDLING_RUMPO,
    HANDLING_RCBANDIT,
    HANDLING_MAFIA,
    HANDLING_AIRTRAIN,
    HANDLING_DEADDODO,
    HANDLING_FLATBED,
    HANDLING_YANKEE,
    HANDLING_GOLFCART,
    HANDLING_VOODOO,
    HANDLING_WASHING,
    HANDLING_CUBAN,
    HANDLING_ROMERO,
    HANDLING_PACKER,
    HANDLING_ADMIRAL,
    HANDLING_GANGBUR,
    HANDLING_ZEBRA,
    HANDLING_TOPFUN,
    HANDLING_GLENDALE,
    HANDLING_OCEANIC,
    HANDLING_HERMES,
    HANDLING_SABRE1,
    HANDLING_SABRETUR,
    HANDLING_PHEONIX,
    HANDLING_WALTON,
    HANDLING_REGINA,
    HANDLING_COMET,
    HANDLING_DELUXO,
    HANDLING_BURRITO,
    HANDLING_SPAND,
    HANDLING_BAGGAGE,
    HANDLING_KAUFMAN,
    HANDLING_RANCHER,
    HANDLING_FBIRANCH,
    HANDLING_VIRGO,
    HANDLING_GREENWOO,
    HANDLING_HOTRING,
    HANDLING_SANDKING,
    HANDLING_BLISTAC,
    HANDLING_BOXVILLE,
    HANDLING_BENSON,
    HANDLING_DESPERAD,
    HANDLING_LOVEFIST,
    HANDLING_BLOODRA,
    HANDLING_BLOODRB,

    HANDLING_BIKE,
    HANDLING_MOPED,
    HANDLING_DIRTBIKE,
    HANDLING_ANGEL,
    HANDLING_FREEWAY,

    HANDLING_PREDATOR,
    HANDLING_SPEEDER,
    HANDLING_REEFER,
    HANDLING_RIO,
    HANDLING_SQUALO,
    HANDLING_TROPIC,
    HANDLING_COASTGRD,
    HANDLING_DINGHY,
    HANDLING_MARQUIS,
    HANDLING_CUPBOAT,
    HANDLING_SEAPLANE, // both boat and plane!
    HANDLING_SPARROW,
    HANDLING_SEASPAR,
    HANDLING_MAVERICK,
    HANDLING_COASTMAV,
    HANDLING_POLMAV,
    HANDLING_HUNTER,
    HANDLING_RCBARON,
    HANDLING_RCGOBLIN,
    HANDLING_RCCOPTER,

    NUMHANDLINGS,

    NUMBIKEHANDLINGS = HANDLING_FREEWAY + 1 - HANDLING_BIKE,
    NUMFLYINGHANDLINGS = HANDLING_RCCOPTER + 1 - HANDLING_SEAPLANE,
    NUMBOATHANDLINGS = HANDLING_SEAPLANE + 1 - HANDLING_PREDATOR,
};

enum eVehicleType
{
    VEHICLE_TYPE_CAR,
    VEHICLE_TYPE_BOAT,
    VEHICLE_TYPE_TRAIN,
    VEHICLE_TYPE_HELI,
    VEHICLE_TYPE_PLANE,
    VEHICLE_TYPE_BIKE,
    NUM_VEHICLE_TYPES
};

enum eCarLock
{
    CARLOCK_NOT_USED,
    CARLOCK_UNLOCKED,
    CARLOCK_LOCKED,
    CARLOCK_LOCKOUT_PLAYER_ONLY,
    CARLOCK_LOCKED_PLAYER_INSIDE,
    CARLOCK_LOCKED_INITIALLY,
    CARLOCK_FORCE_SHUT_DOORS,
    CARLOCK_LOCKED_BUT_CAN_BE_DAMAGED
};

enum
{
    HANDLING_1G_BOOST = 1,
    HANDLING_2G_BOOST = 2,
    HANDLING_REV_BONNET = 4,
    HANDLING_HANGING_BOOT = 8,
    HANDLING_NO_DOORS = 0x10,
    HANDLING_IS_VAN = 0x20,
    HANDLING_IS_BUS = 0x40,
    HANDLING_IS_LOW = 0x80,
    HANDLING_DBL_EXHAUST = 0x100,
    HANDLING_TAILGATE_BOOT = 0x200,
    HANDLING_NOSWING_BOOT = 0x400,
    HANDLING_NONPLAYER_STABILISER = 0x800,
    HANDLING_NEUTRALHANDLING = 0x1000,
    HANDLING_HAS_NO_ROOF = 0x2000,
    HANDLING_IS_BIG = 0x4000,
    HANDLING_HALOGEN_LIGHTS = 0x8000,
    HANDLING_IS_BIKE = 0x10000,
    HANDLING_IS_HELI = 0x20000,
    HANDLING_IS_PLANE = 0x40000,
    HANDLING_IS_BOAT = 0x80000,
    HANDLING_NO_EXHAUST = 0x100000,
    HANDLING_REARWHEEL_1ST = 0x200000,
    HANDLING_HANDBRAKE_TYRE = 0x400000,
    HANDLING_SIT_IN_BOAT = 0x800000,
    HANDLING_FAT_REARW = 0x1000000,
    HANDLING_NARROW_FRONTW = 0x2000000,
    HANDLING_GOOD_INSAND = 0x4000000,
    HANDLING_UNKNOWN = 0x8000000, // something for helis and planes
};

struct tGear
{
    float fMaxVelocity;
    float fShiftUpVelocity;
    float fShiftDownVelocity;
};

class cTransmission
{
public:
    // Gear 0 is reverse, 1-5 are forward
    tGear Gears[6];
    char nDriveType;
    char nEngineType;
    int8_t nNumberOfGears;
    uint8_t Flags;
    float fEngineAcceleration;
    float fMaxVelocity;
    float fMaxCruiseVelocity;
    float fMaxReverseVelocity;
    float fCurVelocity;
};

struct tHandlingData
{
    tVehicleType nIdentifier;
    float fMass;
    float fInvMass;
    float fTurnMass;
    CVector Dimension;
    CVector CentreOfMass;
    int8_t nPercentSubmerged;
    float fBuoyancy;
    float fTractionMultiplier;
    cTransmission Transmission;
    float fBrakeDeceleration;
    float fBrakeBias;
    int8_t bABS;
    float fSteeringLock;
    float fTractionLoss;
    float fTractionBias;
    float fUnused;
    float fSuspensionForceLevel;
    float fSuspensionDampingLevel;
    float fSuspensionUpperLimit;
    float fSuspensionLowerLimit;
    float fSuspensionBias;
    float fSuspensionAntidiveMultiplier;
    float fCollisionDamageMultiplier;
    uint32_t Flags;
    float fSeatOffsetDistance;
    int32_t nMonetaryValue;
    int8_t FrontLights;
    int8_t RearLights;
};

struct tBikeHandlingData
{
    tVehicleType nIdentifier;
    float fLeanFwdCOM;
    float fLeanFwdForce;
    float fLeanBakCOM;
    float fLeanBackForce;
    float fMaxLean;
    float fFullAnimLean;
    float fDesLean;
    float fSpeedSteer;
    float fSlipSteer;
    float fNoPlayerCOMz;
    float fWheelieAng;
    float fStoppieAng;
    float fWheelieSteer;
    float fWheelieStabMult;
    float fStoppieStabMult;
};

struct tBoatHandlingData
{
    tVehicleType nIdentifier;
    float fThrustY;
    float fThrustZ;
    float fThrustAppZ;
    float fAqPlaneForce;
    float fAqPlaneLimit;
    float fAqPlaneOffset;
    float fWaveAudioMult;
    float fLook_L_R_BehindCamHeight;
    CVector vecMoveRes;
    CVector vecTurnRes;
};

struct tFlyingHandlingData
{
    tVehicleType nIdentifier;
    float fThrust;
    float fThrustFallOff;
    float fYaw;
    float fYawStab;
    float fSideSlip;
    float fRoll;
    float fRollStab;
    float fPitch;
    float fPitchStab;
    float fFormLift;
    float fAttackLift;
    float fMoveRes;
    CVector vecTurnRes;
    CVector vecSpeedRes;
};

struct CStoredCollPoly
{
    CVector verts[3];
    bool valid;
};

constexpr auto NUM_PATH_NODES_IN_AUTOPILOT = 8;

class CVehicle;
class CAutoPilot
{
public:
    int32_t m_nCurrentRouteNode;
    int32_t m_nNextRouteNode;
    int32_t m_nPrevRouteNode;
    int32_t m_nTimeEnteredCurve;
    int32_t m_nTimeToSpendOnCurrentCurve;
    uint32_t m_nCurrentPathNodeInfo;
    uint32_t m_nNextPathNodeInfo;
    uint32_t m_nPreviousPathNodeInfo;
    uint32_t m_nAntiReverseTimer;
    uint32_t m_nTimeToStartMission;
    int8_t m_nPreviousDirection;
    int8_t m_nCurrentDirection;
    int8_t m_nNextDirection;
    int8_t m_nCurrentLane;
    int8_t m_nNextLane;
    uint8_t m_nDrivingStyle;
    uint8_t m_nCarMission;
    uint8_t m_nTempAction;
    uint32_t m_nTimeTempAction;
    float m_fMaxTrafficSpeed;
    uint8_t m_nCruiseSpeed;
    uint8_t m_nCruiseSpeedMultiplierType;
    float m_fCruiseSpeedMultiplier;
    uint8_t m_bSlowedDownBecauseOfCars : 1;
    uint8_t m_bSlowedDownBecauseOfPeds : 1;
    uint8_t m_bStayInCurrentLevel : 1;
    uint8_t m_bStayInFastLane : 1;
    uint8_t m_bIgnorePathfinding : 1;
    uint8_t m_nSwitchDistance;
    CVector m_vecDestinationCoors;
    void* m_aPathFindNodesInfo[NUM_PATH_NODES_IN_AUTOPILOT];
    int16_t m_nPathFindNodesCount;
    CVehicle* m_pTargetCar;

    CAutoPilot(void)
    {
    }

    void ModifySpeed(float);
    void RemoveOnePathNode();

    float GetCruiseSpeed(void) { return m_nCruiseSpeed * m_fCruiseSpeedMultiplier; }
};

class CFire
{
public:
    bool m_bIsOngoing;
    bool m_bIsScriptFire;
    bool m_bPropagationFlag;
    bool m_bAudioSet;
    CVector m_vecPos;
    CEntity* m_pEntity;
    CEntity* m_pSource;
    uint32_t m_nExtinguishTime;
    uint32_t m_nStartTime;
    uint32_t m_nNextTimeToAddFlames;
    float m_fStrength;
    float m_fWaterExtinguishCountdown;
    bool m_bExtinguishedWithWater;

    CFire();
    ~CFire();
    void ProcessFire(void);
    void ReportThisFire(void);
    void Extinguish(void);
};

export class CVehicle : public CPhysical
{
public:
    tHandlingData* pHandling;
    tFlyingHandlingData* pFlyingHandling;
    CAutoPilot AutoPilot;
    uint8_t m_currentColour1;
    uint8_t m_currentColour2;
    int8_t m_aExtras[2];
    int16_t m_nAlarmState;
    int16_t m_nRouteSeed;
    CPed* pDriver;
    CPed* pPassengers[8];
    uint8_t m_nNumPassengers;
    int8_t m_nNumGettingIn;
    int8_t m_nGettingInFlags;
    int8_t m_nGettingOutFlags;
    uint8_t m_nNumMaxPassengers;
    float field_1D0[4];
    CEntity* m_pCurGroundEntity;
    CFire* m_pCarFire;
    float m_fSteerAngle;
    float m_fGasPedal;
    float m_fBrakePedal;
    uint8_t VehicleCreatedBy;
    uint8_t bIsLawEnforcer : 1;     // Is this guy chasing the player at the moment
    uint8_t bIsAmbulanceOnDuty : 1; // Ambulance trying to get to an accident
    uint8_t bIsFireTruckOnDuty : 1; // Firetruck trying to get to a fire
    uint8_t bIsLocked : 1;          // Is this guy locked by the script (cannot be removed)
    uint8_t bEngineOn : 1;          // For sound purposes. Parked cars have their engines switched off (so do destroyed cars)
    uint8_t bIsHandbrakeOn : 1;     // How's the handbrake doing ?
    uint8_t bLightsOn : 1;          // Are the lights switched on ?
    uint8_t bFreebies : 1;          // Any freebies left in this vehicle ?

    uint8_t bIsVan : 1;            // Is this vehicle a van (doors at back of vehicle)
    uint8_t bIsBus : 1;            // Is this vehicle a bus
    uint8_t bIsBig : 1;            // Is this vehicle a bus
    uint8_t bLowVehicle : 1;       // Need this for sporty type cars to use low getting-in/out anims
    uint8_t bComedyControls : 1;   // Will make the car hard to control (hopefully in a funny way)
    uint8_t bWarnedPeds : 1;       // Has scan and warn peds of danger been processed?
    uint8_t bCraneMessageDone : 1; // A crane message has been printed for this car allready
    uint8_t bExtendedRange : 1;    // This vehicle needs to be a bit further away to get deleted

    uint8_t bTakeLessDamage : 1;       // This vehicle is stronger (takes about 1/4 of damage)
    uint8_t bIsDamaged : 1;            // This vehicle has been damaged and is displaying all its components
    uint8_t bHasBeenOwnedByPlayer : 1; // To work out whether stealing it is a crime
    uint8_t bFadeOut : 1;              // Fade vehicle out
    uint8_t bIsBeingCarJacked : 1;     // Fade vehicle out
    uint8_t bCreateRoadBlockPeds : 1;  // If this vehicle gets close enough we will create peds (coppers or gang members) round it
    uint8_t bCanBeDamaged : 1;         // Set to FALSE during cut scenes to avoid explosions
    uint8_t bUsingSpecialColModel : 1; // Is player vehicle using special collision model, stored in player strucure

    uint8_t bOccupantsHaveBeenGenerated : 1; // Is true if the occupants have already been generated. (Shouldn't happen again)
    uint8_t bGunSwitchedOff : 1;             // Level designers can use this to switch off guns on boats
    uint8_t bVehicleColProcessed : 1;        // Has ProcessEntityCollision been processed for this car?
    uint8_t bIsCarParkVehicle : 1;           // Car has been created using the special CAR_PARK script command
    uint8_t bHasAlreadyBeenRecorded : 1;     // Used for replays
    uint8_t bPartOfConvoy : 1;
    uint8_t bHeliMinimumTilt : 1;   // This heli should have almost no tilt really
    uint8_t bAudioChangingGear : 1; // sounds like vehicle is changing gear

    uint8_t bIsDrowning : 1;             // is vehicle occupants taking damage in water (i.e. vehicle is dead in water)
    uint8_t bTyresDontBurst : 1;         // If this is set the tyres are invincible
    uint8_t bCreatedAsPoliceVehicle : 1; // True if this guy was created as a police vehicle (enforcer, policecar, miamivice car etc)
    uint8_t bRestingOnPhysical : 1;      // Dont go static cause car is sitting on a physical object that might get removed
    uint8_t bParking : 1;
    uint8_t bCanPark : 1;
    uint8_t m_bombType : 3;
    uint8_t bDriverLastFrame : 1;

    int8_t m_numPedsUseItAsCover;
    uint8_t m_nAmmoInClip; // Used to make the guns on boat do a reload (20 by default)
    int8_t m_nPacManPickupsCarried;
    uint8_t m_nRoadblockType;
    float m_fHealth; // 1000.0f = full health. 250.0f = fire. 0 -> explode
    uint8_t m_nCurrentGear;
    float m_fChangeGearTime;
    #if (!defined GTA_PS2 || defined FIX_BUGS)
    CEntity* m_pBombRigger;
    #endif
    uint32_t m_nSetPieceExtendedRangeTime;
    uint32_t m_nGunFiringTime; // last time when gun on vehicle was fired (used on boats)
    uint32_t m_nTimeOfDeath;
    uint16_t m_nTimeBlocked;
    int16_t m_nBombTimer; // goes down with each frame
    CEntity* m_pBlowUpEntity;
    float m_fMapObjectHeightAhead;  // front Z?
    float m_fMapObjectHeightBehind; // rear Z?
    eCarLock m_nDoorLock;
    int8_t m_nLastWeaponDamage; // see eWeaponType, -1 if no damage
    CEntity* m_pLastDamageEntity;
    uint8_t m_nRadioStation;
    uint8_t m_bRainAudioCounter;
    uint8_t m_bRainSamplesCounter;
    uint32_t m_nCarHornTimer;
    uint8_t m_nCarHornPattern;
    bool m_bSirenOrAlarm;
    uint8_t m_nCarHornDelay;
    int8_t m_comedyControlState;
    CStoredCollPoly m_aCollPolys[2]; // poly which is under front/rear part of car
    float m_fSteerInput;
    eVehicleType m_vehType;

    bool IsCar(void) { return m_vehType == VEHICLE_TYPE_CAR; }
    bool IsBoat(void) { return m_vehType == VEHICLE_TYPE_BOAT; }
    bool IsTrain(void) { return m_vehType == VEHICLE_TYPE_TRAIN; }
    bool IsHeli(void) { return m_vehType == VEHICLE_TYPE_HELI; }
    bool IsPlane(void) { return m_vehType == VEHICLE_TYPE_PLANE; }
    bool IsBike(void) { return m_vehType == VEHICLE_TYPE_BIKE; }

    void ProcessDelayedExplosion(void);
    int FindTyreNearestPoint(float x, float y);
    bool IsLawEnforcementVehicle(void);
    bool UsesSiren(void);
    bool IsVehicleNormal(void);
    bool CarHasRoof(void);
    bool IsUpsideDown(void);
    bool IsOnItsSide(void);
    bool CanBeDeleted(void);
    bool CanPedOpenLocks(CPed* ped);
    bool CanDoorsBeDamaged(void);
    bool CanPedEnterCar(void);
    bool CanPedExitCar(bool jumpExit);
    bool CanPedJumpOutCar(void);
    bool CanPedJumpOffBike(void);
    // do these two actually return something?
    CPed* SetUpDriver(void);
    CPed* SetupPassenger(int n);
    void SetDriver(CPed* driver);
    bool AddPassenger(CPed* passenger);
    bool AddPassenger(CPed* passenger, uint8_t n);
    void RemovePassenger(CPed* passenger);
    void RemoveDriver(void);
    bool IsDriver(CPed* ped);
    bool IsDriver(int32_t model);
    bool IsPassenger(CPed* ped);
    bool IsPassenger(int32_t model);
    void UpdatePassengerList(void);
    void ProcessCarAlarm(void);
    bool IsSphereTouchingVehicle(float sx, float sy, float sz, float radius);
    bool ShufflePassengersToMakeSpace(void);
    void DoFixedMachineGuns(void);
    void FireFixedMachineGuns(void);
    void ActivateBomb(void);
    void ActivateBombWhenEntered(void);
    void KillPedsInVehicle(void);
    void UpdateClumpAlpha(void);

    static void HeliDustGenerate(CEntity* heli, float radius, float ground, int rnd);
    void DoSunGlare(void);

    bool IsAlarmOn(void) { return m_nAlarmState != 0 && m_nAlarmState != -1 && GetStatus() != STATUS_WRECKED; }
    bool IsRealHeli(void) { return !!(pHandling->Flags & HANDLING_IS_HELI); }
    bool IsRealPlane(void) { return !!(pHandling->Flags & HANDLING_IS_PLANE); }

    static bool bWheelsOnlyCheat;
    static bool bAllDodosCheat;
    static bool bCheat3;
    static bool bCheat4;
    static bool bCheat5;
    static bool bCheat8;
    static bool bCheat9;
    static bool bCheat10;
    static bool bHoverCheat;
    static bool bAllTaxisHaveNitro;
    static bool m_bDisableMouseSteering;
    static bool bDisableRemoteDetonation;
    static bool bDisableRemoteDetonationOnContact;
};

export CVehicle* (__cdecl* FindPlayerVehicle)() = nullptr;

class Vehicle
{
public:
    Vehicle()
    {
        WFP::onGameInitEvent() += []()
        {
            auto pattern = hook::pattern("E8 ? ? ? ? 85 C0 74 ? E8 ? ? ? ? 8B 80 ? ? ? ? 83 F8 ? 75 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? EB");
            FindPlayerVehicle = (decltype(FindPlayerVehicle))injector::GetBranchDestination(pattern.get_first()).as_int();
        };
    }
} Vehicle;