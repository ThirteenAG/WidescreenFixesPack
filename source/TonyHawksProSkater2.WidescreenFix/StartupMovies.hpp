#pragma once
#include <cstring>

namespace THPS2Startup
{
    inline bool Install(bool skip, void* module = GetModuleHandleW(nullptr))
    {
        if (!skip) return true;
        auto first = hook::module_pattern(module, "E8 ? ? ? ? 6A FF C6 05 ? ? ? ? 00 C6 05 ? ? ? ? 00 C6 05 ? ? ? ? 00 E8 ? ? ? ? A0 ? ? ? ? 83 C4 04 84 C0");
        auto second = hook::module_pattern(module, "6A FE E8 ? ? ? ? 83 C4 04 68 ? ? ? ? E8 ? ? ? ? A0 ? ? ? ? 83 C4 04");
        auto intro = hook::module_pattern(module, "6A FD E8 ? ? ? ? 59 C3");
        if (first.size() != 1 || second.size() != 1 || intro.size() != 1) return false;
        const auto movie = injector::GetBranchDestination(first.get_first(28));
        const auto begin = reinterpret_cast<uintptr_t>(first.get_first());
        const auto middle = reinterpret_cast<uintptr_t>(second.get_first());
        const auto end = reinterpret_cast<uintptr_t>(intro.get_first());
        if (middle <= begin || end <= middle || end - begin >= 0x100 ||
            movie != injector::GetBranchDestination(second.get_first(2)) ||
            movie != injector::GetBranchDestination(intro.get_first(2)) ||
            std::strcmp(*second.get_first<const char*>(11), "GrayMat.dat") != 0) return false;
        injector::MakeNOP(first.get_first(28), 5, true);
        injector::MakeNOP(second.get_first(2), 5, true);
        injector::MakeNOP(second.get_first(15), 5, true);
        // Only the startup caller skips the skating intro. Other movie requests stay native.
        injector::MakeNOP(intro.get_first(2), 5, true);
        injector::WriteMemory<uint16_t>(intro.get_first(2), 0xC031, true);
        return true;
    }
}
