module;

#include <stdafx.h>

export module NativeMenuContracts;

// Engine.dll menu functions that aren't exported. The patterns match the Steam and the retail Engine.dll.
export namespace DAMenu
{
    namespace Contracts
    {
        inline constexpr const char* CreateArea = "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 8B 44 24 10 83 F8 04 64 89 25 00 00 00 00 0F 87";
        inline constexpr const char* CreateNode = "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 53 56 57 8B 7C 24 1C 83 FF 0C 8B";
        inline constexpr const char* CreateFrame = "6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 56 57 6A 00 6A 10 8B F9";
        inline constexpr const char* AddNode = "8D 44 24 04 50 83 C1 08 E8 83 FF FF FF C2 04 00";
        inline constexpr const char* AddFrame = "6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 56 8B F1 8B 4E 0C 57 33 FF 3B";
        inline constexpr const char* LinkNeighbors = "53 55 56 57 8B 7C 24 14 33 ED 8D 9B 00 00 00 00 8B 4F 0C 85 C9 75 04 33 C0 EB 08 8B 47 10 2B C1";
        inline constexpr const char* AreaTime = "53 8B 5C 24 08 56 57 8B F9 8B 77 0C 3B 77 10 89 5F 18 74 24 55 8B 6C 24 18";
        inline constexpr const char* AddHandler = "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 8D 44 24 10 50 83 C1 40 C7 44 24";
        inline constexpr const char* CreateHandler = "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 8B 44 24 10 48 64 89 25 00 00 00 00 75 35 6A 00 6A 40";
        inline constexpr const char* FindNode = "53 56 8B 71 0C 57 8B 79 10 3B F7 74 1B 8B 5C 24 10 8B 0E 8D 44 24 10 50 E8";
        inline constexpr const char* WideAssign = "53 55 56 8B F1 8B 4E 18 83 F9 08 8D 5E 04 72 04 8B 03 EB 02 8B C3 8B 6C 24 10 3B E8 72 34 83 F9";
        inline constexpr const char* SelectNode = "56 8B F1 8A 46 3C A8 02 74 16 8B 4E 08 0C 08 88 46 3C 8B 01 56 FF 50 40 8B CE 5E E9";
        inline constexpr const char* BlurNode = "56 8B F1 8A 46 3C 8B 4E 08 24 F7 88 46 3C 8B 01 56 FF 50 40 8B CE 5E E9";
        inline constexpr const char* PageDestroy = "56 8B F1 E8 C8 FE FF FF F6 44 24 08 01 74 09 56 E8 FB 96 E5 FF 83 C4 04 8B C6 5E C2 04 00";
        inline constexpr const char* AddPackageChild = "8D 44 24 04 50 83 C1 04 E8 23 FE FF FF C2 04 00";
        inline constexpr const char* FindPage = "53 56 8B 71 08 57 8B 79 0C 3B F7 74 1B 8B 5C 24 10 8B 0E 8D 44 24 10 50 E8";
        // mov eax, [factory]; ret
        inline constexpr const char* FactoryGetter = "A1 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC 8B 0D";
        // Handler constructor: mov dword ptr [eax], vtable at +25
        inline constexpr const char* HandlerConstructor = "8B C1 8A 4C 24 04 C7 40 04 00 00 00 00 88 48 08 C7 40 0C ? ? ? ? C7 00 ? ? ? ? C7 40 0C ? ? ? ? 33 D2";
    }
}
