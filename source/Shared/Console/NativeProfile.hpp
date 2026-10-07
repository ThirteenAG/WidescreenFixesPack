#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace console {
// Instruction fingerprints describe structure; relocatable addresses and the
// game's GP-relative offsets are decoded from the matched executable.
enum class AddressKind : uint8_t { Code, Call, Absolute, GP };
struct AddressRule {
    uintptr_t reference;
    uintptr_t referenceMatch;
    const uint32_t* words;
    const uint8_t* masks;
    uint8_t count, anchor;
    int16_t offset, high;
    uint16_t delta;
    AddressKind kind;
};
struct NativeImage {
    uintptr_t base;
    const uint8_t* bytes;
    size_t size;
    uintptr_t gp;
    bool contains(uintptr_t address, size_t length) const {
        return address >= base && address - base <= size && length <= size - (address - base);
    }
    uint32_t word(uintptr_t address) const {
        uint32_t result = 0;
        if (contains(address, 4)) std::memcpy(&result, bytes + (address - base), 4);
        return result;
    }
};
inline uint32_t instructionMask(uint8_t mask) {
    return mask == 1 ? 0xFFFF0000u : mask == 2 ? 0xFC000000u : 0xFFFFFFFFu;
}
inline bool matches(const NativeImage& image, uintptr_t at, const AddressRule& rule) {
    if (!image.contains(at, size_t(rule.count) * 4)) return false;
    for (unsigned i = 0; i < rule.count; ++i)
        if ((image.word(at + i * 4) & instructionMask(rule.masks[i])) != rule.words[i]) return false;
    return true;
}
inline uintptr_t findUnique(const NativeImage& image, const AddressRule& rule) {
    size_t length = size_t(rule.count) * 4;
    if (!rule.count || rule.anchor >= length || length > image.size) return 0;
    // One selective byte drives memchr; only its hits need full instruction
    // comparisons. This runs during installation, never while rendering.
    unsigned wordIndex = rule.anchor / 4, byteIndex = rule.anchor % 4;
    uint8_t anchor = uint8_t(rule.words[wordIndex] >> (byteIndex * 8));
    uintptr_t unique = 0;
    size_t cursor = 0, limit = image.size - length;
    while (cursor <= limit) {
        auto hit = static_cast<const uint8_t*>(std::memchr(image.bytes + cursor + rule.anchor,
                                                        anchor, limit - cursor + 1));
        if (!hit) break;
        cursor = size_t(hit - image.bytes) - rule.anchor;
        uintptr_t at = image.base + cursor;
        if (!(at & 3) && matches(image, at, rule)) {
            if (unique) return 0;
            unique = at;
        }
        ++cursor;
    }
    return unique;
}
inline uintptr_t decodeAddress(const NativeImage& image, uintptr_t match, const AddressRule& rule) {
    uintptr_t instruction = match + rule.offset;
    if (!image.contains(instruction, 4)) return 0;
    uint32_t low = image.word(instruction);
    switch (rule.kind) {
    case AddressKind::Code: return instruction;
    case AddressKind::Call:
        return low >> 26 == 3 ? ((instruction + 4) & ~uintptr_t(0x0FFFFFFF)) +
                                      uintptr_t((low & 0x03FFFFFF) << 2) + rule.delta : 0;
    case AddressKind::Absolute: {
        uintptr_t highAddress = rule.high < 0 ? match - unsigned(-rule.high) : match + unsigned(rule.high);
        if (!image.contains(highAddress, 4)) return 0;
        uint32_t high = image.word(highAddress);
        if (high >> 26 != 15 || ((high >> 16) & 31) != ((low >> 21) & 31)) return 0;
        return uintptr_t((high & 65535) << 16) +
               (low >> 26 == 13 ? uintptr_t(low & 65535) : uintptr_t(intptr_t(int16_t(low)))) + rule.delta;
    }
    case AddressKind::GP:
        return image.gp && ((low >> 21) & 31) == 28 ?
            image.gp + intptr_t(int16_t(low)) + rule.delta : 0;
    }
    return 0;
}
template<const auto& Rules> class NativeProfile {
    static constexpr size_t count = Rules.size();
    std::array<uintptr_t, count> addresses{};
    uintptr_t rejectedReference = 0;
    static constexpr size_t index(uintptr_t reference) {
        for (size_t i = 0; i < count; ++i) if (Rules[i].reference == reference) return i;
        return count;
    }
public:
    uintptr_t failed_reference() const { return rejectedReference; }
    template<uintptr_t Reference> uintptr_t get() const {
        constexpr size_t i = index(Reference);
        static_assert(i < count, "Missing native address profile");
        return addresses[i];
    }
    uintptr_t get(uintptr_t reference) const {
        size_t first = 0, last = count;
        while (first < last) {
            size_t middle = first + (last - first) / 2;
            if (Rules[middle].reference < reference) first = middle + 1;
            else last = middle;
        }
        return first < count && Rules[first].reference == reference ? addresses[first] : 0;
    }
    bool initialize(const NativeImage& image) {
        rejectedReference = 0;
        if (!image.bytes || !image.size || image.base > UINTPTR_MAX - image.size) {
            addresses.fill(0);
            return false;
        }
        // The reference build takes a short validation path. Other builds must
        // uniquely resolve every required site before any patch is installed.
        bool referenceBuild = true;
        for (const auto& rule : Rules) {
            if (!matches(image, rule.referenceMatch, rule)) {
                referenceBuild = false;
                break;
            }
        }
        for (size_t i = 0; i < count; ++i) {
            const auto& rule = Rules[i];
            uintptr_t match = referenceBuild ? rule.referenceMatch : findUnique(image, rule);
            uintptr_t address = match ? decodeAddress(image, match, rule) : 0;
            if (!address || ((address ^ rule.reference) & 3) ||
                ((rule.kind == AddressKind::Code || rule.kind == AddressKind::Call) &&
                 !image.contains(address & ~uintptr_t(3), 4))) {
                addresses.fill(0);
                rejectedReference = rule.reference;
                return false;
            }
            addresses[i] = address;
        }
        return true;
    }
};
}
