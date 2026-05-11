module;
#include <stdafx.h>
#include <cstdint>
#include "common.h"

export module Physical;

import Placeable;
import Entity;

class CQuaternion
{
public:
    CVector imag;
    float real;
};

export class CPhysical : public CEntity
{
public:
    int field_38;
    unsigned int m_nLastCollisionTime;

    bool bIsHeavy : 1; // double the mass
    bool bApplyGravity : 1;
    bool bDisableCollisionForce : 1;
    bool bCollidable : 1;
    bool bDisableTurnForce : 1;
    bool bDisableMoveForce : 1;
    bool bInfiniteMass : 1;
    bool bDisableZ : 1;

    bool bSubmergedInWater : 1;
    bool bOnSolidSurface : 1;
    bool bBroken : 1;
    bool bProcessCollisionEvenIfStationary : 1;
    bool bSkipLineCol : 1; // only used for peds
    bool bDontApplySpeed : 1;
    bool b15 : 1;
    bool bProcessingShift : 1;

    bool b17 : 1;
    bool bDisableSimpleCollision : 1;
    bool bBulletProof : 1;
    bool bFireProof : 1;
    bool bCollisionProof : 1;
    bool bMeleeProof : 1;
    bool bInvulnerable : 1;
    bool bExplosionProof : 1;

    bool bDontCollideWithFlyers : 1;
    bool bAttachedToEntity : 1;
    bool bAddMovingCollisionSpeed : 1;
    bool bTouchingWater : 1;
    bool bCanBeCollidedWith : 1;
    bool bDestroyed : 1;
    bool b31 : 1;
    bool b32 : 1;

    CVector          m_vecMoveSpeed;
    CVector          m_vecTurnSpeed;
    CVector          m_vecFrictionMoveSpeed;
    CVector          m_vecFrictionTurnSpeed;
    CVector          m_vecForce;
    CVector          m_vecTorque;
    float            m_fMass;
    float            m_fTurnMass;
    float            m_fVelocityFrequency;
    float            m_fAirResistance;
    float            m_fElasticity;
    float            m_fBuoyancyConstant;
    CVector          m_vecCentreOfMass;
    void* m_pCollisionList;
    void* m_pMovingList;
    char             m_nFakePhysics;
    unsigned char    m_nNumEntitiesCollided;
    unsigned char    m_nContactSurface;
    char field_BB;
    CEntity* m_apCollidedEntities[6];
    float            m_fMovingSpeed;
    float            m_fDamageIntensity;
    CEntity* m_pDamageEntity;
    CVector          m_vecLastCollisionImpactVelocity;
    CVector          m_vecLastCollisionPosn;
    unsigned short   m_nPieceType;
    short field_FA;  // padding
    class CPhysical* m_pAttachedTo;
    CVector          m_vecAttachOffset;
    CVector          m_vecAttachedEntityPosn;
    CQuaternion      m_qAttachedEntityRotation;
    CEntity* m_pEntityIgnoredCollision;
    float            m_fContactSurfaceBrightness;
    float            m_fDynamicLighting;
    void* m_pShadowData;
};