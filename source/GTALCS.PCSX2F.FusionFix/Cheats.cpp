#include "Controls.hpp"
#include <cstring>

// Typed cheats for the PC control scheme. The emulator collects the letters
// typed in game (CheatString, newest first); a match enters the PS2 button
// sequence of that cheat through CPad::AddToCheatString, so the game runs its
// own cheat with its message and statistics.
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
bool Matches(const char* word) {
    const size_t length = std::strlen(word);
    if (length > CheatStringLen) return false;
    for (size_t i = 0; i < length; ++i)
        if (CheatString[i] != word[length - 1 - i]) return false;
    return true;
}
}
void UpdateCheats() {
    if (!playerPad || MenuActive()) return;
    for (const auto& cheat : cheats) {
        if (!Matches(cheat.word)) continue;
        const auto add = reinterpret_cast<void (*)(Pad*, char)>(0x16F030); // CPad::AddToCheatString
        for (const char* button = cheat.sequence; *button; ++button) add(playerPad, *button);
        CheatString[0] = 0;
        break;
    }
}
}
