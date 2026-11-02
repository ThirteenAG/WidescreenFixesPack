module;
#include <cstdint>
#include "rw.h"

export module ModelInfo;

enum
{
    EFFECT_LIGHT,
    EFFECT_PARTICLE,
    EFFECT_ATTRACTOR,
    EFFECT_PED_ATTRACTOR,
    EFFECT_SUNGLARE
};

enum
{
    LIGHT_ON,
    LIGHT_ON_NIGHT,
    LIGHT_FLICKER,
    LIGHT_FLICKER_NIGHT,
    LIGHT_FLASH1,
    LIGHT_FLASH1_NIGHT,
    LIGHT_FLASH2,
    LIGHT_FLASH2_NIGHT,
    LIGHT_FLASH3,
    LIGHT_FLASH3_NIGHT,
    LIGHT_RANDOM_FLICKER,
    LIGHT_RANDOM_FLICKER_NIGHT,
    LIGHT_SPECIAL,
    LIGHT_BRIDGE_FLASH1,
    LIGHT_BRIDGE_FLASH2,
};

enum
{
    ATTRACTORTYPE_ICECREAM,
    ATTRACTORTYPE_STARE
};

enum
{
    LIGHTFLAG_LOSCHECK = 1,
    // same order as CPointLights flags, must start at 2
    LIGHTFLAG_FOG_NORMAL = 2,  // can have light and fog
    LIGHTFLAG_FOG_ALWAYS = 4,  // fog only
    LIGHTFLAG_HIDE_OBJECT = 8, // hide the object instead of rendering light (???)
    LIGHTFLAG_LONG_DIST = 16,
    LIGHTFLAG_FOG = (LIGHTFLAG_FOG_NORMAL | LIGHTFLAG_FOG_ALWAYS)
};

class C2dEffect
{
public:
    struct Light
    {
        float dist;
        float range; // of pointlight
        float size;
        float shadowSize;
        uint8_t lightType; // LIGHT_
        uint8_t roadReflection;
        uint8_t flareType;
        uint8_t shadowIntensity;
        uint8_t flags; // LIGHTFLAG_
        RwTexture* corona;
        RwTexture* shadow;
    };
    struct Particle
    {
        int particleType;
        CVector dir;
        float scale;
    };
    struct Attractor
    {
        CVector dir;
        int8_t type;
        uint8_t probability;
    };
    struct PedAttractor
    {
        CVector queueDir;
        CVector useDir;
        int8_t type;
    };

    CVector pos;
    CRGBA col;
    uint8_t type;
    union
    {
        Light light;
        Particle particle;
        Attractor attractor;
        PedAttractor pedattr;
    };

    C2dEffect(void) {}
    void Shutdown(void);
};

export struct CBox
{
    CVector min;
    CVector max;
    CVector GetSize(void) { return max - min; }
    void Set(const CVector& min, const CVector& max)
    {
        this->min = min;
        this->max = max;
    }
};

export struct CColBox : public CBox
{
    uint8_t surface;
    uint8_t piece;

    void Set(const CVector& min, const CVector& max, uint8_t surf, uint8_t piece);
    using CBox::Set;

    CColBox& operator=(const CColBox& other);
};

export struct CSphere
{
    CVector center;
    float radius;
    void Set(float radius, const CVector& center)
    {
        this->center = center;
        this->radius = radius;
    }
};

export struct CColSphere : public CSphere
{
    uint8_t surface;
    uint8_t piece;

    void Set(float radius, const CVector& center, uint8_t surf, uint8_t piece);
    bool IntersectRay(CVector const& from, CVector const& dir, CVector& entry, CVector& exit);
    using CSphere::Set;
};

export struct CColLine
{
    CVector p0;
    int pad0;
    CVector p1;
    int pad1;
};

export struct CompressedVector
{
    float x, y, z;
};

export struct CColTriangle
{
    uint16_t a;
    uint16_t b;
    uint16_t c;
    uint8_t surface;
};

export struct CColTrianglePlane
{
    CVector normal;
    float dist;
    uint8_t dir;
};

export template <typename T>
class CLink
{
public:
    T item;
    CLink<T>* prev;
    CLink<T>* next;

    void Insert(CLink<T>* link)
    {
        link->next = this->next;
        this->next->prev = link;
        link->prev = this;
        this->next = link;
    }
    void Remove(void)
    {
        this->prev->next = this->next;
        this->next->prev = this->prev;
    }
};

export struct CColModel
{
    CSphere boundingSphere;
    CBox boundingBox;
    int16_t numSpheres;
    int16_t numBoxes;
    int16_t numTriangles;
    int8_t numLines;
    uint8_t level; // colstore slot but probably still named level
    bool ownsCollisionVolumes;
    CColSphere* spheres;
    CColLine* lines;
    CColBox* boxes;
    CompressedVector* vertices;
    CColTriangle* triangles;
    CColTrianglePlane* trianglePlanes;

    CColModel(void);
    ~CColModel(void);
    void RemoveCollisionVolumes(void);
    void CalculateTrianglePlanes(void);
    void RemoveTrianglePlanes(void);
    CLink<CColModel*>* GetLinkPtr(void);
    void SetLinkPtr(CLink<CColModel*>*);
    void GetTrianglePoint(CVector& v, int i) const;

    void* operator new(size_t) throw();
    void operator delete(void* p, size_t) throw();
    CColModel& operator=(const CColModel& other);
};

#define MAX_MODEL_NAME (24)

export enum ModelInfoType
{
    MITYPE_NA,
    MITYPE_SIMPLE,
    MITYPE_MLO, // unused but still in enum
    MITYPE_TIME,
    MITYPE_WEAPON,
    MITYPE_CLUMP,
    MITYPE_VEHICLE,
    MITYPE_PED,
    MITYPE_XTRACOMPS, // unused but still in enum
    MITYPE_HAND       // xbox and mobile
};

export class CBaseModelInfo
{
public:
    char m_name[MAX_MODEL_NAME];
    uint8_t m_type;
    uint8_t m_num2dEffects;
    bool m_bOwnsColModel;
    CColModel* m_colModel;
    int16_t m_2dEffectsID;
    int16_t m_objectId;
    uint16_t m_refCount;
    int16_t m_txdSlot;

public:
    uint8_t GetModelType() const { return m_type; }
    bool IsBuilding(void) { return m_type == MITYPE_SIMPLE || m_type == MITYPE_TIME; }
    bool IsSimple(void) { return m_type == MITYPE_SIMPLE || m_type == MITYPE_TIME || m_type == MITYPE_WEAPON; }
    bool IsClump(void) { return m_type == MITYPE_CLUMP || m_type == MITYPE_PED || m_type == MITYPE_VEHICLE; }
    char* GetModelName(void) { return m_name; }
    void SetModelName(const char* name) { strncpy(m_name, name, MAX_MODEL_NAME); }
    void SetColModel(CColModel* col, bool owns = false)
    {
        m_colModel = col;
        m_bOwnsColModel = owns;
    }
    CColModel* GetColModel(void) { return m_colModel; }
    bool DoesOwnColModel(void) { return m_bOwnsColModel; }
    void DeleteCollisionModel(void);
    void ClearTexDictionary(void) { m_txdSlot = -1; }
    int16_t GetObjectID(void) { return m_objectId; }
    void SetObjectID(int16_t id) { m_objectId = id; }
    int16_t GetTxdSlot(void) { return m_txdSlot; }
    void AddRef(void);
    void RemoveRef(void);
    void SetTexDictionary(const char* name);
    void AddTexDictionaryRef(void);
    void RemoveTexDictionaryRef(void);
    void Init2dEffects(void);
    void Add2dEffect(C2dEffect* fx);
    C2dEffect* Get2dEffect(int n);
    uint8_t GetNum2dEffects() const { return m_num2dEffects; }
    uint16_t GetNumRefs() const { return m_refCount; }
};

export class CSimpleModelInfo : public CBaseModelInfo
{
public:
    // atomics[2] is often a pointer to the non-LOD modelinfo
    void* m_atomics[3];
    // m_lodDistances[2] holds the near distance for LODs
    float m_lodDistances[3];
    uint8_t m_numAtomics;
    uint8_t m_alpha;
    uint16_t m_firstDamaged : 2; // 0: no damage model
    // 1: 1 and 2 are damage models
    // 2: 2 is damage model
    uint16_t m_wetRoadReflection : 1;
    uint16_t m_isDamaged : 1;

    uint16_t m_isBigBuilding : 1;
    uint16_t m_noFade : 1;
    uint16_t m_drawLast : 1;
    uint16_t m_additive : 1;

    uint16_t m_isSubway : 1;
    uint16_t m_ignoreLight : 1;
    uint16_t m_noZwrite : 1;
    uint16_t m_noShadows : 1;

    uint16_t m_ignoreDrawDist : 1;
    uint16_t m_isCodeGlass : 1;
    uint16_t m_isArtistGlass : 1;

    CSimpleModelInfo(void)
        : CBaseModelInfo(MITYPE_SIMPLE)
    {
    }
    CSimpleModelInfo(ModelInfoType id)
        : CBaseModelInfo(id)
    {
    }
    ~CSimpleModelInfo() {}
    void DeleteRwObject(void);
    RwObject* CreateInstance(void);
    RwObject* CreateInstance(RwMatrix*);
    RwObject* GetRwObject(void) { return (RwObject*)m_atomics[0]; }

    void Init(void);
    void IncreaseAlpha(void);
    void SetLodDistances(float* dist);
    float GetLodDistance(int i);
    float GetNearDistance(void);
    float GetLargestLodDistance(void);
    void FindRelatedModel(int32_t minID, int32_t maxID);
    void SetupBigBuilding(int32_t minID, int32_t maxID);

    void SetNumAtomics(int n) { m_numAtomics = n; }
    CSimpleModelInfo* GetRelatedModel(void)
    {
        return (CSimpleModelInfo*)m_atomics[2];
    }
};

export namespace CModelInfo
{
    CBaseModelInfo*** pp_modelInfoPtrs = nullptr;
}