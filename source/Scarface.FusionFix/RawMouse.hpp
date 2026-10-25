#pragma once
#include <atomic>
#include <psapi.h>

namespace ScarfaceRawMouse
{
    using ReadData = UINT(WINAPI*)(HRAWINPUT, UINT, LPVOID, PUINT, UINT);
    inline std::atomic<ReadData> Original{ nullptr };
    inline ReadData* NativeSlot = nullptr;
    inline const int* Enabled = nullptr;
    inline const float* Sensitivity = nullptr;
    inline HWND* Window = nullptr;
    inline std::atomic<uint64_t> Pending{ 0 };
    inline std::atomic<bool> HaveRaw{ false };
    inline ULONGLONG NextProbe = 0;
    inline int FailedProbes = 0;   // dinput8 found without the callback: given up after a few tries
    inline bool FrameReady = false, Sampled = false, ProbeReported = false;
    inline unsigned Consumed = 0;
    inline std::pair<float, float> Motion{};

    inline bool Focused()
    {
        return Window && *Window && GetForegroundWindow() == *Window;
    }

    inline void Accumulate(LONG x, LONG y)
    {
        auto previous = Pending.load(std::memory_order_relaxed);
        for (;;)
        {
            const auto oldX = static_cast<int32_t>(previous);
            const auto oldY = static_cast<int32_t>(previous >> 32);
            const auto nextX = static_cast<int32_t>(std::clamp<int64_t>(int64_t(oldX) + x, INT32_MIN, INT32_MAX));
            const auto nextY = static_cast<int32_t>(std::clamp<int64_t>(int64_t(oldY) + y, INT32_MIN, INT32_MAX));
            const uint64_t next = uint64_t(static_cast<uint32_t>(nextX)) | (uint64_t(static_cast<uint32_t>(nextY)) << 32);
            if (Pending.compare_exchange_weak(previous, next, std::memory_order_release, std::memory_order_relaxed))
                return;
        }
    }

    inline void Capture(const RAWINPUT& input)
    {
        if (!Enabled || !*Enabled || !Focused() || input.header.dwType != RIM_TYPEMOUSE)
            return;
        if (input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
        {
            HaveRaw.store(false, std::memory_order_release);
            Pending.exchange(0, std::memory_order_acquire);
            return;
        }
        HaveRaw.store(true, std::memory_order_release);
        Accumulate(input.data.mouse.lLastX, input.data.mouse.lLastY);
    }

    inline UINT WINAPI Read(HRAWINPUT input, UINT command, LPVOID data, PUINT size, UINT headerSize)
    {
        const auto original = Original.load(std::memory_order_acquire);
        if (!original)
            return UINT(-1);
        const auto result = original(input, command, data, size, headerSize);
        const auto error = GetLastError();
        if (command == RID_INPUT && data && result != UINT(-1) && result >= sizeof(RAWINPUT) && headerSize == sizeof(RAWINPUTHEADER))
        {
            const auto& packet = *static_cast<const RAWINPUT*>(data);
            if (packet.header.dwSize >= sizeof(RAWINPUT) && packet.header.dwSize <= result)
                Capture(packet);
        }
        SetLastError(error);
        return result;
    }

    inline ReadData* FindSlot(HMODULE module, ReadData api)
    {
        const auto base = reinterpret_cast<uintptr_t>(module);
        const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0 || dos->e_lfanew > 1024 * 1024)
            return nullptr;
        const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
            nt->OptionalHeader.SizeOfImage > 64 * 1024 * 1024)
            return nullptr;
        const auto sections = IMAGE_FIRST_SECTION(nt);
        std::vector<std::vector<uint8_t>> code;
        std::vector<uintptr_t> slots;
        for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i)
        {
            const auto& section = sections[i];
            if (!(section.Characteristics & IMAGE_SCN_MEM_READ) || section.VirtualAddress >= nt->OptionalHeader.SizeOfImage)
                continue;
            const size_t size = std::min(section.Misc.VirtualSize, nt->OptionalHeader.SizeOfImage - section.VirtualAddress);
            std::vector<uint8_t> bytes(size);
            SIZE_T copied = 0;
            if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(base + section.VirtualAddress),
                bytes.data(), size, &copied) || copied != size)
                continue;
            if (section.Characteristics & IMAGE_SCN_MEM_EXECUTE)
                code.push_back(std::move(bytes));
            else
            {
                for (size_t offset = 0; offset + sizeof(uintptr_t) <= bytes.size(); offset += sizeof(uintptr_t))
                {
                    uintptr_t value = 0;
                    memcpy(&value, bytes.data() + offset, sizeof(value));
                    if (value == reinterpret_cast<uintptr_t>(api))
                        slots.push_back(base + section.VirtualAddress + offset);
                }
            }
        }
        ReadData* result = nullptr;
        for (const auto slot : slots)
        {
            bool referenced = false;
            for (const auto& bytes : code)
            {
                for (size_t offset = 1; offset + sizeof(uintptr_t) <= bytes.size(); ++offset)
                {
                    uintptr_t value = 0;
                    memcpy(&value, bytes.data() + offset, sizeof(value));
                    if (value == slot && (bytes[offset - 1] == 0xA1 || (offset >= 2 &&
                        ((bytes[offset - 2] == 0x8B && (bytes[offset - 1] & 0xC7) == 5) ||
                        (bytes[offset - 2] == 0xFF && bytes[offset - 1] == 0x15)))))
                    {
                        referenced = true;
                        break;
                    }
                }
                if (referenced)
                    break;
            }
            if (referenced)
            {
                if (result)
                    return nullptr;
                result = reinterpret_cast<ReadData*>(slot);
            }
        }
        return result;
    }

    inline void Bind()
    {
        if (NativeSlot || !FrameReady || !Enabled || !*Enabled)
            return;
        const auto now = GetTickCount64();
        if (now < NextProbe)
            return;
        NextProbe = now + 500;
        DWORD needed = 0;
        if (!EnumProcessModules(GetCurrentProcess(), nullptr, 0, &needed))
            return;
        std::vector<HMODULE> modules(needed / sizeof(HMODULE) + 16);
        if (!EnumProcessModules(GetCurrentProcess(), modules.data(), DWORD(modules.size() * sizeof(HMODULE)), &needed))
            return;
        wchar_t windows[MAX_PATH]{};
        if (!GetWindowsDirectoryW(windows, MAX_PATH))
            return;
        const std::wstring system = std::wstring(windows) + L"\\System32\\dinput8.dll";
        const std::wstring wow = std::wstring(windows) + L"\\SysWOW64\\dinput8.dll";
        const auto api = reinterpret_cast<ReadData>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetRawInputData"));
        if (!api)
        {
            OutputDebugStringA("Scarface: raw mouse API unavailable.\n");
            NextProbe = UINT64_MAX;
            return;
        }
        for (size_t i = 0; i < std::min<size_t>(modules.size(), needed / sizeof(HMODULE)); ++i)
        {
            wchar_t path[MAX_PATH]{};
            if (!GetModuleFileNameW(modules[i], path, MAX_PATH) || (_wcsicmp(path, system.c_str()) && _wcsicmp(path, wow.c_str())))
                continue;
            const auto slot = FindSlot(modules[i], api);
            if (slot)
            {
                HMODULE retained = nullptr;
                // Keep the provider alive while its callback is installed.
                if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                    reinterpret_cast<LPCWSTR>(slot), &retained))
                    return;
                Original.store(*slot, std::memory_order_release);
                NativeSlot = slot;
                ProtectedGameRef<ReadData>{ slot } = Read;
            }
            if (!NativeSlot && ++FailedProbes >= 10)
            {
                NextProbe = UINT64_MAX;
                if (!ProbeReported)
                {
                    OutputDebugStringA("Scarface: native raw mouse callback unavailable; using cursor input.\n");
                    ProbeReported = true;
                }
            }
            return;
        }
    }

    inline void Clear()
    {
        Pending.exchange(0, std::memory_order_acquire);
        Motion = {};
        Consumed = 0;
        Sampled = false;
        RawCursorHandler<float>::MouseDeltaX = 0.0f;
        RawCursorHandler<float>::MouseDeltaY = 0.0f;
    }

    inline void EndFrame()
    {
        if (!Sampled || !Enabled || !*Enabled || !Focused())
            Pending.exchange(0, std::memory_order_acquire);
        Motion = {};
        Consumed = 0;
        Sampled = false;
        Bind();
    }

    inline bool Active()
    {
        return FrameReady && Enabled && *Enabled && HaveRaw.load(std::memory_order_acquire);
    }

    inline bool HasMotion(unsigned axes)
    {
        if (Sampled)
            return ((axes & 1) && !(Consumed & 1) && Motion.first != 0.0f) ||
                ((axes & 2) && !(Consumed & 2) && Motion.second != 0.0f);
        const auto counts = Pending.load(std::memory_order_acquire);
        return ((axes & 1) && static_cast<int32_t>(counts) != 0) ||
            ((axes & 2) && static_cast<int32_t>(counts >> 32) != 0);
    }

    inline std::pair<float, float> Consume(unsigned axes, float nativeScale)
    {
        if (!FrameReady)
            Sampled = false;
        if (!Sampled)
        {
            const float multiplier = Sensitivity && std::isfinite(*Sensitivity) ? std::clamp(*Sensitivity, 0.1f, 5.0f) : 1.0f;
            const float scale = nativeScale * multiplier;
            if (FrameReady && Enabled && *Enabled && HaveRaw.load(std::memory_order_acquire))
            {
                const auto counts = Pending.exchange(0, std::memory_order_acquire);
                Motion = { static_cast<int32_t>(counts) * scale, -static_cast<float>(static_cast<int32_t>(counts >> 32)) * scale };
                RECT client{};
                if (Focused() && GetClientRect(*Window, &client))
                {
                    POINT center{ (client.right - client.left) / 2, (client.bottom - client.top) / 2 };
                    if (ClientToScreen(*Window, &center))
                        SetCursorPos(center.x, center.y);
                }
            }
            else
            {
                RawCursorHandler<float>::SetSensitivity(scale);
                RawCursorHandler<float>::UpdateMouseInput(false);
                Motion = { RawCursorHandler<float>::MouseDeltaX, RawCursorHandler<float>::MouseDeltaY };
                RawCursorHandler<float>::MouseDeltaX = 0.0f;
                RawCursorHandler<float>::MouseDeltaY = 0.0f;
            }
            Sampled = true;
            Consumed = 0;
        }
        const std::pair<float, float> result{ (axes & 1) && !(Consumed & 1) ? Motion.first : 0.0f,
            (axes & 2) && !(Consumed & 2) ? Motion.second : 0.0f };
        Consumed |= axes;
        return result;
    }

    inline void Install(const int& enabled, const float& sensitivity, HWND* window, bool frameReady)
    {
        Enabled = &enabled;
        Sensitivity = &sensitivity;
        Window = window;
        FrameReady = frameReady;
        Bind();
    }

    inline void Shutdown()
    {
        if (NativeSlot && *NativeSlot == Read)
            ProtectedGameRef<ReadData>{ NativeSlot } = Original.load(std::memory_order_acquire);
        NativeSlot = nullptr;
        FrameReady = false;
        HaveRaw.store(false, std::memory_order_release);
        Clear();
    }
}
