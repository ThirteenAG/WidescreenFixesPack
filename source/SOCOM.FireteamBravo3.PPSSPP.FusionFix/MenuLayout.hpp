#pragma once

namespace NativeMenu
{
inline constexpr char buttonName[] = "buttonFusionFixPlugins";
alignas(16) inline uint8_t layouts[2][4096];
inline uint32_t* pluginRows[2];
inline int Language();
inline uintptr_t UnsupportedLayout(uintptr_t definition, int index)
{
    pluginRows[index] = nullptr;
    static bool reported;
    if (!reported)
    {
        reported = true;
        static constexpr const char* messages[] = {
            "Fusion Fix: plugin menu unavailable; menu data not recognized.\n",
            "Fusion Fix : menu du plugin indisponible ; donnees du menu inconnues.\n",
            "Fusion Fix: Plugin-Menue nicht verfuegbar; Menue-Daten unbekannt.\n",
            "Fusion Fix: menu plugin non disponibile; dati del menu sconosciuti.\n",
            "Fusion Fix: menu del plugin no disponible; datos del menu desconocidos.\n"
        };
        PatchNotice(messages[Language()]);
    }
    return definition;
}

inline unsigned DefinitionSize(unsigned type)
{
    constexpr unsigned sizes[]{ 48, 112, 112, 140, 68, 72, 48 };
    return type < 7 ? sizes[type] : 0;
}
inline uintptr_t ExtendLayout(uintptr_t definition, uintptr_t strings, const char* label)
{
    const auto source = reinterpret_cast<const uint32_t*>(definition);
    const auto name = reinterpret_cast<const char*>(strings + source[0]);
    const int index = std::strcmp(name, "OptionsScreen") == 0 ? 0 :
        std::strcmp(name, "PauseOptionsScreen") == 0 ? 1 : -1;
    if (index < 0)
        return definition;
    const unsigned count = source[2];
    unsigned length = 32, first = 0, last = 0;
    int top = 272, bottom = -1;
    for (unsigned i = 0; i < count; ++i)
    {
        if (length + 12 > sizeof(layouts[0]) - 112)
            return UnsupportedLayout(definition, index);
        const auto row = reinterpret_cast<const uint32_t*>(definition + length);
        const unsigned size = DefinitionSize(row[2]);
        if (!size || length + size > sizeof(layouts[0]) - 112)
            return UnsupportedLayout(definition, index);
        if (row[2] == 1)
        {
            const int y = int(row[4]);
            if (y < top)
            {
                top = y;
                first = length;
            }
            if (y > bottom)
            {
                bottom = y;
                last = length;
            }
        }
        length += size;
    }
    if (!first || !last)
        return UnsupportedLayout(definition, index);
    auto copy = layouts[index];
    std::memcpy(copy, source, length);
    auto head = reinterpret_cast<uint32_t*>(copy);
    auto firstRow = reinterpret_cast<uint32_t*>(copy + first);
    auto lastRow = reinterpret_cast<uint32_t*>(copy + last);
    auto added = reinterpret_cast<uint32_t*>(copy + length);
    pluginRows[index] = added;
    std::memcpy(added, lastRow, 112);
    const uint32_t relativeName = uint32_t(uintptr_t(buttonName) - strings);
    added[0] = relativeName;
    added[4] = uint32_t(bottom + (index == 0 ? 21 : 19));
    added[10] = lastRow[0];
    added[11] = firstRow[0];
    added[14] = uint32_t(uintptr_t(label) - strings);
    lastRow[11] = relativeName;
    firstRow[10] = relativeName;
    head[2] = count + 1;
    return uintptr_t(copy);
}
}
