#ifndef PCSX2F_PLUGIN_SETTINGS_BATCH_H
#define PCSX2F_PLUGIN_SETTINGS_BATCH_H

namespace PCSX2FSettingsBatch
{
inline constexpr uint32_t Version = 3;
inline constexpr uint32_t Capability = 1;
inline constexpr uint32_t Read = 0;
inline constexpr uint32_t Seed = 1;
inline constexpr uint32_t SeedSkipped = 5;
inline constexpr uint32_t MaxEntries = 64;

struct Request
{
    uint32_t size, version, operation, count;
    uint32_t present[2];
    PCSX2FIniEntry entries[MaxEntries];

    bool has(uint32_t index) const { return (present[index / 32] & (uint32_t{1} << (index % 32))) != 0; }
    void mark(uint32_t index) { present[index / 32] |= uint32_t{1} << (index % 32); }
};
static_assert(sizeof(Request) == 24 + 64 * 192);
}
#endif
