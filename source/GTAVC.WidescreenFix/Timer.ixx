module;

#include <stdafx.h>

export module Timer;

export namespace CTimer
{
    GameRef<unsigned int> m_snTimeInMilliseconds([]() -> unsigned int*
    {
        auto pattern = hook::pattern("A1 ? ? ? ? 05 20 03 00 00 89 83 ? ? ? ? E9 0A 11 00 00");
        if (!pattern.empty())
            return *pattern.get_first<unsigned int*>(1);
        return nullptr;
    });

    GameRef<unsigned int> m_snTimeInMillisecondsPauseMode([]() -> unsigned int*
    {
        auto pattern = hook::pattern("A1 ? ? ? ? 2B 05 ? ? ? ? 83 F8 0A 76 ? A1 ? ? ? ? A3 ? ? ? ? 83 AE");
        if (!pattern.empty())
            return *pattern.get_first<unsigned int*>(1);
        return nullptr;
    });

    GameRef<float> ms_fTimeStep([]() -> float*
    {
        auto pattern = hook::pattern("D8 0D ? ? ? ? D8 40 ? DD D9");
        if (!pattern.empty())
            return *pattern.get_first<float*>(2);
        return nullptr;
    });
}