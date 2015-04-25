/*
 *  RenderWare forwarding, helpful to make the compilation speed faster, no need to include the heavy RenderWare.h when you
 *  only want type names
 */
#ifndef RENDERWAREFWD_H
#define	RENDERWAREFWD_H

typedef struct RwV2d RwV2d;
typedef struct RwV3d RwV3d;
typedef struct RwRect RwRect;
typedef struct RwSphere RwSphere;
typedef struct RwLine RwLine;
typedef struct RwTexCoords RwTexCoords;
typedef struct RwSLLink RwSLLink;
typedef struct RwSingleList RwSingleList;
typedef struct RwLLLink  RwLLLink;
typedef struct RwLinkList RwLinkList;
typedef struct RwSurfaceProperties RwSurfaceProperties;
typedef struct RwPlane RwPlane;
typedef struct RwObject RwObject;
typedef struct RwMemoryFunctions RwMemoryFunctions;
typedef struct RwFreeList RwFreeList;
typedef struct RwStreamMemory RwStreamMemory;
typedef struct RwStreamCustom RwStreamCustom;
typedef struct RwStream RwStream;
typedef struct RwMemory RwMemory;
typedef struct RwStringFunctions RwStringFunctions;
typedef struct RwMatrix RwMatrix;
typedef RwMatrix RwMatrixTag;
typedef struct RwD3D9Vertex RwD3D9Vertex;
typedef struct RwDevice RwDevice;
typedef struct RwGlobals RwGlobals;
typedef struct RwMetrics RwMetrics;
typedef struct RwResEntry RwResEntry;
typedef struct RwRGBAReal RwRGBAReal;
typedef struct RwRGBA RwRGBA;
typedef struct RwChunkHeaderInfo RwChunkHeaderInfo;
typedef struct RxPipeline RxPipeline;
typedef struct RwRaster RwRaster;
typedef struct RxObjSpace3DVertex RxObjSpace3DVertex;
typedef struct RwImage RwImage;
typedef struct RwTexDictionary RwTexDictionary;
typedef struct RwTexture RwTexture;
typedef struct RxD3D9VertexStream RxD3D9VertexStream;
typedef struct RwFrame RwFrame;
typedef struct RpHAnimFrameExtension RpHAnimFrameExtension;
typedef struct RwBBox RwBBox;
typedef struct RwFrustumPlane RwFrustumPlane;
typedef struct RwObjectHasFrame RwObjectHasFrame;
typedef struct RwCamera RwCamera;
typedef struct RxD3D9ResEntryHeader RxD3D9ResEntryHeader;
typedef struct RxD3D9InstanceData RxD3D9InstanceData;
typedef struct RpMaterial RpMaterial;
typedef struct RpUVAnimMaterial RpUVAnimMaterial;
typedef struct RpMatFx RpMatFx;
typedef struct EnvMapFx EnvMapFx;
typedef struct RpMaterialList RpMaterialList;
typedef struct RpMeshHeader RpMeshHeader;
typedef struct RpMesh RpMesh;
typedef struct RpGeometry RpGeometry;
typedef struct RpMorphTarget RpMorphTarget;
typedef struct RpTriangle RpTriangle;
typedef struct RwMatrixWeights RwMatrixWeights;
typedef struct RpSkin RpSkin;
typedef struct RpLight RpLight;
typedef struct RpD3D9AttenuationParams RpD3D9AttenuationParams;
typedef struct RpAtomic RpAtomic;
typedef struct RpInterpolator RpInterpolator;
typedef struct RpClump RpClump;
typedef struct RtQuat RtQuat;
typedef struct RtAnimAnimation RtAnimAnimation;
typedef struct RtAnimInterpolatorInfo RtAnimInterpolatorInfo;
typedef struct RtAnimKeyFrameHeader RtAnimKeyFrameHeader;
typedef struct RtAnimInterpFrameHeader RtAnimInterpFrameHeader;
typedef struct RtAnimInterpolator RtAnimInterpolator;
typedef struct RpHAnimKeyFrame RpHAnimKeyFrame;
typedef struct RpHAnimInterpFrame RpHAnimInterpFrame;
typedef struct RpHAnimHierarchy RpHAnimHierarchy;
typedef struct RpHAnimNodeInfo RpHAnimNodeInfo;
typedef struct RpVertexNormal RpVertexNormal;
typedef struct RpWorldSector RpWorldSector;
typedef struct RpWorld RpWorld;
typedef struct RpSector RpSector;
typedef struct RxD3D9ResEntryHeader RxD3D9ResEntryHeader;
typedef struct RxD3D9InstanceData RxD3D9InstanceData;

// RwTextureAddressMode
enum RwTextureAddressMode
{
    rwTEXTUREADDRESSNATEXTUREADDRESS = 0,
    rwTEXTUREADDRESSWRAP,
    rwTEXTUREADDRESSMIRROR,
    rwTEXTUREADDRESSCLAMP,
    rwTEXTUREADDRESSBORDER,
    rwTEXTUREADDRESSMODEFORCEENUMSIZEINT = ((int)((~((unsigned int)0))>>1))
};

#endif

