#include "stdafx.h"

//#define _LOG
#ifdef _LOG
#include <fstream>
#include <iomanip>
std::ofstream logfile;
uint32_t logit;
#endif // _LOG

struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int32_t nWidth43;
    float fWidth43;
    float fHudScale;
    float fHudScale2;
    float fHudOffset;
    float fHudOffsetReal;
    float fHudOffsetWide;
} Screen;

class CRect
{
public:
    float m_fLeft;
    float m_fBottom;
    float m_fRight;
    float m_fTop;

    bool operator==(const CRect& rect) {
        return ((*(uint32_t*)&this->m_fLeft == *(uint32_t*)&rect.m_fLeft) && (*(uint32_t*)&this->m_fBottom == *(uint32_t*)&rect.m_fBottom) &&
            (*(uint32_t*)&this->m_fRight == *(uint32_t*)&rect.m_fRight) && (*(uint32_t*)&this->m_fTop == *(uint32_t*)&rect.m_fTop));
    }

    inline CRect() {}
    inline CRect(float a, float b, float c, float d)
        : m_fLeft(a), m_fBottom(b), m_fRight(c), m_fTop(d)
    {}
    inline CRect(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
        : m_fLeft(*(float*)&a), m_fBottom(*(float*)&b), m_fRight(*(float*)&c), m_fTop(*(float*)&d)
    {}
};

struct MemFloat
{
    float a01 = 1.0f;
    float a02 = 0.0f;
    float a03 = 0.0f;
    float a04 = 0.0f;
    float a05 = 0.0f;
    float a06 = 1.0f;
    float a07 = 0.0f;
    float a08 = 0.0f;
    float a09 = 0.0f;
    float a10 = 0.0f;
    float a11 = 1.0f;
    float a12 = 0.0f;
    float a13 = 0.0f;
    float a14 = 0.0f;
    float a15 = 0.0f;
    float a16 = 1.0f;
} flt_7933E0;

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("8B 82 C8 02 00 00 89 81 94 05 00 00 8B 4D F8");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    CIniReader iniReader("");
    static bool bWidescreenHud = iniReader.ReadInteger("MAIN", "WidescreenHud", 1) != 0;
    static float fWidescreenHudOffset = iniReader.ReadFloat("MAIN", "WidescreenHudOffset", 100.0f);
    bool bAltTab = iniReader.ReadInteger("MAIN", "AllowAltTabbingWithoutPausing", 0) != 0;

    struct ResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(int32_t*)(regs.ecx + 0x594) = *(int32_t*)(regs.edx + 0x2C8);
            Screen.nWidth =  *(int32_t*)(regs.ecx + 0x590);
            Screen.nHeight = *(int32_t*)(regs.ecx + 0x594);

            Screen.fWidth = static_cast<float>(Screen.nWidth);
            Screen.fHeight = static_cast<float>(Screen.nHeight);
            Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
            Screen.nWidth43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
            Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
            Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
            Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
            Screen.fHudScale = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
            Screen.fHudScale2 = 640.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
            Screen.fHudOffsetWide = fWidescreenHudOffset;
            if (Screen.fAspectRatio < (16.0f / 9.0f))
                Screen.fHudOffsetWide = fWidescreenHudOffset / (((16.0f / 9.0f) / (Screen.fAspectRatio)) * 1.5f);
        }
    }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(12)); //0x67F958

    //Default resolution
    pattern = hook::pattern("A1 ? ? ? ? 50 8B 4D 84 E8 ? ? ? ? 6A 00 68");
    int32_t defX = 0, defY = 0;
    std::tie(defX, defY) = GetDesktopRes();
    injector::WriteMemory(pattern.get_first(-4), defX, true); //497822
    injector::WriteMemory(pattern.get_first(17), defY, true);

    //Hud
    pattern = hook::pattern("8B 0D ? ? ? ? 89 4D E0 8B 15 ? ? ? ? 89 55 E4 D9 05");
    struct HudHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.ebp - 0x20) = Screen.fHudOffset; //x
            *(float*)(regs.ebp - 0x1C) = 0.0f;              //y
            *(float*)(regs.ebp - 0x14) = Screen.fHudScale2; //w
            *(float*)(regs.ebp - 0x10) = 480.0f;            //h
        }
    }; injector::MakeInline<HudHook>(pattern.get_first(0), pattern.get_first(36)); //0x47D5B8, 0x47D5DC

    // xrefs
    static auto dw_61F2EF = hook::get_pattern("EB ? 8B 85 14 FF FF FF 8B 08 E8", 0);// 0x0061F2EF;
    static auto dw_61EA8F = hook::get_pattern("8B 95 14 FF FF FF 8B 4A 1C E8 ? ? ? ? 8B", 0); // 0x0061EA8F
    static auto dw_616FB7 = hook::get_pattern("8B 45 C4 83 C0 01 89 45 C4 8B 4D C8", 0); // 0x00616FB7
    static auto dw_4FD37D = hook::get_pattern("8B 8D 28 FA FF FF 83 79 18 00 74", 0); // 0x004FD37D
    static auto dw_61EAC4 = hook::get_pattern("8B 8D 14 FF FF FF 8B 51 10 8B 85 14", 0); // 0x0061EAC4
    static auto dw_61EADD = hook::get_pattern("8B 8D 14 FF FF FF 8B 51 0C 8B 85 14", 0); // 0x0061EADD
    static auto dw_61EAF6 = hook::get_pattern("C7 45 D8 00 00 00 00 8B 0D ? ? ? ? 8B 91 8C", 0); // 0x0061EAF6
    static auto dw_61F2FE = hook::get_pattern("8B 85 14 FF FF FF 8B 08 E8 ? ? ? ? 8B E5 5D C3", 13); // 0x0061F2FE

    pattern = hook::pattern("BE ? ? ? ? 8D BC 24 00 01 00 00 F3 A5 83 7D 20 00");
    injector::WriteMemory(pattern.get_first(1), &flt_7933E0, true); //0x47CC71 + 1

    pattern = hook::pattern("89 8C 24 AC 01 00 00 83 7D 08 00");
    struct HudHook2
    {
        const CRect fs = CRect(0.0f, 0.0f, 640.0f, 480.0f); // fullscreen images
        const CRect fl = CRect(0.0f, 0.0f, 20.0f, 15.0f);   // strike flash

        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.esp + 0x1AC) = regs.ecx;

            flt_7933E0.a01 = 1.0f;
            flt_7933E0.a13 = 0.0f;

            auto ptr = *(uint32_t*)(regs.ebp + 0x24);
            if (ptr)
            {
                CRect* r = (CRect*)(ptr - 0x08);

                #ifdef _LOG
                if (false)
                    logfile << r->m_fLeft << " " << r->m_fBottom << " " << r->m_fRight << " " << r->m_fTop << " - "
                    << "0x" << std::hex << std::setw(8) << std::setfill('0') << *(uint32_t*)&r->m_fLeft << " "
                    << "0x" << std::hex << std::setw(8) << std::setfill('0') << *(uint32_t*)&r->m_fBottom << " "
                    << "0x" << std::hex << std::setw(8) << std::setfill('0') << *(uint32_t*)&r->m_fRight << " "
                    << "0x" << std::hex << std::setw(8) << std::setfill('0') << *(uint32_t*)&r->m_fTop
                    << std::endl;
                #endif

                if (*r == fs || *r == fl)
                {
                    flt_7933E0.a01 = Screen.fHudScale;
                    flt_7933E0.a13 = -Screen.fHudOffset;
                }
                else
                {
                    if (bWidescreenHud)
                    {
                        static auto idx = 0;
                        static void* stack[6];
                        CaptureStackBackTrace(0, 6, stack, NULL);

                        auto it = std::find_if(std::begin(stack), std::end(stack), [](void* n) { return n == dw_61F2EF || n == dw_61F2FE; });
                        if (!idx && it != std::end(stack))
                            idx = std::distance(std::begin(stack), it);

                        if (idx)
                        {
                            if (stack[idx] == dw_61F2EF || stack[idx + 1] == dw_61F2EF || stack[idx] == dw_61F2FE || stack[idx + 1] == dw_61F2FE)
                            {
                                flt_7933E0.a13 += Screen.fHudOffsetWide;
                            }
                            else
                            {
                            #ifdef _LOG
                                if (false)
                                    logfile << "0x" << std::hex << std::setw(8) << std::setfill('0') << stack[3] << " "
                                    << "0x" << std::hex << std::setw(8) << std::setfill('0') << stack[4] << std::endl;
                            #endif

                                if (stack[idx] == dw_61EA8F || stack[idx] == dw_616FB7 || stack[idx] == dw_4FD37D)
                                {
                                    flt_7933E0.a13 -= Screen.fHudOffsetWide;
                                }
                                else
                                {
                                    if (stack[idx] == dw_61EAC4 || stack[idx] == dw_61EAF6 || stack[idx] == dw_61EADD)
                                    {
                                        flt_7933E0.a13 += Screen.fHudOffsetWide;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }; injector::MakeInline<HudHook2>(pattern.get_first(0), pattern.get_first(7)); //0x47C7CE, 0x47C7CE+7

    //FMVs
    pattern = hook::pattern("8B 8D 68 FF FF FF E8 ? ? ? ? 6A 00 6A 00 8B 85 68 FF FF FF");
    struct MoviesHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = *(uint32_t*)(regs.ebp - 0x98);

            **(float**)(regs.esp + 0x10) = 1.0f / Screen.fHudScale;  //w
            **(float**)(regs.esp + 0x0C) = Screen.fHudOffsetReal;    //x
        }
    }; injector::MakeInline<MoviesHook>(pattern.get_first(0), pattern.get_first(6)); //0x4891EF

    if (bAltTab) //don't pause on minimize
    {
        pattern = hook::pattern("55 8B EC FF 15 ? ? ? ? 33 C9"); //0x465440
        injector::MakeRET(pattern.get_first(0));
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        #ifdef _LOG
        logfile.open("Psy.WidescreenFix.log");
        #endif // _LOG
        Init(NULL);
    }
    return TRUE;
}