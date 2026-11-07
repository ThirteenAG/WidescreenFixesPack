#include "Game.hpp"
#include <cstring>

namespace lcsws
{
namespace
{
constexpr uint32_t getPadWords[] = {0x00042180, 0x00842821, 0x00851021, 0x3C040000,
    0x24840000, 0x03E00008, 0x00441021, 0x27BDFFE0, 0xAFBF0010, 0x0C000000,
    0x00002025, 0x00402025};
constexpr uint8_t getPadMasks[] = {0, 0, 0, 1, 1, 0, 0, 0, 0, 2, 0, 0};
constexpr uint32_t addWords[] = {0x27BDFFD0, 0xAFB10014, 0x00052E00, 0x00808825,
    0x00052E03, 0x3404000A, 0xAFB00010, 0xAFB20018, 0xAFB3001C, 0xAFBF0020,
    0x02243021, 0x80C70095};
constexpr uint8_t addWordMasks[12] = {};
constexpr std::array<console::AddressRule, 2> rules = {{
    {0x08A96408, 0x08A96408, getPadWords, getPadMasks, 12, 8, 0, 0, 0, console::AddressKind::Code},
    {0x08A99710, 0x08A99710, addWords, addWordMasks, 12, 8, 0, 0, 0, console::AddressKind::Code}
}};
console::NativeProfile<rules> profile;
bool available;
bool keys[26]{};
char buffer[24]{};

struct Cheat
{
    const char* word;
    const char* sequence;
};
constexpr Cheat cheats[] = {
    {"THUGSTOOLS", "USSDLSSR"},
    {"PROFESSIONALTOOLS", "UCCDLCCR"},
    {"NUTTERTOOLS", "UXXDLXXR"},
    {"ALWAYSCARRYCASH", "12T12C12"},
    {"PRECIOUSPROTECTION", "12C12X12"},
    {"ASPIRINE", "12X12S12"},
    {"YOUWONTTAKEMEALIVE", "12S12T12"},
    {"LEAVEMEALONE", "11T22XSC"}
};

bool Matches(const char* word)
{
    const size_t length = std::strlen(word);
    if (length > sizeof(buffer)) return false;
    for (size_t i = 0; i < length; ++i)
        if (buffer[i] != word[length - 1 - i]) return false;
    return true;
}
}

void InstallCheats()
{
    available = profile.initialize({pattern.text_addr,
        reinterpret_cast<const uint8_t*>(pattern.text_addr), pattern.text_size,
        console::portable::gameGP});
    if (!available)
        logger.Write("PC cheat input disabled: unsupported cheat routines.\n");
}
bool CheatsAvailable() { return available; }

void TickCheats()
{
    if (!available || !settings.pcCheats) return;
    const bool active = !*reinterpret_cast<const uint8_t*>(Address<0x8B8EE20>() + 0x131);
    if (!active) std::memset(buffer, 0, sizeof(buffer));
    for (unsigned i = 0; i < 26; ++i)
    {
        uint8_t pressed = 0;
        const bool held = sceIoDevctl("kemulator:", 0x34,
            reinterpret_cast<void*>(i + 29), 0, &pressed, sizeof(pressed)) >= 0 && pressed;
        const bool edge = held && !keys[i];
        keys[i] = held;
        if (!active || !edge) continue;
        std::memmove(buffer + 1, buffer, sizeof(buffer) - 1);
        buffer[0] = char('A' + i);
        for (const auto& cheat : cheats)
        {
            if (!Matches(cheat.word)) continue;
            injector::game_fn<void*(int)> get{profile.get<0x08A96408>()};
            injector::game_fn<void(void*, char)> add{profile.get<0x08A99710>()};
            if (injector::cstd<uintptr_t()>::call(Address<0x89D58B0>()))
                if (void* pad = get(0))
                    for (const char* symbol = cheat.sequence; *symbol; ++symbol) add(pad, *symbol);
            std::memset(buffer, 0, sizeof(buffer));
            break;
        }
    }
}
}
