#include "Controls.hpp"
#include "Shared/Console/Ps2TypedCheats.hpp"

extern "C" console::CheatEvents CheatStringV2;

// Typed aliases use the game's own button sequences, messages and statistics.
namespace lcs {
namespace {
struct Cheat { const char* word; const char* sequence; };
// 1 = L1, 2 = R1, T/C/X/S = triangle, circle, cross, square, U/D/L/R = d-pad.
constexpr Cheat cheats[] = {
    {"THUGSTOOLS", "USSDLSSR"},
    {"PROFESSIONALTOOLS", "UCCDLCCR"},
    {"NUTTERTOOLS", "UXXDLXXR"},
    {"ALWAYSCARRYCASH", "12T12C12"},
    {"PRECIOUSPROTECTION", "12C12X12"},
    {"ASPIRINE", "12X12S12"},
    {"YOUWONTTAKEMEALIVE", "12S12T12"},
    {"LEAVEMEALONE", "11T22XSC"},
    {"SKINCANCERFORME", "11C22STX"},
    {"TOODAMNHOT", "UDCUDS12"},
    {"ILIKESCOTLAND", "UDXUDT12"},
    {"ILOVESCOTLAND", "UDSUDC12"},
    {"PEASOUP", "UDTUDX12"},
    {"GIVEUSATANK", "11L11RTC"},
    {"RUBBISHCAR", "TCDTCU11"},
    {"BANGBANGBANG", "11L11RXS"},
    {"ILIKEDRESSINGUP", "11L11RST"},
    {"TIMEFLIESWHENYOU", "11L11RCX"},
    {"NOBODYLIKESME", "112112UT"},
    {"ITSALLGOINGMAAAD", "112112LS"},
    {"WEAPONSFORALL", "221221RC"},
    {"ONSPEED", "221221DX"},
    {"BOOOOORING", "2TX2SCLR"},
    {"CORNERSLIKEMAD", "1UL2TCDX"},
    {"SEAWAYS", "CXDCXU11"},
    {"GREENLIGHT", "TT2SS1XX"},
    {"MIAMITRAFFIC", "SS2XX1CC"},
    {"ICANTTAKEITANYMORE", "1DL2XCUT"},
};
console::TypedCheats typedCheats;
}
void UpdateCheats()
{
    const bool gameplay = settings.pcControls && playerPad && !playerPad->disabled && !MenuActive();
    typedCheats.tick(gameplay, CheatString, CheatStringV2, [](const char* history)
    {
        for (const auto& cheat : cheats)
        {
            if (!console::TypedCheats::matches(history, cheat.word)) continue;
            const auto add = reinterpret_cast<void (*)(Pad*, char)>(0x16F030);
            for (const char* button = cheat.sequence; *button; ++button) add(playerPad, *button);
            return true;
        }
        return false;
    });
}
}
