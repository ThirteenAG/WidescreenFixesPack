#include "Game.hpp"
#include <cstdio>
namespace icons {
namespace {
struct Raster { void* displayList; uint8_t* data; int16_t stride; uint8_t width, height, depth, levels; uint16_t flags; };
struct Texture { Raster* raster; void* dictionary; void* links[2]; char name[32], mask[32]; };
static_assert(sizeof(Raster) == 16 && sizeof(Texture) == 80);

// Icon sets are native rasters converted offline from the PPSSPP replacement
// art (tools/gamepad-icons/convert.py) and stored one file per set next to the
// plugin. The PSP set is the game's own artwork, kept from the loaded texture.
constexpr const char* sets[] = {"PSP", "360", "XBO", "PS3", "PS4", "PS5", "SWI", "DEK", "Auto"};
constexpr unsigned SetCount = sizeof(sets) / sizeof(*sets);
constexpr auto folder = "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.FusionFix/icons/";
struct Icon
{
    const char* name;
    uintptr_t rasterIdentity;
    uint32_t backup, size, markAt, appliedHash, revision;
    bool saved;
    uint8_t mark[16];
};
Icon icons[] = {
    {"btn_down_PSP"}, {"btn_up_PSP"}, {"btn_cross_PSP"}, {"btn_circle_PSP"}, {"btn_left_PSP"}, {"btn_right_PSP"},
    {"btn_square_PSP"}, {"btn_triangle_PSP"}, {"fe_controller_new"}, {"fe_arrows_onfoot_layout1"},
    {"fe_arrows_layout_right"}, {"fe_arrows_invehicle_layout2"}, {"GameText"}
};
// Originals of every icon texture (the PSP set) and the selected set's file.
constexpr unsigned BackupBytes = 128 * 1024, SetBytes = 96 * 1024;
alignas(64) uint8_t backup[BackupBytes];
uint32_t backupUsed;
alignas(64) uint8_t setData[SetBytes];
uint32_t setSize, setHash, setRevision;
unsigned selected = 8, loaded = ~0u, lastPoll;
ScePspDateTime loadedTime{};
bool installed, reported;

// Set file entry: name, raster layout (log2 width/height, depth, levels, stride,
// flags), raster byte count and runs {offset, length, bytes} that differ from the
// game's original; runs are 4-byte aligned.
struct Entry { char name[32]; uint8_t width, height, depth, levels; int16_t stride; uint16_t flags; uint32_t bytes, runs; };
struct Run { uint32_t offset, length; };
static_assert(sizeof(Entry) == 48 && sizeof(Run) == 8);

// Raster bytes: levels of stride * height (stride halving down to 16 bytes), then the palette.
uint32_t Bytes(const Raster& r) {
    const unsigned format = r.flags >> 8, levels = r.levels & 63;
    if (!levels || levels > 8 || r.stride < 16 || (r.width & 63) > 10 || (r.height & 63) > 10) return 0;
    uint32_t bytes = 0, stride = uint32_t(r.stride), h = 1u << (r.height & 63);
    for (unsigned l = 0; l < levels; ++l) { bytes += stride * h; if (stride >= 17) stride /= 2; h = h > 1 ? h / 2 : 1; }
    if (format & 0x40) bytes += 16 * 4;
    else if (format & 0x20) bytes += 256 * 4;
    return bytes;
}
uint32_t Hash(const uint8_t* data, uint32_t bytes)
{
    uint32_t hash = 2166136261u;
    for (uint32_t i = 0; i < bytes; ++i) hash = (hash ^ data[i]) * 16777619u;
    return hash;
}

void PollSet();

// Walks one entry's runs; returns the entry size or 0 when the file is malformed.
uint32_t Size(const Entry* entry, uint32_t at) {
    uint32_t size = sizeof(Entry);
    if (entry->bytes < 16 || entry->bytes > SetBytes || entry->runs > (setSize - at - size) / sizeof(Run)) return 0;
    for (uint32_t i = 0; i < entry->runs; ++i) {
        if (sizeof(Run) > setSize - at - size) return 0;
        const auto run = reinterpret_cast<const Run*>(setData + at + size);
        if (run->offset > entry->bytes || run->length > entry->bytes - run->offset) return 0;
        const auto advance = (sizeof(Run) + run->length + 3) & ~3u;
        if (advance > setSize - at - size) return 0;
        size += advance;
    }
    return size;
}
const Entry* Find(const char* name) {
    for (uint32_t at = 0; at + sizeof(Entry) <= setSize;) {
        const auto entry = reinterpret_cast<const Entry*>(setData + at);
        const uint32_t size = Size(entry, at);
        if (!size) break;
        if (!std::strncmp(entry->name, name, 32)) return entry;
        at += size;
    }
    return nullptr;
}
void PollSet()
{
    const unsigned set = selected == 8 ? 1 : selected;
    const unsigned now = sceKernelGetSystemTimeLow();
    const bool changed = loaded != set;
    if (!changed && (!set || now - lastPoll < 1000000u)) return;
    lastPoll = now;
    loaded = set;
    const bool previouslyValid = setSize != 0;
    const uint32_t previousSize = setSize;
    if (!set)
    {
        setSize = 0;
        ++setRevision;
        reported = false;
        return;
    }
    char path[128];
    std::snprintf(path, sizeof(path), "%s%s.bin", folder, sets[set]);
    SceIoStat stat{};
    const bool present = sceIoGetstat(path, &stat) >= 0 && stat.st_size > 0 && stat.st_size <= sizeof(setData);
    if (present && !changed && previouslyValid && stat.st_size == previousSize &&
        !std::memcmp(&loadedTime, &stat.sce_st_mtime, sizeof(loadedTime))) return;
    setSize = 0;
    if (present)
    {
        const SceUID file = sceIoOpen(path, PSP_O_RDONLY, 0777);
        if (file >= 0)
        {
            const int bytes = sceIoRead(file, setData, unsigned(stat.st_size));
            const int closed = sceIoClose(file);
            if (bytes == stat.st_size && closed >= 0)
            {
                setSize = unsigned(bytes);
                for (uint32_t at = 0; at < setSize;)
                {
                    if (setSize - at < sizeof(Entry))
                    {
                        setSize = 0;
                        break;
                    }
                    const auto* entry = reinterpret_cast<const Entry*>(setData + at);
                    const uint32_t length = Size(entry, at);
                    if (!length)
                    {
                        setSize = 0;
                        break;
                    }
                    at += length;
                }
                loadedTime = stat.sce_st_mtime;
            }
        }
    }
    const uint32_t hash = setSize ? Hash(setData, setSize) : 0;
    if (changed || previouslyValid != (setSize != 0) || setHash != hash) ++setRevision;
    setHash = hash;
    if (!setSize && !reported)
        logger.Write("Controller icon pack unavailable. Reinstall the selected pack.\n");
    reported = !setSize;
}

void Apply(Icon& icon, Raster& raster)
{
    std::memcpy(raster.data, backup + icon.backup, icon.size);
    if (const Entry* entry = Find(icon.name))
        if (entry->width == raster.width && entry->height == raster.height && entry->depth == raster.depth &&
            entry->levels == raster.levels && entry->stride == raster.stride && entry->flags == raster.flags &&
            entry->bytes == icon.size)
        {
            auto at = reinterpret_cast<const uint8_t*>(entry + 1);
            for (uint32_t i = 0; i < entry->runs; ++i)
            {
                const auto* run = reinterpret_cast<const Run*>(at);
                std::memcpy(raster.data + run->offset, at + sizeof(Run), run->length);
                at += (sizeof(Run) + run->length + 3) & ~3u;
            }
        }
    sceKernelDcacheWritebackRange(raster.data, icon.size);
    icon.appliedHash = Hash(raster.data, icon.size);
    icon.markAt = 0;
    for (uint32_t at = 0; at + sizeof(icon.mark) <= icon.size; at += sizeof(icon.mark))
        if (std::memcmp(raster.data + at, backup + icon.backup + at, sizeof(icon.mark)))
        {
            icon.markAt = at;
            break;
        }
    std::memcpy(icon.mark, raster.data + icon.markAt, sizeof(icon.mark));
    icon.rasterIdentity = reinterpret_cast<uintptr_t>(raster.data);
    icon.revision = setRevision;
}

SafetyMipsInline readTexture;
injector::hook_back<void(uintptr_t, uint8_t)> controller;
injector::game_fn<uint8_t*(int)> getPad;
injector::game_fn<uint8_t(uint8_t*)> guiLeft;
injector::game_fn<uint8_t(uint8_t*)> guiRight;

Texture* ReadTexture(const char* name, uintptr_t mask, uintptr_t flags)
{
    auto* texture = readTexture.call<Texture*>(name, mask, flags);
    if (!name || !texture || !texture->raster || !texture->raster->data) return texture;
    for (auto& icon : icons)
    {
        if (std::strcmp(name, icon.name)) continue;
        PollSet();
        auto& raster = *texture->raster;
        const uint32_t size = Bytes(raster);
        if (size < sizeof(icon.mark)) break;
        if (icon.saved && icon.size == size && icon.revision == setRevision &&
            icon.rasterIdentity == reinterpret_cast<uintptr_t>(raster.data) &&
            !std::memcmp(raster.data + icon.markAt, icon.mark, sizeof(icon.mark))) break;
        if (!icon.saved)
        {
            const auto reserved = (size + 63u) & ~63u;
            if (reserved > BackupBytes - backupUsed) break;
            icon.backup = backupUsed;
            icon.size = size;
            backupUsed += reserved;
        }
        else if (icon.size != size) break;
        if (!icon.saved || Hash(raster.data, size) != icon.appliedHash)
            std::memcpy(backup + icon.backup, raster.data, size);
        icon.saved = true;
        Apply(icon, raster);
        break;
    }
    return texture;
}
void SetController(uintptr_t object, uint8_t mode) {
    const auto pad = getPad(0);
    int change = 0;
    if (pad && guiLeft(pad) && (mode == 0 || mode == 2)) change = -1;
    else if (pad && guiRight(pad) && (mode == 1 || mode == 3)) change = 1;
#ifndef NDEBUG
    logger.WriteF("Controller mode %u left %u right %u", unsigned(mode), pad ? guiLeft(pad) : 0, pad ? guiRight(pad) : 0);
#endif
    if (change) {
        vcsws::SetMenuGamepadIcons(unsigned(int(selected) + int(SetCount) + change) % SetCount);
    }
    controller.fun(object, mode);
}
}
void Install(const char* name) {
    for (unsigned i = 0; i < SetCount; ++i)
        if (std::strcmp(name, sets[i]) == 0) { selected = i; break; }
    getPad = decltype(getPad)(pattern.get_first("C0 20 04 00 23 10 85 00 ? ? ? ? ? ? ? ? 08 00 E0 03 21 10 44 00", -8));
    constexpr auto gui = "00 00 B0 AF 04 00 BF AF ? ? ? ? 25 80 80 00 ? ? ? ? 00 00 00 00 ? ? ? ? 25 20 00 02 ? ? ? ? 00 00 00 00 ? ? ? ? 01 00 02 34 25 10 00 00";
    guiLeft = decltype(guiLeft)(pattern.get(0, gui, -4));
    guiRight = decltype(guiRight)(pattern.get(1, gui, -4));
    const auto read = pattern.get_first("25 28 C0 00 00 00 02 AE", -4);
    const auto set = pattern.get_first("25 28 C0 00 ? ? ? ? 00 00 00 00 2B 20 04 00 FF 00 86 30 25 20 A0 00 ? ? ? ? 25 28 C0 00 ? ? ? ? 00 00 00 00 2B 20 04 00 FF 00 86 30 25 20 A0 00 ? ? ? ? 25 28 C0 00 ? ? ? ? 00 00 00 00 2B 20 04 00", -4);
    if (!getPad || !guiLeft || !guiRight || !read || !set)
    {
        logger.Write("Controller icons unavailable: unsupported game code.\n");
        return;
    }
    readTexture = safetymips::create_inline(injector::GetBranchDestination(read).as_int(), ReadTexture);
    controller.fun = injector::MakeCALL(set, SetController).get();
    installed = true;
}
}
namespace vcsws {
// MAIN.GamepadIcons: the set is read at startup; the Plugins page and the
// game's controller menu change it live.
void InstallGamepadIcons(const char* set) { icons::Install(set); }
bool GamepadIconsAvailable() { return icons::installed; }
unsigned GamepadIconSet() { return icons::selected; }
void SelectGamepadIcons(unsigned set) {
    if (!icons::installed || set >= icons::SetCount || set == icons::selected) return;
    icons::selected = set;
}
}
