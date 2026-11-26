#pragma once
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
extern "C" {
#include "../../../external/injector/include/ps2/plugin_settings.h"
}
#include "PS2SettingsBatch.hpp"

extern "C" uint32_t PCSX2FSettingsBatchVersion;

// On older hosts (INI only) settings stay at their defaults.
#define PCSX2F_CFG_SETTINGS() extern "C" { uint32_t PCSX2FSettingsVersion = 0; uint32_t PCSX2FSettingsFormat = 1; uint32_t PCSX2FSettingsBatchVersion = 0; }

#define PCSX2F_CFG_PEER_SETTINGS() extern "C" { uint32_t PCSX2FSettingsPeerVersion = 0; }

// Save states restore the plugin's globals together with the game. The host
// increments this word after loading a state; the plugin then reads its CFG
// again so the current file wins over the restored values.
#define PCSX2F_STATE_LOAD_GENERATION() extern "C" { uint32_t PCSX2FStateLoadGeneration = 0; }
extern "C" uint32_t PCSX2FStateLoadGeneration;

namespace console::ps2settings {
struct Value { const char* section; const char* key; char value[64]; };
inline bool copy(char* destination, const char* source) {
    if (!source || std::strlen(source) >= 64) return false;
    std::strcpy(destination, source);
    return true;
}

inline uint32_t request(PCSX2FIniRequest& data) {
    if (PCSX2FSettingsVersion != 2) return PCSX2F_SETTINGS_UNSUPPORTED;
    return PCSX2F_IniRequest(&data);
}
inline bool supportsBatch()
{
    return PCSX2FSettingsBatchVersion == PCSX2FSettingsBatch::Capability;
}

inline PCSX2FSettingsBatch::Request& batchRequest(uint32_t operation)
{
    // The host only accepts storage inside the module image. Calls are synchronous.
    static PCSX2FSettingsBatch::Request data;
    data.size = sizeof(data);
    data.version = PCSX2FSettingsBatch::Version;
    data.operation = operation;
    data.count = 0;
    data.present[0] = data.present[1] = 0;
    return data;
}

inline bool batchEntry(PCSX2FSettingsBatch::Request& data, const Value& value, bool seed)
{
    auto& entry = data.entries[data.count++];
    return copy(entry.section, value.section) && copy(entry.key, value.key) &&
           (!seed || copy(entry.value, value.value));
}

inline uint32_t requestBatch(PCSX2FSettingsBatch::Request& data)
{
    return PCSX2F_IniRequest(reinterpret_cast<PCSX2FIniRequest*>(&data));
}

inline bool readLegacy(PCSX2FSettingsBatch::Request& batch)
{
    static PCSX2FIniRequest data;
    data.size = sizeof(data);
    data.version = 1;
    data.operation = PCSX2F_SETTINGS_READ;
    data.count = 1;
    for (uint32_t i = 0; i < batch.count; ++i)
    {
        auto& entry = batch.entries[i];
        data.entries[0] = entry;
        const auto status = request(data);
        if (status == PCSX2F_SETTINGS_OK)
        {
            std::memcpy(entry.value, data.entries[0].value, sizeof(entry.value));
            entry.value[sizeof(entry.value) - 1] = 0;
            batch.mark(i);
        }
        else if (status != PCSX2F_SETTINGS_NOT_FOUND)
            return false;
    }
    return true;
}

inline bool readBatch(PCSX2FSettingsBatch::Request& data)
{
    if (!supportsBatch())
        return readLegacy(data);

    const auto status = requestBatch(data);
    if (status == PCSX2F_SETTINGS_OK)
        return true;
    if (status != PCSX2F_SETTINGS_INVALID && status != PCSX2F_SETTINGS_UNSUPPORTED)
        return false;

    // A save state made on a newer injector can restore a capability word
    // that an older injector does not know to reset. Probe the request ABI
    // before falling back, without mistaking a malformed CFG for an old host.
    const auto count = data.count;
    data.count = 0;
    const auto probe = requestBatch(data);
    data.count = count;
    if (probe != PCSX2F_SETTINGS_INVALID && probe != PCSX2F_SETTINGS_UNSUPPORTED)
        return false;
    PCSX2FSettingsBatchVersion = 0;
    return readLegacy(data);
}

// Missing keys retain their supplied defaults. Apply the complete read only
// after the host returns successfully.
template<size_t Count> bool read(Value (&values)[Count], bool* missing = nullptr)
{
    static_assert(Count <= PCSX2FSettingsBatch::MaxEntries);
    if (PCSX2FSettingsVersion != 2)
        return false;

    auto& data = batchRequest(PCSX2FSettingsBatch::Read);
    for (const auto& value : values)
        if (!batchEntry(data, value, false))
            return false;
    if (!readBatch(data))
        return false;
    for (size_t i = 0; i < Count; ++i)
    {
        if (data.has(i))
            std::memcpy(values[i].value, data.entries[i].value, sizeof(values[i].value));
        if (missing)
            missing[i] = !data.has(i);
    }
    return true;
}
// Startup creates a missing CFG when supported. Omitted values in an existing CFG stay at their applied defaults.
template<size_t Count> bool load(Value (&values)[Count], bool* defaultsSaved = nullptr)
{
    if (defaultsSaved)
        *defaultsSaved = false;
    bool missing[Count];
    if (!read(values, missing))
        return false;
    auto& data = batchRequest(PCSX2FSettingsBatch::Seed);
    for (size_t i = 0; i < Count; ++i)
    {
        if (missing[i] && (!supportsBatch() || !batchEntry(data, values[i], true)))
            return true;
    }
    const bool saved = !data.count ||
        requestBatch(data) == PCSX2F_SETTINGS_OK;
    if (defaultsSaved)
        *defaultsSaved = saved;
    return true;
}
// Call once per game frame: runs `refresh` when a save state was loaded since
// the last call. The last-seen value is restored with the state too, so a
// state saved before a pending refresh still refreshes.
struct StateLoads {
    uint32_t seen = 0;
    template<class Refresh> void poll(Refresh refresh) {
        const uint32_t current = *reinterpret_cast<volatile uint32_t*>(&PCSX2FStateLoadGeneration);
        if (current == seen) return;
        refresh();
        seen = current;
    }
};
// A setting another plugin's menu or a state load may change while the game
// runs: read again at most once a second of game time, and after state loads.
template<size_t Count> struct LiveValues {
    Value (&values)[Count];
    StateLoads stateLoads;
    uint32_t lastRead = 0;
    explicit LiveValues(Value (&list)[Count]) : values(list) {}
    // True when the values were read again.
    bool update(uint32_t nowMs) {
        bool changed = false;
        stateLoads.poll([&] { changed = read(values); });
        if (nowMs - lastRead >= 1000 || nowMs < lastRead) {
            lastRead = nowMs;
            changed = read(values) || changed;
        }
        return changed;
    }
};
inline bool boolean(const Value& value, bool fallback) {
    char token[64]{};
    const char* begin = value.value;
    while (*begin == ' ' || *begin == '\t') ++begin;
    size_t length = 0;
    while (*begin && length + 1 < sizeof(token)) {
        if (*begin == ';' && length && (token[length - 1] == ' ' || token[length - 1] == '\t')) break;
        char c = *begin++;
        token[length++] = c >= 'A' && c <= 'Z' ? char(c + ('a' - 'A')) : c;
    }
    while (length && (token[length - 1] == ' ' || token[length - 1] == '\t' || token[length - 1] == '\r')) token[--length] = 0;
    if (!std::strcmp(token, "1") || !std::strcmp(token, "true") || !std::strcmp(token, "on") || !std::strcmp(token, "yes")) return true;
    if (!std::strcmp(token, "0") || !std::strcmp(token, "false") || !std::strcmp(token, "off") || !std::strcmp(token, "no")) return false;
    return fallback;
}
// An int or numeric choice: "3", or a whole number written like a float
// ("3.0"). A fraction or trailing text leaves `out` unchanged.
inline bool integer(const Value& value, long& out) {
    char* end = nullptr;
    const float number = std::strtof(value.value, &end);
    if (end == value.value || !std::isfinite(number) || std::fabs(number) > 1000000.0f || number != std::floor(number)) return false;
    while (*end == ' ' || *end == '\t' || *end == '\r') ++end;
    if (*end) return false;
    out = long(number);
    return true;
}
}
