module;

#include <stdafx.h>
#include <d3d9.h>

export module XenonEffectFix;

import ComVars;

injector::hook_back<void(__fastcall*)(void*, void*, void*)> hb_sub_723380;
void __fastcall sub_723380(void* _this, void* edx, void* a2)
{
    hb_sub_723380.fun(_this, edx, a2);
    Direct3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
}

class XenonEffectFix
{
public:
    XenonEffectFix()
    {
        WFP::onInitEventAsync() += []()
        {
            CIniReader iniReader("");
            bool bFixXenonEffects = iniReader.ReadInteger("GRAPHICS", "FixXenonEffects", 1) != 0;

            if (bFixXenonEffects)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 8B 5B");
                hb_sub_723380.fun = injector::MakeCALL(pattern.get_first(), sub_723380, true).get();
            }
        };
    }
} XenonEffectFix;