#include "Game.hpp"
#include "IconAssets.hpp"
#include "IconProfile.hpp"
#include "IconPack.hpp"
#include <cstdio>
#include <cstring>

namespace lcsws
{
namespace
{
struct Raster
{
    void* displayList;
    uint8_t* data;
    int16_t stride;
    uint8_t width, height, depth, levels;
    uint16_t flags;
};
struct Texture
{
    Raster* raster;
    void* dictionary;
    void* links[2];
    char name[32], mask[32];
};
using icon_pack::Entry;
using icon_pack::Run;
static_assert(sizeof(Raster) == 16 && sizeof(Texture) == 80);
constexpr size_t AssetCount = sizeof(icon_assets::assets) / sizeof(icon_assets::assets[0]);
struct Original
{
    uint32_t offset, appliedHash, markAt;
    uintptr_t data;
    unsigned set;
    bool saved;
    uint8_t mark[16];
};
Original originals[AssetCount]{};
alignas(64) uint8_t backup[96 * 1024];
alignas(64) uint8_t setData[96 * 1024];
uint32_t setSize;
unsigned loadedSet, requestedSet, lastPoll;
bool available, dataValid = true, reported;

bool Valid(const void* pointer, size_t bytes)
{
    const auto address = reinterpret_cast<uintptr_t>(pointer);
    return bytes <= 0x08000000u && address >= 0x08000000u && address <= 0x10000000u - bytes && !(address & 3);
}
uint32_t Hash(const uint8_t* data, size_t bytes)
{
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < bytes; ++i) hash = (hash ^ data[i]) * 16777619u;
    return hash;
}
Texture* FindTexture(const char* name)
{
    // Native owners retain these textures after their source dictionaries can
    // leave the active list. Read the owner afresh; never retain its pointers.
    uintptr_t owned = 0;
    if (!std::strcmp(name, "font2"))
        owned = *reinterpret_cast<const uintptr_t*>(icon_profile::profile.get<0x08E55970>());
    else
    {
        const uintptr_t menu = Address<0x08B8EE20>();
        if (*reinterpret_cast<const uint8_t*>(menu + 0x48C))
            for (const auto& binding : icon_owners::menuBindings)
                if (!std::strcmp(name, binding.name))
                {
                    owned = *reinterpret_cast<const uintptr_t*>(menu + 0x490 + binding.slot * 4);
                    break;
                }
    }
    auto texture = reinterpret_cast<Texture*>(owned);
    return Valid(texture, sizeof(Texture)) && !std::strncmp(texture->name, name, sizeof(texture->name)) ? texture : nullptr;
}
uint32_t EntrySize(uint32_t at)
{
    return icon_pack::View{setData, setSize}.EntrySize(at);
}
const Entry* FindEntry(const char* name)
{
    return icon_pack::View{setData, setSize}.Find(name);
}
bool LoadSet(unsigned selected)
{
    if (loadedSet == selected && setSize) return true;
    setSize = 0;
    loadedSet = selected;
    char path[160]{};
    std::snprintf(path, sizeof(path), "ms0:/PSP/PLUGINS/GTALCS.PPSSPP.FusionFix/icons/%s.bin", iconSets[selected]);
    SceIoStat info{};
    if (sceIoGetstat(path, &info) < 0 || info.st_size <= 0 || info.st_size > sizeof(setData)) return false;
    const auto file = sceIoOpen(path, PSP_O_RDONLY, 0777);
    if (file < 0) return false;
    unsigned count = 0;
    while (count < unsigned(info.st_size))
    {
        const int bytes = sceIoRead(file, setData + count, unsigned(info.st_size) - count);
        if (bytes <= 0) break;
        count += unsigned(bytes);
    }
    sceIoClose(file);
    if (count != unsigned(info.st_size)) return false;
    setSize = count;
    for (uint32_t at = 0; at < setSize;)
    {
        const uint32_t bytes = EntrySize(at);
        if (!bytes) { setSize = 0; return false; }
        at += bytes;
    }
    return true;
}
bool Apply(Texture* texture, size_t index, unsigned selected)
{
    if (!Valid(texture->raster, sizeof(Raster))) return false;
    auto& raster = *texture->raster;
    const auto& asset = icon_assets::assets[index];
    auto& original = originals[index];
    if (!Valid(raster.data, asset.bytes)) return false;
    if (!original.saved && !selected) return true;
    if (original.saved && original.set == selected && original.data == reinterpret_cast<uintptr_t>(raster.data) &&
        !std::memcmp(raster.data + original.markAt, original.mark, sizeof(original.mark))) return true;
    const Entry* entry = selected ? FindEntry(asset.name) : nullptr;
    if (selected && (!entry || entry->bytes != asset.bytes || entry->stride != raster.stride ||
        entry->width != (raster.width & 63) || entry->height != (raster.height & 63) ||
        entry->depth != raster.depth || entry->levels != raster.levels || entry->flags != raster.flags)) return false;
    const uint32_t current = Hash(raster.data, asset.bytes);
    if (!original.saved)
    {
        if (current != asset.hash) return false;
        std::memcpy(backup + original.offset, raster.data, asset.bytes);
        original.saved = true;
    }
    else if (current != asset.hash && current != original.appliedHash) return false;
    // Every write uses a fresh lookup. No cached texture pointer is dereferenced
    // after a dictionary can have been unloaded or replaced.
    std::memcpy(raster.data, backup + original.offset, asset.bytes);
    if (entry)
    {
        auto at = reinterpret_cast<const uint8_t*>(entry + 1);
        for (uint32_t i = 0; i < entry->runs; ++i)
        {
            const auto run = reinterpret_cast<const Run*>(at);
            std::memcpy(raster.data + run->offset, at + sizeof(Run), run->length);
            at += (sizeof(Run) + run->length + 3) & ~3u;
        }
    }
    original.appliedHash = Hash(raster.data, asset.bytes);
    original.markAt = 0;
    for (uint32_t at = 0; at + sizeof(original.mark) <= asset.bytes; at += sizeof(original.mark))
        if (std::memcmp(raster.data + at, backup + original.offset + at, sizeof(original.mark)))
        {
            original.markAt = at;
            break;
        }
    std::memcpy(original.mark, raster.data + original.markAt, sizeof(original.mark));
    original.data = reinterpret_cast<uintptr_t>(raster.data);
    original.set = selected;
    sceKernelDcacheWritebackRange(raster.data, asset.bytes);
    return true;
}
}

bool IconsAvailable() { return available; }
bool IconDataValid() { return dataValid; }
void InstallIcons()
{
    unsigned used = 0;
    for (size_t i = 0; i < AssetCount; ++i)
    {
        originals[i].offset = used;
        used += (icon_assets::assets[i].bytes + 63) & ~63u;
    }
    available = used <= sizeof(backup) && icon_profile::profile.initialize({pattern.text_addr,
        reinterpret_cast<const uint8_t*>(pattern.text_addr), pattern.text_size, console::portable::gameGP});
    if (!available) logger.Write("Controller icons unavailable: unsupported texture lookup code. Original artwork preserved.\n");
}
void TickIcons(unsigned now)
{
    const unsigned desiredSet = settings.icons == 8 ? 1u : unsigned(settings.icons);
    if (!available || (!desiredSet && !requestedSet)) return;
    if (now - lastPoll < 1000000u && requestedSet == desiredSet) return;
    lastPoll = now;
    requestedSet = desiredSet;
    dataValid = !requestedSet || LoadSet(requestedSet);
    const unsigned selected = dataValid ? requestedSet : 0;
    for (size_t i = 0; i < AssetCount; ++i)
        if (auto texture = FindTexture(icon_assets::assets[i].name))
            if (!Apply(texture, i, selected)) dataValid = false;
    if (!dataValid && !reported)
        logger.Write("Controller icon data missing or incompatible; affected artwork preserved. Reinstall the matching icon files.\n");
    reported = !dataValid;
}
}
