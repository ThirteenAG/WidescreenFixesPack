module;

#include <stdafx.h>
#include <cstddef>

export module NativeMenuContracts;

export namespace BlacklistMenuContracts
{
    struct Contract
    {
        const char* name;
        const char* pattern;
        const char* caller = nullptr;
        ptrdiff_t callOffset = 0;
    };

    inline constexpr Contract DX11[] =
    {
        { "Settings", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 18 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B F9 89 7D EC C6 45 F3 01" },
        { "Perform", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 81 EC CC 03 00 00 A1 ? ? ? ? 33 C5 89 45 F0 53 56 57 50 8D 45 F4" },
        { "Back", "55 8B EC 51 A1 ? ? ? ? 33 D2 56 8B F1 85 C0 74 0D 8B 48 04 85 C9 7E 06" },
        { "Destroy", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 10 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B F1 89 75 F0 C7 06 ? ? ? ? C7 45 FC 4D 00 00 00" },
        { "Update", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 30 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B F9 89 7D EC E8 0E EF C6 FE" },
        { "AddRow", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 4C 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B D9 8B 4D 10 8B 55 0C" },
        { "AddTier", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 0C 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B F9 8B 87 68 02 00 00" },
        { "ClearList", "53 56 57 8B F9 33 DB 53 8D 8F 8C 02 00 00 89 9F B0 02 00 00 E8 A7 BE FE FF" },
        { "SelectRow", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 18 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B F1 8B 8E A8 02 00 00" },
        { "CloneChoice", "55 8B EC 53 56 57 6A 01 6A 10 8B F9 E8 8F 11 B9 FE 8B D8 8B F3 C1 E6 04" },
        { "ClearChoices", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 08 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B F9 33 C0 33 DB 39 47 04 7E 4C 89 45 F0 8B 37 03 75 F0 89 75 EC C7 45 FC 01 00 00 00 89 46 04 39 46 08 74 0E 6A 02 8B CE 89 46 08 E8 D7 09 B9 FE" },
        { "Register", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 08 53 56 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B F1 33 DB 39 5E 04 75 4F 53 53 6A 02 68 ? ? ? ? 6A 1C E8 C0 EE C1 FE" },
        { "RegisterInt", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 08 53 56 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B F1 33 DB 39 5E 04 75 4F 53 53 6A 02 68 ? ? ? ? 6A 1C E8 90 E6 C1 FE" },
        { "Disconnect", "55 8B EC 53 8B 5D 08 C7 03 00 00 00 00 8B 51 04 57 33 FF 85 D2 7E 6D 56" },
        { "PushTier", "55 8B EC 56 57 8B 7D 08 8B F1 3B BE 68 02 00 00 7D 18 8D 86 50 02 00 00" },
        { "PopTier", "56 8B F1 83 BE 50 02 00 00 00 7E 11 8D 8E 58 02 00 00 E8 99 10 D8 FE 89 86 54 02 00 00" },
        { "GetFacade", "55 8B EC 8B 45 08 8B 84 81 BC 01 00 00 5D C2 04 00 CC CC CC CC CC CC CC" },
        { "SetTitle", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 0C 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B F1 68 ? ? ? ? 8D 4D E8 E8 2A 78 A2 FE 33 FF" },
        { "DisableRow", "55 8B EC 56 57 8B 7D 08 8B F1 85 FF 0F 88 C4 00 00 00 3B BE 84 02 00 00" },
        { "CurrentLanguage", "B8 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC 55 8B EC 80 3D ? ? ? ? 00 74 2A 68 ? ? ? ? 68 ? ? ? ? E8 95 F5 FE FF" },
        { "Record", "55 8B EC 8B 41 10 56 85 C0 74 37 8B 55 08 8B 32 8B 51 0C 48 23 C6 8B 04 82 83 F8 FF 74 24 8B 11 6B C0 54 8B 0C 10 03 C2 83 F9 FE 7F 09 B8 FD FF FF FF 2B C1 EB 07 39 70 04 74 0E 8B C1 83 F8 FF 75 DE 33 C0 5E 5D C2 04 00 83 C0 08 5E 5D C2 04 00 CC CC CC CC CC CC CC CC CC CC CC CC CC CC CC 55 8B EC 8B 41 10 56 85 C0" },
        { "GameMode", "E8 2B E2 43 FF 8B 80 D0 06 00 00 85 C0 74 08 0F B6 80 7C 03 00 00 C3 B8 0B 00 00 00" },
        { "TopFocus", "8B 15 ? ? ? ? 33 C0 85 D2 74 0D 8B 4A 04 85 C9 7E 06 8B 02 8B 44 88 FC" },
        { "Control", "55 8B EC 51 8B C1 8B 88 68 02 00 00 83 F9 16 77 12 89 4D FC 8D 4D FC 51" },
    };

    inline constexpr Contract DX9[] =
    {
        { "Settings", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 14 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B 5D 0C C6 45 0F 01 E8 BC E7 2D 00" },
        { "Perform", "55 8B EC 83 E4 F8 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 81 EC D8 03 00 00" },
        { "Back", "55 8B EC 83 E4 F8 A1 ? ? ? ? 83 EC 0C 53 56 33 D2 57 8B D9 85 C0 74 0D" },
        { "Destroy", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 10 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B D9 89 5D F0 C7 03 ? ? ? ?" },
        { "Update", "55 8B EC 83 E4 F8 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 38 53 56 57 A1 ? ? ? ? 33 C4 50 8D 44 24 48 64 A3 00 00 00 00 8B D9 89 5C 24 18" },
        { "AddRow", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 4C 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B 75 08 FF 86 B0 02 00 00" },
        { "AddTier", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 0C 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B F9 8B 87 68 02 00 00" },
        { "ClearMap", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 51 53 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 33 FF 39 7E 04 7E 24 33 DB 8B FF 8B 06 03 C3 89 45 F0 C7 45 FC FF FF FF FF 83 C0 08 50 E8 09 5D FA FF 47" },
        { "Rehash", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 51 53 56 57 A1 ? ? ? ?", "Settings", 226 },
        { "ResizeArray", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 56 A1 ? ? ? ? 33 C5", "Settings", 250 },
        { "SelectRow", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 18 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B F1 8B 8E A8 02 00 00" },
        { "HideRows", "8B 87 84 02 00 00 56 33 F6 85 C0 7E 35 53 33 DB 85 F6 78 1F 3B F0 7D 1B" },
        { "CloneChoice", "55 8B EC 53 8B 5D 08 56 6A 01 6A 10 8B CB E8 CD 87 02 00 8B F0 C1 E6 04" },
        { "ClearChoices", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 08 53 56 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 33 DB 39 5F 04 7E 4E 89 5D F0" },
        { "Register", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 08 53 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 33 DB 39 5E 04 75 4F 53 53 6A 02 68 ? ? ? ? 6A 1C E8 33 D1 2C 00 83 C4 14" },
        { "RegisterInt", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 08 53 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 33 DB 39 5E 04 75 4F 53 53 6A 02 68 ? ? ? ? 6A 1C E8 63 A5 2C 00 83 C4 14" },
        { "Disconnect", "55 8B EC 53 8B 5D 08 C7 03 00 00 00 00 8B 51 04 57 33 FF 85 D2 7E 6D 56" },
        { "PopTier", "57 8B F8 83 BF 50 02 00 00 00 7E 11 8D 8F 58 02 00 00 E8 79 73 01 00 89 87 54 02 00 00" },
        { "HistoryPush", "55 8B EC 53 56 8B F1 8B 5E 04 8D 4B 01 89 4E 04 3B 4E 08 7E 2F 8B C1 99 83 E2 07 03 C2 C1 F8 03 83 3D ? ? ? ? 00 57 8D 79 08 8D 44 08 03 74 06 3B F8 7F 02 8B C7 6A 04 8B CE 89 46 08 E8 3D 55 00 00 5F 8B 06 8B 4D 08" },
        { "SwitchTier", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 20 53 56 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B 87 50 02 00 00 8B 97 64 02 00 00" },
        { "SetTitle", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 10 56 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 68 ? ? ? ? 8D 4D E8 E8 4D 7E FF FF" },
        { "CurrentLanguage", "B8 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC 55 8B EC 80 3D ? ? ? ? 00 74 2A 68 ? ? ? ? 68 ? ? ? ? E8 C5 F6 FE FF" },
        { "Record", "8B 41 10 56 85 C0 74 3A 8B 32 8B 51 0C 48 23 C6 8B 04 82 83 F8 FF 74 2A 8B 11 8D 9B 00 00 00 00 6B C0 54 8B 0C 10 03 C2 83 F9 FE 7F 09 B8 FD FF FF FF 2B C1 EB 07 39 70 04 74 0B 8B C1 83 F8 FF 75 DE 33 C0 5E C3 83 C0 08 5E C3 CC CC CC CC CC 55 8B EC 8B 55 0C 8B C1" },
        { "LevelInfo", "E8 AB 80 8B FF 85 C0 74 1C 8B 10 8B C8 8B 82 14 01 00 00 FF D0 85 C0 74 0C" },
        { "TopFocus", "8B 15 ? ? ? ? 33 C0 85 D2 74 0D 8B 4A 04 85 C9 7E 06 8B 02 8B 44 88 FC C3 CC CC CC CC CC CC" },
        { "Control", "55 8B EC 51 8B C1 8B 88 68 02 00 00 83 F9 16 77 11 89 4D FC 8D 88 50 02 00 00" },
    };
}
