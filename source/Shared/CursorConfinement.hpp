#pragma once
#include <mutex>
#include <atomic>
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

namespace CursorConfinement
{
    inline std::mutex Mutex;
    inline bool Enabled = true, Suspended = false, Owned = false;
    inline RECT OwnedRect{};
    using Clip = BOOL(WINAPI*)(const RECT*);
    inline std::atomic<HWND> Window{ nullptr };
    inline std::atomic<Clip> OriginalClip{ nullptr };
    inline Clip* NativeSlot = nullptr;
    inline ULONGLONG NextProbe = 0;
    inline HWND AttachedWindow = nullptr;
    inline bool AttachmentReported = false;
    inline void Detach();
    inline void AdoptNativeClip();


    inline bool Equal(const RECT& first, const RECT& second)
    {
        return first.left == second.left && first.top == second.top && first.right == second.right && first.bottom == second.bottom;
    }

    inline void ReleaseLocked()
    {
        RECT current{};
        if (Owned && GetClipCursor(&current) && Equal(current, OwnedRect))
            ClipCursor(nullptr);
        Owned = false;
    }

    inline void Release()
    {
        const std::lock_guard lock(Mutex);
        ReleaseLocked();
    }

    inline void SetEnabled(bool enabled)
    {
        const std::lock_guard lock(Mutex);
        Enabled = enabled;
        if (!enabled)
            ReleaseLocked();
    }

    inline void Suspend(bool suspended)
    {
        const std::lock_guard lock(Mutex);
        Suspended = suspended;
        if (suspended)
            ReleaseLocked();
    }

    inline void Update(HWND window)
    {
        if (Window.exchange(window) != window)
        {
            Suspend(false);
            AdoptNativeClip();
        }
        const std::lock_guard lock(Mutex);
        GUITHREADINFO info{ .cbSize = sizeof(GUITHREADINFO) };
        const auto capture = GetCapture();
        if (!Enabled || Suspended || !window || GetForegroundWindow() != window || !IsWindowVisible(window) || IsIconic(window) ||
            (capture && capture != window) || !GetGUIThreadInfo(GetWindowThreadProcessId(window, nullptr), &info) ||
            (info.flags & (GUI_INMENUMODE | GUI_INMOVESIZE | GUI_POPUPMENUMODE | GUI_SYSTEMMENUMODE)) ||
            (info.hwndFocus && info.hwndFocus != window))
        {
            ReleaseLocked();
            return;
        }
        RECT client{}, current{};
        POINT origin{};
        if (!GetClientRect(window, &client) || client.right <= 0 || client.bottom <= 0 || !ClientToScreen(window, &origin) ||
            !GetClipCursor(&current))
        {
            ReleaseLocked();
            return;
        }
        OffsetRect(&client, origin.x, origin.y);
        const RECT desktop{ GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN),
            GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN),
            GetSystemMetrics(SM_YVIRTUALSCREEN) + GetSystemMetrics(SM_CYVIRTUALSCREEN) };
        if (!IntersectRect(&client, &client, &desktop))
        {
            ReleaseLocked();
            return;
        }
        // An overlay or another input owner may replace our clip between frames.
        if (!Equal(current, desktop) && !Equal(current, client) && !(Owned && Equal(current, OwnedRect)))
        {
            Owned = false;
            return;
        }
        if (Equal(current, client) || ClipCursor(&client))
        {
            OwnedRect = client;
            Owned = true;
        }
        else
            Owned = false;
    }

    inline BOOL WINAPI NativeClip(const RECT* rect)
    {
        const auto original = OriginalClip.load();
        if (!original)
            return FALSE;
        const auto window = Window.load();
        if (rect && window && GetCapture() == window)
        {
            const std::lock_guard lock(Mutex);
            if (!Enabled || Suspended || GetForegroundWindow() != window)
                return TRUE;
            const auto result = original(rect);
            const auto error = GetLastError();
            if (result && GetClipCursor(&OwnedRect))
                Owned = true;
            SetLastError(error);
            return result;
        }
        return original(rect);
    }

    inline Clip* FindSlot(HMODULE module)
    {
        const auto base = reinterpret_cast<uint8_t*>(module);
        const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (!module || dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0 || dos->e_lfanew > 1024 * 1024)
            return nullptr;
        const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        const auto size = nt->OptionalHeader.SizeOfImage;
        if (nt->Signature != IMAGE_NT_SIGNATURE || size > 64 * 1024 * 1024)
            return nullptr;
        const auto imports = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (!imports.VirtualAddress || imports.VirtualAddress >= size || imports.Size > size - imports.VirtualAddress)
            return nullptr;
        const auto descriptors = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(base + imports.VirtualAddress);
        for (size_t index = 0; (index + 1) * sizeof(IMAGE_IMPORT_DESCRIPTOR) <= imports.Size; ++index)
        {
            const auto& descriptor = descriptors[index];
            if (!descriptor.Name)
                break;
            if (!descriptor.OriginalFirstThunk || !descriptor.FirstThunk || descriptor.OriginalFirstThunk >= size || descriptor.FirstThunk >= size)
                continue;
            const auto names = reinterpret_cast<const IMAGE_THUNK_DATA*>(base + descriptor.OriginalFirstThunk);
            const auto slots = reinterpret_cast<IMAGE_THUNK_DATA*>(base + descriptor.FirstThunk);
            const auto count = (size - std::max(descriptor.OriginalFirstThunk, descriptor.FirstThunk)) / sizeof(IMAGE_THUNK_DATA);
            for (size_t entry = 0; entry < count && names[entry].u1.AddressOfData; ++entry)
            {
                const auto address = names[entry].u1.AddressOfData;
                if (IMAGE_SNAP_BY_ORDINAL(address) || address >= size || size - address < sizeof(WORD) + sizeof("ClipCursor"))
                    continue;
                const auto name = reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(base + address);
                if (!memcmp(name->Name, "ClipCursor", sizeof("ClipCursor")))
                    return reinterpret_cast<Clip*>(&slots[entry].u1.Function);
            }
        }
        return nullptr;
    }

    inline void AdoptNativeClip()
    {
        const auto window = Window.load();
        if (!window || GetCapture() != window)
            return;
        const std::lock_guard lock(Mutex);
        RECT client{}, current{};
        POINT origin{};
        const RECT desktop{ GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN),
            GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN),
            GetSystemMetrics(SM_YVIRTUALSCREEN) + GetSystemMetrics(SM_CYVIRTUALSCREEN) };
        if (!GetClientRect(window, &client) || !ClientToScreen(window, &origin) || !GetClipCursor(&current))
            return;
        OffsetRect(&client, origin.x, origin.y);
        if (!IntersectRect(&client, &client, &desktop) || !Equal(client, current))
            return;
        OwnedRect = current;
        Owned = true;
        if (!Enabled || Suspended || GetForegroundWindow() != window)
            ReleaseLocked();
    }

    inline void Bind()
    {
        if (NativeSlot || GetTickCount64() < NextProbe)
            return;
        NextProbe = GetTickCount64() + 1000;
        wchar_t windows[MAX_PATH]{};
        if (!GetWindowsDirectoryW(windows, MAX_PATH))
            return;
        const auto system = std::wstring(windows) + L"\\System32\\dinput8.dll";
        const auto wow = std::wstring(windows) + L"\\SysWOW64\\dinput8.dll";
        auto module = GetModuleHandleW(system.c_str());
        if (!module)
            module = GetModuleHandleW(wow.c_str());
        if (!module)
            return;
        const auto slot = FindSlot(module);
        const auto api = reinterpret_cast<Clip>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "ClipCursor"));
        if (!slot || !api || *slot != api)
        {
            OutputDebugStringA("WFP: native cursor callback unavailable; native input confinement remains.\n");
            NextProbe = UINT64_MAX;
            return;
        }
        HMODULE retained = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(slot), &retained))
            return;
        OriginalClip.store(*slot);
        NativeSlot = slot;
        ProtectedGameRef<Clip>{ slot } = NativeClip;
        AdoptNativeClip();
    }

    inline void Shutdown()
    {
        Detach();
        Window.store(nullptr);
        if (NativeSlot && *NativeSlot == NativeClip)
            ProtectedGameRef<Clip>{ NativeSlot } = OriginalClip.load();
        NativeSlot = nullptr;
        Release();
        Suspend(false);
    }

    inline void Message(UINT message, WPARAM wParam)
    {
        if (message == WM_NCDESTROY)
        {
            Window.store(nullptr);
            const std::lock_guard lock(Mutex);
            ReleaseLocked();
            Suspended = false;
            return;
        }
        if (message == WM_ENTERSIZEMOVE || message == WM_ENTERMENULOOP)
            Suspend(true);
        else if (message == WM_EXITSIZEMOVE || message == WM_EXITMENULOOP)
            Suspend(false);
        else if (message == WM_KILLFOCUS || (message == WM_ACTIVATEAPP && !wParam))
            Release();
    }

    inline LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        if (Window.load() == window || AttachedWindow == window)
            Message(message, wParam);
        if (message == WM_NCDESTROY)
        {
            RemoveWindowSubclass(window, Procedure, 0);
            if (AttachedWindow == window)
                AttachedWindow = nullptr;
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }

    inline void Detach()
    {
        if (AttachedWindow && IsWindow(AttachedWindow))
            RemoveWindowSubclass(AttachedWindow, Procedure, 0);
        AttachedWindow = nullptr;
    }

    inline bool Attach(HWND window)
    {
        if (window && window == AttachedWindow)
            return true;
        Release();
        Detach();
        if (!window || !IsWindow(window))
            return false;
        if (GetWindowThreadProcessId(window, nullptr) != GetCurrentThreadId() || !SetWindowSubclass(window, Procedure, 0, 0))
        {
            if (!AttachmentReported)
                OutputDebugStringA("WFP: cursor window messages unavailable; confinement disabled.\n");
            AttachmentReported = true;
            return false;
        }
        AttachedWindow = window;
        AttachmentReported = false;
        return true;
    }

    inline void Frame(HWND window)
    {
        if (!Attach(window))
        {
            Update(nullptr);
            return;
        }
        Update(window);
        Bind();
    }
}
