module;
#include <cstdint>
#include "rw.h"

export module ModelInfo;

export struct CBox
{
    CVector min;
    CVector max;
};

export struct CColSurface
{
    char material;
    char flag;
    char lighting;
    char light;
};

export struct CSphere
{
    CVector m_vecCenter;
    float m_fRadius;
};

export struct CColSphere
{
    CSphere sphere;
    CColSurface surface;
};

export struct CColBox
{
    CVector sup;
    CVector inf;
    CColSurface m_Surface;
};

export struct CColLine
{
    CVector m_vecStart;
    float m_fStartSize;
    RwV3d m_vecEnd;
    int m_fEndSize;
};

export struct CCompressedVector
{
    int16_t x;
    int16_t y;
    int16_t z;
};

export struct CColTriangle
{
    int16_t a;
    int16_t b;
    int16_t c;
    char material;
    char light;
};

export struct CColTrianglePlane
{
    CCompressedVector m_vNormal;
    int16_t distance;
    char orientation;
    char _pad;
};

export struct CColData
{
    int16_t m_wNumSpheres;
    int16_t m_wNumBoxes;
    int16_t m_wNumTriangles;
    char m_nNumLines;
    char m_bFlags;
    CColSphere* m_pSpheres;
    CColBox* m_pBoxes;
    CColLine* m_pLines;
    CCompressedVector* m_pVertices;
    CColTriangle* m_pTriangles;
    CColTrianglePlane* m_pTrianglePlanes;
    int m_dwNumShadowTriangles;
    int m_dwNumShadowVertices;
    CCompressedVector* m_pShadowVertices;
    int m_pShadowTriangles;
};

export struct CColModel
{
    CBox m_Box;
    CColSphere m_Sphere;
    CColData* m_pColData;
};

export struct CBaseModelInfo
{
    void* vmt;
    int m_dwKey;
    int16_t m_wUsageCount;
    int16_t m_wTxdIndex;
    char m_nAlpha;
    char m_n2dfxCount;
    int16_t m_w2dfxIndex;
    int16_t m_wObjectInfoIndex;
    uint16_t m_nMdlFlags;
    CColModel* m_pColModel;
    float m_fDrawDistance;
    RpClump* m_pRwObject;
};

struct tEffectLight
{
    RwRGBA m_color;
    float m_fCoronaFarClip;
    float m_fPointlightRange;
    float m_fCoronaSize;
    float m_fShadowSize;
    uint16_t m_nFlags;
    uint8_t m_nCoronaFlashType;
    bool m_bCoronaEnableReflection;
    uint8_t m_nCoronaFlareType;
    uint8_t m_nShadowColorMultiplier;
    char m_nShadowZDistance;
    char offsetX;
    char offsetY;
    char offsetZ;
    char _pad2E[2];
    RwTexture* m_pCoronaTex;
    RwTexture* m_pShadowTex;
    int field_38;
    int field_3C;
};

union t2dEffectUnion
{
    tEffectLight light;
    //tEffectParticle particle;
    //tEffectPedAttractor pedAttractor;
    //tEffectEnEx enEx;
    //tEffectRoadsign roadsign;
    //tEffectCoverPoint coverPoint;
    //tEffectEscalator escalator;
    //tEffectSlotmachineWheel slotmachineWheel;
    //tEffectFurniture furniture;
};

struct C2dEffect
{
    CVector m_vecPosn;
    unsigned int m_nType;
    t2dEffectUnion effect;
};

export namespace CModelInfo
{
    CBaseModelInfo*** pp_modelInfoPtrs = nullptr;
    CBaseModelInfo* (__cdecl* GetModelInfo)(const char* name, int* id) = nullptr;
    C2dEffect* (__fastcall* Get2dEffect)(CBaseModelInfo* minfo, void* edx, int32_t i) = nullptr;
}