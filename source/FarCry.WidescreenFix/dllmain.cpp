#include "stdafx.h"
#ifdef _WIN64
#include "asm-x64/asm-x64.h"
#endif

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
    float fCutOffArea;
    bool bStretch;
    bool bWidescreenHud;
    float fIniHudOffset;
    float fWidescreenHudOffset;
    float fIniFOV;
    float fFOVFactor;

    void AdjustToRes(uint32_t x, uint32_t y)
    {
        this->nWidth = x;
        this->nHeight = y;
        this->fWidth = static_cast<float>(this->nWidth);
        this->fHeight = static_cast<float>(this->nHeight);
        this->fAspectRatio = this->fWidth / this->fHeight;
        this->nWidth43 = static_cast<uint32_t>(this->fHeight * (4.0f / 3.0f));
        this->fWidth43 = static_cast<float>(this->nWidth43);
        this->fHudOffsetReal = (this->fWidth - this->fHeight * (4.0f / 3.0f)) / 2.0f;
        this->fHudScale = 1.0f / (((4.0f / 3.0f)) / (this->fAspectRatio));
        this->fHudOffset = (((600.0f * this->fAspectRatio) - 800.0f) / 2.0f) / this->fHudScale;
        this->fFMVOffsetH = (((600.0f * this->fFMVAspectRatio) - 800.0f) / 2.0f) / this->fHudScale;
        this->fFMVOffsetV = (600.0f - (600.0f / (1.0f / (((4.0f / 3.0f)) / (this->fFMVAspectRatio))))) / 2.0f;
        this->fRadarVerticalOffset = this->fHudOffset * (4.0f / 3.0f);
        this->fCutOffArea = 0.5f * this->fHudScale;
        this->fFOVFactor = this->fHudScale * this->fIniFOV;
        this->fWidescreenHudOffset = fIniHudOffset / this->fHudScale;
        if (this->fAspectRatio < (16.0f / 9.0f))
            this->fWidescreenHudOffset = this->fWidescreenHudOffset / (((16.0f / 9.0f) / (this->fAspectRatio)) * 1.5f);
    }

    void AdjustFMVRes(uint32_t w, uint32_t h)
    {
        this->fFMVAspectRatio = static_cast<float>(w) / static_cast<float>(h);
        this->fFMVOffsetH = (((600.0f * this->fFMVAspectRatio) - 800.0f) / 2.0f) / this->fHudScale;
        this->fFMVOffsetV = (600.0f - (600.0f / (1.0f / (((4.0f / 3.0f)) / (this->fFMVAspectRatio))))) / 2.0f;
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
    {}

    inline bool operator==(const QUAD& rhs)
    {
        return this->ix1 == rhs.ix1 && this->iy1 == rhs.iy1 && this->ix2 == rhs.ix2 && this->iy2 == rhs.iy2;
    }
    inline bool operator!=(const QUAD& rhs) { return !(*this == rhs); }

    inline QUAD() {}
};

#ifdef _WIN64
#define JMPSIZE 14
#define CALLSIZE 16

#define pushad() \
{ \
    a.pushq(reg::rcx); \
    a.pushq(reg::rdx); \
    a.pushq(reg::r8);  \
    a.pushq(reg::r9);  \
    a.pushq(reg::rbx); \
    a.pushq(reg::rbp); \
    a.pushq(reg::rdi); \
    a.pushq(reg::rsi); \
    a.pushq(reg::rsp); \
    a.pushq(reg::r12); \
    a.pushq(reg::r13); \
    a.pushq(reg::r14); \
    a.pushq(reg::r15); \
    a.subq(0x28, reg::rsp); \
}

#define popad() \
{ \
    a.addq(0x28, reg::rsp); \
    a.popq(reg::r15); \
    a.popq(reg::r14); \
    a.popq(reg::r13); \
    a.popq(reg::r12); \
    a.popq(reg::rsp); \
    a.popq(reg::rsi); \
    a.popq(reg::rdi); \
    a.popq(reg::rbp); \
    a.popq(reg::rbx); \
    a.popq(reg::r9);  \
    a.popq(reg::r8);  \
    a.popq(reg::rdx); \
    a.popq(reg::rcx); \
}

inline injector::memory_pointer_raw MakeAbsCALL(injector::memory_pointer_tr at, injector::memory_pointer_raw dest, bool vp = true)
{
    injector::WriteMemory<uint16_t>(at, 0x15FF, vp);
    injector::WriteMemory<uint32_t>(at + sizeof(uint16_t), 2, vp);
    injector::WriteMemory<uint16_t>(at + sizeof(uint16_t) + sizeof(uint32_t), 0x08EB, vp);
    injector::WriteMemory<uint64_t>(at + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t), dest.as_int(), vp);
    return at.as_int() + CALLSIZE;
}

inline injector::memory_pointer_raw MakeAbsJMP(injector::memory_pointer_tr at, injector::memory_pointer_raw dest, bool vp = true)
{
    injector::WriteMemory<uint16_t>(at, 0x25FF, vp);
    injector::WriteMemory<uint32_t>(at + sizeof(uint16_t), 0, vp);
    injector::WriteMemory<uint64_t>(at + sizeof(uint16_t) + sizeof(uint32_t), dest.as_int(), vp);
    return at.as_int() + JMPSIZE;
}

template<class FuncT>
inline void MakeInlineJMP(injector::memory_pointer_tr at, injector::memory_pointer_raw dest, bool vp = true, bool areanop = true)
{
    assert((dest.as_int() - at.as_int()) >= JMPSIZE);
    if (areanop)
        injector::MakeRangedNOP(at, dest, vp);
    FuncT fun;
    auto[frontier, to] = fun();
    MakeAbsJMP(at, to, vp);
    MakeAbsJMP(frontier, dest, vp);
}
#endif

int WINAPI GetSystemMetricsHook(int nIndex)
{
    auto[DesktopResW, DesktopResH] = GetDesktopRes();

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
#endif

void
#ifndef _WIN64
__stdcall
#else
__fastcall
#endif
sub_380E58B5(float *a1, float a2, float a3, float a4, float a5, float a6, float a7)
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
    Screen.bWidescreenHud = iniReader.ReadInteger("MAIN", "WidescreenHud", 1) != 0;
    Screen.fIniHudOffset = iniReader.ReadFloat("MAIN", "WidescreenHudOffset", 130.0f);
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
    struct TextHook
    {
        std::tuple<Address, void*> operator()()
        {
            static uint8_t buffer[100];
            injector::ProtectMemory(buffer, sizeof(buffer), PAGE_EXECUTE_READWRITE);
            CodeBlock cb; cb.init((Address)buffer, sizeof(buffer));
            X64Assembler a(cb);

            cb.setFrontier(Address(MakeAbsCALL(cb.frontier(), sub_380E58B5, true).as_int())); //_asm call sub_1008AED0
            a.movq(reg::rdi[0x20E58], reg::rcx); // _asm mov     rcx, [rdi + 20E58h]
            a.movq(reg::rbx, reg::r8);           // _asm mov     r8, rbx
            a.movq(reg::rcx[0], reg::rax);       // _asm mov     rax, [rcx]
            a.movl(3, reg::edx);                 // _asm mov     edx, 3

            assert(sizeof(buffer) > ((cb.frontier() + JMPSIZE) - cb.base()));
            return std::make_tuple(cb.frontier(), &buffer);
        }
    }; MakeInlineJMP<TextHook>(pattern.get_first(0), pattern.get_first(23)); //0x10027B06, 0x10027B1D

    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "E8 ? ? ? ? 48 8B 8D ? ? ? ? 48 8B 01 4C 8B C3 BA ? ? ? ? FF 90 ? ? ? ? 48 8B 8D");
    struct Hook2D
    {
        std::tuple<Address, void*> operator()()
        {
            static uint8_t buffer[100];
            injector::ProtectMemory(buffer, sizeof(buffer), PAGE_EXECUTE_READWRITE);
            CodeBlock cb; cb.init((Address)buffer, sizeof(buffer));
            X64Assembler a(cb);

            cb.setFrontier(Address(MakeAbsCALL(cb.frontier(), sub_380E58B5, true).as_int())); //_asm call sub_1008AED0
            a.movq(reg::rbp[0x20E58], reg::rcx); // _asm mov     rcx, [rbp+20E58h]
            a.movq(reg::rbx, reg::r8);           // _asm mov     r8, rbx
            a.movq(reg::rcx[0], reg::rax);       // _asm mov     rax, [rcx]
            a.movl(3, reg::edx);                 // _asm mov     edx, 3

            assert(sizeof(buffer) > ((cb.frontier() + JMPSIZE) - cb.base()));
            return std::make_tuple(cb.frontier(), &buffer);
        }
    }; MakeInlineJMP<Hook2D>(pattern.get_first(0), pattern.get_first(23)); //0x1002199B, 0x100219B2

    /*
    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "E8 ? ? ? ? 48 8B 8E ? ? ? ? 4C 8B C3 48 8B 01 BA ? ? ? ? FF 90 ? ? ? ? 48 8B 8E");
    struct Hook3D
    {
        std::tuple<Address, void*> operator()()
        {
            static uint8_t buffer[100];
            injector::ProtectMemory(buffer, sizeof(buffer), PAGE_EXECUTE_READWRITE);
            CodeBlock cb; cb.init((Address)buffer, sizeof(buffer));
            X64Assembler a(cb);

            cb.setFrontier(Address(MakeAbsCALL(cb.frontier(), sub_1008AED0, true).as_int())); //_asm call sub_1008AED0
            a.movq(reg::rsi[0x20E58], reg::rcx); // _asm mov     rcx, [rsi + 20E58h]
            a.movq(reg::rbx, reg::r8);           // _asm mov     r8, rbx
            a.movq(reg::rcx[0], reg::rax);       // _asm mov     rax, [rcx]
            a.movl(3, reg::edx);                 // _asm mov     edx, 3

            assert(sizeof(buffer) > ((cb.frontier() + JMPSIZE) - cb.base()));
            return std::make_tuple(cb.frontier(), &buffer);
        }
    }; MakeInlineJMP<Hook3D>(pattern.get_first(0), pattern.get_first(23)); //0x1003FBFB, 0x1003FC12
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
            DrawImage((void*)_this, 0.0f, 0.0f, Screen.fHudOffset, 600.0f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.5f);
            DrawImage((void*)_this, 800.0f - Screen.fHudOffset, 0.0f, Screen.fHudOffset, 600.0f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.5f);
            Screen.bStretch = true;
        }
    };
#else
    //static auto DrawBorders = [](uint64_t _this)
    //{
    //    if (Screen.bStretch)
    //    {
    //        Screen.bStretch = false;
    //        Screen.bStretch = true;
    //    }
    //};
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
    struct SetResHook
    {
        std::tuple<Address, void*> operator()()
        {
            static uint8_t buffer[100];
            injector::ProtectMemory(buffer, sizeof(buffer), PAGE_EXECUTE_READWRITE);
            CodeBlock cb; cb.init((Address)buffer, sizeof(buffer));
            X64Assembler a(cb);

            a.movl(reg::r9d, reg::rcx[0x17BF0]); //_asm mov[rcx + 17BF0h], r9d
            a.movl(reg::r8d, reg::rcx[0x17BF4]); //_asm mov[rcx + 17BF4h], r8d
            a.movl(reg::edx, reg::rcx[0x17BE8]); //_asm mov[rcx + 17BE8h], edx

            a.pushq(reg::rax);
            a.pushq(reg::rdx);
            a.pushq(reg::rcx);
            a.pushq(reg::r8);
            a.pushq(reg::r9);

            a.movl(reg::r9d, reg::ecx);
            a.movl(reg::r8d, reg::edx);
            a.movq((int64_t)static_cast<void(*)(uint32_t, uint32_t)>([](uint32_t w, uint32_t h)
            {
                Screen.AdjustToRes(w, h);
            }
            ), reg::rax);
            a.callq(reg::rax);

            a.popq(reg::r9);
            a.popq(reg::r8);
            a.popq(reg::rcx);
            a.popq(reg::rdx);
            a.popq(reg::rax);

            a.testq(reg::rbx, reg::rbx); // _asm test    rbx, rbx

            assert(sizeof(buffer) > ((cb.frontier() + JMPSIZE) - cb.base()));
            return std::make_tuple(cb.frontier(), &buffer);
        }
    }; MakeInlineJMP<SetResHook>(pattern.get_first(0), pattern.get_first(20)); //0x1012B5A1, 0x1012B5B5

    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "8B 8B ? ? ? ? 89 48 10 8B 8B ? ? ? ? 89 48 14");
    struct SetResHook2
    {
        std::tuple<Address, void*> operator()()
        {
            static uint8_t buffer[100];
            injector::ProtectMemory(buffer, sizeof(buffer), PAGE_EXECUTE_READWRITE);
            CodeBlock cb; cb.init((Address)buffer, sizeof(buffer));
            X64Assembler a(cb);

            a.movl(reg::rbx[0x17BC8], reg::ecx); //_asm mov ecx, [rbx + 17BC8h]
            a.movl(reg::ecx, reg::rax[0x10]); //_asm mov[rax + 10h], ecx
            a.movl(reg::rbx[0x17BCC], reg::ecx); //_asm mov ecx, [rbx + 17BCCh]
            a.movl(reg::ecx, reg::rax[0x14]); //_asm mov[rax + 14h], ecx

            a.pushq(reg::rax);
            a.pushq(reg::rdx);
            a.pushq(reg::rcx);
            a.pushq(reg::r8);
            a.pushq(reg::r9);

            a.movl(reg::rax[0x10], reg::ecx);
            a.movl(reg::rax[0x14], reg::edx);
            a.movq((int64_t)static_cast<void(*)(uint32_t, uint32_t)>([](uint32_t w, uint32_t h)
            {
                Screen.AdjustToRes(w, h);
            }
            ), reg::rax);
            a.callq(reg::rax);

            a.popq(reg::r9);
            a.popq(reg::r8);
            a.popq(reg::rcx);
            a.popq(reg::rdx);
            a.popq(reg::rax);

            assert(sizeof(buffer) > ((cb.frontier() + JMPSIZE) - cb.base()));
            return std::make_tuple(cb.frontier(), &buffer);
        }
    }; MakeInlineJMP<SetResHook2>(pattern.get_first(0), pattern.get_first(18)); //0x100852B9, 0x100852CB
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

                if (Screen.bWidescreenHud)
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

                if (Screen.bWidescreenHud)
                {
                    *(float*)(regs.esp + 0x30) -= Screen.fWidescreenHudOffset;
                }
            }
            else
            {
                if (ret == dword_365526AD) // Objectives window on Tab
                {
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
    struct Draw_2DImageHook
    {
        std::tuple<Address, void*> operator()()
        {
            static float x1, y1, x2, y2;
            static uint64_t _rcx;

            static uint8_t buffer[300];
            injector::ProtectMemory(buffer, sizeof(buffer), PAGE_EXECUTE_READWRITE);
            CodeBlock cb; cb.init((Address)buffer, sizeof(buffer));
            X64Assembler a(cb);

            pushad();

            a.movq((int64_t)&_rcx, reg::rax);
            a.movq(reg::rcx, reg::rax[0]);
            a.movq((int64_t)&x1, reg::rax);
            a.movss(reg::xmm1, reg::rax[0]);
            a.movq((int64_t)&y1, reg::rax);
            a.movss(reg::xmm2, reg::rax[0]);
            a.movq((int64_t)&x2, reg::rax);
            a.movss(reg::xmm3, reg::rax[0]);
            a.movq((int64_t)&y2, reg::rax);
            a.movss(reg::rsp[0x28 + 0x90], reg::xmm1);
            a.movss(reg::xmm1, reg::rax[0]);

            a.movq(reg::rsp, reg::rcx);
            a.movq((int64_t)static_cast<void(*)(int64_t)>([](int64_t ptr)
            {
                Screen.bStretch = false;

                void* ret = *(void**)(ptr + 0x90);
                void* ret2 = nullptr;
                void* ret3 = nullptr;
                if (ret == dword_100B1018)
                {
                    ret2 = *(void**)(ptr + 0x160);
                    ret3 = *(void**)(ptr + 0x160 + 0x30);
                }
                else if (ret == dword_1005DA88)
                {
                    ret2 = *(void**)(ptr + 0x1F0);
                }

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
                    return;
                }

                if (ret == dword_1018DCD8 || ret == dword_1018DD8F || ret == dword_1018DEA3 || ret == dword_1018DF57)
                {
                    x1 /= Screen.fHudScale;
                    y1 /= Screen.fHudScale;
                    x2 /= Screen.fHudScale;
                    y2 /= Screen.fHudScale;

                    x1 += Screen.fHudOffset;
                    y1 += Screen.fRadarVerticalOffset;
                    Screen.bStretch = false;

                    if (Screen.bWidescreenHud)
                    {
                        x1 -= Screen.fWidescreenHudOffset;
                    }
                }
                else if (ret == dword_1018F2CE || ret == dword_1018E00E || ret == dword_1018ECE0 || ret == dword_1018EB80 || ret == dword_1018E5B4 || ret == dword_1018F11C)
                {
                    x1 /= Screen.fHudScale;
                    y1 /= Screen.fHudScale;
                    x2 /= Screen.fHudScale;

                    x1 += Screen.fHudOffset;
                    y1 += Screen.fRadarVerticalOffset;
                    Screen.bStretch = false;

                    if (Screen.bWidescreenHud)
                    {
                        x1 -= Screen.fWidescreenHudOffset;
                    }
                }
                else
                {
                    if (ret == dword_100B0A58) // Objectives window on Tab
                    {
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
                            //DrawBorders(_rcx);
                        }
                    }
                    else
                    {
                        Screen.bStretch = true;
                        //DrawBorders(_rcx);
                    }
                }
            }
            ), reg::rax);
            a.callq(reg::rax);

            a.movq((int64_t)&x1, reg::rax);
            a.movss(reg::rax[0], reg::xmm1);
            a.movq((int64_t)&y1, reg::rax);
            a.movss(reg::rax[0], reg::xmm2);
            a.movq((int64_t)&x2, reg::rax);
            a.movss(reg::rax[0], reg::xmm3);
            a.movq((int64_t)&y2, reg::rax);
            a.movq(reg::rax[0], reg::rax);
            a.movq(reg::rax, reg::rsp[0x28 + 0x90]);

            popad();

            a.movq(reg::rsp, reg::rax); // _asm mov rax, rsp
            a.subq(0x138, reg::rsp);    // _asm sub rsp, 138h
            a.movq(-2, reg::rsp[0x58]); // _asm mov qword ptr [rsp+58h], -2

            assert(sizeof(buffer) > ((cb.frontier() + JMPSIZE) - cb.base()));
            return std::make_tuple(cb.frontier(), &buffer);
        }
    }; MakeInlineJMP<Draw_2DImageHook>(pattern.get_first(0), pattern.get_first(19)); //0x10021730, 0x10021743
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
    struct DrawLineHook
    {
        std::tuple<Address, void*> operator()()
        {
            static uint8_t buffer[100];
            injector::ProtectMemory(buffer, sizeof(buffer), PAGE_EXECUTE_READWRITE);
            CodeBlock cb; cb.init((Address)buffer, sizeof(buffer));
            X64Assembler a(cb);

            a.movl(reg::eax, reg::r11[0x18]); //_asm mov [r11+18h], eax
            a.movl(reg::rdi[0x4], reg::eax);  //_asm mov eax, [rdi+4]
            a.movl(reg::eax, reg::r11[0x1C]); //_asm mov [r11+1Ch], eax
            a.movl(reg::rdi[0x8], reg::eax);  //_asm mov eax, [rdi+8]
            a.movl(reg::ebp, reg::r11[0x24]); //_asm mov [r11+24h], ebp
            a.movl(reg::eax, reg::r11[0x20]); //_asm mov [r11+20h], eax

            a.pushq(reg::rcx);
            a.movq(reg::r11, reg::rcx);
            a.movq((int64_t)static_cast<void(*)(uintptr_t)>([](uintptr_t ptr)
            {
                *(float*)(ptr + 0x00) /= Screen.fHudScale;
                *(float*)(ptr + 0x00) += Screen.fHudOffset;
                *(float*)(ptr + 0x18) /= Screen.fHudScale;
                *(float*)(ptr + 0x18) += Screen.fHudOffset;
            }
            ), reg::rax);
            a.callq(reg::rax);
            a.popq(reg::rcx);

            assert(sizeof(buffer) > ((cb.frontier() + JMPSIZE) - cb.base()));
            return std::make_tuple(cb.frontier(), &buffer);
        }
    }; MakeInlineJMP<DrawLineHook>(pattern.get_first(0), pattern.get_first(22)); //0x10020E6B, 0x10020E81
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
    struct FMVHook
    {
        std::tuple<Address, void*> operator()()
        {
            static uint8_t buffer[200];
            injector::ProtectMemory(buffer, sizeof(buffer), PAGE_EXECUTE_READWRITE);
            CodeBlock cb; cb.init((Address)buffer, sizeof(buffer));
            X64Assembler a(cb);

            pushad();

            a.movq(reg::rsp[0xC0], reg::rcx);
            a.movq(reg::rsp[0xC8], reg::rdx);
            a.movq((int64_t)static_cast<void(*)(int64_t, int64_t)>([](int64_t w, int64_t h)
            {
                Screen.AdjustFMVRes(w, h);
            }
            ), reg::rax);
            a.callq(reg::rax);

            popad();

            a.movq(reg::rsp, reg::rax); // _asm mov rax, rsp
            a.subq(0x0D8, reg::rsp);    // _asm sub rsp, 0D8h
            a.cmpb(0, reg::rcx[0x150]); // _asm cmp byte ptr [rcx+150h], 0

            assert(sizeof(buffer) > ((cb.frontier() + JMPSIZE) - cb.base()));
            return std::make_tuple(cb.frontier(), &buffer);
        }
    }; MakeInlineJMP<FMVHook>(pattern.get_first(0), pattern.get_first(17)); //0x1006A920, 0x1006A931
#endif

    //FOV
#ifndef _WIN64
    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "D9 45 30 D8 4D 48 D9 1C 24 56 E8 ? ? ? ? 8B 83 ? ? ? ? 8B 08 56 6A 03");
    struct FOVHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float f1 = *(float*)(regs.ebp + 0x30);
            float f2 = *(float*)(regs.ebp + 0x48);
            _asm {fld dword ptr[f1]}
            _asm {fmul dword ptr[f2]}
            _asm {fmul dword ptr[Screen.fFOVFactor]}
        }
    }; injector::MakeInline<FOVHook>(pattern.get_first(0), pattern.get_first(6));
#else
    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "F3 0F 59 4E 30 F3 0F 11 44 24 20 0F 28 D6");
    struct FOVHook
    {
        std::tuple<Address, void*> operator()()
        {
            static uint8_t buffer[100];
            injector::ProtectMemory(buffer, sizeof(buffer), PAGE_EXECUTE_READWRITE);
            CodeBlock cb; cb.init((Address)buffer, sizeof(buffer));
            X64Assembler a(cb);

            a.pushq(reg::rax);
            a.movq((int64_t)&Screen.fFOVFactor, reg::rax);
            a.mulss(reg::rax[0], reg::xmm1);    //_asm mulss   xmm1,[Screen.fFOVFactor]
            a.popq(reg::rax);

            a.mulss(reg::rsi[0x30], reg::xmm1); //_asm mulss   xmm1, dword ptr [rsi+30h]
            a.movss(reg::xmm0, reg::rsp[0x20]); //_asm movss   dword ptr [rsp+20h], xmm0
            a.movaps(reg::xmm6, reg::xmm2);     //_asm movaps  xmm2, xmm6

            assert(sizeof(buffer) > ((cb.frontier() + JMPSIZE) - cb.base()));
            return std::make_tuple(cb.frontier(), &buffer);
        }
    }; MakeInlineJMP<FOVHook>(pattern.get_first(0), pattern.get_first(14)); //0x100429DE, 0x100429EC
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

            if (Screen.bWidescreenHud)
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

    //pattern = hook::module_pattern(GetModuleHandle(L"CryGame"), "8B 45 10 89 41 30 8B 45 1C D9 51 40");
    //struct FOVHook
    //{
    //    void operator()(injector::reg_pack& regs)
    //    {
    //        //auto f = *(float*)(regs.ebp + 0x10) / (M_PI / 180.0);
    //        *(float*)(regs.ecx + 0x30) = AdjustFOV(f, Screen.fAspectRatio) * (M_PI / 180.0);
    //    }
    //}; injector::MakeInline<FOVHook>(pattern.get_first(0), pattern.get_first(6));
#else
    auto pattern = hook::module_pattern(GetModuleHandle(L"CryGame"), "4C 8D A4 24 ? ? ? ? 4C 8D B4 24 ? ? ? ? 45 33 FF");
    struct HUDHook
    {
        std::tuple<Address, void*> operator()()
        {
            static uint8_t buffer[100];
            injector::ProtectMemory(buffer, sizeof(buffer), PAGE_EXECUTE_READWRITE);
            CodeBlock cb; cb.init((Address)buffer, sizeof(buffer));
            X64Assembler a(cb);

            a.pushq(reg::rax);
            a.pushq(reg::rbx);
            a.pushq(reg::rcx);

            a.movq(reg::rsp, reg::rcx);
            a.movq((int64_t)static_cast<void(*)(uintptr_t)>([](uintptr_t ptr)
            {
                auto q = QUAD(*(float*)(ptr + 0x98), *(float*)(ptr + 0x94), *(float*)(ptr + 0xB0), *(float*)(ptr + 0xAC),
                    *(float*)(ptr + 0xC8), *(float*)(ptr + 0xC4), *(float*)(ptr + 0xE0), *(float*)(ptr + 0xDC));

                Screen.bStretch = false;
                *(float*)(ptr + 0x98) /= Screen.fHudScale;
                *(float*)(ptr + 0xB0) /= Screen.fHudScale;
                *(float*)(ptr + 0xC8) /= Screen.fHudScale;
                *(float*)(ptr + 0xE0) /= Screen.fHudScale;

                *(float*)(ptr + 0x98) += Screen.fHudOffset;
                *(float*)(ptr + 0xB0) += Screen.fHudOffset;
                *(float*)(ptr + 0xC8) += Screen.fHudOffset;
                *(float*)(ptr + 0xE0) += Screen.fHudOffset;

                if (Screen.bWidescreenHud)
                {
                    if (q.ix1 >= 551)
                    {
                        *(float*)(ptr + 0x98) += Screen.fWidescreenHudOffset;
                        *(float*)(ptr + 0xB0) += Screen.fWidescreenHudOffset;
                        *(float*)(ptr + 0xC8) += Screen.fWidescreenHudOffset;
                        *(float*)(ptr + 0xE0) += Screen.fWidescreenHudOffset;
                    }
                    else if (q.ix1 <= 99)
                    {
                        *(float*)(ptr + 0x98) -= Screen.fWidescreenHudOffset;
                        *(float*)(ptr + 0xB0) -= Screen.fWidescreenHudOffset;
                        *(float*)(ptr + 0xC8) -= Screen.fWidescreenHudOffset;
                        *(float*)(ptr + 0xE0) -= Screen.fWidescreenHudOffset;
                    }
                }
            }
            ), reg::rax);
            a.callq(reg::rax);

            a.popq(reg::rcx);
            a.popq(reg::rbx);
            a.popq(reg::rax);

            a.leaq(reg::rsp[0x8D], reg::r12);   //_asm lea     r12, [rsp + 8Dh]
            a.leaq(reg::rsp[0x80], reg::r14);   //_asm lea     r14, [rsp + 80h]
            a.xorl(reg::r15d, reg::r15d);       //_asm xor     r15d, r15d

            assert(sizeof(buffer) > ((cb.frontier() + JMPSIZE) - cb.base()));
            return std::make_tuple(cb.frontier(), &buffer);
        }
    }; MakeInlineJMP<HUDHook>(pattern.get_first(0), pattern.get_first(19)); //0x100AF490, 0x100AF4A3
#endif
}

void InitCry3DEngine()
{
#ifndef _WIN64
    auto pattern = hook::module_pattern(GetModuleHandle(L"Cry3DEngine"), "D8 0D ? ? ? ? D9 C0 D9 FF D9 C9 D9 FE D9 C9 D9 C9 8D B4 24");
    injector::WriteMemory(pattern.get_first(2), &Screen.fCutOffArea, true);
#else
    auto pattern = hook::module_pattern(GetModuleHandle(L"Cry3DEngine"), "F3 0F 11 43 58 F3 0F 10 43 48 F3 0F 59 43 30");
    struct CutOffAreaHook1
    {
        std::tuple<Address, void*> operator()()
        {
            static uint8_t buffer[100];
            injector::ProtectMemory(buffer, sizeof(buffer), PAGE_EXECUTE_READWRITE);
            CodeBlock cb; cb.init((Address)buffer, sizeof(buffer));
            X64Assembler a(cb);

            a.movss(reg::xmm0, reg::rbx[0x58]); //_asm movss   dword ptr[rbx + 58h], xmm0
            a.movss(reg::rbx[0x48], reg::xmm0); //_asm movss   xmm0, dword ptr[rbx + 48h]
            a.mulss(reg::rbx[0x30], reg::xmm0); //_asm mulss   xmm0, dword ptr[rbx + 30h]
            a.mulss(reg::xmm7, reg::xmm0);      //_asm mulss   xmm0, xmm7

            a.pushq(reg::rax);
            a.movq((int64_t)&Screen.fCutOffArea, reg::rax);
            a.divss(reg::rax[0], reg::xmm0);    //_asm mulss   xmm0,[Screen.fCutOffArea]
            a.popq(reg::rax);

            assert(sizeof(buffer) > ((cb.frontier() + JMPSIZE) - cb.base()));
            return std::make_tuple(cb.frontier(), &buffer);
        }
    }; MakeInlineJMP<CutOffAreaHook1>(pattern.get_first(0), pattern.get_first(19)); //0x20050ACB, 0x20050ADE

    pattern = hook::module_pattern(GetModuleHandle(L"Cry3DEngine"), "F3 0F 10 43 48 F3 0F 59 43 30 F3 0F 59 C7");
    struct CutOffAreaHook2
    {
        std::tuple<Address, void*> operator()()
        {
            static uint8_t buffer[100];
            injector::ProtectMemory(buffer, sizeof(buffer), PAGE_EXECUTE_READWRITE);
            CodeBlock cb; cb.init((Address)buffer, sizeof(buffer));
            X64Assembler a(cb);

            a.movss(reg::rbx[0x48], reg::xmm0); //_asm movss   xmm0, dword ptr[rbx + 48h]
            a.mulss(reg::rbx[0x30], reg::xmm0); //_asm mulss   xmm0, dword ptr[rbx + 30h]
            a.mulss(reg::xmm7, reg::xmm0);      //_asm mulss   xmm0, xmm7

            a.pushq(reg::rax);
            a.movq((int64_t)&Screen.fCutOffArea, reg::rax);
            a.divss(reg::rax[0], reg::xmm0);    //_asm mulss   xmm0,[Screen.fCutOffArea]
            a.popq(reg::rax);

            assert(sizeof(buffer) > ((cb.frontier() + JMPSIZE) - cb.base()));
            return std::make_tuple(cb.frontier(), &buffer);
        }
    }; MakeInlineJMP<CutOffAreaHook2>(pattern.get_first(0), pattern.get_first(14)); //0x20050AE6, 0x20050AF4
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

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        Init();
        CallbackHandler::RegisterCallback(L"XRenderD3D9.dll", InitXRenderD3D9);
        CallbackHandler::RegisterCallback(L"CryGame.dll", InitCryGame);
        CallbackHandler::RegisterCallback(L"Cry3DEngine.dll", InitCry3DEngine);
        CallbackHandler::RegisterCallback(L"CrySystem.dll", InitCrySystem);
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {

    }
    return TRUE;
}