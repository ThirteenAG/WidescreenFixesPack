module;

#include <stdafx.h>

export module Timer;

export namespace CTimer
{
    GameRef<unsigned int> m_snTimeInMilliseconds([]() -> unsigned int*
    {
        auto pattern = hook::pattern("A1 ? ? ? ? 2B 46 ? 85 C0 89 44 24 ? DB 44 24 ? 7D ? D8 05 ? ? ? ? DA 76");
        return *pattern.get_first<unsigned int*>(1);
    });

    GameRef<unsigned int> m_snTimeInMillisecondsPauseMode([]() -> unsigned int*
    {
        auto pattern = hook::pattern("A1 ? ? ? ? 2B C2 01 44 8E");
        return *pattern.get_first<unsigned int*>(1);
    });

    GameRef<float> ms_fTimeStep([]() -> float*
    {
        auto pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? FF 15");
        return *pattern.get_first<float*>(2);
    });
}