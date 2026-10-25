#pragma once

// Graphics API of a Direct3D 9 (or Direct3D 8 through d3d8to9) game, for the plugins of the fixes. Copy this file
// unchanged, like upscaler_bridge_protocol.h and upscaler_bridge_client.h; the original is in the FusionFixHub
// repository (source/bridge, see docs/upscalers.md).
//
// DLAA and FSR need DXVK: the D3D9 runtime of Windows can't share textures with the hub's D3D12 device. DXVK's
// 32-bit d3d9.dll is installed into the game's folder as vulkan.dll by the hub (upscalers.json), the name GTA IV's
// Fusion Fix uses, so it doesn't clash with the many proxies called d3d9.dll (ReShade, ENB, dxwrapper, the ASI
// loader). No proxy DLL is shipped: Install() points the imports of Direct3DCreate9 and Direct3DCreate9Ex of every
// module of the game (the exe, other plugins, d3d8to9 when it imports them) at vulkan.dll, loaded by full path, before
// the game creates its device. Call it from the plugin's init, with the value of the "Graphics API" setting. Modules
// that got Direct3DCreate9 from the system's d3d9.dll some other way (GetProcAddress, also before Install(); a wrapper
// that hot patched it, like dxwrapper's d3d8to9 and d3d9 wrapper in True Crime: New York City) get the system's
// IDirect3D9, whose CreateDevice and CreateDeviceEx create the device on DXVK for Vulkan (PatchSystemObjects).
//
//   0  Direct3D 9   the system's runtime (default)
//   1  Vulkan       DXVK; offered only when VulkanAvailable()
//   2  D3D9on12     Windows' D3D9on12; hidden, only for testing (too slow for the bridge to be offered)
//
// vulkan.dll is only loaded when its SHA-256 is one of KnownDxvk (the builds upscalers.json installs): anything can
// write to a game folder. A DXVK the player put there by hand counts as missing.

#include <windows.h>
#include <bcrypt.h>
#include <psapi.h>
#include <d3d9.h>
#include <d3d9on12.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace FusionFixGraphicsApi
{
    enum class Api : int
    {
        Direct3D9 = 0,
        Vulkan = 1,
        D3D9On12 = 2,
    };

    constexpr const wchar_t* DxvkFileName = L"vulkan.dll";

    // SHA-256 of the DXVK builds of upscalers.json (component "dxvk"); keep both in step
    constexpr const char* KnownDxvk[] =
    {
        "265888c31ca78dffa290c39cb7e50bfb02762590e41927906e46fb32f01497fa",     // DXVK 3.1.1, x32\d3d9.dll
    };

    inline std::filesystem::path GameRoot()
    {
        wchar_t path[MAX_PATH * 4] = {};
        GetModuleFileNameW(nullptr, path, DWORD(std::size(path)));
        return std::filesystem::path(path).parent_path();
    }

    inline std::filesystem::path DxvkPath()
    {
        return GameRoot() / DxvkFileName;
    }

    namespace detail
    {
        // bcrypt.dll from System32 by full path: game folders can hold a proxy of that name too
        inline std::string Sha256(const std::filesystem::path& file)
        {
            std::ifstream in(file, std::ios::binary);
            if (!in)
                return {};
            std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

            auto bcrypt = LoadLibraryExW(L"bcrypt.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            if (!bcrypt)
                return {};
            using Open = NTSTATUS(WINAPI*)(BCRYPT_ALG_HANDLE*, LPCWSTR, LPCWSTR, ULONG);
            using Hash = NTSTATUS(WINAPI*)(BCRYPT_ALG_HANDLE, PUCHAR, ULONG, PUCHAR, ULONG, PUCHAR, ULONG);
            using Close = NTSTATUS(WINAPI*)(BCRYPT_ALG_HANDLE, ULONG);
            auto open = reinterpret_cast<Open>(GetProcAddress(bcrypt, "BCryptOpenAlgorithmProvider"));
            auto hash = reinterpret_cast<Hash>(GetProcAddress(bcrypt, "BCryptHash"));
            auto close = reinterpret_cast<Close>(GetProcAddress(bcrypt, "BCryptCloseAlgorithmProvider"));
            std::string result;
            BCRYPT_ALG_HANDLE algorithm = nullptr;
            unsigned char digest[32] = {};
            if (open && hash && close && open(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0)
            {
                if (hash(algorithm, nullptr, 0, reinterpret_cast<PUCHAR>(data.data()), ULONG(data.size()), digest, sizeof(digest)) >= 0)
                {
                    static const char* hex = "0123456789abcdef";
                    for (auto b : digest)
                    {
                        result += hex[b >> 4];
                        result += hex[b & 15];
                    }
                }
                close(algorithm, 0);
            }
            FreeLibrary(bcrypt);
            return result;
        }

        using Create9 = IDirect3D9* (WINAPI*)(UINT);
        using Create9Ex = HRESULT(WINAPI*)(UINT, IDirect3D9Ex**);
        using CreateDeviceFn = HRESULT(WINAPI*)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
        using CreateDeviceExFn = HRESULT(WINAPI*)(IDirect3D9Ex*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, D3DDISPLAYMODEEX*, IDirect3DDevice9Ex**);

        struct State
        {
            Api api = Api::Direct3D9;
            HMODULE target = nullptr;       // vulkan.dll or the system's d3d9.dll
            Create9 create9 = nullptr;      // of the first import that was replaced, for Direct3D 9
            Create9Ex create9Ex = nullptr;
            bool systemPatched = false;     // PatchSystemObjects ran
            CreateDeviceFn systemCreateDevice = nullptr;        // the system's IDirect3D9 methods, before the patch
            CreateDeviceExFn systemCreateDeviceEx = nullptr;
            IDirect3D9* dxvk = nullptr;     // DXVK's objects for the system's, made on first use, kept
            IDirect3D9Ex* dxvkEx = nullptr;
        };

        inline State& Get()
        {
            static State state;
            return state;
        }

        inline HMODULE SystemD3D9()
        {
            return LoadLibraryExW(L"d3d9.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        }

        inline IDirect3D9* WINAPI Direct3DCreate9(UINT sdk)
        {
            auto& s = Get();
            if (s.api == Api::D3D9On12)
            {
                using Create9On12 = IDirect3D9* (WINAPI*)(UINT, D3D9ON12_ARGS*, UINT);
                if (auto create = reinterpret_cast<Create9On12>(GetProcAddress(s.target, "Direct3DCreate9On12")))
                {
                    D3D9ON12_ARGS args{};
                    args.Enable9On12 = TRUE;
                    if (auto d3d = create(sdk, &args, 1))
                        return d3d;
                }
            }
            else if (auto create = s.target ? reinterpret_cast<Create9>(GetProcAddress(s.target, "Direct3DCreate9")) : nullptr)
            {
                return create(sdk);
            }
            return s.create9 ? s.create9(sdk) : nullptr;
        }

        inline HRESULT WINAPI Direct3DCreate9Ex(UINT sdk, IDirect3D9Ex** out)
        {
            auto& s = Get();
            if (s.api == Api::D3D9On12)
            {
                using Create9On12Ex = HRESULT(WINAPI*)(UINT, D3D9ON12_ARGS*, UINT, IDirect3D9Ex**);
                if (auto create = reinterpret_cast<Create9On12Ex>(GetProcAddress(s.target, "Direct3DCreate9On12Ex")))
                {
                    D3D9ON12_ARGS args{};
                    args.Enable9On12 = TRUE;
                    if (SUCCEEDED(create(sdk, &args, 1, out)))
                        return S_OK;
                }
            }
            else if (auto create = s.target ? reinterpret_cast<Create9Ex>(GetProcAddress(s.target, "Direct3DCreate9Ex")) : nullptr)
            {
                return create(sdk, out);
            }
            return s.create9Ex ? s.create9Ex(sdk, out) : E_NOTIMPL;
        }

        // -----------------------------------------------------------------------------------------------------------
        // The system's IDirect3D9 for Vulkan: modules that took Direct3DCreate9 from the system's d3d9.dll without the
        // imports Install() replaces, e.g. dxwrapper (as an ASI it looks it up with GetProcAddress and hot patches it
        // before the plugins' init runs, and its d3d8to9 and d3d9 wrapper call the original through its trampoline),
        // make the system's object. Its CreateDevice and CreateDeviceEx, in the vtable every IDirect3D9 of the
        // system's runtime shares, then create the device on DXVK's object. A wrapper around the system's object keeps
        // working: it wraps a DXVK device, and its QueryInterface passes DXVK's interop interfaces through.

        inline IDirect3D9* Dxvk()
        {
            auto& s = Get();
            if (!s.dxvk && s.api == Api::Vulkan && s.target)
                if (auto create = reinterpret_cast<Create9>(GetProcAddress(s.target, "Direct3DCreate9")))
                    s.dxvk = create(D3D_SDK_VERSION);
            return s.dxvk;
        }

        inline IDirect3D9Ex* DxvkEx()
        {
            auto& s = Get();
            if (!s.dxvkEx && s.api == Api::Vulkan && s.target)
                if (auto create = reinterpret_cast<Create9Ex>(GetProcAddress(s.target, "Direct3DCreate9Ex")))
                    if (FAILED(create(D3D_SDK_VERSION, &s.dxvkEx)))
                        s.dxvkEx = nullptr;
            return s.dxvkEx;
        }

        inline HRESULT WINAPI CreateDevice(IDirect3D9* self, UINT adapter, D3DDEVTYPE type, HWND window, DWORD flags,
            D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** device)
        {
            if (auto d3d = Dxvk())
                return d3d->CreateDevice(adapter, type, window, flags, parameters, device);
            return Get().systemCreateDevice(self, adapter, type, window, flags, parameters, device);
        }

        inline HRESULT WINAPI CreateDeviceEx(IDirect3D9Ex* self, UINT adapter, D3DDEVTYPE type, HWND window, DWORD flags,
            D3DPRESENT_PARAMETERS* parameters, D3DDISPLAYMODEEX* mode, IDirect3DDevice9Ex** device)
        {
            if (auto d3d = DxvkEx())
                return d3d->CreateDeviceEx(adapter, type, window, flags, parameters, mode, device);
            return Get().systemCreateDeviceEx(self, adapter, type, window, flags, parameters, mode, device);
        }

        inline bool InImage(const void* address, HMODULE module)
        {
            auto base = reinterpret_cast<const uint8_t*>(module);
            auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + reinterpret_cast<const IMAGE_DOS_HEADER*>(base)->e_lfanew);
            auto p = static_cast<const uint8_t*>(address);
            return p >= base && p < base + nt->OptionalHeader.SizeOfImage;
        }

        inline bool Readable(const void* address, size_t size)
        {
            MEMORY_BASIC_INFORMATION info{};
            if (!address || !VirtualQuery(address, &info, sizeof(info)) || info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
                return false;
            return static_cast<const uint8_t*>(address) + size <= static_cast<const uint8_t*>(info.BaseAddress) + info.RegionSize;
        }

        // The vtable of the system's object: the object's own, or, for a wrapper (the hot patched Direct3DCreate9 made
        // one), that of the object it keeps a pointer to among its first members
        inline void** SystemVtable(IUnknown* object, HMODULE system)
        {
            constexpr size_t slots = 21;    // up to CreateDeviceEx
            auto isSystem = [&](void* p)
            {
                if (!Readable(p, sizeof(void*)))
                    return false;
                auto vtable = *static_cast<void***>(p);
                return InImage(vtable, system) && Readable(vtable, slots * sizeof(void*)) && InImage(vtable[16], system);
            };
            if (!object)
                return nullptr;
            if (isSystem(object))
                return *reinterpret_cast<void***>(object);
            auto members = reinterpret_cast<void**>(object);
            for (size_t i = 1; i < 32 && Readable(&members[i], sizeof(void*)); ++i)
                if (members[i] && isSystem(members[i]))
                    return *static_cast<void***>(members[i]);
            return nullptr;
        }

        template <typename T>
        inline void PatchSlot(void** vtable, size_t index, void* replacement, T& original)
        {
            if (vtable[index] == replacement)
                return;
            if (!original)
                original = reinterpret_cast<T>(vtable[index]);
            else if (reinterpret_cast<void*>(original) != vtable[index])
                return;     // another class with other methods: not one this knows how to call
            DWORD protection = 0;
            if (VirtualProtect(&vtable[index], sizeof(void*), PAGE_READWRITE, &protection))
            {
                vtable[index] = replacement;
                VirtualProtect(&vtable[index], sizeof(void*), protection, &protection);
            }
        }

        // Once, for Vulkan: a system object of each kind shows the vtable (IDirect3D9 slot 16 CreateDevice,
        // IDirect3D9Ex slot 20 CreateDeviceEx)
        inline void PatchSystemObjects()
        {
            auto& s = Get();
            if (s.systemPatched || s.api != Api::Vulkan)
                return;
            s.systemPatched = true;

            auto system = SystemD3D9();     // stays loaded: its vtable is patched
            if (!system)
                return;
            if (auto create = reinterpret_cast<Create9>(GetProcAddress(system, "Direct3DCreate9")))
            {
                if (auto d3d = create(D3D_SDK_VERSION))
                {
                    if (auto vtable = SystemVtable(d3d, system))
                        PatchSlot(vtable, 16, reinterpret_cast<void*>(&CreateDevice), s.systemCreateDevice);
                    d3d->Release();
                }
            }
            if (auto create = reinterpret_cast<Create9Ex>(GetProcAddress(system, "Direct3DCreate9Ex")))
            {
                IDirect3D9Ex* d3d = nullptr;
                if (SUCCEEDED(create(D3D_SDK_VERSION, &d3d)) && d3d)
                {
                    if (auto vtable = SystemVtable(d3d, system))
                    {
                        PatchSlot(vtable, 16, reinterpret_cast<void*>(&CreateDevice), s.systemCreateDevice);
                        PatchSlot(vtable, 20, reinterpret_cast<void*>(&CreateDeviceEx), s.systemCreateDeviceEx);
                    }
                    d3d->Release();
                }
            }
        }

        inline void Replace(IMAGE_THUNK_DATA* address, void* replacement)
        {
            DWORD protection = 0;
            if (VirtualProtect(&address->u1.Function, sizeof(address->u1.Function), PAGE_READWRITE, &protection))
            {
                address->u1.Function = reinterpret_cast<ULONG_PTR>(replacement);
                VirtualProtect(&address->u1.Function, sizeof(address->u1.Function), protection, &protection);
            }
        }

        // The imports of d3d9.dll in one module: by name, or, in exes without the names (no OriginalFirstThunk, e.g.
        // NFS Carbon's, rebuilt after unpacking), by the address the loader bound them to
        inline void PatchModule(HMODULE module)
        {
            auto base = reinterpret_cast<uint8_t*>(module);
            auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
            if (dos->e_magic != IMAGE_DOS_SIGNATURE)
                return;
            auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
            auto& directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
            if (!directory.VirtualAddress)
                return;

            auto& s = Get();
            for (auto import = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + directory.VirtualAddress); import->Name; ++import)
            {
                if (_stricmp(reinterpret_cast<const char*>(base + import->Name), "d3d9.dll") != 0)
                    continue;
                if (!import->OriginalFirstThunk)
                {
                    auto d3d9 = GetModuleHandleW(L"d3d9.dll");
                    auto bound9 = d3d9 ? reinterpret_cast<ULONG_PTR>(GetProcAddress(d3d9, "Direct3DCreate9")) : 0;
                    auto bound9Ex = d3d9 ? reinterpret_cast<ULONG_PTR>(GetProcAddress(d3d9, "Direct3DCreate9Ex")) : 0;
                    for (auto addresses = reinterpret_cast<IMAGE_THUNK_DATA*>(base + import->FirstThunk); addresses->u1.Function; ++addresses)
                    {
                        if (bound9 && addresses->u1.Function == bound9)
                        {
                            if (!s.create9)
                                s.create9 = reinterpret_cast<Create9>(bound9);
                            Replace(addresses, reinterpret_cast<void*>(&Direct3DCreate9));
                        }
                        else if (bound9Ex && addresses->u1.Function == bound9Ex)
                        {
                            if (!s.create9Ex)
                                s.create9Ex = reinterpret_cast<Create9Ex>(bound9Ex);
                            Replace(addresses, reinterpret_cast<void*>(&Direct3DCreate9Ex));
                        }
                    }
                    continue;
                }
                auto names = reinterpret_cast<IMAGE_THUNK_DATA*>(base + import->OriginalFirstThunk);
                auto addresses = reinterpret_cast<IMAGE_THUNK_DATA*>(base + import->FirstThunk);
                for (; names->u1.AddressOfData; ++names, ++addresses)
                {
                    if (IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal))
                        continue;
                    auto name = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + names->u1.AddressOfData)->Name;
                    void* replacement = nullptr;
                    if (strcmp(name, "Direct3DCreate9") == 0)
                    {
                        if (!s.create9 && addresses->u1.Function != reinterpret_cast<ULONG_PTR>(&Direct3DCreate9))
                            s.create9 = reinterpret_cast<Create9>(addresses->u1.Function);
                        replacement = reinterpret_cast<void*>(&Direct3DCreate9);
                    }
                    else if (strcmp(name, "Direct3DCreate9Ex") == 0)
                    {
                        if (!s.create9Ex && addresses->u1.Function != reinterpret_cast<ULONG_PTR>(&Direct3DCreate9Ex))
                            s.create9Ex = reinterpret_cast<Create9Ex>(addresses->u1.Function);
                        replacement = reinterpret_cast<void*>(&Direct3DCreate9Ex);
                    }
                    if (replacement)
                        Replace(addresses, replacement);
                }
            }
        }

        // Every module of the process but the system's and the target: modules loaded later (a plugin that
        // creates the device itself) are patched when Install() runs again
        inline void PatchAll()
        {
            wchar_t system[MAX_PATH] = {};
            GetSystemDirectoryW(system, MAX_PATH);
            // From kernel32 by name: psapi.h declares K32EnumProcessModules only for Windows 7 and later targets
            using EnumModules = BOOL(WINAPI*)(HANDLE, HMODULE*, DWORD, LPDWORD);
            auto enumModules = reinterpret_cast<EnumModules>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "K32EnumProcessModules"));
            HMODULE modules[1024];
            DWORD needed = 0;
            if (!enumModules || !enumModules(GetCurrentProcess(), modules, sizeof(modules), &needed))
                return;
            for (DWORD i = 0; i < needed / sizeof(HMODULE) && i < std::size(modules); ++i)
            {
                wchar_t path[MAX_PATH * 4] = {};
                if (modules[i] == Get().target || !GetModuleFileNameW(modules[i], path, DWORD(std::size(path))))
                    continue;
                if (_wcsnicmp(path, system, wcslen(system)) == 0)
                    continue;
                PatchModule(modules[i]);
            }
        }
    }

    // vulkan.dll is DXVK's x32\d3d9.dll of upscalers.json
    inline bool VulkanAvailable()
    {
        std::error_code ec;
        auto path = DxvkPath();
        if (!std::filesystem::is_regular_file(path, ec))
            return false;
        auto hash = detail::Sha256(path);
        for (auto known : KnownDxvk)
            if (hash == known)
                return true;
        return false;
    }

    // The API the game runs on, once its device exists
    inline Api Running()
    {
        if (GetModuleHandleW(L"d3d9on12.dll"))
            return Api::D3D9On12;
        if (GetModuleHandleW(L"vulkan-1.dll") || GetModuleHandleW(L"winevulkan.dll"))
            return Api::Vulkan;
        return Api::Direct3D9;
    }

    // Applies the setting (call before the game creates its Direct3D object; again after loading modules that
    // import d3d9.dll). Vulkan without a known vulkan.dll, and D3D9on12 without allowD3D9On12, stay Direct3D 9.
    // Returns the API that will be used.
    inline Api Install(Api api, bool allowD3D9On12 = false)
    {
        auto& s = detail::Get();
        if (!s.target)
        {
            if (api == Api::Vulkan && VulkanAvailable())
            {
                // By full path; DXVK's own imports (vulkan-1.dll) come from System32
                s.target = LoadLibraryExW(DxvkPath().c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
                s.api = s.target ? Api::Vulkan : Api::Direct3D9;
            }
            else if (api == Api::D3D9On12 && allowD3D9On12)
            {
                s.target = detail::SystemD3D9();
                s.api = s.target ? Api::D3D9On12 : Api::Direct3D9;
            }
            if (!s.target)
                return Api::Direct3D9;
        }
        detail::PatchAll();
        detail::PatchSystemObjects();
        return s.api;
    }
}
