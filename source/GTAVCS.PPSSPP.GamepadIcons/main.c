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
#define MODULE_NAME "GTAVCS.PPSSPP.GamepadIcons"
#define LOG_PATH "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.GamepadIcons/GTAVCS.PPSSPP.GamepadIcons.log"
#define INI_PATH "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.GamepadIcons/GTAVCS.PPSSPP.GamepadIcons.ini"

#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, PSP_MODULE_USER, 1, 0);
_Static_assert(sizeof(MODULE_NAME) - 1 < 28, "MODULE_NAME can't have more than 28 characters");
#endif

#ifndef __INTELLISENSE__
#define align16 __attribute__((aligned(16)))
#else
#define align16
#endif

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

struct Texture
{
    struct RslTexture texture;
    struct RslRaster raster;
};

struct Tex
{
    const char* name;
    struct RslTexture* texture;
};

struct Tex texNames[] = {
    "btn_down_PSP", 0,
    "btn_up_PSP", 0,
    "btn_cross_PSP", 0,
    "btn_circle_PSP", 0,
    "btn_left_PSP", 0,
    "btn_right_PSP", 0,
    "btn_square_PSP", 0,
    "fe_controller_new", 0,
    "fe_arrows_onfoot_layout1", 0,
    "fe_arrows_layout_right", 0,
    "fe_arrows_invehicle_layout2", 0,
    "GameText", 0,
};

const char* btnPrefix[] = {
    "PSP",
    "360",
    "XBO",
    "PS3",
    "PS4",
    "PS5",
    "SWI",
    "DEK",
}; int curBtnPrefix = 0;

struct RslTexture* (*RslTextureRead)(const char*);
struct RslTexture* RslTextureReadHook(const char* name)
{
    struct RslTexture* tex = RslTextureRead(name);

    for (size_t i = 0; i < sizeof(texNames) / sizeof(texNames[0]); i++)
    {
        if (!strcmp(name, texNames[i].name)) {
            strcpy((char*)tex->raster->data, btnPrefix[curBtnPrefix]);
            texNames[i].texture = tex;
        }
    }

    return tex;
}

char* (*CPad__GetPad)(int pad_index);
char (*GuiLeft)(char* pad);
char (*GuiRight)(char* pad);
void (*SetControllerMode)(int a1, char a2);
void SetControllerModeHook(int a1, char a2)
{
    {
        if (GuiLeft(CPad__GetPad(0)) && (a2 == 0 || a2 == 2))
            curBtnPrefix--;
        else if (GuiRight(CPad__GetPad(0)) && (a2 == 1 || a2 == 3))
            curBtnPrefix++;

        int size = sizeof(btnPrefix) / sizeof(btnPrefix[0]);
        if (curBtnPrefix < 0)
            curBtnPrefix = size - 1;
        else if (curBtnPrefix >= size)
            curBtnPrefix = 0;

        for (size_t i = 0; i < sizeof(texNames) / sizeof(texNames[0]); i++)
        {
            if (texNames[i].texture) {
                strcpy((char*)texNames[i].texture->raster->data, btnPrefix[curBtnPrefix]);
            }
        }

        SceUID cfg_uid = sceIoOpen(INI_PATH, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
        if (cfg_uid >= 0)
        {
            sceIoWrite(cfg_uid, btnPrefix[curBtnPrefix], strlen(btnPrefix[curBtnPrefix]));
            sceIoClose(cfg_uid);
        }
    }

    return SetControllerMode(a1, a2);
}

int OnModuleStart() {
    sceKernelDelayThread(120000);

    SceIoStat info;
    sceIoGetstat(INI_PATH, &info);
    SceUID cfg_uid = sceIoOpen(INI_PATH, PSP_O_RDONLY, 0777);
    if (cfg_uid >= 0)
    {
        #define MIN(a,b) (((a)<(b))?(a):(b))
        char s[4];
        sceIoRead(cfg_uid, s, MIN(info.st_size, sizeof(s) / sizeof(s[0])));
        sceIoClose(cfg_uid);
        s[3] = 0;
        #undef MIN

        for (size_t i = 0; i < sizeof(btnPrefix) / sizeof(btnPrefix[0]); i++)
        {
            if (!strcmp(s, btnPrefix[i])) {
                curBtnPrefix = i;
                break;
            }
        }
    }

    // HUD and fonts
    uintptr_t ptr_8AF4558 = pattern.get(0, "25 28 C0 00 00 00 02 AE", -4);
    RslTextureRead = (struct RslTexture*(*)(const char*))injector.MakeJAL(ptr_8AF4558, (uintptr_t)RslTextureReadHook);
    
    // fronten1 btns
    uintptr_t ptr_8904790 = pattern.get(0, "25 20 A0 00 ? ? ? ? B8 00 02 AE", -4);
    RslTextureRead = (struct RslTexture*(*)(const char*))injector.MakeJAL(ptr_8904790, (uintptr_t)RslTextureReadHook);
    
    // fe_controller_new
    uintptr_t ptr_8B3F610 = pattern.get(0, "25 20 A0 00 D8 00 05 8E", -4);
    RslTextureRead = (struct RslTexture*(*)(const char*))injector.MakeJAL(ptr_8B3F610, (uintptr_t)RslTextureReadHook);
    
    // fe_arrows
    uintptr_t ptr_8B3F64C = pattern.get(0, "25 20 A0 00 D4 00 04 8E", -4);
    RslTextureRead = (struct RslTexture*(*)(const char*))injector.MakeJAL(ptr_8B3F64C, (uintptr_t)RslTextureReadHook);

    // fe_arrows_layout_right
    uintptr_t ptr_8B3F688 = pattern.get(0, "25 20 A0 00 ? ? ? ? F4 00 02 AE", -4);
    RslTextureRead = (struct RslTexture* (*)(const char*))injector.MakeJAL(ptr_8B3F688, (uintptr_t)RslTextureReadHook);

    uintptr_t ptr_898B428 = pattern.get(0, "40 29 04 00 23 20 A4 00", -0);
    CPad__GetPad = (char*(*)(int))ptr_898B428;

    uintptr_t ptr_898BEC8 = pattern.get(0, "00 00 B0 AF 04 00 BF AF ? ? ? ? 25 80 80 00 ? ? ? ? 00 00 00 00 ? ? ? ? 25 20 00 02 ? ? ? ? 00 00 00 00 ? ? ? ? 01 00 02 34 25 10 00 00", -4);
    GuiLeft = (char(*)(char*))ptr_898BEC8;

    uintptr_t ptr_898BF10 = pattern.get(1, "00 00 B0 AF 04 00 BF AF ? ? ? ? 25 80 80 00 ? ? ? ? 00 00 00 00 ? ? ? ? 25 20 00 02 ? ? ? ? 00 00 00 00 ? ? ? ? 01 00 02 34 25 10 00 00", -4);
    GuiRight = (char(*)(char*))ptr_898BF10;

    uintptr_t ptr_89C6C64 = pattern.get(0, "25 28 C0 00 ? ? ? ? 00 00 00 00 2B 20 04 00 FF 00 86 30 25 20 A0 00 ? ? ? ? 25 28 C0 00 ? ? ? ? 00 00 00 00 2B 20 04 00 FF 00 86 30 25 20 A0 00 ? ? ? ? 25 28 C0 00 ? ? ? ? 00 00 00 00 2B 20 04 00", -4);
    SetControllerMode = (void(*)(int, char))injector.MakeJAL(ptr_89C6C64, (uintptr_t)SetControllerModeHook);

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