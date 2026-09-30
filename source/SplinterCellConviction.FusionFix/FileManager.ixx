module;

#include <stdafx.h>
#include <fstream>
#include <unordered_map>
#include <numeric>
#include <atomic>
#include <map>

export module FileManager;

import ComVars;
import BlacklistControls;

// File managers are chained, each one forwards what it doesn't handle to the next one (+4):
//   FFileManagerArc (Loc-<lang>.umd) -> FFileManagerArc (DynamicXbox.umd) -> FFileManagerLinear -> FFileManagerWindows (disk)
// FFileManagerArc: an archive with a directory of named files (Loc-<lang>.umd).
// FFileManagerLinear: linear load archives (UMDs\Conviction.umd, UMDs\<map>.umd, DynamicXbox.umd, menu and PEC archives), files are looked up
// by path hash and only the parts that were read when the archive was recorded are streamed, in that order.
// Files read in parts (flag 0x200) come from UMDs\Sparse.<map>.umd through FArchiveSparseLoader.
// Files that exist in the loader's overload folder (update) skip all of them and are read from disk.

struct FString
{
    char* data;
    int32_t count;
    int32_t max;
    int32_t pad;

    std::string_view view() const { return count > 1 ? std::string_view(data, count - 1) : std::string_view(); }
};

template<typename T>
struct TArray
{
    T* data;
    int32_t count;
    int32_t max;
    void* allocator; // null for the default allocator
};

// FArchive virtuals
struct FArchive
{
    void Release() { reinterpret_cast<void(__thiscall*)(FArchive*, int)>((*reinterpret_cast<uintptr_t**>(this))[0])(this, 1); }
    void Serialize(void* data, int32_t size) { reinterpret_cast<void(__thiscall*)(FArchive*, void*, int32_t)>((*reinterpret_cast<uintptr_t**>(this))[8 / 4])(this, data, size); }
    int32_t Tell() { return reinterpret_cast<int32_t(__thiscall*)(FArchive*)>((*reinterpret_cast<uintptr_t**>(this))[52 / 4])(this); }
    int32_t TotalSize() { return reinterpret_cast<int32_t(__thiscall*)(FArchive*)>((*reinterpret_cast<uintptr_t**>(this))[56 / 4])(this); }
    void Seek(int32_t pos) { reinterpret_cast<void(__thiscall*)(FArchive*, int32_t)>((*reinterpret_cast<uintptr_t**>(this))[72 / 4])(this, pos); }
};

// FFileManager virtuals
struct FFileManager
{
    FFileManager* Inner() { return *reinterpret_cast<FFileManager**>(reinterpret_cast<uintptr_t>(this) + 4); } // next file manager in the chain

    FArchive* CreateFileReader(const char* path, int32_t flags, void* error, int32_t bufferSize)
    {
        return reinterpret_cast<FArchive*(__thiscall*)(void*, const char*, int32_t, void*, int32_t)>((*reinterpret_cast<uintptr_t**>(this))[16 / 4])(this, path, flags, error, bufferSize);
    }
};

// FFileManagerArc: +12 archive path, +56 directory, +72 archive reader, +80 reader that positioned the archive reader last
struct FArcEntry
{
    FString name;
    int32_t unk;
    int32_t offset;
    int32_t size;
    int32_t flags;
};

// FFileManagerLinear table: hash map of files (56 byte elements) and the recorded read sequences
struct FLinearEntry
{
    int32_t next;
    uint32_t hash;
    FString name;
    int32_t unk;
    FString alias;
    int32_t unk2;
    int32_t size;
    int32_t used;
};

struct FLinearChunk
{
    int32_t offset;
    int32_t unk;
    int32_t size;
};

struct FLinearSequence
{
    uint32_t hash;
    TArray<FLinearChunk> chunks;
};

struct FLinearTable
{
    TArray<FLinearEntry> entries;
    int32_t hash[2];
    TArray<FLinearSequence> sequences;
    uint8_t rest[68 - 40];
};

namespace FileLoader
{
    std::filesystem::path exePath;
    std::unordered_map<std::string, std::string> packagePaths; // extension -> directory, from [Core.System] Paths= in Conviction.ini
    int* (__stdcall* NormalizeName)(FString* out, const char* path, int32_t packagesByName) = nullptr; // the name linear tables use
    void(__thiscall* FStringDestructor)(FString* string) = nullptr;

    bool starts_with_i(std::string_view str, std::string_view prefix)
    {
        return str.size() >= prefix.size() && _strnicmp(str.data(), prefix.data(), prefix.size()) == 0;
    }

    void ReadPackagePaths()
    {
        std::ifstream ini(exePath / "Conviction.ini");
        std::string line;
        bool coreSystem = false;
        while (std::getline(ini, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (line.starts_with("["))
                coreSystem = _stricmp(line.c_str(), "[Core.System]") == 0;
            else if (coreSystem && starts_with_i(line, "Paths="))
            {
                // Paths=..\..\data\Maps\*.unr
                auto value = line.substr(6);
                auto wildcard = value.rfind("\\*.");
                if (wildcard != std::string::npos)
                {
                    auto ext = value.substr(wildcard + 2);
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    packagePaths.try_emplace(ext, value.substr(0, wildcard));
                }
            }
        }
    }

    // The path (relative to the exe folder) a file stored under an archive name is at on disk
    // Archive and linear names are relative to the data folder (..\..\data), System\ and bare names are in the exe folder,
    // except packages, linear tables store them by file name only and the game finds them through the package paths.
    std::string DiskPathFor(std::string name)
    {
        std::replace(name.begin(), name.end(), '/', '\\');
        while (starts_with_i(name, ".\\"))
            name.erase(0, 2);

        if (starts_with_i(name, "..\\"))
            return name;
        if (starts_with_i(name, "system\\"))
            return name.substr(7);
        if (name.find('\\') != std::string::npos)
            return "..\\..\\data\\" + name;

        auto ext = std::filesystem::path(name).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (auto it = packagePaths.find(ext); it != packagePaths.end())
            return it->second + "\\" + name;
        return name;
    }

    // Requests don't always use the disk path (maps are opened as "maps\menu.unr", packages by any path), the archives find them by
    // the normalized name. Returns the path to open the override with, or nullptr if the file isn't overridden.
    const char* ResolveOverride(const char* path)
    {
        if (!path || !*path || !GetOverloadedFilePathA)
            return nullptr;

        if (bBlacklistControlScheme)
        {
            if (auto blacklistPath = BlacklistControlsOverride(path))
            {
                thread_local std::string overridePath;
                overridePath = std::move(*blacklistPath);
                return overridePath.c_str();
            }
        }

        if (GetOverloadedFilePathA(path, nullptr, 0))
            return path;

        if (!NormalizeName)
            return nullptr;

        FString normalized = {};
        NormalizeName(&normalized, path, 1);
        thread_local std::string diskPath;
        diskPath = DiskPathFor(std::string(normalized.view()));
        FStringDestructor(&normalized);

        if (_stricmp(diskPath.c_str(), path) != 0 && GetOverloadedFilePathA(diskPath.c_str(), nullptr, 0))
            return diskPath.c_str();
        return nullptr;
    }

    FFileManager* Inner(void* fileManager)
    {
        return static_cast<FFileManager*>(fileManager)->Inner();
    }
}

namespace FFileManagerArc
{
    SafetyHookInline shLookup{};
    FArcEntry* __fastcall Lookup(void* fileManager, void* edx, const char* path)
    {
        if (FileLoader::ResolveOverride(path))
            return nullptr;
        return shLookup.fastcall<FArcEntry*>(fileManager, edx, path);
    }

    // Whether a file is available without localization (CFlashMenuFile loads the [%loc%] variant through .locinfo otherwise).
    // Archived files are, which Lookup no longer reports for overridden ones.
    SafetyHookInline shIsAvailable{};
    int32_t __fastcall IsAvailable(void* fileManager, void* edx, const char* path)
    {
        if (FileLoader::ResolveOverride(path))
            return 1;
        return shIsAvailable.fastcall<int32_t>(fileManager, edx, path);
    }
}

namespace FFileManagerLinear
{
    SafetyHookInline shCreateFileReader{};
    FArchive* __fastcall CreateFileReader(void* fileManager, void* edx, const char* path, int32_t flags, void* error, int32_t bufferSize)
    {
        if (auto overridePath = FileLoader::ResolveOverride(path))
            return FileLoader::Inner(fileManager)->CreateFileReader(overridePath, flags, error, bufferSize);
        return shCreateFileReader.fastcall<FArchive*>(fileManager, edx, path, flags, error, bufferSize);
    }

    SafetyHookInline shFileSize{};
    int32_t __fastcall FileSize(void* fileManager, void* edx, const char* path, int32_t flags)
    {
        if (auto overridePath = FileLoader::ResolveOverride(path))
        {
            auto inner = FileLoader::Inner(fileManager);
            return reinterpret_cast<int32_t(__thiscall*)(void*, const char*, int32_t)>((*reinterpret_cast<uintptr_t**>(inner))[24 / 4])(inner, overridePath, flags);
        }
        return shFileSize.fastcall<int32_t>(fileManager, edx, path, flags);
    }

    SafetyHookInline shFileExists{};
    int32_t __fastcall FileExists(void* fileManager, void* edx, const char* path, int32_t* out)
    {
        if (auto overridePath = FileLoader::ResolveOverride(path))
        {
            auto inner = FileLoader::Inner(fileManager);
            return reinterpret_cast<int32_t(__thiscall*)(void*, const char*, int32_t*)>((*reinterpret_cast<uintptr_t**>(inner))[92 / 4])(inner, overridePath, out);
        }
        return shFileExists.fastcall<int32_t>(fileManager, edx, path, out);
    }

    // Linear entries can alias another file
    SafetyHookInline shResolveAlias{};
    const char* __fastcall ResolveAlias(void* fileManager, void* edx, const char* path)
    {
        if (auto overridePath = FileLoader::ResolveOverride(path))
            return overridePath;
        return shResolveAlias.fastcall<const char*>(fileManager, edx, path);
    }
}

namespace Dumper
{
    FFileManager** GFileManager = nullptr;
    void** GFileManagerWindows = nullptr;
    void** GError = nullptr;
    void* (__cdecl* appMalloc)(int32_t size) = nullptr;
    FArchive* (__cdecl* CreatePackReader)(FArchive* reader, int32_t owns, int32_t a3, int32_t a4, int32_t a5) = nullptr; // returns a PACK decompressing reader that owns 'reader', or 'reader' itself
    void(__thiscall* PackReaderPrecache)(FArchive* reader, int32_t size) = nullptr;
    void* (__cdecl* SerializeString)(FArchive* reader, FString* string) = nullptr;
    void* (__cdecl* SerializeArcDirectory)(FArchive* reader, TArray<FArcEntry>* entries) = nullptr;
    void* (__cdecl* SerializeLinearTable)(FArchive* reader, FLinearTable* table) = nullptr;
    void* (__thiscall* LinearTableConstructor)(FLinearTable* table) = nullptr;
    void(__thiscall* LinearTableDestructor)(FLinearTable* table) = nullptr;
    void(__cdecl* appFree)(void* memory) = nullptr;
    void* (__cdecl* SerializeSparseRanges)(FArchive* reader, TArray<FLinearChunk>* ranges) = nullptr;
    FArchive* (__thiscall* MemReaderConstructor)(void* memory, int32_t blockSize) = nullptr;
    void* (__cdecl* CopyToMemReader)(FArchive* reader, FArchive* memReader) = nullptr;
    uintptr_t FFileManagerArcVTable = 0;
    std::atomic<bool> dumping = false; // the main thread doesn't report to the watchdog meanwhile

    std::filesystem::path outputPath;
    std::vector<std::string> log;
    using FileLoader::exePath;
    using FileLoader::starts_with_i;

    // Where the loader looks for an override of a requested path: <overload folder>\<path relative to the exe folder, '..' dropped>
    std::filesystem::path OutputPathFor(std::string_view name)
    {
        auto request = FileLoader::DiskPathFor(std::string(name));
        if (request.empty() || request.find(':') != std::string::npos)
            return {};

        auto absolute = (exePath / request).lexically_normal();
        auto a = absolute.begin();
        for (auto e = exePath.begin(); a != absolute.end() && e != exePath.end() && _wcsicmp(a->c_str(), e->c_str()) == 0; ++e)
            ++a;

        std::filesystem::path relative;
        for (; a != absolute.end(); ++a)
            relative /= *a;
        if (relative.empty())
            return {};
        return outputPath / relative;
    }

    // Output files, several archives can contain parts of the same file (linear and sparse archives only store what was read when they were recorded)
    // Archives can also contain different files under the same name (the PEC archives are character variants, only the selected one is mounted),
    // those can't be loaded from one folder and are left in the archives.
    struct OutputFile
    {
        std::filesystem::path path;
        int64_t size = 0;
        bool writable = false;
        bool conflict = false;
        std::map<int64_t, int64_t> written; // start -> end, merged
        std::vector<std::string> archives;
        bool variant = false;
        std::wstring source; // variant: key of the file in the main folder
    };
    std::string currentArchive;

    // Second pass: every version of the files that differ between archives goes to _conflicts\<archive>\<path>,
    // two different files with the same name in one archive get a ~2 suffix
    bool variantPass = false;
    std::unordered_map<std::wstring, bool> conflictKeys;
    std::unordered_map<std::wstring, OutputFile> variantFiles; // variant path | id
    std::unordered_map<std::wstring, int> variantNames;

    std::wstring ToLower(std::wstring s)
    {
        std::transform(s.begin(), s.end(), s.begin(), ::towlower);
        return s;
    }
    std::unordered_map<std::wstring, OutputFile> outputFiles;
    std::fstream stream;
    OutputFile* streamFile = nullptr;

    OutputFile* GetVariantFile(const std::filesystem::path& path, int64_t size, uint32_t id)
    {
        if (!conflictKeys.contains(ToLower(path.wstring())))
            return nullptr;

        auto variantPath = outputPath / "_conflicts" / std::filesystem::path(currentArchive).stem() / path.lexically_relative(outputPath);
        auto [it, inserted] = variantFiles.try_emplace(ToLower(variantPath.wstring()) + L"|" + std::to_wstring(id));
        auto& file = it->second;
        if (inserted)
        {
            if (auto count = ++variantNames[ToLower(variantPath.wstring())]; count > 1)
                variantPath.replace_filename(variantPath.stem().string() + "~" + std::to_string(count) + variantPath.extension().string());
            std::error_code ec;
            std::filesystem::create_directories(variantPath.parent_path(), ec);
            std::ofstream(variantPath, std::ios::binary).close();
            std::filesystem::resize_file(variantPath, size, ec);
            file.path = variantPath;
            file.size = size;
            file.writable = true;
            file.variant = true;
            file.source = ToLower(path.wstring());
            file.archives.push_back(currentArchive);
        }
        return &file;
    }

    OutputFile* GetOutputFile(std::string_view name, int64_t size, uint32_t id)
    {
        auto path = OutputPathFor(name);
        if (path.empty() || size < 0)
            return nullptr;
        if (variantPass)
            return GetVariantFile(path, size, id);

        auto key = path.wstring();
        std::transform(key.begin(), key.end(), key.begin(), ::towlower);
        auto [it, inserted] = outputFiles.try_emplace(key);
        auto& file = it->second;
        std::error_code ec;
        if (inserted)
        {
            file.path = path;
            file.writable = !std::filesystem::exists(path, ec); // never overwrite what was in the folder before
            if (!file.writable)
                log.push_back(std::format("  kept existing: {}", path.lexically_relative(outputPath).string()));
            else
            {
                std::filesystem::create_directories(path.parent_path(), ec);
                std::ofstream(path, std::ios::binary).close();
            }
        }
        if (file.archives.empty() || file.archives.back() != currentArchive)
            file.archives.push_back(currentArchive);
        if (!file.writable || file.conflict)
            return &file;
        if (inserted)
        {
            file.size = size;
            std::filesystem::resize_file(path, size, ec);
        }
        else if (size != file.size)
            file.conflict = true;
        return &file;
    }

    void Write(OutputFile* file, int64_t offset, const void* data, int64_t size)
    {
        if (!file || !file->writable || file->conflict || offset < 0 || size <= 0 || offset + size > file->size)
            return;
        if (streamFile != file || !stream.is_open())
        {
            stream.close();
            stream.open(file->path, std::ios::in | std::ios::out | std::ios::binary);
            streamFile = file;
        }

        // parts another archive already wrote must be identical
        auto end = offset + size;
        if (!file->variant)
        {
        auto it = file->written.upper_bound(offset);
        if (it != file->written.begin())
            --it;
        std::vector<char> existing;
        for (; it != file->written.end() && it->first < end; ++it)
        {
            auto b = std::max(offset, it->first), e = std::min(end, it->second);
            if (b >= e)
                continue;
            existing.resize(static_cast<size_t>(e - b));
            stream.seekg(b);
            stream.read(existing.data(), existing.size());
            if (memcmp(existing.data(), static_cast<const char*>(data) + (b - offset), existing.size()) != 0)
            {
                file->conflict = true;
                return;
            }
        }
        }

        stream.seekp(offset);
        stream.write(static_cast<const char*>(data), size);

        // merge the range
        auto b = offset, e = end;
        auto m = file->written.upper_bound(b);
        if (m != file->written.begin() && std::prev(m)->second >= b)
            --m;
        while (m != file->written.end() && m->first <= e)
        {
            b = std::min(b, m->first);
            e = std::max(e, m->second);
            m = file->written.erase(m);
        }
        file->written.emplace(b, e);
    }

    // Strings and arrays on the stack are allocated from the game's stack allocator (sub_40BD30 picks it by the address of the array), which is small,
    // so they have to be freed, the destructor frees any array (realloc to zero)
    template<typename T>
    void Free(T& array)
    {
        FileLoader::FStringDestructor(reinterpret_cast<FString*>(&array));
    }

    FArchive* OpenArchive(const char* path, int32_t flags, int32_t a3, int32_t a4, bool pack = true)
    {
        auto windows = reinterpret_cast<FFileManager*>(*GFileManagerWindows);
        auto reader = windows->CreateFileReader(path, flags, *GError, 0x10000);
        if (!reader || !pack)
            return reader;
        auto packReader = CreatePackReader(reader, 1, a3, a4, 0);
        if (packReader && packReader != reader)
            PackReaderPrecache(packReader, 0x7FFFFFFF);
        return packReader;
    }

    void DumpArcEntries(FArchive* reader, const TArray<FArcEntry>& entries, int32_t totalSize, std::string_view archiveName)
    {
        std::vector<uint8_t> buffer;
        for (int32_t i = 0; i < entries.count; i++)
        {
            auto& entry = entries.data[i];
            if (entry.size < 0 || entry.offset < 0 || entry.offset + entry.size > totalSize)
                continue;
            auto file = GetOutputFile(entry.name.view(), entry.size, i);
            if (!file || !file->writable)
                continue;

            buffer.resize(entry.size);
            reader->Seek(entry.offset);
            if (entry.size)
                reader->Serialize(buffer.data(), entry.size);
            Write(file, 0, buffer.data(), buffer.size());
        }
        log.push_back(std::format("{}: {} files", archiveName, entries.count));
    }

    // Archives that are mounted in the chain (the active Loc-<lang>.umd)
    void DumpMountedArchives(std::vector<std::string>& dumped)
    {
        auto windowsVTable = *reinterpret_cast<uintptr_t*>(*GFileManagerWindows);
        for (auto fileManager = *GFileManager; fileManager; fileManager = fileManager->Inner())
        {
            auto vtable = *reinterpret_cast<uintptr_t*>(fileManager);
            if (vtable == windowsVTable)
                break;
            if (vtable != FFileManagerArcVTable)
                continue;

            auto base = reinterpret_cast<uintptr_t>(fileManager);
            auto& name = *reinterpret_cast<FString*>(base + 12);
            auto& entries = *reinterpret_cast<TArray<FArcEntry>*>(base + 56);
            auto reader = *reinterpret_cast<FArchive**>(base + 72);
            if (!reader || !entries.count)
                continue;

            auto fileName = std::filesystem::path(name.view()).filename().string();
            currentArchive = fileName;
            DumpArcEntries(reader, entries, reader->TotalSize(), fileName);
            *reinterpret_cast<void**>(base + 80) = nullptr; // make the next FFileReaderArc read seek again
            dumped.push_back(fileName);
        }
    }

    void DumpArcArchive(FArchive* reader, std::string_view archiveName)
    {
        auto totalSize = reader->TotalSize();
        int32_t footer[5] = {};
        if (totalSize > 20)
        {
            reader->Seek(totalSize - 20);
            reader->Serialize(footer, sizeof(footer));
        }
        auto directoryOffset = footer[1];
        if (directoryOffset <= 0 || directoryOffset >= totalSize - 20)
        {
            log.push_back(std::format("{}: unknown format, skipped", archiveName));
            return;
        }

        TArray<FArcEntry> entries = {};
        reader->Seek(directoryOffset);
        SerializeArcDirectory(reader, &entries);
        DumpArcEntries(reader, entries, totalSize, archiveName);
        for (int32_t i = 0; i < entries.count; i++)
            Free(entries.data[i].name);
        Free(entries);
    }

    // Linear archive: file table (names by hash) and the recorded reads, whose data follows at the next 128 KB boundary in record order, xored with 0xB7
    void DumpLinearArchive(FArchive* reader, std::string_view archiveName)
    {
        auto table = static_cast<FLinearTable*>(appMalloc(sizeof(FLinearTable)));
        LinearTableConstructor(table);
        SerializeLinearTable(reader, table);

        std::unordered_map<uint32_t, FLinearEntry*> entries;
        for (int32_t i = 0; i < table->entries.count; i++)
            entries[table->entries.data[i].hash] = &table->entries.data[i];

        auto position = reader->Tell();
        std::vector<uint8_t> buffer(((position + 0x1FFFF) / 0x20000 << 17) - position);
        if (!buffer.empty())
            reader->Serialize(buffer.data(), static_cast<int32_t>(buffer.size()));

        for (int32_t s = 0; s < table->sequences.count; s++)
        {
            auto& sequence = table->sequences.data[s];
            auto it = entries.find(sequence.hash);
            auto file = it != entries.end() ? GetOutputFile(it->second->name.view(), it->second->size, sequence.hash) : nullptr;
            for (int32_t c = 0; c < sequence.chunks.count; c++)
            {
                auto& chunk = sequence.chunks.data[c];
                buffer.resize(chunk.size);
                if (chunk.size)
                    reader->Serialize(buffer.data(), chunk.size);
                for (auto& b : buffer)
                    b ^= 0xB7;
                Write(file, chunk.offset, buffer.data(), buffer.size());
            }
        }
        log.push_back(std::format("{}: {} files", archiveName, table->entries.count));
        LinearTableDestructor(table);
        appFree(table);
    }

    // Sparse archive: files that were read in parts (FArchiveSparseLoader), each one is file size, the ranges that were read (offset, size) and their data
    void DumpSparseArchive(FArchive* reader, std::string_view archiveName)
    {
        int32_t count = 0;
        reader->Serialize(&count, sizeof(count));
        if (count <= 0 || count > 100000)
        {
            log.push_back(std::format("{}: unknown format, skipped", archiveName));
            return;
        }

        std::vector<uint8_t> buffer;
        for (int32_t i = 0; i < count; i++)
        {
            FString name = {};
            int32_t size = 0;
            SerializeString(reader, &name);
            reader->Serialize(&size, sizeof(size));

            auto memReader = MemReaderConstructor(appMalloc(88), 4096);
            reinterpret_cast<void(__thiscall*)(FArchive*, int32_t)>((*reinterpret_cast<uintptr_t**>(memReader))[120 / 4])(memReader, size);
            CopyToMemReader(reader, memReader);
            auto sparse = CreatePackReader(memReader, 1, 0, 0, 0);

            int32_t fileSize = 0;
            TArray<FLinearChunk> ranges = {};
            sparse->Seek(0);
            sparse->Serialize(&fileSize, sizeof(fileSize));
            SerializeSparseRanges(sparse, &ranges);

            auto file = GetOutputFile(name.view(), fileSize, i);
            for (int32_t r = 0; r < ranges.count; r++)
            {
                auto offset = ranges.data[r].offset;
                auto rangeSize = ranges.data[r].unk; // elements are serialized as offset, size
                buffer.resize(std::max(rangeSize, 0));
                if (rangeSize > 0)
                    sparse->Serialize(buffer.data(), rangeSize);
                Write(file, offset, buffer.data(), buffer.size());
            }
            sparse->Release();
            Free(ranges);
            Free(name);
        }
        log.push_back(std::format("{}: {} files", archiveName, count));
    }

    void DumpArchive(const std::filesystem::path& relativePath)
    {
        auto path = relativePath.string();
        currentArchive = path;
        auto name = relativePath.filename().string();

        if (starts_with_i(name, "sparse."))
        {
            if (auto reader = OpenArchive(path.c_str(), 64, 0, 0, false))
            {
                DumpSparseArchive(reader, path);
                reader->Release();
            }
            return;
        }

        // linear tables start with the "PC_CODE_455" version FString, opened like FFileManagerLinear does
        if (auto reader = OpenArchive(path.c_str(), 0x440, 0, 0))
        {
            char header[12] = {};
            if (reader->TotalSize() > static_cast<int32_t>(sizeof(header)))
                reader->Serialize(header, sizeof(header));
            if (header[0] == 12 && std::string_view(header + 1, 7) == "PC_CODE")
            {
                reader->Seek(0);
                DumpLinearArchive(reader, path);
                reader->Release();
                return;
            }
            reader->Release();
        }

        // named archive, opened like FFileManagerArc does
        if (auto reader = OpenArchive(path.c_str(), 0, 0x80000, 0x80000))
        {
            DumpArcArchive(reader, path);
            reader->Release();
        }
    }

    void Dump()
    {
        dumping = true;
        outputPath = exePath / "unpacked";
        std::error_code ec;
        std::filesystem::create_directories(outputPath, ec);

        std::vector<std::string> dumped;
        DumpMountedArchives(dumped);

        // everything else in the exe folder and UMDs: Conviction.umd, per map linear and sparse archives, menu and PEC archives.
        // Loc-<lang>.umd of other languages would overwrite each other, only the mounted one is dumped.
        std::vector<std::filesystem::path> archives;
        for (auto& dir : { std::filesystem::path("UMDs"), std::filesystem::path() })
        {
            for (auto& entry : std::filesystem::directory_iterator(exePath / dir, ec))
            {
                auto name = entry.path().filename().string();
                if (!entry.is_regular_file(ec) || _stricmp(entry.path().extension().string().c_str(), ".umd") != 0 || starts_with_i(name, "loc-"))
                    continue;
                if (std::any_of(dumped.begin(), dumped.end(), [&](auto& d) { return _stricmp(d.c_str(), name.c_str()) == 0; }))
                    continue;
                archives.push_back(dir / name);
            }
        }

        for (auto& archive : archives)
            DumpArchive(archive);
        stream.close();

        // every version of the files that differ between archives
        std::vector<std::string> variantArchives;
        for (auto& [key, file] : outputFiles)
        {
            if (!file.writable || !file.conflict)
                continue;
            conflictKeys[key] = true;
            for (auto& archive : file.archives)
                if (std::find(variantArchives.begin(), variantArchives.end(), archive) == variantArchives.end())
                    variantArchives.push_back(archive);
        }
        variantPass = true;
        for (auto& archive : variantArchives)
            DumpArchive(archive);
        variantPass = false;
        stream.close();

        // report files that differ between archives (removed) and files that are not complete (parts no archive contains are zero)
        size_t written = 0, incomplete = 0, conflicts = 0;
        std::vector<std::string> incompleteLog;
        std::map<std::string, std::vector<std::string>> conflictLog; // file -> its line and the versions in _conflicts
        std::unordered_map<std::wstring, std::vector<std::string>> versions;
        for (auto& [key, file] : variantFiles)
        {
            int64_t covered = 0;
            for (auto [b, e] : file.written)
                covered += e - b;
            versions[file.source].push_back(std::format("    {} ({} of {} bytes)", file.path.lexically_relative(outputPath).string(), covered, file.size));
        }
        for (auto& [key, file] : outputFiles)
        {
            if (!file.writable)
                continue;
            auto relative = file.path.lexically_relative(outputPath).string();
            if (file.conflict)
            {
                conflicts++;
                std::filesystem::remove(file.path, ec);
                auto& lines = conflictLog[relative];
                lines.push_back(std::format("  {} ({})", relative, std::accumulate(file.archives.begin(), file.archives.end(), std::string(), [](auto a, auto& b) { return a.empty() ? b : a + ", " + b; })));
                auto& fileVersions = versions[key];
                std::sort(fileVersions.begin(), fileVersions.end());
                lines.insert(lines.end(), fileVersions.begin(), fileVersions.end());
                continue;
            }
            written++;
            int64_t covered = 0;
            for (auto [b, e] : file.written)
                covered += e - b;
            if (covered < file.size)
            {
                // not loadable, the game can read parts no archive has (they're zero), kept for reference where no path points to
                incomplete++;
                incompleteLog.push_back(std::format("  {} ({} of {} bytes)", relative, covered, file.size));
                auto target = outputPath / "_incomplete" / relative;
                std::filesystem::create_directories(target.parent_path(), ec);
                std::filesystem::rename(file.path, target, ec);
            }
        }
        std::sort(incompleteLog.begin(), incompleteLog.end());
        log.push_back(std::format("\n{} files differ between archives and were left in them, every version of them is in _conflicts:", conflicts));
        for (auto& [relative, lines] : conflictLog)
            log.insert(log.end(), lines.begin(), lines.end());
        log.push_back(std::format("\n{} files written, {} of them are not complete and were moved to _incomplete (the game can read parts no archive contains, they would be zero):", written, incomplete));
        log.insert(log.end(), incompleteLog.begin(), incompleteLog.end());

        std::ofstream(outputPath / "_dump.log") << std::accumulate(log.begin(), log.end(), std::string(), [](auto a, auto& b) { return a + b + "\n"; });
        MessageBoxA(nullptr, std::format("Dumped {} files to\n{}\n\n{} files are not complete and were moved to _incomplete, {} differ between archives and were left in them (their versions are in _conflicts).\n"
            "Rename the folder to 'update' to load the files from disk instead of the archives.\nSee _dump.log for details.",
            written - incomplete, outputPath.string(), incomplete, conflicts).c_str(), "Splinter Cell Conviction FusionFix", MB_OK | MB_ICONINFORMATION);
        dumping = false;
    }
}

export void InitFileManager()
{
    CIniReader iniReader("");
    auto bDumpPackedFiles = iniReader.ReadInteger("FILELOADER", "DumpPackedFiles", 0) != 0;

    ModuleList dlls;
    dlls.Enumerate(ModuleList::SearchLocation::LocalOnly);
    for (auto& e : dlls.m_moduleList)
    {
        auto m = std::get<HMODULE>(e);
        if (IsModuleUAL(m))
        {
            GetOverloadedFilePathA = (decltype(GetOverloadedFilePathA))GetProcAddress(m, "GetOverloadedFilePathA");
            break;
        }
    }

    FileLoader::exePath = GetExeModulePath().lexically_normal();
    if (!FileLoader::exePath.has_filename())
        FileLoader::exePath = FileLoader::exePath.parent_path();
    FileLoader::ReadPackagePaths();

    // Load files from the loader's overload folder instead of the archives
    if (GetOverloadedFilePathA)
    {
        auto pattern = hook::pattern("55 8B EC 83 EC 3C 56 33 F6 39 75 10 74 2A FF 75 0C E8");
        FileLoader::NormalizeName = (decltype(FileLoader::NormalizeName))pattern.get_first();

        pattern = hook::pattern("56 8B F1 33 C0 39 46 08 89 46 04 74 0A 6A 01 89 46 08 E8");
        FileLoader::FStringDestructor = (decltype(FileLoader::FStringDestructor))pattern.get_first();

        pattern = hook::pattern("55 8B EC 83 EC 18 53 56 57 FF 75 08 8D 45 E8");
        FFileManagerArc::shLookup = safetyhook::create_inline(pattern.get_first(), FFileManagerArc::Lookup);

        pattern = hook::pattern("56 FF 74 24 08 8B F1 E8 ? ? ? ? 85 C0 74 07 33 C0 40 5E C2 04 00");
        FFileManagerArc::shIsAvailable = safetyhook::create_inline(pattern.get_first(), FFileManagerArc::IsAvailable);

        pattern = hook::pattern("55 8B EC 83 EC 30 53 56 57 FF 75 08 8B F1 8D 4D E4 33 DB 33 FF E8");
        FFileManagerLinear::shCreateFileReader = safetyhook::create_inline(pattern.get_first(), FFileManagerLinear::CreateFileReader);

        pattern = hook::pattern("55 8B EC 83 EC 18 83 3D ? ? ? ? ? 56 8B F1 75 5C");
        FFileManagerLinear::shFileSize = safetyhook::create_inline(pattern.get_first(), FFileManagerLinear::FileSize);

        pattern = hook::pattern("55 8B EC 83 EC 18 83 3D ? ? ? ? ? 56 8B F1 75 66");
        FFileManagerLinear::shFileExists = safetyhook::create_inline(pattern.get_first(), FFileManagerLinear::FileExists);

        pattern = hook::pattern("55 8B EC 83 EC 18 56 8B F1 8B 86 8C 01 00 00 8D 44 86 2C 83 38 00 57 74");
        FFileManagerLinear::shResolveAlias = safetyhook::create_inline(pattern.get_first(), FFileManagerLinear::ResolveAlias);
    }

    // Dump the contents of all archives to 'unpacked', once the file managers are set up (after appInit)
    if (bDumpPackedFiles)
    {
        auto pattern = hook::pattern("A1 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? 50 68");
        Dumper::GFileManagerWindows = *pattern.get_first<void**>(1);

        pattern = hook::pattern("68 00 00 01 00 FF 35 ? ? ? ? 53 50 FF 52 10 8B D8");
        Dumper::GError = *pattern.get_first<void**>(7);

        pattern = hook::pattern("6A 00 B8 00 00 08 00 50 50 6A 01 53 E8");
        Dumper::CreatePackReader = (decltype(Dumper::CreatePackReader))injector::GetBranchDestination(pattern.get_first(12)).as_int();

        pattern = hook::pattern("68 FF FF FF 7F 8B C8 89 46 4C E8");
        Dumper::PackReaderPrecache = (decltype(Dumper::PackReaderPrecache))injector::GetBranchDestination(pattern.get_first(10)).as_int();

        pattern = hook::pattern("89 5D E4 E8 ? ? ? ? 39 58 30");
        Dumper::SerializeString = (decltype(Dumper::SerializeString))injector::GetBranchDestination(pattern.get_first(3)).as_int();

        pattern = hook::pattern("68 00 10 00 00 8B C8 E8 ? ? ? ? 8B F8 EB 02 33 FF FF 75 F4");
        Dumper::MemReaderConstructor = (decltype(Dumper::MemReaderConstructor))injector::GetBranchDestination(pattern.get_first(7)).as_int();

        pattern = hook::pattern("FF 50 78 57 56 E8 ? ? ? ? 53 53 53 6A 01 57 E8");
        Dumper::CopyToMemReader = (decltype(Dumper::CopyToMemReader))injector::GetBranchDestination(pattern.get_first(5)).as_int();

        pattern = hook::pattern("8D 7B 3C 57 56 E8");
        Dumper::SerializeSparseRanges = (decltype(Dumper::SerializeSparseRanges))injector::GetBranchDestination(pattern.get_first(5)).as_int();

        pattern = hook::pattern("8D 46 38 50 FF 76 48 E8");
        Dumper::SerializeArcDirectory = (decltype(Dumper::SerializeArcDirectory))injector::GetBranchDestination(pattern.get_first(7)).as_int();

        pattern = hook::pattern("55 8B EC 83 EC 14 53 56 8B 75 0C 57 8B CE E8");
        Dumper::SerializeLinearTable = (decltype(Dumper::SerializeLinearTable))pattern.get_first();

        pattern = hook::pattern("E8 ? ? ? ? 3B C7 59 74 09 8B C8 E8 ? ? ? ? EB 02 33");
        Dumper::appMalloc = (decltype(Dumper::appMalloc))injector::GetBranchDestination(pattern.get_first(0)).as_int();
        Dumper::LinearTableConstructor = (decltype(Dumper::LinearTableConstructor))injector::GetBranchDestination(pattern.get_first(12)).as_int();

        pattern = hook::pattern("3B DF 74 0E 8B CB E8 ? ? ? ? 53 E8 ? ? ? ? 59 8B 86 8C 01");
        Dumper::LinearTableDestructor = (decltype(Dumper::LinearTableDestructor))injector::GetBranchDestination(pattern.get_first(6)).as_int();
        Dumper::appFree = (decltype(Dumper::appFree))injector::GetBranchDestination(pattern.get_first(12)).as_int();

        // FFileManagerArc constructor
        pattern = hook::pattern("FF 74 24 0C 8D 4E 0C C7 06 ? ? ? ? E8 ? ? ? ? 8D 4E 24 E8");
        Dumper::FFileManagerArcVTable = *pattern.get_first<uintptr_t>(9);

        // GFileManager->Mount("UMDs\Conviction.umd")
        pattern = hook::pattern("8B 0D ? ? ? ? 8B 01 74 07 68");
        Dumper::GFileManager = *pattern.get_first<FFileManager**>(2);

        // mg::common::WatchDog crashes the game when a registered thread (the main thread) doesn't report in time, which it can't while dumping.
        // Its check (every second): if (GetTickCount() - thread.lastReport >= thread.timeout) crash, mark the threads as reported instead.
        pattern = hook::pattern("FF 15 ? ? ? ? 8B C8 2B 4F 04 3B 0F 72 07");
        static auto WatchDogCheck = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
        {
            if (Dumper::dumping)
                *reinterpret_cast<uint32_t*>(regs.edi + 4) = regs.eax;
        });

        // appInit(..., GFileManager, ...) in the startup code, the file manager chain is complete once it returns
        pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 50 8D 45 24 50 E8");
        static auto AfterAppInit = safetyhook::create_mid(pattern.get_first(10), [](SafetyHookContext& regs)
        {
            static std::once_flag flag;
            std::call_once(flag, []() { Dumper::Dump(); });
        });
    }
}
