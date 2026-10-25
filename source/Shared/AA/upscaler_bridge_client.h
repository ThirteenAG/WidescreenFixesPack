#pragma once

// Plugin side of the upscaler bridge (see upscaler_bridge_protocol.h): finds FusionFixHub.exe and the vendor
// runtimes in the game's folder (the folder of the game's exe), starts the hub, and sends it the requests. Header only, for 32- and 64-bit
// plugins; copy it unchanged with upscaler_bridge_protocol.h. The GPU side (copying the game's textures into the
// shared ones) belongs to the plugin, it depends on the game's renderer.

#include <windows.h>
#include <cstring>
#include <filesystem>
#include <string>
#include "upscaler_bridge_protocol.h"

namespace UpscalerBridge
{
    // The game's folder: where the game's exe, the ASI loader, FusionFixHub.exe and the vendor runtimes are
    inline std::filesystem::path GameRoot()
    {
        wchar_t path[MAX_PATH * 4] = {};
        GetModuleFileNameW(nullptr, path, DWORD(std::size(path)));
        return std::filesystem::path(path).parent_path();
    }

    inline std::filesystem::path HubPath(const std::filesystem::path& gameRoot)
    {
        return gameRoot / HubFileName;
    }

    // The one folder the hub loads the vendor runtimes from: the game's folder itself (the folder of the game's exe)
    inline std::filesystem::path RuntimeDirectory(const std::filesystem::path& gameRoot)
    {
        return gameRoot;
    }

    // Whether all files of a backend's runtime are there
    inline bool RuntimePresent(const std::filesystem::path& gameRoot, Backend backend)
    {
        std::error_code ec;
        auto dir = RuntimeDirectory(gameRoot);
        if (backend == Backend::DLSS)
        {
            for (auto name : DLSSFiles)
                if (!std::filesystem::is_regular_file(dir / name, ec))
                    return false;
            return true;
        }
        for (auto name : FSRFiles)
            if (!std::filesystem::is_regular_file(dir / name, ec))
                return false;
        return true;
    }

    // Whether the hub can be started at all and has a runtime to run: without FusionFixHub.exe or the vendor
    // runtimes there is nothing to offer. nvidiaGpu: DLSS needs an NVIDIA GPU, the hub checks the rest.
    inline bool Present(const std::filesystem::path& gameRoot, bool nvidiaGpu)
    {
        std::error_code ec;
        if (!std::filesystem::is_regular_file(HubPath(gameRoot), ec))
            return false;
        return (nvidiaGpu && RuntimePresent(gameRoot, Backend::DLSS)) || RuntimePresent(gameRoot, Backend::FSR);
    }

    class Client
    {
    public:
        HANDLE process = nullptr;
        HANDLE job = nullptr;
        HANDLE mapping = nullptr;
        HANDLE request = nullptr;
        HANDLE response = nullptr;
        Shared* shared = nullptr;
        uint32_t serial = 0;

        ~Client()
        {
            Stop(false);
            if (shared)
                UnmapViewOfFile(shared);
            for (auto handle : { mapping, request, response })
                if (handle)
                    CloseHandle(handle);
        }

        // name: a unique prefix for the shared objects, e.g. L"Local\\MyPlugin.Upscaler"
        bool Start(const std::filesystem::path& gameRoot, const LUID& luid, const std::wstring& prefix, const char* clientName, const std::filesystem::path& logPath)
        {
            auto name = prefix + L"." + std::to_wstring(GetCurrentProcessId()) + L"." + std::to_wstring(GetTickCount64());

            mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(Shared), (name + MappingSuffix).c_str());
            request = CreateEventW(nullptr, FALSE, FALSE, (name + RequestSuffix).c_str());
            response = CreateEventW(nullptr, FALSE, FALSE, (name + ResponseSuffix).c_str());
            if (!mapping || !request || !response)
                return false;

            shared = static_cast<Shared*>(MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Shared)));
            if (!shared)
                return false;

            std::memset(shared, 0, sizeof(Shared));
            shared->Version = Version;
            shared->ClientProcessId = GetCurrentProcessId();
            shared->AdapterLuidLow = luid.LowPart;
            shared->AdapterLuidHigh = luid.HighPart;
            wcsncpy_s(shared->GameDirectory, gameRoot.wstring().c_str(), _TRUNCATE);
            wcsncpy_s(shared->LogPath, logPath.wstring().c_str(), _TRUNCATE);
            strncpy_s(shared->ClientName, clientName ? clientName : "", _TRUNCATE);

            // The hub ends with the game
            job = CreateJobObjectW(nullptr, nullptr);
            if (job)
            {
                JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
                limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
                SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits));
            }

            auto exe = HubPath(gameRoot);
            auto commandLine = L"\"" + exe.wstring() + L"\" " + ArgumentName + L" " + name;
            STARTUPINFOW startup{ sizeof(startup) };
            PROCESS_INFORMATION info{};
            if (!CreateProcessW(exe.c_str(), commandLine.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr,
                gameRoot.c_str(), &startup, &info))
                return false;

            if (job)
                AssignProcessToJobObject(job, info.hProcess);
            ResumeThread(info.hThread);
            CloseHandle(info.hThread);
            process = info.hProcess;
            return true;
        }

        // Returns true once the hub answered its start; ok tells whether it runs
        bool PollStarted(bool& ok)
        {
            if (WaitForSingleObject(response, 0) != WAIT_OBJECT_0)
            {
                if (WaitForSingleObject(process, 0) == WAIT_OBJECT_0)
                {
                    ok = false;
                    return true;
                }
                return false;
            }
            MemoryBarrier();
            ok = shared->ResponseStatus == Status::Ok;
            return true;
        }

        bool IsAvailable(Backend backend) const
        {
            return shared && (shared->AvailableBackends & BackendBit(backend)) != 0;
        }

        bool Exited() const
        {
            return !process || WaitForSingleObject(process, 0) == WAIT_OBJECT_0;
        }

        bool Request(Command command, DWORD timeout)
        {
            shared->RequestCommand = command;
            shared->RequestSerial = ++serial;
            MemoryBarrier();
            SetEvent(request);

            auto start = GetTickCount64();
            while (true)
            {
                auto elapsed = static_cast<DWORD>(GetTickCount64() - start);
                if (elapsed >= timeout)
                    return false;

                HANDLE handles[] = { response, process };
                auto wait = WaitForMultipleObjects(2, handles, FALSE, timeout - elapsed);
                if (wait != WAIT_OBJECT_0)
                    return false;

                MemoryBarrier();
                // A late answer to an earlier request is skipped
                if (shared->ResponseSerial == serial)
                    return shared->ResponseStatus == Status::Ok;
            }
        }

        // The hub duplicated its handles into this process, they are closed once imported or not
        void CloseSharedHandles()
        {
            for (auto& handle : shared->TextureHandles)
            {
                if (handle)
                    CloseHandle(reinterpret_cast<HANDLE>(handle));
                handle = 0;
            }
            if (shared->FenceHandle)
                CloseHandle(reinterpret_cast<HANDLE>(shared->FenceHandle));
            shared->FenceHandle = 0;
        }

        // graceful: ask the hub to release everything first, not while the game is being unloaded
        void Stop(bool graceful)
        {
            if (graceful && shared && process && WaitForSingleObject(process, 0) == WAIT_TIMEOUT)
                Request(Command::Shutdown, 500);
            if (process)
                CloseHandle(process);
            if (job)
                CloseHandle(job);
            process = job = nullptr;
        }
    };
}
