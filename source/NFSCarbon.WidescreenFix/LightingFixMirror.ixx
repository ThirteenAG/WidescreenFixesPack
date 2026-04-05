module;

#include <stdafx.h>

export module LightingFixMirror;

import ComVars;

namespace LightingFixMirror
{
    uint32_t* LightingFixUpdateMirrorCave_Exit = (uint32_t*)0x00748C5D;
    uint32_t* sub_713AA0 = (uint32_t*)0x713AA0;
    uint32_t* sub_713A30 = (uint32_t*)0x713A30;
    uint32_t* ptr_dword_AB095C = (uint32_t*)0x00AB095C;
    uint32_t* ptr_dword_AB0914 = (uint32_t*)0x00AB0914;
    void __declspec(naked) LightingFixUpdateMirrorCave()
    {
        _asm
        {
            cmp dword ptr[edi + 8], 01
            mov ecx, esi
            jne IsMirror
            mov eax, ds:ptr_dword_AB095C
            mov eax, [eax]
            push eax
            push 0x59
            call sub_713AA0
            jmp ExitCode

            IsMirror :
            mov eax, ds : ptr_dword_AB0914
                mov eax, [eax]
                push eax
                push 0x59
                call sub_713AA0
                mov eax, [esi + 0x1330]
                test eax, eax
                je ExitCode
                push 0
                push 0x80
                mov ecx, esi
                call sub_713A30

                ExitCode :
            jmp LightingFixUpdateMirrorCave_Exit
        }
    }
}

export void InitLightingFixMirror()
{
    CIniReader iniReader("");
    bool bLightingFix = iniReader.ReadInteger("GRAPHICS", "LightingFix", 1) != 0;

    if (bLightingFix)
    {
        uint32_t* dword_7497B9 = hook::pattern("8B 0D ? ? ? ? 83 F8 06 89 0D ? ? ? ? C7 05 ? ? ? ? CD CC CC 3E").count(2).get(1).get<uint32_t>(2);
        static float f20 = 2.0f;
        injector::WriteMemory((uint32_t)dword_7497B9, &f20, true);
        static float f19 = 1.9f;
        uint32_t* dword_748A70 = hook::pattern("A1 ? ? ? ? A3 ? ? ? ? 83 3D ? ? ? ? 06 C7 05 ? ? ? ? CD CC CC 3E").count(2).get(1).get<uint32_t>(1);
        injector::WriteMemory(dword_748A70, &f19, true);
        uint32_t* dword_73E7CB = *hook::pattern("C7 05 ? ? ? ? CD CC CC 3E B8 ? ? ? ? 74 05 B8 ? ? ? ? C3").count(1).get(0).get<uint32_t*>(11);
        injector::WriteMemory<float>(dword_73E7CB, 1.35f, true);

        //Lighting Fix Update (mirror)
        auto pattern = hook::pattern("8B 4F 6C 8B 91 8C 02 00 00 52 6A 6C 8B CE"); //0x00748C17
        uint32_t* dword_748C2A = pattern.count(1).get(0).get<uint32_t>(0x13);
        LightingFixMirror::ptr_dword_AB095C = *pattern.count(1).get(0).get<uint32_t*>(0x1C);
        LightingFixMirror::ptr_dword_AB0914 = *hook::pattern("52 50 FF 51 5C A1 ? ? ? ? 8B 08").count(1).get(0).get<uint32_t*>(6);
        LightingFixMirror::sub_713AA0 = hook::pattern("C2 08 00 CC 8B 54 24 08 8B 41 44 56").count(1).get(0).get<uint32_t>(4);
        LightingFixMirror::sub_713A30 = hook::pattern("89 81 84 17 00 00 C3 CC CC").count(1).get(0).get<uint32_t>(9);
        LightingFixMirror::LightingFixUpdateMirrorCave_Exit = hook::pattern("A1 ? ? ? ? 8B 15 ? ? ? ? 83 F8 06 89 15 ? ? ? ? C7 05 ? ? ? ? CD CC CC 3E").count(1).get(0).get<uint32_t>(0); //0x00748C5D
        injector::MakeJMP(dword_748C2A, LightingFixMirror::LightingFixUpdateMirrorCave, true);

        uint32_t* dword_72E382 = hook::pattern("C7 05 ? ? ? ? 01 00 00 00 C7 05 ? ? ? ? 00 00 80").count(1).get(0).get<uint32_t>(6);
        injector::WriteMemory(dword_72E382, 0, true);
    }
}