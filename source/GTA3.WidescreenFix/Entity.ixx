module;
#include <cstdint>
#include "common.h"

export module Entity;

import Placeable;

export enum eEntityType
{
    ENTITY_TYPE_NOTHING = 0,
    ENTITY_TYPE_BUILDING,
    ENTITY_TYPE_VEHICLE,
    ENTITY_TYPE_PED,
    ENTITY_TYPE_OBJECT,
    ENTITY_TYPE_DUMMY,
};

export enum eEntityStatus
{
    STATUS_PLAYER,
    STATUS_PLAYER_PLAYBACKFROMBUFFER,
    STATUS_SIMPLE,
    STATUS_PHYSICS,
    STATUS_ABANDONED,
    STATUS_WRECKED,
    STATUS_TRAIN_MOVING,
    STATUS_TRAIN_NOT_MOVING,
    STATUS_HELI,
    STATUS_PLANE,
    STATUS_PLAYER_REMOTE,
    STATUS_PLAYER_DISABLED,
};

export class CEntity : public CPlaceable
{
public:
    void* m_rwObject;

protected:
    uint32_t m_type : 3;

private:
    uint32_t m_status : 5;

public:
    // flagsA
    uint32_t bUsesCollision : 1;      // does entity use collision
    uint32_t bCollisionProcessed : 1; // has object been processed by a ProcessEntityCollision function
    uint32_t bIsStatic : 1;           // is entity static
    uint32_t bHasContacted : 1;       // has entity processed some contact forces
    uint32_t bPedPhysics : 1;
    uint32_t bIsStuck : 1;          // is entity stuck
    uint32_t bIsInSafePosition : 1; // is entity in a collision free safe position
    uint32_t bUseCollisionRecords : 1;

    // flagsB
    uint32_t bWasPostponed : 1; // was entity control processing postponed
    uint32_t bExplosionProof : 1;
    uint32_t bIsVisible : 1; //is the entity visible
    uint32_t bHasCollided : 1;
    uint32_t bRenderScorched : 1;
    uint32_t bHasBlip : 1;
    uint32_t bIsBIGBuilding : 1; // Set if this entity is a big building
    uint32_t bRenderDamaged : 1; // use damaged LOD models for objects with applicable damage

    // flagsC
    uint32_t bBulletProof : 1;
    uint32_t bFireProof : 1;
    uint32_t bCollisionProof : 1;
    uint32_t bMeleeProof : 1;
    uint32_t bOnlyDamagedByPlayer : 1;
    uint32_t bStreamingDontDelete : 1; // Dont let the streaming remove this
    uint32_t bZoneCulled : 1;
    uint32_t bZoneCulled2 : 1; // only treadables+10m

    // flagsD
    uint32_t bRemoveFromWorld : 1; // remove this entity next time it should be processed
    uint32_t bHasHitWall : 1;      // has collided with a building (changes subsequent collisions)
    uint32_t bImBeingRendered : 1; // don't delete me because I'm being rendered
    uint32_t bTouchingWater : 1;   // used by cBuoyancy::ProcessBuoyancy
    uint32_t bIsSubway : 1;        // set when subway, but maybe different meaning?
    uint32_t bDrawLast : 1;        // draw object last
    uint32_t bNoBrightHeadLights : 1;
    uint32_t bDoNotRender : 1;

    // flagsE
    uint32_t bDistanceFade : 1; // Fade entity because it is far away
    uint32_t m_flagE2 : 1;

    uint16_t m_scanCode;
    uint16_t m_randomSeed;
    int16_t m_modelIndex;
    uint16_t m_level; // int16
    void* m_pFirstReference;

public:
    uint8_t GetType() const { return m_type; }
    void SetType(uint8_t type) { m_type = type; }
    uint8_t GetStatus() const { return m_status; }
    void SetStatus(uint8_t status) { m_status = status; }
    bool GetIsStatic(void) const { return bIsStatic; }
    void SetIsStatic(bool state) { bIsStatic = state; }

    CEntity(void);
    ~CEntity(void);

    virtual void Add(void);
    virtual void Remove(void);
    virtual void SetModelIndex(uint32_t id);
    virtual void SetModelIndexNoCreate(uint32_t id);
    virtual void CreateRwObject(void);
    virtual void DeleteRwObject(void);
    virtual CRect GetBoundRect(void);
    virtual void ProcessControl(void) {}
    virtual void ProcessCollision(void) {}
    virtual void ProcessShift(void) {}
    virtual void Teleport(CVector v) {}
    virtual void PreRender(void);
    virtual void Render(void);
    virtual bool SetupLighting(void);
    virtual void RemoveLighting(bool);
    virtual void FlagToDestroyWhenNextProcessed(void) {}

    bool IsBuilding(void) { return m_type == ENTITY_TYPE_BUILDING; }
    bool IsVehicle(void) { return m_type == ENTITY_TYPE_VEHICLE; }
    bool IsPed(void) { return m_type == ENTITY_TYPE_PED; }
    bool IsObject(void) { return m_type == ENTITY_TYPE_OBJECT; }
    bool IsDummy(void) { return m_type == ENTITY_TYPE_DUMMY; }

    CVector GetBoundCentre(void);
    float GetBoundRadius(void);
    float GetDistanceFromCentreOfMassToBaseOfModel(void);
    bool GetIsOnScreen(void);
    bool GetIsOnScreenComplex(void);
    bool IsVisible(void);
    bool IsVisibleComplex(void);
    int16_t GetModelIndex(void) const { return m_modelIndex; }
    void UpdateRwFrame(void);
    void SetupBigBuilding(void);

    void AttachToRwObject(void* obj);
    void DetachFromRwObject(void);

    void RegisterReference(CEntity** pent);
    void ResolveReferences(void);
    void PruneReferences(void);

    void PreRenderForGlassWindow(void);
    void AddSteamsFromGround(CVector* unused);
    void ModifyMatrixForTreeInWind(void);
    void ModifyMatrixForBannerInWind(void);
    void ProcessLightsForEntity(void);
};
