#pragma once
#include <math.h>
#include "../../includes/psp/injector.h"

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
extern void(*CCoronas__RegisterCorona)(int id, char r, char g, char b, char a, void* pos, char coronaType, char flareType, float radius, float farClip, float unk3, float unk4, char reflection, char LOScheck, char drawStreak, char flag4);
extern uintptr_t TheCamera;
extern CVector* pCamPos;
extern uintptr_t CDraw__ms_fNearClipZOffset;
extern uintptr_t CDraw__ms_fFarClipZOffset;
extern uintptr_t CurrentTimeHoursOffset;
extern uintptr_t CurrentTimeMinutesOffset;
extern uintptr_t CTimer__m_snTimeInMillisecondsPauseModeOffset;
extern uintptr_t CTimer__ms_fTimeStepOffset;

float CDraw__ms_fNearClipZ();
float CDraw__ms_fFarClipZ();
char CurrentTimeHours();
char CurrentTimeMinutes();
uint32_t CTimer__m_snTimeInMillisecondsPauseMode();
float CTimer__ms_fTimeStep();
char GetIsTimeInRange(int hourA, int hourB);
CVector* GetCamPos();
int IsBlinkingNeeded(int BlinkType);
void RegisterLODLights();