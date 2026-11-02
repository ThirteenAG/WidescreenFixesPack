#pragma once
#include <cstdint>
#include <cstring>
extern "C" {
#include "../../../external/injector/include/ps2/plugin_settings.h"
}

// On older hosts (INI only) settings stay at their defaults.
#define PCSX2F_CFG_SETTINGS() extern "C" { uint32_t PCSX2FSettingsVersion = 0; uint32_t PCSX2FSettingsFormat = 1; }

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
// Startup only: read individual keys, then persist every described default or
// loaded value in bounded batches. Host writes preserve unrelated file contents.
template<size_t Count> bool load(Value (&values)[Count]) {
    bool saved = PCSX2FSettingsVersion == 2;
    if (!saved) return false;
    // The host accepts request storage in the module image, excluding the game
    // stack and heap. Startup calls are synchronous on the emulation thread.
    static PCSX2FIniRequest data;
    for (auto& value : values) {
        data = {};
        data.size = sizeof(data); data.version = 1; data.operation = PCSX2F_SETTINGS_READ; data.count = 1;
        if (!copy(data.entries[0].section, value.section) || !copy(data.entries[0].key, value.key)) return false;
        auto status = request(data);
        if (status == PCSX2F_SETTINGS_OK) { copy(value.value, data.entries[0].value); continue; }
        if (status != PCSX2F_SETTINGS_NOT_FOUND) return false;
    }
    for (size_t first = 0; first < Count; first += PCSX2F_SETTINGS_MAX_ENTRIES) {
        data = {};
        data.size = sizeof(data); data.version = 1; data.operation = PCSX2F_SETTINGS_WRITE;
        for (size_t i = first; i < Count && data.count < PCSX2F_SETTINGS_MAX_ENTRIES; ++i) {
            auto& entry = data.entries[data.count++];
            if (!copy(entry.section, values[i].section) || !copy(entry.key, values[i].key) || !copy(entry.value, values[i].value)) return false;
        }
        saved = request(data) == PCSX2F_SETTINGS_OK && saved;
    }
    return saved;
}
// Reads the current values without writing anything back (refresh after a
// state load, periodic checks). Missing keys keep their current value.
template<size_t Count> bool read(Value (&values)[Count]) {
    if (PCSX2FSettingsVersion != 2) return false;
    static PCSX2FIniRequest data;
    for (auto& value : values) {
        data = {};
        data.size = sizeof(data); data.version = 1; data.operation = PCSX2F_SETTINGS_READ; data.count = 1;
        if (!copy(data.entries[0].section, value.section) || !copy(data.entries[0].key, value.key)) return false;
        const auto status = request(data);
        if (status == PCSX2F_SETTINGS_OK) { data.entries[0].value[sizeof(data.entries[0].value) - 1] = 0; copy(value.value, data.entries[0].value); }
        else if (status != PCSX2F_SETTINGS_NOT_FOUND) return false;
    }
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
}
