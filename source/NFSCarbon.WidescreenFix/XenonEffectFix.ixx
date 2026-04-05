module;

#include <stdafx.h>
#include <d3d9.h>

export module XenonEffectFix;

import ComVars;

namespace XenonEffectFix
{
    uint32_t* dword_AB0ABC = (uint32_t*)0x00AB0ABC;
    void(__thiscall* sub_723380)(void* that, void* texture) = (void(__thiscall*)(void*, void*))0x723380;
    #pragma runtime_checks( "", off )
    void __stdcall sub_723380_hook(void* texture)
    {
        void* that;
        _asm mov that, ecx

        sub_723380(that, texture);
        LPDIRECT3DDEVICE9 gDevice = *(LPDIRECT3DDEVICE9*)dword_AB0ABC;
        gDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
    }

    #pragma runtime_checks( "", restore )
}

export void InitXenonEffectFix()
{
    CIniReader iniReader("");
    bool bFixXenonEffects = iniReader.ReadInteger("GRAPHICS", "FixXenonEffects", 1) != 0;

    if (bFixXenonEffects)
    {
        uint32_t* dword_749BC5 = hook::pattern("FF 91 90 01 00 00 8B 4B 04 8B 07 51 57 FF 90 A0 01 00 00").count(1).get(0).get<uint32_t>(0x1D); //0x00749BA8
        auto pattern = hook::pattern("A1 ? ? ? ? 8B 08 6A 01 6A 16 50 FF 91 E4 00 00 00 5F 5E C2 04 00"); //0x007233A1
        XenonEffectFix::dword_AB0ABC = *pattern.count(1).get(0).get<uint32_t*>(1);
        XenonEffectFix::sub_723380 = (void(__thiscall*)(void*, void*))pattern.count(1).get(0).get<uint32_t>(-0x21);

        injector::MakeCALL(dword_749BC5, XenonEffectFix::sub_723380_hook, true);
    }
}