#include "stdafx.h"
#include <assembly64.hpp>
#include <ppc/ppc.hpp>

void Init()
{
    CIniReader iniReader("");
    static bool bEnableDoorSkip = iniReader.ReadInteger("MAIN", "EnableDoorSkip", 1) != 0;
    static bool bUnthrottleEmuDuringDoorSkip = iniReader.ReadInteger("MAIN", "UnthrottleEmuDuringDoorSkip", 1) != 0;

    if (!bEnableDoorSkip && !bUnthrottleEmuDuringDoorSkip)
        return;

    static auto SetIsThrottlerTempDisabled = (void(__fastcall*)(bool disable))(nullptr);
    auto pattern = hook::pattern("0F B6 C8 E8 ? ? ? ? 33 D2");
    if (!pattern.empty())
        SetIsThrottlerTempDisabled = (void(__fastcall*)(bool disable))(injector::GetBranchDestination(pattern.get_first(3)).as_int());

    static auto MenuBarClearCache = (void(__fastcall*)())(nullptr);
    pattern = hook::pattern("45 33 C9 45 33 C0 33 D2");
    if (!pattern.empty())
    {
        for (size_t i = 0; i < pattern.size(); i++)
        {
            auto range_pattern = hook::pattern((uintptr_t)pattern.get(i).get<uintptr_t>(0), (uintptr_t)pattern.get(i).get<uintptr_t>(200), "45 ? ? ? 8D");
            if (!range_pattern.empty())
            {
                std::string_view str(injector::ReadRelativeOffset(range_pattern.get(0).get<uintptr_t>(6)).get_raw<char>());
                if (str == "Clear Cache")
                {
                    MenuBarClearCache = (void(__fastcall*)())(injector::ReadRelativeOffset(pattern.get(i).get<uintptr_t>(22)).as_int());
                    break;
                }
            }
        }        
    }

    std::thread([]()
    {
        using namespace powerpc;
        using namespace std::chrono_literals;
        static uintptr_t MainMemoryStart = 0;
        static uintptr_t MainMemoryEnd = 0;
        static uintptr_t ImageBase = 0;

        static auto FindEmulatorMem = []()
        {
            while (MainMemoryStart == 0)
            {
                uintptr_t curAddr = 0;
                do
                {
                    MEMORY_BASIC_INFORMATION MemoryInf;
                    if (VirtualQuery((LPCVOID)curAddr, &MemoryInf, sizeof(MemoryInf)) == 0) break;
                    if (MemoryInf.AllocationProtect == PAGE_READWRITE && MemoryInf.State == MEM_COMMIT &&
                        MemoryInf.Protect == PAGE_READWRITE && MemoryInf.Type == MEM_MAPPED && MemoryInf.RegionSize == 0x2000000)
                    {
                        if (MainMemoryStart == 0)
                        {
                            MainMemoryStart = (uintptr_t)MemoryInf.BaseAddress;
                            MainMemoryEnd = MainMemoryStart + 0x01800000;
                            ImageBase = 0x80000000;
                            break;
                        }
                    }
                    curAddr += MemoryInf.RegionSize;
                } while (true);
            }
        };

        static MEMORY_BASIC_INFORMATION MemoryInf;
        static uint32_t* data = nullptr;

        while (true)
        {
            std::this_thread::yield();
            while (VirtualQuery((LPCVOID)MainMemoryStart, &MemoryInf, sizeof(MemoryInf)) == 0 || MemoryInf.AllocationProtect != PAGE_READWRITE)
            {
                MainMemoryStart = 0;
                data = nullptr;
                FindEmulatorMem();
                std::this_thread::yield();
            }

            auto GameID = std::string_view(reinterpret_cast<char*>(MainMemoryStart));
            if (GameID == "GHAE08" || GameID == "GHAP08")
            {
                static uint32_t* memcheck = nullptr;
                static uint32_t bytes = 0;
                if (data == nullptr || (memcheck && *memcheck != bytes))
                {
                    std::this_thread::yield();
                    auto swap16 = [](uint16_t n) -> uint16_t
                    {
                        return (n >> 8) | (n << 8);
                    };
                    auto convert = [](uint32_t n) -> uint32_t
                    {
                        auto ptr = (unsigned char*)&n;
                        return (ptr[1] << 24) | (ptr[0] << 16) | 0 | 0;
                    };
                    auto pattern = hook::pattern(MainMemoryStart, MainMemoryEnd, "38 a0 00 01 3c 60 ? ? 38 83 ? ? 90 a4 00 00 3c 60");
                    if (pattern.size() >= 1)
                    {
                        memcheck = pattern.get(0).get<uint32_t>(4);
                        bytes = injector::ReadMemory<uint32_t>(memcheck, true);
                        uint16_t a = swap16(*pattern.get(0).get<uint16_t>(6));
                        uint16_t b = swap16(*pattern.get(0).get<uint16_t>(10));
                        uint16_t c = swap16(*pattern.get(0).get<uint16_t>(14));
                        if (a != 0 && b != 0)
                            data = (uint32_t*)((convert(a) - int16_t(0 - b) + c) - ImageBase + MainMemoryStart);
                    }
                    if (bEnableDoorSkip)
                    {
                        pattern = hook::pattern(MainMemoryStart, MainMemoryEnd, "54 7d 04 3e 38 00 00 01 98 1f 00 01");
                        if (pattern.size() >= 1)
                        {
                            injector::WriteMemory(pattern.get(0).get<void>(4), li(r0, 0), true);
                        }
                    }
                    if (MenuBarClearCache)
                        MenuBarClearCache();
                }
                else
                {
                    while (bUnthrottleEmuDuringDoorSkip && SetIsThrottlerTempDisabled && data[0] == 0x01000000)
                    {
                        SetIsThrottlerTempDisabled(true);
                    }
                }
            }
        }
    }
    ).detach();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("0F B6 C8 E8 ? ? ? ? 33 D2"));
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
