#include "Game.hpp"
namespace rain {
namespace {
uintptr_t libguPacket;
uint32_t DisplayList() {
    auto* packet=libguPacket ? *reinterpret_cast<const uint32_t**>(libguPacket) : nullptr;
    return packet ? packet[2] : 0;
}
struct FVector
{
    float X, Y, Z;
};

struct FRotator
{
    int Pitch, Yaw, Roll;
};
#define RAIN_TRACE_UP 600.0f // how far over the camera a roof is looked for, about six metres
#define RAIN_STALE_TICKS 30 // ticks the rain of the game is remembered after it was drawn
#define RAIN_TRACE_EVERY 10 // ticks between two traces, the roof over the camera does not hurry

typedef int (*FastTrace_t)(void* pActor, const struct FVector* pEnd, const struct FVector* pStart, const struct FVector* pExtent, double unknown);

uintptr_t p_FastTrace = 0; // AActor::FastTrace

static float gRainStrength = 1.0f; // how much rain is over the camera, handed to the effect as is
#define RAIN_MEMORY_LOW 0x08800000u
#define RAIN_MEMORY_HIGH 0x0A000000u

static int IsGameMemory(uint32_t address, uint32_t size)
{
    return address >= RAIN_MEMORY_LOW && address <= RAIN_MEMORY_HIGH - size;
}
static int IsCameraUnderRoof(void* pActor, const struct FVector* pCamPos)
{
    struct FVector above;
    float extent[4];
    const uint32_t level = *(const uint32_t*)((const uint8_t*)pActor + 240);

    if (!p_FastTrace)
        return -1;

    if (!IsGameMemory(level, 64) || !IsGameMemory(*(const uint32_t*)level, 4))
        return -1;

    if (!IsGameMemory(*(const uint32_t*)(level + 44), 4))
        return -1;

    above.X = pCamPos->X;
    above.Y = pCamPos->Y;
    above.Z = pCamPos->Z + RAIN_TRACE_UP;
    extent[0] = 0.0f;
    extent[1] = 0.0f;
    extent[2] = 0.0f;
    extent[3] = 0.0f;
    return ((FastTrace_t)p_FastTrace)(pActor, &above, pCamPos, reinterpret_cast<const FVector*>(extent), 0.0) == 0;
}

void* gCurrentPlayerController;
int gFramesWithoutRain; // how long the rain of the game has not been drawn for

void _0fRAPlayerControllerETickf6KELevelTickWrapper(void* PlayerController, int a2, float a3)
{
    static void* prevPlayerController = 0;
    static uint32_t traceTick = 0;
    const struct FVector* pCamPos = (const struct FVector*)((uintptr_t)PlayerController + 0x1B0);
    if (!Ready()) return;
    Packet* data = &Data();
    if (!IsGameMemory((uint32_t)(uintptr_t)PlayerController, 0x1B0 + 32)) {
        gCurrentPlayerController = nullptr;
        data->enabled = 0;
        data->rain = 0.0f;
        return;
    }
    gCurrentPlayerController = PlayerController;

    if (PlayerController != prevPlayerController)
    {
        prevPlayerController = PlayerController;
        gFramesWithoutRain = 1000;
    }

    if (gFramesWithoutRain < 1000)
        ++gFramesWithoutRain;
    if (gFramesWithoutRain > RAIN_STALE_TICKS)
    {
        gRainStrength = 0.0f;
    }
    else if ((traceTick++ % RAIN_TRACE_EVERY) == 0 && PlayerController && IsGameMemory((uint32_t)(uintptr_t)PlayerController, 0x1B0 + 12))
    {
        const int underRoof = IsCameraUnderRoof(PlayerController, pCamPos);

        if (underRoof >= 0)
            gRainStrength = underRoof ? 0.0f : 1.0f;
    }

    data->enabled = 1;
    data->rain = gRainStrength;
    data->rainAddress = (uint32_t)(uintptr_t)&gRainStrength;
}

Matrix matrix;

void _0FIDrawRainP6LUStaticMeshP6PFLevelSceneNodeP6QFRenderInterfaceWrapper(void* a1, int a2, int a3)
{
    if (!Ready()) return;
    Packet* data = &Data();
    if (gCurrentPlayerController)
    {
        struct FVector* gCamPos = (struct FVector*)((uintptr_t)gCurrentPlayerController + 0x1B0);
        struct FRotator* gCamRot = (struct FRotator*)((uintptr_t)gCurrentPlayerController + 0x1B0 + sizeof(struct FRotator) + 4);

        float UnrealToRadians = (2.0f * 3.14159265359f) / 65536.0f;

        float SR = sinf(gCamRot->Roll * UnrealToRadians);
        float CR = cosf(gCamRot->Roll * UnrealToRadians);
        float SP = sinf(gCamRot->Pitch * UnrealToRadians);
        float CP = cosf(gCamRot->Pitch * UnrealToRadians);
        float SY = sinf(gCamRot->Yaw * UnrealToRadians);
        float CY = cosf(gCamRot->Yaw * UnrealToRadians);
        matrix.right.x = SR * SP * CY - CR * SY;
        matrix.right.y = SR * SP * SY + CR * CY;
        matrix.right.z = -SR * CP;
        matrix.flags = 0;
        matrix.up.x = -(CR * SP * CY + SR * SY);
        matrix.up.y = CY * SR - CR * SP * SY;
        matrix.up.z = CR * CP;
        matrix.pad1 = 0;
        matrix.at.x = CP * CY;
        matrix.at.y = CP * SY;
        matrix.at.z = SP;
        matrix.pad2 = 0;
        matrix.position.x = gCamPos->X;
        matrix.position.y = gCamPos->Y;
        matrix.position.z = gCamPos->Z;
        matrix.pad3 = 0;
        matrix.right.x = -matrix.right.x;
        matrix.right.y = -matrix.right.y;
        matrix.right.z = -matrix.right.z;

        const float fSpeedAdjuster = 0.05f;
        matrix.right.x *= fSpeedAdjuster;
        matrix.right.y *= fSpeedAdjuster;
        matrix.right.z *= fSpeedAdjuster;
        matrix.up.x *= fSpeedAdjuster;
        matrix.up.y *= fSpeedAdjuster;
        matrix.up.z *= fSpeedAdjuster;
        matrix.at.x *= fSpeedAdjuster;
        matrix.at.y *= fSpeedAdjuster;
        matrix.at.z *= fSpeedAdjuster;

        console::droplets::Camera(*data,matrix);
        gFramesWithoutRain = 0;
    }
    gCurrentPlayerController = 0;
}

void _0fIUGUIPageEDrawP6HUCanvasWrapper(void* a1, int* a2)
{
    if (!Ready()) return;
    Packet* data = &Data();
    data->enabled = 0;
}


std::array<SafetyMipsMid,4> hooks;
}
bool Essentials() {
    uintptr_t tick=pattern.get(0,"F0 00 86 8C 88 00 C7 8C 00 41 07 00",-4);
    uintptr_t draw=pattern.get(0,"B8 00 B4 AF ? ? ? ? ? ? ? ? AC 00 B1 AF C4 00 B7 AF",-4);
    uintptr_t menu=pattern.get(0,"C0 00 86 8C 38 00 B1 AF 25 88 80 00 08 00 C4 30 28 00 B4 E7",-4);
    uintptr_t phase=pattern.get(0,"5C 00 B2 AF 25 90 A0 00 18 00 A0 AF",-4);
    p_FastTrace=pattern.get(0,"30 00 A0 AF 00 60 80 44 34 00 A0 AF 40 00 AC E7 44 00 AC E7 48 00 AC E7 25 40 C0 00",-4);
    uintptr_t gu=pattern.get(0,"1C 00 B3 AF 20 00 43 34 ? ? ? ? 20 00 BF AF",-12);
    if (!tick || !draw || !menu || !phase || !p_FastTrace || !gu) return false;
    libguPacket=GetAbsoluteAddress(gu,0,0x14)+0x48;
    if (!console::portable::Begin()) return false;
    hooks[0]=safetymips::create_mid(tick,[](SafetyMipsContext& regs) {
        _0fRAPlayerControllerETickf6KELevelTickWrapper(reinterpret_cast<void*>(uintptr_t(regs.a0)),int(regs.a1),float(regs.f12));
    });
    // The tick callback calls AActor::FastTrace (game code) and keeps the full
    // VFPU save. The others use no VFPU; skipping its save also avoids restoring
    // prefix registers before returning to the game.
    safetymips::Options scalar; scalar.preserve=PSP_HOOK_SAVE_FPU;
    hooks[1]=safetymips::create_mid(draw,[](SafetyMipsContext& regs) {
        _0FIDrawRainP6LUStaticMeshP6PFLevelSceneNodeP6QFRenderInterfaceWrapper(reinterpret_cast<void*>(uintptr_t(regs.a0)),int(regs.a1),int(regs.a2));
    },scalar);
    hooks[2]=safetymips::create_mid(menu,[](SafetyMipsContext& regs) {
        _0fIUGUIPageEDrawP6HUCanvasWrapper(reinterpret_cast<void*>(uintptr_t(regs.a0)),reinterpret_cast<int*>(uintptr_t(regs.a1)));
    },scalar);
    hooks[3]=safetymips::create_mid(phase,[](SafetyMipsContext&) { Report(DisplayList()); },scalar);
    return console::portable::Finish()==0;
}
}
