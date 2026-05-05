module;
#include <stdafx.h>
#include <cstdint>
#include "common.h"

export module Physical;

import Placeable;
import Entity;

class CPtrNode
{
public:
    void* item;
    CPtrNode* prev;
    CPtrNode* next;
};

class CPtrList
{
public:
    CPtrNode* first;

    CPtrList(void) { first = nil; }
    ~CPtrList(void) { Flush(); }
    CPtrNode* FindItem(void* item)
    {
        CPtrNode* node;
        for (node = first; node; node = node->next)
            if (node->item == item)
                return node;
        return nil;
    }
    CPtrNode* InsertNode(CPtrNode* node)
    {
        node->prev = nil;
        node->next = first;
        if (first)
            first->prev = node;
        first = node;
        return node;
    }
    CPtrNode* InsertItem(void* item)
    {
        CPtrNode* node = new CPtrNode;
        node->item = item;
        InsertNode(node);
        return node;
    }
    void RemoveNode(CPtrNode* node)
    {
        if (node == first)
            first = node->next;
        if (node->prev)
            node->prev->next = node->next;
        if (node->next)
            node->next->prev = node->prev;
    }
    void DeleteNode(CPtrNode* node)
    {
        RemoveNode(node);
        delete node;
    }
    void RemoveItem(void* item)
    {
        CPtrNode* node, * next;
        for (node = first; node; node = next)
        {
            next = node->next;
            if (node->item == item)
                DeleteNode(node);
        }
    }
    void Flush(void)
    {
        CPtrNode* node, * next;
        for (node = first; node; node = next)
        {
            next = node->next;
            DeleteNode(node);
        }
    }
};

class CSector;

// This records in which sector list a Physical is
class CEntryInfoNode
{
public:
    CPtrList* list;     // list in sector
    CPtrNode* listnode; // node in list
    CSector* sector;

    CEntryInfoNode* prev;
    CEntryInfoNode* next;
};

class CEntryInfoList
{
public:
    CEntryInfoNode* first;

    CEntryInfoNode* InsertNode(CEntryInfoNode* node)
    {
        node->prev = nil;
        node->next = first;
        if (first)
            first->prev = node;
        first = node;
        return node;
    }
    CEntryInfoNode* InsertItem(CPtrList* list, CPtrNode* listnode, CSector* sect)
    {
        CEntryInfoNode* node = new CEntryInfoNode;
        node->list = list;
        node->listnode = listnode;
        node->sector = sect;
        InsertNode(node);
        return node;
    }
    void RemoveNode(CEntryInfoNode* node)
    {
        if (node == first)
            first = node->next;
        if (node->prev)
            node->prev->next = node->next;
        if (node->next)
            node->next->prev = node->prev;
    }
    void DeleteNode(CEntryInfoNode* node)
    {
        RemoveNode(node);
        delete node;
    }
    void Flush(void)
    {
        CEntryInfoNode* node, * next;
        for (node = first; node; node = next)
        {
            next = node->next;
            DeleteNode(node);
        }
    }
};

enum
{
    PHYSICAL_MAX_COLLISIONRECORDS = 6
};

export class CPhysical : public CEntity
{
public:
    int32_t m_audioEntityId;
    float m_phys_unused1;
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
    CEntryInfoList m_entryInfoList;
    CPtrNode* m_movingListNode;

    int8_t m_phys_unused2;
    uint8_t m_nStaticFrames;
    uint8_t m_nCollisionRecords;
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
    uint8_t m_phy_flagA08 : 1;
    uint8_t bIsInWater : 1;
    uint8_t m_phy_flagA20 : 1; // unused
    uint8_t bHitByTrain : 1;
    uint8_t bSkipLineCol : 1;

    uint8_t bIsFrozen : 1;
    uint8_t bDontLoadCollision : 1;
    uint8_t m_bIsVehicleBeingShifted : 1; // wrong name - also used on but never set for peds
    uint8_t bJustCheckCollision : 1;      // just see if there is a collision

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
    void AddToMoveSpeed(float x, float y, float z)
    {
        m_vecMoveSpeed.x += x;
        m_vecMoveSpeed.y += y;
        m_vecMoveSpeed.z += z;
    }
    void AddToMoveSpeed(const CVector& addition)
    {
        m_vecMoveSpeed += addition;
    }
    void AddToMoveSpeed(const CVector2D& addition)
    {
        m_vecMoveSpeed += CVector(addition.x, addition.y, 0.0f);
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

    void ApplyMoveSpeed(void);
    void ApplyTurnSpeed(void);
    // j(x,y,z) is direction of force, p(x,y,z) is point relative to model center where force is applied
    void ApplyTurnForce(float jx, float jy, float jz, float px, float py, float pz);
    // springRatio: 1.0 fully extended, 0.0 fully compressed
    bool ApplySpringCollision(float springConst, CVector& springDir, CVector& point, float springRatio, float bias);
    bool ApplySpringCollisionAlt(float springConst, CVector& springDir, CVector& point, float springRatio, float bias, CVector& forceDir);
    bool ApplySpringDampening(float damping, CVector& springDir, CVector& point, CVector& speed);
    void ApplyGravity(void);
    void ApplyFriction(void);
    void ApplyAirResistance(void);

    bool ProcessShiftSectorList(CPtrList* ptrlists);
    bool ProcessCollisionSectorList_SimpleCar(CPtrList* lists);
    bool ProcessCollisionSectorList(CPtrList* lists);
    bool CheckCollision(void);
    bool CheckCollision_SimpleCar(void);
};