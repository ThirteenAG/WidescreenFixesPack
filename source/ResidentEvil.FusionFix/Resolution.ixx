module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Resolution;

// The game's resolution list: every display mode instead of the predefined list that stopped at 2560x1600. The game
// prints the display modes with sprintf; the fix collects them, sorts and de-duplicates them and points the game's
// list tables, counts and lookups at its own list. The 2026 update was rebuilt with a newer compiler, so most sites
// have a second pattern.

namespace
{
    bool bUpdatedRelease = false; // 2026 update, rebuilt with a newer compiler

    std::unordered_map<std::string, hook::pattern> Patterns;

    hook::pattern ResolutionPattern(const char* primary, const char* updated = nullptr)
    {
        auto lookup = [](const char* signature) -> hook::pattern
        {
            auto [it, inserted] = Patterns.try_emplace(signature, signature);
            it->second.size();
            return it->second;
        };
        auto pattern = lookup(primary);
        return pattern.empty() && updated ? lookup(updated) : pattern;
    }

    struct StringRes
    {
        const char* entry;
        uint32_t id;
    }; std::vector<StringRes> resListString;

    struct NumRes
    {
        uint32_t x;
        uint32_t y;
        uint32_t id;
    }; std::vector<NumRes> resListNum;

    bool ResolutionListReady = false;
    std::vector<std::string> resList;
    const char* __fastcall GetResString(void* _this, int edx, unsigned int id)
    {
        if (resList.empty()) return "1024x768";
        return resList[std::min<size_t>(id, resList.size() - 1)].c_str();
    }

    int sprintfHook(char* str, const char* format, int x, int y)
    {
        auto result = sprintf(str, format, x, y);
        if (!ResolutionListReady && x >= 640 && y >= 480)
            resList.push_back(str);
        return result;
    }

    int __stdcall GetResID(int a1, int a2)
    {
        auto it = std::find_if(resListNum.begin(), resListNum.end(), [&] (auto& m) -> bool { return m.x == a1 && m.y == a2; });
        if (it != resListNum.end())
            return it->id;
        return 0;
    }
}

export namespace Resolution
{
    void Init()
    {
        bUpdatedRelease = ResolutionPattern("8B 04 C5 ? ? ? ? 57").empty();
        if (!FixSettings().Bool("MAIN.UnlockAllResolutions"))
            return;

        // Resolve the complete table redirection before changing its producer or consumers.
        const bool unavailable = bUpdatedRelease
            ? (ResolutionPattern("E8 ? ? ? ? 83 C4 20 47").size() != 1 ||
            ResolutionPattern("33 DB 33 ED 89 5C 24 58", "33 C0 33 FF 89 44 24 2C").size() != 1 ||
            ResolutionPattern("8B 0C D5 ? ? ? ? 8B C6 8A 10").size() != 1 ||
            ResolutionPattern("83 FA 0C 72 82").size() != 1 ||
            ResolutionPattern("8B 0C DD ? ? ? ? 8B 84 F2").size() != 1 ||
            ResolutionPattern("83 FB 0C 72 87").size() != 1 ||
            ResolutionPattern("8B 04 F5 ? ? ? ? 8B CF 0F 1F 00 8A 11 3A 10 75 19").size() != 1 ||
            ResolutionPattern("83 FE 0C 72 C6").size() != 1 ||
            ResolutionPattern("83 F8 0B 77 63 56").size() != 1 ||
            ResolutionPattern("8B 04 C5 ? ? ? ? 89 44 24 14").size() != 1 ||
            ResolutionPattern("8B 0C F5 ? ? ? ? 8B C3 0F 1F 80").size() != 1 ||
            ResolutionPattern("83 FE 0C 72 C1").size() != 1 ||
            ResolutionPattern("8B 84 F2 ? ? ? ? B9 ? ? ? ? 0F 1F 40 00").size() != 1 ||
            ResolutionPattern("FF 34 85 ? ? ? ? E8 ? ? ? ? 6A 01 6A 20").size() != 1 ||
            ResolutionPattern("E8 ? ? ? ? 6A 01 6A 20").size() != 1 ||
            ResolutionPattern("81 F9 00 04 00 00 75 0B 3D 00 03 00 00 0F 84").size() != 1 ||
            ResolutionPattern("6A 08 89 83 ? ? ? ? 8B 0D ? ? ? ? 68").size() != 1 ||
            ResolutionPattern("0F 87 ? ? ? ? FF 24 85 ? ? ? ? C7 04 24", "0F 87 ? ? ? ? FF 24 85 ? ? ? ? C7 44 24 04").size() != 1)
            : (ResolutionPattern("E8 ? ? ? ? 83 C4 20 47").size() != 1 ||
            ResolutionPattern("33 DB 33 ED 89 5C 24 58", "33 C0 33 FF 89 44 24 2C").size() != 1 ||
            ResolutionPattern("B9 ? ? ? ? 89 44 24 0C 89 74 24 28").size() != 1 ||
            ResolutionPattern("BA ? ? ? ? 89 44 24 0C 89 4C 24 14").size() != 1 ||
            ResolutionPattern("83 FE 0C 72 C2").size() != 1 ||
            ResolutionPattern("83 F8 0B 77 65 53").size() != 1 ||
            ResolutionPattern("83 C6 08 83 FE 60").size() != 1 ||
            ResolutionPattern("8B 8E ? ? ? ? 8B C7 8D A4 24").size() != 1 ||
            ResolutionPattern("BE ? ? ? ? B9 ? ? ? ? 89 44 24 0C").size() != 1 ||
            ResolutionPattern("B9 ? ? ? ? BA ? ? ? ? 89 44 24 0C").size() != 1 ||
            ResolutionPattern("8B 04 F5 ? ? ? ? 8B CF 8D A4 24 ? ? ? ? 8A 11 3A 10 75 19").size() != 1 ||
            ResolutionPattern("8B 04 C5 ? ? ? ? 57").size() != 1 ||
            ResolutionPattern("8B 0D ? ? ? ? 8B 07").size() != 1 ||
            ResolutionPattern("FF 34 85 ? ? ? ? E8 ? ? ? ? 6A 01 6A 20").size() != 1 ||
            ResolutionPattern("E8 ? ? ? ? 6A 01 6A 20").size() != 1 ||
            ResolutionPattern("E8 ? ? ? ? 6A 08 89 83").size() != 1 ||
            ResolutionPattern("0F 87 ? ? ? ? FF 24 85 ? ? ? ? C7 04 24", "0F 87 ? ? ? ? FF 24 85 ? ? ? ? C7 44 24 04").size() != 1);
        if (unavailable)
        {
            OutputDebugStringA("Resident Evil: resolution table patterns unavailable; native list retained.\n");
            return;
        }

        // Gather a resolution list from the game
        auto pattern = ResolutionPattern("E8 ? ? ? ? 83 C4 20 47");
        injector::MakeCALL(pattern.get_first(), sprintfHook, true);

        // Hook right after sprintf loop
        pattern = ResolutionPattern("33 DB 33 ED 89 5C 24 58", "33 C0 33 FF 89 44 24 2C");
        struct ResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                if (!bUpdatedRelease)
                {
                    regs.ebx = 0;
                    regs.ebp = 0;
                    *(uint32_t*)(regs.esp + 0x58) = regs.ebx;
                }
                else
                {
                    regs.eax = 0;
                    regs.edi = 0;
                    *(uint32_t*)(regs.esp + 0x2C) = regs.eax;
                }

                if (ResolutionListReady)
                    return;

                resListString.clear();
                resListNum.clear();
                if (resList.empty()) resList.emplace_back("1024x768");

                std::sort(resList.begin(), resList.end(), [](const std::string& lhs, const std::string& rhs)
                {
                    int32_t x1 = 0, y1 = 0, x2 = 0, y2 = 0;
                    sscanf_s(lhs.c_str(), "%dx%d", &x1, &y1);
                    sscanf_s(rhs.c_str(), "%dx%d", &x2, &y2);
                    return (x1 != x2) ? (x1 < x2) : (x1 * y1 < x2 * y2);
                });
                resList.erase(std::unique(std::begin(resList), std::end(resList)), resList.end());

                if (resList.size() > 127) resList.resize(127); //native signed-byte comparisons

                for (size_t idx = 0; idx < resList.size(); idx++)
                {
                    int x = 0, y = 0;
                    auto str = resList[idx].c_str();
                    sscanf_s(str, "%dx%d", &x, &y);
                    resListString.emplace_back(str, idx);
                    resListNum.emplace_back(x, y, idx);
                }

                if (!bUpdatedRelease)
                {
                    auto pattern = ResolutionPattern("B9 ? ? ? ? 89 44 24 0C 89 74 24 28");
                    WriteGameValue(pattern.get_first(1), resList.size());
                    pattern = ResolutionPattern("BA ? ? ? ? 89 44 24 0C 89 4C 24 14");
                    WriteGameValue(pattern.get_first(1), resList.size());
                    pattern = ResolutionPattern("83 FE 0C 72 C2");
                    WriteGameValue<uint8_t>(pattern.get_first(2), static_cast<uint8_t>(resList.size()));
                    pattern = ResolutionPattern("83 F8 0B 77 65 53");
                    WriteGameValue<uint8_t>(pattern.get_first(2), static_cast<uint8_t>(resList.size() - 1));
                    pattern = ResolutionPattern("83 C6 08 83 FE 60");
                    static auto loc_47D471 = ResolutionPattern("8B 8E ? ? ? ? 8B C7 8D A4 24").get_first();
                    struct LoopBreakHook
                    {
                        void operator()(injector::reg_pack& regs)
                        {
                            regs.esi += 8;
                            if (regs.esi >= resList.size() * 8)
                                *(void**)(regs.esp - 4) = loc_47D471;
                        }
                    }; injector::MakeInline<LoopBreakHook>(pattern.get_first(0), pattern.get_first(8));

                    pattern = ResolutionPattern("BE ? ? ? ? B9 ? ? ? ? 89 44 24 0C");
                    WriteGameValue(pattern.get_first(1), resListString.data());
                    pattern = ResolutionPattern("B9 ? ? ? ? BA ? ? ? ? 89 44 24 0C");
                    WriteGameValue(pattern.get_first(1), resListString.data());
                    pattern = ResolutionPattern("8B 04 F5 ? ? ? ? 8B CF 8D A4 24 ? ? ? ? 8A 11 3A 10 75 19");
                    WriteGameValue(pattern.get_first(3), resListString.data());
                    pattern = ResolutionPattern("8B 04 C5 ? ? ? ? 57");
                    WriteGameValue(pattern.get_first(3), resListString.data());
                    pattern = ResolutionPattern("8B 8E ? ? ? ? 8B C7 8D A4 24");
                    WriteGameValue(pattern.get_first(2), resListString.data());
                    pattern = ResolutionPattern("8B 0D ? ? ? ? 8B 07");
                    WriteGameValue(pattern.get_first(2), resListString.data());
                }
                else
                {
                    // The table is indexed directly and the loop counters are 8-bit compares
                    auto pattern = ResolutionPattern("8B 0C D5 ? ? ? ? 8B C6 8A 10");
                    WriteGameValue(pattern.get_first(3), resListString.data());
                    pattern = ResolutionPattern("83 FA 0C 72 82");
                    WriteGameValue<uint8_t>(pattern.get_first(2), static_cast<uint8_t>(resList.size()));
                    pattern = ResolutionPattern("8B 0C DD ? ? ? ? 8B 84 F2");
                    WriteGameValue(pattern.get_first(3), resListString.data());
                    pattern = ResolutionPattern("83 FB 0C 72 87");
                    WriteGameValue<uint8_t>(pattern.get_first(2), static_cast<uint8_t>(resList.size()));
                    pattern = ResolutionPattern("8B 04 F5 ? ? ? ? 8B CF 0F 1F 00 8A 11 3A 10 75 19");
                    WriteGameValue(pattern.get_first(3), resListString.data());
                    pattern = ResolutionPattern("83 FE 0C 72 C6");
                    WriteGameValue<uint8_t>(pattern.get_first(2), static_cast<uint8_t>(resList.size()));
                    pattern = ResolutionPattern("83 F8 0B 77 63 56");
                    WriteGameValue<uint8_t>(pattern.get_first(2), static_cast<uint8_t>(resList.size() - 1));
                    pattern = ResolutionPattern("8B 04 C5 ? ? ? ? 89 44 24 14");
                    WriteGameValue(pattern.get_first(3), resListString.data());
                    pattern = ResolutionPattern("8B 0C F5 ? ? ? ? 8B C3 0F 1F 80");
                    WriteGameValue(pattern.get_first(3), resListString.data());
                    pattern = ResolutionPattern("83 FE 0C 72 C1");
                    WriteGameValue<uint8_t>(pattern.get_first(2), static_cast<uint8_t>(resList.size()));

                    // "1280x720" is an immediate here instead of a table read, use the same entry as the original release
                    pattern = ResolutionPattern("8B 84 F2 ? ? ? ? B9 ? ? ? ? 0F 1F 40 00");
                    WriteGameValue(pattern.get_first(8), resListString[0].entry);
                }

                auto pattern = ResolutionPattern("FF 34 85 ? ? ? ? E8 ? ? ? ? 6A 01 6A 20");
                WriteGameValue(pattern.get_first(3), &resListNum[0].id);

                pattern = ResolutionPattern("E8 ? ? ? ? 6A 01 6A 20");
                injector::MakeCALL(pattern.get_first(), GetResString, true);

                if (!bUpdatedRelease)
                {
                    pattern = ResolutionPattern("E8 ? ? ? ? 6A 08 89 83");
                    injector::MakeCALL(pattern.get_first(), GetResID, true);
                }
                else
                {
                    // GetResID is inlined, width is in ecx and height is in eax
                    pattern = ResolutionPattern("81 F9 00 04 00 00 75 0B 3D 00 03 00 00 0F 84");
                    static auto loc_5284C4 = ResolutionPattern("6A 08 89 83 ? ? ? ? 8B 0D ? ? ? ? 68").get_first();
                    struct GetResIDHook
                    {
                        void operator()(injector::reg_pack& regs)
                        {
                            regs.eax = GetResID(regs.ecx, regs.eax);
                            *(void**)(regs.esp - 4) = loc_5284C4;
                        }
                    }; injector::MakeInline<GetResIDHook>(pattern.get_first(0), pattern.get_first(6));
                }

                pattern = ResolutionPattern("0F 87 ? ? ? ? FF 24 85 ? ? ? ? C7 04 24", "0F 87 ? ? ? ? FF 24 85 ? ? ? ? C7 44 24 04");
                struct GetResValueHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        auto nStackOffset = bUpdatedRelease ? 0x04 : 0x00;
                        *(uint32_t*)(regs.esp + nStackOffset + 0x00) = 1024;
                        *(uint32_t*)(regs.esp + nStackOffset + 0x04) = 768;

                        auto it = std::find_if(resListNum.begin(), resListNum.end(), [&](auto& m) -> bool { return m.id == regs.eax; });
                        if (it != resListNum.end())
                        {
                            *(uint32_t*)(regs.esp + nStackOffset + 0x00) = it->x;
                            *(uint32_t*)(regs.esp + nStackOffset + 0x04) = it->y;
                        }
                    }
                }; injector::MakeInline<GetResValueHook>(pattern.get_first(0), pattern.get_first(bUpdatedRelease ? 29 : 28));
                ResolutionListReady = true;
            }
        }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(8));
    }
}
