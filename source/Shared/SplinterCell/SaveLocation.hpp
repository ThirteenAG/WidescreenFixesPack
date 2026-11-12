#pragma once

// MAIN.SaveLocation: where the game keeps its profiles and saves
namespace SCSaveLocation
{
    // Documents\My Games\<name>, created when missing
    inline std::filesystem::path Documents(const wchar_t* name)
    {
        std::filesystem::path path;
        PWSTR documents = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_CREATE, nullptr, &documents)))
            path = std::filesystem::path(documents) / L"My Games" / name;
        CoTaskMemFree(documents);
        if (!path.empty())
            SHCreateDirectoryExW(nullptr, path.c_str(), nullptr);
        return path;
    }

    // The folder that holds the game's System folder
    inline std::filesystem::path GameFolder()
    {
        wchar_t exe[MAX_PATH * 2] = {};
        GetModuleFileNameW(nullptr, exe, DWORD(std::size(exe)));
        return std::filesystem::path(exe).parent_path().parent_path();
    }

    // Every file the process opens, finds, creates or deletes inside one folder goes to another folder instead. For games
    // that build their save paths in several places (engine, scripts, menus).
    namespace Redirect
    {
        inline std::wstring from; // full path, no trailing separator
        inline std::wstring to;
        inline std::wstring leaf; // last part of 'from', every path into it names it

        inline bool NamesLeaf(std::wstring_view path)
        {
            auto it = std::search(path.begin(), path.end(), leaf.begin(), leaf.end(), [](wchar_t a, wchar_t b)
            {
                return towlower(a) == towlower(b);
            });
            return it != path.end();
        }

        inline bool Map(LPCWSTR path, std::wstring& out)
        {
            if (!path || !*path || !NamesLeaf(path))
                return false;
            wchar_t full[MAX_PATH * 2];
            auto length = GetFullPathNameW(path, DWORD(std::size(full)), full, nullptr);
            if (!length || length >= std::size(full) || length < from.size())
                return false;
            if (_wcsnicmp(full, from.c_str(), from.size()) != 0)
                return false;
            auto rest = full + from.size();
            if (*rest != L'\0' && *rest != L'\\' && *rest != L'/')
                return false;
            out = to + rest;
            return true;
        }

        // The path to hand to the original function
        struct Path
        {
            std::wstring mapped;
            LPCWSTR value;
            Path(LPCWSTR path) : value(path)
            {
                if (Map(path, mapped))
                    value = mapped.c_str();
            }
            operator LPCWSTR() const { return value; }
        };

        // The same for the ANSI functions that don't end in a hooked one
        struct AnsiPath
        {
            std::wstring wide;
            std::wstring mapped;
            LPCWSTR value{ nullptr };
            bool redirected{ false };
            AnsiPath(LPCSTR path)
            {
                if (!path)
                    return;
                auto codePage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
                wide.resize(MultiByteToWideChar(codePage, 0, path, -1, nullptr, 0));
                MultiByteToWideChar(codePage, 0, path, -1, wide.data(), int(wide.size()));
                redirected = Map(wide.c_str(), mapped);
                value = redirected ? mapped.c_str() : wide.c_str();
            }
            operator LPCWSTR() const { return value; }
        };

        inline SafetyHookInline shCreateFileW, shFindFirstFileW, shFindFirstFileExW, shCreateDirectoryW, shRemoveDirectoryW,
            shDeleteFileW, shGetFileAttributesW, shGetFileAttributesExW, shSetFileAttributesW, shMoveFileW, shMoveFileExW,
            shCopyFileW, shMoveFileA, shMoveFileExA, shCopyFileA;

        inline HANDLE WINAPI CreateFileWHook(LPCWSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES sa, DWORD disposition, DWORD flags, HANDLE templateFile)
        {
            return shCreateFileW.stdcall<HANDLE>(LPCWSTR(Path(name)), access, share, sa, disposition, flags, templateFile);
        }

        inline HANDLE WINAPI FindFirstFileWHook(LPCWSTR name, LPWIN32_FIND_DATAW data)
        {
            return shFindFirstFileW.stdcall<HANDLE>(LPCWSTR(Path(name)), data);
        }

        inline HANDLE WINAPI FindFirstFileExWHook(LPCWSTR name, FINDEX_INFO_LEVELS level, LPVOID data, FINDEX_SEARCH_OPS op, LPVOID filter, DWORD flags)
        {
            return shFindFirstFileExW.stdcall<HANDLE>(LPCWSTR(Path(name)), level, data, op, filter, flags);
        }

        inline BOOL WINAPI CreateDirectoryWHook(LPCWSTR name, LPSECURITY_ATTRIBUTES sa)
        {
            return shCreateDirectoryW.stdcall<BOOL>(LPCWSTR(Path(name)), sa);
        }

        inline BOOL WINAPI RemoveDirectoryWHook(LPCWSTR name)
        {
            return shRemoveDirectoryW.stdcall<BOOL>(LPCWSTR(Path(name)));
        }

        inline BOOL WINAPI DeleteFileWHook(LPCWSTR name)
        {
            return shDeleteFileW.stdcall<BOOL>(LPCWSTR(Path(name)));
        }

        inline DWORD WINAPI GetFileAttributesWHook(LPCWSTR name)
        {
            return shGetFileAttributesW.stdcall<DWORD>(LPCWSTR(Path(name)));
        }

        inline BOOL WINAPI GetFileAttributesExWHook(LPCWSTR name, GET_FILEEX_INFO_LEVELS level, LPVOID info)
        {
            return shGetFileAttributesExW.stdcall<BOOL>(LPCWSTR(Path(name)), level, info);
        }

        inline BOOL WINAPI SetFileAttributesWHook(LPCWSTR name, DWORD attributes)
        {
            return shSetFileAttributesW.stdcall<BOOL>(LPCWSTR(Path(name)), attributes);
        }

        inline BOOL WINAPI MoveFileWHook(LPCWSTR source, LPCWSTR target)
        {
            return shMoveFileW.stdcall<BOOL>(LPCWSTR(Path(source)), LPCWSTR(Path(target)));
        }

        inline BOOL WINAPI MoveFileExWHook(LPCWSTR source, LPCWSTR target, DWORD flags)
        {
            return shMoveFileExW.stdcall<BOOL>(LPCWSTR(Path(source)), LPCWSTR(Path(target)), flags);
        }

        inline BOOL WINAPI CopyFileWHook(LPCWSTR source, LPCWSTR target, BOOL failIfExists)
        {
            return shCopyFileW.stdcall<BOOL>(LPCWSTR(Path(source)), LPCWSTR(Path(target)), failIfExists);
        }

        inline BOOL WINAPI MoveFileAHook(LPCSTR source, LPCSTR target)
        {
            AnsiPath sourcePath(source), targetPath(target);
            if (sourcePath.redirected || targetPath.redirected)
                return shMoveFileW.stdcall<BOOL>(LPCWSTR(sourcePath), LPCWSTR(targetPath));
            return shMoveFileA.stdcall<BOOL>(source, target);
        }

        inline BOOL WINAPI MoveFileExAHook(LPCSTR source, LPCSTR target, DWORD flags)
        {
            AnsiPath sourcePath(source), targetPath(target);
            if (sourcePath.redirected || targetPath.redirected)
                return shMoveFileExW.stdcall<BOOL>(LPCWSTR(sourcePath), LPCWSTR(targetPath), flags);
            return shMoveFileExA.stdcall<BOOL>(source, target, flags);
        }

        inline BOOL WINAPI CopyFileAHook(LPCSTR source, LPCSTR target, BOOL failIfExists)
        {
            AnsiPath sourcePath(source), targetPath(target);
            if (sourcePath.redirected || targetPath.redirected)
                return shCopyFileW.stdcall<BOOL>(LPCWSTR(sourcePath), LPCWSTR(targetPath), failIfExists);
            return shCopyFileA.stdcall<BOOL>(source, target, failIfExists);
        }

        // The implementation in KernelBase where it has one (callers of either DLL end up there), else the kernel32 export
        inline void* Export(const char* name)
        {
            static auto kernelBase = GetModuleHandleW(L"kernelbase.dll");
            static auto kernel32 = GetModuleHandleW(L"kernel32.dll");
            void* function = kernelBase ? reinterpret_cast<void*>(GetProcAddress(kernelBase, name)) : nullptr;
            return function ? function : reinterpret_cast<void*>(GetProcAddress(kernel32, name));
        }

        inline void Install(const std::filesystem::path& source, const std::filesystem::path& target)
        {
            // No Documents folder: the game keeps its own
            if (source.empty() || target.empty())
                return;

            from = source.lexically_normal().wstring();
            while (!from.empty() && (from.back() == L'\\' || from.back() == L'/'))
                from.pop_back();
            to = target.lexically_normal().wstring();
            while (!to.empty() && (to.back() == L'\\' || to.back() == L'/'))
                to.pop_back();
            leaf = std::filesystem::path(from).filename().wstring();

            shCreateFileW = safetyhook::create_inline(Export("CreateFileW"), CreateFileWHook);
            shFindFirstFileW = safetyhook::create_inline(Export("FindFirstFileW"), FindFirstFileWHook);
            shFindFirstFileExW = safetyhook::create_inline(Export("FindFirstFileExW"), FindFirstFileExWHook);
            shCreateDirectoryW = safetyhook::create_inline(Export("CreateDirectoryW"), CreateDirectoryWHook);
            shRemoveDirectoryW = safetyhook::create_inline(Export("RemoveDirectoryW"), RemoveDirectoryWHook);
            shDeleteFileW = safetyhook::create_inline(Export("DeleteFileW"), DeleteFileWHook);
            shGetFileAttributesW = safetyhook::create_inline(Export("GetFileAttributesW"), GetFileAttributesWHook);
            shGetFileAttributesExW = safetyhook::create_inline(Export("GetFileAttributesExW"), GetFileAttributesExWHook);
            shSetFileAttributesW = safetyhook::create_inline(Export("SetFileAttributesW"), SetFileAttributesWHook);
            shMoveFileW = safetyhook::create_inline(Export("MoveFileW"), MoveFileWHook);
            shMoveFileExW = safetyhook::create_inline(Export("MoveFileExW"), MoveFileExWHook);
            shCopyFileW = safetyhook::create_inline(Export("CopyFileW"), CopyFileWHook);
            shMoveFileA = safetyhook::create_inline(Export("MoveFileA"), MoveFileAHook);
            shMoveFileExA = safetyhook::create_inline(Export("MoveFileExA"), MoveFileExAHook);
            shCopyFileA = safetyhook::create_inline(Export("CopyFileA"), CopyFileAHook);
        }
    }
}
