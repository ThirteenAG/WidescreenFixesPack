module;

#include <stdafx.h>

export module NOSTrailFix;

import ComVars;

struct bVector3
{
    float x;
    float y;
    float z;
    float pad;
};

#pragma runtime_checks( "", off )
float NOSTrailScalar = 2.0f;
float NOSTrailPositionScalar = 0.3f;
bMatrix4 carbody_nos;

uintptr_t CarRenderInfo_RenderFlaresOnCar_Addr = 0x007CBC40;

void __stdcall CarRenderInfo_RenderFlaresOnCar_Hook(void* eView, bVector3* position, bMatrix4* body_matrix, int force_light_state, int reflexion, int renderFlareFlags, int nonplayercar)
{
    void* thethis = 0;
    _asm mov thethis, ecx
    memcpy(&carbody_nos, body_matrix, sizeof(bMatrix4));

    float pos_scale = (NOSTrailScalar * NOSTrailPositionScalar);
    if (pos_scale < 1.0f)
        pos_scale = 1.0f;

    carbody_nos.v0.x *= pos_scale;
    carbody_nos.v0.y *= pos_scale;
    carbody_nos.v0.z *= pos_scale;

    carbody_nos.v2.x *= pos_scale;
    carbody_nos.v2.y *= pos_scale;

    return reinterpret_cast<void(__thiscall*)(void*, void*, bVector3*, bMatrix4*, int, int, int, int)>(CarRenderInfo_RenderFlaresOnCar_Addr)(thethis, eView, position, &carbody_nos, force_light_state, reflexion, renderFlareFlags, nonplayercar);
}

bVector3* WorldPos1;
bVector3* WorldPos2;
bVector3* NOSFlarePos;

uintptr_t NOSTrailCave2Exit = 0x007CCD30;
void __declspec(naked) NOSTrailCave2()
{
    _asm
    {
        mov WorldPos1, edx
        mov WorldPos2, esi
        lea edx, [esp + 0x40]
        mov NOSFlarePos, edx
    }

    (*NOSFlarePos).x = ((*WorldPos1).x - (*WorldPos2).x) * NOSTrailScalar;
    (*NOSFlarePos).y = ((*WorldPos1).y - (*WorldPos2).y) * NOSTrailScalar;
    (*NOSFlarePos).z = ((*WorldPos1).z - (*WorldPos2).z) * NOSTrailScalar;

    _asm
    {
        xor eax, eax
        jmp NOSTrailCave2Exit
    }
}
#pragma runtime_checks( "", restore )

class NOSTrailFix
{
public:
    NOSTrailFix()
    {
        WFP::onInitEventAsync() += []()
        {
            CIniReader iniReader("");
            bool bFixNOSTrailLength = iniReader.ReadInteger("NOSTrail", "FixNOSTrailLength", 1) == 1;
            bool bFixNOSTrailPosition = iniReader.ReadInteger("NOSTrail", "FixNOSTrailPosition", 0) != 0;
            static float fCustomNOSTrailLength = iniReader.ReadFloat("NOSTrail", "CustomNOSTrailLength", 1.0f);
            NOSTrailPositionScalar = iniReader.ReadFloat("NOSTrail", "NOSTrailPositionScalar", 0.3f);

            if (bFixNOSTrailLength)
            {
                static int TargetRate = 60;

                if (SimRate)
                    TargetRate = SimRate;

                constexpr float NOSTargetFPS = 60.0f; // original FPS we're targeting from. Consoles target 60 but run at 30, hence have longer trails than PC. Targeting 60 is smarter due to less issues with shorter trails. Use SimRate -2 to get the same effect as console versions.
                NOSTrailScalar = (TargetRate / NOSTargetFPS) * fCustomNOSTrailLength;

                uintptr_t loc_7CCD28 = reinterpret_cast<uintptr_t>(hook::pattern("EB 06 8D 9B 00 00 00 00 40 89 44 24 24").get_first(0));
                injector::MakeJMP(loc_7CCD28, NOSTrailCave2, true);
                NOSTrailCave2Exit = loc_7CCD28 + 8;

                if (bFixNOSTrailPosition)
                {
                    uintptr_t loc_7CCDD6 = reinterpret_cast<uintptr_t>(hook::pattern("D9 44 24 40 6A 01 D8 4C 24 1C").get_first(0)) + 0x5E;
                    CarRenderInfo_RenderFlaresOnCar_Addr = static_cast<uintptr_t>(injector::GetBranchDestination(loc_7CCDD6));
                    injector::MakeCALL(loc_7CCDD6, CarRenderInfo_RenderFlaresOnCar_Hook, true);
                }
            }
        };
    }
} NOSTrailFix;