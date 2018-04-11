#include "stdafx.h"

struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    int32_t nWidth43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fHudOffsetReal;
    float fMainMenuPlayerFOV;
    float fCutOffArea;
    bool bHudFlash;
} Screen;

struct ResEntry
{
    int32_t offset;
    size_t length;
};

struct ResList
{
    ResEntry r01 = { 0  , length("800x600") };
    ResEntry r02 = { 8  , length("960x600") };
    ResEntry r03 = { 16 , length("1024x640xxx") };
    ResEntry r04 = { 28 , length("1024x768xxx") };
    ResEntry r05 = { 40 , length("1152x864xxx") };
    ResEntry r06 = { 52 , length("1360x765xxx") };
    ResEntry r07 = { 64 , length("1360x768xxx") };
    ResEntry r08 = { 76 , length("1280x768xxx") };
    ResEntry r09 = { 88 , length("1280x720xxx") };
    ResEntry r10 = { 100, length("1280x960xxx") };
    ResEntry r11 = { 112, length("1280x1024xx") };
    ResEntry r12 = { 124, length("1344x840xxx") };
    ResEntry r13 = { 136, length("1440x900xxx") };
    ResEntry r14 = { 148, length("1440x1080xx") };
    ResEntry r15 = { 160, length("1400x1050xx") };
    ResEntry r16 = { 172, length("1600x1024xx") };
    ResEntry r17 = { 184, length("1600x1200xx") };
    ResEntry r18 = { 196, length("1680x1050xx") };

    constexpr ResList()
    {}
};

void __cdecl sub_88C8E0(ptrdiff_t a1, ptrdiff_t a2, ptrdiff_t a3, int32_t a4)
{
    auto ptr = a2 + 8;

    do
    {
        ptr += 0x10;

        auto v1 = *(float *)(a1 + 0x20) * *(float *)(ptr - 0x10) + *(float *)(a1 + 0x30) * *(float *)(ptr - 0xC) + *(float *)(a1 + 0x10) * *(float *)(ptr - 0x14) + *(float *)(a1 + 0x00) * (*(float*)(ptr - 0x18) / (Screen.bHudFlash ? Screen.fHudScale : 1.0f));
        auto v2 = *(float *)(a1 + 0x24) * *(float *)(ptr - 0x10) + *(float *)(a1 + 0x34) * *(float *)(ptr - 0xC) + *(float *)(a1 + 0x14) * *(float *)(ptr - 0x14) + *(float *)(a1 + 0x04) * (*(float*)(ptr - 0x18) / (Screen.bHudFlash ? Screen.fHudScale : 1.0f));
        auto v3 = *(float *)(a1 + 0x28) * *(float *)(ptr - 0x10) + *(float *)(a1 + 0x38) * *(float *)(ptr - 0xC) + *(float *)(a1 + 0x18) * *(float *)(ptr - 0x14) + *(float *)(a1 + 0x08) * (*(float*)(ptr - 0x18) / (Screen.bHudFlash ? Screen.fHudScale : 1.0f));
        auto v4 = *(float *)(a1 + 0x2C) * *(float *)(ptr - 0x10) + *(float *)(a1 + 0x3C) * *(float *)(ptr - 0xC) + *(float *)(a1 + 0x1C) * *(float *)(ptr - 0x14) + *(float *)(a1 + 0x0C) * (*(float*)(ptr - 0x18) / (Screen.bHudFlash ? Screen.fHudScale : 1.0f));

        *(float *)(ptr + (a3 - a2) - 0x0C) = v4;
        *(float *)(ptr + (a3 - a2) - 0x10) = v3;
        *(float *)(ptr + (a3 - a2) - 0x14) = v2;
        *(float *)(ptr + (a3 - a2) - 0x18) = v1;

        --a4;
    } while (a4 > 0);
}

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("8B 74 24 10 0F BE 06 85 C0 57");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    CIniReader iniReader("");
    auto bPreferWidescreenResolutions = iniReader.ReadInteger("MAIN", "PreferWidescreenResolutions", 1) != 0;

    static constexpr ResList mem_chunk;
    static std::vector<std::string> list;
    GetResolutionsList(list);

    if (bPreferWidescreenResolutions && (list.size() > (sizeof(ResList) / sizeof(ResEntry))))
    {
        list.erase(std::remove_if(list.begin(), list.end(), [](const std::string& s) {
            int32_t x, y;
            sscanf(s.c_str(), "%dx%d", &x, &y);
            return (fabs(((float)x / (float)y) - (4.0f / 3.0f)) < FLT_EPSILON);
        }), list.end());
    }

    struct ResListHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.esi = *(uint32_t*)(regs.esp + 0x10);

            if (*(uint32_t*)(regs.esi + mem_chunk.r01.offset) == from_bytes<uint32_t>(to_bytes("800x")) && *(uint32_t*)(regs.esi + mem_chunk.r02.offset) == from_bytes<uint32_t>(to_bytes("960x"))) // strings from frontend_pc.str
            {
                ResEntry* data = (ResEntry*)&mem_chunk;
                injector::MemoryFill(regs.esi, 0, mem_chunk.r18.offset + mem_chunk.r18.length, true);

                for (int32_t i = (sizeof(mem_chunk) / sizeof(ResEntry)) - 1, k = list.size() - 1; (i >= 0 && k >= 0); i--, k--)
                {
                    if (list[k].length() <= data[i].length)
                    {
                        injector::WriteMemoryRaw(regs.esi + data[i].offset, list[k].data(), list[k].length(), true);
                    }
                }
            }

            regs.eax = *(uint8_t*)(regs.esi);
        }
    }; injector::MakeInline<ResListHook>(pattern.get_first(0), pattern.get_first(7)); //0x67EBB3

    static auto pGameHwnd = *hook::get_pattern<HWND*>("8B 0D ? ? ? ? 50 51 FF 15", 2); //0x102EBE4
    static auto dword_1062BA4 = *hook::get_pattern<uint32_t*>("8B 54 24 18 A1 ? ? ? ? 8B 4C 24", 5);
    static auto dword_1062BA8 = *hook::get_pattern<uint32_t*>("03 C2 8B 15 ? ? ? ? 89 44 24 20", 4);
    struct ResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = 1;
            Screen.nWidth = *dword_1062BA4;
            Screen.nHeight = *dword_1062BA8;

            Screen.fWidth = static_cast<float>(Screen.nWidth);
            Screen.fHeight = static_cast<float>(Screen.nHeight);
            Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
            Screen.nWidth43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
            Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
            Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
            Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
            Screen.fHudScale = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
            Screen.fCutOffArea = 0.5f * Screen.fHudScale;

            tagRECT REKT;
            auto[DesktopResW, DesktopResH] = GetDesktopRes();
            REKT.left = (LONG)(((float)DesktopResW / 2.0f) - (Screen.fWidth / 2.0f));
            REKT.top = (LONG)(((float)DesktopResH / 2.0f) - (Screen.fHeight / 2.0f));
            REKT.right = (LONG)Screen.nWidth;
            REKT.bottom = (LONG)Screen.nHeight;
            SetWindowPos(*pGameHwnd, NULL, REKT.left, REKT.top, REKT.right, REKT.bottom, SWP_NOACTIVATE | SWP_NOZORDER);
        }
    };
    pattern = hook::pattern("5E 5B A3 ? ? ? ? B8 ? ? ? ? 5F C3");
    injector::MakeInline<ResHook>(pattern.get_first(7)); //0x753712
    pattern = hook::pattern("E8 ? ? ? ? B8 ? ? ? ? 81 C4 ? ? ? ? C3");
    injector::MakeInline<ResHook>(pattern.get_first(5)); //0x753EA4
    pattern = hook::pattern("B8 ? ? ? ? 5B 83 C4 48 C3");
    for (size_t i = 1; i < pattern.size(); ++i)
    {
        injector::MakeInline<ResHook>(pattern.get(i).get<void>(0)); //0x7547D4 0x7548AC 0x754972
    }

    static void* spgfRet = nullptr;
    static auto dw81BE6F = hook::get_pattern("83 C4 14 8B 54 24 54 A1 ? ? ? ? 52 6A 1D FF 50 20", 0);
    static auto dw81A292 = hook::get_pattern("83 C4 14 5F 5E C2 04 00", 0);
    static auto dw81A3D2 = hook::get_pattern("D9 5F 6C 8B 15 ? ? ? ? FF 52 30 83 C4 14 5F 5E C2 04 00", 12);
    static auto dw81B4DC = hook::get_pattern("83 C4 0C 8B 54 24 30 8B 7C 24 4C", 0);
    static auto dw7CEA9B = hook::get_pattern("83 C4 0C 66 89 7E 3C 8B 4C 24 68 D9 44 24 18", 0);
    static auto dw7CC347 = hook::get_pattern("83 C4 0C 66 C7 45 3C 00 00 8B 45 28 D9 44 24 10 D8 A0 90 00 00 00", 0);
    static auto dw79F715 = hook::get_pattern("8B 44 24 48 83 C4 14 3B F0", 0);
    static auto dw7CF68D = hook::get_pattern("A1 ? ? ? ? 83 C4 08 85 C0 89 44 24 20 89 74 24 24", 0);
    static auto dw7CE562 = hook::get_pattern("83 C4 0C 66 C7 46 3C 00 00 8B 4D 00 8B 15 ? ? ? ? 51 6A 01", 0); //objective popup
    pattern = hook::pattern("A1 ? ? ? ? 53 55 56 57 33 FF 3B C7 89 7C 24 10");
    static auto dword_1062988 = *pattern.get_first<uint32_t*>(1);
    struct ScalingHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *dword_1062988;
            auto ptr = *(uint32_t*)(regs.esp + 0x24);
            auto length = *(size_t*)(regs.esp + 0x28);
            auto ret = *(void**)(regs.esp + 0x1C);
            auto clr = *(uint32_t*)(ptr + 0x10);

            if (length == 4)
            {
                auto a1 = *(float*)(ptr + (0 * 0x1C));
                auto a2 = *(float*)(ptr + (1 * 0x1C));
                auto a3 = *(float*)(ptr + (2 * 0x1C));
                auto a4 = *(float*)(ptr + (3 * 0x1C));

                if (clr == 0xFFFFFFFF)
                {
                    if (spgfRet) // movies
                    {
                        if (ret == spgfRet)
                            return;
                    }
                    else
                    {
                        HMODULE hModule = NULL;
                        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)ret, &hModule);
                        if (hModule != GetModuleHandle(NULL))
                        {
                            spgfRet = ret;
                            return;
                        }
                    }
                }
                else if (a1 == 0.0f && a2 == 0.0f && a3 >= Screen.fWidth && a4 >= Screen.fWidth)
                    return;
            }

            if (ret == dw81BE6F || ret == dw81A292 || ret == dw81A3D2 || ret == dw81B4DC) //radar
            {
                if (ret == dw81BE6F)
                {
                    for (size_t i = 0; i < length; i++)
                    {
                        *(float*)(ptr + (i * 0x1C)) /= Screen.fHudScale;
                        *(float*)(ptr + (i * 0x1C)) += Screen.fHudOffsetReal;
                    }
                }
                return;
            }

            if (ret != dw7CEA9B && ret != dw7CC347 && ret != dw7CE562 /*&& ret != dw79F715*/)
            {
                for (size_t i = 0; i < length; i++)
                {

                    *(float*)(ptr + (i * 0x1C)) /= Screen.fHudScale;
                    *(float*)(ptr + (i * 0x1C)) += Screen.fHudOffsetReal;
                }
            }
            else
            {
                for (size_t i = 0; i < length; i++)
                    *(float*)(ptr + (i * 0x1C)) += Screen.fHudOffsetReal;
            }

        }
    }; injector::MakeInline<ScalingHook>(pattern.get_first(0)); //0x76FDE3

    pattern = hook::pattern("D9 1D ? ? ? ? DB 44 24 08 D9 1D ? ? ? ? D9 40 68 DC C0 ");
    static auto dword_1061AF0 = *pattern.get_first<float*>(2);
    struct TextBreakHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto ret = *(void**)(regs.esp + 0x4);
            float temp = 0.0f;

            _asm {fstp dword ptr[temp]}
            if (ret != dw7CF68D)
                *dword_1061AF0 = temp / Screen.fHudScale;
            else
                *dword_1061AF0 = temp;
        }
    }; injector::MakeInline<TextBreakHook>(pattern.get_first(0), pattern.get_first(6)); //0x7231C4

    pattern = hook::pattern("B9 ? ? ? ? DC C0 D8 2D ? ? ? ? D9 5C 24 0C DB 44 24 10");
    static auto byte_1044190 = *pattern.get_first<uint32_t>(1);
    injector::MakeNOP(pattern.get_first(0), 5, true); //0x77D8E2
    pattern = hook::pattern("D9 E0 D9 1C 24 6A FF 50 DD D8 DD D8 E8");
    struct MouseFixHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float curWidth = 32.0f * Screen.fHudScale;
            float temp = 0.0f;
            _asm {fchs}
            _asm {fstp dword ptr[temp]}
            *(float*)(regs.esp + 0x00) = temp;

            auto v7 = static_cast<float>(*(int32_t*)(regs.ecx + 0x04) - *(int32_t*)(regs.ecx + 0x0C));
            auto v8 = static_cast<float>(*(int32_t*)(regs.ecx + 0x08) - *(int32_t*)(regs.ecx + 0x10));
            auto v9 = 1.0f - ((v8 + 32.0f) / Screen.fHeight + (v8 + 32.0f) / Screen.fHeight);
            auto v10 = -((1.0f / Screen.fHudScale) - ((v7 + curWidth) / (Screen.fWidth * Screen.fHudScale) + (v7 + curWidth) / (Screen.fWidth * Screen.fHudScale)));
            auto v11 = 1.0f - (v8 / Screen.fHeight + v8 / Screen.fHeight);
            auto v12 = -((1.0f / Screen.fHudScale) - (v7 / (Screen.fWidth * Screen.fHudScale) + v7 / (Screen.fWidth * Screen.fHudScale)));

            *(float*)(regs.esp + 0x00) = v12;
            *(float*)(regs.esp + 0x04) = v11;
            *(float*)(regs.esp + 0x08) = v10;
            *(float*)(regs.esp + 0x0C) = v9;

            regs.ecx = byte_1044190;
        }
    }; injector::MakeInline<MouseFixHook>(pattern.get_first(0)); //0x77D927

    static auto changeFOV = [](float f) -> float
    {
        return GetFOV((f <= 0.0f) ? 1.0471976f : f, Screen.fAspectRatio) / 0.5f;
    };

    pattern = hook::pattern("C7 44 24 58 ? ? ? ? D9 44 24 58 57");
    struct FOVHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.esp + 0x58) = changeFOV(*(float*)(regs.esp + 0x58));
        }
    }; injector::MakeInline<FOVHook1>(pattern.get_first(-2), pattern.get_first(8)); //0x7DC06B

    pattern = hook::pattern("C7 84 24 98 00 00 00 ? ? ? ? D9 84 24 98 00 00 00 8B 43 60");
    struct FOVHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.esp + 0x98) = changeFOV(*(float*)(regs.esp + 0x98));
        }
    }; injector::MakeInline<FOVHook2>(pattern.get_first(-2), pattern.get_first(11)); //0x81A133

    pattern = hook::pattern("8B 4E 40 C1 E9 03 F6 C1 01 D8 0D ? ? ? ? D9 5C 24 18");
    struct MainMenuPlayerFOVHook
    {
        void operator()(injector::reg_pack& regs)
        {
            if (!Screen.fMainMenuPlayerFOV)
            {
                Screen.fMainMenuPlayerFOV = 0.005f / (1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio)));
            }
            regs.ecx = *(uint32_t*)(regs.esi + 0x40);
            regs.ecx >>= 3;
        }
    }; injector::MakeInline<MainMenuPlayerFOVHook>(pattern.get_first(0), pattern.get_first(6)); //0x64DC2C

    pattern = hook::pattern("D8 0D ? ? ? ? D9 5C 24 18 74 0A 56 8B CF");
    injector::WriteMemory(pattern.get_first(2), &Screen.fMainMenuPlayerFOV, true); //0x64DC35

    //peds disappearing issue
    pattern = hook::pattern("D8 0D ? ? ? ? D9 FF D9 C0 D8 C9 D9 1D");
    injector::WriteMemory(pattern.get_first(2), &Screen.fCutOffArea, true); //0x5ADFFE

    //hud flashes
    pattern = hook::pattern("8B 81 9C 00 00 00 85 C0 74 32 D9 84 24 80 00 00 00");
    struct FlashHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.ecx + 0x9C);
            if (regs.eax)
                Screen.bHudFlash = true;
            else
                Screen.bHudFlash = false;
        }
    }; injector::MakeInline<FlashHook1>(pattern.get_first(0), pattern.get_first(6)); //0x783450

    pattern = hook::pattern("E8 ? ? ? ? 83 C4 10 5F 5E 8B E5 5D C2 04 00");
    injector::MakeCALL(pattern.count(3).get(2).get<void>(0), sub_88C8E0, true); //0x7837F1
    //7590A0 hud weapon related

    //videos crash, just in case
    pattern = hook::pattern("C6 86 DE 01 00 00 01");
    injector::MakeNOP(pattern.count(6).get(1).get<void>(0), 7, true);
    injector::MakeNOP(pattern.count(6).get(2).get<void>(0), 7, true);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        Init(NULL);
    }
    return TRUE;
}