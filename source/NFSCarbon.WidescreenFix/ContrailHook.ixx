module;

#include <stdafx.h>

export module ContrailHook;

import ComVars;

namespace ContrailHook
{
    float* fContrailSpeed;
    bool bLimitContrailRate;
    float fContrailTargetFPS;
    float fContrailMinIntensity;
    float fContrailMaxIntensity;

    uintptr_t AddXenonEffect = 0x00754CD0;

    uint32_t ContrailFrameDelay = 1;

    uint32_t ContrailFC = 0;
    uint32_t RenderConnFC = 0;
    void AddXenonEffect_Contrail_Hook(void* piggyback_fx, void* spec, bMatrix4* mat, bVector4* vel, float intensity)
    {
        float newintensity = fContrailMaxIntensity;

        double carspeed = ((sqrt((*vel).x * (*vel).x + (*vel).y * (*vel).y + (*vel).z * (*vel).z) - *fContrailSpeed)) / *fContrailSpeed;
        newintensity = std::lerp(fContrailMinIntensity, fContrailMaxIntensity, carspeed);
        if (newintensity > fContrailMaxIntensity)
            newintensity = fContrailMaxIntensity;


        if (!bLimitContrailRate)
            return reinterpret_cast<void(*)(void*, void*, bMatrix4*, bVector4*, float)>(AddXenonEffect)(piggyback_fx, spec, mat, vel, newintensity);

        // TODO: this could use actual timers instead of a simple framecounter to make it better, but it is good enough for this purpose
        if ((ContrailFC + ContrailFrameDelay) <= RenderConnFC)
        {
            if (ContrailFC != RenderConnFC)
            {
                ContrailFC = RenderConnFC;
                reinterpret_cast<void(*)(void*, void*, bMatrix4*, bVector4*, float)>(AddXenonEffect)(piggyback_fx, spec, mat, vel, newintensity);
            }
        }
        RenderConnFC++;
    }
}

export void InitContrailHook()
{
    CIniReader iniReader("");
    bool bDisableContrails = iniReader.ReadInteger("GRAPHICS", "DisableContrails", 0) != 0;
    float cfgContrailSpeed = iniReader.ReadFloat("Contrails", "ContrailSpeed", 35.0f);
    ContrailHook::bLimitContrailRate = iniReader.ReadInteger("Contrails", "LimitContrailRate", 1) != 0;
    ContrailHook::fContrailTargetFPS = iniReader.ReadFloat("Contrails", "ContrailTargetFPS", 30.0f);
    ContrailHook::fContrailMinIntensity = iniReader.ReadFloat("Contrails", "ContrailMinIntensity", 0.1f);
    ContrailHook::fContrailMaxIntensity = iniReader.ReadFloat("Contrails", "ContrailMaxIntensity", 0.75f);

    if (bDisableContrails)
    {
        uintptr_t loc_7E1351 = reinterpret_cast<uintptr_t>(hook::pattern("8A 87 9C 01 00 00 84 C0").get_first(0)) + 0xD0;
        uintptr_t loc_7E13A9 = loc_7E1351 + 0x58;

        injector::MakeJMP(loc_7E1351, loc_7E13A9, true);
    }
    else if (ContrailHook::bLimitContrailRate && (ContrailHook::fContrailTargetFPS > 0.0f))
    {
        uintptr_t loc_6B4D30 = reinterpret_cast<uintptr_t>(hook::pattern("D9 05 ? ? ? ? B9 ? ? ? ? D8 44 24 14 D9 5C 24 14").get_first(0)) + 0x35;
        static float fGameTargetFPS = 1.0f / **reinterpret_cast<float**>(loc_6B4D30);

        if (fGameTargetFPS != ContrailHook::fContrailTargetFPS)
        {
            // TODO: use a timer instead of this because of rounding errors!
            float fContrailFrameDelay = (fGameTargetFPS / ContrailHook::fContrailTargetFPS);
            ContrailHook::ContrailFrameDelay = static_cast<uint32_t>(round(fContrailFrameDelay));

            uintptr_t loc_7E139A = reinterpret_cast<uintptr_t>(hook::pattern("8A 87 9C 01 00 00 84 C0").get_first(0)) + 0x119;
            ContrailHook::AddXenonEffect = static_cast<uintptr_t>(injector::GetBranchDestination(loc_7E139A));
            injector::MakeCALL(loc_7E139A, ContrailHook::AddXenonEffect_Contrail_Hook, true);
        }
    }
    else
    {
        uintptr_t loc_7E136D = reinterpret_cast<uintptr_t>(hook::pattern("8A 87 9C 01 00 00 84 C0").get_first(0)) + 0xEC;
        uintptr_t loc_7E1372 = loc_7E136D + 5;

        injector::WriteMemory<float>(loc_7E136D + 1, ContrailHook::fContrailMaxIntensity, true);
        injector::WriteMemory<float>(loc_7E1372 + 1, ContrailHook::fContrailMinIntensity, true);
    }

    uintptr_t loc_7E1364 = reinterpret_cast<uintptr_t>(hook::pattern("8A 87 9C 01 00 00 84 C0").get_first(0)) + 0xE3;
    ContrailHook::fContrailSpeed = *reinterpret_cast<float**>(loc_7E1364 + 2);
    DWORD oldprotect;
    injector::UnprotectMemory(ContrailHook::fContrailSpeed, sizeof(float), oldprotect);
    *ContrailHook::fContrailSpeed = cfgContrailSpeed;
}