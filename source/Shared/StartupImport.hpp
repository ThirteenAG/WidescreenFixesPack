#pragma once

template<typename T>
void ReplaceStartupImport(HMODULE module, const char* library, const char* name, T replacement, T& original)
{
    auto base = reinterpret_cast<uintptr_t>(module);
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    auto imports = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    for (; imports->Name; ++imports)
    {
        if (_stricmp(reinterpret_cast<const char*>(base + imports->Name), library) != 0)
            continue;
        auto names = reinterpret_cast<IMAGE_THUNK_DATA*>(base + imports->OriginalFirstThunk);
        auto addresses = reinterpret_cast<IMAGE_THUNK_DATA*>(base + imports->FirstThunk);
        for (size_t i = 0; names[i].u1.AddressOfData; ++i)
        {
            if (IMAGE_SNAP_BY_ORDINAL(names[i].u1.Ordinal))
                continue;
            auto import = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + names[i].u1.AddressOfData);
            if (strcmp(reinterpret_cast<const char*>(import->Name), name) != 0)
                continue;
            auto slot = reinterpret_cast<T*>(&addresses[i].u1.Function);
            DWORD protect;
            VirtualProtect(slot, sizeof(*slot), PAGE_READWRITE, &protect);
            original = *slot;
            *slot = replacement;
            VirtualProtect(slot, sizeof(*slot), protect, &protect);
            return;
        }
    }
    std::terminate();
}
