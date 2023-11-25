#ifndef H_LODLIGHTS
#define H_LODLIGHTS
#include <stdlib.h>
#include "../../includes/pcsx2/injector.h"
#include "../../includes/pcsx2/patterns.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define corona_struct_size 144
#define CoronaLimitMax 10000
#define aCoronasSizeMax corona_struct_size * CoronaLimitMax

typedef struct CVector
{
    float x;
    float y;
    float z;
} CVector;

typedef struct {
    float x;
    float y;
    float z;
    float fCustomSizeMult;
    float fThisCoronaFarClip;
    float fHeading;
    int r;
    int g;
    int b;
    int a;
    int nCoronaShowMode;
    int nNoDistance;
    /*char* name;*/
} LodLights;

enum BlinkTypes
{
    DEFAULT,
    RANDOM_FLASHING,
    T_1S_ON_1S_OFF,
    T_2S_ON_2S_OFF,
    T_3S_ON_3S_OFF,
    T_4S_ON_4S_OFF,
    T_5S_ON_5S_OFF,
    T_6S_ON_4S_OFF
};

extern float fCoronaFarClip;
extern float fCoronaRadiusMultiplier;
extern void CCoronas__RegisterCorona(int id, unsigned char r, unsigned char g, unsigned char b, unsigned char a, void* pos, char coronaType, char flareType, float radius, float farClip, float unk3, float unk4, char reflection, char LOScheck, char drawStreak, char flag4, char flag5);
extern void(*CCoronas__RegisterCoronaINT)(unsigned int id, unsigned char r, unsigned char g, unsigned char b, unsigned char a, void* pos, unsigned char coronaType, unsigned char flareType, unsigned char reflection, unsigned char LOScheck, unsigned char drawStreak, unsigned char flag4, unsigned char flag5);
extern CVector* pCamPos;
extern uintptr_t CurrentTimeHoursOffset;
extern uintptr_t CurrentTimeMinutesOffset;
extern uintptr_t CTimer__m_snTimeInMillisecondsPauseModeOffset;
extern uintptr_t CTimer__ms_fTimeStepOffset;
extern void (*CSprite__FlushSpriteBuffer)();
extern void (*CCoronas__Render)();
extern void (*RslRenderStateSet)(int, int);
extern int (*CSprite__CalcScreenCoors)(CVector* in, CVector* out, float* outW, float* outH, uint8_t farClip);
extern void (*CSprite__RenderBufferedOneXLUSprite)();

void IncreaseCoronasLimit(size_t CoronaLimit);
char CurrentTimeHours();
char CurrentTimeMinutes();
uint32_t CTimer__m_snTimeInMillisecondsPauseMode();
float CTimer__ms_fTimeStep();
char GetIsTimeInRange(int hourA, int hourB);
CVector* GetCamPos();
int IsBlinkingNeeded(int BlinkType);
void RegisterLODLights();
void RenderLODLightsBuffered();
int IsSphereVisible(float radius, CVector* center);
#endif
