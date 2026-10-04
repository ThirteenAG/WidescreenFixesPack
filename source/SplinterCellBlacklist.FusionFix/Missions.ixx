module;

#include <stdafx.h>
#include <sstream>
#include <format>
#include <array>

export module Missions;

import ComVars;

namespace ExtractionOnOtherMaps { void OnGameModeSet(int gameMode); }

SafetyHookInline shLead_SetCurrentGameMode{};
void __cdecl Lead_SetCurrentGameMode(int gameMode, int a2)
{
    CurrentGameMode = gameMode;
    ExtractionOnOtherMaps::OnGameModeSet(gameMode);
    return shLead_SetCurrentGameMode.ccall(gameMode, a2);
}

std::string sExtractionWaveConfigs = "Default";
int nExtractionWaveEnemyMultiplier = 1;
int nExtractionWaveEnemyRandomRangeMin = 0;
int nExtractionWaveEnemyRandomRangeMax = 4;

namespace ExtractionSubWaveEnemy
{
    int curWaveEnemyCount = 0;
    int curStartConditionType = 0;
    injector::hook_back<int(__cdecl*)(const char*)> hbappAtoi;

    int __cdecl appAtoi(const char* String)
    {
        auto i = hbappAtoi.fun(String);
        if (iequals(sExtractionWaveConfigs, "Random"))
            i *= GetRandomInt(nExtractionWaveEnemyRandomRangeMin, nExtractionWaveEnemyRandomRangeMax);
        else
            i *= nExtractionWaveEnemyMultiplier;
        curWaveEnemyCount += i;
        return i;
    }
}

// Charlie missions on the maps of the other co-op modes: the waves are given to the level's AI spawner, a Hunter (Kobin) map's
// AECoopHunterSpawner ignores them (virtual 4FCh is empty), only AECoopExtractionSpawner (derived from it, 548h bytes, the same properties)
// takes them. In Extraction mode such a spawner is made as an extraction spawner when the level loads.
namespace ExtractionOnOtherMaps
{
    void** pHunterSpawnerClass = nullptr;
    void** pExtractionSpawnerClass = nullptr;

    // AECoopExtractionSpawner: an enemy's spawn group (request +520h, FString +10h) is looked up by name among the spawner's groups
    // (+38Ch or +398h, group name FString +368h). The maps of other modes don't have the groups a wave config names, the enemy is then
    // spawned the Hunter way (any of the map's groups).
    SafetyHookInline shSpawnFromGroup{};
    int32_t(__fastcall* SpawnFromAnyGroup)(uint8_t* spawner, void* edx, void* a2, int32_t a3) = nullptr;

    struct FString { const char* data; int32_t count; int32_t max; };

    bool IsA(uint8_t* object, void* objectClass)
    {
        for (auto c = *reinterpret_cast<uint8_t**>(object + 0x24); c; c = *reinterpret_cast<uint8_t**>(c + 0x28)) // UStruct super
            if (c == objectClass)
                return true;
        return false;
    }

    bool SpawnAtAttractionPoint(uint8_t* spawner, uint32_t* spawningInfo, int32_t initial);

    int32_t __fastcall SpawnFromGroup(uint8_t* spawner, void* edx, void* a2, int32_t a3)
    {
        // a spawner made for a map without spawn groups (Grim's maps)
        if (reinterpret_cast<FString*>(spawner + 0x38C)->count == 0 && reinterpret_cast<FString*>(spawner + 0x398)->count == 0)
        {
            return SpawnAtAttractionPoint(spawner, static_cast<uint32_t*>(a2), a3);
        }

        auto request = *reinterpret_cast<uint8_t**>(spawner + 0x520);
        static FString empty{};
        auto& name = request ? *reinterpret_cast<FString*>(request + 0x10) : empty;
        if (request && name.count > 1 && name.data && *name.data)
        {
            auto& groups = *reinterpret_cast<FString*>(spawner + (a3 ? 0x38C : 0x398)); // TArray<group*>: data, count, max
            auto list = reinterpret_cast<uint8_t* const*>(groups.data);
            bool found = false;
            for (int32_t i = 0; i < groups.count && !found; i++)
            {
                auto& groupName = *reinterpret_cast<FString*>(list[i] + 0x368);
                found = list[i] && groupName.count > 1 && groupName.data && _stricmp(groupName.data, name.data) == 0;
            }
            if (!found)
                return SpawnFromAnyGroup(spawner, edx, a2, a3);
        }
        return shSpawnFromGroup.fastcall<int32_t>(spawner, edx, a2, a3);
    }

    bool IsCharlieMap(uint8_t* level)
    {
        // ULevel +5Ch: URL map
        auto& map = *reinterpret_cast<FString*>(level + 0x5C);
        if (map.count <= 1 || !map.data)
            return true;
        std::string_view mapName(map.data);
        for (std::string_view name : { "D_Amman", "D_Bratislava", "D_Kigali", "D_Sanaa" })
            if (std::search(mapName.begin(), mapName.end(), name.begin(), name.end(), [](char a, char b) { return ::tolower(a) == ::tolower(b); }) != mapName.end())
                return true;
        return false;
    }

    // the object's class is at +24h
    void(__cdecl* HunterConstructor)(void*) = nullptr;
    void(__cdecl* ExtractionConstructor)(void*) = nullptr;

    // A Charlie spawner for a map that has no co-op spawner (Grim's maps), made when the first wave starts and handed out as the
    // black box's spawner (AEchelonLevelInfo +12F0h: FBlackBoxInfo { id, spawner })
    void** pAttractionPointClass = nullptr;
    uint8_t* createdSpawner = nullptr;
    uint8_t* createdSpawnerLevelInfo = nullptr;
    int32_t createdSpawnerInfo[2]{};

    SafetyHookInline shGetBlackBoxInfo{};
    int32_t* __fastcall GetBlackBoxInfo(uint8_t* levelInfo, void* edx, int32_t blackBox)
    {
        auto info = shGetBlackBoxInfo.thiscall<int32_t*>(levelInfo, blackBox);
        if (createdSpawner && levelInfo == createdSpawnerLevelInfo && (!info || !info[1]))
        {
            createdSpawnerInfo[0] = blackBox;
            createdSpawnerInfo[1] = reinterpret_cast<int32_t>(createdSpawner);
            return createdSpawnerInfo;
        }
        return info;
    }

    void CreateSpawner(uint8_t* matchManager)
    {
        auto levelInfo = *reinterpret_cast<uint8_t**>(matchManager + 0x318);
        auto level = *reinterpret_cast<uint8_t**>(matchManager + 0x148);
        if (!levelInfo || !level || !pExtractionSpawnerClass || !*pExtractionSpawnerClass || IsCharlieMap(level))
            return;
        if (createdSpawner && createdSpawnerLevelInfo == levelInfo)
            return;

        auto& list = *reinterpret_cast<FString*>(levelInfo + 0x12F0);
        auto entries = reinterpret_cast<const int32_t*>(list.data);
        for (int32_t i = 0; i < list.count; i++)
            if (entries[i * 2 + 1])
                return; // the map has its own spawner

        // ULevel::SpawnActor(class, name, location, rotation, template, bNoCollisionFail, ...)
        using SpawnActorFn = uint8_t*(__thiscall*)(uint8_t*, void*, uint32_t, float, float, float, int32_t, int32_t, int32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
        auto SpawnActor = reinterpret_cast<SpawnActorFn>((*reinterpret_cast<void***>(level))[0x98 / 4]);
        auto spawner = SpawnActor(level, *pExtractionSpawnerClass, 0, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0);
        if (!spawner)
            return;
        *reinterpret_cast<uint32_t*>(spawner + 0x330) |= 1; // enabled
        createdSpawner = spawner;
        createdSpawnerLevelInfo = levelInfo;
    }

    // a random attraction point (AI position) away from the players
    bool SpawnAtAttractionPoint(uint8_t* spawner, uint32_t* spawningInfo, int32_t initial)
    {
        auto level = *reinterpret_cast<uint8_t**>(spawner + 0x148);
        auto game = *reinterpret_cast<uint8_t**>(spawner + 0x144);
        if (!level || !game || !spawningInfo || !pAttractionPointClass || !*pAttractionPointClass)
            return false;

        std::vector<const float*> players;
        auto& controllers = *reinterpret_cast<FString*>(game + 0x36C);
        for (int32_t i = 0; i < controllers.count; i++)
        {
            auto controller = reinterpret_cast<uint8_t* const*>(controllers.data)[i];
            auto pawn = controller ? *reinterpret_cast<uint8_t**>(controller + 0x520) : nullptr;
            if (pawn)
                players.push_back(reinterpret_cast<const float*>(pawn + 0x9C));
        }

        std::vector<uint8_t*> nearPoints, farPoints;
        auto& actors = *reinterpret_cast<FString*>(level + 0x28);
        for (int32_t i = 0; i < actors.count; i++)
        {
            auto actor = reinterpret_cast<uint8_t* const*>(actors.data)[i];
            if (!actor || !IsA(actor, *pAttractionPointClass))
                continue;
            auto location = reinterpret_cast<const float*>(actor + 0x9C);
            float closest = FLT_MAX;
            for (auto p : players)
                closest = std::min(closest, std::hypot(location[0] - p[0], location[1] - p[1], location[2] - p[2]));
            if (closest > 2500.0f)
                farPoints.push_back(actor);
            else if (closest > 800.0f)
                nearPoints.push_back(actor);
        }

        auto& points = farPoints.empty() ? nearPoints : farPoints;
        if (points.empty())
            return false;

        // FSpawningInfo { bit 31: initial, attraction point, spawn group }
        spawningInfo[0] = (spawningInfo[0] & 0x7FFFFFFF) | (initial ? 0x80000000 : 0);
        spawningInfo[1] = reinterpret_cast<uint32_t>(points[GetRandomInt(0, int(points.size()) - 1)]);
        spawningInfo[2] = 0;
        return true;
    }

    void OnGameModeSet(int gameMode)
    {
        createdSpawner = nullptr;
        createdSpawnerLevelInfo = nullptr;
    }

    void __cdecl ClassConstructor(void* object)
    {
        if (object && *reinterpret_cast<void**>(static_cast<uint8_t*>(object) + 0x24) == *pExtractionSpawnerClass)
            return ExtractionConstructor(object);
        return HunterConstructor(object);
    }
}

// Charlie missions on the SMI for the maps of Kobin and Grim. They're added to SC6MissionData.xml when the game reads it, as copies of
// those missions in a category of their own (not counted with the Deniable Ops missions), and are purely cosmetic: not saved in the
// profile, not counted in the SMI stat bars or for mastery.
namespace CharlieMissions
{
    constexpr int32_t ContextIDBase = 1000; // contextID of a copy = 1000 + the original's contextID

    struct FArchive
    {
        uintptr_t* vtable() { return *reinterpret_cast<uintptr_t**>(this); }
        void Release() { reinterpret_cast<void(__thiscall*)(FArchive*, int)>(vtable()[0])(this, 1); }
        void Serialize(void* data, int32_t size) { reinterpret_cast<void(__thiscall*)(FArchive*, void*, int32_t)>(vtable()[0x0C / 4])(this, data, size); }
        int32_t TotalSize() { return reinterpret_cast<int32_t(__thiscall*)(FArchive*)>(vtable()[0x48 / 4])(this); }
    };

    // An FArchive reading from memory, with the virtuals CFArchiveStream (the stream the XML parser reads) uses: the destructor (0),
    // Serialize (0Ch), Tell (44h), TotalSize (48h), Seek (50h) and IsError (64h). The game deletes the reader after parsing, this one is static.
    struct MemoryArchive
    {
        void** vtable;
        std::string data;
        size_t position;

        static void* __fastcall Destructor(MemoryArchive* archive, void*, int32_t) { return archive; }
        static void __fastcall Serialize(MemoryArchive* archive, void*, void* out, int32_t size)
        {
            auto count = std::min<size_t>(std::max(size, 0), archive->data.size() - std::min(archive->position, archive->data.size()));
            memcpy(out, archive->data.data() + archive->position, count);
            archive->position += count;
        }
        static int32_t __fastcall Tell(MemoryArchive* archive, void*) { return int32_t(archive->position); }
        static int32_t __fastcall TotalSize(MemoryArchive* archive, void*) { return int32_t(archive->data.size()); }
        static void __fastcall Seek(MemoryArchive* archive, void*, int32_t position) { archive->position = std::clamp<size_t>(position, 0, archive->data.size()); }
        static int32_t __fastcall IsError(MemoryArchive*, void*) { return 0; }
        static int32_t __fastcall Unused(MemoryArchive*, void*) { return 0; }

        MemoryArchive(std::string&& text) : data(std::move(text)), position(0)
        {
            static void* table[32];
            std::fill(std::begin(table), std::end(table), reinterpret_cast<void*>(&Unused));
            table[0x00 / 4] = reinterpret_cast<void*>(&Destructor);
            table[0x0C / 4] = reinterpret_cast<void*>(&Serialize);
            table[0x44 / 4] = reinterpret_cast<void*>(&Tell);
            table[0x48 / 4] = reinterpret_cast<void*>(&TotalSize);
            table[0x50 / 4] = reinterpret_cast<void*>(&Seek);
            table[0x64 / 4] = reinterpret_cast<void*>(&IsError);
            vtable = table;
        }
    };

    // the value of name="..." in a tag
    std::string Attribute(std::string_view tag, std::string_view name)
    {
        auto key = " " + std::string(name) + "=\"";
        auto start = tag.find(key);
        if (start == std::string_view::npos)
            return {};
        start += key.size();
        auto end = tag.find('"', start);
        return end == std::string_view::npos ? std::string() : std::string(tag.substr(start, end - start));
    }

    // the value of <string key="..." value="..." /> in a mission
    std::string StringItem(std::string_view mission, std::string_view key)
    {
        auto start = mission.find("<string key=\"" + std::string(key) + "\"");
        if (start == std::string_view::npos)
            return {};
        return Attribute(mission.substr(start, mission.find('>', start) - start), "value");
    }

    std::unordered_map<int32_t, uint32_t> copyMapAssets; // contextID of a copy -> NetOnlineMapId of its map

    std::string AddMissions(const std::string& xml)
    {
        // <mission id="..." ...> ... </mission>
        std::vector<std::string_view> blocks;
        for (size_t start = xml.find("<mission "); start != std::string::npos; start = xml.find("<mission ", start + 1))
        {
            auto end = xml.find("</mission>", start);
            if (end == std::string::npos)
                break;
            blocks.push_back(std::string_view(xml).substr(start, end + 10 - start));
        }

        std::string_view charlie;
        for (auto block : blocks)
        {
            if (Attribute(block.substr(0, block.find('>')), "id") == "E01")
                charlie = block;
        }
        if (charlie.empty())
            return xml;

        std::string missions;
        for (auto block : blocks)
        {
            auto openTag = block.substr(0, block.find('>') + 1);
            auto id = Attribute(openTag, "id");
            if (id.size() != 3 || (id[0] != 'H' && id[0] != 'G') || !std::isdigit(uint8_t(id[1])) || !std::isdigit(uint8_t(id[2])))
                continue;

            auto contextID = ContextIDBase + std::atoi(Attribute(openTag, "contextID").c_str());
            missions += std::format("\t<mission id=\"FFC_{}\" name=\"{}\" itemType=\"5\" initialState=\"1\" contextID=\"{}\">", id, Attribute(openTag, "name"), contextID);
            copyMapAssets[contextID] = std::strtoul(StringItem(block, "NetOnlineMapId").c_str(), nullptr, 16);

            auto body = block.substr(openTag.size());
            for (size_t pos = 0; pos < body.size();)
            {
                auto next = body.find('\n', pos);
                auto line = std::string(body.substr(pos, next == std::string_view::npos ? std::string_view::npos : next - pos));
                pos = next == std::string_view::npos ? body.size() : next + 1;

                if (line.find("<unlocks") != std::string::npos || line.find("<locks") != std::string::npos || line.find("<condition") != std::string::npos)
                    continue;

                // next to the original on the map
                if (auto smi = line.find("<smi "); smi != std::string::npos)
                {
                    auto x = std::atof(Attribute(line, "locx").c_str()) + 0.012;
                    auto y = std::atof(Attribute(line, "locy").c_str()) + 0.012;
                    line = line.substr(0, smi) + std::format("<smi locx=\"{:.7f}\" locy=\"{:.7f}\" />", x, y);
                }

                // Charlie's briefing and sequences
                for (auto key : { "BriefingSequence", "AcceptSequence", "CancelSequence", "PostBriefingSequence", "MsnSound" })
                {
                    if (auto item = line.find(std::string("<string key=\"") + key + "\""); item != std::string::npos)
                        line = line.substr(0, item) + std::format("<string key=\"{}\" value=\"{}\" />", key, StringItem(charlie, key));
                }

                // the title
                if (auto translation = line.find("</Translation>"); translation != std::string::npos)
                    line.insert(translation, " - Charlie");

                missions += line + "\n";
            }
            if (!missions.ends_with("\n"))
                missions += "\n";
        }

        auto root = xml.rfind("</root>");
        if (missions.empty() || root == std::string::npos)
            return xml;
        return xml.substr(0, root) + "  <category name=\"FusionFix\">\n" + missions + "  </category>\n" + xml.substr(root);
    }

    // SC6MissionSystem: TMap<FName, SC6Mission> at +4 (pairs +4, count +8, 236 bytes each: hash next, key, mission)
    uint8_t* missionSystem = nullptr;

    template<typename F>
    void ForEachMission(F&& f)
    {
        if (!missionSystem)
            return;
        auto pairs = *reinterpret_cast<uint8_t**>(missionSystem + 4);
        auto count = *reinterpret_cast<int32_t*>(missionSystem + 8);
        for (int32_t i = 0; i < count; i++)
        {
            if (*reinterpret_cast<int32_t*>(pairs + 236 * i) > -2)
                f(pairs + 236 * i + 8);
        }
    }

    // SC6Mission: +152 itemType, +164 contextID, +180 status, +205 mastered difficulties
    bool IsCopy(uint8_t* mission)
    {
        return mission && *reinterpret_cast<int32_t*>(mission + 164) >= ContextIDBase;
    }

    uint8_t* launchedMission = nullptr;

    bool IsCopyOf(uint8_t* copy, uint8_t* mission)
    {
        return IsCopy(copy) && mission && !IsCopy(mission) && *reinterpret_cast<int32_t*>(copy + 164) == ContextIDBase + *reinterpret_cast<int32_t*>(mission + 164);
    }

    // The starting wave can be picked on all waves (SC6Mission +184: the last completed extraction wave, +185: the selected starting wave), the
    // SMI offers it on completed missions (+180 status 3). The briefing and the completion are marked as seen (+188, +192), the SMI would show
    // the completed animation every time otherwise.
    void UnlockWaves(uint8_t* mission)
    {
        mission[184] = std::max<uint8_t>(mission[184], 20);
        mission[185] = std::max<uint8_t>(mission[185], 1);
        auto& status = *reinterpret_cast<int32_t*>(mission + 180);
        status = std::max(status, 3);
        *reinterpret_cast<int32_t*>(mission + 188) = 1;
        *reinterpret_cast<int32_t*>(mission + 192) = 1;
    }

    // The map list of the lobby only has maps made for the mode (NetOnlineMapAsset +38h: game mode flags, Extraction 4); the map of a launched
    // copy is added for Extraction, so the lobby finds the map (its image, the starting waves). Assets: data manager +34h -> +5Ch count, +64h items.
    uint8_t** pNetOnlineManager = nullptr;
    uint8_t* patchedMapAsset = nullptr;
    uint32_t patchedMapAssetModes = 0;

    void SetMapForExtraction(uint8_t* mission)
    {
        if (patchedMapAsset)
        {
            *reinterpret_cast<uint32_t*>(patchedMapAsset + 0x38) = patchedMapAssetModes;
            patchedMapAsset = nullptr;
        }

        auto it = IsCopy(mission) ? copyMapAssets.find(*reinterpret_cast<int32_t*>(mission + 164)) : copyMapAssets.end();
        if (it == copyMapAssets.end() || !pNetOnlineManager || !*pNetOnlineManager)
            return;
        auto dataManager = *reinterpret_cast<uint8_t**>(*pNetOnlineManager + 8);
        auto maps = dataManager ? *reinterpret_cast<uint8_t**>(dataManager + 0x34) : nullptr;
        if (!maps)
            return;
        auto count = *reinterpret_cast<uint32_t*>(maps + 0x5C);
        auto assets = *reinterpret_cast<uint8_t***>(maps + 0x64);
        for (uint32_t i = 0; i < count; i++)
        {
            if (assets[i] && *reinterpret_cast<uint32_t*>(assets[i]) == it->second)
            {
                patchedMapAsset = assets[i];
                patchedMapAssetModes = *reinterpret_cast<uint32_t*>(patchedMapAsset + 0x38);
                *reinterpret_cast<uint32_t*>(patchedMapAsset + 0x38) |= 4;
                return;
            }
        }
    }

    // the lobby finds the mission by the map asset, which finds the original
    SafetyHookInline shGetMissionInfoFromAssetId{};
    uint8_t* __fastcall GetMissionInfoFromAssetId(uint8_t* system, void* edx, void* assetId)
    {
        auto mission = shGetMissionInfoFromAssetId.thiscall<uint8_t*>(system, assetId);
        if (IsCopyOf(launchedMission, mission))
        {
            UnlockWaves(launchedMission);
            return launchedMission;
        }
        return mission;
    }

    // the current mission is found by the map, which finds the original
    SafetyHookInline shGetMissionInfoFromMap{};
    uint8_t* __fastcall GetMissionInfoFromMap(uint8_t* system, void* edx, void* map)
    {
        auto mission = shGetMissionInfoFromMap.thiscall<uint8_t*>(system, map);
        return IsCopyOf(launchedMission, mission) ? launchedMission : mission;
    }

    SafetyHookInline shGetStaticValues{};
    int32_t __fastcall GetStaticValues(int32_t* screen, void* edx)
    {
        auto result = shGetStaticValues.thiscall<int32_t>(screen);
        ForEachMission([&](uint8_t* mission)
        {
            if (!IsCopy(mission))
                return;
            screen[223]--; // Charlie
            screen[215]--; // co-op
            if (*reinterpret_cast<int32_t*>(mission + 180) >= 3)
            {
                screen[224]--;
                screen[216]--;
            }
        });
        return result;
    }

    SafetyHookInline shGetNumMissionsMastered{};
    int32_t __fastcall GetNumMissionsMastered(void* system, void* edx, void* a2, void* a3, void* a4, void* a5, int32_t a6, int32_t a7)
    {
        std::vector<std::pair<uint8_t*, std::array<uint8_t, 3>>> saved;
        ForEachMission([&](uint8_t* mission)
        {
            if (!IsCopy(mission))
                return;
            saved.push_back({ mission, { mission[205], mission[206], mission[207] } });
            mission[205] = mission[206] = mission[207] = 0xFF;
        });
        auto result = shGetNumMissionsMastered.thiscall<int32_t>(system, a2, a3, a4, a5, a6, a7);
        for (auto& [mission, mastered] : saved)
            std::copy(mastered.begin(), mastered.end(), mission + 205);
        return result;
    }

    uintptr_t returnFromSerialize = 0; // a retn 4

    // Scoring: the copies use the score settings of their original (Score_Map.xml of the map, by mission id), not read again
    // (a linear archive streams each file once)
    bool IsCopyId(const char* id)
    {
        return id && std::string_view(id).starts_with("FFC_");
    }

    struct TString { const char* data; int32_t count; int32_t max; };

    SafetyHookInline shLoadMapFromXml{};
    int32_t __fastcall LoadMapFromXml(void* scoring, void* edx, TString* id, TString* map)
    {
        if (id && id->count > 0 && IsCopyId(id->data))
            return 1;
        return shLoadMapFromXml.thiscall<int32_t>(scoring, id, map);
    }

    SafetyHookInline shGetMapScoringInfoIndex{};
    int32_t __fastcall GetMapScoringInfoIndex(void* scoring, void* edx, const char* id)
    {
        return shGetMapScoringInfoIndex.thiscall<int32_t>(scoring, IsCopyId(id) ? id + 4 : id);
    }
}

namespace UI
{
    SafetyHookInline shGetIsDifficultyLevelPerfectionist{};
    bool GetIsDifficultyLevelPerfectionist()
    {
        return shGetIsDifficultyLevelPerfectionist.ccall<bool>();
    }

    bool GetIsDifficultyLevelPerfectionistHook()
    {
        return false;
    }
}

namespace FThermalSonarVisionComponent
{
    SafetyHookInline shSetCurrentActiveSonarWaveRange{};
    void __fastcall SetCurrentActiveSonarWaveRange(void* _this, void* edx, float range)
    {
        if (UI::GetIsDifficultyLevelPerfectionist())
            range /= 1.5f;
        return shSetCurrentActiveSonarWaveRange.fastcall(_this, edx, range);
    }
}

export void InitMissions()
{
    CIniReader iniReader("");
    auto bDisableNightVisionFlash = iniReader.ReadInteger("MAIN", "DisableNightVisionFlash", 1) != 0;
    auto bDisablePerfectionistChecks = iniReader.ReadInteger("MAIN", "DisablePerfectionistChecks", 1) != 0;
    auto nDefaultMissionFilter = std::clamp(iniReader.ReadInteger("MAIN", "DefaultMissionFilter", 1), 0, 3);
    auto bSMIMapDisableStartupAnimation = iniReader.ReadInteger("MAIN", "SMIMapDisableStartupAnimation", 1) != 0;

    sExtractionWaveConfigs = iniReader.ReadString("EXTRACTION", "ExtractionWaveConfigs", "Default");
    nExtractionWaveEnemyMultiplier = std::clamp(iniReader.ReadInteger("EXTRACTION", "ExtractionWaveEnemyMultiplier", 1), 1, 9999);
    nExtractionWaveEnemyRandomRangeMin = std::clamp(iniReader.ReadInteger("EXTRACTION", "ExtractionWaveEnemyRandomRangeMin", 0), 0, 9999);
    nExtractionWaveEnemyRandomRangeMax = std::clamp(iniReader.ReadInteger("EXTRACTION", "ExtractionWaveEnemyRandomRangeMax", 4), 1, 9999);
    auto bEnableKobinAndGrimMaps = iniReader.ReadInteger("EXTRACTION", "EnableKobinAndGrimMaps", 0) != 0;

    static auto sHUNTERReinforcementsNumber = iniReader.ReadString("HUNTER", "ReinforcementsNumber", "Default");
    static auto nHUNTERReinforcementsEnemyMultiplier = std::clamp(iniReader.ReadInteger("HUNTER", "ReinforcementsEnemyMultiplier", 1), 1, 9999);
    static auto nHUNTERReinforcementsEnemyRandomRangeMin = std::clamp(iniReader.ReadInteger("HUNTER", "ReinforcementsEnemyRandomRangeMin", 1), 1, 9999);
    static auto nHUNTERReinforcementsEnemyRandomRangeMax = std::clamp(iniReader.ReadInteger("HUNTER", "ReinforcementsEnemyRandomRangeMax", 1), 1, 9999);

    static auto bGHOSTDisableMissionFailOnDetection = iniReader.ReadInteger("GHOST", "DisableMissionFailOnDetection", 1) != 0;

    static auto bCOOPDisableMissionFailOnDetection = iniReader.ReadInteger("COOP", "DisableMissionFailOnDetection", 0) != 0;
    static auto sCOOPReinforcementsNumber = iniReader.ReadString("COOP", "ReinforcementsNumber", "Default");
    static auto nCOOPReinforcementsEnemyMultiplier = std::clamp(iniReader.ReadInteger("COOP", "ReinforcementsEnemyMultiplier", 1), 1, 9999);
    static auto nCOOPReinforcementsEnemyRandomRangeMin = std::clamp(iniReader.ReadInteger("COOP", "ReinforcementsEnemyRandomRangeMin", 1), 1, 9999);
    static auto nCOOPReinforcementsEnemyRandomRangeMax = std::clamp(iniReader.ReadInteger("COOP", "ReinforcementsEnemyRandomRangeMax", 1), 1, 9999);

    static auto bCAMPAIGNDisableMissionFailOnDetection = iniReader.ReadInteger("CAMPAIGN", "DisableMissionFailOnDetection", 0) != 0;
    auto bEnableRunDuringForcedWalk = iniReader.ReadInteger("CAMPAIGN", "EnableRunDuringForcedWalk", 1) != 0;

    // GameMode
    auto pattern = hook::pattern("55 8B EC 83 3D ? ? ? ? ? 75 55");
    shLead_SetCurrentGameMode = safetyhook::create_inline(pattern.get_first(), Lead_SetCurrentGameMode);

    // Disable perfectionist checks
    pattern = hook::pattern("53 32 DB E8 ? ? ? ? 85 C0 74 29");
    UI::shGetIsDifficultyLevelPerfectionist = safetyhook::create_inline(pattern.get_first(), bDisablePerfectionistChecks ? UI::GetIsDifficultyLevelPerfectionistHook : UI::GetIsDifficultyLevelPerfectionist);

    pattern = hook::pattern("E8 ? ? ? ? F3 0F 10 46 ? 51 8B CE");
    FThermalSonarVisionComponent::shSetCurrentActiveSonarWaveRange = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), FThermalSonarVisionComponent::SetCurrentActiveSonarWaveRange);

    {
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 04 89 45 E4 85 C0 0F 84"); // quantity in ExtractionWaveConfigXMLParser::LoadSubWaves
        ExtractionSubWaveEnemy::hbappAtoi.fun = injector::MakeCALL(pattern.get_first(0), ExtractionSubWaveEnemy::appAtoi).get();

        pattern = hook::pattern("E8 ? ? ? ? 83 C4 0C 85 C0 75 0C 8B 45 E8"); // ExtractionWaveConfigXMLParser::LoadSubWaves
        static auto KillCountSoftlockFix = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
        {
            auto type = std::string_view((const char*)regs.esi);
            int value = regs.edi;
            if (type == "Kills")
            {
                if (ExtractionSubWaveEnemy::curWaveEnemyCount < value)
                {
                    value = ExtractionSubWaveEnemy::curWaveEnemyCount;
                    regs.edi = value;
                    regs.eax = value;
                }

                ExtractionSubWaveEnemy::curStartConditionType = value;
                ExtractionSubWaveEnemy::curWaveEnemyCount = 0;
            }
            else
            {
                ExtractionSubWaveEnemy::curStartConditionType = 0;
                ExtractionSubWaveEnemy::curWaveEnemyCount = 0;
            }
        });
    }

    if (bEnableKobinAndGrimMaps)
    {
        // UClass storage of the spawner classes, InitializePrivateStaticClass: push size, push &class (the name is pushed before)
        for (auto [size, out, className] : { std::tuple{ "68 18 05 00 00 68 ? ? ? ? E8", &ExtractionOnOtherMaps::pHunterSpawnerClass, "AECoopHunterSpawner" },
                                             std::tuple{ "68 48 05 00 00 68 ? ? ? ? E8", &ExtractionOnOtherMaps::pExtractionSpawnerClass, "AECoopExtractionSpawner" },
                                             std::tuple{ "68 38 04 00 00 68 ? ? ? ? E8", &ExtractionOnOtherMaps::pAttractionPointClass, "AEAIAttractionPoint" } })
        {
            hook::pattern(size).for_each_result([out, className](hook::pattern_match match)
            {
                auto name = *match.get<const char*>(-4);
                if (*match.get<uint8_t>(-5) == 0x68 && !IsBadReadPtr(name, 1) && strcmp(name, className) == 0)
                    *out = *match.get<void**>(6);
            });
        }

        // UObject::StaticAllocateObject(class, ...), before it reads the class
        pattern = hook::pattern("8B 7D 08 85 FF 75 ? 8B 45 10 8B C8 C1 E9 13 51 8B 0D");
        if (!pattern.empty() && ExtractionOnOtherMaps::pHunterSpawnerClass && ExtractionOnOtherMaps::pExtractionSpawnerClass)
        {
            static auto AllocateObject = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto& objectClass = *reinterpret_cast<void**>(regs.ebp + 8);
                if (CurrentGameMode == EXTRACTION && objectClass && objectClass == *ExtractionOnOtherMaps::pHunterSpawnerClass)
                {
                    // the caller then runs the constructor of the class it asked for (UClass +140h), redirected by the object's class
                    using namespace ExtractionOnOtherMaps;
                    auto& constructor = *reinterpret_cast<void(__cdecl**)(void*)>(static_cast<uint8_t*>(objectClass) + 0x140);
                    if (constructor != ClassConstructor)
                    {
                        HunterConstructor = constructor;
                        ExtractionConstructor = *reinterpret_cast<void(__cdecl**)(void*)>(static_cast<uint8_t*>(*pExtractionSpawnerClass) + 0x140);
                        constructor = ClassConstructor;
                    }
                    objectClass = *pExtractionSpawnerClass;
                }
            });
        }

        pattern = hook::pattern("55 8B EC 83 EC 08 57 8B F9 8B 87 20 05 00 00 83 78 14 00 74 05 8B 40 10 EB 05 B8 ? ? ? ? 50 E8 ? ? ? ? 83 C4 04 85 C0 7F 16 8B 45 0C 8B 4D 08 50 51 8B CF E8");
        if (!pattern.empty())
        {
            ExtractionOnOtherMaps::SpawnFromAnyGroup = reinterpret_cast<decltype(ExtractionOnOtherMaps::SpawnFromAnyGroup)>(injector::GetBranchDestination(pattern.get_first(0x36)).as_int());
            ExtractionOnOtherMaps::shSpawnFromGroup = safetyhook::create_inline(pattern.get_first(), ExtractionOnOtherMaps::SpawnFromGroup);
        }

        // AEchelonLevelInfo::GetBlackBoxInfo
        pattern = hook::pattern("55 8B EC 8B 91 F4 12 00 00 56 33 C0 57 85 D2 7E ? 8B B1 F0 12 00 00 8B 7D 08 8B CE");
        if (!pattern.empty())
            ExtractionOnOtherMaps::shGetBlackBoxInfo = safetyhook::create_inline(pattern.get_first(), ExtractionOnOtherMaps::GetBlackBoxInfo);

        // AEExtractionMatchManager::StartWave
        pattern = hook::pattern("8B F1 80 BE A8 04 00 00 04 0F 84 ? ? ? ? E8 ? ? ? ? 0F 57 C0");
        if (!pattern.empty())
        {
            static auto StartWave = safetyhook::create_mid(pattern.get_first(2), [](SafetyHookContext& regs)
            {
                if (CurrentGameMode == EXTRACTION)
                    ExtractionOnOtherMaps::CreateSpawner(reinterpret_cast<uint8_t*>(regs.esi));
            });
        }

        // UESEQJobCoopObjectiveAddLocator::Start: the other mode's objective markers (Grim's A, B, C) are not added, as if there was no
        // co-op match manager
        pattern = hook::pattern("8B F1 E8 ? ? ? ? 8B F8 85 FF 0F 84 ? ? ? ? 8B 46 58 6A 00 6A 01");
        if (!pattern.empty())
        {
            static auto CoopObjectiveAddLocator = safetyhook::create_mid(pattern.get_first(7), [](SafetyHookContext& regs)
            {
                auto level = regs.eax ? *reinterpret_cast<uint8_t**>(regs.eax + 0x148) : nullptr;
                if (CurrentGameMode == EXTRACTION && level && !ExtractionOnOtherMaps::IsCharlieMap(level))
                    regs.eax = 0;
            });
        }
    }

    {
        // AECoopHunterSpawner
        pattern = hook::pattern("85 5E 08 75 29 8B 06 8B 50 0C 6A 04 8D 8F ? ? ? ? 51 8B CE FF D2 85 5E 08 75 12 8B 06 8B 50 0C 6A 04 8D 8F ? ? ? ? 51 8B CE FF D2 F6 46 08 02");
        static auto FCheckpointPackReaderHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (CurrentGameMode == HUNTER)
            {
                if (iequals(sHUNTERReinforcementsNumber, "Random"))
                    *(uint32_t*)(regs.edi + 0x370) = GetRandomInt(nHUNTERReinforcementsEnemyRandomRangeMin, nHUNTERReinforcementsEnemyRandomRangeMax);
            }
            else if (CurrentGameMode == COOP)
            {
                if (iequals(sCOOPReinforcementsNumber, "Random"))
                    *(uint32_t*)(regs.edi + 0x370) = GetRandomInt(nCOOPReinforcementsEnemyRandomRangeMin, nCOOPReinforcementsEnemyRandomRangeMax);
            }
        });

        pattern = hook::pattern("8B 16 8B 82 ? ? ? ? 8B CE FF D0 8B 16 8B 82 ? ? ? ? 8B CE FF D0 C6 86");
        static auto AECoopHunterSpawnerHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (CurrentGameMode == HUNTER)
            {
                if (iequals(sHUNTERReinforcementsNumber, "Random"))
                    *(uint32_t*)(regs.esi + 0x370) = GetRandomInt(nHUNTERReinforcementsEnemyRandomRangeMin, nHUNTERReinforcementsEnemyRandomRangeMax);
                else
                    *(uint32_t*)(regs.esi + 0x370) *= nHUNTERReinforcementsEnemyMultiplier;
            }
            else if (CurrentGameMode == COOP)
            {
                if (iequals(sCOOPReinforcementsNumber, "Random"))
                    *(uint32_t*)(regs.esi + 0x370) = GetRandomInt(nCOOPReinforcementsEnemyRandomRangeMin, nCOOPReinforcementsEnemyRandomRangeMax);
                else
                    *(uint32_t*)(regs.esi + 0x370) *= nCOOPReinforcementsEnemyMultiplier;
            }
        });
    }

    {
        pattern = hook::pattern("F6 86 ? ? ? ? ? 0F 84 ? ? ? ? 8B 96 ? ? ? ? 52");
        static auto loc_F1D09C = (uintptr_t)pattern.get_first(0);

        pattern = hook::pattern("0F 86 ? ? ? ? 0F B6 8E");
        struct AECooperativeMatchManager__TickSpecial
        {
            void operator()(injector::reg_pack& regs)
            {
                if ((bGHOSTDisableMissionFailOnDetection && CurrentGameMode == GHOST) ||
                    (bCOOPDisableMissionFailOnDetection && CurrentGameMode == COOP) ||
                    (bCAMPAIGNDisableMissionFailOnDetection && CurrentGameMode == CAMPAIGN))
                    *(uintptr_t*)(regs.esp - 4) = loc_F1D09C;
            }
        }; injector::MakeInline<AECooperativeMatchManager__TickSpecial>(pattern.get_first(0), pattern.get_first(6));
    }

    if (!sExtractionWaveConfigs.empty() && !iequals(sExtractionWaveConfigs, "Default"))
    {
        std::error_code ec;
        std::string prefix = GetOverloadedFilePathA ? "..\\..\\" : ".\\update\\";
        std::filesystem::path xmlPathDefault = prefix + std::string("Data\\ExtractionWaveConfigs\\") + sExtractionWaveConfigs + "\\DefaultWaveConfig.xml";
        std::filesystem::path xmlPath = prefix + std::string("Data\\ExtractionWaveConfigs\\") + sExtractionWaveConfigs + "\\%s.xml";
        auto p = GetExeModulePath() / std::filesystem::path(xmlPath).remove_filename();

        if (std::filesystem::exists(p / "D_Amman.xml", ec) && std::filesystem::exists(p / "D_Bratislava.xml", ec) && std::filesystem::exists(p / "D_Kigali.xml", ec) && std::filesystem::exists(p / "D_Sanaa.xml", ec))
        {
            static std::string s = xmlPath.string();
            auto pattern = find_pattern("68 ? ? ? ? E8 ? ? ? ? 83 C4 08 50 8D 4D D8 E8 ? ? ? ? BB");
            injector::WriteMemory(pattern.get_first(1), s.data(), true);
        }

        if (std::filesystem::exists(GetExeModulePath() / xmlPathDefault, ec))
        {
            static std::string s = xmlPathDefault.string();
            auto pattern = find_pattern("68 ? ? ? ? E8 ? ? ? ? 8B F0 83 C4 08 39 75 D8 74 3D 80 3E 00 74 0C 56 E8 ? ? ? ? 83 C4 04 40 EB 02 33 C0 89 45 DC 39 45 E0 7D 10 6A 01 8D 4D D8 89 45 E0 E8 ? ? ? ? 8B 45 DC 85 C0");
            injector::WriteMemory(pattern.get_first(1), s.data(), true);
        }
    }

    // Charlie missions on the other co-op maps
    if (bEnableKobinAndGrimMaps)
    {
        using namespace CharlieMissions;

        // SC6MissionSystem::LoadFromXML, after GFileManager->CreateFileReader("..\..\Data\Config\SC6MissionData.xml", 0, GError, 0)
        hook::pattern("8B 15 ? ? ? ? 8B 0D ? ? ? ? 8B 01 8B 40 10 57 52 57 68 ? ? ? ? FF D0").for_each_result([](hook::pattern_match match)
        {
            auto path = *match.get<const char*>(0x15);
            if (IsBadReadPtr(path, 1) || !std::string_view(path).ends_with("SC6MissionData.xml"))
                return;

            static auto LoadFromXML = safetyhook::create_mid(match.get<void>(0x1B), [](SafetyHookContext& regs)
            {
                missionSystem = *reinterpret_cast<uint8_t**>(regs.ebp - 0x10);

                auto reader = reinterpret_cast<FArchive*>(regs.eax);
                if (!reader)
                    return;
                std::string xml(std::max(reader->TotalSize(), 0), '\0');
                reader->Serialize(xml.data(), int32_t(xml.size()));
                reader->Release();

                // read from memory instead
                static MemoryArchive merged(AddMissions(xml));
                regs.eax = reinterpret_cast<uintptr_t>(&merged);
            });
        });

        // UI::SetupCreationsParams(mission, ...): the mission launched from the SMI
        auto pattern = hook::pattern("8B 81 98 00 00 00 48 83 F8 04 77 ? FF 24 85");
        if (!pattern.empty())
        {
            static auto SetupCreationsParams = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                launchedMission = reinterpret_cast<uint8_t*>(regs.ecx);
                if (IsCopy(launchedMission))
                    UnlockWaves(launchedMission);
                SetMapForExtraction(launchedMission);
            });
        }

        // UI::SMI::ShowSamMakesTheCall: the starting wave of the selected Charlie mission
        pattern = hook::pattern("83 B8 98 00 00 00 05 75 ? 83 B8 B4 00 00 00 03");
        if (!pattern.empty())
        {
            static auto SamMakesTheCall = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (IsCopy(reinterpret_cast<uint8_t*>(regs.eax)))
                    UnlockWaves(reinterpret_cast<uint8_t*>(regs.eax));
            });
        }

        // NetOnlineHelpers::GetMapId: the online manager
        pattern = hook::pattern("A1 ? ? ? ? 8B 40 08 83 C0 34 0F 84 ? ? ? ? 8B 38 89 7D ? 85 FF");
        if (!pattern.empty())
            pNetOnlineManager = *pattern.get_first<uint8_t**>(1);

        // SC6MissionSystem::GetMissionInfoFromAssetId
        pattern = hook::pattern("55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 14 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8D 79 04 33 F6 89 75 E8 8B 47 04 33 D2 89 7D E0");
        if (!pattern.empty())
            shGetMissionInfoFromAssetId = safetyhook::create_inline(pattern.get_first(), GetMissionInfoFromAssetId);

        pattern = hook::pattern("55 8B EC 6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 83 EC 44 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 ? ? ? ? 89 4D F0 68");
        if (!pattern.empty())
            shGetMissionInfoFromMap = safetyhook::create_inline(pattern.get_first(), GetMissionInfoFromMap);

        // UI::SMIHubScreens::GetStaticValues
        pattern = hook::pattern("55 8B EC 6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 83 EC 10 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 ? ? ? ? 8B F1 33 FF 89 7D E8");
        if (!pattern.empty())
            shGetStaticValues = safetyhook::create_inline(pattern.get_first(), GetStaticValues);

        // SC6MissionSystem::GetNumMissionsMastered
        pattern = hook::pattern("55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 10 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B 5D 08 8B 55 0C 8B 75 14 33 C0 89 03 89 02 8B 55 10 89 02");
        if (!pattern.empty())
            shGetNumMissionsMastered = safetyhook::create_inline(pattern.get_first(), GetNumMissionsMastered);

        // UI::BehaviorImageField::SetImage(text, width, height): the images of a mission are named by its id, a copy uses the original's
        for (auto [imagePattern, offset] : { std::pair{ "8B F9 68 ? ? ? ? 8D 4D DC E8 ? ? ? ? 33 DB 89 5D FC 68 ? ? ? ? 8D 4D E8 E8 ? ? ? ? C6 45 FC 01 8B 45 0C 83 F8 FF 74", 0 },
                                             std::pair{ "8B 5D 0C 8B F1 8B FA 68 ? ? ? ? 8D 4D DC E8 ? ? ? ? C7 45 FC 00 00 00 00 68 ? ? ? ? 8D 4D E8 E8 ? ? ? ? C6 45 FC 01 83 FE FF 74", 0 } })
        {
            pattern = hook::pattern(imagePattern);
            if (pattern.empty())
                continue;
            static auto SetImage = safetyhook::create_mid(pattern.get_first(offset), [](SafetyHookContext& regs)
            {
                auto& text = *reinterpret_cast<const char**>(regs.ebp + 8);
                if (!text || !strstr(text, "img://") || !strstr(text, "FFC_"))
                    return;
                static std::string image;
                image = text;
                for (auto pos = image.find("FFC_"); pos != std::string::npos; pos = image.find("FFC_", pos))
                    image.erase(pos, 4);
                text = image.c_str();
            });
            break;
        }

        // CScoringSystem::LoadMapFromXml(mission id, map)
        pattern = hook::pattern("55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 B8 C0 12 00 00 E8");
        if (!pattern.empty())
            shLoadMapFromXml = safetyhook::create_inline(pattern.get_first(), LoadMapFromXml);

        // CScoringSystem::GetMapScoringInfoIndex(mission id)
        pattern = hook::pattern("55 8B EC 53 56 8B F1 33 DB 57 39 9E D4 00 00 00 7E ? 33 FF");
        if (!pattern.empty())
            shGetMapScoringInfoIndex = safetyhook::create_inline(pattern.get_first(), GetMapScoringInfoIndex);

        // SC6Mission::SerializeProfile(archive): the copies are not saved or loaded, they keep their initial state
        auto retn4 = hook::pattern("F7 D8 1B C0 40 5D C2 04 00");
        pattern = hook::pattern("55 8B EC 83 EC 0C 56 8B F1 83 7E 08 00 57 74 05 8B 46 04 EB 05 B8 ? ? ? ? 6A 05 50 8D 4D F4 E8");
        if (!pattern.empty() && !retn4.empty())
        {
            returnFromSerialize = reinterpret_cast<uintptr_t>(retn4.get_first(6));
            static auto SerializeProfile = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (IsCopy(reinterpret_cast<uint8_t*>(regs.ecx)))
                {
                    UnlockWaves(reinterpret_cast<uint8_t*>(regs.ecx));
                    regs.eax = 1;
                    regs.eip = returnFromSerialize;
                }
            });
        }
    }

    // ExtractionWaveConfigXMLParser::LoadFile: a map without its own wave config (the maps of the other co-op modes) gets the 20 waves
    // of a random Charlie map instead of DefaultWaveConfig.xml (waves 1, 3, 4 and 6 only, the mission ends after wave 6)
    pattern = hook::pattern("50 68 ? ? ? ? E8 ? ? ? ? 83 C4 08 50 8D 4D D8 E8 ? ? ? ? BB");
    if (bEnableKobinAndGrimMaps && !pattern.empty())
    {
        static auto WaveConfigMapName = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            static constexpr const char* charlieMaps[] = { "D_Amman", "D_Bratislava", "D_Kigali", "D_Sanaa" };
            auto mapName = reinterpret_cast<const char*>(regs.eax);
            if (!mapName || !*mapName || std::any_of(std::begin(charlieMaps), std::end(charlieMaps), [&](auto name) { return iequals(mapName, name); }))
                return;

            auto folder = std::filesystem::path("update") / "Data" / "ExtractionWaveConfigs";
            if (!sExtractionWaveConfigs.empty() && !iequals(sExtractionWaveConfigs, "Default"))
                folder /= sExtractionWaveConfigs;
            std::error_code ec;
            if (std::filesystem::exists(GetExeModulePath() / folder / (std::string(mapName) + ".xml"), ec))
                return;

            regs.eax = reinterpret_cast<uintptr_t>(charlieMaps[GetRandomInt(0, std::size(charlieMaps) - 1)]);
        });
    }

    if (bEnableRunDuringForcedWalk)
    {
        pattern = hook::pattern("74 18 8B 80 ? ? ? ? 85 C0 74 0E 8B 4E 3C 83 E1 01 51 8B C8 E8");
        injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true); // jz -> jmp
    }

    if (bDisableNightVisionFlash)
    {
        // FThermalSonarVisionComponent::SetActiveVisionMode
        pattern = hook::pattern("D9 46 70 D9 9E");
        injector::MakeNOP(pattern.get_first(), 9, true);
        static auto SetActiveVisionModeHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(float*)(regs.esi + 0x8C) = -0.1f;
        });
    }

    {
        pattern = hook::pattern("B9 ? ? ? ? 89 8E ? ? ? ? F3 0F 7E 05");
        injector::WriteMemory(pattern.get_first(1), nDefaultMissionFilter, true);
    }

    if (bSMIMapDisableStartupAnimation)
    {
        pattern = hook::pattern("0F 8E ? ? ? ? 8D 8E ? ? ? ? E8 ? ? ? ? 80 BE");
        injector::WriteMemory<uint16_t>(pattern.count(2).get(1).get<void>(), 0xE990, true); // jle -> jmp
    }
}
