module;

#include <stdafx.h>
#include <fstream>
#include <unordered_map>
#include <numeric>
#include <map>
#include <commctrl.h>
#include <winioctl.h>
#include <atomic>
#include <mutex>
#include <thread>

export module FileManager;

import ComVars;

// A task dialog with a progress bar and a Cancel button, shown on its own thread while the caller does the work.
// Games don't have a comctl32 v6 manifest, the dialog thread loads it through an activation context (shell32's manifest, resource 124).
//
//     ProgressDialog dialog(L"Game", L"Dumping archives...");
//     for (...) { if (dialog.Cancelled()) break; dialog.Set(done, total, L"file.umd"); ... }
//     dialog.Close();
class ProgressDialog
{
    std::wstring title, heading;
    std::thread thread;
    std::mutex mutex;
    std::wstring text;
    std::atomic<uint64_t> done = 0, total = 1;
    std::atomic<bool> cancelled = false, closing = false;
    std::atomic<HWND> window = nullptr;

    static HRESULT CALLBACK Callback(HWND hwnd, UINT message, WPARAM wParam, LPARAM, LONG_PTR data)
    {
        auto self = reinterpret_cast<ProgressDialog*>(data);
        switch (message)
        {
        case TDN_CREATED:
            self->window = hwnd;
            SendMessageW(hwnd, TDM_SET_PROGRESS_BAR_RANGE, 0, MAKELPARAM(0, 1000));
            break;
        case TDN_TIMER:
        {
            auto total = std::max<uint64_t>(self->total, 1);
            auto position = static_cast<WPARAM>(std::min<uint64_t>(self->done * 1000 / total, 1000));
            SendMessageW(hwnd, TDM_SET_PROGRESS_BAR_POS, position, 0);
            std::wstring text;
            {
                std::lock_guard lock(self->mutex);
                text = self->text;
            }
            SendMessageW(hwnd, TDM_SET_ELEMENT_TEXT, TDE_CONTENT, reinterpret_cast<LPARAM>(text.c_str()));
            if (self->closing)
                SendMessageW(hwnd, TDM_CLICK_BUTTON, IDOK, 0);
            break;
        }
        case TDN_BUTTON_CLICKED:
            if (wParam == IDCANCEL && !self->closing)
            {
                // stays open until the work stops
                self->cancelled = true;
                SendMessageW(hwnd, TDM_ENABLE_BUTTON, IDCANCEL, FALSE);
                std::lock_guard lock(self->mutex);
                self->text = L"Cancelling...";
                return S_FALSE;
            }
            break;
        }
        return S_OK;
    }

    void Run()
    {
        // comctl32 v6
        wchar_t shell32[MAX_PATH] = {};
        GetSystemDirectoryW(shell32, MAX_PATH);
        wcscat_s(shell32, L"\\shell32.dll");
        ACTCTXW context = { sizeof(context) };
        context.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
        context.lpSource = shell32;
        context.lpResourceName = MAKEINTRESOURCEW(124);
        auto activation = CreateActCtxW(&context);
        ULONG_PTR cookie = 0;
        if (activation != INVALID_HANDLE_VALUE)
            ActivateActCtx(activation, &cookie);

        using TaskDialogIndirectFn = HRESULT(WINAPI*)(const TASKDIALOGCONFIG*, int*, int*, BOOL*);
        auto comctl32 = LoadLibraryW(L"comctl32.dll");
        auto taskDialogIndirect = comctl32 ? reinterpret_cast<TaskDialogIndirectFn>(GetProcAddress(comctl32, "TaskDialogIndirect")) : nullptr;
        if (taskDialogIndirect)
        {
            TASKDIALOGCONFIG config = { sizeof(config) };
            config.dwFlags = TDF_SHOW_PROGRESS_BAR | TDF_CALLBACK_TIMER | TDF_SIZE_TO_CONTENT | TDF_POSITION_RELATIVE_TO_WINDOW;
            config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
            config.pszWindowTitle = title.c_str();
            config.pszMainInstruction = heading.c_str();
            config.pszContent = L" ";
            config.pfCallback = Callback;
            config.lpCallbackData = reinterpret_cast<LONG_PTR>(this);
            config.cxWidth = 300;
            taskDialogIndirect(&config, nullptr, nullptr, nullptr);
        }
        else
        {
            // no dialog, just wait for the work
            while (!closing)
                Sleep(100);
        }

        if (activation != INVALID_HANDLE_VALUE)
        {
            DeactivateActCtx(0, cookie);
            ReleaseActCtx(activation);
        }
    }

public:
    ProgressDialog(std::wstring title, std::wstring heading) : title(std::move(title)), heading(std::move(heading))
    {
        thread = std::thread([this]() { Run(); });
    }

    ~ProgressDialog()
    {
        Close();
    }

    void Set(uint64_t done, uint64_t total, std::wstring text = {})
    {
        this->total = total;
        this->done = done;
        if (!text.empty())
        {
            std::lock_guard lock(mutex);
            if (!cancelled)
                this->text = std::move(text);
        }
    }

    bool Cancelled() const
    {
        return cancelled;
    }

    // the window, for message boxes shown after it
    HWND Window() const
    {
        return window;
    }

    void Close()
    {
        if (!thread.joinable())
            return;
        closing = true;
        thread.join();
    }
};

// File managers are chained, each one forwards what it doesn't handle to the next one (+4):
//   FFileManagerArc (loc-<lang>.umd) -> FFileManagerArc (dynamicflash.umd) -> FFileManagerArc (dynamicwin.umd) -> FFileManagerLinear -> FFileManagerWindows (disk)
// FFileManagerArc: an archive with a directory of named files.
// FFileManagerLinear: linear load archives (UMDs\Blacklist.umd, UMDs\<map>.umd, menu, lobby and PEC archives), files are looked up by path hash
// and only the parts that were read when the archive was recorded are streamed, in that order.
// The texture streams next to them (UMDs\<map>[_m].ass, .asstrm) are read directly, they aren't file archives.
// Files that exist in the loader's overload folder (update) skip all of them and are read from disk.

struct FString
{
    char* data;
    int32_t count;
    int32_t max;

    std::string_view view() const { return data && count > 1 ? std::string_view(data, count - 1) : std::string_view(); }
};

// FArchive virtuals
struct FArchive
{
    uintptr_t* vtable() { return *reinterpret_cast<uintptr_t**>(this); }
    void Release() { reinterpret_cast<void(__thiscall*)(FArchive*, int)>(vtable()[0])(this, 1); }
    void Serialize(void* data, int32_t size) { reinterpret_cast<void(__thiscall*)(FArchive*, void*, int32_t)>(vtable()[0x0C / 4])(this, data, size); }
    int32_t Tell() { return reinterpret_cast<int32_t(__thiscall*)(FArchive*)>(vtable()[0x44 / 4])(this); }
    int32_t TotalSize() { return reinterpret_cast<int32_t(__thiscall*)(FArchive*)>(vtable()[0x48 / 4])(this); }
    void Seek(int32_t pos) { reinterpret_cast<void(__thiscall*)(FArchive*, int32_t)>(vtable()[0x50 / 4])(this, pos); }
};

// FFileManager virtuals
struct FFileManager
{
    uintptr_t* vtable() { return *reinterpret_cast<uintptr_t**>(this); }
    FFileManager* Inner() { return *reinterpret_cast<FFileManager**>(reinterpret_cast<uintptr_t>(this) + 4); } // next file manager in the chain

    FArchive* CreateFileReader(const char* path, int32_t flags, void* error, int32_t bufferSize)
    {
        return reinterpret_cast<FArchive*(__thiscall*)(void*, const char*, int32_t, void*, int32_t)>(vtable()[0x10 / 4])(this, path, flags, error, bufferSize);
    }
};

namespace FileLoader
{
    std::filesystem::path exePath;
    std::unordered_map<std::string, std::string> packagePaths; // extension -> directory, from [Core.System] Paths= in Blacklist.ini
    FString* (__stdcall* NormalizeName)(FString* out, const char* path, int32_t packagesByName) = nullptr; // the name linear tables use
    void(__thiscall* FStringDestructor)(FString* string) = nullptr;

    bool starts_with_i(std::string_view str, std::string_view prefix)
    {
        return str.size() >= prefix.size() && _strnicmp(str.data(), prefix.data(), prefix.size()) == 0;
    }

    void ReadPackagePaths()
    {
        std::ifstream ini(exePath / "Blacklist.ini");
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
    // Names under data\ are relative to the game folder (..\..\), System\ and bare names are in the exe folder,
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
        if (starts_with_i(name, "data\\"))
            return "..\\..\\" + name;

        auto ext = std::filesystem::path(name).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (name.find('\\') == std::string::npos)
        {
            if (auto it = packagePaths.find(ext); it != packagePaths.end())
                return it->second + "\\" + name;
        }
        return name;
    }

    // Requests don't always use the disk path (maps are opened as "maps\x.unr", packages by any path), the archives find them by
    // the normalized name. Returns the path to open the override with, or nullptr if the file isn't overridden.
    const char* ResolveOverride(const char* path)
    {
        if (!path || !*path || !GetOverloadedFilePathA)
            return nullptr;

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
    void* __fastcall Lookup(void* fileManager, void* edx, const char* path)
    {
        if (FileLoader::ResolveOverride(path))
            return nullptr;
        return shLookup.fastcall<void*>(fileManager, edx, path);
    }

    // Whether a file is available (virtual A0h): archived files are, which Lookup no longer reports for overridden ones
    SafetyHookInline shIsAvailable{};
    int32_t __fastcall IsAvailable(void* fileManager, void* edx, const char* path)
    {
        if (FileLoader::ResolveOverride(path))
            return 1;
        return shIsAvailable.fastcall<int32_t>(fileManager, edx, path);
    }
}

// FFileManagerLinear virtuals pass overridden files to the next file manager in the chain, with the path the override is at
namespace FFileManagerLinear
{
    template<size_t Offset, typename R, typename... Args>
    R CallInner(void* fileManager, const char* path, Args... args)
    {
        auto inner = FileLoader::Inner(fileManager);
        return reinterpret_cast<R(__thiscall*)(void*, const char*, Args...)>(inner->vtable()[Offset / 4])(inner, path, args...);
    }

    // A linear archive streams its recorded reads in order (active table: manager +2Ch + 4 * [+14Ch]; reads at +18h, 16 bytes each starting
    // with the path hash, count +1Ch, current one +2Ch, +44h seekable). A reader (path hash +10h) skips the stream forward to its file, a file
    // read again or out of the recorded order (a mission that loads another map changes which score files are read at startup) is only
    // behind it: the reader skips to the end and waits for the stream forever. Such a file is read from disk when it's there.
    uintptr_t LinearReaderVTable = 0;

    // the archive each table was mounted from, a file behind the stream is rebuilt from it (Dumper::ExtractLinearFile)
    std::mutex tablesMutex;
    std::unordered_map<void*, std::string> tableArchives;
    std::string(*ExtractLinearFile)(const std::string& archive, uint32_t hash, const char* path) = nullptr;

    void* ActiveTable(void* fileManager)
    {
        auto manager = static_cast<uint8_t*>(fileManager);
        return *reinterpret_cast<void**>(manager + 0x2C + 4 * *reinterpret_cast<int32_t*>(manager + 0x14C));
    }

    // FFileManagerLinear::Mount(path, ...) (virtual 88h) pushes a table for the archive, Mount(nullptr) pops it
    SafetyHookInline shMount{};
    void* __fastcall Mount(void* fileManager, void* edx, const char* path, int32_t a3, int32_t a4)
    {
        auto result = shMount.fastcall<void*>(fileManager, edx, path, a3, a4);
        if (path && *path)
        {
            if (auto table = ActiveTable(fileManager))
            {
                std::lock_guard lock(tablesMutex);
                tableArchives[table] = path;
            }
        }
        return result;
    }

    bool IsBehindStream(void* fileManager, uint32_t hash)
    {
        auto manager = static_cast<uint8_t*>(fileManager);
        auto table = static_cast<uint8_t*>(ActiveTable(manager));
        if (!table || *reinterpret_cast<int32_t*>(table + 0x44) != 0)
            return false;
        auto reads = *reinterpret_cast<uint8_t**>(table + 0x18);
        auto count = *reinterpret_cast<int32_t*>(table + 0x1C);
        for (auto i = std::max(*reinterpret_cast<int32_t*>(table + 0x2C), 0); i < count; i++)
        {
            if (*reinterpret_cast<uint32_t*>(reads + 16 * i) == hash)
                return false;
        }
        return true;
    }

    SafetyHookInline shCreateFileReader{};
    FArchive* __fastcall CreateFileReader(void* fileManager, void* edx, const char* path, int32_t flags, void* error, int32_t bufferSize)
    {
        if (auto overridePath = FileLoader::ResolveOverride(path))
            return CallInner<0x10, FArchive*>(fileManager, overridePath, flags, error, bufferSize);

        auto reader = shCreateFileReader.fastcall<FArchive*>(fileManager, edx, path, flags, error, bufferSize);
        if (!reader || !LinearReaderVTable || *reinterpret_cast<uintptr_t*>(reader) != LinearReaderVTable)
            return reader;

        auto hash = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(reader) + 0x10);
        if (!IsBehindStream(fileManager, hash))
            return reader;

        // the file rebuilt from its archive, or a copy on disk
        std::string archive;
        {
            std::lock_guard lock(tablesMutex);
            if (auto it = tableArchives.find(ActiveTable(fileManager)); it != tableArchives.end())
                archive = it->second;
        }
        auto rebuilt = !archive.empty() && ExtractLinearFile ? ExtractLinearFile(archive, hash, path) : std::string();
        auto diskReader = CallInner<0x10, FArchive*>(fileManager, rebuilt.empty() ? path : rebuilt.c_str(), flags, error, bufferSize);
        if (!diskReader)
            return reader;
        reader->Release();
        return diskReader;
    }

    SafetyHookInline shFileSize{};
    int32_t __fastcall FileSize(void* fileManager, void* edx, const char* path, int32_t flags)
    {
        if (auto overridePath = FileLoader::ResolveOverride(path))
            return CallInner<0x18, int32_t>(fileManager, overridePath, flags);
        return shFileSize.fastcall<int32_t>(fileManager, edx, path, flags);
    }

    SafetyHookInline shFileExists{};
    int32_t __fastcall FileExists(void* fileManager, void* edx, const char* path, int32_t* out)
    {
        if (auto overridePath = FileLoader::ResolveOverride(path))
            return CallInner<0x5C, int32_t>(fileManager, overridePath, out);
        return shFileExists.fastcall<int32_t>(fileManager, edx, path, out);
    }

    // virtual 8Ch, forwarded with the same arguments
    SafetyHookInline shFileTime{};
    int32_t __fastcall FileTime(void* fileManager, void* edx, const char* path, int32_t a3)
    {
        if (auto overridePath = FileLoader::ResolveOverride(path))
            return CallInner<0x8C, int32_t>(fileManager, overridePath, a3);
        return shFileTime.fastcall<int32_t>(fileManager, edx, path, a3);
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
    void** GError = nullptr;
    FArchive* (__cdecl* CreatePackReader)(FArchive* reader, int32_t owns, void* a3, void* a4, int32_t a5) = nullptr; // returns a PACK decompressing reader that owns 'reader', or 'reader' itself
    void(__thiscall* PackReaderPrecache)(FArchive* reader, int32_t size) = nullptr;
    uintptr_t FFileManagerArcVTable = 0;

    std::filesystem::path outputPath;
    std::vector<std::string> log;
    using FileLoader::exePath;
    using FileLoader::starts_with_i;

    // Reading the table formats: integers are compact indices, strings a compact length (negative for UTF-16) and the characters with a null
    int32_t ReadCompact(FArchive* reader)
    {
        uint8_t b = 0;
        reader->Serialize(&b, 1);
        auto negative = (b & 0x80) != 0;
        int32_t value = b & 0x3F;
        if (b & 0x40)
        {
            for (int shift = 6; shift < 32; shift += 7)
            {
                reader->Serialize(&b, 1);
                value |= (b & 0x7F) << shift;
                if ((b & 0x80) == 0)
                    break;
            }
        }
        return negative ? -value : value;
    }

    int32_t ReadInt(FArchive* reader)
    {
        int32_t value = 0;
        reader->Serialize(&value, 4);
        return value;
    }

    bool ReadString(FArchive* reader, std::string& out)
    {
        auto length = ReadCompact(reader);
        out.clear();
        if (length < -0x10000 || length > 0x10000 || reader->Tell() + std::abs(length) * (length < 0 ? 2 : 1) > reader->TotalSize())
            return false;
        if (length < 0)
        {
            std::wstring wide(-length, L'\0');
            reader->Serialize(wide.data(), -length * 2);
            for (auto c : wide)
                out.push_back(c < 0x100 ? static_cast<char>(c) : '_');
        }
        else
        {
            out.resize(length);
            if (length)
                reader->Serialize(out.data(), length);
        }
        while (!out.empty() && out.back() == '\0')
            out.pop_back();
        return true;
    }

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

    // Output files, several archives can contain parts of the same file (linear archives only store what was read when they were recorded).
    // Archives can also contain different files under the same name, those can't be loaded from one folder and are left in the archives.
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
    // Output files are sparse, the parts no archive contains take no space
    void CreateSparseFile(const std::filesystem::path& path, int64_t size)
    {
        auto file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
            return;
        DWORD bytes = 0;
        DeviceIoControl(file, FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &bytes, nullptr);
        LARGE_INTEGER end;
        end.QuadPart = size;
        SetFilePointerEx(file, end, nullptr, FILE_BEGIN);
        SetEndOfFile(file);
        CloseHandle(file);
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
            CreateSparseFile(variantPath, size);
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

        auto key = ToLower(path.wstring());
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
                std::filesystem::create_directories(path.parent_path(), ec);
        }
        if (file.archives.empty() || file.archives.back() != currentArchive)
            file.archives.push_back(currentArchive);
        if (!file.writable || file.conflict)
            return &file;
        if (inserted)
        {
            file.size = size;
            CreateSparseFile(path, size);
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

    FArchive* OpenArchive(const char* path, int32_t flags)
    {
        // the last file manager in the chain is FFileManagerWindows
        auto windows = *GFileManager;
        while (windows && windows->Inner())
            windows = windows->Inner();
        auto reader = windows ? windows->CreateFileReader(path, flags, *GError, 0x10000) : nullptr;
        if (!reader)
            return nullptr;
        return CreatePackReader(reader, 1, nullptr, nullptr, 0);
    }

    // Named archive: a footer (5 ints, the second one is the directory offset) and the directory: count, then name, unk, offset, size, flags
    bool DumpArcArchive(FArchive* reader, std::string_view archiveName)
    {
        auto totalSize = reader->TotalSize();
        if (totalSize <= 20)
            return false;
        int32_t footer[5] = {};
        reader->Seek(totalSize - 20);
        reader->Serialize(footer, sizeof(footer));
        auto directoryOffset = footer[1];
        if (directoryOffset <= 0 || directoryOffset >= totalSize - 20)
            return false;

        struct Entry { std::string name; int32_t offset, size; };
        std::vector<Entry> entries;
        reader->Seek(directoryOffset);
        auto count = ReadCompact(reader);
        if (count < 0 || count > 1000000)
            return false;
        for (int32_t i = 0; i < count; i++)
        {
            Entry entry;
            if (!ReadString(reader, entry.name))
                return false;
            int32_t values[4] = {};
            reader->Serialize(values, sizeof(values));
            entry.offset = values[0];
            entry.size = values[1];
            entries.push_back(std::move(entry));
        }

        std::vector<uint8_t> buffer;
        for (int32_t i = 0; i < count; i++)
        {
            auto& entry = entries[i];
            if (i < 3)
                log.push_back(std::format("    {} ({} bytes)", entry.name, entry.size));
            if (entry.size < 0 || entry.offset < 0 || entry.offset + entry.size > totalSize)
                continue;
            auto file = GetOutputFile(entry.name, entry.size, i);
            if (!file || !file->writable)
                continue;
            buffer.resize(entry.size);
            reader->Seek(entry.offset);
            if (entry.size)
                reader->Serialize(buffer.data(), entry.size);
            Write(file, 0, buffer.data(), buffer.size());
        }
        log.push_back(std::format("{}: {} files", archiveName, count));
        return true;
    }

    // Linear archive: version string, file table (hash, name, alias, size), the recorded reads (hash, chunks: offset, unk, size),
    // whose data follows at the next 128 KB boundary in record order
    bool DumpLinearArchive(FArchive* reader, std::string_view archiveName)
    {
        std::string version;
        if (!ReadString(reader, version) || version.empty() || version.size() > 64 ||
            std::any_of(version.begin(), version.end(), [](char c) { return c < 0x20 || c > 0x7E; }))
            return false;

        struct Entry { std::string name; int32_t size; };
        std::unordered_map<uint32_t, Entry> entries;
        auto count = ReadCompact(reader);
        if (count < 0 || count > 1000000)
            return false;
        for (int32_t i = 0; i < count; i++)
        {
            auto hash = static_cast<uint32_t>(ReadInt(reader));
            Entry entry;
            std::string alias;
            if (!ReadString(reader, entry.name) || !ReadString(reader, alias))
                return false;
            entry.size = ReadCompact(reader);
            if (i < 3)
                log.push_back(std::format("    {} ({} bytes){}", entry.name, entry.size, alias.empty() ? "" : " -> " + alias));
            entries[hash] = std::move(entry);
        }

        struct Chunk { int32_t offset, unk, size; };
        struct Sequence { uint32_t hash; std::vector<Chunk> chunks; };
        std::vector<Sequence> sequences;
        auto sequenceCount = ReadCompact(reader);
        if (sequenceCount < 0 || sequenceCount > 1000000)
            return false;
        for (int32_t s = 0; s < sequenceCount; s++)
        {
            Sequence sequence;
            sequence.hash = static_cast<uint32_t>(ReadInt(reader));
            auto chunkCount = ReadCompact(reader);
            if (chunkCount < 0 || chunkCount > 1000000)
                return false;
            for (int32_t c = 0; c < chunkCount; c++)
            {
                Chunk chunk;
                chunk.offset = ReadCompact(reader);
                chunk.unk = ReadCompact(reader);
                chunk.size = ReadCompact(reader);
                sequence.chunks.push_back(chunk);
            }
            sequences.push_back(std::move(sequence));
        }

        auto position = reader->Tell();
        std::vector<uint8_t> buffer(((position + 0x1FFFF) / 0x20000 << 17) - position);
        if (!buffer.empty())
            reader->Serialize(buffer.data(), static_cast<int32_t>(buffer.size()));

        auto totalSize = reader->TotalSize();
        for (auto& sequence : sequences)
        {
            auto it = entries.find(sequence.hash);
            auto file = it != entries.end() ? GetOutputFile(it->second.name, it->second.size, sequence.hash) : nullptr;
            for (auto& chunk : sequence.chunks)
            {
                if (chunk.size < 0 || reader->Tell() + chunk.size > totalSize)
                {
                    log.push_back(std::format("{}: data ends early", archiveName));
                    return true;
                }
                buffer.resize(chunk.size);
                if (chunk.size)
                    reader->Serialize(buffer.data(), chunk.size);
                Write(file, chunk.offset, buffer.data(), buffer.size());
            }
        }
        log.push_back(std::format("{}: {} files ({})", archiveName, count, version));
        return true;
    }

    // A file of a linear archive rebuilt from its recorded reads (the format DumpLinearArchive reads) into a cache file, for a read the
    // archive's stream already passed. Returns the cache file's path, or an empty string.
    std::string ExtractLinearFile(const std::string& archive, uint32_t hash, const char* path)
    {
        static std::mutex mutex;
        static std::unordered_map<uint64_t, std::string> extracted;
        std::lock_guard lock(mutex);

        auto key = (static_cast<uint64_t>(std::hash<std::string>{}(archive)) << 32) ^ hash;
        if (auto it = extracted.find(key); it != extracted.end())
            return it->second;

        auto reader = OpenArchive(archive.c_str(), 0x440);
        if (!reader)
            return {};

        std::vector<uint8_t> data;
        std::vector<bool> filled;
        bool found = false;
        std::string version;
        if (ReadString(reader, version) && !version.empty() && version.size() <= 64)
        {
            auto count = ReadCompact(reader);
            for (int32_t i = 0; i >= 0 && i < count && i < 1000000; i++)
            {
                auto entryHash = static_cast<uint32_t>(ReadInt(reader));
                std::string name, alias;
                if (!ReadString(reader, name) || !ReadString(reader, alias))
                    break;
                auto size = ReadCompact(reader);
                if (entryHash == hash && size >= 0)
                {
                    data.assign(size, 0);
                    filled.assign(size, false);
                    found = true;
                }
            }

            struct Chunk { int32_t offset, size; bool wanted; };
            std::vector<Chunk> chunks;
            auto sequenceCount = ReadCompact(reader);
            for (int32_t s = 0; found && s >= 0 && s < sequenceCount && s < 1000000; s++)
            {
                auto sequenceHash = static_cast<uint32_t>(ReadInt(reader));
                auto chunkCount = ReadCompact(reader);
                for (int32_t c = 0; c >= 0 && c < chunkCount && c < 1000000; c++)
                {
                    Chunk chunk;
                    chunk.offset = ReadCompact(reader);
                    ReadCompact(reader);
                    chunk.size = ReadCompact(reader);
                    chunk.wanted = sequenceHash == hash;
                    chunks.push_back(chunk);
                }
            }

            // the data follows at the next 128 KB boundary in record order
            auto position = reader->Tell();
            std::vector<uint8_t> buffer(((position + 0x1FFFF) / 0x20000 << 17) - position);
            if (found && !buffer.empty())
                reader->Serialize(buffer.data(), static_cast<int32_t>(buffer.size()));
            auto totalSize = reader->TotalSize();
            for (auto& chunk : chunks)
            {
                if (chunk.size < 0 || reader->Tell() + chunk.size > totalSize)
                    break;
                buffer.resize(chunk.size);
                if (chunk.size)
                    reader->Serialize(buffer.data(), chunk.size);
                if (chunk.wanted && chunk.offset >= 0 && chunk.offset + chunk.size <= static_cast<int32_t>(data.size()))
                {
                    std::copy(buffer.begin(), buffer.end(), data.begin() + chunk.offset);
                    std::fill(filled.begin() + chunk.offset, filled.begin() + chunk.offset + chunk.size, true);
                    if (std::all_of(filled.begin(), filled.end(), [](bool b) { return b; }))
                        break;
                }
            }
        }
        reader->Release();

        // only a complete file (the archive may hold just the parts that were read when it was recorded)
        std::string result;
        if (found && std::all_of(filled.begin(), filled.end(), [](bool b) { return b; }))
        {
            std::error_code ec;
            auto cachePath = std::filesystem::temp_directory_path(ec) / "BlacklistFusionFix";
            std::filesystem::create_directories(cachePath, ec);
            cachePath /= std::format("{:08X}{}", hash, std::filesystem::path(path ? path : "").extension().string());
            std::ofstream file(cachePath, std::ios::binary | std::ios::trunc);
            if (file.write(reinterpret_cast<const char*>(data.data()), data.size()))
                result = cachePath.string();
        }
        extracted[key] = result;
        return result;
    }

    void DumpArchive(const std::filesystem::path& relativePath)
    {
        auto path = relativePath.string();
        currentArchive = path;

        // linear tables (UMDs), opened like FFileManagerLinear does
        if (auto reader = relativePath.has_parent_path() ? OpenArchive(path.c_str(), 0x440) : nullptr)
        {
            auto linear = DumpLinearArchive(reader, path);
            reader->Release();
            if (linear)
                return;
        }

        // named archive, opened like FFileManagerArc does
        if (auto reader = OpenArchive(path.c_str(), 0))
        {
            if (reader->TotalSize() > 0)
                PackReaderPrecache(reader, 0x7FFFFFFF);
            if (!DumpArcArchive(reader, path))
                log.push_back(std::format("{}: unknown format, skipped", path));
            reader->Release();
        }
    }

    void Dump()
    {
        outputPath = exePath / "unpacked";
        std::error_code ec;
        std::filesystem::create_directories(outputPath, ec);

        // the mounted named archives (the active loc-<lang>.umd, dynamicflash.umd, dynamicwin.umd)
        std::vector<std::string> mounted;
        for (auto fileManager = *GFileManager; fileManager; fileManager = fileManager->Inner())
        {
            if (*reinterpret_cast<uintptr_t*>(fileManager) != FFileManagerArcVTable)
                continue;
            auto& name = *reinterpret_cast<FString*>(reinterpret_cast<uintptr_t>(fileManager) + 12);
            mounted.push_back(std::filesystem::path(name.view()).filename().string());
        }

        // loc-<lang>.umd of other languages would overwrite each other, only the mounted one is dumped
        std::vector<std::filesystem::path> archives;
        uint64_t totalSize = 0;
        for (auto& dir : { std::filesystem::path(), std::filesystem::path("UMDs") })
        {
            // incremented with an error code, the listing can end with an error (the loader merges the overload folder into it)
            for (auto it = std::filesystem::directory_iterator(exePath / dir, ec); !ec && it != std::filesystem::directory_iterator(); it.increment(ec))
            {
                auto& entry = *it;
                auto name = entry.path().filename().string();
                if (!entry.is_regular_file(ec) || _stricmp(entry.path().extension().string().c_str(), ".umd") != 0)
                    continue;
                if (starts_with_i(name, "loc-") && std::none_of(mounted.begin(), mounted.end(), [&](auto& m) { return _stricmp(m.c_str(), name.c_str()) == 0; }))
                    continue;
                archives.push_back(dir / name);
                std::error_code sizeError;
                totalSize += entry.file_size(sizeError);
            }
            if (ec && archives.empty())
                log.push_back(std::format("listing {}: {}", (exePath / dir).string(), ec.message()));
            ec.clear();
        }
        log.push_back(std::format("{} archives\n", archives.size()));

        bool cancelled = false;
        {
            ProgressDialog dialog(L"Splinter Cell Blacklist FusionFix", L"Unpacking the game archives...");
            uint64_t done = 0;
            for (auto& archive : archives)
            {
                if (dialog.Cancelled())
                {
                    cancelled = true;
                    break;
                }
                dialog.Set(done, totalSize, archive.wstring());
                DumpArchive(archive);
                done += std::filesystem::file_size(exePath / archive, ec);
            }
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
            for (size_t i = 0; i < variantArchives.size() && !cancelled; i++)
            {
                if (dialog.Cancelled())
                {
                    cancelled = true;
                    break;
                }
                dialog.Set(i, variantArchives.size(), L"Conflicting files: " + std::filesystem::path(variantArchives[i]).wstring());
                DumpArchive(variantArchives[i]);
            }
            variantPass = false;
            stream.close();
        }

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
        if (cancelled)
            log.push_back("\nCancelled, the folder is not complete.");
        log.push_back(std::format("\n{} files differ between archives and were left in them, every version of them is in _conflicts:", conflicts));
        for (auto& [relative, lines] : conflictLog)
            log.insert(log.end(), lines.begin(), lines.end());
        log.push_back(std::format("\n{} files written, {} of them are not complete and were moved to _incomplete (the game can read parts no archive contains, they would be zero):", written, incomplete));
        log.insert(log.end(), incompleteLog.begin(), incompleteLog.end());

        std::ofstream(outputPath / "_dump.log") << std::accumulate(log.begin(), log.end(), std::string(), [](auto a, auto& b) { return a + b + "\n"; });
        MessageBoxA(nullptr, std::format("{}Unpacked {} files to\n{}\n\n{} files are not complete and were moved to _incomplete, {} differ between archives and were left in them (their versions are in _conflicts).\n"
            "Move the files to the 'update' folder to load them from disk instead of the archives.\nSee _dump.log for details.",
            cancelled ? "Cancelled.\n\n" : "", written - incomplete, outputPath.string(), incomplete, conflicts).c_str(), "Splinter Cell Blacklist FusionFix", MB_OK | MB_ICONINFORMATION);
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

    // FFileManagerLinear::FileSize: NormalizeName (stdcall) and the FString destructor
    auto pattern = hook::pattern("6A 01 57 8D 4D E8 51 8B 4C 96 2C E8");
    FileLoader::NormalizeName = (decltype(FileLoader::NormalizeName))injector::GetBranchDestination(pattern.get_first(11)).as_int();
    pattern = hook::pattern("C7 45 FC FF FF FF FF 8D 4D E8 E8 ? ? ? ? 8B 8E 4C 01 00 00");
    FileLoader::FStringDestructor = (decltype(FileLoader::FStringDestructor))injector::GetBranchDestination(pattern.get_first(10)).as_int();

    // FFileManagerArc constructor
    pattern = hook::pattern("C7 06 ? ? ? ? E8 ? ? ? ? B9 01 00");
    auto arcVTable = *pattern.get_first<uintptr_t*>(2);
    Dumper::FFileManagerArcVTable = reinterpret_cast<uintptr_t>(arcVTable);

    // FFileManagerArc init: GError, the PACK reader and its precache, GFileManager (archives read by the dumper and ExtractLinearFile)
    pattern = hook::pattern("8B 3D ? ? ? ? 8B 4E 04 8B 11 68 00 00 01 00 57 6A 00 50 8B 42 10 FF D0");
    Dumper::GError = *pattern.get_first<void**>(2);
    pattern = hook::pattern("6A 00 51 6A 00 8B F8 6A 01 57 E8");
    Dumper::CreatePackReader = (decltype(Dumper::CreatePackReader))injector::GetBranchDestination(pattern.get_first(10)).as_int();
    pattern = hook::pattern("68 FF FF FF 7F E8 ? ? ? ? 8B 4E 48 8B 11 8B 42 48");
    Dumper::PackReaderPrecache = (decltype(Dumper::PackReaderPrecache))injector::GetBranchDestination(pattern.get_first(5)).as_int();
    pattern = hook::pattern("8B 0D ? ? ? ? 8B 11 8B 82 88 00 00 00 56 56 68 ? ? ? ? FF D0 E8 ? ? ? ? 84 C0");
    Dumper::GFileManager = *pattern.get_first<FFileManager**>(2);

    // Load files from the loader's overload folder instead of the archives
    if (GetOverloadedFilePathA)
    {
        pattern = hook::pattern("55 8B EC 6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 83 EC 1C 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 ? ? ? ? 8B F1 8B 45 08");
        FFileManagerArc::shLookup = safetyhook::create_inline(pattern.get_first(), FFileManagerArc::Lookup);
        FFileManagerArc::shIsAvailable = safetyhook::create_inline(arcVTable[0xA0 / 4], FFileManagerArc::IsAvailable);

        // FFileManagerLinear constructor
        pattern = hook::pattern("C7 06 ? ? ? ? 89 7E 0C 89 4D 08");
        auto linearVTable = *pattern.get_first<uintptr_t*>(2);
        // linear archive file reader constructor
        pattern = hook::pattern("89 4E 0C 8B 4D 14 89 56 10 8B 55 0C C7 06 ? ? ? ? 89 4E 14");
        if (!pattern.empty())
            FFileManagerLinear::LinearReaderVTable = *pattern.get_first<uintptr_t>(14);
        FFileManagerLinear::shCreateFileReader = safetyhook::create_inline(linearVTable[0x10 / 4], FFileManagerLinear::CreateFileReader);
        FFileManagerLinear::shFileSize = safetyhook::create_inline(linearVTable[0x18 / 4], FFileManagerLinear::FileSize);
        FFileManagerLinear::shResolveAlias = safetyhook::create_inline(linearVTable[0x4C / 4], FFileManagerLinear::ResolveAlias);
        FFileManagerLinear::shFileExists = safetyhook::create_inline(linearVTable[0x5C / 4], FFileManagerLinear::FileExists);
        FFileManagerLinear::shFileTime = safetyhook::create_inline(linearVTable[0x8C / 4], FFileManagerLinear::FileTime);
        FFileManagerLinear::shMount = safetyhook::create_inline(linearVTable[0x88 / 4], FFileManagerLinear::Mount);
        FFileManagerLinear::ExtractLinearFile = Dumper::ExtractLinearFile;
    }

    // Unpack all archives to 'unpacked' once they're mounted, after GFileManager->Mount("UMDs\Blacklist.umd")
    if (bDumpPackedFiles)
    {
        pattern = hook::pattern("8B 0D ? ? ? ? 8B 11 8B 82 88 00 00 00 56 56 68 ? ? ? ? FF D0 E8 ? ? ? ? 84 C0");
        static auto AfterMount = safetyhook::create_mid(pattern.get_first(23), [](SafetyHookContext& regs)
        {
            static std::once_flag flag;
            std::call_once(flag, []() { Dumper::Dump(); });
        });
    }
}
