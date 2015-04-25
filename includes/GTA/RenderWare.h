/********************************SA::Render source file*************************************/
/* File creator: DK22Pac                                                                   */
/* File editors: DK22Pac                                                                   */
/* File descrip: RenderWare structures/enumerations/functions/defines are represented here.*/
/* File created: 17.04.2013                                                                */
/* File last ed: 27.04.2013                                                                */
#pragma once

#include "RenderWareTypes.h"

#ifndef _IDA_EXPORT
#include <stdarg.h>
#endif
// defines
#ifdef _D3D_INCLUDE
#include <D3D9Headers/d3dx9.h>
#endif
#define RWFORCEENUMSIZEINT ((int)((~((unsigned int)0))>>1))
#define RWRGBALONG(r,g,b,a) ((unsigned int) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))
#define MAKECHUNKID(vendorID, chunkID) (((vendorID & 0xFFFFFF) << 8) | (chunkID & 0xFF))
#define GETOBJECTID(chunkID) (chunkID & 0xFF)
#define GETVENDORID(chunkID) ((chunkID >> 8) & 0xFFFFFF)
#define rwRASTERLOCKREADWRITE   (rwRASTERLOCKREAD|rwRASTERLOCKWRITE)
#define rwRASTERPIXELLOCKED     (rwRASTERPIXELLOCKEDREAD | rwRASTERPIXELLOCKEDWRITE)
#define rwRASTERPALETTELOCKED   (rwRASTERPALETTELOCKEDREAD | rwRASTERPALETTELOCKEDWRITE)
#define rwRASTERLOCKED          (rwRASTERPIXELLOCKED|rwRASTERPALETTELOCKED)
#define rwTEXTUREFILTERMODEMASK     0x000000FF
#define rwTEXTUREADDRESSINGUMASK    0x00000F00
#define rwTEXTUREADDRESSINGVMASK    0x0000F000
#define rwTEXTUREADDRESSINGMASK     (rwTEXTUREADDRESSINGUMASK | rwTEXTUREADDRESSINGVMASK)
#define rpLIGHTPOSITIONINGSTART 0x80
#define rtANIMGETINTERPFRAME( anim, nodeIndex ) ((void *)(((unsigned char *)&(anim[1]) + ((nodeIndex) * anim->currentInterpKeyFrameSize))))
#define rpHANIMPOPPARENTMATRIX      0x01
#define rpHANIMPUSHPARENTMATRIX     0x02
#define rwFRAME 0
#define rpATOMIC 1
#define rpCLUMP 2
#define rpLIGHT 3
#define rwCAMERA 4
#define rwTEXDICTIONARY 6
#define rpWORLD 7
#define rpGEOMETRY 8

#define MACRO_START do
#define MACRO_STOP while(0)

// structures
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
// classes
#ifndef _IDA_EXPORT
typedef class CEnvMapMaterial CEnvMapMaterial;
typedef class CSpecularMaterial CSpecularMaterial;
typedef class CNightVertexColors CNightVertexColors;
typedef class CBreakableGeometry CBreakableGeometry;
typedef class CHAnimIFrame CHAnimIFrame;
typedef class CBone CBone;
typedef class C2dfxStore C2dfxStore;
typedef class CHAnim CHAnim;
typedef class CClumpVisibility CClumpVisibility;
typedef class CXEnvMapInfo CXEnvMapInfo;
typedef class CAtomicVisibility CAtomicVisibility;
#endif

// unions
typedef union RwStreamFile RwStreamFile;
typedef union RwStreamUnion RwStreamUnion;


#if 1
/****************************************Enumerations***************************************/
// RwStreamType
enum RwStreamType
{
    rwNASTREAM = 0,
    rwSTREAMFILE,
    rwSTREAMFILENAME,
    rwSTREAMMEMORY,
    rwSTREAMCUSTOM,
    rwSTREAMTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwStreamAccessType
enum RwStreamAccessType
{
    rwNASTREAMACCESS = 0,
    rwSTREAMREAD,
    rwSTREAMWRITE,
    rwSTREAMAPPEND,
    rwSTREAMACCESSTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwRenderState
enum RwRenderState
{
    rwRENDERSTATENARENDERSTATE = 0,
    rwRENDERSTATETEXTURERASTER,
    rwRENDERSTATETEXTUREADDRESS,
    rwRENDERSTATETEXTUREADDRESSU,
    rwRENDERSTATETEXTUREADDRESSV,
    rwRENDERSTATETEXTUREPERSPECTIVE,
    rwRENDERSTATEZTESTENABLE,
    rwRENDERSTATESHADEMODE,
    rwRENDERSTATEZWRITEENABLE,
    rwRENDERSTATETEXTUREFILTER,
    rwRENDERSTATESRCBLEND,
    rwRENDERSTATEDESTBLEND,
    rwRENDERSTATEVERTEXALPHAENABLE,
    rwRENDERSTATEBORDERCOLOR,
    rwRENDERSTATEFOGENABLE,
    rwRENDERSTATEFOGCOLOR,
    rwRENDERSTATEFOGTYPE,
    rwRENDERSTATEFOGDENSITY,
    rwRENDERSTATECULLMODE = 20,
    rwRENDERSTATESTENCILENABLE,
    rwRENDERSTATESTENCILFAIL,
    rwRENDERSTATESTENCILZFAIL,
    rwRENDERSTATESTENCILPASS,
    rwRENDERSTATESTENCILFUNCTION,
    rwRENDERSTATESTENCILFUNCTIONREF,
    rwRENDERSTATESTENCILFUNCTIONMASK,
    rwRENDERSTATESTENCILFUNCTIONWRITEMASK,
    rwRENDERSTATEALPHATESTFUNCTION,
    rwRENDERSTATEALPHATESTFUNCTIONREF,
    rwRENDERSTATEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwTextureCoordinateIndex
enum RwTextureCoordinateIndex
{
    rwNARWTEXTURECOORDINATEINDEX = 0,
    rwTEXTURECOORDINATEINDEX0,
    rwTEXTURECOORDINATEINDEX1,
    rwTEXTURECOORDINATEINDEX2,
    rwTEXTURECOORDINATEINDEX3,
    rwTEXTURECOORDINATEINDEX4,
    rwTEXTURECOORDINATEINDEX5,
    rwTEXTURECOORDINATEINDEX6,
    rwTEXTURECOORDINATEINDEX7,
    rwTEXTURECOORDINATEINDEXFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwPlaneType
enum RwPlaneType
{
    rwXPLANE = 0,
    rwYPLANE = 4,
    rwZPLANE = 8,
    rwPLANETYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwPluginVendor
enum RwPluginVendor
{
    rwVENDORID_CORE             = 0x000000L,
    rwVENDORID_CRITERIONTK      = 0x000001L,
    rwVENDORID_REDLINERACER     = 0x000002L,
    rwVENDORID_CSLRD            = 0x000003L,
    rwVENDORID_CRITERIONINT     = 0x000004L,
    rwVENDORID_CRITERIONWORLD   = 0x000005L,
    rwVENDORID_BETA             = 0x000006L,
    rwVENDORID_CRITERIONRM      = 0x000007L,
    rwVENDORID_CRITERIONRWA     = 0x000008L,
    rwVENDORID_CRITERIONRWP     = 0x000009L,
    rwPLUGINVENDORFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
#ifndef _IDA_EXPORT
// RwCorePluginID
enum RwCorePluginID
{
    rwID_NAOBJECT               = MAKECHUNKID(rwVENDORID_CORE, 0x00),
    rwID_STRUCT                 = MAKECHUNKID(rwVENDORID_CORE, 0x01),
    rwID_STRING                 = MAKECHUNKID(rwVENDORID_CORE, 0x02),
    rwID_EXTENSION              = MAKECHUNKID(rwVENDORID_CORE, 0x03),
    rwID_CAMERA                 = MAKECHUNKID(rwVENDORID_CORE, 0x05),
    rwID_TEXTURE                = MAKECHUNKID(rwVENDORID_CORE, 0x06),
    rwID_MATERIAL               = MAKECHUNKID(rwVENDORID_CORE, 0x07),
    rwID_MATLIST                = MAKECHUNKID(rwVENDORID_CORE, 0x08),
    rwID_ATOMICSECT             = MAKECHUNKID(rwVENDORID_CORE, 0x09),
    rwID_PLANESECT              = MAKECHUNKID(rwVENDORID_CORE, 0x0A),
    rwID_WORLD                  = MAKECHUNKID(rwVENDORID_CORE, 0x0B),
    rwID_SPLINE                 = MAKECHUNKID(rwVENDORID_CORE, 0x0C),
    rwID_MATRIX                 = MAKECHUNKID(rwVENDORID_CORE, 0x0D),
    rwID_FRAMELIST              = MAKECHUNKID(rwVENDORID_CORE, 0x0E),
    rwID_GEOMETRY               = MAKECHUNKID(rwVENDORID_CORE, 0x0F),
    rwID_CLUMP                  = MAKECHUNKID(rwVENDORID_CORE, 0x10),
    rwID_LIGHT                  = MAKECHUNKID(rwVENDORID_CORE, 0x12),
    rwID_UNICODESTRING          = MAKECHUNKID(rwVENDORID_CORE, 0x13),
    rwID_ATOMIC                 = MAKECHUNKID(rwVENDORID_CORE, 0x14),
    rwID_TEXTURENATIVE          = MAKECHUNKID(rwVENDORID_CORE, 0x15),
    rwID_TEXDICTIONARY          = MAKECHUNKID(rwVENDORID_CORE, 0x16),
    rwID_ANIMDATABASE           = MAKECHUNKID(rwVENDORID_CORE, 0x17),
    rwID_IMAGE                  = MAKECHUNKID(rwVENDORID_CORE, 0x18),
    rwID_SKINANIMATION          = MAKECHUNKID(rwVENDORID_CORE, 0x19),
    rwID_GEOMETRYLIST           = MAKECHUNKID(rwVENDORID_CORE, 0x1A),
    rwID_ANIMANIMATION          = MAKECHUNKID(rwVENDORID_CORE, 0x1B),
    rwID_HANIMANIMATION         = MAKECHUNKID(rwVENDORID_CORE, 0x1B),
    rwID_TEAM                   = MAKECHUNKID(rwVENDORID_CORE, 0x1C),
    rwID_CROWD                  = MAKECHUNKID(rwVENDORID_CORE, 0x1D),
    rwID_DMORPHANIMATION        = MAKECHUNKID(rwVENDORID_CORE, 0x1E),
    rwID_RIGHTTORENDER          = MAKECHUNKID(rwVENDORID_CORE, 0x1f),
    rwID_MTEFFECTNATIVE         = MAKECHUNKID(rwVENDORID_CORE, 0x20),
    rwID_MTEFFECTDICT           = MAKECHUNKID(rwVENDORID_CORE, 0x21),
    rwID_TEAMDICTIONARY         = MAKECHUNKID(rwVENDORID_CORE, 0x22),
    rwID_PITEXDICTIONARY        = MAKECHUNKID(rwVENDORID_CORE, 0x23),
    rwID_TOC                    = MAKECHUNKID(rwVENDORID_CORE, 0x24),
    rwID_PRTSTDGLOBALDATA       = MAKECHUNKID(rwVENDORID_CORE, 0x25),
    rwID_ALTPIPE                = MAKECHUNKID(rwVENDORID_CORE, 0x26),
    rwID_PIPEDS                 = MAKECHUNKID(rwVENDORID_CORE, 0x27),
    rwID_PATCHMESH              = MAKECHUNKID(rwVENDORID_CORE, 0x28),
    rwID_CHUNKGROUPSTART        = MAKECHUNKID(rwVENDORID_CORE, 0x29),
    rwID_CHUNKGROUPEND          = MAKECHUNKID(rwVENDORID_CORE, 0x2A),
    rwID_UVANIMDICT             = MAKECHUNKID(rwVENDORID_CORE, 0x2B),
    rwID_COLLTREE               = MAKECHUNKID(rwVENDORID_CORE, 0x2C),
    rwID_ENVIRONMENT            = MAKECHUNKID(rwVENDORID_CORE, 0x2D),
    rwID_COREPLUGINIDMAX        = MAKECHUNKID(rwVENDORID_CORE, 0x2E),
    rwCOREPLUGINIDFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
#endif
// RwCriterionCoreID
enum RwCriterionCoreID
{
    rwID_NACOREID               = MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x00),
    rwID_VECTORMODULE           = MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x01),
    rwID_MATRIXMODULE           = MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x02),
    rwID_FRAMEMODULE            = MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x03),
    rwID_STREAMMODULE           = MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x04),
    rwID_CAMERAMODULE           = MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x05),
    rwID_IMAGEMODULE            = MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x06),
    rwID_RASTERMODULE           = MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x07),
    rwID_TEXTUREMODULE          = MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x08),
    rwID_PIPEMODULE             = MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x09),
    rwID_IMMEDIATEMODULE        = MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x0A),
    rwID_RESOURCESMODULE        = MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x0B),
    rwID_DEVICEMODULE           = MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x0C),
    rwID_COLORMODULE            = MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x0D),
    rwID_POLYPIPEMODULE         = MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x0E),
    rwID_ERRORMODULE            = MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x0F),
    rwID_METRICSMODULE          = MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x10),
    rwID_DRIVERMODULE           = MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x11),
    rwID_CHUNKGROUPMODULE       = MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x12),
    rwCRITERIONCOREIDFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwPlatformID
enum RwPlatformID
{
    rwID_PCD3D7 = 1,
    rwID_PCOGL,
    rwID_MAC,
    rwID_PS2,
    rwID_XBOX,
    rwID_GAMECUBE,
    rwID_SOFTRAS,
    rwID_PCD3D8,
    rwID_PCD3D9,
    rwPLATFROMIDFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwOpCombineType
enum RwOpCombineType
{
    rwCOMBINEREPLACE = 0,
    rwCOMBINEPRECONCAT,
    rwCOMBINEPOSTCONCAT,
    rwOPCOMBINETYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwMatrixType
enum RwMatrixType
{
    rwMATRIXTYPENORMAL = 0x00000001,
    rwMATRIXTYPEORTHOGONAL = 0x00000002,
    rwMATRIXTYPEORTHONORMAL = 0x00000003,
    rwMATRIXTYPEMASK = 0x00000003,
    rwMATRIXTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwMatrixFlag
enum RwMatrixFlag
{
    rwMATRIXINTERNALIDENTITY = 0x00020000,
    rwMATRIXFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwMatrixOptimizations
enum RwMatrixOptimizations
{
    rwMATRIXOPTIMIZE_IDENTITY = 0x00020000,
    rwMATRIXOPTIMIZATIONSFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwShadeMode
enum RwShadeMode
{
    rwSHADEMODENASHADEMODE = 0,
    rwSHADEMODEFLAT,
    rwSHADEMODEGOURAUD,
    rwSHADEMODEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwTextureFilterMode
enum RwTextureFilterMode
{
    rwFILTERNAFILTERMODE = 0,
    rwFILTERNEAREST,
    rwFILTERLINEAR,
    rwFILTERMIPNEAREST,
    rwFILTERMIPLINEAR,
    rwFILTERLINEARMIPNEAREST,
    rwFILTERLINEARMIPLINEAR,
    rwTEXTUREFILTERMODEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwFogType
enum RwFogType
{
    rwFOGTYPENAFOGTYPE = 0,
    rwFOGTYPELINEAR,
    rwFOGTYPEEXPONENTIAL,
    rwFOGTYPEEXPONENTIAL2,
    rwFOGTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwBlendFunction
enum RwBlendFunction
{
    rwBLENDNABLEND = 0,
    rwBLENDZERO,
    rwBLENDONE,
    rwBLENDSRCCOLOR,
    rwBLENDINVSRCCOLOR,
    rwBLENDSRCALPHA,
    rwBLENDINVSRCALPHA,
    rwBLENDDESTALPHA,
    rwBLENDINVDESTALPHA,
    rwBLENDDESTCOLOR,
    rwBLENDINVDESTCOLOR,
    rwBLENDSRCALPHASAT,
    rwBLENDFUNCTIONFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
#if 0
// RwTextureAddressMode
enum RwTextureAddressMode
{
    rwTEXTUREADDRESSNATEXTUREADDRESS = 0,
    rwTEXTUREADDRESSWRAP,
    rwTEXTUREADDRESSMIRROR,
    rwTEXTUREADDRESSCLAMP,
    rwTEXTUREADDRESSBORDER,
    rwTEXTUREADDRESSMODEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
#endif
// RwStencilOperation
enum RwStencilOperation
{
    rwSTENCILOPERATIONNASTENCILOPERATION = 0,
    rwSTENCILOPERATIONKEEP,
    rwSTENCILOPERATIONZERO,
    rwSTENCILOPERATIONREPLACE,
    rwSTENCILOPERATIONINCRSAT,
    rwSTENCILOPERATIONDECRSAT,   
    rwSTENCILOPERATIONINVERT,
    rwSTENCILOPERATIONINCR,
    rwSTENCILOPERATIONDECR,
    rwSTENCILOPERATIONFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwStencilFunction
enum RwStencilFunction
{
    rwSTENCILFUNCTIONNASTENCILFUNCTION = 0,
    rwSTENCILFUNCTIONNEVER,
    rwSTENCILFUNCTIONLESS,
    rwSTENCILFUNCTIONEQUAL,
    rwSTENCILFUNCTIONLESSEQUAL,
    rwSTENCILFUNCTIONGREATER,
    rwSTENCILFUNCTIONNOTEQUAL,
    rwSTENCILFUNCTIONGREATEREQUAL,
    rwSTENCILFUNCTIONALWAYS,
    rwSTENCILFUNCTIONFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwAlphaTestFunction
enum RwAlphaTestFunction
{
    rwALPHATESTFUNCTIONNAALPHATESTFUNCTION = 0,
    rwALPHATESTFUNCTIONNEVER,
    rwALPHATESTFUNCTIONLESS,
    rwALPHATESTFUNCTIONEQUAL,
    rwALPHATESTFUNCTIONLESSEQUAL,
    rwALPHATESTFUNCTIONGREATER,
    rwALPHATESTFUNCTIONNOTEQUAL,
    rwALPHATESTFUNCTIONGREATEREQUAL,
    rwALPHATESTFUNCTIONALWAYS,
    rwALPHATESTFUNCTIONFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwCullMode
enum RwCullMode
{
    rwCULLMODENACULLMODE = 0,
    rwCULLMODECULLNONE,
    rwCULLMODECULLBACK,
    rwCULLMODECULLFRONT,
    rwCULLMODEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwPrimitiveType
enum RwPrimitiveType
{
    rwPRIMTYPENAPRIMTYPE = 0,
    rwPRIMTYPELINELIST = 1,
    rwPRIMTYPEPOLYLINE = 2,
    rwPRIMTYPETRILIST = 3,
    rwPRIMTYPETRISTRIP = 4,
    rwPRIMTYPETRIFAN = 5,
    rwPRIMTYPEPOINTLIST = 6,
    rwPRIMITIVETYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwEngineStatus
enum RwEngineStatus
{
    rwENGINESTATUSIDLE = 0,
    rwENGINESTATUSINITED = 1,
    rwENGINESTATUSOPENED = 2,
    rwENGINESTATUSSTARTED = 3,
    rwENGINESTATUSFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// rxEmbeddedPacketState
enum rxEmbeddedPacketState
{
    rxPKST_PACKETLESS = 0,
    rxPKST_UNUSED     = 1,
    rxPKST_INUSE      = 2,
    rxPKST_PENDING    = 3,
    rxEMBEDDEDPACKETSTATEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwRasterLockMode
enum RwRasterLockMode
{
    rwRASTERLOCKWRITE = 0x01,
    rwRASTERLOCKREAD = 0x02,
    rwRASTERLOCKNOFETCH = 0x04,
    rwRASTERLOCKRAW = 0x08,
    rwRASTERLOCKMODEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwRasterFlipMode
enum RwRasterFlipMode
{
    rwRASTERFLIPDONTWAIT = 0,
    rwRASTERFLIPWAITVSYNC = 1,
    rwRASTERFLIPMODEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwRasterType
enum RwRasterType
{
    rwRASTERTYPENORMAL = 0x00,
    rwRASTERTYPEZBUFFER = 0x01,
    rwRASTERTYPECAMERA = 0x02,
    rwRASTERTYPETEXTURE = 0x04,
    rwRASTERTYPECAMERATEXTURE = 0x05,
    rwRASTERTYPEMASK = 0x07,
    rwRASTERPALETTEVOLATILE = 0x40,
    rwRASTERDONTALLOCATE = 0x80,
    rwRASTERTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwRasterFormat
enum RwRasterFormat
{
    rwRASTERFORMATDEFAULT = 0x0000,
    rwRASTERFORMAT1555 = 0x0100,
    rwRASTERFORMAT565 = 0x0200,
    rwRASTERFORMAT4444 = 0x0300,
    rwRASTERFORMATLUM8 = 0x0400,
    rwRASTERFORMAT8888 = 0x0500,
    rwRASTERFORMAT888 = 0x0600,
    rwRASTERFORMAT16 = 0x0700,
    rwRASTERFORMAT24 = 0x0800,
    rwRASTERFORMAT32 = 0x0900,
    rwRASTERFORMAT555 = 0x0a00,
    rwRASTERFORMATAUTOMIPMAP = 0x1000,
    rwRASTERFORMATPAL8 = 0x2000,
    rwRASTERFORMATPAL4 = 0x4000,
    rwRASTERFORMATMIPMAP = 0x8000,
    rwRASTERFORMATPIXELFORMATMASK = 0x0f00,
    rwRASTERFORMATMASK = 0xff00,
    rwRASTERFORMATFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwRasterPrivateFlag
enum RwRasterPrivateFlag
{
    rwRASTERGAMMACORRECTED = 0x01,
    rwRASTERPIXELLOCKEDREAD = 0x02,
    rwRASTERPIXELLOCKEDWRITE = 0x04,
    rwRASTERPALETTELOCKEDREAD = 0x08,
    rwRASTERPALETTELOCKEDWRITE = 0x10,
    rwRASTERPIXELLOCKEDRAW = 0x20,
    rwRASTERPRIVATEFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwImageFlag
enum RwImageFlag
{
    rwNAIMAGEFLAG = 0x00,
    rwIMAGEALLOCATED = 0x1,
    rwIMAGEGAMMACORRECTED = 0x2,
    rwIMAGEFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwIm3DTransformFlags
enum RwIm3DTransformFlags
{
    rwIM3D_VERTEXUV      = 1,
    rwIM3D_ALLOPAQUE     = 2,
    rwIM3D_NOCLIP        = 4,
    rwIM3D_VERTEXXYZ     = 8,
    rwIM3D_VERTEXRGBA    = 16,
    rwIM3DTRANSFORMFLAGSFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwCameraClearMode
enum RwCameraClearMode
{
    rwCAMERACLEARIMAGE = 0x1,
    rwCAMERACLEARZ = 0x2,
    rwCAMERACLEARSTENCIL = 0x4,
    rwCAMERACLEARMODEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwCameraProjection
enum RwCameraProjection
{
    rwNACAMERAPROJECTION = 0,
    rwPERSPECTIVE = 1,
    rwPARALLEL = 2,
    rwCAMERAPROJECTIONFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RwFrustumTestResult
enum RwFrustumTestResult
{
    rwSPHEREOUTSIDE = 0,
    rwSPHEREBOUNDARY = 1,
    rwSPHEREINSIDE = 2,
    rwFRUSTUMTESTRESULTFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// rwD3D9VertexShaderEffectType
enum rwD3D9VertexShaderEffectType
{
    rwD3D9VERTEXSHADEREFFECT_NONE = 0,
    rwD3D9VERTEXSHADEREFFECT_BUMPMAP = 1,
    rwD3D9VERTEXSHADEREFFECT_ENVMAP,
    rwD3D9VERTEXSHADEREFFECT_BUMPENVMAP,
    rwD3D9VERTEXSHADEREFFECT_DUAL,
    rwD3D9VERTEXSHADEREFFECT_DUALREPLICATE,
    rwD3D9VERTEXSHADEREFFECT_UVANIM,
    rwD3D9VERTEXSHADEREFFECT_DUALUVANIM,
    rwD3D9VERTEXSHADEREFFECT_NORMALMAP,
    rwD3D9VERTEXSHADEREFFECT_NORMALENVMAP,
    rwD3D9VERTEXSHADEREFFECT_CUBEMAP,
    rwD3D9VERTEXSHADEREFFECT_BUMPCUBEMAP,
    rwD3D9VERTEXSHADEREFFECT_MAX = (1 << 6),
};
// rwD3D9VertexShaderFogMode
enum rwD3D9VertexShaderFogMode
{
    rwD3D9VERTEXSHADERFOG_NONE = 0,
    rwD3D9VERTEXSHADERFOG_LINEAR = 1,
    rwD3D9VERTEXSHADERFOG_EXP,
    rwD3D9VERTEXSHADERFOG_EXP2,
    rwD3D9VERTEXSHADERFOG_MAX
};
// RpMeshHeaderFlags
enum RpMeshHeaderFlags
{
    rpMESHHEADERTRISTRIP  = 0x0001,
    rpMESHHEADERTRIFAN    = 0x0002,
    rpMESHHEADERLINELIST  = 0x0004,
    rpMESHHEADERPOLYLINE  = 0x0008,
    rpMESHHEADERPOINTLIST = 0x0010,
    rpMESHHEADERPRIMMASK  = 0x00FF,
    rpMESHHEADERUNINDEXED = 0x0100,
    rpMESHHEADERFLAGSFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RpGeometryFlag
enum RpGeometryFlag
{
    rpGEOMETRYTRISTRIP  = 0x00000001,
    rpGEOMETRYPOSITIONS = 0x00000002,
    rpGEOMETRYTEXTURED  = 0x00000004,
    rpGEOMETRYPRELIT    = 0x00000008,
    rpGEOMETRYNORMALS   = 0x00000010,
    rpGEOMETRYLIGHT     = 0x00000020,
    rpGEOMETRYMODULATEMATERIALCOLOR = 0x00000040,
    rpGEOMETRYTEXTURED2 = 0x00000080,
    rpGEOMETRYNATIVE            = 0x01000000,
    rpGEOMETRYNATIVEINSTANCE    = 0x02000000,
    rpGEOMETRYFLAGSMASK         = 0x000000FF,
    rpGEOMETRYNATIVEFLAGSMASK   = 0x0F000000,
    rpGEOMETRYFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RpGeometryLockMode
enum RpGeometryLockMode
{
    rpGEOMETRYLOCKPOLYGONS   =   0x01,
    rpGEOMETRYLOCKVERTICES   =   0x02,
    rpGEOMETRYLOCKNORMALS    =   0x04,
    rpGEOMETRYLOCKPRELIGHT   =   0x08,
    rpGEOMETRYLOCKTEXCOORDS  =   0x10,
    rpGEOMETRYLOCKTEXCOORDS1 =   0x10,
    rpGEOMETRYLOCKTEXCOORDS2 =   0x20,
    rpGEOMETRYLOCKTEXCOORDS3 =   0x40,
    rpGEOMETRYLOCKTEXCOORDS4 =   0x80,
    rpGEOMETRYLOCKTEXCOORDS5 = 0x0100,
    rpGEOMETRYLOCKTEXCOORDS6 = 0x0200,
    rpGEOMETRYLOCKTEXCOORDS7 = 0x0400,
    rpGEOMETRYLOCKTEXCOORDS8 = 0x0800,
    rpGEOMETRYLOCKTEXCOORDSALL = 0x0ff0,
    rpGEOMETRYLOCKALL        = 0x0fff,
    rpGEOMETRYLOCKMODEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RpLightType
enum RpLightType
{
    rpNALIGHTTYPE = 0,
    rpLIGHTDIRECTIONAL,
    rpLIGHTAMBIENT,
    rpLIGHTPOINT = rpLIGHTPOSITIONINGSTART,
    rpLIGHTSPOT,
    rpLIGHTSPOTSOFT,
    rpLIGHTTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RpLightFlag
enum RpLightFlag
{
    rpLIGHTLIGHTATOMICS = 0x01,
    rpLIGHTLIGHTWORLD = 0x02,
    rpLIGHTFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// rpLightPrivateFlag
enum rpLightPrivateFlag
{
    rpLIGHTPRIVATENOCHROMA = 0x01,
    rpLIGHTPRIVATEFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RpInterpolatorFlag
enum RpInterpolatorFlag
{
    rpINTERPOLATORDIRTYINSTANCE = 0x01,
    rpINTERPOLATORDIRTYSPHERE = 0x02,
    rpINTERPOLATORNOFRAMEDIRTY = 0x04,
    rpINTERPOLATORFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RpAtomicFlag
enum RpAtomicFlag
{
    rpATOMICCOLLISIONTEST = 0x01,
    rpATOMICRENDER = 0x04,
    rpATOMICFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RpAtomicSetGeomFlag
enum RpAtomicSetGeomFlag
{
    rpATOMICSAMEBOUNDINGSPHERE = 0x01,
    rpATOMICSETGEOMFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RpAtomicPrivateFlag
enum RpAtomicPrivateFlag
{
    rpATOMICPRIVATEWORLDBOUNDDIRTY = 0x01,
    rpATOMICPRIVATEFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RpHAnimHierarchyFlag
enum RpHAnimHierarchyFlag
{
    rpHANIMHIERARCHYSUBHIERARCHY =              0x01,
    rpHANIMHIERARCHYNOMATRICES =                0x02,
    rpHANIMHIERARCHYUPDATEMODELLINGMATRICES = 0x1000,
    rpHANIMHIERARCHYUPDATELTMS =              0x2000,
    rpHANIMHIERARCHYLOCALSPACEMATRICES =      0x4000,
    rpHANIMHIERARCHYFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RpMatFXD3D9Pipeline
enum RpMatFXD3D9Pipeline
{
    rpNAMATFXD3D9PIPELINE          = 0,
    rpMATFXD3D9ATOMICPIPELINE      = 1,
    rpMATFXD3D9WORLDSECTORPIPELINE = 2,
    rpMATFXD3D9PIPELINEMAX,
    rpMATFXD3D9PIPELINEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RpMatFXMaterialFlags
enum RpMatFXMaterialFlags
{
    rpMATFXEFFECTNULL            = 0,
    rpMATFXEFFECTBUMPMAP         = 1,
    rpMATFXEFFECTENVMAP          = 2,
    rpMATFXEFFECTBUMPENVMAP      = 3,
    rpMATFXEFFECTDUAL            = 4,
    rpMATFXEFFECTUVTRANSFORM     = 5,
    rpMATFXEFFECTDUALUVTRANSFORM = 6,
    rpMATFXEFFECTMAX,
    rpMATFXNUMEFFECTS       = rpMATFXEFFECTMAX - 1,
    rpMATFXFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RpWorldPrivateFlag
enum RpWorldPrivateFlag
{
    rpWORLDSINGLEMALLOC = 0x01,
    rpWORLDPRIVATEFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RpWorldFlag
enum RpWorldFlag
{
    rpWORLDTRISTRIP  = 0x01,
    rpWORLDPOSITIONS = 0x02,
    rpWORLDTEXTURED  = 0x04,
    rpWORLDPRELIT    = 0x08,
    rpWORLDNORMALS   = 0x10,
    rpWORLDLIGHT     = 0x20,
    rpWORLDMODULATEMATERIALCOLOR = 0x40, 
    rpWORLDTEXTURED2 = 0x80,
    rpWORLDNATIVE               = 0x01000000,
    rpWORLDNATIVEINSTANCE       = 0x02000000,
    rpWORLDFLAGSMASK            = 0x000000FF,
    rpWORLDNATIVEFLAGSMASK      = 0x0F000000,
    rpWORLDSECTORSOVERLAP       = 0x40000000,
    rpWORLDFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RpWorldRenderOrder
enum RpWorldRenderOrder
{
    rpWORLDRENDERNARENDERORDER = 0,
    rpWORLDRENDERFRONT2BACK,
    rpWORLDRENDERBACK2FRONT,
    rpWORLDRENDERORDERFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RpD3D9GeometryUsageFlag
enum RpD3D9GeometryUsageFlag
{
    rpD3D9GEOMETRYUSAGE_DYNAMICPOSITIONS  =     0x02,
    rpD3D9GEOMETRYUSAGE_DYNAMICNORMALS    =     0x04,
    rpD3D9GEOMETRYUSAGE_DYNAMICPRELIT     =     0x08,
    rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDS  =     0x10,
    rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDS1 =     0x10,
    rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDS2 =     0x20,
    rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDS3 =     0x40,
    rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDS4 =     0x80,
    rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDS5 =   0x0100,
    rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDS6 =   0x0200,
    rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDS7 =   0x0400,
    rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDS8 =   0x0800,
    rpD3D9GEOMETRYUSAGE_DYNAMICTEXCOORDSALL = 0x0ff0,
    rpD3D9GEOMETRYUSAGE_DYNAMICMASK       =   0x0fff,
    rpD3D9GEOMETRYUSAGE_CREATETANGENTS    =  0x10000,
    rpD3D9GEOMETRYUSAGEFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
// RpD3D9WorldSectorUsageFlag
enum RpD3D9WorldSectorUsageFlag
{
    rpD3D9WORLDSECTORUSAGE_CREATETANGENTS    =  0x10000,
    rpD3D9WORLDSECTORUSAGEFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
#endif

/******************************************Funcs****************************************/
#ifndef _IDA_EXPORT
typedef int(*rwCustomStreamFnClose) (void *data);
typedef unsigned int(*rwCustomStreamFnRead) (void *data, void *buffer, unsigned int length);
typedef int(*rwCustomStreamFnWrite) (void *data, const void *buffer, unsigned int length);
typedef int(*rwCustomStreamFnSkip) (void *data, unsigned int offset);
typedef int (*vecSprintfFunc)(char *buffer, const char *format, ...); 
typedef int (*vecVsprintfFunc)(char *buffer, const char *format, va_list argptr); 
typedef char *(*vecStrcpyFunc)(char *dest, const char *srce);
typedef char *(*vecStrncpyFunc)(char *dest, const char *srce, size_t size);
typedef char *(*vecStrcatFunc)(char *dest, const char *srce);
typedef char *(*vecStrncatFunc)(char *dest, const char *srce, size_t size);
typedef char *(*vecStrrchrFunc)(const char *string, int findThis);
typedef char *(*vecStrchrFunc)(const char *string, int findThis);
typedef char *(*vecStrstrFunc)(const char *string, const char *findThis);
typedef int (*vecStrcmpFunc)(const char *string1, const char *string2);
typedef int (*vecStrncmpFunc)(const char *string1, const char *string2, size_t max_size);
typedef int (*vecStricmpFunc)(const char *string1, const char *string2);
typedef size_t (*vecStrlenFunc)(const char *string);
typedef char *(*vecStruprFunc)(char *string);
typedef char *(*vecStrlwrFunc)(char *string);
typedef char *(*vecStrtokFunc)(char *string, const char *delimit);
typedef int (*vecSscanfFunc)(const char *buffer, const char *format, ...);
typedef RwD3D9Vertex    RwIm2DVertex;
typedef unsigned short  RxVertexIndex;
typedef RxVertexIndex   RwImVertexIndex;
typedef int(*RwSystemFunc)(int nOption, void *pOut, void *pInOut, int nIn);
typedef int(*RwRenderStateSetFunction)(RwRenderState nState,void *pParam);
typedef int(*RwRenderStateGetFunction)(RwRenderState nState,void *pParam);
typedef int(*RwIm2DRenderLineFunction)(RwIm2DVertex *vertices, int numVertices, int vert1, int vert2);
typedef int(*RwIm2DRenderTriangleFunction)(RwIm2DVertex *vertices, int numVertices, int vert1, int vert2, int vert3);
typedef int(*RwIm2DRenderPrimitiveFunction)(RwPrimitiveType primType, RwIm2DVertex *vertices, int numVertices);
typedef int(*RwIm2DRenderIndexedPrimitiveFunction)(RwPrimitiveType primType, RwIm2DVertex *vertices, int numVertices, RwImVertexIndex *indices, int numIndices);
typedef int(*RwIm3DRenderLineFunction)(int vert1, int vert2);
typedef int(*RwIm3DRenderTriangleFunction)(int vert1, int vert2, int vert3);
typedef int(*RwIm3DRenderPrimitiveFunction)(RwPrimitiveType primType);
typedef int(*RwIm3DRenderIndexedPrimitiveFunction)(RwPrimitiveType primtype, RwImVertexIndex *indices, int numIndices);
typedef int(*RwStandardFunc)(void *pOut, void *pInOut, int nI);
typedef void       *(*RwMemoryAllocFn)    (RwFreeList * fl, unsigned int hint);
typedef RwFreeList *(*RwMemoryFreeFn)     (RwFreeList * fl, void *pData);
typedef void        (*RwResEntryDestroyNotify) (RwResEntry * resEntry);
typedef RxObjSpace3DVertex RxObjSpace3DLitVertex;
typedef RxObjSpace3DLitVertex RwIm3DVertex;
typedef RwIm2DVertex RxScrSpace2DVertex;
typedef RwObjectHasFrame * (*RwObjectHasFrameSyncFunction)(RwObjectHasFrame *object);
typedef RwCamera   *(*RwCameraBeginUpdateFunc) (RwCamera * camera);
typedef RwCamera   *(*RwCameraEndUpdateFunc) (RwCamera * camera);
typedef RpClump    *(*RpClumpCallBack) (RpClump * clump, void *data);
typedef unsigned int (*VisibilityCallBack)(RpClump * clump);
typedef RpAtomic   *(*RpAtomicCallBackRender) (RpAtomic * atomic);
typedef void (*RtAnimKeyFrameApplyCallBack) (void *result, void *voidIFrame);
typedef void (*RtAnimKeyFrameBlendCallBack) (void *voidOut, void *voidIn1, void *voidIn2, float alpha);
typedef void (*RtAnimKeyFrameInterpolateCallBack) (void *voidOut, void *voidIn1, void *voidIn2, float time, void *customData);
typedef void (*RtAnimKeyFrameAddCallBack) (void *voidOut, void *voidIn1, void *voidIn2);
typedef void (*RtAnimKeyFrameMulRecipCallBack) (void *voidFrame, void *voidStart);
typedef RtAnimAnimation * (*RtAnimKeyFrameStreamReadCallBack) (RwStream *stream, RtAnimAnimation *animation);
typedef int (*RtAnimKeyFrameStreamWriteCallBack) (const RtAnimAnimation *animation, RwStream *stream);
typedef int (*RtAnimKeyFrameStreamGetSizeCallBack) (const RtAnimAnimation *animation);
typedef RtAnimInterpolator * (*RtAnimCallBack) (RtAnimInterpolator *animInstance, void *data);
typedef RpWorldSector *(*RpWorldSectorCallBackRender) (RpWorldSector *worldSector);
#endif

/******************************************Structures****************************************/
// RwV2d
struct RwV2d
{
    float x;
    float y;
};
// RwV3d
struct RwV3d
{
    float x;
    float y;
    float z;
};
// RwRect
struct RwRect
{
    float x;
    float y;
    float w;
    float h;
};
// RwSphere
struct RwSphere
{
    RwV3d center;
    float radius;
};
// RwLine
struct RwLine
{
    RwV3d start;
    RwV3d end;
};
// RwTexCoords
struct RwTexCoords
{
    float u;
    float v;
};
// RwSLLink
struct RwSLLink
{
    RwSLLink  *next;
};
// RwSingleList
struct RwSingleList
{
    RwSLLink link;
};
// RwLLLink
struct RwLLLink
{
    RwLLLink *next;
    RwLLLink *prev;
};
// RwLinkList
struct RwLinkList
{
    RwLLLink link;
};
// RwSurfaceProperties
struct RwSurfaceProperties
{
    float ambient;
	// GTA Flags
	union
	{
		float specular;
		unsigned int m_dwFlags; /* HAS_ENV_MAP   = 0x00000001,
							       HAS_ENV_MAP_X = 0x00000002,
							       HAS_SPECULAR  = 0x00000004 */
	};
    float diffuse;
};
// RwPlane
struct RwPlane
{
    RwV3d normal;
    float distance;
};
// RwObject
struct RwObject
{
    unsigned char  type;
    unsigned char  subType;
    unsigned char  flags;
    unsigned char  privateFlags;
	RwFrame       *parent;
};
// RwMemoryFunctions
struct RwMemoryFunctions
{
    void *(*rwmalloc)(size_t size, unsigned int hint);
    void  (*rwfree)(void *mem);
    void *(*rwrealloc)(void *mem, size_t newSize, unsigned int hint);
    void *(*rwcalloc)(size_t numObj, size_t sizeObj, unsigned int hint);
};
// RwFreeList
struct RwFreeList
{
    unsigned int   entrySize;
    unsigned int   entriesPerBlock;
    unsigned int   heapSize;
    unsigned int   alignment;
    RwLinkList     blockList;
    unsigned int   flags;
    RwLLLink       link;
};
// RwStreamMemory
struct RwStreamMemory
{
    unsigned int   position; 
    unsigned int   nSize;
    unsigned char *memBlock;
};
// RwStreamFile
union RwStreamFile
{
    void    *fpFile;
    const void    *constfpFile;
};
// RwStreamCustom
struct RwStreamCustom
{
#ifndef _IDA_EXPORT
    rwCustomStreamFnClose sfnclose;
    rwCustomStreamFnRead sfnread;
    rwCustomStreamFnWrite sfnwrite;
    rwCustomStreamFnSkip sfnskip;
#else
	int   sfnclose;
    int  sfnread;
    int   sfnwrite;
    int  sfnskip;
#endif
    void               *data;
};
// RwStreamUnion
union RwStreamUnion
{
    RwStreamMemory      memory;
    RwStreamFile        file;
    RwStreamCustom      custom;
};
// RwStream
struct RwStream
{
#ifndef _IDA_EXPORT
    RwStreamType        type;
    RwStreamAccessType  accessType;
#else
	int                 type;
	int                 accessType;
#endif
    int                 position;
    RwStreamUnion       Type;
    int                 rwOwned;
};
// RwMemory
struct RwMemory
{
    unsigned char *start;
    unsigned int   length;
};
// RwStringFunctions
struct RwStringFunctions
{
#ifndef _IDA_EXPORT
    vecSprintfFunc vecSprintf ;
    vecVsprintfFunc vecVsprintf;
    vecStrcpyFunc vecStrcpy;
    vecStrncpyFunc vecStrncpy;
    vecStrcatFunc vecStrcat;
    vecStrncatFunc vecStrncat;
    vecStrrchrFunc vecStrrchr;
    vecStrchrFunc vecStrchr;
    vecStrstrFunc vecStrstr;
    vecStrcmpFunc vecStrcmp;
    vecStrncmpFunc vecStrncmp;
    vecStricmpFunc vecStricmp;
    vecStrlenFunc vecStrlen;
    vecStruprFunc vecStrupr;
    vecStrlwrFunc vecStrlwr;
    vecStrtokFunc vecStrtok;
    vecSscanfFunc vecSscanf;
#else
	int   vecSprintf ;
    int   vecVsprintf;
    int  vecStrcpy;
    int   vecStrncpy;
    int  vecStrcat;
    int   vecStrncat;
    int   vecStrrchr;
    int  vecStrchr;
    int  vecStrstr;
    int  vecStrcmp;
    int   vecStrncmp;
    int   vecStricmp;
    int  vecStrlen;
    int  vecStrupr;
    int  vecStrlwr;
    int  vecStrtok;
    int  vecSscanf;
#endif
};
// RwMatrix
struct RwMatrix
{
    RwV3d        right;
    unsigned int flags;
    RwV3d        up;
    unsigned int pad1;
    RwV3d        at;
    unsigned int pad2;
    RwV3d        pos;
    unsigned int pad3;
};
// RwD3D9Vertex
struct RwD3D9Vertex
{
    float        x;
    float        y;
    float        z;
    float        rhw;
    unsigned int emissiveColor;
    float        u;
    float        v;
};
// RwDevice
struct RwDevice
{
    float                                gammaCorrection;
#ifndef _IDA_EXPORT
    RwSystemFunc                         fpSystem;
#else
	int                                  fpSystem;
#endif
    float                                zBufferNear;
    float                                zBufferFar;
#ifndef _IDA_EXPORT
    RwRenderStateSetFunction             fpRenderStateSet;
    RwRenderStateGetFunction             fpRenderStateGet;
    RwIm2DRenderLineFunction             fpIm2DRenderLine;
    RwIm2DRenderTriangleFunction         fpIm2DRenderTriangle;
    RwIm2DRenderPrimitiveFunction        fpIm2DRenderPrimitive;
    RwIm2DRenderIndexedPrimitiveFunction fpIm2DRenderIndexedPrimitive;
    RwIm3DRenderLineFunction             fpIm3DRenderLine;
    RwIm3DRenderTriangleFunction         fpIm3DRenderTriangle;
    RwIm3DRenderPrimitiveFunction        fpIm3DRenderPrimitive;
    RwIm3DRenderIndexedPrimitiveFunction fpIm3DRenderIndexedPrimitive;
#else
	int                                  fpRenderStateSet;
    int                                  fpRenderStateGet;
    int                                  fpIm2DRenderLine;
    int                                  fpIm2DRenderTriangle;
    int                                  fpIm2DRenderPrimitive;
    int                                  fpIm2DRenderIndexedPrimitive;
    int                                  fpIm3DRenderLine;
    int                                  fpIm3DRenderTriangle;
    int                                  fpIm3DRenderPrimitive;
    int                                  fpIm3DRenderIndexedPrimitive;
#endif
};
// RwGlobals
struct RwGlobals
{
	void               *curCamera;
	void               *curWorld;
	unsigned short      renderFrame;
	unsigned short      lightFrame;
	unsigned short      pad[2];
	RwDevice            dOpenDevice;
#ifndef _IDA_EXPORT
	RwStandardFunc      stdFunc[29];
#else
	int                 stdFunc[29];
#endif
	RwLinkList          dirtyFrameList;
	void               *fileFuncs[0xB];
	RwStringFunctions   stringFuncs;
	RwMemoryFunctions   memoryFuncs;
#ifndef _IDA_EXPORT
	RwMemoryAllocFn     memoryAlloc;
	RwMemoryFreeFn      memoryFree;
#else
	int                 memoryAlloc;
	int                 memoryFree;
#endif
	RwMetrics          *metrics;
#ifndef _IDA_EXPORT
	RwEngineStatus      engineStatus;
#else
	int                 engineStatus;
#endif
	unsigned int        resArenaInitSize;
};
// RwMetrics
struct RwMetrics
{
    unsigned int  numTriangles;
    unsigned int  numProcTriangles;
    unsigned int  numVertices;
    unsigned int  numTextureUploads;
    unsigned int  sizeTextureUploads;
    unsigned int  numResourceAllocs;
    void         *devSpecificMetrics;
};
// RxD3D9VertexStream
struct RxD3D9VertexStream
{
    void           *vertexBuffer;
    unsigned int    offset;
    unsigned int    stride;
    unsigned short  geometryFlags;
    unsigned char   managed;
    unsigned char   dynamicLock;
};
// RxD3D9ResEntryHeader
struct RxD3D9ResEntryHeader
{
    unsigned int        serialNumber;
    unsigned int        numMeshes;
    void               *indexBuffer;
    unsigned int        primType;
    RxD3D9VertexStream  vertexStream[2];
    int                 useOffsets;
    void               *vertexDeclaration;
    unsigned int        totalNumIndex;
    unsigned int        totalNumVertex;
};
// RxD3D9InstanceData
struct RxD3D9InstanceData
{
    unsigned int  numIndex;
    unsigned int  minVert;
    RpMaterial   *material;
    int           vertexAlpha;
    void         *vertexShader;
    unsigned int  baseIndex;
    unsigned int  numVertices;
    unsigned int  startIndex;
    unsigned int  numPrimitives;
};
// RwResEntry
struct RwResEntry
{
    RwLLLink            link;
    int                 size;
    void               *owner;
    RwResEntry        **ownerRef;
#ifndef _IDA_EXPORT
    RwResEntryDestroyNotify destroyNotify;
#else
	int destroyNotify;
#endif
	RxD3D9ResEntryHeader header;
	RxD3D9InstanceData   meshData;
};
// RwRGBAReal
struct RwRGBAReal
{
    float red;
    float green;
    float blue;
    float alpha;
};
// RwRGBA
struct RwRGBA
{
    unsigned char red; 
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
};
// RwChunkHeaderInfo
struct RwChunkHeaderInfo
{
    unsigned int type;
    unsigned int length;
    unsigned int version; 
    unsigned int buildNum;
    int isComplex;
};
// RxPipeline
struct RxPipeline
{
    int                   locked;
    unsigned int           numNodes;
    void                  *nodes;
    unsigned int           packetNumClusterSlots;
#ifndef _IDA_EXPORT
    rxEmbeddedPacketState  embeddedPacketState;
#else
	int                    embeddedPacketState;
#endif
    void                  *embeddedPacket;
    unsigned int           numInputRequirements;
    void                  *inputRequirements;
    void                  *superBlock;
    unsigned int           superBlockSize;
    unsigned int           entryPoint;
    unsigned int           pluginId;
    unsigned int           pluginData;
};
// RwRaster
struct RwRaster
{
    RwRaster      *parent;
    unsigned char *cpPixels;
    unsigned char *palette;
    int            width, height, depth;
    int            stride;
    short          nOffsetX, nOffsetY;
    unsigned char  cType;
    unsigned char  cFlags;
    unsigned char  privateFlags;
    unsigned char  cFormat;
    unsigned char *originalPixels;
    int            originalWidth;
    int            originalHeight;
    int            originalStride;
#ifndef _IDA_EXPORT
#ifdef _D3D_INCLUDE
    // RenderWare plugin
    struct RwD3DRaster
    {
        union
        {
            IDirect3DTexture9 *texture;
			IDirect3DCubeTexture9 *cubeTexture;
            IDirect3DSurface9 *surface;
        };
        unsigned char       *palette;
        unsigned char        hasAlpha;
        unsigned char        isCubeMap : 4;
		unsigned char        selectedCubeFace : 4;
        unsigned char        autoMipMapping : 4;
		unsigned char        hasCompression : 4;
        unsigned char        lockFlags;
        IDirect3DSurface9   *lockedSurface;
        D3DLOCKED_RECT       lockedRect;
        D3DFORMAT            format;
        IDirect3DSwapChain9 *swapChain;
        HWND                *hwnd;
    } d3dRaster;
#endif
#endif
};
// RxObjSpace3DVertex
struct RxObjSpace3DVertex
{
    RwV3d        objVertex;
    RwV3d        objNormal;
    unsigned int color;
    float        u;
    float        v;
};
// RwImage
struct RwImage
{
    int            flags;
    int            width;
    int            height;
    int            depth;
    int            stride;
    unsigned char *cpPixels;
    RwRGBA        *palette;
};
// RwTexDictionary
struct RwTexDictionary
{
    RwObject         object;
    RwLinkList       texturesInDict;
    RwLLLink         lInInstance;
    // GTA plugin
	RwTexDictionary *parent;
};
// RwTexture
struct RwTexture
{
    RwRaster        *raster;
    RwTexDictionary *dict;
    RwLLLink        lInDictionary;
    char            name[32];
    char            mask[32];
	union{
		unsigned int    filterAddressing;
		struct{
			unsigned int filter : 8;
			unsigned int uAddressing : 4;
			unsigned int vAddressing : 4;
		};
	};
    int             refCount;
    // Renderware plugin
    unsigned char   maxAnisotropy;
    char            pad[3];
};
// RpHAnimFrameExtension
struct RpHAnimFrameExtension
{
    int               id;
    void             *hierarchy;
};
// RwFrame
struct RwFrame
{
    RwObject              object;
    RwLLLink              inDirtyListLink;
    RwMatrix              modelling;
    RwMatrix              ltm;
    RwLinkList            objectList;
    RwFrame              *child;
    RwFrame              *next;
    RwFrame              *root;
    // RenderWare plugins
    RpHAnimFrameExtension hAnimFrame;
    // GTA Plugins
    char                  nodeName[24];
    union
    {
        void             *model;
        int               hierId;
    };
};
// RwBBox
struct RwBBox
{
    RwV3d sup;
    RwV3d inf;
};
// RwFrustumPlane
struct RwFrustumPlane
{
    RwPlane       plane;
    unsigned char closestX;
    unsigned char closestY;
    unsigned char closestZ;
    unsigned char pad;
};
// RwObjectHasFrame
struct RwObjectHasFrame
{
    RwObject                     object;
    RwLLLink                     lFrame;
#ifndef _IDA_EXPORT
    RwObjectHasFrameSyncFunction sync;
#else
	int                          sync;
#endif
};
// RwCamera
struct RwCamera
{
    RwObjectHasFrame         object;
#ifndef _IDA_EXPORT
    RwCameraProjection       projectionType;
    RwCameraBeginUpdateFunc  beginUpdate;
    RwCameraEndUpdateFunc    endUpdate;
#else
	int                      projectionType;
    int                      beginUpdate;
    int                      endUpdate;
#endif
    RwMatrix                 viewMatrix;
    RwRaster                *frameBuffer;
    RwRaster                *zBuffer;
    RwV2d                    viewWindow;
    RwV2d                    recipViewWindow;
    RwV2d                    viewOffset;
    float                    nearPlane;
    float                    farPlane;
    float                    fogPlane;
    float                    zScale, zShift;
    RwFrustumPlane           frustumPlanes[6];
    RwBBox                   frustumBoundBox;
    RwV3d                    frustumCorners[8];
};
// RpUVAnimMaterial
struct RpUVAnimMaterial
{
    RwMatrix           *pMatrix[2];
    RtAnimInterpolator *pInterpolator[8];
};
// RpMaterial
struct RpMaterial
{
    RwTexture           *texture;
	union
	{
		RwRGBA               color;
		unsigned int         colorHex;
	};
    RxPipeline          *pipeline;
    RwSurfaceProperties  surfaceProps;
    short                refCount;
    short                pad;
    // RenderWare plugins
	RpMatFx             *matFx;
    RpUVAnimMaterial     UVAnim;
#ifndef _IDA_EXPORT
    // GTA plugins
    CEnvMapMaterial     *m_pReflection;
    CSpecularMaterial   *m_pSpecular;
#endif
};
// EnvMapFx
struct EnvMapFx
{
    RwFrame      *frame;
    RwTexture    *texture;
    float         coefficient;
    unsigned int  frameBufferAlpha;
    float         size;
    unsigned int  effectType;
};
// RpMatFx
struct RpMatFx
{
    EnvMapFx     effect[2];
    unsigned int flags;
};
// RpMaterialList
struct RpMaterialList
{
    RpMaterial **materials;
    int          numMaterials;
    int          space;
};
// RpMesh
struct RpMesh
{
#ifndef _IDA_EXPORT
    RxVertexIndex *indices;
#else
	short         *indices;
#endif
    unsigned int   numIndices;
    RpMaterial    *material;
};
// RpMeshHeader
struct RpMeshHeader
{
    unsigned int   flags;
    unsigned short numMeshes;
    unsigned short serialNum;
    unsigned int   totalIndicesInMesh;
    unsigned int   firstMeshOffset;
};
// RpMorphTarget
struct RpMorphTarget
{
    RpGeometry *parentGeom; 
    RwSphere    boundingSphere;
    RwV3d      *verts;
    RwV3d      *normals;
};
// RpTriangle
struct RpTriangle
{
    unsigned short vertIndex[3];
    unsigned short matIndex;
};
// CNightVertexColors
#ifndef _IDA_EXPORT
class CNightVertexColors
{
	RwRGBA *m_pNightColors;
	RwRGBA *m_pDayColors;
	float   m_fLightingState;
};
#endif
struct RsInputDevice
{
  int inputDeviceType;
  int used;
  int inputEventHandler;
};
struct RsGlobalType
{
  char *AppName;
  int MaximumWidth;
  int MaximumHeight;
  int frameLimit;
  int quit;
  int ps;
  RsInputDevice keyboard;
  RsInputDevice mouse;
  RsInputDevice pad;
};
// RpGeometry
struct RpGeometry
{
    RwObject            object;
    unsigned int        flags;
    unsigned short      lockedSinceLastInst;
    short               refCount;
    int                 numTriangles;
    int                 numVertices;
    int                 numMorphTargets;
    int                 numTexCoordSets;
    RpMaterialList      matList;
    RpTriangle         *triangles;
    RwRGBA             *preLitLum;
    RwTexCoords        *texCoords[8];
    RpMeshHeader       *mesh;
    RwResEntry         *repEntry;
    RpMorphTarget      *morphTarget;
    // RenderWare plugins
	unsigned int        usageFlags;
    RpSkin             *skin;
    // GTA plugins
#ifndef _IDA_EXPORT
    CNightVertexColors  nightVertexColors;
    CBreakableGeometry *breakableGeometry;
    C2dfxStore         *_2dfxStore;
#endif
};
// RwMatrixWeights
struct RwMatrixWeights
{
    float w0;
    float w1;
    float w2;
    float w3;
};
// RpSkin
struct RpSkin
{
    unsigned int     numBones;
    unsigned int     numBoneIds;
    unsigned char   *boneIds;
    RwMatrix        *skinToBoneMatrices;
    unsigned int     maxNumWeightsForVertex;
    unsigned int    *vertexBoneIndices;
    RwMatrixWeights *vertexBoneWeights;
    char             field_1C[8];
    unsigned int     boneLimit;
    unsigned int     numMeshes;
    unsigned int     numRLE;
    unsigned char   *meshBoneRemapIndices;
    unsigned int     meshBoneRLECount;
    void            *meshBoneRLE;
    void            *field_3C;
};
#ifndef _IDA_EXPORT
// CBreakableGeometry
class CBreakableGeometry
{
public:
    unsigned int         m_uiPosRule;
    unsigned short       m_usNumVertices;
    char                 _pad0[2];
    RwV3d               *m_pVertexPos;
    RwTexCoords         *m_pTexCoors;
    RwRGBA              *m_pVertexColors;
    unsigned short       m_usNumTriangles;
    char                 _pad1[2];
    RpTriangle          *m_pTriangles;
    unsigned short      *m_pMaterialAssignments;
    unsigned short       m_usNumMaterials;
    char                 _pad2[2];
    RwTexture          **m_pTextures;
    char                *m_pTextureNames;
    char                *m_pMaskNames;
    RwSurfaceProperties *m_pMaterialProperties;
	/* some data here with random size
	RwV3d                m_avVertexPos[m_usNumVertices];
	RwTexCoords          m_asTexCoords[m_usNumVertices];
	RwRGBA               m_asVertexColors[m_usNumVertices];
	RpTriangle           m_asTriangles[m_usNumTriangles];
	unsigned short       m_ausMaterialAssignments[m_usNumTriangles];
	char                 m_acTextureNames[m_usNumMaterials][32];
	char                 m_acMaskNames[m_usNumMaterials][32];
	RwSurfaceProperties  m_asMaterialProperties[m_usNumMaterials];
	*/
};
// C2dfx
class C2dfxStore
{
public:
	unsigned int m_uiNum2dfx;
	/* Data with random size here
	C2dfx        m_as2dfx[m_uiNumEffects];
	*/
};
// CEnvMapMaterial
class CEnvMapMaterial
{
public:
    unsigned char   m_ucTransformParams[4];
    unsigned char   m_ucIntensity;
    char            _pad;
    unsigned short  m_usRenderFrame;
    RwTexture      *m_pTexture;
};
// CSpecularMaterial
class CSpecularMaterial
{
public:
	float      m_fLevel;
	RwTexture *m_pTexture;
};
#endif
// RpLight
struct RpLight
{
    RwObjectHasFrame object;
    float            radius;
    RwRGBAReal       color;
    float            minusCosAngle;
    RwLinkList       WorldSectorsInLight;
    RwLLLink         inWorld;
    unsigned short   lightFrame;
    unsigned short   pad;
};
// RpD3D9AttenuationParams
struct RpD3D9AttenuationParams
{
    float constant;
    float linear;
    float quadratic;
};
// RpInterpolator
struct RpInterpolator
{
    int   flags;
    short startMorphTarget;
    short endMorphTarget;
    float time;
    float recipTime;
    float position;
};
#ifndef _IDA_EXPORT
// CClumpVisibility
class CClumpVisibility
{
	VisibilityCallBack m_pVisibilityCB;
	unsigned int       m_uiClumpAlpha;
};
#endif
// RpClump
struct RpClump
{
    RwObject          object;
    RwLinkList        atomicList;
    RwLinkList        lightList;
    RwLinkList        cameraList;
    RwLLLink          inWorldLink;
#ifndef _IDA_EXPORT
    RpClumpCallBack   callback;
#else
	int               callback;
#endif
    // RenderWare plugins
    char              worldData[8];
#ifndef _IDA_EXPORT
    // GTA plugins
    CClumpVisibility  visibility;
    CHAnim           *anim;
#endif
};

typedef struct RpClumpChunkInfo RpClumpChunkInfo;
typedef struct RpClumpChunkInfo _rpClump;

struct RpClumpChunkInfo
{
    int numAtomics;
    int numLights;
    int numCameras;
};

// CBone
#ifndef _IDA_EXPORT
class CBone
{
public:
    unsigned char m_ucFlags;
	RwV3d         m_vOffset;
	CHAnimIFrame *m_pIFrame;
	int           m_iNodeId;
};
#endif
// RtQuat
struct RtQuat
{
    RwV3d imag;
    float real;
};
#ifndef _IDA_EXPORT
// CHAnimIFrame
class CHAnimIFrame
{
public:
	RtQuat m_qRotation;
	RwV3d  m_vTranslation;
};
// CHAnim
class CHAnim
{
public:
	void         *field_0;
	int           field_4;
	unsigned int  m_uiNumBones;
	int           field_C;
	CBone        *m_pBones;
};
// CAtomicVisibility
class CAtomicVisibility
{
public:
	short          m_sModelId;
	unsigned short m_usFlags;
};
#endif
// RpAtomic
struct RpAtomic
{
    RwObjectHasFrame        object;
    RwResEntry             *repEntry;
    RpGeometry             *geometry;
    RwSphere                boundingSphere;
    RwSphere                worldBoundingSphere;
    RpClump                *clump;
    RwLLLink                inClumpLink;
#ifndef _IDA_EXPORT
    RpAtomicCallBackRender  renderCallBack;
#else
	int                     renderCallBack;
#endif
    RpInterpolator          interpolator;
    unsigned short          renderFrame;
    unsigned short          pad;
    RwLinkList              llWorldSectorsInAtomic;
    RxPipeline             *pipeline;
#ifndef _IDA_EXPORT
	// RenderWare plugins
	char                    worldData[8];
	RpHAnimHierarchy       *animHierarchy;
	// GTA plugins
	CAtomicVisibility       visibility;
	// RenderWare plugins
	unsigned int            matFxEnabled;
	// GTA plugins
	CXEnvMapInfo           *xEnvMapInfo;
	unsigned int            customPipeline;
#endif
};
#ifndef _IDA_EXPORT
// CXEnvMapInfo
class CXEnvMapInfo
{
public:
	float data[3];
};
#endif
// RtAnimInterpolatorInfo
struct RtAnimInterpolatorInfo
{
    int                                 typeID;
    int                                 interpKeyFrameSize;
    int                                 animKeyFrameSize;
#ifndef _IDA_EXPORT
    RtAnimKeyFrameApplyCallBack         keyFrameApplyCB;
    RtAnimKeyFrameBlendCallBack         keyFrameBlendCB;
    RtAnimKeyFrameInterpolateCallBack   keyFrameInterpolateCB;
    RtAnimKeyFrameAddCallBack           keyFrameAddCB;
    RtAnimKeyFrameMulRecipCallBack      keyFrameMulRecipCB;
    RtAnimKeyFrameStreamReadCallBack    keyFrameStreamReadCB;
    RtAnimKeyFrameStreamWriteCallBack   keyFrameStreamWriteCB;
    RtAnimKeyFrameStreamGetSizeCallBack keyFrameStreamGetSizeCB;
#else
	int                                 keyFrameApplyCB;
    int                                 keyFrameBlendCB;
    int                                 keyFrameInterpolateCB;
    int                                 keyFrameAddCB;
    int                                 keyFrameMulRecipCB;
    int                                 keyFrameStreamReadCB;
    int                                 keyFrameStreamWriteCB;
    int                                 keyFrameStreamGetSizeCB;
#endif
    int                                 customDataSize;
};
// RtAnimAnimation
struct RtAnimAnimation
{
    RtAnimInterpolatorInfo *interpInfo;
    int                     numFrames;
    int                     flags;
    float                   duration;
    void                   *pFrames;
    void                   *customData;
};

typedef struct RwPluginRegistry RwPluginRegistry;
typedef struct RwPluginRegEntry RwPluginRegEntry;

struct RwPluginRegistry
{
    unsigned int      sizeOfStruct;
    unsigned int      origSizeOfStruct;
    unsigned int      maxSizeOfStruct;
    unsigned int      staticAlloc;
    RwPluginRegEntry *firstRegEntry;
    RwPluginRegEntry *lastRegEntry;
};

typedef RwStream *(*RwPluginDataChunkWriteCallBack)(RwStream *stream, int binaryLength, const void *object, int offsetInObject, int sizeInObject);
typedef RwStream *(*RwPluginDataChunkReadCallBack)(RwStream *stream, int binaryLength, void *object, int offsetInObject, int sizeInObject);
typedef int(*RwPluginDataChunkGetSizeCallBack)(const void *object, int offsetInObject, int sizeInObject);
typedef bool(*RwPluginDataChunkAlwaysCallBack)(void *object, int offsetInObject, int sizeInObject);
typedef bool(*RwPluginDataChunkRightsCallBack)(void *object, int offsetInObject, int sizeInObject, int extraData);
typedef void *(*RwPluginObjectConstructor)(void *object, int offsetInObject, int sizeInObject);
typedef void *(*RwPluginObjectCopy)(void *dstObject, const void *srcObject, int offsetInObject, int sizeInObject);
typedef void *(*RwPluginObjectDestructor)(void *object, int offsetInObject, int sizeInObject);
typedef void *(*RwPluginErrorStrCallBack)(void *);

struct RwPluginRegEntry
{
    int             offset;
    int             size;
    unsigned int    pluginID;
    RwPluginDataChunkReadCallBack readCB;
    RwPluginDataChunkWriteCallBack writeCB;
    RwPluginDataChunkGetSizeCallBack getSizeCB;
    RwPluginDataChunkAlwaysCallBack alwaysCB;
    RwPluginDataChunkRightsCallBack rightsCB;
    RwPluginObjectConstructor constructCB;
    RwPluginObjectDestructor destructCB;
    RwPluginObjectCopy copyCB;
    RwPluginErrorStrCallBack errStrCB;
    RwPluginRegEntry *nextRegEntry;
    RwPluginRegEntry *prevRegEntry;
    RwPluginRegistry *parentRegistry;
};

// RtAnimKeyFrameHeader
struct RtAnimKeyFrameHeader
{
    void  *prevFrame;
    float  time;
};
// RtAnimInterpFrameHeader
struct RtAnimInterpFrameHeader
{
    RtAnimKeyFrameHeader *keyFrame1;
    RtAnimKeyFrameHeader *keyFrame2;
};
// RtAnimInterpolator
struct RtAnimInterpolator
{
    RtAnimAnimation                   *pCurrentAnim;
    float                              currentTime;
    void                              *pNextFrame;
#ifndef _IDA_EXPORT
    RtAnimCallBack                     pAnimCallBack;
#else
	int                                pAnimCallBack;
#endif
    void                              *pAnimCallBackData;
    float                              animCallBackTime;
#ifndef _IDA_EXPORT
    RtAnimCallBack                     pAnimLoopCallBack;
#else
	int                                pAnimLoopCallBack;
#endif
    void                              *pAnimLoopCallBackData;
    int                                maxInterpKeyFrameSize;
    int                                currentInterpKeyFrameSize;
    int                                currentAnimKeyFrameSize;
    int                                numNodes;
    int                                isSubInterpolator;
    int                                offsetInParent;
    RtAnimInterpolator                *parentAnimation;
#ifndef _IDA_EXPORT
    RtAnimKeyFrameApplyCallBack        keyFrameApplyCB;
    RtAnimKeyFrameBlendCallBack        keyFrameBlendCB;
    RtAnimKeyFrameInterpolateCallBack  keyFrameInterpolateCB;
    RtAnimKeyFrameAddCallBack          keyFrameAddCB;
#else
	int                                keyFrameApplyCB;
    int                                keyFrameBlendCB;
    int                                keyFrameInterpolateCB;
    int                                keyFrameAddCB;
#endif
};
// RpHAnimKeyFrame
struct RpHAnimKeyFrame
{
    RpHAnimKeyFrame *prevFrame;
    float            time;
    RtQuat           q;
    RwV3d            t;
};
// RpHAnimInterpFrame
struct RpHAnimInterpFrame
{
    RpHAnimKeyFrame *keyFrame1;
    RpHAnimKeyFrame *keyFrame2;
    RtQuat           q;
    RwV3d            t;
};
// RpHAnimNodeInfo
struct RpHAnimNodeInfo
{
    int      nodeID;
    int      nodeIndex;
    int      flags;
    RwFrame *pFrame;
};
// RpHAnimHierarchy
struct RpHAnimHierarchy
{
    int                 flags;
    int                 numNodes;
    RwMatrix           *pMatrixArray;
    void               *pMatrixArrayUnaligned;
    RpHAnimNodeInfo    *pNodeInfo;
    RwFrame            *parentFrame;
    RpHAnimHierarchy   *parentHierarchy;
    int                 rootParentOffset;
    RtAnimInterpolator *currentAnim;
};
// RpVertexNormal
struct RpVertexNormal
{
    char          x;
    char          y;
    char          z;
    unsigned char pad;
};
// RpWorldSector
struct RpWorldSector
{
    int                 type;
    RpTriangle         *triangles;
    RwV3d              *vertices;
    RpVertexNormal     *normals;
    RwTexCoords        *texCoords[8];
    RwRGBA             *preLitLum;
    RwResEntry         *repEntry;
    RwLinkList          collAtomicsInWorldSector;
    RwLinkList          lightsInWorldSector;
    RwBBox              boundingBox;
    RwBBox              tightBoundingBox;
    RpMeshHeader       *mesh;
    RxPipeline         *pipeline;
    unsigned short      matListWindowBase;
    unsigned short      numVertices;
    unsigned short      numTriangles;
    unsigned short      pad;
};
// RpWorld
struct RpWorld
{
    RwObject                     object;
    unsigned int                 flags;
#ifndef _IDA_EXPORT
    RpWorldRenderOrder           renderOrder;
#else
	int                          renderOrder;
#endif
    RpMaterialList               matList;
    RpSector                    *rootSector;
    int                          numTexCoordSets;
    int                          numClumpsInWorld;
    RwLLLink                    *currentClumpLink;
    RwLinkList                   clumpList;
    RwLinkList                   lightList;
    RwLinkList                   directionalLightList;
    RwV3d                        worldOrigin;
    RwBBox                       boundingBox;
#ifndef _IDA_EXPORT
    RpWorldSectorCallBackRender  renderCallBack;
#else
	int                          renderCallBack;
#endif
    RxPipeline                  *pipeline;
};
// RpSector
struct RpSector
{
    int type;
};

struct RpGeometryChunkInfo
{
    int format;  /* Compression flags and number of texture coord sets */
    int numTriangles;
    int numVertices;
    int numMorphTargets;
};

/******************************************Functions****************************************
deleted.

 *******************************************************************************************/
// VECTOR
#define RwV3dTransformPoint(pointOut, pointIn, transformMatrix) ((void (__cdecl *)(RwV3d *, RwV3d *, RwMatrixTag *))0x7EDD60)(pointOut, pointIn, transformMatrix)

// CLUMP
#define RpClumpRender(clump) ((float (__cdecl *)(RpClump *))0x749B20)(clump)
#define RpClumpForAllAtomics(clump, callback, data) ((void (__cdecl *)(RpClump *, void *, void *))0x749B70)(clump, callback, data)

// TEXTURE
#define RwTextureRegisterPlugin(size, pluginId, ctor, dtor, copy) ((int (__cdecl *)(int, int, void *, void *, void *))0x7F3BB0)(size, pluginId, ctor, dtor, copy)
#define RpAnisotPluginAttach() ((signed int (__cdecl *)())0x748F70)()
#define RwStreamFindChunk(stream, type, lengthOut, versionOut) ((unsigned int (__cdecl *)(RwStream *, unsigned int, unsigned int *, unsigned int *))0x7ED2D0)(stream, type, lengthOut, versionOut)
#define RwTexDictionaryCreate() ((RwTexDictionary *(__cdecl *)())0x7F3600)()
#define readTextureNative(stream) ((RwTexture *(__cdecl *)(RwStream *))0x730E60)(stream)
#define RwTexDictionaryAddTexture(dict, texture) ((RwTexture *(__cdecl *)(RwTexDictionary *, RwTexture *))0x7F3980)(dict, texture)
#define RwTexDictionaryForAllTextures(dict, callback, data) ((RwTexDictionary *(__cdecl *)(RwTexDictionary *, void *, void *))0x7F3730)(dict, callback, data)
#define RwTexDictionaryDestroy(dict) ((int (__cdecl *)(RwTexDictionary *))0x7F36A0)(dict)
#define RwTexDictionaryFindNamedTexture(dict, name) ((RwTexture *(__cdecl *)(RwTexDictionary *, char *))0x7F39F0)(dict, name)
#define RwTextureSetName(texture, name) ((void (__cdecl *)(RwTexture *, char *))0x7F38A0)(texture, name)
#define RwTextureSetMaskName(texture, name) ((void (__cdecl *)(RwTexture *, char *))0x7F3910)(texture, name)

// STREAM
#define RwStreamReadReal(stream, buffer, length) ((RwStream *(__cdecl *)(RwStream *, void *, unsigned int))0x7ED4F0)(stream, buffer, length)
#define RwStreamRead(stream, space, size) ((unsigned int (__cdecl *)(RwStream *, void *, unsigned int))0x7EC9D0)(stream, space, size)
// GEOMETRY
#define RpGeometryCreate(numVerts, numTriangles, format) ((RpGeometry *(__cdecl *)(int, int, unsigned int))0x74CA90)(numVerts, numTriangles, format)

#define RwCameraSetFarClipPlane(camera, farClip) ((RwCamera *(__cdecl *)(RwCamera *, float))0x7EE2A0)(camera, farClip)
#define RwCameraSetNearClipPlane(camera, nearClip) ((RwCamera *(__cdecl*)(RwCamera *, float))0x7EE1D0)(camera, nearClip)
// FRAME
#define RwFrameGetLTM(frame) ((RwMatrix *(__cdecl *)(RwFrame *))0x7F0990)(frame)
#define RwFrameCreate() ((RwFrame *(__cdecl *)())0x7F0410)()
#define RwFrameDestroy(frame) ((void (__cdecl *)(RwFrame *))0x7F05A0)(frame)
#define RwFrameRotate(frame, axis, angle, combine) ((void (__cdecl *)(RwFrame *, RwV3d *, float, int))0x7F1010)(frame, axis, angle, combine)
#define RwFrameTranslate(frame, pos, combine) ((void (__cdecl *)(RwFrame *, RwV3d *, int))0x7F0E30)(frame, pos, combine)
#define RwFrameUpdateObjects(frame) ((RwFrame *(__cdecl *)(RwFrame *))0x7F0910)(frame)
#define RwFrameRegisterPlugin(size, pluginId, ctor, dtor, copy) ((int (__cdecl *)(int, int, void *, void *, void *))0x7F1260)(size, pluginId, ctor, dtor, copy)
#define RwFrameForAllObjects(frame, callback, data) ((RwFrame *(__cdecl *)(RwFrame *, void *, void *))0x7F1200)(frame, callback, data)

// GEOMETRY
#define RpGeometryStreamRead(version) ((RpGeometry *(__cdecl *)(unsigned int))0x74D190)(version)
#define RpGeometryForAllMaterials(geometry, callback, data) ((void (__cdecl *)(RpGeometry *, void *, void *))0x74C790)(geometry, callback, data)

// D3D9
#define rwD3D9EnableClippingIfNeeded(object, type) ((int (__cdecl *)(RpAtomic *, int))0x756D90)(object, type)
#define rwD3D9GetRenderState(state, value) ((int (__cdecl *)(int, void *))0x7FC320)(state, value)
#define RpD3D9GeometryGetUsageFlags(geometry) ((unsigned int (__cdecl *)(RpGeometry *))0x7588D0)(geometry)
#define RpD3D9GeometrySetUsageFlags(geometry, flags) ((unsigned int (__cdecl *)(RpGeometry *, unsigned int))0x7588B0)(geometry, flags)
#define rwD3D9RenderStateFlushCache() ((int (__cdecl *)()) 0x7FC200)()
#define rwD3D9CheckValidTextureFormat(format) ((bool (__cdecl *)(D3DFORMAT)) 0x4CBDE0)(format)
#define rwD3D9CheckAutoMipmapGenTextureFormat(format) ((bool (__cdecl *)(D3DFORMAT)) 0x4CBF80)(format)
#define rwD3D9CheckAutoMipmapGenCubeTextureFormat(format) ((bool (__cdecl *)(D3DFORMAT)) 0x4CC020)(format)
#define rwD3D9CheckValidZBufferTextureFormat(format) ((bool (__cdecl *)(D3DFORMAT)) 0x4CBE80)(format)

// VEHICLE PIPE RENDER
#define rwD3D9SetTexture(texture, stage) ((int (__cdecl *)(RwTexture *, int))0x7FDE70)(texture, stage)
#define rwD3D9SetRenderState(state, value) ((int (__cdecl *)(int, int))0x7FC2D0)(state, value)
#define rwD3D9SetTextureStageState(stage, type, value) ((int (__cdecl *)(int, int, int))0x7FC340)(stage, type, value)
#define rwD3D9SetIndices(indices) ((int (__cdecl *)(void *))0x7FA1C0)(indices)
#define rwD3D9SetStreams(streams, useOffsets) ((void (__cdecl *)(RxD3D9VertexStream *, int))0x7FA090)(streams, useOffsets)
#define rwD3D9SetVertexDeclaration(vertexDeclaration) ((int  (__cdecl *)(void *))0x7F9F70)(vertexDeclaration)
#define rwD3D9SetLight(index, light) ((int  (__cdecl *)(int, void *))0x7FA660)(index, light)
#define rwD3D9EnableLight(index, enable) ((int  (__cdecl *)(int, int))0x7FA860)(index, enable)
#define rwD3D9SetTransform(state, matrix) ((int (__cdecl *)(int, RwMatrix *))0x7FA390)(state, matrix)
#define rwD3D9RenderStateVertexAlphaEnable(enable) ((signed int (__cdecl *)(int))0x7FE0A0)(enable)
#define rwD3D9SetPixelShader(shader) ((int (__cdecl *)(void *))0x7F9FF0)(shader)
#define rwD3D9SetVertexShader(shader) ((int (__cdecl *)(void *))0x7F9FB0)(shader)
#define rwD3D9DrawIndexedPrimitive(primitiveType, baseVertexIndex, minIndex, numVertices, startIndex, primitiveCount) ((int (__cdecl *)(int, int, int, int, int, int))0x7FA320)(primitiveType, baseVertexIndex, minIndex, numVertices, startIndex, primitiveCount)
#define rwD3D9DrawPrimitive(primitiveType, startVertex, primitiveCount) ((int (__cdecl *)(int, int, int))0x7FA360)(primitiveType, startVertex, primitiveCount)
#define rwD3D9VSSetActiveWorldMatrix(matrix) ((int (__cdecl *)(RwMatrix *))0x764650)(matrix)
#define rwD3D9VSGetComposedTransformMatrix(m_out) ((D3DXMATRIX *(__cdecl *)(D3DXMATRIX *))0x7646E0)(m_out)
#define rwD3D9VSGetWorldNormalizedTransposeMatrix(m_out) ((D3DXMATRIX *(__cdecl *)(D3DXMATRIX *))0x764C60)(m_out)
#define rwD3D9VSGetWorldViewTransposedMatrix(m_out) ((D3DXMATRIX *(__cdecl *)(D3DXMATRIX *))0x764730)(m_out)
#define rwD3D9SetRenderState(state, value) ((int (__cdecl *)(int, int))0x7FC2D0)(state, value)
#define rwD3D9RWSetRasterStage(raster, stage) ((int (__cdecl *)(RwRaster *, int))0x7FDCD0)(raster, stage)
// CAMERA
#define RsCameraShowRaster(camera) ((int (__cdecl *)(RwCamera *))0x619440)(camera)
#define RwCameraEndUpdate(camera) ((RwCamera *(__cdecl *)(RwCamera *))0x7EE180)(camera)
#define RwCameraBeginUpdate(camera) ((RwCamera *(__cdecl *)(RwCamera *))0x7EE190)(camera)
#define RsCameraBeginUpdate(camera) ((signed int (__cdecl *)(RwCamera *))0x619450)(camera)
#define RwCameraClear(camera, color, clearMode) ((RwCamera *(__cdecl *)(RwCamera *, RwRGBA *, int))0x7EE340)(camera, color, clearMode)
#define RwCameraCreate() ((RwCamera *(__cdecl *)())0x7EE4F0)()
#define RwCameraDestroy(camera) ((void (__cdecl *)(RwCamera *))0x7EE4B0)(camera)
#define RwCameraSetProjection(camera, projection) ((void (__cdecl *)(RwCamera *, unsigned int))0x7EE3A0)(camera, projection)
#define RwCameraSetViewWindow(camera, viewWindow) ((void (__cdecl *)(RwCamera *, RwV2d *))0x7EE410)(camera, viewWindow)
#define RwCameraSetFarClipPlane(camera, farClip) ((RwCamera *(__cdecl *)(RwCamera *, float))0x7EE2A0)(camera, farClip)
#define RwTextureSetAutoMipmapping(enable) ((signed int (__cdecl*)(int))0x7F3560)(enable)
// MOUSE
#define RsMouseSetPos(pos) ((int (__cdecl *)(RwV2d *)) 0x6194A0)(pos)

// OBJECT
#define RwObjectHasFrameSetFrame(object, frame) ((void (__cdecl *)(RwObject *, RwFrame *))0x804EF0)(object, frame)

//RASTER
#define RwRasterCreate(width, height, depth, flags) ((RwRaster *(__cdecl *)(int, int, int, int))0x7FB230)(width, height, depth, flags)
#define RwRasterDestroy(raster) ((void (__cdecl *)(RwRaster *))0x7FB020)(raster)
#define RwRasterRenderFast(raster, x, y) ((RwRaster * (__cdecl *)(RwRaster *, int, int))0x7FAF50)(raster, x, y)
#define RwRasterPushContext(raster) ((RwRaster * (__cdecl *)(RwRaster *))0x7FB060)(raster)
#define RwRasterPopContext() ((RwRaster * (__cdecl *)())0x7FB110)()
#define RenderStatesSave() ((void (__cdecl *)())0x700CC0)()
#define RenderStatesSetHSL() ((void (__cdecl *)())0x700D70)()
#define RenderStatesReset() ((void (__cdecl *)())0x700E00)()

//MATRIX
#define RwMatrixUpdate(matrix) ((RwMatrixTag *(__cdecl *)(RwMatrixTag *)) 0x7F18A0)(matrix)
#define RwMatrixMultiply(result, left, right) ((RwMatrixTag *(__cdecl *)(RwMatrixTag *, RwMatrixTag *, RwMatrixTag *)) 0x7F18B0)(result, left, right)
#define RwMatrixRotate(matrix, axis, angle, combineOp) ((RwMatrixTag *(__cdecl *)(RwMatrixTag *, RwV3d *, float, int)) 0x7F1FD0)(matrix, axis, angle, combineOp);
#define RwMatrixInvert(out, in) ((RwMatrixTag *(__cdecl *)(RwMatrixTag *, RwMatrixTag *)) 0x7F2070)(out, in)
#define RwMatrixScale(matrix, scale, combineOp) ((RwMatrixTag *(__cdecl *)(RwMatrixTag *, RwV3d *, int)) 0x7F22C0)(matrix, scale, combineOp)

// globals
#define RwEngineInstance (*(RwGlobals **)0xC97B24)

#define RtPNGImageRead(path) ((RwImage *(__cdecl *)(const char *))0x7CF9B0)(path)
#define RwImageFindRasterFormat(image, rasterType, width, height, depth, format) ((RwImage *(__cdecl *)(RwImage *, unsigned int, unsigned int *, \
	unsigned int *, unsigned int *, unsigned int *))0x8042C0)(image, rasterType, width, height, depth, format)
#define RwRasterSetFromImage(raster, image) ((RwRaster *(__cdecl *)(RwRaster *, RwImage *))0x804290)(raster, image)
#define RwImageDestroy(image) ((bool (__cdecl *)(RwImage *))0x802740)(image)
#define RwTextureCreate(raster) ((RwTexture *(__cdecl *)(RwRaster *))0x7F37C0)(raster)
#define RwTextureDestroy(texture) ((unsigned int (__cdecl *)(RwTexture *))0x7F3820)(texture)

// MATERIAL
#define RpMaterialUVAnimExists(material) ((unsigned int (__cdecl *)(RpMaterial *))0x7CC530)(material)
#define RpMaterialUVAnimAddAnimTime(material, time) ((unsigned int (__cdecl *)(RpMaterial *, float))0x7CC4B0)(material, time)
#define RpMaterialUVAnimApplyUpdate(material) ((unsigned int (__cdecl *)(RpMaterial *))0x7CC110)(material)

// RPHANIM
#define RpHAnimIDGetIndex(hierarchy, id) ((unsigned int (__cdecl *)(RpHAnimHierarchy *, unsigned int))0x7C51A0)(hierarchy, id)
#define RpHAnimHierarchyGetMatrixArray(hierarchy) ((RwMatrixTag *(__cdecl *)(RpHAnimHierarchy *))0x7C5120)(hierarchy)

// RWIM3D
#define RwIm3DEnd() ((void (__cdecl *)())0x7EF520)()
#define RwIm3DTransform(pVerts, numVerts, ltm, flags) ((RxObjSpace3DVertex *(__cdecl *)(RxObjSpace3DVertex *, unsigned int, RwMatrixTag *, unsigned int))0x7EF450)(pVerts, numVerts, ltm, flags)
#define RwIm3DRenderIndexedPrimitive(primType, indices, numIndices) ((void (__cdecl *)(int, __int16 *, int))0x7EF550)(primType, indices, numIndices)

// QUAT
#define RtQuatRotate(quat, axis, angle, combineOp) ((void (__cdecl *)(RtQuat *, RwV3d *, float, int))0x7EB7C0)(quat, axis, angle, combineOp)

// BMP
#define RtBMPImageRead(path) ((RwImage *(__cdecl *)(char *))0x7CDF60)(path)

// IMAGE
#define RwImageApplyMask(image, mask) ((RwImage *(__cdecl *)(RwImage *, RwImage *))0x802AF0)(image, mask)
#define RwImageMakeMask(image) ((RwImage *(__cdecl *)(RwImage *))0x802A20)(image)

// DDS
#define RwD3D9DDSTextureRead(path) ((RwTexture *(__cdecl *)(char *))0x820A10)(path)

#define gtaD3D9TexCreate(width, height, numLevels, d3dFormat, texture) ((HRESULT (__cdecl *)(int, int, unsigned int, D3DFORMAT, IDirect3DTexture9 **))0x730510)(width, height, numLevels, d3dFormat, texture)
#define RwD3D9RasterCreate(width, height, d3dFormat, rasterFormat) ((RwRaster *(__cdecl *)(int, int, D3DFORMAT, unsigned int))0x4CD050)(width, height, d3dFormat, rasterFormat)

#define RwRasterLockPalette(raster, lockMode) ((char *(__cdecl *)(RwRaster *, unsigned int))0x7FB0E0)(raster, lockMode)
#define RwRasterUnlockPalette(raster) ((char *(__cdecl *)(RwRaster *))0x7FAFF0)(raster)
#define RwRasterLock(raster, level, lockMode) ((char *(__cdecl *)(RwRaster *, unsigned int, unsigned int))0x7FB2D0)(raster, level, lockMode)
#define RwRasterUnlock(raster) ((char *(__cdecl *)(RwRaster *))0x7FAEC0)(raster)
#define RwRasterRegisterPlugin(size, pluginId, ctor, dtor, copy) ((int (__cdecl *)(int, int, void *, void *, void *))0x7FB0B0)(size, pluginId, ctor, dtor, copy)

#define RwD3D9DeviceCaps ((D3DCAPS9 *)0xC9BF00)

#define RwD3DDevice (*(IDirect3DDevice9 **)0xC97C28)

//#define RsGlobal ((RsGlobalType *)0xC17040)

#define _RwGameCamera    (*(RwCamera**)0xC1703C)
