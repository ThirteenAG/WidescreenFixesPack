#include "stdafx.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib") // needed for timeBeginPeriod()/timeEndPeriod()

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int32_t Width43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
} Screen;

int32_t nLanguage;
int32_t __cdecl SetLanguage(LPCSTR lpValueName)
{
    return nLanguage;
}

int32_t nFrameLimitType;
float fFpsLimit;
float fGameSpeed = 1.0f;

class FrameLimiter
{
public:
    enum FPSLimitMode { FPS_NONE, FPS_REALTIME, FPS_ACCURATE };
    FPSLimitMode mFPSLimitMode = FPS_NONE;
private:
    double TIME_Frequency = 0.0;
    double TIME_Ticks = 0.0;
    double TIME_Frametime = 0.0;
    float  fFPSLimit = 0.0f;
public:
    void Init(FPSLimitMode mode, float fps_limit)
    {
        mFPSLimitMode = mode;
        fFPSLimit = fps_limit;

        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        static constexpr auto TICKS_PER_FRAME = 1;
        auto TICKS_PER_SECOND = (TICKS_PER_FRAME * fFPSLimit);
        if (mFPSLimitMode == FPS_ACCURATE)
        {
            TIME_Frametime = 1000.0 / (double)fFPSLimit;
            TIME_Frequency = (double)frequency.QuadPart / 1000.0; // ticks are milliseconds
        }
        else // FPS_REALTIME
        {
            TIME_Frequency = (double)frequency.QuadPart / (double)TICKS_PER_SECOND; // ticks are 1/n frames (n = fFPSLimit)
        }
        Ticks();
    }
    DWORD Sync_RT()
    {
        DWORD lastTicks, currentTicks;
        LARGE_INTEGER counter;

        QueryPerformanceCounter(&counter);
        lastTicks = (DWORD)TIME_Ticks;
        TIME_Ticks = (double)counter.QuadPart / TIME_Frequency;
        currentTicks = (DWORD)TIME_Ticks;

        return (currentTicks > lastTicks) ? currentTicks - lastTicks : 0;
    }
    DWORD Sync_SLP()
    {
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        double millis_current = (double)counter.QuadPart / TIME_Frequency;
        double millis_delta = millis_current - TIME_Ticks;
        if (TIME_Frametime <= millis_delta)
        {
            TIME_Ticks = millis_current;
            return 1;
        }
        else if (TIME_Frametime - millis_delta > 2.0) // > 2ms
            Sleep(1); // Sleep for ~1ms
        else
            Sleep(0); // yield thread's time-slice (does not actually sleep)
        
        return 0;
    }
    void Sync()
    {
        if (mFPSLimitMode == FPS_REALTIME)
            while (!Sync_RT());
        else if (mFPSLimitMode == FPS_ACCURATE)
            while (!Sync_SLP());
    }
private:
    void Ticks()
    {
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        TIME_Ticks = (double)counter.QuadPart / TIME_Frequency;
    }
};

FrameLimiter FpsLimiter;
float fGameSpeedFactor = 1.0f;
void __cdecl sub_648AC0(int a1)
{
    if (fFpsLimit)
        FpsLimiter.Sync();

    static std::list<int> m_times;
    LARGE_INTEGER frequency;
    LARGE_INTEGER time;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&time);

    if (m_times.size() == 50)
        m_times.pop_front();
    m_times.push_back(static_cast<int>(time.QuadPart));

    if (m_times.size() >= 2)
    {
        auto fps = (0.5f + (static_cast<float>(m_times.size() - 1) * static_cast<float>(frequency.QuadPart)) / static_cast<float>(m_times.back() - m_times.front()));
        fGameSpeed = (30.0f / fps);
        if (fGameSpeedFactor > 0.0f)
            fGameSpeed /= fGameSpeedFactor;
    }
}

void Init()
{
    CIniReader iniReader("");
    bool bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    bool bDoNotUseRegistryPath = iniReader.ReadInteger("MAIN", "DoNotUseRegistryPath", 1) != 0;
    nLanguage = iniReader.ReadInteger("MAIN", "Language", -1);
    static bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;
    static bool bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) != 0;

    static bool bFixGameSpeed = iniReader.ReadInteger("FRAMELIMIT", "FixGameSpeed", 1) != 0;
    fGameSpeedFactor = iniReader.ReadFloat("FRAMELIMIT", "GameSpeedFactor", 0.0f);
    nFrameLimitType = iniReader.ReadInteger("FRAMELIMIT", "FrameLimitType", 1);
    fFpsLimit = static_cast<float>(iniReader.ReadInteger("FRAMELIMIT", "FpsLimit", 30));
    
    static auto fSensitivityFactor = iniReader.ReadFloat("MOUSE", "SensitivityFactor", 0.0f);


    if (bSkipIntro)
    {
        auto pattern = hook::pattern("6A 01 6A 01 68 ? ? ? ? E8 ? ? ? ? 6A 01 6A 01");
        injector::MakeJMP(pattern.get_first(0), hook::get_pattern("68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 08 E8 ? ? ? ? A1 ? ? ? ? 3B C3 75 23 6A 10"));
    }

    if (bDoNotUseRegistryPath)
    {
        auto pattern = hook::pattern("B9 20 00 00 00 8D 7C 24 18 F3 AB 8D 44 24 0C"); //0x496F27
        struct RegHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.ecx = 0x20;
                regs.edi = (regs.esp + 0x18);

                GetModuleFileNameA(NULL, (char*)regs.edi, MAX_PATH);
                *strrchr((char*)regs.edi, '\\') = '\0';
                strcat((char*)regs.edi, "\\data");
            }
        }; injector::MakeInline<RegHook>(pattern.get_first(0), pattern.get_first(11));
        injector::MakeJMP(pattern.get_first(11), hook::pattern("8D 44 24 18 8D 50 01").count(2).get(1).get<uintptr_t>(0), true); //0x496FD8
    }

    if (nLanguage >= 0)
    {
        auto pattern = hook::pattern("E8 ? ? ? ? 8B 04 85 ? ? ? ? 83 C4 04 C3"); //0x495E95
        injector::MakeCALL(pattern.count(1).get(0).get<uintptr_t>(0), SetLanguage, true);
        pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 83 C4 04 C3"); //0x495EB5
        injector::MakeCALL(pattern.count(2).get(1).get<uintptr_t>(5), SetLanguage, true);
    }

    auto pattern = hook::pattern("89 55 00 89 5D 04 C7 45 08 15 00 00 00 89 7D 0C"); //0x649478
    struct ResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            Screen.Width = regs.edx;
            Screen.Height = regs.ebx;
            *(uint32_t*)(regs.ebp + 0x00) = Screen.Width;
            *(uint32_t*)(regs.ebp + 0x04) = Screen.Height;

            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
            Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
            Screen.fWidth43 = static_cast<float>(Screen.Width43);
            Screen.fHudOffset = (1.0f / (Screen.fHeight * (4.0f / 3.0f))) * ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f);
        }
    }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(6));

    if (bFixHUD)
    {
        uintptr_t dword_654780 = (uintptr_t)hook::pattern("8B 44 24 04 F3 0F 2A C0 0F 28 C8").count(1).get(0).get<uintptr_t>(0);
        struct HudScaleHook
        {
            void operator()(injector::reg_pack& regs)
            {
                int32_t a2 = *(int32_t*)(regs.esp + 4);
                if (a2 == Screen.Width)
                    a2 = Screen.Width43;
        
                *(float*)(regs.ecx + 0xE0) = (float)a2 * (1.0f / 640.0f);
                *(int32_t*)(regs.ecx + 0xD8) = a2;
                *(float*)(regs.ecx + 0xE8) = (1.0f / (float)a2);
            }
        }; injector::MakeInline<HudScaleHook>(dword_654780, dword_654780 + 53);
        
        pattern = hook::pattern("F3 0F 11 44 24 24 F3 0F 11 44 24 20 F3 0F 11"); //0x65E870
        struct HudPosHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x18) = Screen.fHudOffset;
                *(float*)(regs.esp + 0x20) = 1.0f;
                *(float*)(regs.esp + 0x24) = 1.0f;
                *(float*)(regs.esp + 0x28) = 1.0f;
                *(float*)(regs.esp + 0x2C) = 1.0f;
            }
        }; injector::MakeInline<HudPosHook>(pattern.get_first(0), pattern.get_first(24));
        injector::WriteMemory(pattern.get_first(24 - 4), 0x9001F883, true); //cmp     eax, 1
    }

    if (bFixFOV)
    {
        pattern = hook::pattern("F3 0F 10 45 08 56 8B F1 8B 46 1C"); //0x64BDF9 
        struct FOVHook
        {
            void operator()(injector::reg_pack& regs)
            {
                Screen.fFieldOfView = *(float*)(regs.ebp + 0x8) * (((4.0f / 3.0f)) / (Screen.fAspectRatio));
                regs.xmm0.f32[0] = Screen.fFieldOfView;
            }
        }; injector::MakeInline<FOVHook>(pattern.get_first(0));
    }

    if (fFpsLimit)
    {
        auto mode = (nFrameLimitType == 2) ? FrameLimiter::FPSLimitMode::FPS_ACCURATE : FrameLimiter::FPSLimitMode::FPS_REALTIME;
        if (mode == FrameLimiter::FPSLimitMode::FPS_ACCURATE)
            timeBeginPeriod(1);

        FpsLimiter.Init(mode, fFpsLimit);

        pattern = hook::pattern("A1 ? ? ? ? 83 EC 1C");
        injector::MakeJMP(pattern.get_first(), sub_648AC0, true);
    }

    if (bFixGameSpeed)
    {
        pattern = hook::pattern("E8 ? ? ? ? D8 4E 1C");
        struct GameSpeedHook
        {
            void operator()(injector::reg_pack& regs)
            {
                _asm fld dword ptr[fGameSpeed]
            }
        }; injector::MakeInline<GameSpeedHook>(pattern.get_first(0), pattern.get_first(8));
    }

    if (fSensitivityFactor)
    {
        pattern = hook::pattern("D8 0D ? ? ? ? 6A 00 68 ? ? ? ? 8B CE D9 1D ? ? ? ? E8 ? ? ? ? D8 0D ? ? ? ? 6A 00");
        injector::WriteMemory(pattern.get_first(2), &fSensitivityFactor, true);
        pattern = hook::pattern("D8 0D ? ? ? ? 6A 00 68 ? ? ? ? 8B CE D9 1D ? ? ? ? E8 ? ? ? ? D9 1D ? ? ? ? 6A 00");
        injector::WriteMemory(pattern.get_first(2), &fSensitivityFactor, true);
        pattern = hook::pattern("D8 0D ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? D8 0D ? ? ? ? 8B CE D9 1D ? ? ? ? E8");
        injector::WriteMemory(pattern.get_first(2), &fSensitivityFactor, true);
        pattern = hook::pattern("D8 0D ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 8B CE D9 1D ? ? ? ? E8 ? ? ? ? 68");
        injector::WriteMemory(pattern.get_first(2), &fSensitivityFactor, true);
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("BF 94 00 00 00 8B C7"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    if (reason == DLL_PROCESS_DETACH)
    {
        if (nFrameLimitType == FrameLimiter::FPSLimitMode::FPS_ACCURATE)
            timeEndPeriod(1);
    }
    return TRUE;
}