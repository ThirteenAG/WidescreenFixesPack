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

    auto pattern = hook::pattern("0F B6 C8 E8 ? ? ? ? 33 D2");
    static auto SetIsThrottlerTempDisabled = (void(__fastcall*)(bool disable))(injector::GetBranchDestination(pattern.get_first(3)).as_int());

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
        static uint8_t* data = nullptr;

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
            if (GameID == "GLEE08"/*||GameID == "GLEJ08"*/)
            {
                if (data == nullptr)
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
                    auto pattern = hook::pattern(MainMemoryStart, MainMemoryEnd, "38 00 00 01 3c 60 ? ? 38 63 ? ? ? ? 00 00 3c 60");
                    if (pattern.size() > 1)
                    {
                        uint16_t a = swap16(*pattern.get(1).get<uint16_t>(6));
                        uint16_t b = swap16(*pattern.get(1).get<uint16_t>(10));
                        uint16_t c = swap16(*pattern.get(1).get<uint16_t>(14));
                        data = (uint8_t*)((convert(a) - int16_t(0 - b) + c) - ImageBase + MainMemoryStart);
                    }
                    if (bEnableDoorSkip)
                    {
                        pattern = hook::pattern(MainMemoryStart, MainMemoryEnd, "94 21 ff f0 7c 08 02 a6 ? ? ? ? ? ? ? ? 38 60 00 01");
                        if (pattern.size() > 1)
                        {
                            injector::WriteMemory(pattern.get(0).get<void>(0), blr(), true); //return
                        }
                    }
                }
                else
                {
                    while (bUnthrottleEmuDuringDoorSkip && data[0] == 1)
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
