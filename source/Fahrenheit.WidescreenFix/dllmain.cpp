#include "stdafx.h"

void Init()
{
    CIniReader iniReader("");
    int ResX = iniReader.ReadInteger("MAIN", "X", 0);
    int ResY = iniReader.ReadInteger("MAIN", "Y", 0);

    if (!ResX || !ResY)
        std::tie(ResX, ResY) = GetDesktopRes();

    injector::WriteMemory(0x403DA2, ResX, true);
    injector::WriteMemory(0x403DAC, ResY, true);

    injector::WriteMemory(0x40949A, ResX, true);
    injector::WriteMemory(0x409495, ResY, true);

    injector::WriteMemory(0x432B8C, ResX, true);
    injector::WriteMemory(0x432B91, ResY, true);

    injector::WriteMemory(0x4553FC, ResX, true);
    injector::WriteMemory(0x455406, ResY, true);

    injector::WriteMemory(0x90B9DC, ResX, true);
    injector::WriteMemory(0x90B9E0, ResY, true);

    injector::WriteMemory(0xA0A930, ResX, true);
    injector::WriteMemory(0xA0A934, ResY, true);

    injector::WriteMemory<float>(0x552487 + 0x1, (float)ResX / (float)ResY, true);
    injector::WriteMemory<float>(0x55248E + 0x1, (float)ResX / (float)ResY, true);
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        Init();
    }
    return TRUE;
}