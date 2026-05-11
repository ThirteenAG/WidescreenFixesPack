module;

#include <stdafx.h>
#include "rw.h"

export module Entity;

import Placeable;

export class CEntity : public CPlaceable
{
public:
    RwObject* m_pRwObject;						// 0x18

    /********** BEGIN CFLAGS (0x1C) **************/
    unsigned long	bUsesCollision : 1;				// does entity use collision
    unsigned long	bCollisionProcessed : 1;			// has object been processed by a ProcessEntityCollision function
    unsigned long	bIsStatic : 1;					// is entity static
    unsigned long	bHasContacted : 1;				// has entity processed some contact forces
    unsigned long	bIsStuck : 1;						// is entity stuck
    unsigned long	bIsInSafePosition : 1;			// is entity in a collision free safe position
    unsigned long	bWasPostponed : 1;				// was entity control processing postponed
    unsigned long	bIsVisible : 1;					//is the entity visible

    unsigned long	bIsBIGBuilding : 1;				// Set if this entity is a big building
    unsigned long	bRenderDamaged : 1;				// use damaged LOD models for objects with applicable damage
    unsigned long	bStreamingDontDelete : 1;			// Dont let the streaming remove this
    unsigned long	bRemoveFromWorld : 1;				// remove this entity next time it should be processed
    unsigned long	bHasHitWall : 1;					// has collided with a building (changes subsequent collisions)
    unsigned long	bImBeingRendered : 1;				// don't delete me because I'm being rendered
    unsigned long	bDrawLast : 1;						// draw object last
    unsigned long	bDistanceFade : 1;					// Fade entity because it is far away

    unsigned long	bDontCastShadowsOn : 1;			// Dont cast shadows on this object
    unsigned long	bOffscreen : 1;					// offscreen flag. This can only be trusted when it is set to true
    unsigned long	bIsStaticWaitingForCollision : 1; // this is used by script created entities - they are static until the collision is loaded below them
    unsigned long	bDontStream : 1;					// tell the streaming not to stream me
    unsigned long	bUnderwater : 1;					// this object is underwater change drawing order
    unsigned long	bHasPreRenderEffects : 1;			// Object has a prerender effects attached to it
    unsigned long	bIsTempBuilding : 1;				// whether or not the building is temporary (i.e. can be created and deleted more than once)
    unsigned long	bDontUpdateHierarchy : 1;			// Don't update the aniamtion hierarchy this frame

    unsigned long	bHasRoadsignText : 1;				// entity is roadsign and has some 2deffect text stuff to be rendered
    unsigned long	bDisplayedSuperLowLOD : 1;
    unsigned long	bIsProcObject : 1;				// set object has been generate by procedural object generator
    unsigned long	bBackfaceCulled : 1;				// has backface culling on
    unsigned long	bLightObject : 1;					// light object with directional lights
    unsigned long	bUnimportantStream : 1;			// set that this object is unimportant, if streaming is having problems
    unsigned long	bTunnel : 1;						// Is this model part of a tunnel
    unsigned long	bTunnelTransition : 1;			// This model should be rendered from within and outside of the tunnel
    /********** END CFLAGS **************/

    WORD			RandomSeed;					// 0x20
    short			m_nModelIndex;				// 0x22
    void* pReferences;				// 0x24
    void* m_pLastRenderedLink;		// 0x28
    WORD			m_nScanCode;				// 0x2C
    BYTE			m_iplIndex;					// 0x2E
    BYTE			m_areaCode;					// 0x2F
    CEntity* m_pLod;					// 0x30
    BYTE			numLodChildren;				// 0x34
    char			numLodChildrenRendered;		// 0x35

    //********* BEGIN CEntityInfo **********//
    BYTE			nType : 3;							// what type is the entity	// 0x36 (2 == Vehicle)
    BYTE			nStatus : 5;						// control status			// 0x36
    //********* END CEntityInfo ************//

public:
    inline short& ModelIndex()
    {
        return m_nModelIndex;
    };
    inline short GetModelIndex()
    {
        return m_nModelIndex;
    }
};