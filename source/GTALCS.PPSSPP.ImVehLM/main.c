#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <systemctrl.h>

#include "../../includes/psp/log.h"
#include "../../includes/psp/injector.h"
#include "../../includes/psp/patterns.h"
#include "../../includes/psp/inireader.h"
#include "../../includes/psp/gvm.h"
#include "../../includes/psp/mips.h"

#define MODULE_NAME_INTERNAL "GTA3"
#define MODULE_NAME "GTALCS.PPSSPP.ImVehLM"
#define LOG_PATH "ms0:/PSP/PLUGINS/GTALCS.PPSSPP.ImVehLM/GTALCS.PPSSPP.ImVehLM.log"
#define INI_PATH "ms0:/PSP/PLUGINS/GTALCS.PPSSPP.ImVehLM/GTALCS.PPSSPP.ImVehLM.ini"
#define ASSETS_PATH "ms0:/PSP/PLUGINS/GTALCS.PPSSPP.ImVehLM/assets/"

#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, PSP_MODULE_USER, 1, 0);
#endif

#ifndef __INTELLISENSE__
#define align16 __attribute__((aligned(16)))
#else
#define align16
#endif

//#define DUMPER_MODE

struct RslV3
{
    float x, y, z;
};

struct RslMatrix
{
    struct RslV3 right;
    uint32_t flags;
    struct RslV3 up;
    uint32_t pad1;
    struct RslV3 at;
    uint32_t pad2;
    struct RslV3 pos;
    uint32_t pad3;
};

struct RslLLLink
{
    struct RslLLLink* next;
    struct RslLLLink* prev;
};

struct RslLinkList
{
    struct RslLLLink link;
};

struct RslObject {
    uint8_t  type;
    uint8_t  subType;
    uint8_t  flags;
    uint8_t  privateFlags;
    void* parent;
};

struct RslObjectHasNode {
    struct RslObject   object;
    struct RslLLLink   lNode;
    void      (*sync)();
};

struct RslRaster {
    uint32_t unk1;
    uint8_t* data;
    int16_t  minWidth;
    uint8_t  logWidth;
    uint8_t  logHeight;
    uint8_t  depth;
    uint8_t  mipmaps;
    uint16_t unk2;	// like RW raster format?
};

struct RslTexList {
    struct RslObject   object;
    struct RslLinkList texturesInDict;
    struct RslLLLink   lInInstance;
};

struct RslTexture {
    struct RslRaster* raster;
    struct RslTexList* dict;
    struct RslLLLink lInDictionary;
    char name[32];
    char mask[32];
};

struct RslNode {
    struct RslObject object;
    struct RslLinkList objectList;
    struct RslMatrix modelling;
    struct RslMatrix ltm;
    struct RslNode* child;
    struct RslNode* next;
    struct RslNode* root;

    // RwHAnimNodeExtension
#ifdef LCS
    int32              nodeId;
#else
    int32_t		   unknown1;
    int32_t		   unknown2;
#endif
    void*/*RslTAnimTree**/ hier;
    // R* Node name
    char* name;
    // R* Visibility
    int32_t hierId;
};

struct RslElementGroup {
    struct RslObject   object;
    struct RslLinkList atomicList;
};

struct RslMaterialList {
    struct RslMaterial** materials;
    int32_t         numMaterials;
    int32_t         space;
};

struct RslGeometry {
    struct RslObject object;
    int16_t refCount;
    int16_t pad1;
    struct RslMaterialList matList;
    void* /*RslSkin**/ skin;
};

struct RslMatFXEnv {
    struct RslNode* frame;
    union {
        char* texname;
        struct RslTexture* texture;
    };
    float   intensity;
};

struct RslMatFX {
    union {
        struct RslMatFXEnv env;
    };
    int32_t     effectType;
};

struct RGBA {
    char r, g, b, a;
};

struct RslMaterial {
    union {
        char* texname;
        struct RslTexture* texture;
    };
    struct RGBA color;
    uint32_t refCount;
    struct RslMatFX* matfx;
};

struct RslElement {
    struct RslObjectHasNode object;
    struct RslGeometry* geometry;
    struct RslElementGroup* clump;
    struct RslLLLink inElementGroupLink;
    void*/*RslElementCallBackRender*/ renderCallBack;
    // CVisibilityComponents
    int16_t modelInfoId;
    uint16_t visIdFlag;
    // RpSkin
    void*/*RslTAnimTree**/ hier;
};

struct m_nVehicleFlags {
    unsigned char bIsLawEnforcer : 1; // Is this guy chasing the player at the moment
    unsigned char bIsAmbulanceOnDuty : 1; // Ambulance trying to get to an accident
    unsigned char bIsFireTruckOnDuty : 1; // Firetruck trying to get to a fire
    unsigned char bIsLocked : 1; // Is this guy locked by the script (cannot be removed)
    unsigned char bEngineOn : 1; // For sound purposes. Parked cars have their engines switched off (so do destroyed cars)
    unsigned char bIsHandbrakeOn : 1; // How's the handbrake doing ?
    unsigned char bLightsOn : 1; // Are the lights switched on ?
    unsigned char bFreebies : 1; // Any freebies left in this vehicle ?

    unsigned char bIsVan : 1; // Is this vehicle a van (doors at back of vehicle)
    unsigned char bIsBus : 1; // Is this vehicle a bus
    unsigned char bIsBig : 1; // Is this vehicle a bus
    unsigned char bLowVehicle : 1; // Need this for sporty type cars to use low getting-in/out anims
    unsigned char bComedyControls : 1; // Will make the car hard to control (hopefully in a funny way)
    unsigned char bWarnedPeds : 1; // Has scan and warn peds of danger been processed?
    unsigned char bCraneMessageDone : 1; // A crane message has been printed for this car allready
    unsigned char bTakeLessDamage : 1; // This vehicle is stronger (takes about 1/4 of damage)

    unsigned char bIsDamaged : 1; // This vehicle has been damaged and is displaying all its components
    unsigned char bHasBeenOwnedByPlayer : 1;// To work out whether stealing it is a crime
    unsigned char bFadeOut : 1; // Fade vehicle out
    unsigned char bIsBeingCarJacked : 1; // Fade vehicle out
    unsigned char bCreateRoadBlockPeds : 1;// If this vehicle gets close enough we will create peds (coppers or gang members) round it
    unsigned char bCanBeDamaged : 1; // Set to FALSE during cut scenes to avoid explosions
    unsigned char bOccupantsHaveBeenGenerated : 1; // Is true if the occupants have already been generated. (Shouldn't happen again)
    unsigned char bGunSwitchedOff : 1; // Level designers can use this to switch off guns on boats

    unsigned char bVehicleColProcessed : 1;// Has ProcessEntityCollision been processed for this car?
    unsigned char bIsCarParkVehicle : 1; // Car has been created using the special CAR_PARK script command
    unsigned char bHasAlreadyBeenRecorded : 1; // Used for replays
    unsigned char bPartOfConvoy : 1;
    unsigned char bHeliMinimumTilt : 1; // This heli should have almost no tilt really
    unsigned char bAudioChangingGear : 1; // sounds like vehicle is changing gear
    unsigned char bIsDrowning : 1; // is vehicle occupants taking damage in water (i.e. vehicle is dead in water)
    unsigned char bTyresDontBurst : 1; // If this is set the tyres are invincible

    unsigned char bCreatedAsPoliceVehicle : 1;// True if this guy was created as a police vehicle (enforcer, policecar, miamivice car etc)
    unsigned char bRestingOnPhysical : 1; // Dont go static cause car is sitting on a physical object that might get removed
    unsigned char bParking : 1;
    unsigned char bCanPark : 1;
    unsigned char bFireGun : 1; // Does the ai of this vehicle want to fire it's gun?
    unsigned char bDriverLastFrame : 1; // Was there a driver present last frame ?
    unsigned char bNeverUseSmallerRemovalRange : 1;// Some vehicles (like planes) we don't want to remove just behind the camera.
    unsigned char bIsRCVehicle : 1; // Is this a remote controlled (small) vehicle. True whether the player or AI controls it.

    unsigned char bAlwaysSkidMarks : 1; // This vehicle leaves skidmarks regardless of the wheels' states.
    unsigned char bEngineBroken : 1; // Engine doesn't work. Player can get in but the vehicle won't drive
    unsigned char bVehicleCanBeTargetted : 1;// The ped driving this vehicle can be targetted, (for Torenos plane mission)
    unsigned char bPartOfAttackWave : 1; // This car is used in an attack during a gang war
    unsigned char bWinchCanPickMeUp : 1; // This car cannot be picked up by any ropes.
    unsigned char bImpounded : 1; // Has this vehicle been in a police impounding garage
    unsigned char bVehicleCanBeTargettedByHS : 1;// Heat seeking missiles will not target this vehicle.
    unsigned char bSirenOrAlarm : 1; // Set to TRUE if siren or alarm active, else FALSE

    unsigned char bHasGangLeaningOn : 1;
    unsigned char bGangMembersForRoadBlock : 1;// Will generate gang members if NumPedsForRoadBlock > 0
    unsigned char bDoesProvideCover : 1; // If this is false this particular vehicle can not be used to take cover behind.
    unsigned char bMadDriver : 1; // This vehicle is driving like a lunatic
    unsigned char bUpgradedStereo : 1; // This vehicle has an upgraded stereo
    unsigned char bConsideredByPlayer : 1; // This vehicle is considered by the player to enter
    unsigned char bPetrolTankIsWeakPoint : 1;// If false shootong the petrol tank will NOT Blow up the car
    unsigned char bDisableParticles : 1; // Disable particles from this car. Used in garage.

    unsigned char bHasBeenResprayed : 1; // Has been resprayed in a respray garage. Reset after it has been checked.
    unsigned char bUseCarCheats : 1; // If this is true will set the car cheat stuff up in ProcessControl()
    unsigned char bDontSetColourWhenRemapping : 1;// If the texture gets remapped we don't want to change the colour with it.
    unsigned char bUsedForReplay : 1; // This car is controlled by replay and should be removed when replay is done.
};

enum VEHICLES
{
    _6ATV = 170,
    ADMIRAL = 171,
    CHEETAH = 172,
    AUTOGYRO = 173,
    BAGGAGE = 174,
    BANSHEE = 175,
    PEREN = 176,
    BLISTAC = 177,
    BMXBOY = 178,
    BMXGIRL = 179,
    BOBCAT = 180,
    BULLDOZE = 181,
    BURRITO = 182,
    CABBIE = 183,
    CADDY = 184,
    SPEEDER2 = 185,
    PIMP = 186,
    DELUXO = 187,
    HUEY = 188,
    HUEYHOSP = 189,
    ELECTRAG = 190,
    ELECTRAP = 191,
    ESPERANT = 192,
    FBICAR = 193,
    FIRETRUK = 194,
    GLENDALE = 195,
    GREENWOO = 196,
    HERMES = 197,
    HOVERCR = 198,
    IDAHO = 199,
    LANDSTAL = 200,
    MANANA = 201,
    MOP50 = 202,
    OCEANIC = 203,
    VICECHEE = 204,
    SANCHEZ = 205,
    STALLION = 206,
    POLICEM = 207,
    BOBO = 208,
    PATRIOT = 209,
    PONY = 210,
    SENTINEL = 211,
    PCJ600 = 212,
    MAVERICK = 213,
    REEFER = 214,
    SPEEDER = 215,
    LINERUN = 216,
    WALTON = 217,
    BARRACKS = 218,
    PREDATOR = 219,
    FLATBED = 220,
    AMMOTRUK = 221,
    BIPLANE = 222,
    MOONBEAM = 223,
    RUMPO = 224,
    YOLA = 225,
    TAXI = 226,
    AMBULAN = 227,
    STRETCH = 228,
    FAGGIO = 229,
    QUAD = 230,
    ANGEL = 231,
    FREEWAY = 232,
    JETSKI = 233,
    ENFORCE = 234,
    BOXVIL = 235,
    BENSON = 236,
    COACH = 237,
    MULE = 238,
    VOODOO = 239,
    SECURICA = 240,
    TRASH = 241,
    TOPFUN = 242,
    YANKEE = 243,
    MRWHOO = 244,
    SANDKING = 245,
    RHINO = 246,
    DINGHY = 247,
    MARQUIS = 248,
    RIO = 249,
    TROPIC = 250,
    FORKLIFT = 251,
    STREETFI = 252,
    VIRGO = 253,
    STINGER = 254,
    BFINJECT = 255,
    PHEONIX = 256,
    SQUALO = 257,
    JETMAX = 258,
    MESA = 259,
    VCNMAV = 260,
    POLMAV = 261,
    SPARROW = 262,
    SESPAROW = 263,
    SCARAB = 264,
    CHOLLO = 265,
    COMET = 266,
    CUBAN = 267,
    FBIRAN = 268,
    GANGBUR = 269,
    INFERNUS = 270,
    REGINA = 271,
    SABRE = 272,
    SABRETB = 273,
    SENTXS = 274,
    HUNTER = 275,
    WASHIN = 276,
    COASTG = 277,
    SKIMMER = 278,
    CHOPPER = 279,
    AIRTRAIN = 280,

    NUM_VEHICLES
};

char* carTextures[] = {
    "vcs_6atvbit",
    "admiral868bit_sl",
    "vicechee868bit128",
    "NA",
    "baggage8bit128",
    "banshee868bit_sl",
    "iz_perennial_interior",
    "blistac868bit_sl",
    "NA",
    "NA",
    "vcs_bobcatbit",
    "vcs_dozerbit2",
    "burritobit",
    "cabbie8bit",
    "caddy8bit128",
    "NA",
    "pimpbit",
    "deluxobit",
    "NA",
    "NA",
    "NA",
    "NA",
    "esperanto",
    "vc_vehicledoors_sl",
    "firetruk8bit128",
    "glendale_main64x64",
    "greenwoodbit",
    "hermesbody64detail_ca",
    "NA",
    "iz_idaho_interior",
    "iz_landstalker_interior",
    "iz_manana_interior",
    "vcs_mopedbit",
    "vcs_oceanicbit",
    "vicechee868bit128",
    "sanchez8bit128",
    "stallion868bit128",
    "policebits",
    "NA",
    "patriotbit",
    "ponybits_ca",
    "iz_sentinel86_interior",
    "pcj6008bit128",
    "NA",
    "NA",
    "NA",
    "linerun_lights",
    "waltonbit",
    "barracksbits",
    "NA",
    "flatbed868bit128",
    "armytruk8bit128",
    "NA",
    "iz_moonbeam_interior",
    "rumpobits_ca",
    "NA",
    "miami_cabbietex128",
    "ambulan8bit128",
    "stretchbit",
    "faggio8bit128",
    "vcs_quadbitv3_sjl",
    "angel8bit128",
    "angel8bit128",
    "NA",
    "enforcer868bit128",
    "boxville868bit128",
    "benson_bits",
    "coach868bit128",
    "mule8bit128",
    "voodoobit",
    "securica868bit128",
    "trash868bit128",
    "NA",
    "yankee_bits",
    "mrwhoo868bit128",
    "sandkingbit",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "forkbit",
    "vcs_streetfighterbit",
    "virgobit",
    "stingerbit",
    "bfinject868bit128",
    "pheonixbit",
    "NA",
    "NA",
    "mesabit",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "chollo_details1",
    "vcs_cometbit",
    "cubanbody64detail_ca",
    "ca_fbi_rancherbits",
    "gangburbit2",
    "infernus_bitsca",
    "vcs_reginabit",
    "sabre_details1",
    "sabre_details1",
    "vcs_sentbit",
    "NA",
    "vc_vehicledoors_sl",
    "NA",
    "NA",
    "NA",
    "NA",
};

enum
{
    HEADL_OFF__TAILLIGHTS_OFF,
    HEADL_OFF__TAILLIGHTS_ON,
    HEADL_OFF__TAILLIGHTS_BREAK,
    HEADL_OFF__TAILLIGHTS_REVERSE,
    HEADL_ON__TAILLIGHTS_OFF,
    HEADL_ON__TAILLIGHTS_ON,
    HEADL_ON__TAILLIGHTS_BREAK,
    HEADL_ON__TAILLIGHTS_REVERSE,

    NUM_LIGHTS
};

char* rasterNames[] = {
    "headl_off__taillights_off",
    "headl_off__taillights_on",
    "headl_off__taillights_break",
    "headl_off__taillights_reverse",
    "headl_on__taillights_off",
    "headl_on__taillights_on",
    "headl_on__taillights_break",
    "headl_on__taillights_reverse",
};

struct Texture
{
    struct RslTexture texture;
    struct RslRaster raster;
};

struct VehicleLightsData
{
    char bInitialized;
    struct Texture align16 lights[NUM_LIGHTS];
    uint8_t align16 rasterBuffer[NUM_LIGHTS][16384];
};

struct VehicleLightsData align16 arrayofData[NUM_VEHICLES - _6ATV] = { 0 };

size_t getRasterSize(struct RslRaster* raster)
{
#define max(x, y) (((x) > (y)) ? (x) : (y))
    uint8_t* data = raster->data;
    int32_t n = raster->mipmaps;
    uint32_t w = 1 << raster->logWidth;
    uint32_t h = 1 << raster->logHeight;

    while (n--) {
        w = max(w, raster->minWidth);
        data += w * h * raster->depth / 8;
        w /= 2;
        h /= 2;
    }
    return (size_t)(data - raster->data);
#undef max
}

struct RslTexture* DuplicateCarTexture(int arrI, int lightI, struct RslTexture* src)
{
    arrayofData[arrI].lights[lightI].texture = *src;
    arrayofData[arrI].lights[lightI].texture.raster = &arrayofData[arrI].lights[lightI].raster;
    *arrayofData[arrI].lights[lightI].texture.raster = *src->raster;
    arrayofData[arrI].lights[lightI].texture.raster->data = arrayofData[arrI].rasterBuffer[lightI];
    return &arrayofData[arrI].lights[lightI].texture;
}

void PreFillRasters()
{
    //logger.WriteF("sceKernelTotalFreeMemSize %d, %d\n", sceKernelTotalFreeMemSize(), sceKernelMaxFreeMemSize());
    for (size_t i = 0; i < sizeof(arrayofData) / sizeof(arrayofData[0]); i++)
    {
        if (strcmp(carTextures[i], "NA") != 0)
        {
            char path[255];
            strcpy(path, ASSETS_PATH);
            strcat(path, carTextures[i]);
            strcat(path, ".bin");

            SceIoStat info;
            int err = sceIoGetstat(path, &info);
            if (err >= 0)
            {
                SceUID uid = sceIoOpen(path, PSP_O_RDONLY, 0777);
                sceIoRead(uid, arrayofData[i].rasterBuffer[HEADL_OFF__TAILLIGHTS_OFF], info.st_size);
                for (size_t j = HEADL_OFF__TAILLIGHTS_ON; j < sizeof(arrayofData[i].lights) / sizeof(arrayofData[i].lights[0]); j++)
                {
                    memcpy(arrayofData[i].rasterBuffer[j], arrayofData[i].rasterBuffer[HEADL_OFF__TAILLIGHTS_OFF], info.st_size);
                    strcpy((char*)arrayofData[i].rasterBuffer[j], rasterNames[j]);
                }
                if (uid >= 0) sceIoClose(uid);
            }
        }
    }
}

struct RslGeometry* (*RslGeometryForAllMaterials)(struct RslGeometry* geometry, struct RslMaterial*(*)(struct RslMaterial*, void*), void* data);
struct RslElement* (*RslElementGroupForAllElements)(struct RslElement* clump, void*(*)(struct RslElement*, void*), void* data);
int (*CEntityRender)(void*);

enum eLightStatus
{
    LIGHT_STATUS_OK,
    LIGHT_STATUS_BROKEN
};

enum eLights {
    LIGHT_FRONT_LEFT,
    LIGHT_FRONT_RIGHT,
    LIGHT_REAR_LEFT,
    LIGHT_REAR_RIGHT
};

int GetLightStatus(int damage_manager, char light)
{
    return (*(int*)(damage_manager + 16) >> (2 * light)) & 3;
}

#ifdef DUMPER_MODE
int gCounter = 0;
#endif

struct RslTexture** gTex;
struct RslTexture* gTexData;
struct RslMaterial* SetCarTextureCB(struct RslMaterial* material, void* data)
{
    int16_t m_modelIndex = *(int16_t*)((int)data + 0x56);
    int16_t arrayIndex = m_modelIndex - _6ATV;

    if (m_modelIndex >= _6ATV && m_modelIndex < NUM_VEHICLES && material && material->texture && !strcmp(carTextures[arrayIndex], material->texture->name))
    {
        if (!arrayofData[arrayIndex].bInitialized)
        {
            //char path[255];
            //strcpy(path, ASSETS_PATH);
            //strcat(path, carTextures[arrayIndex]);
            //strcat(path, ".bin");
            //
            //logger.WriteF("%s\n", carTextures[arrayIndex]);
            //
            //SceUID uid = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
            //sceIoWrite(uid, material->texture->raster->data, getRasterSize(material->texture->raster));
            //sceIoClose(uid);

            for (size_t i = 0; i < sizeof(arrayofData[arrayIndex].lights) / sizeof(arrayofData[arrayIndex].lights[0]); i++)
                DuplicateCarTexture(arrayIndex, i, material->texture);

            arrayofData[arrayIndex].bInitialized = 1;
        }
        else
        {
#ifdef DUMPER_MODE
            material->texture = &arrayofData[arrayIndex].lights[gCounter].texture;
#else
            struct m_nVehicleFlags m_nVehicleFlags = *(struct m_nVehicleFlags*)((int)data + 0x265);
            float m_fGasPedal = *(float*)((int)data + 0x25C);
            float m_fBreakPedal = *(float*)((int)data + 0x260);
            int m_carDamage = ((int)data + 0x3B0);

            int front_status = !(GetLightStatus(m_carDamage, LIGHT_FRONT_LEFT) == LIGHT_STATUS_BROKEN && GetLightStatus(m_carDamage, LIGHT_FRONT_RIGHT) == LIGHT_STATUS_BROKEN);
            int rear_status = !(GetLightStatus(m_carDamage, LIGHT_REAR_LEFT) == LIGHT_STATUS_BROKEN && GetLightStatus(m_carDamage, LIGHT_REAR_RIGHT) == LIGHT_STATUS_BROKEN);

            gTex = &material->texture;
            gTexData = material->texture;

            if (m_nVehicleFlags.bEngineOn)
            {
                if (m_nVehicleFlags.bLightsOn)
                {
                    if (front_status)
                    {
                        if (rear_status)
                            material->texture = &arrayofData[arrayIndex].lights[HEADL_ON__TAILLIGHTS_ON].texture;
                        else
                            material->texture = &arrayofData[arrayIndex].lights[HEADL_ON__TAILLIGHTS_OFF].texture;
                    }
                    else
                    {
                        if (rear_status)
                            material->texture = &arrayofData[arrayIndex].lights[HEADL_OFF__TAILLIGHTS_ON].texture;
                        else
                            material->texture = &arrayofData[arrayIndex].lights[HEADL_OFF__TAILLIGHTS_OFF].texture;
                    }
                }
                else
                    material->texture = &arrayofData[arrayIndex].lights[HEADL_OFF__TAILLIGHTS_OFF].texture;

                if (rear_status)
                {
                    if (m_fBreakPedal > 0.0 || m_nVehicleFlags.bIsHandbrakeOn)
                    {
                        if (m_nVehicleFlags.bLightsOn && front_status)
                            material->texture = &arrayofData[arrayIndex].lights[HEADL_ON__TAILLIGHTS_BREAK].texture;
                        else
                            material->texture = &arrayofData[arrayIndex].lights[HEADL_OFF__TAILLIGHTS_BREAK].texture;
                    }
                    if (m_fGasPedal < 0.0)
                    {
                        if (m_nVehicleFlags.bLightsOn && front_status)
                            material->texture = &arrayofData[arrayIndex].lights[HEADL_ON__TAILLIGHTS_REVERSE].texture;
                        else
                            material->texture = &arrayofData[arrayIndex].lights[HEADL_OFF__TAILLIGHTS_REVERSE].texture;
                    }
                }
            }
            else
                material->texture = &arrayofData[arrayIndex].lights[HEADL_OFF__TAILLIGHTS_OFF].texture;
#endif
        }
    }
    return material;
}

void* SetElementRendererCB_ImVehLM(struct RslElement* atomic, void* data)
{
    if (atomic->geometry) {
        RslGeometryForAllMaterials(atomic->geometry, SetCarTextureCB, data);
    }
    return atomic;
}

int CEntity__Render(void* car)
{
#ifdef DUMPER_MODE
    int (*FindPlayerVehicle)() = (void*)0x089602C8;
    if (FindPlayerVehicle() == (int)car)
#endif
    {
        // PreRender
        struct RslElement* m_pRwClump = *(struct RslElement**)((int)car + 0x50);
        RslElementGroupForAllElements(m_pRwClump, SetElementRendererCB_ImVehLM, car);
        // Render
        CEntityRender(car);

#ifdef DUMPER_MODE
        enum
        {
            EMULATOR_DEVCTL__TOGGLE_FASTFORWARD = 0x30,
            EMULATOR_DEVCTL__GET_ASPECT_RATIO,
            EMULATOR_DEVCTL__GET_SCALE,
            EMULATOR_DEVCTL__GET_AXIS,
            EMULATOR_DEVCTL__GET_VKEY,
        };

        static const int NKCODE_NUMPAD_0 = 144;
        static const int NKCODE_NUMPAD_7 = 151;

        for (size_t i = NKCODE_NUMPAD_0; i <= NKCODE_NUMPAD_7; i++)
        {
            char key = 0;
            sceIoDevctl("kemulator:", EMULATOR_DEVCTL__GET_VKEY, (void*)i, 0, &key, sizeof(key));
            if (key)
            {
                gCounter = i - NKCODE_NUMPAD_0;
                logger.WriteF("gCounter: %d\n", gCounter);
            }
        }
#endif

        //PostRender
        if (gTex)
        {
            *gTex = gTexData;
            gTex = 0;
        }
    }
}

int OnModuleStart() {
    //sceKernelDelayThread(100000);
    return 0;
    PreFillRasters();
    uintptr_t ptr_19CD0 = pattern.get(0, "00 00 B0 AF 04 00 B1 AF 08 00 B2 AF 25 90 80 00 25 88 A0 00 25 80 C0 00 0C 00 B3 AF 10 00 B4 AF 14 00 B5 AF", -4);
    RslGeometryForAllMaterials = (struct RslGeometry* (*)(struct RslGeometry*, struct RslMaterial* (*)(struct RslMaterial*, void*), void*))ptr_19CD0;
    uintptr_t ptr_1D4700 = pattern.get(0, "0C 00 B3 AF 08 00 93 8C 04 00 B1 AF", -4);
    RslElementGroupForAllElements = (struct RslElement* (*)(struct RslElement*, void* (*)(struct RslElement*, void*), void*))ptr_1D4700;
    uintptr_t ptr_2CC938 = pattern.get(0, "00 00 BF AF 50 00 85 8C ? ? ? ? 00 00 00 00 4C 00 84 8C", -4);
    CEntityRender = (int (*)(void*))ptr_2CC938;

    uintptr_t ptr_40744 = pattern.get(0, "25 20 00 02 10 00 B4 C7", -4);
    injector.MakeJAL(ptr_40744, (uintptr_t)CEntity__Render); //cars
    uintptr_t ptr_254D84 = pattern.get(0, "B8 0B C6 24", 16);
    injector.MakeJAL(ptr_254D84, (uintptr_t)CEntity__Render); //bikes

    sceKernelDcacheWritebackAll();
    sceKernelIcacheClearAll();

    return 0;
}

int module_start(SceSize args, void* argp) {
    if (sceIoDevctl("kemulator:", 0x00000003, NULL, 0, NULL, 0) == 0) {
        SceUID modules[10];
        int count = 0;
        int result = 0;
        if (sceKernelGetModuleIdList(modules, sizeof(modules), &count) >= 0) {
            int i;
            SceKernelModuleInfo info;
            for (i = 0; i < count; ++i) {
                info.size = sizeof(SceKernelModuleInfo);
                if (sceKernelQueryModuleInfo(modules[i], &info) < 0) {
                    continue;
                }

                if (strcmp(info.name, MODULE_NAME_INTERNAL) == 0)
                {
                    injector.SetGameBaseAddress(info.text_addr, info.text_size);
                    pattern.SetGameBaseAddress(info.text_addr, info.text_size);
                    inireader.SetIniPath(INI_PATH);
                    logger.SetPath(LOG_PATH);
                    result = 1;
                }
                else if (strcmp(info.name, MODULE_NAME) == 0)
                {
                    injector.SetModuleBaseAddress(info.text_addr, info.text_size);
                }
            }

            if (result)
                OnModuleStart();
        }
    }
    return 0;
}