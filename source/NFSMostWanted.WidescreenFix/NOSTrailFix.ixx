module;

#include <stdafx.h>

export module NOSTrailFix;

import ComVars;

float NOSTrailPositionScalar = 0.3f;
constexpr float NOSTargetFPS = 60.0f; // original FPS we're targeting from. Consoles target 60 but run at 30, hence have longer trails than PC. Targeting 60 is smarter due to less issues with shorter trails. Use SimRate -2 to get the same effect as console versions.
bMatrix4 carbody_nos;
float fCustomNOSTrailLength = 1.0f;

injector::hook_back<void(__fastcall*)(void*, void*, void*, bVector3*, bMatrix4*, int, int, int)> hbCarRenderInfo_RenderFlaresOnCar;
void __fastcall CarRenderInfo_RenderFlaresOnCar(void* _this, void* edx, void* eView, bVector3* position, bMatrix4* body_matrix, int unk1, int unk2, int unk3)
{
    int TargetRate = 60;
    if (SimRate)
        TargetRate = SimRate;

    float NOSTrailScalar = (TargetRate / NOSTargetFPS) * fCustomNOSTrailLength;

    memcpy(&carbody_nos, body_matrix, sizeof(bMatrix4));

    float pos_scale = (NOSTrailScalar * NOSTrailPositionScalar);
    if (pos_scale < 1.0f)
        pos_scale = 1.0f;

    carbody_nos.v0.x *= pos_scale;
    carbody_nos.v0.y *= pos_scale;
    carbody_nos.v0.z *= pos_scale;

    carbody_nos.v2.x *= pos_scale;
    carbody_nos.v2.y *= pos_scale;

    return hbCarRenderInfo_RenderFlaresOnCar.fun(_this, edx, eView, position, &carbody_nos, unk1, unk2, unk3);
}

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
            fCustomNOSTrailLength = iniReader.ReadFloat("NOSTrail", "CustomNOSTrailLength", 1.0f);
            NOSTrailPositionScalar = iniReader.ReadFloat("NOSTrail", "NOSTrailPositionScalar", 0.3f);

            if (bFixNOSTrailLength)
            {
                auto pattern = hook::pattern("EB ? 8D 9B ? ? ? ? 40");
                injector::MakeNOP(pattern.get_first(0), 8, true);
                static auto NOSTrailHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    auto WorldPos1 = (bVector3*)regs.edx;
                    auto WorldPos2 = (bVector3*)regs.esi;
                    regs.edx = regs.esp + 0x30;
                    auto NOSFlarePos = (bVector3*)regs.edx;

                    int TargetRate = 60;
                    if (SimRate)
                        TargetRate = SimRate;

                    float NOSTrailScalar = (TargetRate / NOSTargetFPS) * fCustomNOSTrailLength;

                    (*NOSFlarePos).x = ((*WorldPos1).x - (*WorldPos2).x) * NOSTrailScalar;
                    (*NOSFlarePos).y = ((*WorldPos1).y - (*WorldPos2).y) * NOSTrailScalar;
                    (*NOSFlarePos).z = ((*WorldPos1).z - (*WorldPos2).z) * NOSTrailScalar;

                    regs.eax = 0;
                });

                if (bFixNOSTrailPosition)
                {
                    auto pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 ? 3B 05 ? ? ? ? 0F 8C");
                    hbCarRenderInfo_RenderFlaresOnCar.fun = injector::MakeCALL(pattern.get_first(), CarRenderInfo_RenderFlaresOnCar, true).get();
                }
            }
        };
    }
} NOSTrailFix;