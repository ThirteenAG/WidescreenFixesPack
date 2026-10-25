#pragma once
#include <cstdint>
#include <cstring>
extern "C" {
#include "../../../external/injector/include/ps2/plugin_settings.h"
}

// On older hosts (INI only) settings stay at their defaults.
#define PCSX2F_CFG_SETTINGS() extern "C" { uint32_t PCSX2FSettingsVersion = 0; uint32_t PCSX2FSettingsFormat = 1; }

#define PCSX2F_CFG_PEER_SETTINGS() extern "C" { uint32_t PCSX2FSettingsPeerVersion = 0; }

namespace console::ps2settings {
struct Value { const char* section; const char* key; char value[64]; };
inline bool copy(char* destination, const char* source) {
    if (!source || std::strlen(source) >= 64) return false;
    std::strcpy(destination, source);
    return true;
}

// Static instances keep request storage in the calling module's image.
class RestartBoolPeer {
    const char* module; const char* section; const char* key;
    PCSX2FBoolPeerRequest data{};
    bool invoke(uint32_t operation, bool& value) {
        data = {}; data.size = sizeof(data); data.version = 2; data.operation = operation; data.count = 1;
        if (!copy(data.module, module) || !copy(data.section, section) || !copy(data.key, key)) return false;
        if (operation == PCSX2F_SETTINGS_WRITE) { data.value[0] = value ? '1' : '0'; data.value[1] = 0; }
        if (PCSX2F_BoolPeerRequest(&data) != PCSX2F_SETTINGS_OK ||
            (data.value[0] != '0' && data.value[0] != '1') || data.value[1]) return false;
        value = data.value[0] == '1'; return true;
    }
public:
    RestartBoolPeer(const char* peer, const char* group, const char* setting) : module(peer), section(group), key(setting) {}
    bool Read(bool& value) { return invoke(PCSX2F_SETTINGS_READ, value); }
    bool Write(bool value) { const bool requested = value; return invoke(PCSX2F_SETTINGS_WRITE, value) && value == requested; }
};

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
