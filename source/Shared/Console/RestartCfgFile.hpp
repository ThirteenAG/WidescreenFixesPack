#pragma once
#include <cstddef>
#include <cstring>
extern "C" {
#include <pspiofilemgr.h>
}

namespace console::portable {
struct RestartBoolValue {
    using Value = bool;
    bool Valid(Value) const { return true; }
    bool Parse(const char* begin, const char* end, Value& value) const {
        if (end - begin != 1 || (*begin != '0' && *begin != '1')) return false;
        value = *begin == '1';
        return true;
    }
    bool Encode(char* output, size_t width, Value value) const {
        if (width != 1) return false;
        output[0] = value ? '1' : '0';
        return true;
    }
};
// Known fixed-width CFG tokens; no arbitrary string input or value resizing.
template<size_t Count, size_t Width> struct RestartChoiceValue {
    static_assert(Count > 0 && Count <= 256 && Width > 0 && Width <= 16);
    using Value = unsigned;
    const char (*choices)[Width + 1];
    bool Valid(Value value) const {
        if (!choices || value >= Count || choices[value][Width] ||
            std::memchr(choices[value], 0, Width)) return false;
        for (unsigned i = 0; i < Count; ++i)
            if (i != value && !std::memcmp(choices[value], choices[i], Width + 1)) return false;
        return true;
    }
    bool Parse(const char* begin, const char* end, Value& value) const {
        if (size_t(end - begin) != Width) return false;
        bool found = false;
        for (unsigned i = 0; i < Count; ++i) {
            if (!Valid(i) || std::memcmp(begin, choices[i], Width)) continue;
            if (found) return false;
            value = i; found = true;
        }
        return found;
    }
    bool Encode(char* output, size_t width, Value value) const {
        if (width != Width || !Valid(value)) return false;
        std::memcpy(output, choices[value], Width);
        return true;
    }
};
// A known sibling's restart request. Preserve its complete CFG; never read
// legacy INI files, create a missing CFG, or infer the running plugin's value.
template<size_t Capacity = 8192, class Codec = RestartBoolValue> class RestartCfgFile {
    using Value = typename Codec::Value;
    struct Snapshot {
        char data[Capacity];
        size_t size = 0, begin = 0, end = 0;
        Value value{};
    };
    Snapshot original{}, check{};
    char output[Capacity]{};
    char temporary[192]{}, backup[192]{};
    const char* path;
    const char* section;
    const char* key;
    const Codec codec;
    bool pathsValid = false;

    static bool same(const char* begin, const char* end, const char* expected) {
        const size_t length = size_t(end - begin);
        if (std::strlen(expected) != length) return false;
        for (size_t i = 0; i < length; ++i) {
            char a = begin[i], b = expected[i];
            if (a >= 'A' && a <= 'Z') a = char(a - 'A' + 'a');
            if (b >= 'A' && b <= 'Z') b = char(b - 'A' + 'a');
            if (a != b) return false;
        }
        return true;
    }
    static void trim(const char*& begin, const char*& end) {
        while (begin < end && (*begin == ' ' || *begin == '\t')) ++begin;
        while (end > begin && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r')) --end;
    }
    static bool missing(const char* name) {
        SceIoStat stat{};
        // ENOENT only. Permission/device failures must not authorize creation.
        return unsigned(sceIoGetstat(name, &stat)) == 0x80010002u;
    }
    bool readFile(const char* name, Snapshot& snapshot) {
        snapshot.size = 0;
        const SceUID handle = sceIoOpen(name, PSP_O_RDONLY, 0);
        if (handle < 0) return false;
        bool valid = true;
        while (snapshot.size < Capacity) {
            const int count = sceIoRead(handle, snapshot.data + snapshot.size, unsigned(Capacity - snapshot.size));
            if (count < 0) { valid = false; break; }
            if (!count) break;
            if (size_t(count) > Capacity - snapshot.size) { valid = false; break; }
            snapshot.size += size_t(count);
        }
        if (valid && snapshot.size == Capacity) {
            char extra;
            valid = sceIoRead(handle, &extra, 1) == 0;
        }
        if (sceIoClose(handle) < 0) valid = false;
        return valid && !std::memchr(snapshot.data, 0, snapshot.size);
    }
    bool parse(Snapshot& snapshot) const {
        bool inSection = false, found = false;
        const char* cursor = snapshot.data;
        const char* end = cursor + snapshot.size;
        while (cursor < end) {
            const char* newline = static_cast<const char*>(std::memchr(cursor, '\n', size_t(end - cursor)));
            const char* next = newline ? newline + 1 : end;
            const char* begin = cursor;
            const char* stop = newline ? newline : end;
            trim(begin, stop);
            if (begin < stop && *begin == '[') {
                const char* close = static_cast<const char*>(std::memchr(begin, ']', size_t(stop - begin)));
                if (!close) return false;
                const char* tail = close + 1;
                while (tail < stop && (*tail == ' ' || *tail == '\t')) ++tail;
                if (tail < stop && *tail != ';' && *tail != '#') return false;
                const char* sectionBegin = begin + 1;
                const char* sectionEnd = close;
                trim(sectionBegin, sectionEnd);
                inSection = same(sectionBegin, sectionEnd, section);
            } else if (inSection && begin < stop && *begin != ';' && *begin != '#') {
                const char* equals = static_cast<const char*>(std::memchr(begin, '=', size_t(stop - begin)));
                if (equals) {
                    const char* keyBegin = begin;
                    const char* keyEnd = equals;
                    trim(keyBegin, keyEnd);
                    if (same(keyBegin, keyEnd, key)) {
                        if (found) return false;
                        const char* value = equals + 1;
                        const char* valueEnd = value;
                        while (valueEnd < stop && *valueEnd != ';' && *valueEnd != '#') ++valueEnd;
                        trim(value, valueEnd);
                        if (!codec.Parse(value, valueEnd, snapshot.value)) return false;
                        snapshot.begin = size_t(value - snapshot.data);
                        snapshot.end = size_t(valueEnd - snapshot.data);
                        found = true;
                    }
                }
            }
            cursor = next;
        }
        return found;
    }
    bool equal(const Snapshot& a, const Snapshot& b) const {
        return a.size == b.size && !std::memcmp(a.data, b.data, a.size);
    }
    bool writeTemporary(size_t size) {
        const SceUID handle = sceIoOpen(temporary, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_EXCL, 0777);
        if (handle < 0) return false;
        size_t written = 0;
        while (written < size) {
            const int count = sceIoWrite(handle, output + written, unsigned(size - written));
            if (count <= 0 || size_t(count) > size - written) break;
            written += size_t(count);
        }
        const bool closed = sceIoClose(handle) >= 0;
        if (written != size || !closed) { sceIoRemove(temporary); return false; }
        if (!readFile(temporary, check) || check.size != size || std::memcmp(check.data, output, size)) {
            sceIoRemove(temporary); return false;
        }
        return true;
    }
public:
    RestartCfgFile(const char* filename, const char* sectionName, const char* keyName, Codec valueCodec = {})
        : path(filename), section(sectionName), key(keyName), codec(valueCodec) {
        if (!path || !section || !key || !*section || !*key) return;
        const size_t length = std::strlen(path);
        if (length + 10 > sizeof(temporary) || length < 4 || std::strcmp(path + length - 4, ".cfg")) return;
        std::memcpy(temporary, path, length); std::memcpy(temporary + length, ".menu.tmp", 10);
        std::memcpy(backup, path, length); std::memcpy(backup + length, ".menu.bak", 10);
        pathsValid = true;
    }
    bool Read(Value& requested) {
        if (!pathsValid || !missing(temporary) || !missing(backup) || !readFile(path, check) || !parse(check)) return false;
        requested = check.value;
        return true;
    }
    template<class Current> bool Write(Value requested, Current current) {
        if (!pathsValid || !codec.Valid(requested) || !current() || !missing(temporary) || !missing(backup) ||
            !readFile(path, original) || !parse(original) || !current()) return false;
        if (original.value == requested) return true;
        std::memcpy(output, original.data, original.size);
        if (!codec.Encode(output + original.begin, original.end - original.begin, requested)) return false;
        if (!writeTemporary(original.size)) return false;
        // Check the complete original immediately before moving it aside. This
        // detects changed comments/other keys as well as another value edit.
        if (!readFile(path, check) || !equal(original, check) || !current() || !missing(backup)) {
            sceIoRemove(temporary); return false;
        }
        if (sceIoRename(path, backup) < 0) { sceIoRemove(temporary); return false; }
        // A writer can change the file between the last comparison and rename;
        // compare the file we actually moved before publishing the replacement.
        if (!readFile(backup, check) || !equal(original, check)) {
            sceIoRename(backup, path);
            sceIoRemove(temporary);
            return false;
        }
        if (!current() || sceIoRename(temporary, path) < 0) {
            // Never remove a concurrently created destination to force rollback.
            // If rollback fails, keep the original in .menu.bak and refuse edits.
            sceIoRename(backup, path);
            sceIoRemove(temporary);
            return false;
        }
        if (!readFile(path, check) || check.size != original.size ||
            std::memcmp(check.data, output, original.size) || !parse(check) || check.value != requested || !current()) return false;
        // Retain the recoverable original if publication/readback was uncertain.
        if (!readFile(backup, check) || !equal(original, check)) return false;
        return sceIoRemove(backup) >= 0;
    }
};
}
