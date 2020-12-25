#include "common.h"
#include "CDraw.h"

extern RsGlobalType* RsGlobal;
float*  CDraw::pfScreenAspectRatio;
float*  CDraw::pfScreenFieldOfView;

extern float fWideScreenWidthScaleDown;
extern float fCustomAspectRatioHor, fCustomAspectRatioVer;
extern float fEmergencyVehiclesFix;
extern bool bRestoreCutsceneFOV;
extern bool bDontTouchFOV;
extern bool* bIsInCutscene;
extern std::map<void*, float> FOVMods;

void CDraw::CalculateAspectRatio()
{
    if (!fCustomAspectRatioHor && !fCustomAspectRatioVer)
    {
        *pfScreenAspectRatio = (float)RsGlobal->MaximumWidth / (float)RsGlobal->MaximumHeight;
    }
    else
    {
        *pfScreenAspectRatio = fCustomAspectRatioHor / fCustomAspectRatioVer;
    }

    fWideScreenWidthScaleDown = (1.0f / 640.0f) / (*pfScreenAspectRatio / (4.0f / 3.0f));
}

void CDraw::SetFOV(float fFactor)
{
    fEmergencyVehiclesFix = 70.0f / fFactor;

    if ((*bIsInCutscene == true && bRestoreCutsceneFOV) || bDontTouchFOV)
        *pfScreenFieldOfView = fFactor;
    else
        *pfScreenFieldOfView = AdjustFOV(fFactor, *pfScreenAspectRatio);

    for (auto var : FOVMods)
    {
        *pfScreenFieldOfView *= var.second;
    }
}

CEXP void __cdecl GetCurrentFOV(float* out)
{
    *out = *CDraw::pfScreenFieldOfView;
}

CEXP void __cdecl SetFOVMultiplier(void* hash, float value)
{
    if (value <= 0.0f)
        value = 1.0f;

    if (!FOVMods.emplace(hash, value).second)
    {
        FOVMods[hash] = value;
    }
}

CEXP void __cdecl RemoveFOVMultiplier(void* hash)
{
    FOVMods.erase(hash);
}