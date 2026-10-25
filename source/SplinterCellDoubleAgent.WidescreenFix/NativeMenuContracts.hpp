#pragma once

// The two PC Engine.dll variants, each with its own RVAs, function bodies
// (relocated bytes masked) and data references.
namespace DAMenu
{
    struct Contract { uint32_t rva; const char* bytes; };
    inline bool Matches(HMODULE module, Contract contract, const SafetyHookInline* owned = nullptr)
    {
        auto address = uintptr_t(module) + contract.rva;
        if (owned && owned->target_address() == address)
        {
            const auto& original = owned->original_bytes();
            auto live = reinterpret_cast<const uint8_t*>(address);
            // The bundled Win32 SafetyHook routes through its own two-E9
            // epilogue. These two native prologues have no branch expansion.
            const auto& trampoline = owned->trampoline();
            if (!owned->enabled() || original.size() < 5 || original.size() > 64 ||
                trampoline.size() != original.size() + 10 || live[0] != 0xE9) return false;
            auto callbackJump = trampoline.address() + trampoline.size() - 5;
            auto originalJump = callbackJump - 5;
            auto destination = [](uintptr_t at) { return at + 5 + *reinterpret_cast<const int32_t*>(at + 1); };
            if (destination(address) != callbackJump || *reinterpret_cast<const uint8_t*>(callbackJump) != 0xE9 ||
                destination(callbackJump) != owned->destination_address() ||
                *reinterpret_cast<const uint8_t*>(originalJump) != 0xE9 ||
                destination(originalJump) != address + original.size()) return false;
            for (size_t i = 5; i < original.size(); ++i) if (live[i] != 0x90) return false;
            std::array<uint8_t, 64> body;
            std::copy_n(live, body.size(), body.begin());
            std::copy(original.begin(), original.end(), body.begin());
            auto start = reinterpret_cast<uintptr_t>(body.data());
            auto match = hook::range_pattern(start, start + body.size(), contract.bytes);
            return match.size() == 1 && match.get_first() == body.data();
        }
        auto match = hook::range_pattern(address, address + 64, contract.bytes);
        return match.size() == 1 && match.get_first() == reinterpret_cast<void*>(address);
    }
    namespace Contracts
    {
        enum Index { CreateArea, CreateNode, CreateFrame, AddNode, AddFrame, LinkNeighbors, AreaTime, AddHandler, CreateHandler, FindNode, WideAssign, SelectNode, BlurNode, PageDestroy, Display, Uninitialize, InputTick, AddPackageChild, FindPage, GetTop, GetPackage, GetInteraction, AddCursor, Count };
        inline constexpr std::array<const char*, Count> Names = { "CreateArea", "CreateNode", "CreateFrame", "AddNode", "AddFrame", "LinkNeighbors", "AreaTime", "AddHandler", "CreateHandler", "FindNode", "WideAssign", "SelectNode", "BlurNode", "PageDestroy", "Display", "Uninitialize", "InputTick", "AddPackageChild", "FindPage", "GetTop", "GetPackage", "GetInteraction", "AddCursor" };
        struct Layout
        {
            const char* name;
            std::array<Contract, Count> functions;
            Contract factoryGetter, handlerConstructor;
            uint32_t factoryRva, handlerRva;
            std::array<uint32_t, 14> handlerTargets;
        };
        inline constexpr Layout Steam{ "Steam", {{
            { 0x3DA7F0, "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 8B 44 24 10 83 F8 04 64 89 25 00 00 00 00 0F 87 0E 01" }, // CreateArea
            { 0x3DACE0, "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 53 56 57 8B 7C 24 1C 83 FF 0C 8B" }, // CreateNode
            { 0x3DA530, "6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 56 57 6A 00 6A 10 8B F9 E8 FD" }, // CreateFrame
            { 0x3EF090, "8D 44 24 04 50 83 C1 08 E8 83 FF FF FF C2 04 00" }, // AddNode
            { 0x3EE220, "6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 56 8B F1 8B 4E 0C 57 33 FF 3B" }, // AddFrame
            { 0x3D24A0, "53 55 56 57 8B 7C 24 14 33 ED 8D 9B 00 00 00 00 8B 4F 0C 85 C9 75 04 33 C0 EB 08 8B 47 10 2B C1" }, // LinkNeighbors
            { 0x3EE950, "53 8B 5C 24 08 56 57 8B F9 8B 77 0C 3B 77 10 89 5F 18 74 24 55 8B 6C 24 18 8D A4 24 00 00 00 00" }, // AreaTime
            { 0x3EC440, "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 8D 44 24 10 50 83 C1 40 C7 44 24" }, // AddHandler
            { 0x3DA690, "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 8B 44 24 10 48 64 89 25 00 00 00 00 75 35 6A 00 6A 40" }, // CreateHandler
            { 0x3EE7F0, "53 56 8B 71 0C 57 8B 79 10 3B F7 74 1B 8B 5C 24 10 8B 0E 8D 44 24 10 50 E8 93 FB DD FF 39 18 74" }, // FindNode
            { 0x1D25D0, "53 55 56 8B F1 8B 4E 18 83 F9 08 8D 5E 04 72 04 8B 03 EB 02 8B C3 8B 6C 24 10 3B E8 72 34 83 F9" }, // WideAssign
            { 0x3EBC20, "56 8B F1 8A 46 3C A8 02 74 16 8B 4E 08 0C 08 88 46 3C 8B 01 56 FF 50 40 8B CE 5E E9 70 FC FF FF" }, // SelectNode
            { 0x3EBC50, "56 8B F1 8A 46 3C 8B 4E 08 24 F7 88 46 3C 8B 01 56 FF 50 40 8B CE 5E E9 84 FC FF FF" }, // BlurNode
            { 0x1AC960, "56 8B F1 E8 C8 FE FF FF F6 44 24 08 01 74 09 56 E8 FB 96 E5 FF 83 C4 04 8B C6 5E C2 04 00" }, // PageDestroy
            { 0x1CEC90, "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 0C 53 8B 5D 08 85" }, // Display
            { 0x1D1C20, "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 0C 53 56 8B F1 57" }, // Uninitialize
            { 0x1D5DD0, "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 14 53 56 57 8B F9" }, // InputTick
            { 0x3E7E50, "8D 44 24 04 50 83 C1 04 E8 23 FE FF FF C2 04 00" }, // AddPackageChild
            { 0x3E6DD0, "53 56 8B 71 08 57 8B 79 0C 3B F7 74 1B 8B 5C 24 10 8B 0E 8D 44 24 10 50 E8 E3 AF DB FF 39 18 74" }, // FindPage
            { 0x1CF020, "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 0C 53 56 57 89 65" }, // GetTop
            { 0x1CF7D0, "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 83 EC 1C 80 3D ? ? ? ? 00 56" }, // GetPackage
            { 0x1A2450, "8B 41 08 C3" }, // GetInteraction
            { 0x1D0CF0, "6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 20 56 57 8D 44 24 0C 68 ?" }, // AddCursor
        }},
            { 0x3DA2F0, "A1 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC" },
            { 0x3FBC10, "8B C1 8A 4C 24 04 C7 40 04 00 00 00 00 88 48 08" },
            0xEB530C, 0x5EDC04, { 0x3FC090, 0x3FBC60, 0x3FBD50, 0x3FBD60, 0x3FBD70, 0x3FBD80, 0x3FBD90, 0x3FBDA0, 0x3FBDB0, 0x3FBDC0, 0x3FBDD0, 0x3FBDE0, 0x3FBDF0, 0x3FBE00 } };
        inline constexpr Layout Retail{ "Retail", {{
            { 0x3DA9B0, "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 8B 44 24 10 83 F8 04 64 89 25 00 00 00 00 0F 87 0E 01" }, // CreateArea
            { 0x3DAEA0, "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 53 56 57 8B 7C 24 1C 83 FF 0C 8B" }, // CreateNode
            { 0x3DA6F0, "6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 56 57 6A 00 6A 10 8B F9 E8 3D" }, // CreateFrame
            { 0x3EF250, "8D 44 24 04 50 83 C1 08 E8 83 FF FF FF C2 04 00" }, // AddNode
            { 0x3EE3E0, "6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 56 8B F1 8B 4E 0C 57 33 FF 3B" }, // AddFrame
            { 0x3D2660, "53 55 56 57 8B 7C 24 14 33 ED 8D 9B 00 00 00 00 8B 4F 0C 85 C9 75 04 33 C0 EB 08 8B 47 10 2B C1" }, // LinkNeighbors
            { 0x3EEB10, "53 8B 5C 24 08 56 57 8B F9 8B 77 0C 3B 77 10 89 5F 18 74 24 55 8B 6C 24 18 8D A4 24 00 00 00 00" }, // AreaTime
            { 0x3EC600, "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 8D 44 24 10 50 83 C1 40 C7 44 24" }, // AddHandler
            { 0x3DA850, "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 8B 44 24 10 48 64 89 25 00 00 00 00 75 35 6A 00 6A 40" }, // CreateHandler
            { 0x3EE9B0, "53 56 8B 71 0C 57 8B 79 10 3B F7 74 1B 8B 5C 24 10 8B 0E 8D 44 24 10 50 E8 F3 F9 DD FF 39 18 74" }, // FindNode
            { 0x1D25F0, "53 55 56 8B F1 8B 4E 18 83 F9 08 8D 5E 04 72 04 8B 03 EB 02 8B C3 8B 6C 24 10 3B E8 72 34 83 F9" }, // WideAssign
            { 0x3EBDE0, "56 8B F1 8A 46 3C A8 02 74 16 8B 4E 08 0C 08 88 46 3C 8B 01 56 FF 50 40 8B CE 5E E9 70 FC FF FF" }, // SelectNode
            { 0x3EBE10, "56 8B F1 8A 46 3C 8B 4E 08 24 F7 88 46 3C 8B 01 56 FF 50 40 8B CE 5E E9 84 FC FF FF" }, // BlurNode
            { 0x1AC960, "56 8B F1 E8 C8 FE FF FF F6 44 24 08 01 74 09 56 E8 FB 96 E5 FF 83 C4 04 8B C6 5E C2 04 00" }, // PageDestroy
            { 0x1CECB0, "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 0C 53 8B 5D 08 85" }, // Display
            { 0x1D1C40, "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 0C 53 56 8B F1 57" }, // Uninitialize
            { 0x1D5DF0, "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 14 53 56 57 8B F9" }, // InputTick
            { 0x3E8010, "8D 44 24 04 50 83 C1 04 E8 23 FE FF FF C2 04 00" }, // AddPackageChild
            { 0x3E6F90, "53 56 8B 71 08 57 8B 79 0C 3B F7 74 1B 8B 5C 24 10 8B 0E 8D 44 24 10 50 E8 23 AE DB FF 39 18 74" }, // FindPage
            { 0x1CF040, "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 0C 53 56 57 89 65" }, // GetTop
            { 0x1CF7F0, "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 83 EC 1C 80 3D ? ? ? ? 00 56" }, // GetPackage
            { 0x1A2450, "8B 41 08 C3" }, // GetInteraction
            { 0x1D0D10, "6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 20 56 57 8D 44 24 0C 68 ?" }, // AddCursor
        }},
            { 0x3DA4B0, "A1 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC" },
            { 0x3FBDD0, "8B C1 8A 4C 24 04 C7 40 04 00 00 00 00 88 48 08" },
            0xEB532C, 0x5EDBBC, { 0x3FC250, 0x3FBE20, 0x3FBF10, 0x3FBF20, 0x3FBF30, 0x3FBF40, 0x3FBF50, 0x3FBF60, 0x3FBF70, 0x3FBF80, 0x3FBF90, 0x3FBFA0, 0x3FBFB0, 0x3FBFC0 } };
        inline const char* Mismatch(HMODULE module, const Layout& candidate, const SafetyHookInline* addFrame = nullptr, const SafetyHookInline* display = nullptr)
        {
            for (size_t i = 0; i < Count; ++i)
                if (!Matches(module, candidate.functions[i], i == AddFrame ? addFrame : i == Display ? display : nullptr)) return Names[i];
            if (!Matches(module, candidate.factoryGetter)) return "FactoryGetter";
            if (!Matches(module, candidate.handlerConstructor)) return "HandlerConstructor";
            auto base = reinterpret_cast<uint8_t*>(module);
            // The getter's A1 operand is relocated by the native PE loader.
            if (*reinterpret_cast<uint32_t*>(base + candidate.factoryGetter.rva + 1) !=
                uintptr_t(module) + candidate.factoryRva) return "FactorySingleton";
            auto table = reinterpret_cast<uint32_t*>(base + candidate.handlerRva);
            for (size_t i = 0; i < candidate.handlerTargets.size(); ++i)
                if (table[i] != uintptr_t(module) + candidate.handlerTargets[i]) return "HandlerVtable";
            return nullptr;
        }
        inline const Layout* Select(HMODULE module, const SafetyHookInline* addFrame, const SafetyHookInline* display)
        {
            for (auto candidate : { &Steam, &Retail })
                if (!Mismatch(module, *candidate, addFrame, display)) return candidate;
            return nullptr;
        }
    }
    inline const Contracts::Layout* layout = nullptr;
}
