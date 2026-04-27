module;
#include <stdafx.h>
#include <cstdint>
#include "common.h"

export module Physical;

import Placeable;
import Entity;

enum
{
    PHYSICAL_MAX_COLLISIONRECORDS = 6
};

export class CPhysical : public CEntity
{
public:
    int32_t m_audioEntityId;
    float m_phys_unused1;
    void* m_treadable[2]; // car and ped
    uint32_t m_nLastTimeCollided;
    CVector m_vecMoveSpeed; // velocity
    CVector m_vecTurnSpeed; // angular velocity
    CVector m_vecMoveFriction;
    CVector m_vecTurnFriction;
    CVector m_vecMoveSpeedAvg;
    CVector m_vecTurnSpeedAvg;
    float m_fMass;
    float m_fTurnMass; // moment of inertia
    float m_fForceMultiplier;
    float m_fAirResistance;
    float m_fElasticity;
    float m_fBuoyancy;
    CVector m_vecCentreOfMass;
    void* m_entryInfoList;
    void* m_movingListNode;

    int8_t m_phys_unused2;
    uint8_t m_nStaticFrames;
    uint8_t m_nCollisionRecords;
    bool m_bIsVehicleBeingShifted;
    CEntity* m_aCollisionRecords[PHYSICAL_MAX_COLLISIONRECORDS];

    float m_fDistanceTravelled;

    // damaged piece
    float m_fDamageImpulse;
    CEntity* m_pDamageEntity;
    CVector m_vecDamageNormal;
    int16_t m_nDamagePieceType;

    uint8_t bIsHeavy : 1;
    uint8_t bAffectedByGravity : 1;
    uint8_t bInfiniteMass : 1;
    uint8_t bIsInWater : 1;
    uint8_t m_phy_flagA10 : 1; // unused
    uint8_t m_phy_flagA20 : 1; // unused
    uint8_t bHitByTrain : 1;
    uint8_t bSkipLineCol : 1;

    uint8_t m_nSurfaceTouched;
    int8_t m_nZoneLevel;

    CPhysical(void);
    ~CPhysical(void);

    // from CEntity
    void Add(void);
    void Remove(void);
    CRect GetBoundRect(void);
    void ProcessControl(void);
    void ProcessShift(void);
    void ProcessCollision(void);

    void RemoveAndAdd(void);
    void AddToMovingList(void);
    void RemoveFromMovingList(void);
    void SetDamagedPieceRecord(uint16_t piece, float impulse, CEntity* entity, CVector dir);
    void AddCollisionRecord(CEntity* ent);
    void AddCollisionRecord_Treadable(CEntity* ent);
    bool GetHasCollidedWith(CEntity* ent);
    void RemoveRefsToEntity(CEntity* ent);
    static void PlacePhysicalRelativeToOtherPhysical(CPhysical* other, CPhysical* phys, CVector localPos);

    // get speed of point p relative to entity center
    CVector GetSpeed(const CVector& r)
    {
        return m_vecMoveSpeed + m_vecMoveFriction + CrossProduct(m_vecTurnFriction + m_vecTurnSpeed, r);
    }
    CVector GetSpeed(void) { return GetSpeed(CVector(0.0f, 0.0f, 0.0f)); }
    float GetMass(const CVector& pos, const CVector& dir)
    {
        return 1.0f / (CrossProduct(pos, dir).MagnitudeSqr() / m_fTurnMass +
                          1.0f / m_fMass);
    }
    float GetMassTweak(const CVector& pos, const CVector& dir, float t)
    {
        return 1.0f / (CrossProduct(pos, dir).MagnitudeSqr() / (m_fTurnMass * t) +
                          1.0f / (m_fMass * t));
    }

    const CVector& GetMoveSpeed() { return m_vecMoveSpeed; }
    void SetMoveSpeed(float x, float y, float z)
    {
        m_vecMoveSpeed.x = x;
        m_vecMoveSpeed.y = y;
        m_vecMoveSpeed.z = z;
    }
    void SetMoveSpeed(const CVector& speed)
    {
        m_vecMoveSpeed = speed;
    }
    const CVector& GetTurnSpeed() { return m_vecTurnSpeed; }
    void SetTurnSpeed(float x, float y, float z)
    {
        m_vecTurnSpeed.x = x;
        m_vecTurnSpeed.y = y;
        m_vecTurnSpeed.z = z;
    }
    const CVector& GetCenterOfMass() { return m_vecCentreOfMass; }
    void SetCenterOfMass(float x, float y, float z)
    {
        m_vecCentreOfMass.x = x;
        m_vecCentreOfMass.y = y;
        m_vecCentreOfMass.z = z;
    }
};