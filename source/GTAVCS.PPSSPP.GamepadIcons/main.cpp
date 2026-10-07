#include "../Shared/Console/PSP.hpp"
#include <cstdio>
extern "C" { PSP_MODULE_INFO("GTAVCS.GamepadIcons", PSP_MODULE_USER, 2, 0); }
namespace icons {
using namespace console::portable;
namespace {
struct Raster { void* displayList; uint8_t* data; int16_t stride; uint8_t width, height, depth, levels; uint16_t flags; };
struct Texture { Raster* raster; void* dictionary; void* links[2]; char name[32], mask[32]; };
static_assert(sizeof(Raster) == 16 && sizeof(Texture) == 80);

// Icon sets are native rasters converted offline from the PPSSPP replacement
// art (tools/gamepad-icons/convert.py) and stored one file per set next to the
// plugin. The PSP set is the game's own artwork, kept from the loaded texture.
constexpr const char* sets[] = {"PSP", "360", "XBO", "PS3", "PS4", "PS5", "SWI", "DEK"};
constexpr unsigned SetCount = sizeof(sets) / sizeof(*sets);
constexpr auto path = "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.GamepadIcons/GTAVCS.PPSSPP.GamepadIcons.ini";
constexpr auto folder = "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.GamepadIcons/icons/";
struct Icon { const char* name; Texture* texture; uint8_t* data; uint32_t backup, size, markAt; bool saved; uint8_t mark[16]; };
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
uint32_t setSize;
unsigned selected, loaded = ~0u;

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
bool LoadSet(unsigned set) {
    if (loaded == set) return setSize != 0;
    loaded = set; setSize = 0;
    if (!set) return false;
    char file[128];
    std::snprintf(file, sizeof(file), "%s%s.bin", folder, sets[set]);
    const SceUID handle = sceIoOpen(file, PSP_O_RDONLY, 0777);
    if (handle < 0) return false;
    const int count = sceIoRead(handle, setData, sizeof(setData));
    sceIoClose(handle);
    if (count > 0) setSize = uint32_t(count);
#ifndef NDEBUG
    logger.WriteF("Icon set %s: %d bytes", sets[set], count);
#endif
    return setSize != 0;
}
// Walks one entry's runs; returns the entry size or 0 when the file is malformed.
uint32_t Size(const Entry* entry, uint32_t at) {
    uint32_t size = sizeof(Entry);
    for (uint32_t i = 0; i < entry->runs; ++i) {
        if (at + size + sizeof(Run) > setSize) return 0;
        const auto run = reinterpret_cast<const Run*>(setData + at + size);
        if (run->offset > entry->bytes || run->length > entry->bytes - run->offset) return 0;
        size += (sizeof(Run) + run->length + 3) & ~3u;
        if (at + size > setSize) return 0;
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
bool SameName(const char* a, const char* b) {
    for (unsigned i = 0; i < 32; ++i) {
        char x = a[i], y = b[i];
        if (x >= 'A' && x <= 'Z') x = char(x - 'A' + 'a');
        if (y >= 'A' && y <= 'Z') y = char(y - 'A' + 'a');
        if (x != y) return false;
        if (!x) return true;
    }
    return true;
}
void Apply(Icon& icon) {
    Texture* texture = icon.texture;
    if (!texture || !texture->raster || !texture->raster->data || !icon.saved) return;
    // A dictionary can be freed without the display-list callback (menu
    // unload while I/O is suspended). Only write a texture that still is the
    // one this icon backed up.
    if (texture->raster->data != icon.data || !SameName(texture->name, icon.name)) {
        icon.texture = nullptr;
        return;
    }
    Raster& raster = *texture->raster;
    std::memcpy(raster.data, backup + icon.backup, icon.size);
    if (selected && LoadSet(selected))
        if (const Entry* entry = Find(icon.name))
            if (entry->width == raster.width && entry->height == raster.height && entry->depth == raster.depth &&
                entry->levels == raster.levels && entry->stride == raster.stride && entry->flags == raster.flags &&
                entry->bytes == icon.size) {
                auto at = reinterpret_cast<const uint8_t*>(entry + 1);
                for (uint32_t i = 0; i < entry->runs; ++i) {
                    const auto run = reinterpret_cast<const Run*>(at);
                    std::memcpy(raster.data + run->offset, at + sizeof(Run), run->length);
                    at += (sizeof(Run) + run->length + 3) & ~3u;
                }
            }
    sceKernelDcacheWritebackRange(raster.data, icon.size);
    // Remember bytes that differ from the original so a reload at the same address is noticed.
    icon.markAt = 0;
    for (uint32_t at = 0; at + sizeof(icon.mark) <= icon.size; at += sizeof(icon.mark))
        if (std::memcmp(raster.data + at, backup + icon.backup + at, sizeof(icon.mark))) { icon.markAt = at; break; }
    std::memcpy(icon.mark, raster.data + icon.markAt, sizeof(icon.mark));
}
void ApplyAll() { for (auto& icon : icons) Apply(icon); }

SafetyMipsInline readTexture, destroyDisplayList;
injector::hook_back<void(uintptr_t, uint8_t)> controller;
uint8_t* (*getPad)(int);
uint8_t (*guiLeft)(uint8_t*);
uint8_t (*guiRight)(uint8_t*);

Texture* ReadTexture(const char* name, uintptr_t mask, uintptr_t flags) {
    auto texture = readTexture.call<Texture*>(name, mask, flags);
    if (!name || !texture || !texture->raster || !texture->raster->data) return texture;
    for (auto& icon : icons) if (std::strcmp(name, icon.name) == 0) {
        const Raster& r = *texture->raster;
        // The game looks textures up every frame: act only on a new or reloaded raster.
        if (icon.saved && icon.texture == texture && icon.data == r.data && !std::memcmp(r.data + icon.markAt, icon.mark, sizeof(icon.mark))) break;
        const uint32_t size = Bytes(r);
#ifndef NDEBUG
        logger.WriteF("Icon %s raster %ux%u depth %u levels %u stride %d flags %04x bytes %u data %08x",
                      name, 1u << (r.width & 63), 1u << (r.height & 63), r.depth, r.levels, r.stride, r.flags,
                      unsigned(size), unsigned(uintptr_t(r.data)));
#endif
        if (!size) break;
        // Keep the original artwork: the first load reserves its bounded backup slot.
        if (!icon.saved) {
            if (backupUsed + size > BackupBytes) break;
            icon.backup = backupUsed; icon.size = size; backupUsed += (size + 63) & ~63u;
        } else if (icon.size != size) break;
        std::memcpy(backup + icon.backup, r.data, size);
        icon.saved = true;
        icon.texture = texture;
        icon.data = r.data;
        Apply(icon);
        break;
    }
    return texture;
}
Texture* DestroyDisplayList(Texture* texture, void* data) {
    // Streaming releases a texture dictionary immediately after this callback.
    // Remove cached pointers while their objects are still alive.
    for (auto& icon : icons) if (icon.texture == texture) icon.texture = nullptr;
    return destroyDisplayList.call<Texture*>(texture, data);
}
void Save() {
    const SceUID file = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (file >= 0) { sceIoWrite(file, sets[selected], 3); sceIoClose(file); }
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
        selected = unsigned(int(selected) + int(SetCount) + change) % SetCount;
        ApplyAll();
        Save();
    }
    controller.fun(object, mode);
}
int Install() {
    sceKernelDelayThread(120000);
    if (!Begin()) return -1;
    char stored[4]{};
    const SceUID file = sceIoOpen(path, PSP_O_RDONLY, 0777);
    if (file >= 0) {
        const int count = sceIoRead(file, stored, 3); sceIoClose(file);
        if (count == 3) for (unsigned i = 0; i < SetCount; ++i)
            if (std::memcmp(stored, sets[i], 3) == 0) { selected = i; break; }
    }
    getPad = reinterpret_cast<decltype(getPad)>(pattern.get_first("C0 20 04 00 23 10 85 00 ? ? ? ? ? ? ? ? 08 00 E0 03 21 10 44 00", -8));
    constexpr auto gui = "00 00 B0 AF 04 00 BF AF ? ? ? ? 25 80 80 00 ? ? ? ? 00 00 00 00 ? ? ? ? 25 20 00 02 ? ? ? ? 00 00 00 00 ? ? ? ? 01 00 02 34 25 10 00 00";
    guiLeft = reinterpret_cast<decltype(guiLeft)>(pattern.get(0, gui, -4));
    guiRight = reinterpret_cast<decltype(guiRight)>(pattern.get(1, gui, -4));
    const auto read = pattern.get_first("25 28 C0 00 00 00 02 AE", -4);
    const auto set = pattern.get_first("25 28 C0 00 ? ? ? ? 00 00 00 00 2B 20 04 00 FF 00 86 30 25 20 A0 00 ? ? ? ? 25 28 C0 00 ? ? ? ? 00 00 00 00 2B 20 04 00 FF 00 86 30 25 20 A0 00 ? ? ? ? 25 28 C0 00 ? ? ? ? 00 00 00 00 2B 20 04 00", -4);
    // The destruction profile belongs to the same supported executable as the
    // load patterns. Reject an unknown version before installing any hooks.
    if (!getPad || !guiLeft || !guiRight || injector::ReadMemory<uint32_t>(0x08A65C4C) != 0x27BDFFF0 ||
        injector::GetBranchDestination(read).as_int() != 0x08A65C4C) return -1;
    readTexture = safetymips::create_inline(0x08A65C4C, ReadTexture);
    destroyDisplayList = safetymips::create_inline(0x08B21348, DestroyDisplayList);
    controller.fun = injector::MakeCALL(set, SetController).get();
    return Finish();
}
}
}
extern "C" int module_start(SceSize, void*) {
    if (!console::portable::Start("GTA3", icons::path, "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.GamepadIcons/GTAVCS.PPSSPP.GamepadIcons.log")) return 0;
    return icons::Install();
}
