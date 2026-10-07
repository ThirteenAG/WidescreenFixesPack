#include "Game.hpp"
#include <cstring>
namespace vcsws {
namespace {
struct Cheat { const char* word; uint8_t id; };
constexpr Cheat cheats[] = {
    {"THUGSTOOLS", 0},
    {"NAKEDGUN", 0},
    {"PROFESSIONALTOOLS", 1},
    {"LETHALWEAPON", 1},
    {"NUTTERTOOLS", 2},
    {"TOPGUN", 2},
    {"INEEDSOMEHELP", 3},
    {"ALWAYSCARRYCASH", 3},
    {"PRECIOUSPROTECTION", 4},
    {"KEVLAR", 4},
    {"ASPIRINE", 5},
    {"CODEINE", 5},
    {"YOUWONTTAKEMEALIVE", 6},
    {"COMEATME", 6},
    {"LEAVEMEALONE", 7},
    {"LAYOFFME", 7},
    {"ALOVELYDAY", 8},
    {"VICEHEAT", 8},
    {"TOODAMNHOT", 9},
    {"EXTRASUNNY", 9},
    {"DULLDULLDAY", 10},
    {"CLOUDYWEATHER", 10},
    {"STAYINANDWATCHTV", 11},
    {"INMANCHESTER", 11},
    {"CANTSEEATHING", 12},
    {"SEAMIST", 12},
    {"PANZER", 13},
    {"TANKYOU", 13},
    {"TIMEJUSTFLIESBY", 14},
    {"POLARITYREVERSED", 14},
    {"BIGBANG", 15},
    {"FINALFLASH", 15},
    {"BETTERSTAYINDOORS", 16},
    {"MAYHEM", 16},
    {"ROUGHNEIGHBOURHOOD", 17},
    {"PUBLICENEMY1", 17},
    {"SURROUNDEDBYNUTTERS", 18},
    {"AMMUSUPERSALE", 18},
    {"SPEEDITUP", 19},
    {"LIKEARUSH", 19},
    {"SLOWITDOWN", 20},
    {"LIKEADRENALINE", 20},
    {"GRIPISEVERYTHING", 21},
    {"STRONGGRIP", 21},
    {"GOODBYECRUELWORLD", 22},
    {"YOUAREALREADYDEAD", 22},
    {"DONTTRYANDSTOPME", 23},
    {"NOTIMETOSTOP", 23},
    {"ALLDRIVERSARECRIMINALS", 24},
    {"ROADWARRIOR", 24},
    {"IWANTITPAINTEDBLACK", 25},
    {"NIGHTRIDERS", 25},
    {"RUBBISHCAR", 26},
    {"MENATWORK", 26},
    {"LOVECONQUERSALL", 27},
    {"ESCORTSERVICE", 27},
    {"FANNYMAGNET", 28},
    {"CLASSOF84", 28},
    {"SOSHINYSOCHROME", 29},
    {"TOPSYTURVY", 30},
};
constexpr char sequences[][9] = {
    "LRXUDSLR",
    "LRSUDTLR",
    "LRTUDCLR",
    "UDLRXX12",
    "UDLRSS12",
    "UDLRCC12",
    "URSSDLCC",
    "URTTDLXX",
    "LD21RULC",
    "LD21RULX",
    "LD12RULS",
    "LD12RULT",
    "LDTXRUL1",
    "U1D2L1R2",
    "211DUXD1",
    "122LRSD2",
    "211DLCD1",
    "DTUX1212",
    "U1D2LCRT",
    "LL22UTDX",
    "LLCCDUTX",
    "DLU12TCX",
    "RRCC12DX",
    "UDTX12LC",
    "UURLTCCS",
    "1212LCUX",
    "DURT1T1T",
    "DUR11SU1",
    "R1D1CU1S",
    "RULDTT12",
    "SSS112LR",
};

char buffer[24]{};
bool keys[26]{};
bool Matches(const char* word) {
    size_t length=std::strlen(word);
    if (length>sizeof(buffer)) return false;
    for (size_t i=0;i<length;++i) if (buffer[i]!=word[length-1-i]) return false;
    return true;
}
}
void InstallCheats() {}
void TickCheats() {
    if (*reinterpret_cast<uint8_t*>(Address<0x8BC9100>()+0x20)) return;
    for (unsigned i=0;i<26;++i) {
        uint8_t pressed=0;
        if (sceIoDevctl("kemulator:",0x34,reinterpret_cast<void*>(i+29),0,&pressed,sizeof(pressed))<0) continue;
        bool edge=pressed && !keys[i]; keys[i]=pressed!=0;
        if (!edge) continue;
        std::memmove(buffer+1,buffer,sizeof(buffer)-1);buffer[0]=char('A'+i);
        for (const auto& cheat:cheats) {
            if (!Matches(cheat.word)) continue;
            auto get=reinterpret_cast<void* (*)(int)>(Address<0x898B428>());
            auto add=reinterpret_cast<void (*)(void*,char)>(Address<0x898EBC0>());
            void* pad=get(0);
            if (pad) for (unsigned j=0;j<8;++j) add(pad,sequences[cheat.id][j]);
            buffer[0]=0;break;
        }
    }
}
}
