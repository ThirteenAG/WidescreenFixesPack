#ifndef PCSX2F_API_H_INCLUDED
#define PCSX2F_API_H_INCLUDED
#include <stdint.h>

struct PluginInfo
{
    uint32_t Base;
    uint32_t EntryPoint;
    uint32_t SegmentFileOffset;
    uint32_t Size;
    uint32_t PluginDataAddr;
    uint32_t PluginDataSize;
    uint32_t PCSX2DataAddr;
    uint32_t PCSX2DataSize;
    uint32_t CompatibleCRCListAddr;
    uint32_t CompatibleCRCListSize;
    uint32_t CompatibleElfCRCListAddr;
    uint32_t CompatibleElfCRCListSize;
    uint32_t KeyboardStateAddr;
    uint32_t KeyboardStateSize;
    uint32_t MouseStateAddr;
    uint32_t MouseStateSize;
    uint32_t CheatStringAddr;
    uint32_t CheatStringSize;
    uint32_t OSDTextAddr;
    uint32_t OSDTextSize;
    uint32_t FrameLimitUnthrottleAddr;
    uint32_t FrameLimitUnthrottleSize;
    uint32_t CLEOScriptsAddr;
    uint32_t CLEOScriptsSize;

#ifdef __cplusplus
    bool isValid() { return (Base != 0 && EntryPoint != 0 && Size != 0); }
#endif
};

struct PluginInfoInvoker
{
    uint32_t Base;
    uint32_t EntryPoint;
};

enum PCSX2DataType
{
    PCSX2Data_DesktopSizeX,
    PCSX2Data_DesktopSizeY,
    PCSX2Data_WindowSizeX,
    PCSX2Data_WindowSizeY,
    PCSX2Data_IsFullscreen,
    PCSX2Data_AspectRatioSetting,

    PCSX2Data_Size
};

enum AspectRatioType
{
    Stretch,
    RAuto4_3_3_2,
    R4_3,
    R16_9,
    MaxCount
};

enum KeyboardBufState
{
    CurrentState,
    PreviousState,

    StateNum,

    CheatStringLen = 25,
    StateSize = 256
};

struct CMouseControllerState
{
    int8_t	lmb;
    int8_t	rmb;
    int8_t	mmb;
    int8_t	wheelUp;
    int8_t	wheelDown;
    int8_t	bmx1;
    int8_t	bmx2;
    float   Z;
    float   X;
    float   Y;
};

enum OSDString
{
    OSDStringNum = 10,
    OSDStringSize = 255
};

enum PtrType
{
    KeyboardData,
    MouseData,
    CheatStringData
};

enum
{
    MaxIniSize = 5000
};

enum
{
    PluginsMaxNum = 100
};

extern int CompatibleCRCList[];
extern char ElfPattern[];
#ifdef PLUGIN_INVOKER
struct PluginInfo PluginData[PluginsMaxNum];
#else
extern char PluginData[MaxIniSize];
#endif
extern int PCSX2Data[PCSX2Data_Size];
extern char KeyboardState[StateNum][StateSize];
extern struct CMouseControllerState MouseState[StateNum];
extern char CheatString[CheatStringLen];
extern char OSDText[OSDStringNum][OSDStringSize];
extern char FrameLimitUnthrottle;

#endif
