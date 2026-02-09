#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <float.h>

#include "../../includes/pcsx2/pcsx2f_api.h"
#include "../../includes/pcsx2/log.h"
#include "../../includes/pcsx2/injector.h"
#include "../../includes/pcsx2/inireader.h"
#include "../../includes/pcsx2/patterns.h"

int CompatibleCRCList[] = { 0x42A9C4EC, 0x7A9D67B8, 0x77B4F13C, 0xB1AC3BEB };
int CompatibleElfCRCList[] = { 0x42A9C4EC, 0x1118ACD0, 0x7A9D67B8, 0xB73CDCFA, 0x77B4F13C, 0xA4334B91, 0xB1AC3BEB, 0xB2D44C6C };
int PCSX2Data[PCSX2Data_Size] = { 1 };
char OSDText[OSDStringNum][OSDStringSize] = { {1} };
char PluginData[MaxIniSize] = { 1 };

struct ScreenX
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fInternalWidth;
    float fInternalHeight;
    float fAspectRatio;
    float f3DScale;
    float fHudScale;
    float fHudOffset;
    float fHudOffset2;
    float fRadarOffset;
    float fHudAspectRatioConstraint;
    float fWidescreenHudOffset;
    float fWidescreenHudOffsetPhone;
    float fWidescreenHudOffsetRadar;
} Screen;

void HudScaleX(int a1, int a2)
{
    a2 = (int)(Screen.fInternalWidth * Screen.fHudScale + 0.5f);

    *(int*)(a1 + 0xD0) = a2;
    *(float*)(a1 + 0xD8) = (float)a2 * 0.0015625f;
    *(float*)(a1 + 0xE0) = 1.0f / (float)a2;
}

void sub_23C638(const float* base, const float* modifier, unsigned int alignment_mode, float* result)
{
    if (!base || !modifier || !result)
    {
        return;
    }

    float width_scale = base[5] * modifier[5];
    float height_scale = base[6] * modifier[6];

    result[5] = width_scale;
    result[6] = height_scale;

    result[3] = modifier[3] * width_scale;
    result[4] = modifier[4] * height_scale;

    result[0] = base[0] * modifier[0];

    float x;

    switch (alignment_mode)
    {
        case 0: case 3: case 6:     // left / start aligned
            x = modifier[1] * width_scale + base[1];
            break;

        case 1: case 4: case 7:     // center aligned
            x = ((base[3] - result[3]) * 0.5f) + (modifier[1] * width_scale) + base[1];
            break;

        case 2: case 5: case 8:     // right / end aligned
            x = (base[3] + base[1]) - result[3] + (modifier[1] * width_scale);
            break;

        default:
            return;
    }

    result[1] = x + Screen.fHudOffset;

    float y;

    switch (alignment_mode)
    {
        case 0: case 1: case 2:     // top aligned
            y = modifier[2] * height_scale + base[2];
            break;

        case 3: case 4: case 5:     // middle / vertically centered
            y = ((base[4] - result[4]) * 0.5f) + (modifier[2] * height_scale) + base[2];
            break;

        case 6: case 7: case 8:     // bottom aligned
            y = (base[4] + base[2]) - result[4] + (modifier[2] * height_scale);
            break;

        default:
            return;
    }

    result[2] = y;
}

typedef struct
{
    uint32_t values[6];
} HUDIdentifier;

HUDIdentifier HUDIdentifier_create(float* input1)
{
    HUDIdentifier id;
    for (int i = 0; i < 6; ++i)
    {
        id.values[i] = *(uint32_t*)&input1[i + 1];
    }
    return id;
}

int HUDIdentifier_equals(HUDIdentifier a, HUDIdentifier b)
{
    for (int i = 0; i < 6; ++i)
    {
        if (i == 0)
        {
            float fa = *(float*)&a.values[i];
            float fb = *(float*)&b.values[i];
            if (fabs(fa - fb) >= 0.001f)
                return 0;
        }
        else if (a.values[i] != b.values[i])
            return 0;
    }
    return 1;
}

HUDIdentifier Silhouette = { { 0x3F579998, 0x3F340001, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000 } };
HUDIdentifier SilhouetteBars = { { 0x3F65FFFE, 0x3F3C0001, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000 } };
HUDIdentifier Badge = { { 0x3F473332, 0x3F3F6DB7, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000 } };
HUDIdentifier WeaponIcon = { { 0x3EE3FFFE, 0x3CB6DB6E, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000 } };
HUDIdentifier WeaponAmmoLeft = { { 0x3F3DFFFE, 0x3D8DB6DC, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000 } };
HUDIdentifier WeaponAmmoRight = { { 0x3F69FFFE, 0x3D8DB6DC, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000 } };
HUDIdentifier Phone = { { 0xBDCCCCCC, 0x3D924925, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000 } };

#if 0
enum KeyCodes
{
    VK_KEY_A = 0x41,
    VK_KEY_D = 0x44,
    VK_KEY_S = 0x53,
    VK_KEY_W = 0x57,
};

char KeyboardState[StateNum][StateSize] = { {1} };

char GetAsyncKeyState(int vKey)
{
    return KeyboardState[CurrentState][vKey];
}
#endif

void WidescreenHUD(float* input1, float* input2, int alignmentMode, float* output)
{
    if (!input1 || !input2 || !output) return;

    HUDIdentifier current = HUDIdentifier_create(input1);

    #if 0
    // Debug: Track processed elements (simplified for C)
    static char processedElements[100][256];  // Fixed array for strings
    static int processedCount = 0;
    static int currentElementIndex = 0;
    static char currentSelectedElement[256] = "";

    // Simple string representation (for logging)
    char logString[256];
    npf_snprintf(logString, sizeof(logString), "HUDIdentifier {0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X}",
                 current.values[0], current.values[1], current.values[2],
                 current.values[3], current.values[4], current.values[5]);

    // Check if already processed
    int alreadyExists = 0;
    for (int i = 0; i < processedCount; ++i)
    {
        if (strcmp(processedElements[i], logString) == 0)
        {
            alreadyExists = 1;
            break;
        }
    }
    if (!alreadyExists && processedCount < 100)
    {
        strcpy(processedElements[processedCount], logString);  // Direct copy to fixed array
        processedCount++;
    }
    #endif

    // Scale factors
    output[5] = input1[5] * input2[5];  // Width scale
    output[6] = input1[6] * input2[6];  // Height scale
    output[3] = input2[3] * output[5];  // Scaled width component
    output[4] = input2[4] * output[6];  // Scaled height component
    output[0] = input1[0] * input2[0];  // Base scale

    float xPos;
    switch (alignmentMode)
    {
        case 0: case 3: case 6:  // Left/top alignment
            xPos = (input2[1] * output[5]) + input1[1];
            break;
        case 1: case 4: case 7:  // Center alignment
            xPos = ((input1[3] - output[3]) * 0.5f) + (input2[1] * output[5]) + input1[1];
            break;
        case 2: case 5: case 8:  // Right/bottom alignment
            xPos = (input1[3] + input1[1]) - output[3] + (input2[1] * output[5]);
            break;
        default:
            return;
    }
    output[1] = xPos + Screen.fHudOffset;  // Apply HUD offset

    // Apply specific offsets for identified elements
    if (HUDIdentifier_equals(current, Silhouette) || HUDIdentifier_equals(current, SilhouetteBars) ||
        HUDIdentifier_equals(current, Badge) || HUDIdentifier_equals(current, WeaponIcon) ||
        HUDIdentifier_equals(current, WeaponAmmoLeft) || HUDIdentifier_equals(current, WeaponAmmoRight))
    {
        output[1] += Screen.fWidescreenHudOffset;
    }
    else if (HUDIdentifier_equals(current, Phone))
    {
        output[1] -= Screen.fWidescreenHudOffsetPhone;
    }
    #if 0
    else
    {
        // Debug: Key press handling for cycling elements
        static char wasAddDown = 0;
        char isAddDown = GetAsyncKeyState(VK_KEY_W);  // Cycle forward
        if (wasAddDown && !isAddDown)
        {
            if (processedCount > 0)
            {
                currentElementIndex = (currentElementIndex + 1) % processedCount;
                strcpy(currentSelectedElement, processedElements[currentElementIndex]);
                logger.WriteF("Current element: %s", currentSelectedElement);
            }
        }
        wasAddDown = isAddDown;

        static char wasSubtractDown = 0;
        char isSubtractDown = GetAsyncKeyState(VK_KEY_S);  // Cycle backward
        if (wasSubtractDown && !isSubtractDown)
        {
            if (processedCount > 0)
            {
                currentElementIndex = (currentElementIndex - 1 + processedCount) % processedCount;
                strcpy(currentSelectedElement, processedElements[currentElementIndex]);
                logger.WriteF("Current element: %s", currentSelectedElement);
            }
        }
        wasSubtractDown = isSubtractDown;

        // Apply offset to the currently selected element
        if (processedCount > 0 && strcmp(logString, processedElements[currentElementIndex]) == 0)
        {
            output[1] += Screen.fHudOffset;
        }
    }
    #endif

    switch (alignmentMode)
    {
        case 0: case 1: case 2:  // Top alignment
            output[2] = (input2[2] * output[6]) + input1[2];
            break;
        case 3: case 4: case 5:  // Middle alignment
            output[2] = ((input1[4] - output[4]) * 0.5f) + (input2[2] * output[6]) + input1[2];
            break;
        case 6: case 7: case 8:  // Bottom alignment
            output[2] = (input1[4] + input1[2]) - output[4] + (input2[2] * output[6]);
            break;
    }
}

float fOriginalRadarOffset = 0.0f;
void (*fnsub_2517A8)(int a1, int a2);
void sub_2517A8(int a1, int a2)
{
    if (!fOriginalRadarOffset)
        fOriginalRadarOffset = *(float*)(a1 + 36);
    *(float*)(a1 + 36) = fOriginalRadarOffset + Screen.fRadarOffset - Screen.fWidescreenHudOffsetRadar;
}

float ClampHudAspectRatio(float value, float screenAspect, float minAspect, float maxAspect)
{
    if (value < minAspect) return minAspect;
    if (value > maxAspect) return maxAspect;
    return value;
}

float ParseWidescreenHudOffset(char* input)
{
    if (!input || strlen(input) == 0) return -1.0f;
    char* colon = strchr(input, ':');
    if (!colon) return -1.0f;
    *colon = '\0';  // Split string
    float num1 = atof(input);
    float num2 = atof(colon + 1);
    if (num2 == 0.0f) return -1.0f;
    return num1 / num2;
}

float BlipsOffset = 195.0f;
void (*fnsub_268058)(void* a1, void* a2);
void TransformMapToScreen(void* a1, void* a2)
{
    BlipsOffset = 195.0f + ((640.0f / ((640.0f / 480.0f) / Screen.fAspectRatio)) - 640.0f) / 2.0f;
    fnsub_268058(a1, a2);
}

void TransformMapToScreen_original(void* a1, void* a2)
{
    BlipsOffset = 195.0f;
    fnsub_268058(a1, a2);
}

int CurrentCaseName = -1;
int CurrentMissionName = -1;
void* (*fnsub_1FD738)(void* a1);
void* sub_1FD738(void* a1)
{
    void* ret = fnsub_1FD738(a1);
    CurrentCaseName = *(int*)((uintptr_t)a1 + 0x1078);
    CurrentMissionName = *(int*)((uintptr_t)a1 + 0x107C);
    return ret;
}

enum eCaseNames
{
    Mision0,
    Intro,
    BC01,
    BC02,
    BC03,
    BC04,
    EndGame,
    CIKingMission,
    CIMadamMission,
    CICabbieMission,
    FC,
    RedmanBonus,
    Police_Challenges,
    StreetRacing,
    SC03,
};

enum eMissionNames_BC03
{
    BC3_Prologue,
    BC3_M0F,
    BC3_M1,
    BC3_M1F,
    BC3_M2,
    BC3_M2F,
    BC3_M4,
    BC3_M4F,
    BC3_M5,
    BC3_M5F,
    BC3_M6,
    BC3_M7,
    BC3_M8,
};

typedef struct
{
    int caseName;
    int missionName;
} MissionPair;

MissionPair missionList[] =
{
    { BC03, BC3_M6 },
};

int GameNeeds30FPS()
{
    for (size_t i = 0; i < sizeof(missionList) / sizeof(MissionPair); ++i)
    {
        if (CurrentCaseName == missionList[i].caseName && CurrentMissionName == missionList[i].missionName)
        {
            return 1;
        }
    }
    return 0;
}

void init()
{
    //logger.SetBuffer(OSDText, sizeof(OSDText) / sizeof(OSDText[0]), sizeof(OSDText[0]));
    //logger.Write("Loading TrueCrimeNewYorkCity.PCSX2F.WidescreenFix...");

    inireader.SetIniPath((char*)PluginData + sizeof(uint32_t), *(uint32_t*)PluginData);

    int SkipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 1);
    char buffer[100];
    char* constraintStr = inireader.ReadString("MAIN", "HudAspectRatioConstraint", "auto", buffer, sizeof(buffer));
    Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(constraintStr);
    int bEnable60FPS = inireader.ReadInteger("MAIN", "Enable60FPS", 0);

    //logger.WriteF("Screen.fHudAspectRatioConstraint: %f", Screen.fHudAspectRatioConstraint);

    uintptr_t ptr_100350 = pattern.get(0, "2D 20 40 02 ? ? ? ? 2D 40 20 02", -4);
    if (ptr_100350 != 0)
    {
        if (SkipIntro)
        {
            //logger.Write("Skipping intro...");
            injector.MakeNOP(ptr_100350);
        }
        return;
    }

    if (bEnable60FPS)
    {
        //uintptr_t ptr_4A1C3C = pattern.get(0, "70 00 B3 7F ? ? ? ? 60 00 B4 7F ? ? ? ? A0 00 B0 7F", -4);
        //injector.WriteInstr(ptr_4A1C3C, addiu(s1, zero, 2));

        uintptr_t ptr_4A1C3C = pattern.get(0, "70 00 B3 7F ? ? ? ? 60 00 B4 7F ? ? ? ? A0 00 B0 7F", -4);
        MakeInlineWrapperWithNOP(ptr_4A1C3C,
            jal((uintptr_t)GameNeeds30FPS),
            nop(),
            beq(v0, zero, 2),
            addiu(s1, zero, 2),
            addiu(s1, zero, 4)
        );

        //uintptr_t ptr_4A1C54 = pattern.get(0, "50 00 BF FF 40 00 A0 AF ? ? ? ? ? ? ? ? ? ? ? ? 00 00 00 00", -4);
        //injector.WriteInstr(ptr_4A1C54, li(s4, 1));

        uintptr_t ptr_4A1C54 = pattern.get(0, "50 00 BF FF 40 00 A0 AF ? ? ? ? ? ? ? ? ? ? ? ? 00 00 00 00", -4);
        MakeInlineWrapperWithNOP(ptr_4A1C54,
            jal((uintptr_t)GameNeeds30FPS),
            nop(),
            beq(v0, zero, 2),
            addiu(s4, zero, 1),
            addiu(s4, zero, 2)
        );

        //uintptr_t ptr_4A1CC4 = pattern.get(0, "10 00 A3 FB ? ? 12 0C 02 00 04 24", 8);
        //injector.WriteInstr(ptr_4A1CC4, addiu(a0, zero, 1));

        uintptr_t ptr_4A1CC0 = pattern.get(0, "10 00 A3 FB ? ? 12 0C 02 00 04 24", 4);
        uintptr_t sub_4AB5E8 = injector.GetBranchDestination(ptr_4A1CC0);
        injector.MakeNOP(ptr_4A1CC0 + 4);
        MakeInlineWrapperWithNOP(ptr_4A1CC0,
            jal((uintptr_t)GameNeeds30FPS),
            nop(),
            beq(v0, zero, 2),
            addiu(a0, zero, 1),
            addiu(a0, zero, 2),
            jal(sub_4AB5E8),
            nop()
        );

        uintptr_t ptr_1FE0C4 = pattern.get(0, "C0 26 00 AE ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? C3 01 03 92 ? ? ? ? BC 00 02 8E", -20);
        fnsub_1FD738 = (void* (*)(void*))injector.MakeJAL(ptr_1FE0C4, (uintptr_t)sub_1FD738);
    }

    uint32_t DesktopSizeX = PCSX2Data[PCSX2Data_DesktopSizeX];
    uint32_t DesktopSizeY = PCSX2Data[PCSX2Data_DesktopSizeY];
    Screen.nWidth = PCSX2Data[PCSX2Data_WindowSizeX];
    Screen.nHeight = PCSX2Data[PCSX2Data_WindowSizeY];
    Screen.fWidth = (float)Screen.nWidth;
    Screen.fHeight = (float)Screen.nHeight;
    uint32_t IsFullscreen = PCSX2Data[PCSX2Data_IsFullscreen];
    uint32_t AspectRatioSetting = PCSX2Data[PCSX2Data_AspectRatioSetting];

    if (IsFullscreen || !Screen.nWidth || !Screen.nHeight)
    {
        Screen.nWidth = DesktopSizeX;
        Screen.nHeight = DesktopSizeY;
    }

    switch (AspectRatioSetting)
    {
        case RAuto4_3_3_2: //not implemented
            //if (GSgetDisplayMode() == GSVideoMode::SDTV_480P)
            //    AspectRatio = 3.0f / 2.0f;
            //else
            Screen.fAspectRatio = 4.0f / 3.0f;
            break;
        case R4_3:
            Screen.fAspectRatio = 4.0f / 3.0f;
            break;
        case R16_9:
            Screen.fAspectRatio = 16.0f / 9.0f;
            break;
        case Stretch:
        default:
            Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
            break;
    }

    //logger.WriteF("Resolution: %dx%d", Screen.nWidth, Screen.nHeight);
    //logger.WriteF("Aspect Ratio: %f", Screen.fAspectRatio);

    Screen.fInternalWidth = 640.0f;
    Screen.fInternalHeight = 480.0f;

    uintptr_t ptr_204DB0 = pattern.get(0, "CC 3A 01 3C CD CC 21 34 00 00 81 44 ? ? ? ? 03 01 02 24", 0);
    if (ptr_204DB0)
    {
        Screen.fInternalWidth = 512.0f;
        Screen.fInternalHeight = 384.0f;
    }

    Screen.fHudScale = (Screen.fInternalWidth / Screen.fInternalHeight) / Screen.fAspectRatio;
    Screen.fHudOffset = (1.0f / (448.0f * (4.0f / 3.0f))) * (((448.0f * Screen.fAspectRatio) - 448.0f * (4.0f / 3.0f)) / 2.0f);
    Screen.fHudOffset2 = (Screen.fInternalWidth - Screen.fInternalWidth * Screen.fHudScale) / 2.0f;
    Screen.f3DScale = (((Screen.fInternalWidth / Screen.fInternalHeight)) / (Screen.fAspectRatio));
    Screen.fRadarOffset = -5.0f + Screen.fHudOffset2;
    Screen.fWidescreenHudOffset = Screen.fHudOffset;
    Screen.fWidescreenHudOffsetPhone = Screen.fWidescreenHudOffset;
    Screen.fWidescreenHudOffsetRadar = Screen.fHudOffset2;
    if (Screen.fHudAspectRatioConstraint >= 0.0f)
    {
        float value = Screen.fHudAspectRatioConstraint;
        if (value < 0.0f || value > (32.0f / 9.0f))
            Screen.fWidescreenHudOffset = value;
        else
        {
            value = ClampHudAspectRatio(value, Screen.fAspectRatio, 4.0f / 3.0f, 32.0f / 9.0f);
            Screen.fWidescreenHudOffset = (1.0f / (448.0f * (4.0f / 3.0f))) * (((448.0f * value) - 448.0f * (4.0f / 3.0f)) / 2.0f);
            Screen.fWidescreenHudOffsetRadar = (Screen.fInternalWidth - Screen.fInternalWidth * ((Screen.fInternalWidth / Screen.fInternalHeight) / value)) / 2.0f;
        }
    }

    //logger.WriteF("Screen.fHudOffset: %f", Screen.fHudOffset);
    //logger.WriteF("Screen.fWidescreenHudOffset: %f", Screen.fWidescreenHudOffset);
    //logger.WriteF("Screen.fWidescreenHudOffsetPhone: %f", Screen.fWidescreenHudOffsetPhone);
    //logger.WriteF("Screen.fWidescreenHudOffsetRadar: %f", Screen.fWidescreenHudOffsetRadar);

    //Turn off native widescreen option
    uintptr_t ptr_204EBC = pattern.get(0, "2B 28 05 00 20 00 B0 7F 10 00 B1 7F", 0);
    injector.WriteInstr(ptr_204EBC, sltu(a1, zero, zero));

    //3D Scaling
    uintptr_t ptr_204DD8 = pattern.get(0, "CD CC 21 34 00 00 81 44 ? ? ? ? ? ? ? ? ? ? ? ? 9A 99 21 34", -4);
    injector.MakeInlineLUIORI(ptr_204DD8, 0.0015625f * Screen.fHudScale);

    //Hud Scaling
    uintptr_t ptr_2346E8 = pattern.get(0, "00 00 85 44 00 00 00 00 20 00 80 46 ? ? ? ? 00 08 81 44 ? ? ? ? CD CC 21 34", 0);
    injector.MakeJMPwNOP(ptr_2346E8, (uintptr_t)HudScaleX);

    //Hud Positioning
    uintptr_t ptr_23C638 = pattern.get(0, "00 00 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 09 00 C9 2C", -8);
    //injector.MakeJMPwNOP(ptr_23C638, (uintptr_t)sub_23C638);
    injector.MakeJMPwNOP(ptr_23C638, (uintptr_t)WidescreenHUD);

    //Subtitles
    uintptr_t ptr_20BA48 = pattern.get(0, "00 20 81 44 ? ? ? ? CD CC 21 34 00 08 81 44", 0);
    float subOffsetDiff = Screen.fAspectRatio / (4.0f / 3.0f);
    MakeInlineWrapper(ptr_20BA48,
        mtc1(at, f4),
        lui(at, HIWORD(subOffsetDiff)),
        ori(at, at, LOWORD(subOffsetDiff)),
        mtc1(at, f1),
        muls(f4, f4, f1)
    );

    // 3D models (armoury etc)
    uintptr_t ptr_255E88 = pattern.get(0, "15 26 A5 34 02 00 01 46 ? ? ? ? 44 00 40 E6", 0);
    MakeInlineWrapperWithNOP(ptr_255E88,
        ori(a1, a1, 0x2615),
        muls(f0, f0, f1),
        lui(at, HIWORD(Screen.fHudOffset2)),
        ori(at, at, LOWORD(Screen.fHudOffset2)),
        mtc1(at, f30),
        adds(f0, f0, f30)
    );

    // Menu map
    uintptr_t ptr_265744 = pattern.get(0, "24 01 00 46 00 20 02 44 50 00 A2 A7", -4);
    MakeInlineWrapper(ptr_265744,
        add(f0, f0, f21),
        lui(at, HIWORD(Screen.fHudOffset2)),
        ori(at, at, LOWORD(Screen.fHudOffset2)),
        mtc1(at, f30),
        adds(f0, f0, f30)
    );

    // Menu blips
    uintptr_t ptr_265224 = pattern.get(0, "00 00 81 44 00 10 A2 48 00 00 03 44 00 18 A3 48 44 10 43 4A", -12);
    fnsub_268058 = (void(*)(void*, void*))injector.MakeJAL(ptr_265224, (uintptr_t)TransformMapToScreen);
    uintptr_t ptr_266758 = pattern.get(0, "DA 4F 21 34 00 00 81 44 00 08 82 44 ? ? ? ? 43 16 21 34 00 20 81 44 42 08 00 46 ? ? ? ? 00 18 81 44 89 1F 02 70 00 00 83 44 ? ? ? ? 00 10 81 44 02 00 04 46 42 08 03 46 02 00 02 46 A4 08 00 46 00 10 04 44", -12);
    fnsub_268058 = (void(*)(void*, void*))injector.MakeJAL(ptr_266758, (uintptr_t)TransformMapToScreen_original);
    uintptr_t ptr_266844 = pattern.get(0, "DA 4F 21 34 00 00 81 44 00 08 82 44 ? ? ? ? 43 16 21 34 00 20 81 44 42 08 00 46 ? ? ? ? 00 18 81 44 89 1F 02 70 00 00 83 44 ? ? ? ? 00 10 81 44 02 00 04 46 42 08 03 46 02 00 02 46 A4 08 00 46 00 10 06 44", -12);
    fnsub_268058 = (void(*)(void*, void*))injector.MakeJAL(ptr_266844, (uintptr_t)TransformMapToScreen_original);
    uintptr_t ptr_267F74 = pattern.get(0, "00 08 A2 48 ? ? ? ? 00 00 81 44 00 00 00 00 00 00 02 44", -8);
    fnsub_268058 = (void(*)(void*, void*))injector.MakeJAL(ptr_267F74, (uintptr_t)TransformMapToScreen);

    uintptr_t ptr_268094 = pattern.get(0, "00 08 81 44 00 10 03 44 00 10 A7 48", -4);
    uintptr_t BlipsOffsetAddr = (uintptr_t)&BlipsOffset;
    MakeInlineWrapperWithNOP(ptr_268094,
        lui(at, HIWORD(BlipsOffsetAddr)),
        ori(at, at, LOWORD(BlipsOffsetAddr)),
        lwc1(f1, at, 0)
    );

    // Radar
    uintptr_t ptr_2517A8 = pattern.get(0, "20 02 B6 7F 10 02 B7 7F ? ? ? ? 80 02 B0 7F", -12);
    fnsub_2517A8 = (void (*)(int, int))injector.MakeTrampoline(ptr_2517A8, (uintptr_t)sub_2517A8);

    uintptr_t ptr_2478FC = pattern.get(0, "A4 00 00 46 00 10 02 44 24 08 00 46 00 00 03 44 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? FF FF 21 34", -4);
    float fRadarDiscOffset = Screen.fRadarOffset - Screen.fWidescreenHudOffsetRadar;
    MakeInlineWrapper(ptr_2478FC,
        add(f1, f1, f3),
        lui(at, HIWORD(fRadarDiscOffset)),
        ori(at, at, LOWORD(fRadarDiscOffset)),
        mtc1(at, f30),
        adds(f1, f1, f30)
    );

    uintptr_t ptr_2519D0 = pattern.get(0, "00 39 07 00 ? ? ? ? ? ? ? ? 00 00 81 44 C0 01 A8 AF", 0);
    injector.WriteInstr(ptr_2519D0, sll(a3, a3, 8)); //Radar Clip Area

    //logger.Write("TrueCrimeNewYorkCity.PCSX2F.WidescreenFix loaded");
    //logger.ClearLog();
}

int main()
{
    return 0;
}
