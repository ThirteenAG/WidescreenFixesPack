module;

#include "stdafx.h"

export module Engine;

import ComVars;

export void InitEngine()
{
    CIniReader iniReader("");
    bool bSingleCoreAffinity = iniReader.ReadInteger("MAIN", "SingleCoreAffinity", 1);

    static bool bIsOPSAT = false;
    static bool bIsVideoPlaying = false;
    static auto GIsWideScreen = *hook::module_pattern(GetModuleHandle(L"Engine"), "8B 0D ? ? ? ? F3 0F 59 D9 F3 0F 10 0D ? ? ? ? F3 0F 5C D8 F3 0F 58 DC").get_first<uint8_t*>(2);
    static auto GIsSameFrontBufferOnNormalTV = *hook::module_pattern(GetModuleHandle(L"Engine"), "8B 15 ? ? ? ? 83 3A 00 74 58 F3 0F 10 88").get_first<uint8_t*>(2);
    auto pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "83 39 00 75 63 8B 15 ? ? ? ? 83 3A 00"); //
    static auto dword_109E09F0 = *pattern.get_first<float*>(60);
    static auto dword_109E09F4 = dword_109E09F0 + 1;
    static auto dword_109E09F8 = dword_109E09F0 + 2;
    static auto dword_109E09FC = dword_109E09F0 + 3;
    struct ImageUnrealDrawHook
    {
        void operator()(injector::reg_pack& regs)
        {
            //auto x = *(float*)&dword_109E09F0[regs.eax / 4];
            //auto y = *(float*)&dword_109E09F4[regs.eax / 4];
            auto w = *(float*)&dword_109E09F8[regs.eax / 4];
            auto h = *(float*)&dword_109E09FC[regs.eax / 4];

            if (!*GIsWideScreen && *GIsSameFrontBufferOnNormalTV)
            {
                *(float*)&dword_109E09F0[regs.eax / 4] *= 0.75f;
                *(float*)&dword_109E09F4[regs.eax / 4] *= 0.75f;
                *(float*)&dword_109E09F8[regs.eax / 4] *= 0.75f;
                *(float*)&dword_109E09FC[regs.eax / 4] *= 0.75f;
            }
            else
            {
                if (bIsOPSAT && bIsVideoPlaying)
                {
                    *(float*)&dword_109E09F0[regs.eax / 4] *= (4.0f / 3.0f);
                    *(float*)&dword_109E09F4[regs.eax / 4] *= (4.0f / 3.0f);
                    *(float*)&dword_109E09F8[regs.eax / 4] *= (4.0f / 3.0f);
                    *(float*)&dword_109E09FC[regs.eax / 4] *= (4.0f / 3.0f);
                }
            }

            if ((w == 1275.0f && h == 637.0f) || (w == 800.0f)) // loadscreen and fullscreen images
            {
                *(float*)&dword_109E09F0[regs.eax / 4] *= Screen.fHudScaleX;
                *(float*)&dword_109E09F8[regs.eax / 4] *= Screen.fHudScaleX;

                if (w == 1275.0f && h == 637.0f)
                {
                    *(float*)&dword_109E09F4[regs.eax / 4] *= Screen.fHudScaleX;
                    *(float*)&dword_109E09FC[regs.eax / 4] *= Screen.fHudScaleX;
                }
            }
        }
    }; injector::MakeInline<ImageUnrealDrawHook>(pattern.get_first(0), pattern.get_first(104));

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "D9 05 ? ? ? ? 83 C4 08 D8 C9 D9 5C 24 24");
    injector::WriteMemory(pattern.get_first(2), &Screen.fHudScaleX2, true);
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "D9 05 ? ? ? ? D8 4C 24 10 D9 5C 24 28");
    injector::WriteMemory(pattern.get_first(2), &Screen.fHudScaleX2, true);
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 10 0D ? ? ? ? F3 0F 11 44 24 ? F3 0F 11 44 24 ? F3 0F 10 05 ? ? ? ? F3 0F 11 4C 24 ? F3 0F 11 44 24");
    injector::WriteMemory(pattern.get_first(4), &Screen.fHudScaleX2, true);
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 10 0D ? ? ? ? F3 0F 10 15 ? ? ? ? 2B D1 F3 0F 2A C2");
    injector::WriteMemory(pattern.get_first(4), &Screen.fHudScaleX2, true);
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 5C 1D ? ? ? ? 0F 28 FD F3 0F 59 FA"); //0x103069A5 + 0x4
    injector::WriteMemory(pattern.get_first(4), &Screen.fHudOffset, true);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 5C D8 F3 0F 58 DC"); //0x1030503C
    struct HUDPosHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.xmm3.f32[0] -= Screen.fHudOffset;
            regs.xmm3.f32[0] += regs.xmm4.f32[0];
        }
    }; injector::MakeInline<HUDPosHook>(pattern.get_first(0), pattern.get_first(8));

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 5E CA F3 0F 11 4C 24 14 83 C4 04"); //0x1030678D
    struct TextHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.xmm1.f32[0] /= Screen.fTextScaleX;
        }
    }; injector::MakeInline<TextHook>(pattern.get_first(-4), pattern.get_first(4));

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 5C 1D ? ? ? ? 0F 28 FD F3 0F 59 FA"); //0x103069A5 + 0x4
    injector::WriteMemory(pattern.get_first(4), &Screen.fHudOffset, true);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 0D ? ? ? ? 83 39 00 89 54 24 20 75 3D 8B 15 ? ? ? ? 83 3A 00");
    struct TextHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            auto f3 = regs.xmm3.f32[0];
            auto f4 = *(float*)(regs.esp + 0x24);
            auto f5 = regs.xmm5.f32[0];
            auto f6 = regs.xmm6.f32[0];

            if (!*GIsWideScreen && *GIsSameFrontBufferOnNormalTV)
            {
                *(float*)(regs.ebp + 0x00) = f3 * 0.75f;
                *(float*)(regs.ebp + 0x04) = f4 * 0.75f;
                *(float*)(regs.ebp + 0x08) = f5 * 0.75f;
                *(float*)(regs.ebp + 0x0C) = f6 * 0.75f;
            }
            else
            {
                if (bIsOPSAT && bIsVideoPlaying)
                {
                    *(float*)(regs.ebp + 0x00) = f3 * (4.0f / 3.0f);
                    *(float*)(regs.ebp + 0x04) = f4 * (4.0f / 3.0f);
                    *(float*)(regs.ebp + 0x08) = f5 * (4.0f / 3.0f);
                    *(float*)(regs.ebp + 0x0C) = f6 * (4.0f / 3.0f);
                }
            }
        }
    }; injector::MakeInline<TextHook2>(pattern.get_first(0), pattern.get_first(76));

    //OPSAT and computer videos (scale text and hud like x360 version)
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 86 ? ? ? ? 83 C4 10 85 C0 74 49 83 78 64 00"); //0x10515086
    struct UCanvasOpenVideoHook
    {
        void operator()(injector::reg_pack& regs)
        {
            static auto getVideoPath = [](const std::string_view& str, char beg, char end) -> std::string
            {
                std::size_t begPos;
                if ((begPos = str.find(beg)) != std::string_view::npos)
                {
                    std::size_t endPos;
                    if ((endPos = str.find(end, begPos)) != std::string_view::npos && endPos != begPos + 1)
                        return std::string(str).substr(begPos + 1, endPos - begPos - 1);
                }

                return std::string();
            };
            bIsVideoPlaying = true;
            regs.eax = *(uint32_t*)(regs.esi + 0x80);
            auto str = getVideoPath(std::string_view(*(char**)(regs.esp + 0x0C)), '\\', '.');
            if (iequals(str, "computer_hq_1") || iequals(str, "computer_hq_2") || iequals(str, "computer_hq_3") ||
                iequals(str, "computer_mission_1") || iequals(str, "computer_mission_2") || starts_with(str.c_str(), "opsat", false))
                bIsOPSAT = true;
            else
                bIsOPSAT = false;
        }
    }; injector::MakeInline<UCanvasOpenVideoHook>(pattern.get_first(0), pattern.get_first(6));

    //
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 86 ? ? ? ? 85 C0 74 12 83 78 64 00 74 0C"); //0x10515110
    struct UCanvasCloseVideoHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.esi + 0x80);
            bIsVideoPlaying = false;
        }
    }; injector::MakeInline<UCanvasCloseVideoHook>(pattern.get_first(0), pattern.get_first(6));

    //FOV
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 91 28 06 00 00 52 8B"); //?Draw@UGameEngine@@UAEXPAVUViewport@@HPAEPAH@Z  10530BD7
    struct UGameEngine_Draw_Hook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)&regs.edx = AdjustFOV(*(float*)(regs.ecx + 0x628), Screen.fAspectRatio, Screen.fDefaultARforFOV);
        }
    }; injector::MakeInline<UGameEngine_Draw_Hook>(pattern.get_first(0), pattern.get_first(6));

    if (bSingleCoreAffinity)
    {
        if (AffinityChanges::Init())
        {
            IATHook::Replace(GetModuleHandle(L"Engine"), "kernel32.dll",
                std::forward_as_tuple("CreateThread", AffinityChanges::CreateThread_GameThread)
            );
        }
    }
}
