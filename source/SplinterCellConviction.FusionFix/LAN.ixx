module;

#include <stdafx.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <fstream>
#include <deque>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

export module LAN;

import ComVars;

// LAN play goes through Ubisoft's Agora SDK:
// - Os::CoreTaskGetGameConnectSettings gets the matchmaking config from gconnect.ubi.com (GET /MatchMakingConfig.aspx on port 3074),
//   without it the LAN menu can't be entered. The host name is replaced by ServerAddr (127.0.0.1) and a local server answers with the config.
// - Os::Agora::LANMessageManager::broadcastMessage sends every LAN message (search, get session, session reply) to 255.255.255.255,
//   which Windows sends out of one network adapter only, with several adapters (VPN, virtual machines) the other PC may not get it.
//   It's sent to the broadcast address of every adapter instead.
// - The session reply has the host's addresses (LANSessionInfo +8), Os::CoreTaskUpdateNetworkInformation adds the address of every
//   adapter and 127.0.0.1, the joining PC can connect to one it can't reach. They're replaced with the address the reply came from.
namespace LAN
{
    // Matchmaking config server
    constexpr uint16_t ConfigPort = 3074;
    constexpr std::string_view ConfigResponse =
        "<RESPONSE xmlns=\"\"><AuthenticationServer><VALUE>lb-agora.ubisoft.com:3081</VALUE></AuthenticationServer>"
        "<CreateAccount><VALUE>https://secure.ubi.com/login/CreateUser.aspx?lang=%s</VALUE></CreateAccount>"
        "<LobbyServer><VALUE>lb-lsg-prod.ubisoft.com:3105</VALUE></LobbyServer>"
        "<MmpTitleId><VALUE>0xA004</VALUE></MmpTitleId>"
        "<SandboxUrl><VALUE>prudp:/address=lb-rdv-as-prod01.ubisoft.com;port=23931</VALUE></SandboxUrl>"
        "<SandboxUrlWS><VALUE>ne1-z3-as-rdv03.ubisoft.com:23930</VALUE></SandboxUrlWS>"
        "<SerialName><VALUE>SPLINTERCELL5PC</VALUE></SerialName>"
        "<uplay_DownloadServiceUrl><VALUE>https://secure.ubi.com/UplayServices/UplayFacade/DownloadServicesRESTXML.svc/REST/XML/?url=</VALUE></uplay_DownloadServiceUrl>"
        "<uplay_DynContentBaseUrl><VALUE>http://static8.cdn.ubi.com/u/Uplay/</VALUE></uplay_DynContentBaseUrl>"
        "<uplay_DynContentSecureBaseUrl><VALUE>http://static8.cdn.ubi.com/</VALUE></uplay_DynContentSecureBaseUrl>"
        "<uplay_PackageBaseUrl><VALUE>http://static8.cdn.ubi.com/u/Uplay/Packages/1.0.1/</VALUE></uplay_PackageBaseUrl>"
        "<uplay_WebServiceBaseUrl><VALUE>https://secure.ubi.com/UplayServices/UplayFacade/ProfileServicesFacadeRESTXML.svc/REST/</VALUE></uplay_WebServiceBaseUrl>"
        "</RESPONSE>";

    void ConfigServer(bool loopbackOnly)
    {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            return;

        auto server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        sockaddr_in address = {};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(loopbackOnly ? INADDR_LOOPBACK : INADDR_ANY); // other PCs can use this one with ServerAddr
        address.sin_port = htons(ConfigPort);
        // with two instances on one PC the port is taken, the first one's server answers both
        if (server == INVALID_SOCKET || bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR || listen(server, SOMAXCONN) == SOCKET_ERROR)
        {
            if (server != INVALID_SOCKET)
                closesocket(server);
            WSACleanup();
            return;
        }

        auto response = std::format("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{}", ConfigResponse.size(), ConfigResponse);
        SOCKET client;
        while ((client = accept(server, nullptr, nullptr)) != INVALID_SOCKET)
        {
            char request[1024];
            recv(client, request, sizeof(request), 0);
            send(client, response.data(), static_cast<int>(response.size()), 0);
            shutdown(client, SD_SEND);
            closesocket(client);
        }
        closesocket(server);
        WSACleanup();
    }

    // Broadcast address of every IPv4 adapter that is up (the adapter's address with all host bits set)
    std::vector<in_addr> GetBroadcastAddresses()
    {
        std::vector<in_addr> result;
        ULONG size = 16 * 1024;
        std::vector<uint8_t> buffer;
        ULONG error;
        do
        {
            buffer.resize(size);
            error = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()), &size);
        } while (error == ERROR_BUFFER_OVERFLOW);
        if (error != NO_ERROR)
            return result;

        for (auto adapter = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()); adapter; adapter = adapter->Next)
        {
            if (adapter->OperStatus != IfOperStatusUp || adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
                continue;
            for (auto unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next)
            {
                auto ip = ntohl(reinterpret_cast<sockaddr_in*>(unicast->Address.lpSockaddr)->sin_addr.s_addr);
                auto mask = unicast->OnLinkPrefixLength ? ~0u << (32 - unicast->OnLinkPrefixLength) : 0u;
                in_addr broadcast = {};
                broadcast.s_addr = htonl(ip | ~mask);
                if (std::ranges::none_of(result, [&](auto& a) { return a.s_addr == broadcast.s_addr; }))
                    result.push_back(broadcast);
            }
        }
        return result;
    }

    int WINAPI sendto(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen)
    {
        auto destination = reinterpret_cast<const sockaddr_in*>(to);
        if (to->sa_family != AF_INET || destination->sin_addr.s_addr != INADDR_BROADCAST)
            return ::sendto(s, buf, len, flags, to, tolen);

        // adapters can change while the game runs (VPN connected...)
        static std::vector<in_addr> broadcastAddresses;
        static auto lastUpdate = std::chrono::steady_clock::time_point();
        if (auto now = std::chrono::steady_clock::now(); now - lastUpdate > std::chrono::seconds(5))
        {
            broadcastAddresses = GetBroadcastAddresses();
            lastUpdate = now;
        }

        int result = SOCKET_ERROR;
        for (auto& address : broadcastAddresses)
        {
            auto adapterDestination = *destination;
            adapterDestination.sin_addr = address;
            if (auto sent = ::sendto(s, buf, len, flags, reinterpret_cast<sockaddr*>(&adapterDestination), sizeof(adapterDestination)); sent != SOCKET_ERROR)
                result = sent;
        }
        return result == SOCKET_ERROR ? ::sendto(s, buf, len, flags, to, tolen) : result;
    }

    // A broadcast sent on every adapter comes back once per adapter on the same PC, the copies are dropped (the game would list a lobby twice)
    // Sender of the last datagram, the game's own address of it is built with the wrong byte order (it's only logged)
    uint32_t lastSender = 0;

    int WINAPI recvfrom(SOCKET s, char* buf, int len, int flags, sockaddr* from, int* fromlen)
    {
        auto received = ::recvfrom(s, buf, len, flags, from, fromlen);
        if (received > 0)
        {
            if (from && from->sa_family == AF_INET)
                lastSender = reinterpret_cast<sockaddr_in*>(from)->sin_addr.s_addr;

            static std::deque<std::pair<size_t, std::chrono::steady_clock::time_point>> recent;
            auto now = std::chrono::steady_clock::now();
            std::erase_if(recent, [&](auto& r) { return now - r.second > std::chrono::milliseconds(250); });
            auto hash = std::hash<std::string_view>{}(std::string_view(buf, received));
            if (std::ranges::any_of(recent, [&](auto& r) { return r.first == hash; }))
                memset(buf, 0, received); // not an Agora message anymore, it's ignored
            else
                recent.emplace_back(hash, now);
        }
        return received;
    }

    // Agora log (Os::Log), level 0 (everything) when it's written to a file
    std::ofstream logFile;
    SafetyHookInline shLog = {};
    void __cdecl Log(const char* category, int32_t level, const char* file, const char* function, int32_t line, void* message)
    {
        if (!logFile.is_open())
            logFile.open(GetExeModulePath<std::filesystem::path>() / (bInstance1 ? "Conviction_LAN.log" : "Conviction_LAN2.log"));
        // Gear::GearBasicString: rep at +4, rep: length at +4, data at +12
        std::string_view text;
        if (auto rep = message ? *reinterpret_cast<uint8_t**>(reinterpret_cast<uintptr_t>(message) + 4) : nullptr)
            text = std::string_view(*reinterpret_cast<const char**>(rep + 12), *reinterpret_cast<int32_t*>(rep + 4));
        auto time = std::chrono::zoned_time(std::chrono::current_zone(), std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now()));
        logFile << std::format("{:%H:%M:%S} [{}] {} {}: {}", time, level, category ? category : "", function ? function : "", text) << std::endl;
        shLog.ccall<void>(category, level, file, function, line, message);
    }

    SafetyHookInline shLogLevel = {};
    int32_t __cdecl LogLevel()
    {
        return 0;
    }

    // Address a session reply came from, while it's handled (Os::Agora address: IPv4 in network byte order, port at +4)
    std::optional<uint32_t> replySource;

    SafetyHookInline shDeserializeSessionInfo = {};
    bool __fastcall DeserializeSessionInfo(uintptr_t sessionInfo, void* edx, void* buffer)
    {
        auto result = shDeserializeSessionInfo.fastcall<bool>(sessionInfo, edx, buffer);
        if (result && replySource)
        {
            // host addresses: count at +8, pointer to 8 byte addresses at +0Ch, the port of each is kept
            auto hostAddresses = sessionInfo + 8;
            auto count = *reinterpret_cast<uint32_t*>(hostAddresses + 8);
            auto addresses = *reinterpret_cast<uint8_t**>(hostAddresses + 0x0C);
            for (uint32_t i = 0; addresses && i < count; i++)
                *reinterpret_cast<uint32_t*>(addresses + i * 8) = *replySource;
        }
        return result;
    }
}

export void InitLAN()
{
    CIniReader iniReader("");
    auto sLANHelperExePath = iniReader.ReadString("LAN", "LANHelperExePath", "");
    auto bFixLAN = iniReader.ReadInteger("LAN", "FixLAN", 1) != 0;
    static auto sServerAddr = iniReader.ReadString("LAN", "ServerAddr", "127.0.0.1");
    auto bLog = iniReader.ReadInteger("LAN", "Log", 0) != 0;

    if (!sLANHelperExePath.empty())
    {
        std::error_code ec;
        auto dsPath = std::filesystem::path(sLANHelperExePath);
        auto processPath = dsPath.is_absolute() ? dsPath : (GetExeModulePath() / dsPath);
        auto workingDir = std::filesystem::path(processPath).remove_filename();
        if (std::filesystem::exists(processPath, ec))
        {
            HANDLE hJob = CreateJobObject(nullptr, nullptr);
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = { };
            info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &info, sizeof(info));
            PROCESS_INFORMATION pi;
            if (CreateProcessInJobAsAdmin(hJob,
                processPath.c_str(),
                NULL,
                SW_HIDE,
                workingDir.c_str(), &pi))
            {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }
    }

    if (bFixLAN)
    {
        // Quazal::DSoundSource destructors (voice chat): leaving the LAN lobby released an object that was already freed (+0Ch or +10h,
        // the call went through a garbage vtable). Objects whose Release (vtable +8) isn't code of a module are left alone.
        static auto IsCode = [](uintptr_t address)
        {
            MEMORY_BASIC_INFORMATION info{};
            return address && VirtualQuery(reinterpret_cast<void*>(address), &info, sizeof(info)) && info.State == MEM_COMMIT && info.Type == MEM_IMAGE &&
                (info.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY));
        };
        static auto IsReadable = [](uintptr_t address)
        {
            MEMORY_BASIC_INFORMATION info{};
            return address && VirtualQuery(reinterpret_cast<void*>(address), &info, sizeof(info)) && info.State == MEM_COMMIT &&
                !(info.Protect & (PAGE_NOACCESS | PAGE_GUARD));
        };
        auto soundSource = hook::pattern("56 8B F1 8B 46 10 85 C0 C7 06 ? ? ? ? 74 08 8B 08 8B 51 08 50 FF D2 8B 46 0C 85 C0 74 08 8B 08 8B 51 08 50 FF D2");
        static std::vector<SafetyHookMid> SoundSourceDestructors;
        soundSource.for_each_result([](hook::pattern_match match)
        {
            SoundSourceDestructors.push_back(safetyhook::create_mid(match.get<void>(3), [](SafetyHookContext& regs)
            {
                for (auto offset : { 0x0C, 0x10 })
                {
                    auto& object = *reinterpret_cast<uintptr_t*>(regs.esi + offset);
                    if (!object)
                        continue;
                    auto vtable = IsReadable(object) ? *reinterpret_cast<uintptr_t*>(object) : 0;
                    if (!IsReadable(vtable + 8) || !IsCode(*reinterpret_cast<uintptr_t*>(vtable + 8)))
                        object = 0;
                }
            }));
        });

        // the game connects before it sends the request, the server has to be up first
        std::thread(LAN::ConfigServer, sServerAddr == "127.0.0.1").detach();

        // LANMessageManager::broadcastMessage
        auto pattern = hook::pattern("FF 15 ? ? ? ? 8B F8 83 FF FF 75 0D FF 15 ? ? ? ? E8 ? ? ? ? 8B D8 85 DB 8B C7 74 03 89 5E 04 5B 5F 5E C2 0C 00");
        injector::MakeNOP(pattern.get_first(), 6, true);
        injector::MakeCALL(pattern.get_first(), LAN::sendto, true);

        // LANMessageManager::receive
        pattern = hook::pattern("C7 44 24 24 10 00 00 00 FF 15 ? ? ? ? 8B F8 85 FF 74 05 83 FF FF");
        injector::MakeNOP(pattern.get_first(8), 6, true);
        injector::MakeCALL(pattern.get_first(8), LAN::recvfrom, true);

        // LANMessageManager::receive, session reply handler
        pattern = hook::pattern("8D 45 E0 50 FF 75 F0 E8 ? ? ? ? E9");
        static auto SessionReplyStart = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
        {
            LAN::replySource = LAN::lastSender;
        });
        static auto SessionReplyEnd = safetyhook::create_mid(pattern.get_first(12), [](SafetyHookContext& regs)
        {
            LAN::replySource.reset();
        });

        // LANSessionInfo deserialize
        pattern = hook::pattern("6A 70 B8 ? ? ? ? E8 ? ? ? ? 8B F1 8B 7D 08 33 DB 43 53");
        LAN::shDeserializeSessionInfo = safetyhook::create_inline(pattern.get_first(), LAN::DeserializeSessionInfo);
    }

    if (bLog)
    {
        auto pattern = hook::pattern("55 8B EC 51 53 56 57 E8 ? ? ? ? 8B 70 34 83 C0 2C 33 DB");
        auto logger = injector::GetBranchDestination(pattern.get_first(7)).as_int();
        LAN::shLog = safetyhook::create_inline(pattern.get_first(), LAN::Log);

        // the level getter calls the same logger getter
        pattern = hook::pattern("E8 ? ? ? ? 8B 40 28 C3");
        for (size_t i = 0; i < pattern.size(); i++)
        {
            if (injector::GetBranchDestination(pattern.get(i).get<void>(0)).as_int() == logger)
            {
                LAN::shLogLevel = safetyhook::create_inline(pattern.get(i).get<void>(0), LAN::LogLevel);
                break;
            }
        }
    }

    // gconnect.ubi.com
    if (!sServerAddr.empty())
    {
        auto pattern = hook::pattern("68 ? ? ? ? 8D 4D E0 E8 ? ? ? ? C7 45 ? ? ? ? ? E8");
        injector::WriteMemory(pattern.get_first(1), sServerAddr.data(), true);

        pattern = hook::pattern("68 ? ? ? ? 8B CE E8 ? ? ? ? 68 ? ? ? ? 8B CE E8 ? ? ? ? 8B C6");
        injector::WriteMemory(pattern.get_first(1), sServerAddr.data(), true);
    }
}
