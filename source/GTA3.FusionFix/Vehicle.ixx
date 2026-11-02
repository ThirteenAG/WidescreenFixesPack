module;
#include <stdafx.h>
#include <cstdint>
#include "common.h"

export module Vehicle;

import Placeable;
import Entity;
import Physical;

using CPed = void;

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

enum
{
    RANDOM_VEHICLE = 1,
    MISSION_VEHICLE = 2,
    PARKED_VEHICLE = 3,
    PERMANENT_VEHICLE = 4,
};

enum eCarLock
{
    CARLOCK_NOT_USED,
    CARLOCK_UNLOCKED,
    CARLOCK_LOCKED,
    CARLOCK_LOCKOUT_PLAYER_ONLY,
    CARLOCK_LOCKED_PLAYER_INSIDE,
    CARLOCK_LOCKED_INITIALLY,
    CARLOCK_FORCE_SHUT_DOORS
};

enum eDoors
{
    DOOR_BONNET = 0,
    DOOR_BOOT,
    DOOR_FRONT_LEFT,
    DOOR_FRONT_RIGHT,
    DOOR_REAR_LEFT,
    DOOR_REAR_RIGHT
};

enum ePanels
{
    VEHPANEL_FRONT_LEFT,
    VEHPANEL_FRONT_RIGHT,
    VEHPANEL_REAR_LEFT,
    VEHPANEL_REAR_RIGHT,
    VEHPANEL_WINDSCREEN,
    VEHBUMPER_FRONT,
    VEHBUMPER_REAR,
};

enum eLights
{
    VEHLIGHT_FRONT_LEFT,
    VEHLIGHT_FRONT_RIGHT,
    VEHLIGHT_REAR_LEFT,
    VEHLIGHT_REAR_RIGHT,
};

enum
{
    CAR_PIECE_BONNET = 1,
    CAR_PIECE_BOOT,
    CAR_PIECE_BUMP_FRONT,
    CAR_PIECE_BUMP_REAR,
    CAR_PIECE_DOOR_LF,
    CAR_PIECE_DOOR_RF,
    CAR_PIECE_DOOR_LR,
    CAR_PIECE_DOOR_RR,
    CAR_PIECE_WING_LF,
    CAR_PIECE_WING_RF,
    CAR_PIECE_WING_LR,
    CAR_PIECE_WING_RR,
    CAR_PIECE_WHEEL_LF,
    CAR_PIECE_WHEEL_RF,
    CAR_PIECE_WHEEL_LR,
    CAR_PIECE_WHEEL_RR,
    CAR_PIECE_WINDSCREEN,
};

enum tWheelState
{
    WHEEL_STATE_NORMAL,   // standing still or rolling normally
    WHEEL_STATE_SPINNING, // rotating but not moving
    WHEEL_STATE_SKIDDING,
    WHEEL_STATE_FIXED, // not rotating
};

enum eFlightModel
{
    FLIGHT_MODEL_DODO,
    // not used in III
    FLIGHT_MODEL_RCPLANE,
    FLIGHT_MODEL_HELI,
    FLIGHT_MODEL_SEAPLANE
};

// TODO: what is this even?
enum eBikeWheelSpecial
{
    BIKE_WHEELSPEC_0, // both wheels on ground
    BIKE_WHEELSPEC_1, // rear wheel on ground
    BIKE_WHEELSPEC_2, // only front wheel on ground
    BIKE_WHEELSPEC_3, // can't happen
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
    uint8_t m_bSlowedDownBecauseOfCars : 1;
    uint8_t m_bSlowedDownBecauseOfPeds : 1;
    uint8_t m_bStayInCurrentLevel : 1;
    uint8_t m_bStayInFastLane : 1;
    uint8_t m_bIgnorePathfinding : 1;
    CVector m_vecDestinationCoors;
    void* m_aPathFindNodesInfo[NUM_PATH_NODES_IN_AUTOPILOT];
    int16_t m_nPathFindNodesCount;
    CVehicle* m_pTargetCar;
};

export class CVehicle : public CPhysical
{
public:
    // 0x128
    void* pHandling;
    CAutoPilot AutoPilot;
    uint8_t m_currentColour1;
    uint8_t m_currentColour2;
    int8_t m_aExtras[2];
    int16_t m_nAlarmState;
    int16_t m_nMissionValue;
    CPed* pDriver;
    CPed* pPassengers[8];
    uint8_t m_nNumPassengers;
    int8_t m_nNumGettingIn;
    int8_t m_nGettingInFlags;
    int8_t m_nGettingOutFlags;
    uint8_t m_nNumMaxPassengers;
    float field_1D0[4];
    CEntity* m_pCurGroundEntity;
    void* m_pCarFire;
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

    int8_t m_numPedsUseItAsCover;
    uint8_t m_nAmmoInClip; // Used to make the guns on boat do a reload (20 by default)
    int8_t m_nPacManPickupsCarried;
    uint8_t m_nRoadblockType;
    int16_t m_nRoadblockNode;
    float m_fHealth; // 1000.0f = full health. 250.0f = fire. 0 -> explode
    uint8_t m_nCurrentGear;
    float m_fChangeGearTime;
    uint32_t m_nGunFiringTime; // last time when gun on vehicle was fired (used on boats)
    uint32_t m_nTimeOfDeath;
    uint16_t m_nTimeBlocked;
    int16_t m_nBombTimer; // goes down with each frame
    CEntity* m_pBlowUpEntity;
    float m_fMapObjectHeightAhead;  // front Z?
    float m_fMapObjectHeightBehind; // rear Z?
    eCarLock m_nDoorLock;
    int8_t m_nLastWeaponDamage; // see eWeaponType, -1 if no damage
    uint8_t m_nRadioStation;
    uint8_t m_bRainAudioCounter;
    uint8_t m_bRainSamplesCounter;
    uint8_t m_nCarHornTimer;
    uint8_t m_nCarHornPattern; // last horn?
    bool m_bSirenOrAlarm;
    int8_t m_comedyControlState;
    CStoredCollPoly m_aCollPolys[2]; // poly which is under front/rear part of car
    float m_fSteerInput;
    eVehicleType m_vehType;

    static void* operator new(size_t) throw();
    static void* operator new(size_t sz, int slot) throw();
    static void operator delete(void*, size_t) throw();
    static void operator delete(void*, int) throw();

    // from CEntity
    void SetModelIndex(uint32_t id);
    bool SetupLighting(void);
    void RemoveLighting(bool);
    void FlagToDestroyWhenNextProcessed(void) {}

    bool IsCar(void) { return m_vehType == VEHICLE_TYPE_CAR; }
    bool IsBoat(void) { return m_vehType == VEHICLE_TYPE_BOAT; }
    bool IsTrain(void) { return m_vehType == VEHICLE_TYPE_TRAIN; }
    bool IsHeli(void) { return m_vehType == VEHICLE_TYPE_HELI; }
    bool IsPlane(void) { return m_vehType == VEHICLE_TYPE_PLANE; }
    bool IsBike(void) { return m_vehType == VEHICLE_TYPE_BIKE; }

    void FlyingControl(eFlightModel flightModel);
    void ExtinguishCarFire(void);
    void ProcessDelayedExplosion(void);
    float ProcessWheelRotation(tWheelState state, const CVector& fwd, const CVector& speed, float radius);
    bool IsLawEnforcementVehicle(void);
    void ChangeLawEnforcerState(uint8_t enable);
    bool UsesSiren(uint32_t id);
    bool IsVehicleNormal(void);
    bool CarHasRoof(void);
    bool IsUpsideDown(void);
    bool IsOnItsSide(void);
    bool CanBeDeleted(void);
    bool CanPedOpenLocks(CPed* ped);
    bool CanPedEnterCar(void);
    bool CanPedExitCar(void);
    CPed* SetUpDriver(void);
    CPed* SetupPassenger(int n);
    void SetDriver(CPed* driver);
    bool AddPassenger(CPed* passenger);
    bool AddPassenger(CPed* passenger, uint8_t n);
    void RemovePassenger(CPed* passenger);
    void RemoveDriver(void);
    void ProcessCarAlarm(void);
    bool IsSphereTouchingVehicle(float sx, float sy, float sz, float radius);
    bool ShufflePassengersToMakeSpace(void);
    void DoFixedMachineGuns(void);
    bool IsAlarmOn(void) { return m_nAlarmState != 0 && m_nAlarmState != -1 && GetStatus() != STATUS_WRECKED; }
};

export CVehicle* (__cdecl* FindPlayerVehicle)() = nullptr;

class Vehicle
{
public:
    Vehicle()
    {
        WFP::onGameInitEvent() += []()
        {
            auto pattern = hook::pattern("E8 ? ? ? ? 85 C0 0F 84 ? ? ? ? E8 ? ? ? ? 89 D9");
            FindPlayerVehicle = (decltype(FindPlayerVehicle))injector::GetBranchDestination(pattern.get_first()).as_int();
        };
    }
} Vehicle;