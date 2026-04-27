#pragma once

class CDraw
{
public:
    static float* pfScreenAspectRatio;
    static float* pfScreenFieldOfView;

    static void CalculateAspectRatio();
    static void SetFOV(float);
};

void __cdecl LegacyGetCurrentFOV(float* out);
void __cdecl LegacySetFOVMultiplier(void* hash, float value);
void __cdecl LegacyRemoveFOVMultiplier(void* hash);