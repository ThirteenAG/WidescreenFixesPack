module;

#include <stdafx.h>

export module WindowBindings;

export namespace REWindow
{
    inline uint8_t* fullscreenInstruction = nullptr;
    inline void* changeResolution = nullptr;
    inline void* resetDisplay = nullptr;
    inline void* resetResult = nullptr;

    inline bool Bind()
    {
        fullscreenInstruction = nullptr;
        changeResolution = nullptr;
        auto startup = hook::pattern("6A 00 68 ? ? ? ? 68 ? ? ? ? B9 ? ? ? ? 88 83 81 3E 43 00 C6 83 ? ? ? ? 01 C7 83 F8 00 00 00 00 00 00 00 E8 ? ? ? ? 6A 38");
        auto site = startup.get_first<uint8_t>();
        auto current = hook::pattern("0F B6 44 24 04 53 56 8B F1 33 DB 8B 0D ? ? ? ? 38 9E ? ? ? ? FF B6 98 00 00 00 0F 45 D8 E8");
        auto original = hook::pattern("53 0F B6 5C 24 08 56 8B F1 8B 0D ? ? ? ? FF B6 98 00 00 00 33 C0 38 86 ? ? ? ? 0F 44 D8 E8");
        const bool updated = !current.empty();
        auto setter = updated ? current.get_first<uint8_t>() : original.get_first<uint8_t>();
        fullscreenInstruction = site + 45;
        changeResolution = setter;
        resetDisplay = hook::pattern(updated ?
            "83 EC 4C 53 55 8B E9 56 57 8B 85 84 3E 43 00 89 85 94 3E 43 00" :
            "83 EC 4C 53 55 8B E9 56 8B 85 84 3E 43 00 57 89 85 94 3E 43 00").get_first();
        resetResult = hook::pattern(updated ?
            "FF B3 90 26 00 00 8B 02 52 FF 50 40 50 8B CE E8" :
            "FF B3 90 26 00 00 52 FF 50 40 50 8B CE E8").get_first(updated ? 12 : 10);
        return true;
    }
}
