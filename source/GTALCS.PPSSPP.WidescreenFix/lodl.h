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
    float radius;
    float heading;
    int r;
    int g;
    int b;
    int a;
    char* name;
} LodLights;

extern float fCoronaFarClip;
extern float fCoronaRadiusMultiplier;
extern void(*CCoronas__RegisterCorona)(int id, char r, char g, char b, char a, void* pos, char coronaType, char flareType, float radius, float farClip, float unk3, float unk4, char reflection, char LOScheck, char drawStreak, char flag4);
extern CVector* pCamPos;
extern uintptr_t CurrentTimeHoursOffset;
extern uintptr_t CurrentTimeMinutesOffset;

char CurrentTimeHours();
char CurrentTimeMinutes();
char GetIsTimeInRange(int hourA, int hourB);
CVector* GetCamPos();
void RegisterLODLights();