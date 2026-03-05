module;

#include <stdafx.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

export module LAN;

import ComVars;

std::string getLocalIPAddress()
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        DBGONLY(spd::log()->error("WSAStartup failed: {0:x}", result);)
            return "";
    }

    struct sockaddr_in destination
    {
    };
    destination.sin_family = AF_INET;
    destination.sin_port = htons(80);
    inet_pton(AF_INET, "8.8.8.8", &destination.sin_addr);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET)
    {
        DBGONLY(spd::log()->error("Socket creation failed.");)
            WSACleanup();
        return "";
    }

    result = connect(sock, reinterpret_cast<struct sockaddr*>(&destination), sizeof(destination));
    if (result == SOCKET_ERROR)
    {
        DBGONLY(spd::log()->error("Socket connection failed.");)
            closesocket(sock);
        WSACleanup();
        return "";
    }

    struct sockaddr_in localAddress
    {
    };
    int addressLength = sizeof(localAddress);
    result = getsockname(sock, reinterpret_cast<struct sockaddr*>(&localAddress), &addressLength);
    if (result == SOCKET_ERROR)
    {
        DBGONLY(spd::log()->error("getsockname failed.");)
            closesocket(sock);
        WSACleanup();
        return "";
    }

    std::string ipAddress(INET_ADDRSTRLEN, '\0');
    inet_ntop(AF_INET, &localAddress.sin_addr, ipAddress.data(), ipAddress.size());

    closesocket(sock);
    WSACleanup();

    return std::string(ipAddress.c_str());
}

void LocalServer()
{
    WSADATA wsaData;
    SOCKET sockserv, sock;
    struct sockaddr_in server, client;
    int c = sizeof(struct sockaddr_in);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        DBGONLY(spd::log()->error("Failed. Error Code : {0:x}", WSAGetLastError());)
            return;
    }

    // Create a socket
    if ((sockserv = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        DBGONLY(spd::log()->error("Could not create socket : {0:x}", WSAGetLastError());)
            WSACleanup();
        return;
    }

    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(3074);

    // Bind
    if (bind(sockserv, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
    {
        DBGONLY(spd::log()->error("Bind failed with error code : {0:x}", WSAGetLastError());)
            closesocket(sockserv);
        WSACleanup();
        return;
    }

    // Listen to incoming connections
    listen(sockserv, 10);

    // Accept and incoming connection
    DBGONLY(spd::log()->info("Waiting for incoming connections...");)

        while ((sock = accept(sockserv, (struct sockaddr*)&client, &c)) != INVALID_SOCKET)
        {
            DBGONLY(spd::log()->info("Connection accepted");)
                char client_message[1024] = { 0 };
            int recv_size;
            if ((recv_size = recv(sock, client_message, 1024, 0)) == SOCKET_ERROR)
            {
                DBGONLY(spd::log()->error("recv failed");)
            }
            DBGONLY(spd::log()->info("Client says: {}", client_message);)

                auto h = "HTTP/1.1 200 OK\r\n"
                "Cache-Control: priva"
                "te\r\n"
                "Content-Type: text/h"
                "tml; charset=utf-8\r"
                "\n"
                "Server: Microsoft-II"
                "S/10.0\r\n"
                "X-AspNet-Version: 2."
                "0.50727\r\n"
                "X-Powered-By: ASP.NE"
                "T\r\n"
                "Date: Mon, 01 Jan 20"
                "24 23:10:02 GMT\r\n"
                "Content-Length: 1183"
                "\r\n\r\n";

            auto r = "<RESPONSE xmlns=\"\""
                "><AuthenticationServ"
                "er><VALUE>lb-agora.u"
                "bisoft.com:3081</VAL"
                "UE></AuthenticationS"
                "erver><CreateAccount"
                "><VALUE>https://secu"
                "re.ubi.com/login/Cre"
                "ateUser.aspx?lang=%s"
                "</VALUE></CreateAcco"
                "unt><LobbyServer><VA"
                "LUE>lb-lsg-prod.ubis"
                "oft.com:3105</VALUE>"
                "</LobbyServer><MmpTi"
                "tleId><VALUE>0xA004<"
                "/VALUE></MmpTitleId>"
                "<SandboxUrl><VALUE>p"
                "rudp:/address=lb-rdv"
                "-as-prod01.ubisoft.c"
                "om;port=23931</VALUE"
                "></SandboxUrl><Sandb"
                "oxUrlWS><VALUE>ne1-z"
                "3-as-rdv03.ubisoft.c"
                "om:23930</VALUE></Sa"
                "ndboxUrlWS><SerialNa"
                "me><VALUE>SPLINTERCE"
                "LL5PC</VALUE></Seria"
                "lName><uplay_Downloa"
                "dServiceUrl><VALUE>h"
                "ttps://secure.ubi.co"
                "m/UplayServices/Upla"
                "yFacade/DownloadServ"
                "icesRESTXML.svc/REST"
                "/XML/?url=</VALUE></"
                "uplay_DownloadServic"
                "eUrl><uplay_DynConte"
                "ntBaseUrl><VALUE>htt"
                "p://static8.cdn.ubi."
                "com/u/Uplay/</VALUE>"
                "</uplay_DynContentBa"
                "seUrl><uplay_DynCont"
                "entSecureBaseUrl><VA"
                "LUE>http://static8.c"
                "dn.ubi.com/</VALUE><"
                "/uplay_DynContentSec"
                "ureBaseUrl><uplay_Pa"
                "ckageBaseUrl><VALUE>"
                "http://static8.cdn.u"
                "bi.com/u/Uplay/Packa"
                "ges/1.0.1/</VALUE></"
                "uplay_PackageBaseUrl"
                "><uplay_WebServiceBa"
                "seUrl><VALUE>https:/"
                "/secure.ubi.com/Upla"
                "yServices/UplayFacad"
                "e/ProfileServicesFac"
                "adeRESTXML.svc/REST/"
                "</VALUE></uplay_WebS"
                "erviceBaseUrl></RESP"
                "ONSE>";

            std::string matchmaking_config = std::string(h) + std::string(r);
            send(sock, matchmaking_config.c_str(), matchmaking_config.size(), 0);
            closesocket(sock);
        }

    if (sock == INVALID_SOCKET)
    {
        DBGONLY(spd::log()->error("accept failed with error code : {0:x}", WSAGetLastError());)
    }

    closesocket(sockserv);
    WSACleanup();
}

namespace Os
{
    namespace CoreTaskGetGameConnectSettings
    {
        SafetyHookInline shonSendRequest{};
        void __fastcall onSendRequest(int _this, void* edx, int a2, int a3, char* response, char* header, int a6, int a7)
        {
            return shonSendRequest.fastcall<void>(_this, edx, a2, a3, response, header, a6, a7);
        }
    }

    namespace NetworkTaskHTTPSend
    {
        SafetyHookInline shparseHeader{};
        uint8_t __fastcall parseHeader(void* _this, void* edx)
        {
            return shparseHeader.fastcall<uint8_t>(_this, edx);
        }

        SafetyHookInline shreadResponse{};
        uint8_t __fastcall readResponse(void* _this, void* edx)
        {
            return shreadResponse.fastcall<uint8_t>(_this, edx);
        }
    }
}

int WINAPI sendhook(SOCKET s, const char* buf, int len, int flags)
{
    return send(s, buf, len, flags);
}

int WINAPI sendtohook(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen)
{
    if (to->sa_family == AF_INET)
    {
        auto addr = getLocalIPAddress();
        inet_pton(AF_INET, addr.c_str(), const_cast<char*>(buf) + 93);
    }

    return sendto(s, buf, len, flags, to, tolen);
}

int WINAPI WSASendToHook(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, const struct sockaddr* lpTo, int iTolen, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    return WSASendTo(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpTo, iTolen, lpOverlapped, lpCompletionRoutine);
}

export void InitLAN()
{
    CIniReader iniReader("");
    auto sLANHelperExePath = iniReader.ReadString("LAN", "LANHelperExePath", "");
    auto bFixLAN = iniReader.ReadInteger("LAN", "FixLAN", 1) != 0;
    static auto sServerAddr = iniReader.ReadString("LAN", "ServerAddr", "127.0.0.1");

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
        auto pattern = hook::pattern("E8 ? ? ? ? 89 5D FC 33 FF");
        static auto MatchMakingConfigHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            static std::once_flag flag;
            std::call_once(flag, []()
            {
                std::thread(LocalServer).detach();
            });
        });

        pattern = hook::pattern("FF 15 ? ? ? ? 8B F8 83 FF FF 75 0D FF 15 ? ? ? ? E8 ? ? ? ? 8B D8 85 DB 8B C7 74 03 89 5E 04 5B 5F 5E C2 0C 00");
        injector::MakeNOP(pattern.get_first(), 6, true);
        injector::MakeCALL(pattern.get_first(), sendtohook, true);

        //Os::NetworkTaskHTTPSend::shparseHeader = safetyhook::create_inline(0xC14154, Os::NetworkTaskHTTPSend::parseHeader);
        //Os::NetworkTaskHTTPSend::shreadResponse = safetyhook::create_inline(0xC1475C, Os::NetworkTaskHTTPSend::readResponse);
        //Os::CoreTaskGetGameConnectSettings::shonSendRequest = safetyhook::create_inline(0xC0ED58, Os::CoreTaskGetGameConnectSettings::onSendRequest);
    }

    if (!sServerAddr.empty())
    {
        auto pattern = hook::pattern("68 ? ? ? ? 8D 4D E0 E8 ? ? ? ? C7 45 ? ? ? ? ? E8");
        injector::WriteMemory(pattern.get_first(1), sServerAddr.data(), true);

        pattern = hook::pattern("68 ? ? ? ? 8B CE E8 ? ? ? ? 68 ? ? ? ? 8B CE E8 ? ? ? ? 8B C6");
        injector::WriteMemory(pattern.get_first(1), sServerAddr.data(), true);
    }
}