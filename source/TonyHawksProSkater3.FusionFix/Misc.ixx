module;

#include <stdafx.h>
#include <random>

export module Misc;


export void InstallMisc()
{
    hook::pattern pattern("");
    {
        pattern = hook::pattern("E8 ? ? ? ? 8B 96 E8");
        struct RandomHook
        {
            void operator()(injector::reg_pack& regs)
            {
                std::mt19937 r{ std::random_device{}() };
                std::uniform_int_distribution<uint32_t> uid(0, regs.eax);
                regs.eax = uid(r);
            }
        }; injector::MakeInline<RandomHook>(pattern.get_first(0));
    }
}
