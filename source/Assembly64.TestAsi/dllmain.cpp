#include "stdafx.h"
#include "raw_mem.hpp"

void Init()
{
    auto pattern = hook::pattern("48 8D 15 ? ? ? ? 33 C9 FF 15 ? ? ? ? 45 33 C9");
    struct TestHook
    {
        void operator()(injector::reg_pack& regs)
        {
            static constexpr auto str = "Hello x64!";
            regs.rdx = (uint64_t)(str);
        }
    }; injector::MakeInline<TestHook>(pattern.get_first(0), pattern.get_first(7));

    pattern = hook::pattern("FF 15 ? ? ? ? 48 8D 15 ? ? ? ? 48 8D 0D");
    struct TestHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            static constexpr auto str = "Hello x64!";
            regs.rdx = (uint64_t)(str);

            injector::asm_invoke([](x86::Assembler& a)
            {
                Zone zone(1024);
                ConstPool constPool(&zone);
                Label constPoolLabel = a.newLabel();
                auto doubleAsMem = [&](double value) {
                    size_t valueOffset;
                    constPool.add(&value, sizeof(double), valueOffset);
                    return x86::ptr(constPoolLabel, valueOffset);
                };

                a.movss(x86::xmm0, doubleAsMem(1.5));
                a.movss(x86::xmm1, doubleAsMem(1.0));
                a.ret();
            })();
        }
    }; injector::MakeInline<TestHook2>(pattern.get_first(6), pattern.get_first(6+7));
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("48 8D 15 ? ? ? ? 33 C9 FF 15 ? ? ? ? 45 33 C9"));
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
