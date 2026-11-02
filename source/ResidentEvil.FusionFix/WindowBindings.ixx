module;

#include <stdafx.h>

export module WindowBindings;

export namespace REWindow
{
    inline uint8_t* fullscreenInstruction = nullptr;
    inline void* changeResolution = nullptr;

    inline bool Bind()
    {
        fullscreenInstruction = nullptr;
        changeResolution = nullptr;
        auto startup = hook::pattern("6A 00 68 ? ? ? ? 68 ? ? ? ? B9 ? ? ? ? 88 83 81 3E 43 00 C6 83 ? ? ? ? 01 C7 83 F8 00 00 00 00 00 00 00 E8 ? ? ? ? 6A 38");
        auto site = startup.get_first<uint8_t>();
        auto current = hook::pattern("0F B6 44 24 04 53 56 8B F1 33 DB 8B 0D ? ? ? ? 38 9E ? ? ? ? FF B6 98 00 00 00 0F 45 D8 E8");
        auto original = hook::pattern("53 0F B6 5C 24 08 56 8B F1 8B 0D ? ? ? ? FF B6 98 00 00 00 33 C0 38 86 ? ? ? ? 0F 44 D8 E8");
        auto setter = current.size() ? current.get_first<uint8_t>() : original.get_first<uint8_t>();
        fullscreenInstruction = site + 45;
        changeResolution = setter;
        return true;
    }
}
