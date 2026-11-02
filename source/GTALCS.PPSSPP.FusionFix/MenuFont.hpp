#pragma once
#include "Game.hpp"

namespace lcsws::menu_font
{
inline constexpr uint32_t words0[] = {0x3C040000u, 0x24840000u, 0xE48C0000u, 0x03E00008u, 0xE48D0000u, 0x3C040000u, 0x24840000u, 0x03E00008u, 0xE48C0000u, 0x3C040000u, 0x24840000u, 0xE48C0000u, 0x03E00008u, 0xE48D0000u, 0x00802825u, 0x90A40000u};
inline constexpr uint8_t masks0[] = {1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1};
inline constexpr uint32_t words1[] = {0x00802825u, 0x90A40000u, 0x3C060000u, 0xA0C40000u, 0x90A70000u, 0x24C40000u, 0xA0870000u, 0x90A60000u, 0xC48D0000u, 0xA0860000u, 0x90A50000u, 0x3C060000u, 0x44866000u, 0x460C683Cu, 0x00000000u, 0x45000009u};
inline constexpr uint8_t masks1[] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0};
inline constexpr uint32_t words2[] = {0x3C040000u, 0x24840000u, 0x03E00008u, 0xA0800000u, 0x3C050000u, 0x34040001u, 0x24A50000u, 0xA0A40000u, 0xA0A00000u, 0x03E00008u, 0xA0A00000u, 0x3C040000u, 0x24840000u, 0xA0800000u, 0xA0800000u, 0x03E00008u};
inline constexpr uint8_t masks2[] = {1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0};
inline constexpr uint32_t words3[] = {0x3C040000u, 0x24840000u, 0xA0800000u, 0xA0800000u, 0x03E00008u, 0xA0800000u, 0x44807000u, 0x3C040000u, 0x460E603Cu, 0x00000000u, 0x45000002u, 0x44846800u, 0x46007306u, 0x460D603Eu, 0x00000000u, 0x45010002u};
inline constexpr uint8_t masks3[] = {1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0};
inline constexpr uint32_t words4[] = {0x3C040000u, 0x24840000u, 0x03E00008u, 0xE48C0000u, 0x3C050000u, 0x34040001u, 0x24A50000u, 0x03E00008u, 0xA0A40000u, 0x3C040000u, 0x24840000u, 0x03E00008u, 0xA0800000u, 0x90850000u, 0x3C060000u, 0x24C60000u};
inline constexpr uint8_t masks4[] = {1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1};
inline constexpr uint32_t words5[] = {0x00042C00u, 0x3C040000u, 0x00052C03u, 0x34060002u, 0x14A60005u, 0x24840000u, 0xA4850000u, 0x34050001u, 0x10000003u, 0xA0850000u, 0xA4850000u, 0xA0800000u, 0x03E00008u, 0x00000000u, 0x3C040000u, 0x24840000u};
inline constexpr uint8_t masks5[] = {0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1};
inline constexpr uint32_t words6[] = {0x3C050000u, 0x24A50000u, 0x03E00008u, 0xA4A40000u, 0x00802825u, 0x90A60000u, 0x3C040000u, 0x24840000u, 0xA0860000u, 0x90A60000u, 0xA0860000u, 0x90A60000u, 0xC48D0000u, 0xA0860000u, 0x90A50000u, 0x3C060000u};
inline constexpr uint8_t masks6[] = {1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
inline constexpr uint32_t words7[] = {0x27BDFF90u, 0xE7B40040u, 0xE7B60044u, 0xAFB00050u, 0xAFB10054u, 0x46006D86u, 0x00808025u, 0x46006506u, 0x00A08825u, 0xE7B80048u, 0xE7BA004Cu, 0xAFB20058u, 0xAFB3005Cu, 0xAFB40060u, 0xAFB50064u, 0xAFBF0068u};
inline constexpr uint8_t masks7[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
inline constexpr std::array<console::AddressRule, 8> rules = {{
    {0x08A54DF8u, 0x08A54DF8u, words0, masks0, 16, 2, 0, 0, 0, console::AddressKind::Code},
    {0x08A54E30u, 0x08A54E30u, words1, masks1, 16, 2, 0, 0, 0, console::AddressKind::Code},
    {0x08A54EE8u, 0x08A54EE8u, words2, masks2, 16, 2, 0, 0, 0, console::AddressKind::Code},
    {0x08A54F14u, 0x08A54F14u, words3, masks3, 16, 2, 0, 0, 0, console::AddressKind::Code},
    {0x08A54FACu, 0x08A54FACu, words4, masks4, 16, 2, 0, 0, 0, console::AddressKind::Code},
    {0x08A55054u, 0x08A55054u, words5, masks5, 16, 2, 0, 0, 0, console::AddressKind::Code},
    {0x08A5509Cu, 0x08A5509Cu, words6, masks6, 16, 2, 0, 0, 0, console::AddressKind::Code},
    {0x08A56AD4u, 0x08A56AD4u, words7, masks7, 16, 2, 0, 0, 0, console::AddressKind::Code},
}};
inline console::NativeProfile<rules> profile;
inline bool available;
inline bool initialized;
inline void Initialize()
{
    if (initialized) return;
    initialized = true;
    available = profile.initialize({pattern.text_addr,
        reinterpret_cast<const uint8_t*>(pattern.text_addr), pattern.text_size, console::portable::gameGP});
}
inline void Text(const char16_t* text, float x, float y)
{
    reinterpret_cast<void (*)(const char16_t*, int, float, float)>(profile.get<0x08A56AD4>())(text, 0, x, y);
}
inline void Begin()
{
    const console::portable::StoryColor white{253, 179, 54, 255};
    reinterpret_cast<void (*)(int)>(profile.get<0x08A55054>())(2);
    reinterpret_cast<void (*)(int)>(profile.get<0x08A5509C>())(1);
    reinterpret_cast<void (*)()>(profile.get<0x08A54EE8>())();
    reinterpret_cast<void (*)()>(profile.get<0x08A54F14>())();
    reinterpret_cast<void (*)(float, float)>(profile.get<0x08A54DF8>())(0.4f, 0.65f);
    reinterpret_cast<void (*)(float)>(profile.get<0x08A54FAC>())(460.0f);
    reinterpret_cast<void (*)(const console::portable::StoryColor*)>(profile.get<0x08A54E30>())(&white);
}
}
