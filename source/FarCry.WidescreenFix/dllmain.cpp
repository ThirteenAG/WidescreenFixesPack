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
    float fFMVAspectRatio = 4.0f / 3.0f;
    float fFMVOffsetH;
    float fFMVOffsetV;
    float fRadarVerticalOffset;
    bool bStretch;
    std::optional<float> fHudAspectRatioConstraint;
    float fWidescreenHudOffset;
    float fFOV;
    float fIniFOV;
    float fFOVFactor;
    bool bUnstretchedText = true;

    void AdjustToRes(uint32_t x, uint32_t y)
    {
        nWidth = x;
        nHeight = y;
        fWidth = static_cast<float>(nWidth);
        fHeight = static_cast<float>(nHeight);
        fAspectRatio = fWidth / fHeight;
        nWidth43 = static_cast<uint32_t>(fHeight * (4.0f / 3.0f));
        fWidth43 = static_cast<float>(nWidth43);
        fHudOffsetReal = (fWidth - fHeight * (4.0f / 3.0f)) / 2.0f;
        fHudScale = 1.0f / (((4.0f / 3.0f)) / (fAspectRatio));
        fHudOffset = (((600.0f * fAspectRatio) - 800.0f) / 2.0f) / fHudScale;
        fRadarVerticalOffset = fHudOffset * (4.0f / 3.0f);
        fFOVFactor = fHudScale * fIniFOV;
        fWidescreenHudOffset = -CalculateWidescreenOffset(fWidth, fHeight, 800.0f, 600.0f, 3.5f);
        if (fHudAspectRatioConstraint.has_value())
        {
            float value = fHudAspectRatioConstraint.value();
            if (value < 0.0f || value > (32.0f / 9.0f))
                fWidescreenHudOffset = value;
            else
            {
                value = ClampHudAspectRatio(value, fAspectRatio);
                fWidescreenHudOffset = -CalculateWidescreenOffset(fHeight * value, fHeight, 800.0f, 600.0f, 3.5f);
            }
        }
        fWidescreenHudOffset /= fHudScale;
    }

    void AdjustFMVRes(uint32_t w, uint32_t h)
    {
        fFMVAspectRatio = static_cast<float>(w) / static_cast<float>(h);
        fFMVOffsetH = (((600.0f * fFMVAspectRatio) - 800.0f) / 2.0f) / fHudScale;
        fFMVOffsetV = (600.0f - (600.0f / (1.0f / (((4.0f / 3.0f)) / (fFMVAspectRatio))))) / 2.0f;
    }
} Screen;

class QUAD
{
public:
    float x1;
    float y1;
    float x2;
    float y2;
    float x3;
    float y3;
    float x4;
    float y4;

    int32_t ix1;
    int32_t iy1;
    int32_t ix2;
    int32_t iy2;
    int32_t ix3;
    int32_t iy3;
    int32_t ix4;
    int32_t iy4;

    inline QUAD(float x_1, float y_1, float x_2, float y_2, float x_3, float y_3, float x_4, float y_4)
        : x1(x_1), y1(y_1), x2(x_2), y2(y_2), x3(x_3), y3(y_3), x4(x_4), y4(y_4)
    {
        ix1 = static_cast<int32_t>(floor(x1));
        iy1 = static_cast<int32_t>(floor(y1));
        ix2 = static_cast<int32_t>(floor(x2));
        iy2 = static_cast<int32_t>(floor(y2));
        ix3 = static_cast<int32_t>(floor(x3));
        iy3 = static_cast<int32_t>(floor(y3));
        ix4 = static_cast<int32_t>(floor(x4));
        iy4 = static_cast<int32_t>(floor(y4));
    }

    inline QUAD(int32_t x_1, int32_t y_1, int32_t x_2, int32_t y_2, int32_t x_3, int32_t y_3, int32_t x_4, int32_t y_4)
        : ix1(x_1), iy1(y_1), ix2(x_2), iy2(y_2), ix3(x_3), iy3(y_3), ix4(x_4), iy4(y_4)
    {
    }

    inline bool operator==(const QUAD& rhs)
    {
        return this->ix1 == rhs.ix1 && this->iy1 == rhs.iy1 && this->ix2 == rhs.ix2 && this->iy2 == rhs.iy2;
    }
    inline bool operator!=(const QUAD& rhs) { return !(*this == rhs); }

    inline QUAD() {}
};

int WINAPI GetSystemMetricsHook(int nIndex)
{
    auto [DesktopResW, DesktopResH] = GetDesktopRes();

    if (nIndex == SM_CXFULLSCREEN)
        return DesktopResW;

    if (nIndex == SM_CYFULLSCREEN)
        return DesktopResH;

    if (nIndex != SM_CXDLGFRAME && nIndex != SM_CYCAPTION)
        return ::GetSystemMetrics(nIndex);
    else
        return 0;
}

#ifndef _WIN64
void* dword_36552A15 = nullptr;
void* dword_365526AD = nullptr;
void* dword_3302ED49 = nullptr;
void* dword_330D054C = nullptr;
void* dword_330D05E4 = nullptr;
void* dword_330D06E5 = nullptr;
void* dword_330D0771 = nullptr;
void* dword_330D0832 = nullptr;
void* dword_330D0D61 = nullptr;
void* dword_330D134A = nullptr;
void* dword_330193CE = nullptr;
void* dword_33038051 = nullptr;
#else
void* dword_1005DA88 = nullptr;
void* dword_10068D88 = nullptr;
void* dword_1018F2CE = nullptr;
void* dword_1018DCD8 = nullptr;
void* dword_1018DD8F = nullptr;
void* dword_1018DEA3 = nullptr;
void* dword_1018DF57 = nullptr;
void* dword_1018E00E = nullptr;
void* dword_1018ECE0 = nullptr;
void* dword_100B0A58 = nullptr;
void* dword_100B1018 = nullptr;
void* dword_1018EB80 = nullptr;
void* dword_1018E5B4 = nullptr;
void* dword_100B72BB = nullptr;
void* dword_1018F11C = nullptr;
void* dword_1018E4C6 = nullptr;
#endif

void
#ifndef _WIN64
__stdcall
#else
__fastcall
#endif
sub_380E58B5(float* a1, float a2, float a3, float a4, float a5, float a6, float a7)
{
    auto v8 = (Screen.bStretch ? 1.0f / Screen.fHudScale : 1.0f) / (a3 - a2);
    auto v9 = 1.0f / (a5 - a4);
    a1[0] = v8 * 2.0f;
    a1[1] = 0.0f;
    a1[2] = 0.0f;
    a1[3] = 0.0f;
    a1[4] = 0.0f;
    a1[5] = v9 * 2.0f;
    a1[6] = 0.0f;
    a1[7] = 0.0f;
    a1[8] = 0.0f;
    a1[9] = 0.0f;
    a1[10] = 1.0f / (a7 - a6);
    a1[11] = 0.0f;
    a1[12] = -((a2 + a3) * v8);
    a1[13] = -((a4 + a5) * v9);
    a1[14] = -((1.0f / (a7 - a6)) * a6);
    a1[15] = 1.0f;
}

void Init()
{
    CIniReader iniReader("");
    Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(iniReader.ReadString("MAIN", "HudAspectRatioConstraint", ""));
    Screen.fIniFOV = iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f);
    if (!Screen.fIniFOV) { Screen.fIniFOV = 1.0f; }
}

void InitXRenderD3D9()
{
    #ifndef _WIN64
    auto pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "E8 ? ? ? ? 8B 86 98 C6 01 00 8B 08 57 6A 03 50 FF 91 B0 00 00 00 8B 86 18 D3 01 00");
    injector::MakeCALL(pattern.get_first(0), sub_380E58B5, true); //text 0x38012EB7
    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "55 E8 ? ? ? ? 8B 86 98 C6 01 00");
    injector::MakeCALL(pattern.get_first(1), sub_380E58B5, true); //2d 0x3806B8A6
    //injector::MakeCALL(0x3806726B, sub_380E58B5, true); //3d? and crosshair (2d lines)
    #else
    auto pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "E8 ? ? ? ? 48 8B 8F ? ? ? ? 4C 8B C3 48 8B 01 BA ? ? ? ? FF 90 ? ? ? ? 48 8B 8F");
    injector::MakeCALLTrampoline(pattern.get_first(), sub_380E58B5, true);

    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "E8 ? ? ? ? 48 8B 8D ? ? ? ? 48 8B 01 4C 8B C3 BA ? ? ? ? FF 90 ? ? ? ? 48 8B 8D");
    injector::MakeCALLTrampoline(pattern.get_first(), sub_380E58B5, true);

    /*
    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "E8 ? ? ? ? 48 8B 8E ? ? ? ? 4C 8B C3 48 8B 01 BA ? ? ? ? FF 90 ? ? ? ? 48 8B 8E");
    */
    #endif

    #ifndef _WIN64
    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 83 EC 18 55 8B 6C 24 3C 81 FD 00 10 00 00 56");
    static auto DrawImage = (void(__thiscall*)(void* _this, float x, float y, float w, float h, int tex, float a6, float a7, float a8, float a9, float a10, float r, float g, float b, float alpha, float a15)) pattern.get_first(0);

    static auto DrawBorders = [](uint32_t _this)
    {
        if (Screen.bStretch)
        {
            Screen.bStretch = false;
            DrawImage((void*)_this, 0.0f, 0.0f, Screen.fHudOffset + 1.0f, 600.0f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.5f);
            DrawImage((void*)_this, 800.0f - Screen.fHudOffset - 1.0f, 0.0f, Screen.fHudOffset + 1.0f, 600.0f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.5f);
            Screen.bStretch = true;
        }
    };
    #else
    static SafetyHookInline Draw_2DImageHook{};
    static auto DrawImage = [](void* _this, float x, float y, float w, float h, int tex, float a6, float a7, float a8, float a9, float a10, float r, float g, float b, float alpha, float a15)
    {
        return Draw_2DImageHook.unsafe_fastcall(_this, x, y, w, h, tex, a6, a7, a8, a9, a10, r, g, b, alpha, a15);
    };

    static auto DrawBorders = [](void* _this)
    {
        if (Screen.bStretch)
        {
            Screen.bStretch = false;
            DrawImage(_this, 0.0f, 0.0f, Screen.fHudOffset + 1.0f, 600.0f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.5f);
            DrawImage(_this, 800.0f - Screen.fHudOffset - 1.0f, 0.0f, Screen.fHudOffset + 1.0f, 600.0f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.5f);
            Screen.bStretch = true;
        }
    };
    #endif

    #ifndef _WIN64
    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "8B B1 98 C6 01 00 85 F6 89 81 54 6D 01 00");
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            Screen.AdjustToRes(*(uint32_t*)(regs.esi + 0x0C), *(uint32_t*)(regs.esi + 0x10));
            regs.esi = *(uint32_t*)(regs.ecx + 0x1C698);
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "89 BE 84 D4 01 00 8B 9E 7C D4 01 00");
    struct SetResHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            Screen.AdjustToRes(*(uint32_t*)(regs.edi + 0x0C), *(uint32_t*)(regs.edi + 0x10));
            *(uint32_t*)(regs.esi + 0x1D484) = regs.edi;
        }
    }; injector::MakeInline<SetResHook2>(pattern.get_first(0), pattern.get_first(6));
    #else
    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "44 89 89 ? ? ? ? 44 89 81 ? ? ? ? 89 91 ? ? ? ? 44 89 91");
    static auto SetResHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        Screen.AdjustToRes(static_cast<uint32_t>(regs.r9), static_cast<uint32_t>(regs.r8));
    });

    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "8B 8B ? ? ? ? 89 48 10 8B 8B ? ? ? ? 89 48 14");
    static auto SetResHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto w = *(uint32_t*)(regs.rbx + 0x17BC8);
        auto h = *(uint32_t*)(regs.rbx + 0x17BCC);
        Screen.AdjustToRes(w, h);
    });
    #endif

    #ifndef _WIN64
    //borderless windowed
    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "8B 35 ? ? ? ? 6A 10");
    injector::WriteMemory(*pattern.get_first<uint32_t*>(2), GetSystemMetricsHook, true); //0x3816A218

    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "8B 86 ? ? ? ? 83 C0 F0 3B F8 7E 02 8B F8");
    injector::WriteMemory<uint8_t>(pattern.get_first(-2), 0xEB, true); //jmp

    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "B8 00 00 CA 10 89 83 34 B7 01 00");
    injector::WriteMemory(pattern.get_first(1), *pattern.get_first<LONG>(1) & ~WS_OVERLAPPEDWINDOW, true);
    #else
    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "FF 15 ? ? ? ? B9 ? ? ? ? 2B C3 99 2B C2 D1 F8 44 8B F0");
    auto ptr = (uint64_t)pattern.get_first(0) + injector::ReadMemory<uint32_t>(pattern.get_first(2), true) + 6;
    injector::WriteMemory(ptr, GetSystemMetricsHook, true); //0x1008845E

    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "41 8B 84 24 ? ? ? ? 83 C0 F0 3B F8 0F 4F F8");
    injector::WriteMemory<uint8_t>(pattern.get_first(-2), 0xEB, true); //jmp

    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "BD ? ? ? ? 89 AF ? ? ? ? 4C 89 AC 24 ? ? ? ? 0F 85 ? ? ? ? 41 BD");
    injector::WriteMemory(pattern.get_first(1), *pattern.get_first<LONG>(1) & ~WS_OVERLAPPEDWINDOW, true);
    #endif

    //2D
    #ifndef _WIN64
    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "8B 86 ? ? ? ? 8B 80 ? ? ? ? 8D 8D");
    struct Draw_2DImageHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.esi + 0x171C4);

            Screen.bStretch = false;

            auto x1 = *(float*)(regs.esp + 0x30);
            auto y1 = *(float*)(regs.esp + 0x34);
            auto x2 = *(float*)(regs.esp + 0x38);
            auto y2 = *(float*)(regs.esp + 0x3C);

            void* ret = *(void**)(regs.esp + 0x2C);
            void* ret2 = nullptr;
            void* ret3 = nullptr;
            if (ret == dword_36552A15)
            {
                ret2 = *(void**)(regs.esp + 0x1F4);
                ret3 = *(void**)(regs.esp + 0x208);
            }
            else if (ret == dword_3302ED49)
            {
                ret2 = *(void**)(regs.esp + 0x90);
            }

            if (ret == dword_3302ED49 && ret2 == dword_33038051) //fmv
            {
                Screen.bStretch = false;
                *(float*)(regs.esp + 0x30) /= Screen.fHudScale;
                *(float*)(regs.esp + 0x38) /= Screen.fHudScale;
                *(float*)(regs.esp + 0x30) += Screen.fHudOffset;

                *(float*)(regs.esp + 0x30) -= Screen.fFMVOffsetH;
                *(float*)(regs.esp + 0x34) -= Screen.fFMVOffsetV;
                *(float*)(regs.esp + 0x38) += Screen.fFMVOffsetH + Screen.fFMVOffsetH;
                *(float*)(regs.esp + 0x3C) += Screen.fFMVOffsetV + Screen.fFMVOffsetV;
                return;
            }

            if (ret == dword_330D054C || ret == dword_330D05E4 || ret == dword_330D06E5 || ret == dword_330D0771)
            {
                *(float*)(regs.esp + 0x30) /= Screen.fHudScale;
                *(float*)(regs.esp + 0x34) /= Screen.fHudScale;
                *(float*)(regs.esp + 0x38) /= Screen.fHudScale;
                *(float*)(regs.esp + 0x3C) /= Screen.fHudScale;

                *(float*)(regs.esp + 0x30) += Screen.fHudOffset;
                *(float*)(regs.esp + 0x34) += Screen.fRadarVerticalOffset;
                Screen.bStretch = false;

                {
                    *(float*)(regs.esp + 0x30) -= Screen.fWidescreenHudOffset;
                }
            }
            else if (ret == dword_330D0832 || ret == dword_330D0D61 || ret == dword_330D134A)
            {
                *(float*)(regs.esp + 0x30) /= Screen.fHudScale;
                *(float*)(regs.esp + 0x34) /= Screen.fHudScale;
                *(float*)(regs.esp + 0x38) /= Screen.fHudScale;

                *(float*)(regs.esp + 0x30) += Screen.fHudOffset;
                *(float*)(regs.esp + 0x34) += Screen.fRadarVerticalOffset;
                Screen.bStretch = false;

                {
                    *(float*)(regs.esp + 0x30) -= Screen.fWidescreenHudOffset;
                }
            }
            else
            {
                if (ret == dword_365526AD) // Objectives window on Tab
                {
                    if (x2 == 50.0f && y2 == 25.0f) // enemy markers (binoculars)
                    {
                        *(float*)(regs.esp + 0x38) /= Screen.fHudScale;
                        *(float*)(regs.esp + 0x30) += (50.0f - *(float*)(regs.esp + 0x38)) / 2.0f;
                        Screen.bStretch = false;
                    }
                    else
                        Screen.bStretch = true;
                }
                else if (ret2 == dword_330193CE) // Damage overlay
                {
                    Screen.bStretch = false;
                }
                else
                {
                    Screen.bStretch = true;
                    DrawBorders(regs.ecx);
                }
            }
        }
    }; injector::MakeInline<Draw_2DImageHook>(pattern.get_first(0), pattern.get_first(6));
    #else
    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "48 8B C4 48 81 EC ? ? ? ? 48 C7 44 24 ? ? ? ? ? 48 89 58 F8 48 89 68 F0 48 89 70 E8 48 89 78 E0 4C 89 60 D8 66 0F 7F 70");
    Draw_2DImageHook = safetyhook::create_inline(pattern.get_first(), +[](void* _this, float x1, float y1, float x2, float y2, int tex, float a6, float a7, float a8, float a9, float a10, float r, float g, float b, float alpha, float a15) -> int64_t
    {
        static void* stack[6];
        CaptureStackBackTrace(0, 6, stack, NULL);
        Screen.bStretch = false;

        void* ret = stack[2];
        void* ret2 = stack[3];

        if (ret == dword_1005DA88 && ret2 == dword_10068D88) //fmv
        {
            Screen.bStretch = false;
            x1 /= Screen.fHudScale;
            x2 /= Screen.fHudScale;
            x1 += Screen.fHudOffset;

            x1 -= Screen.fFMVOffsetH;
            y1 -= Screen.fFMVOffsetV;
            x2 += Screen.fFMVOffsetH + Screen.fFMVOffsetH;
            y2 += Screen.fFMVOffsetV + Screen.fFMVOffsetV;
        }
        else if (ret == dword_1018DCD8 || ret == dword_1018DD8F || ret == dword_1018DEA3 || ret == dword_1018DF57)
        {
            x1 /= Screen.fHudScale;
            y1 /= Screen.fHudScale;
            x2 /= Screen.fHudScale;
            y2 /= Screen.fHudScale;

            x1 += Screen.fHudOffset;
            y1 += Screen.fRadarVerticalOffset;
            Screen.bStretch = false;

            {
                x1 -= Screen.fWidescreenHudOffset;
            }
        }
        else if (ret == dword_1018F2CE || ret == dword_1018E00E || ret == dword_1018ECE0 || ret == dword_1018EB80 || ret == dword_1018E5B4 || ret == dword_1018F11C || ret == dword_1018E4C6)
        {
            x1 /= Screen.fHudScale;
            y1 /= Screen.fHudScale;
            x2 /= Screen.fHudScale;

            x1 += Screen.fHudOffset;
            y1 += Screen.fRadarVerticalOffset;
            Screen.bStretch = false;

            {
                x1 -= Screen.fWidescreenHudOffset;
            }
        }
        else
        {
            if (ret == dword_100B0A58) // Objectives window on Tab
            {
                if (x2 == 50.0f && y2 == 25.0f) // enemy markers (binoculars)
                {
                    x2 /= Screen.fHudScale;
                    x1 += (50.0f - x2) / 2.0f;
                    Screen.bStretch = false;
                }
                else
                    Screen.bStretch = true;
            }
            else if (ret2 == dword_100B72BB) // Damage overlay (and scopes in x64 version)
            {
                if ((x1 == 0.0f && x2 == 800.0f && y1 == 0.0f && y2 == 90.0f) || (x1 == 0.0f && x2 == 800.0f && y1 == 510.0f && y2 == 90.0f) ||
                    (x1 == 0.0f && x2 == 90.0f && y1 == 0.0f && y2 == 600.0f) || (x1 == 710.0f && x2 == 90.0f && y1 == 0.0f && y2 == 600.0f))
                    Screen.bStretch = false;
                else
                {
                    Screen.bStretch = true;
                    DrawBorders(_this);
                }
            }
            else
            {
                Screen.bStretch = true;
                DrawBorders(_this);
            }
        }

        return Draw_2DImageHook.unsafe_fastcall<int64_t>(_this, x1, y1, x2, y2, tex, a6, a7, a8, a9, a10, r, g, b, alpha, a15);
    });
    #endif

    //Crosshair
    #ifndef _WIN64
    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "89 51 08 89 58 24 8B 86");
    struct DrawLineHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.eax + 0x00) /= Screen.fHudScale;
            *(float*)(regs.eax + 0x00) += Screen.fHudOffset;
            *(float*)(regs.eax + 0x18) /= Screen.fHudScale;
            *(float*)(regs.eax + 0x18) += Screen.fHudOffset;

            *(uint32_t*)(regs.ecx + 0x08) = regs.edx;
            *(uint32_t*)(regs.eax + 0x24) = regs.ebx;
        }
    }; injector::MakeInline<DrawLineHook>(pattern.get_first(0), pattern.get_first(6));
    #else
    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "41 89 43 18 8B 47 04 41 89 43 1C 8B 47 08 41 89 6B 24 41 89 43 20");
    static auto DrawLineHook = safetyhook::create_mid(pattern.get_first(22), [](SafetyHookContext& regs)
    {
        *(float*)(regs.r11 + 0x00) /= Screen.fHudScale;
        *(float*)(regs.r11 + 0x00) += Screen.fHudOffset;
        *(float*)(regs.r11 + 0x18) /= Screen.fHudScale;
        *(float*)(regs.r11 + 0x18) += Screen.fHudOffset;
    });
    #endif

    //FullscreenFMV
    #ifndef _WIN64
    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "8A 87 ? ? ? ? 84 C0 0F 85 ? ? ? ? 8B 07 53 8B 5C 24 44");
    struct FMVHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *((uint8_t*)regs.edi + 0x150);

            auto x = *(int32_t*)(regs.esp + 0x48);
            auto y = *(int32_t*)(regs.esp + 0x4C);
            auto w = *(int32_t*)(regs.esp + 0x50);
            auto h = *(int32_t*)(regs.esp + 0x54);
            Screen.AdjustFMVRes(w, h);
        }
    }; injector::MakeInline<FMVHook>(pattern.get_first(0), pattern.get_first(6));
    #else
    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "48 8B C4 48 81 EC ? ? ? ? 80 B9 ? ? ? ? ? 0F 85 ? ? ? ? 48 89 58 F8");
    static auto FMVHook = safetyhook::create_mid(pattern.get_first(3), [](SafetyHookContext& regs)
    {
        auto w = *(int32_t*)(regs.rsp + 0x30);
        auto h = *(int32_t*)(regs.rsp + 0x38);
        Screen.AdjustFMVRes(w, h);
    });
    #endif

    //Language Switch (for controls)
    #ifndef _WIN64
    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "8B 83 ? ? ? ? 85 C0 75 16 A1");
    struct LayoutSwitch
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.ebx + 0x1B614);

            HKL* lpList = NULL;
            wchar_t szBuf[512];

            UINT uLayouts = GetKeyboardLayoutList(0, NULL);
            lpList = (HKL*)LocalAlloc(LPTR, (uLayouts * sizeof(HKL)));
            uLayouts = GetKeyboardLayoutList(uLayouts, lpList);

            for (int i = 0; i < uLayouts; ++i)
            {
                GetLocaleInfo(MAKELCID(((UINT)lpList[i] & 0xffffffff), SORT_DEFAULT), LOCALE_SLANGUAGE, szBuf, 512);
                if (wcsstr(szBuf, L"English") != NULL)
                {
                    PostMessage((HWND)regs.eax, WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM)LoadKeyboardLayout(std::to_wstring((UINT)lpList[i]).c_str(), KLF_ACTIVATE));
                    break;
                }
                memset(szBuf, 0, 512);
            }

            if (lpList)
                LocalFree(lpList);
        }
    }; injector::MakeInline<LayoutSwitch>(pattern.get_first(0), pattern.get_first(6));
    #else
    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "48 83 BF ? ? ? ? ? 4C 8B 7C 24");
    static auto LayoutSwitch = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto hWnd = *(HWND*)(regs.rdi + 0x1FD18);
        if (!hWnd)
            return;

        HKL* lpList = NULL;
        wchar_t szBuf[512];

        UINT uLayouts = GetKeyboardLayoutList(0, NULL);
        lpList = (HKL*)LocalAlloc(LPTR, (uLayouts * sizeof(HKL)));
        uLayouts = GetKeyboardLayoutList(uLayouts, lpList);

        for (int i = 0; i < uLayouts; ++i)
        {
            GetLocaleInfo(MAKELCID(((UINT)lpList[i] & 0xffffffff), SORT_DEFAULT), LOCALE_SLANGUAGE, szBuf, 512);
            if (wcsstr(szBuf, L"English") != NULL)
            {
                PostMessage(hWnd, WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM)LoadKeyboardLayout(std::to_wstring((UINT)lpList[i]).c_str(), KLF_ACTIVATE));
                break;
            }
            memset(szBuf, 0, 512);
        }

        if (lpList)
            LocalFree(lpList);
    });
    #endif
}

void InitCryGame()
{
    #ifndef _WIN64
    dword_3302ED49 = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "33 C0 40 5F EB 02 33 C0 5E C9 C2 10 00").get_first(0);
    dword_330D054C = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "D9 85 7C FF FF FF D8 25 ? ? ? ? 83 EC 28").get_first(0);
    dword_330D05E4 = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "8B 8B 80 06 00 00 8B 01 68 78 00 02 00").get_first(0);
    dword_330D06E5 = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "8B 4D DC 8B 01 FF 50 14 DC C0 83 EC 28").get_first(0);
    dword_330D0771 = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "8B 8B 80 06 00 00 8B 01 68 65 00 02 00 FF 90 38 02 00 00 D9 05 ? ? ? ? D8 15 ? ? ? ? DF E0").get_first(0);
    dword_330D0832 = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "8B 06 6A 01 8B CE FF 50 04 8B 8B 80 06 00 00").get_first(0);
    dword_330D0D61 = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "D9 05 ? ? ? ? 8B 4D E0 8B 75 D0 83 C1 20").get_first(0);
    dword_330D134A = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "EB 02 DD D8 8B 75 20 8B 0E 8B 01").get_first(0);
    dword_330193CE = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "8B 10 8B C8 FF 52 14 5E C2 08 00").get_first(7);
    dword_33038051 = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "8D 86 ? ? ? ? 83 38 FF 7E 2A D9 E8 6A 01").get_first(0);
    #else
    dword_1005DA88 = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "E9 ? ? ? ? F3 44 0F 10 35 ? ? ? ? 66 44 0F 12 3D ? ? ? ? F3 41 0F 5C F0 41 0F 2F F6").get_first(0);
    dword_10068D88 = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "48 83 BB ? ? ? ? ? 66 44 0F 6F 94 24 ? ? ? ? 66 44 0F 6F 8C 24 ? ? ? ? 66 0F 6F BC 24").get_first(0);

    dword_1018F2CE = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "F3 44 0F 10 0D ? ? ? ? 49 8B 4D 00 48 8B 01 FF 90 ? ? ? ? 84 C0").get_first(0);
    dword_1018DCD8 = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "F3 0F 10 BC 24 ? ? ? ? 44 0F 28 C7 F3 0F 10 0D ? ? ? ? F3 44 0F").get_first(0);
    dword_1018DD8F = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "49 8B 8C 24 ? ? ? ? 48 8B 01 BA ? ? ? ? FF 90 ? ? ? ? 8B 15 ? ? ? ? F6 C2 01").get_first(0);
    dword_1018DEA3 = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "49 8B 17 49 8B CF FF 52 28 F3 0F 59 05 ? ? ? ? F3 0F 10 2D ? ? ? ? F3 0F 58 E8").get_first(0);
    dword_1018DF57 = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "49 8B 8C 24 ? ? ? ? 48 8B 01 BA ? ? ? ? FF 90 ? ? ? ? F3 0F 10 2D ? ? ? ? F3 0F").get_first(0);
    dword_1018E00E = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "48 8B 06 B2 01 48 8B CE FF 50 08 48 8D 94 24 ? ? ? ? 8B 08 89 0A").get_first(0);
    dword_1018ECE0 = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "F3 44 0F 10 94 24 ? ? ? ? F3 0F 10 BC 24 ? ? ? ? F3 0F 10 B4 24").get_first(0);
    dword_1018EB80 = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "F3 0F 10 BC 24 ? ? ? ? F3 0F 10 B4 24 ? ? ? ? E9 ? ? ? ? 45 0F 28 C5 45 0F 28 D4").get_first(0);
    dword_1018E5B4 = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "F3 44 0F 10 9C 24 ? ? ? ? F3 44 0F 10 84 24").get_first(0);
    dword_1018F11C = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "E9 ? ? ? ? F3 0F 5E 15 ? ? ? ? F3 0F 10 05").get_first(0);
    dword_1018E4C6 = hook::make_module_pattern(GetModuleHandle(L"CryGame"), "F3 44 0F 10 84 24 84 00 00 00 E9 ? ? ? ? F3 44 0F 5E 05").get_first(0);
    #endif

    #ifndef _WIN64
    auto pattern = hook::module_pattern(GetModuleHandle(L"CryGame"), "8D 85 65 FF FF FF 89 45 08 D9 45 D8 D8 0D");
    struct HUDHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = regs.ebp - 0x9B;

            auto q = QUAD(*(float*)(regs.ebp - 0xA8), *(float*)(regs.ebp - 0xA4), *(float*)(regs.ebp - 0x90), *(float*)(regs.ebp - 0x8C),
                *(float*)(regs.ebp - 0x78), *(float*)(regs.ebp - 0x74), *(float*)(regs.ebp - 0x60), *(float*)(regs.ebp - 0x5C));

            Screen.bStretch = false;
            *(float*)(regs.ebp - 0xA8) /= Screen.fHudScale;
            *(float*)(regs.ebp - 0x90) /= Screen.fHudScale;
            *(float*)(regs.ebp - 0x78) /= Screen.fHudScale;
            *(float*)(regs.ebp - 0x60) /= Screen.fHudScale;

            *(float*)(regs.ebp - 0xA8) += Screen.fHudOffset;
            *(float*)(regs.ebp - 0x90) += Screen.fHudOffset;
            *(float*)(regs.ebp - 0x78) += Screen.fHudOffset;
            *(float*)(regs.ebp - 0x60) += Screen.fHudOffset;

            {
                DBGONLY(KEYPRESS(VK_F1) { spd::log()->info("{}, {}, {}, {}, {}, {}, {}, {}", q.ix1, q.iy1, q.ix2, q.iy2, q.ix3, q.iy3, q.ix4, q.iy4); });

                if (q.ix1 >= 551)
                {
                    *(float*)(regs.ebp - 0xA8) += Screen.fWidescreenHudOffset;
                    *(float*)(regs.ebp - 0x90) += Screen.fWidescreenHudOffset;
                    *(float*)(regs.ebp - 0x78) += Screen.fWidescreenHudOffset;
                    *(float*)(regs.ebp - 0x60) += Screen.fWidescreenHudOffset;
                }
                else if (q.ix1 <= 99)
                {
                    *(float*)(regs.ebp - 0xA8) -= Screen.fWidescreenHudOffset;
                    *(float*)(regs.ebp - 0x90) -= Screen.fWidescreenHudOffset;
                    *(float*)(regs.ebp - 0x78) -= Screen.fWidescreenHudOffset;
                    *(float*)(regs.ebp - 0x60) -= Screen.fWidescreenHudOffset;
                }
            }
        }
    }; injector::MakeInline<HUDHook>(pattern.get_first(0), pattern.get_first(6));
    #else
    auto pattern = hook::module_pattern(GetModuleHandle(L"CryGame"), "4C 8D A4 24 ? ? ? ? 4C 8D B4 24 ? ? ? ? 45 33 FF");
    static auto HUDHook = safetyhook::create_mid(pattern.get_first(19), [](SafetyHookContext& regs)
    {
        auto q = QUAD(*(float*)(regs.rsp + 0x80), *(float*)(regs.rsp + 0x7C), *(float*)(regs.rsp + 0x98), *(float*)(regs.rsp + 0x94),
            *(float*)(regs.rsp + 0xB0), *(float*)(regs.rsp + 0xAC), *(float*)(regs.rsp + 0xC8), *(float*)(regs.rsp + 0xC4));

        Screen.bStretch = false;
        *(float*)(regs.rsp + 0x80) /= Screen.fHudScale;
        *(float*)(regs.rsp + 0x98) /= Screen.fHudScale;
        *(float*)(regs.rsp + 0xB0) /= Screen.fHudScale;
        *(float*)(regs.rsp + 0xC8) /= Screen.fHudScale;

        *(float*)(regs.rsp + 0x80) += Screen.fHudOffset;
        *(float*)(regs.rsp + 0x98) += Screen.fHudOffset;
        *(float*)(regs.rsp + 0xB0) += Screen.fHudOffset;
        *(float*)(regs.rsp + 0xC8) += Screen.fHudOffset;

        if (q.ix1 >= 551)
        {
            *(float*)(regs.rsp + 0x80) += Screen.fWidescreenHudOffset;
            *(float*)(regs.rsp + 0x98) += Screen.fWidescreenHudOffset;
            *(float*)(regs.rsp + 0xB0) += Screen.fWidescreenHudOffset;
            *(float*)(regs.rsp + 0xC8) += Screen.fWidescreenHudOffset;
        }
        else if (q.ix1 <= 99)
        {
            *(float*)(regs.rsp + 0x80) -= Screen.fWidescreenHudOffset;
            *(float*)(regs.rsp + 0x98) -= Screen.fWidescreenHudOffset;
            *(float*)(regs.rsp + 0xB0) -= Screen.fWidescreenHudOffset;
            *(float*)(regs.rsp + 0xC8) -= Screen.fWidescreenHudOffset;
        }
    });
    #endif
}

#ifndef _WIN64
SafetyHookInline shSetCamera = {};
void __fastcall SetCamera(void* engine, void* edx, float* cam, char bToTheScreen)
{
    if (bToTheScreen && Screen.fFOVFactor > 0.0f)
        cam[12] *= Screen.fFOVFactor;      // fHudScale * fIniFOV = aspect/(4/3) * fIniFOV

    shSetCamera.unsafe_fastcall(engine, edx, cam, bToTheScreen);
}
#else
SafetyHookInline shSetCamera = {};
void __fastcall SetCamera(void* engine, float* cam, char bToTheScreen)
{
    if (bToTheScreen && Screen.fFOVFactor > 0.0f)
        cam[12] *= Screen.fFOVFactor;      // fHudScale * fIniFOV = aspect/(4/3) * fIniFOV

    shSetCamera.unsafe_fastcall(engine, cam, bToTheScreen);
}
#endif

void InitCry3DEngine()
{
    #ifndef _WIN64
    auto pattern = hook::module_pattern(GetModuleHandle(L"Cry3DEngine"), "74 ? 32 C0 5F 8B E5 5D C2");
    injector::MakeNOP(pattern.get_first(), 2, true);

    pattern = hook::module_pattern(GetModuleHandle(L"Cry3DEngine"), "83 EC ? 53 55 56 57 8B 7C 24");
    shSetCamera = safetyhook::create_inline(pattern.get_first(), SetCamera);
    #else
    auto pattern = hook::module_pattern(GetModuleHandle(L"Cry3DEngine"), "0F 87 ? ? ? ? 32 C0 66 44 0F 6F 84 24");
    injector::MakeNOP(pattern.get_first(), 6, true);

    pattern = hook::module_pattern(GetModuleHandle(L"Cry3DEngine"), "48 8B C4 48 81 EC ? ? ? ? 48 89 58 ? 48 89 68 ? 48 8B E9");
    shSetCamera = safetyhook::create_inline(pattern.get_first(), SetCamera);
    #endif
}

void InitCrySystem()
{
    #ifndef _WIN64
    dword_36552A15 = hook::make_module_pattern(GetModuleHandle(L"CrySystem"), "83 7D F8 00 74 10 8B 4F 1C 8B 01 68 00 01 00 00 FF 90 38 02 00 00 8B 06 8B CE FF 50 40 5F 5E 5B C9 C2 04 00 55 8B EC 83 EC 30").get_first(0);
    dword_365526AD = hook::make_module_pattern(GetModuleHandle(L"CrySystem"), "83 7D F8 00 74 10 8B 4F 1C 8B 01 68 00 01 00 00 FF 90 38 02 00 00").get(1).get<void>(0);
    #else
    dword_100B0A58 = hook::make_module_pattern(GetModuleHandle(L"CrySystem"), "83 BC 24 ? ? ? ? ? 74 12 48 8B 4E 38 BA ? ? ? ? 48 8B 01").count(4).get(1).get<void*>(0);
    dword_100B1018 = hook::make_module_pattern(GetModuleHandle(L"CrySystem"), "83 BC 24 ? ? ? ? ? 74 12 48 8B 4E 38 BA ? ? ? ? 48 8B 01").count(4).get(3).get<void*>(0);
    dword_100B72BB = hook::make_module_pattern(GetModuleHandle(L"CrySystem"), "48 8B CB 4C 63 C0 48 8B 05 ? ? ? ? 42 FF 14 C0 48 83 C4 20 5B C3").count(7).get(5).get<void*>(17);
    #endif
}

#ifndef _WIN64
SafetyHookInline shDrawStringW = {};
void __fastcall DrawStringW(void* a1, void* edx, float fBaseX, float fBaseY, const wchar_t* szMsg, const bool bASCIIMultiLine)
{
    if (!Screen.bUnstretchedText)
    {
        shDrawStringW.unsafe_fastcall(a1, edx, fBaseX, fBaseY, szMsg, bASCIIMultiLine);
        return;
    }

    const bool bOldStretch = Screen.bStretch;
    Screen.bStretch = true;

    shDrawStringW.unsafe_fastcall(a1, edx, fBaseX, fBaseY, szMsg, bASCIIMultiLine);

    Screen.bStretch = bOldStretch;
}
#else
SafetyHookInline shDrawStringW = {};
void __fastcall DrawStringW(void* a1, float fBaseX, float fBaseY, const wchar_t* szMsg, const bool bASCIIMultiLine)
{
    if (!Screen.bUnstretchedText)
    {
        shDrawStringW.unsafe_fastcall(a1, fBaseX, fBaseY, szMsg, bASCIIMultiLine);
        return;
    }

    const bool bOldStretch = Screen.bStretch;
    Screen.bStretch = true;

    shDrawStringW.unsafe_fastcall(a1, fBaseX, fBaseY, szMsg, bASCIIMultiLine);

    Screen.bStretch = bOldStretch;
}
#endif

void InitCryFont()
{
    #ifndef _WIN64
    auto pattern = hook::module_pattern(GetModuleHandle(L"CryFont"), "55 8D 6C 24 ? 81 EC ? ? ? ? 53 56");
    shDrawStringW = safetyhook::create_inline(pattern.get_first(), DrawStringW);
    #else
    auto pattern = hook::module_pattern(GetModuleHandle(L"CryFont"), "4C 89 4C 24 ? F3 0F 11 54 24");
    shDrawStringW = safetyhook::create_inline(pattern.get_first(), DrawStringW);
    #endif
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        Init();
        CallbackHandler::RegisterCallback(L"XRenderD3D9.dll", InitXRenderD3D9);
        CallbackHandler::RegisterCallback(L"CryGame.dll", InitCryGame);
        CallbackHandler::RegisterCallback(L"Cry3DEngine.dll", InitCry3DEngine);
        CallbackHandler::RegisterCallback(L"CrySystem.dll", InitCrySystem);
        CallbackHandler::RegisterCallback(L"CryFont.dll", InitCryFont);
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}