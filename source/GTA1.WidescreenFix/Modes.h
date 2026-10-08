#pragma once

namespace GTA1Modes
{
    // Mode list entries: a 20-character label, the group and a pointer to the
    // mode (MGL mode, width, height, bits). Classics pads them to 28 bytes; the
    // retail builds pack them into 25 (GTA1Build::ModeEntrySize).
    using Entry = uint8_t;
    inline SafetyHookInline ListHook, ApplyHook, MeasureHook;
    inline int *OutputWidth, *OutputHeight, *CameraWidth;
    inline Entry* Entries = nullptr;
    inline Entry* At(Entry* entries, int index) { return entries + size_t(index) * GTA1Build::ModeEntrySize; }
    inline char* Label(Entry* entries, int index) { return reinterpret_cast<char*>(At(entries, index)); }
    inline int* Mode(Entry* entries, int index) { return *reinterpret_cast<int**>(At(entries, index) + GTA1Build::ModeEntryMode); }
    inline int Count = 0;
    inline bool Prepared = false, FirstApply = true;
    inline std::wstring IniPath;
    inline int __cdecl Measure(int font, const char* label)
    {
        int width = MeasureHook.call<int>(font, label);
        // Native layout measures only its first legacy label. Modern modes
        // need the widest label, otherwise the columns overlap.
        if (Prepared && Entries && label == Label(Entries, 0))
            for (int i = 1; i < Count; ++i)
                width = std::max(width, MeasureHook.call<int>(font, Label(Entries, i)));
        return width;
    }
    inline void Prepare(Entry* entries, int count, const uint8_t* groups)
    {
        if (Prepared || !entries || count <= 0 || count > 128) return;
        Entries = entries; Count = count;
        const std::pair<int, int> presets[] = {{640,480},{1280,720},{1366,768},{1600,900},{1920,1080},{1920,1200},
            {2560,1080},{2560,1440},{3440,1440},{3840,1600},{3840,2160},{5120,1440}};
        constexpr int last = int(std::size(presets)) - 1;
        const bool custom = std::none_of(std::begin(presets), std::end(presets), [](auto resolution)
            { return resolution.first == *OutputWidth && resolution.second == *OutputHeight; });
        int slot = 0;
        for (int group = 0; group < 3; ++group)
            for (int row = 0; row < groups[group] && slot < count; ++row, ++slot)
            {
                auto [width, height] = presets[std::min(row, last)];
                // Preserve a configured resolution that isn't one of the presets.
                if (custom && row == groups[group] - 1)
                { width = *OutputWidth; height = *OutputHeight; }
                auto mode = Mode(entries, slot);
                mode[1] = width; mode[2] = height;
                sprintf_s(Label(entries, slot), 20, "%dx%dx%d", width, height, mode[3]);
            }
        Prepared = true;
    }
    inline void* __cdecl List(Entry** entries, uint16_t* count, uint16_t* columns, uint8_t** groups)
    {
        auto result = ListHook.call<void*>(entries, count, columns, groups);
        Prepare(*entries, *count, *groups);
        return result;
    }
    inline void* __cdecl Apply(uint8_t* index)
    {
        Entry* entries = nullptr; uint16_t count = 0, columns = 0; uint8_t* groups = nullptr;
        List(&entries, &count, &columns, &groups);
        if (FirstApply && Prepared)
        {
            for (int i = 0; i < Count; ++i)
                if (Mode(Entries, i)[1] == *OutputWidth && Mode(Entries, i)[2] == *OutputHeight &&
                    Mode(Entries, i)[3] == 16) { *index = uint8_t(i); break; }
        }
        if (Prepared && *index < Count)
        {
            *OutputWidth = Mode(Entries, *index)[1];
            *OutputHeight = Mode(Entries, *index)[2];
            *CameraWidth = int(*OutputHeight * (4.0 / 3.0));
        }
        auto result = ApplyHook.call<void*>(index);
        if (!FirstApply && Prepared && *index < Count)
        {
            CIniReader ini("");
            ini.WriteString("MAIN", "ResX", std::to_string(*OutputWidth));
            ini.WriteString("MAIN", "ResY", std::to_string(*OutputHeight));
        }
        FirstApply = false;
        return result;
    }
    inline uint8_t* ModeIndex = nullptr;           // mode applied when a game starts
    inline void* (__cdecl* Reload)(int) = nullptr;   // recreates the display (-1: frontend mode)
    inline void (__cdecl* Palette)() = nullptr;      // converts the style palette to the new pixel format
    // A gameplay resolution change, as the native in-game mode list (F11) does it.
    inline bool ApplyGameplay()
    {
        if (!Prepared || !ModeIndex || !Palette) return false;
        uint8_t index = *ModeIndex;
        Apply(&index);
        Palette();
        return true;
    }
    // A resolution chosen while the game runs: make it the mode games start with.
    inline void Select(int width, int height)
    {
        if (!Prepared || !ModeIndex) return;
        int index = -1, fallback = -1;
        for (int i = 0; i < Count; ++i)
        {
            auto mode = Mode(Entries, i);
            if (mode[3] != 16) continue;
            if (fallback < 0 || i == *ModeIndex) fallback = i;
            if (mode[1] == width && mode[2] == height) { index = i; break; }
        }
        if (index < 0 && fallback >= 0)
        {
            index = fallback;
            auto mode = Mode(Entries, index);
            mode[1] = width; mode[2] = height;
            sprintf_s(Label(Entries, index), 20, "%dx%dx%d", width, height, mode[3]);
        }
        if (index >= 0) *ModeIndex = uint8_t(index);
    }
    inline void Install(int& width, int& height, int& cameraWidth)
    {
        OutputWidth = &width; OutputHeight = &height; CameraWidth = &cameraWidth;
        using GTA1Build::Find, GTA1Build::Off;
        // The mode list builder and the mode switch (with the selected mode index).
        auto list = Find("8B 4C 24 0C 8B 44 24 04 56 8B 74 24 0C 66 C7 01 00 00 66 C7 06 00 00 8B 15",
            "8B 54 24 08 56 8B 74 24 10 8B 44 24 08 66 C7 06 00 00 66 C7 02 00 00 8B 0D",
            "8B 44 24 04 56 8B 74 24 10 57 8B 7C 24 10 66 C7 06 00 00 66 C7 07 00 00 8B 0D");
        auto apply = Find("56 8B 74 24 08 8B 15 ? ? ? ? 8A 06 A2 ? ? ? ? 33 C0 8A 06 8D 0C C5 00 00 00 00 2B C8",
            "53 33 C9 56 8B 74 24 0C 8A 06 A2 ? ? ? ? 8A 0E 8D 04 89",
            "56 8B 74 24 08 8B 0D ? ? ? ? 8A 06 A2 ? ? ? ? 33 C0 8A 06 8D 04 80");
        if (list.size() != 1 || apply.size() != 1)
        { Log::Write("GTA1 resolution selector signatures unavailable."); return; }
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Install), &module);
        wchar_t path[MAX_PATH]{}; GetModuleFileNameW(module, path, MAX_PATH);
        IniPath = std::filesystem::path(path).replace_extension(L".ini").wstring();
        ModeIndex = *apply.get_first<uint8_t*>(Off(14, 11, 14));
        // The frontend's display reload and the style palette conversion of the in-game mode list.
        auto reload = Find("53 56 57 6A FF C7 05 ? ? ? ? 02 00 00 00 E8", "C7 05 ? ? ? ? 02 00 00 00 56 57 6A FF E8", "6A FF E8 ? ? ? ? 83 C4 04 C3");
        if (reload.size() == 1) Reload = reinterpret_cast<decltype(Reload)>(GTA1Build::Target(reload.get_first(Off(15, 14, 2))));
        auto palette = Find("E8 ? ? ? ? 83 C4 04 E8 ? ? ? ? 6A 01 E8 ? ? ? ? 8B 15", "E8 ? ? ? ? 83 C4 04 E8 ? ? ? ? 6A 01 E8 ? ? ? ? 33 C9",
            "E8 ? ? ? ? 83 C4 04 E8 ? ? ? ? 6A 01 E8 ? ? ? ? 8B 15");
        if (palette.size() == 1) Palette = reinterpret_cast<decltype(Palette)>(GTA1Build::Target(palette.get_first(8)));
        ListHook = safetyhook::create_inline(list.get_first(), List);
        ApplyHook = safetyhook::create_inline(apply.get_first(), Apply);
        // Text width, found through its call in the in-game mode list (the game has two
        // functions with identical code, only this one measures the mode labels).
        auto measure = Find("66 A3 ? ? ? ? 8B 0D ? ? ? ? 8B 15 ? ? ? ? 51 52 E8", "50 51 E8 ? ? ? ? 83 C4 08 83 C0 10 8B 0D",
            "66 A3 ? ? ? ? 8B 0D ? ? ? ? 8B 15 ? ? ? ? 51 52 E8");
        if (measure.size() == 1) MeasureHook = safetyhook::create_inline(GTA1Build::Target(measure.get_first(Off(20, 2, 20))), Measure);
        Log::Write("GTA1 modern resolution selector installed.");
    }
}
