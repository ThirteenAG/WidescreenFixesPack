#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "../../includes/pcsx2/pcsx2f_api.h"

#include "../../includes/pcsx2/log.h"
#include "../../includes/pcsx2/patterns.h"
#include "../../includes/pcsx2/injector.h"

int CompatibleCRCList[] = { 0xC0498D24, 0xABE2FDE9 };
char ElfPattern[] = "00 00 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 2D 28 00 00 ? ? ? ? 2D 30 00 00 2D 38 00 00";
char OSDText[OSDStringNum][OSDStringSize] = { {1} };

void init()
{
    logger.SetBuffer(OSDText, sizeof(OSDText) / sizeof(OSDText[0]), sizeof(OSDText[0]));
    logger.Write("Loading SplinterCellDoubleAgent.PCSX2F.SkipIntro...");

    uintptr_t ptr_222DB8 = pattern.get_first((const char*)&ElfPattern, -12);

    if (ptr_222DB8 != 0)
    {
        logger.Write("Skipping intro...");
        injector.WriteMemory16(ptr_222DB8 + 2, 0x1000); // beq -> b
        uintptr_t ptr_223CBC = pattern.get(1, "00 00 45 8C ? ? ? ? B8 00 0B 24 ? ? ? ? 00 00 00 00", 20);
        injector.MakeNOP(ptr_223CBC);
        uintptr_t ptr_2231F4 = pattern.get_first("04 00 02 24 ? ? ? ? 03 00 02 24 00 00 00 00", 4);
        injector.MakeNOP(ptr_2231F4);
    }

    logger.Write("SplinterCellDoubleAgent.PCSX2F.SkipIntro loaded");
}

int main()
{
    return 0;
}
