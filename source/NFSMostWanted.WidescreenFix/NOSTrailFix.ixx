module;

#include <stdafx.h>

export module NOSTrailFix;

import ComVars;

#pragma runtime_checks( "", off )
namespace NOSTrailFix
{
    float NOSTrailScalar = 2.0f;
    float NOSTrailPositionScalar = 0.3f;
    bMatrix4 carbody_nos;

    uintptr_t CarRenderInfo_RenderFlaresOnCar_Addr = 0x00742950;

    void __stdcall CarRenderInfo_RenderFlaresOnCar_Hook(void* eView, bVector3* position, bMatrix4* body_matrix, int unk1, int unk2, int unk3)
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

        return reinterpret_cast<void(__thiscall*)(void*, void*, bVector3*, bMatrix4*, int, int, int)>(CarRenderInfo_RenderFlaresOnCar_Addr)(thethis, eView, position, &carbody_nos, unk1, unk2, unk3);
    }

    bVector3* WorldPos1;
    bVector3* WorldPos2;
    bVector3* NOSFlarePos;

    uintptr_t NOSTrailCave2Exit = 0x745040;
    void __declspec(naked) NOSTrailCave2()
    {
        _asm
        {
            mov WorldPos1, edx
            mov WorldPos2, esi
            lea edx, [esp + 0x30]
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
}
#pragma runtime_checks( "", restore )

export void InitNOSTrailFix()
{
    CIniReader iniReader("");
    bool bFixNOSTrailLength = iniReader.ReadInteger("NOSTrail", "FixNOSTrailLength", 1) == 1;
    bool bFixNOSTrailPosition = iniReader.ReadInteger("NOSTrail", "FixNOSTrailPosition", 0) != 0;
    static float fCustomNOSTrailLength = iniReader.ReadFloat("NOSTrail", "CustomNOSTrailLength", 1.0f);
    NOSTrailFix::NOSTrailPositionScalar = iniReader.ReadFloat("NOSTrail", "NOSTrailPositionScalar", 0.3f);

    if (bFixNOSTrailLength)
    {
        static int TargetRate = 60;

        if (SimRate)
            TargetRate = SimRate;

        constexpr float NOSTargetFPS = 60.0f; // original FPS we're targeting from. Consoles target 60 but run at 30, hence have longer trails than PC. Targeting 60 is smarter due to less issues with shorter trails. Use SimRate -2 to get the same effect as console versions.
        NOSTrailFix::NOSTrailScalar = (TargetRate / NOSTargetFPS) * fCustomNOSTrailLength;

        uintptr_t loc_745038 = reinterpret_cast<uintptr_t>(hook::pattern("EB 06 8D 9B 00 00 00 00 40 89 44 24 18").get_first(0));
        injector::MakeJMP(loc_745038, NOSTrailFix::NOSTrailCave2, true);
        NOSTrailFix::NOSTrailCave2Exit = loc_745038 + 8;

        if (bFixNOSTrailPosition)
        {
            uintptr_t loc_7450E8 = reinterpret_cast<uintptr_t>(hook::pattern("D9 44 24 30 6A 02 D8 4C 24 10 6A 00 8B CB C1 E1 06").get_first(0)) + 0x60;
            NOSTrailFix::CarRenderInfo_RenderFlaresOnCar_Addr = static_cast<uintptr_t>(injector::GetBranchDestination(loc_7450E8));
            injector::MakeCALL(loc_7450E8, NOSTrailFix::CarRenderInfo_RenderFlaresOnCar_Hook, true);
        }
    }
}