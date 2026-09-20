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
    // 1 when the emulator answers PCSX2F_GuestBeforeUIDraw, 0 when it does not
    // (an emulator without the support of the plugin injector hands an unknown
    // syscall to its BIOS). Must stay at the index the injector writes to.
    PCSX2Data_GuestRenderPhase,

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

// ---------------------------------------------------------------------------
// Drawing into the frame of the game, before its UI
//
// The frame of a PS2 game is its world and its UI drawn into the same buffer, so
// everything drawn after the frame (which is what a present hook does) ends up on
// top of the UI. A guest plugin knows the point of its game where the world is
// done and the UI is not drawn yet, so it reports it with PCSX2F_GuestBeforeUIDraw.
// The emulator then stops the EE where the game is, drains the queue of the GS so
// that the world is really in the frame, lets the plugins of the injector draw
// into the frame, and only then continues with the UI of the game: what they drew
// is under the UI.
//
// The call costs one synchronization of the GS and the GPU, so it is worth doing
// once per frame and not for every bit of UI the game draws.
// ---------------------------------------------------------------------------

// The syscall the guest plugin executes. The magic value is what tells a call of
// this API apart from a game that happens to use the same syscall number, so
// every call has to carry it.
enum
{
    PCSX2F_GuestSyscallNumber = 0xF0,
    PCSX2F_GuestSyscallMagic = 0x50434652 /* 'PCFR' */
};

enum PCSX2FRenderPhase
{
    // The guest is between the world and its UI: what a plugin draws now is under
    // the UI of the game.
    PCSX2FRenderPhase_BeforeGuestUI = 1,
};

#if defined(__mips__)
// Call this where the frame of the game is between the world and the UI. Does
// nothing when the emulator does not support it, see PCSX2Data_GuestRenderPhase.
// A plugin that reports more than one point of its frame calls the one below
// instead, with the phase of each of them, see PCSX2FRenderPhase. Only the phase
// of this one draws into the frame.
static inline void PCSX2F_GuestBeforeUIDrawPhase(uint32_t phase)
{
    uint32_t number = PCSX2F_GuestSyscallNumber;
    uint32_t magic = PCSX2F_GuestSyscallMagic;

    if (!PCSX2Data[PCSX2Data_GuestRenderPhase])
        return;

    __asm__ __volatile__(
        "addu $3, %0, $0\n"     /* v1: the number of the syscall */
        "addu $4, %1, $0\n"     /* a0: the magic, so the emulator knows it is this API */
        "addu $5, %2, $0\n"     /* a1: the phase */
        "syscall\n"
        :
    : "r"(number), "r"(magic), "r"(phase)
        : "$3", "$4", "$5", "memory");
}

static inline void PCSX2F_GuestBeforeUIDraw(void)
{
    PCSX2F_GuestBeforeUIDrawPhase(PCSX2FRenderPhase_BeforeGuestUI);
}
#endif

#endif
