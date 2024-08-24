#include "stdafx.h"
#include <d3d9.h>
#include <LEDEffects.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#include <list>

std::string getLocalIPAddress()
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        DBGONLY(spd::log()->error("WSAStartup failed: {0:x}", result);)
        return "";
    }

    struct sockaddr_in destination {};
    destination.sin_family = AF_INET;
    destination.sin_port = htons(80);
    inet_pton(AF_INET, "8.8.8.8", &destination.sin_addr);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        DBGONLY(spd::log()->error("Socket creation failed.");)
        WSACleanup();
        return "";
    }

    result = connect(sock, reinterpret_cast<struct sockaddr*>(&destination), sizeof(destination));
    if (result == SOCKET_ERROR) {
        DBGONLY(spd::log()->error("Socket connection failed.");)
        closesocket(sock);
        WSACleanup();
        return "";
    }

    struct sockaddr_in localAddress {};
    int addressLength = sizeof(localAddress);
    result = getsockname(sock, reinterpret_cast<struct sockaddr*>(&localAddress), &addressLength);
    if (result == SOCKET_ERROR) {
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
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        DBGONLY(spd::log()->error("Failed. Error Code : {0:x}", WSAGetLastError());)
        return;
    }

    // Create a socket
    if ((sockserv = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        DBGONLY(spd::log()->error("Could not create socket : {0:x}", WSAGetLastError());)
        WSACleanup();
        return;
    }

    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(3074);

    // Bind
    if (bind(sockserv, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        DBGONLY(spd::log()->error("Bind failed with error code : {0:x}", WSAGetLastError());)
        closesocket(sockserv);
        WSACleanup();
        return;
    }

    // Listen to incoming connections
    listen(sockserv, 10);

    // Accept and incoming connection
    DBGONLY(spd::log()->info("Waiting for incoming connections...");)

    while ((sock = accept(sockserv, (struct sockaddr*)&client, &c)) != INVALID_SOCKET) {
        DBGONLY(spd::log()->info("Connection accepted");)
        char client_message[1024] = { 0 };
        int recv_size;
        if ((recv_size = recv(sock, client_message, 1024, 0)) == SOCKET_ERROR) {
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

    if (sock == INVALID_SOCKET) {
        DBGONLY(spd::log()->error("accept failed with error code : {0:x}", WSAGetLastError());)
    }

    closesocket(sockserv);
    WSACleanup();
}

float gVisibility = 1.0f;
int32_t gBlacklistIndicators = 0;
bool bDisableBlackAndWhiteFilter = false;
bool bBlacklistControlScheme = true;
IDirect3DDevice9* pDevice = nullptr;
int BackBufferWidth = 0;
int BackBufferHeight = 0;
bool bVideoRender = false;
std::list<int> OpenedVideosList;

static constexpr float fDefaultAspectRatio = 16.0f / 9.0f;

static auto GetAspectRatio = []() -> float
{
    if (!BackBufferWidth || !BackBufferHeight)
        return fDefaultAspectRatio;
    return static_cast<float>(BackBufferWidth) / static_cast<float>(BackBufferHeight);
};

bool (WINAPI* GetOverloadedFilePathA)(const char* lpFilename, char* out, size_t out_size) = nullptr;

void OverrideForBlacklistControls(const char* path)
{
    static const std::string blacklistINI = "blacklist.ini";
    static const std::string LocalizationMenus = "Localization\\Menus";

    auto path_view = std::string_view(path);
    auto dest = const_cast<char*>(path);
    if (path_view == "ActionScheme.ini")
    {
        if (GetOverloadedFilePathA(blacklistINI.c_str(), nullptr, 0))
            strcpy_s(dest, blacklistINI.length() + 1, blacklistINI.c_str());
    }
    else if (path_view.contains(LocalizationMenus))
    {
        auto replace_view = [](std::string_view str, std::string_view pattern, std::string_view replacement) -> auto
        {
            return str | std::views::split(pattern) | std::views::join_with(replacement);
        };

        auto new_path = replace_view(path_view, LocalizationMenus, "Blacklist\\Menus") | std::ranges::to<std::string>();
        if (GetOverloadedFilePathA(new_path.c_str(), nullptr, 0))
            strcpy_s(dest, new_path.length() + 1, new_path.c_str());
    }
}

injector::hook_back<void (__cdecl*)(float a1, float a2, int a3)> hb_8330DB;
void __cdecl sub_8330DB(float a1, float a2, int a3)
{
    gVisibility = a1;

    if (!bDisableBlackAndWhiteFilter)
        return hb_8330DB.fun(a1, a2, a3);
}

injector::hook_back<void(__cdecl*)(void* a1, int a2, int a3)> hb_100177B7;
void __cdecl sub_100177B7(void* a1, int a2, int a3)
{
    if (gBlacklistIndicators) //enables bloom in dark areas
    {
        if (gVisibility == 1.0f)
            return hb_100177B7.fun(a1, a2, a3);
        else
            return;
    }
    return hb_100177B7.fun(a1, a2, a3);
}

// Distortion during movement (sonar)
injector::hook_back<void(__fastcall*)(void* self, int edx)> hb_1002581C;
void __fastcall sub_1002581C(void* self, int edx)
{
    //return hb_1002581C.fun(self, edx);
}

namespace FFileManagerArc
{
    SafetyHookInline shLookup{};
    void* __fastcall Lookup(void* pFFileManagerArc, void* edx, const char* path)
    {
        if (bBlacklistControlScheme)
            OverrideForBlacklistControls(path);

        if (GetOverloadedFilePathA(path, nullptr/*s.data()*/, 0/*s.size()*/))
            return nullptr;
        return shLookup.fastcall<void*>(pFFileManagerArc, edx, path);
    }
}

namespace FFileManagerLinear
{
    int* dword_13A334C = nullptr;
    SafetyHookInline shLookup{};
    void* __fastcall Lookup(void* pFFileManagerLinear, void* edx, const char* path, int a3)
    {
        if (bBlacklistControlScheme)
            OverrideForBlacklistControls(path);

        if (GetOverloadedFilePathA(path, nullptr/*s.data()*/, 0/*s.size()*/))
        {
            int cur = *dword_13A334C;
            *dword_13A334C = 1;
            auto ret = shLookup.fastcall<void*>(pFFileManagerLinear, edx, path, a3);
            *dword_13A334C = cur;
            return ret;
        }
        return shLookup.fastcall<void*>(pFFileManagerLinear, edx, path, a3);
    }

    static int curdword_13A3348 = 0;
    int* dword_13A3348 = nullptr;
    static injector::hook_back<void(__fastcall*)(void*, void*, const char*)> hbsub_43A943;
    void __fastcall sub_43A943(void* _this, void* edx, const char* path)
    {
        curdword_13A3348 = *dword_13A3348;

        if (bBlacklistControlScheme)
            OverrideForBlacklistControls(path);

        if (GetOverloadedFilePathA(path, nullptr/*s.data()*/, 0/*s.size()*/))
        {
            *dword_13A3348 = 0;
        }

        return hbsub_43A943.fun(_this, edx, path);
    }
}

int WhichPrecisionModeNeeded = 0;
int CrouchSprint = 0;
int CrouchSprintOff = 0;
int Sprint = 0;
int SprintOff = 0;
bool bSprintNeeded = false;
bool bCrouchSprintNeeded = false;
int TogglePrecisionMode = 0;
int TogglePrecisionModeOn = 0;
int TogglePrecisionModeOff = 0;
static injector::hook_back<void(__fastcall*)(int*, void*, const char*, int)> hb458CD3;
void __fastcall fnTogglePrecisionMode(int* out, void* edx, const char* name, int a3)
{
    hb458CD3.fun(&Sprint, edx, "Sprint", a3);
    hb458CD3.fun(&SprintOff, edx, "SprintOff", a3);
    hb458CD3.fun(&CrouchSprint, edx, "CrouchSprint", a3);
    hb458CD3.fun(&CrouchSprintOff, edx, "CrouchSprintOff", a3);
    hb458CD3.fun(&TogglePrecisionModeOn, edx, "TogglePrecisionModeOn", a3);
    hb458CD3.fun(&TogglePrecisionModeOff, edx, "TogglePrecisionModeOff", a3);
    hb458CD3.fun(out, edx, name, a3);
    TogglePrecisionMode = *out;
}

int DetectB2W = 0;
void __fastcall fnDetectB2W(int* out, void* edx, const char* name, int a3)
{
    hb458CD3.fun(out, edx, name, a3);
    DetectB2W = *out;
}

int StopDetectB2W = 0;
void __fastcall fnStopDetectB2W(int* out, void* edx, const char* name, int a3)
{
    hb458CD3.fun(out, edx, name, a3);
    StopDetectB2W = *out;
}

BOOL __fastcall sub_56C0AD(uint32_t* _this, void* edx)
{
    auto aimcheck = (*(uint8_t*)((uint8_t*)_this + 0xA42));

    // run speed, disable run when player wants to aim
    if (WhichPrecisionModeNeeded == TogglePrecisionModeOn)
    {
        *(float*)&_this[753] = 200.0f;
    }
    else if (WhichPrecisionModeNeeded == TogglePrecisionModeOff)
    {
        *(float*)&_this[753] = 1000000.0f;
    }
    
    enum AimType
    {
        eNormal = 4,
        eFromCover = 5,
        eFirstPerson = 12,
    };

    if ((WhichPrecisionModeNeeded == TogglePrecisionModeOn) && (aimcheck == eNormal || aimcheck == eFromCover || aimcheck == eFirstPerson))
        return true;
    if ((WhichPrecisionModeNeeded == TogglePrecisionModeOff) && (aimcheck != eNormal && aimcheck != eFromCover && aimcheck != eFirstPerson))
        return true;

    return ((_this[655] & 0x100000) != 0) || ((_this[656] & 0x200000) != 0);
}

SafetyHookInline shsub_46E388{};
int __fastcall sub_46E388(void* a1, void* edx, void* a2, int a3, int a4, int fullscreen, int a6)
{
    return shsub_46E388.fastcall<int>(a1, edx, a2, a3, a4, 0, a6);
}

enum WindowVerticalPos
{
    Center,
    Top,
    Bottom,
};

static BOOL WINAPI CenterWindowPosition(HWND hWnd, int nWidth, int nHeight, WindowVerticalPos put = Center)
{
    // fix the window to open at the center of the screen...
    HMONITOR monitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
    MONITORINFOEX info = { sizeof(MONITORINFOEX) };
    GetMonitorInfo(monitor, &info);
    DEVMODE devmode = {};
    devmode.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devmode);
    DWORD DesktopX = devmode.dmPelsWidth;
    DWORD DesktopY = devmode.dmPelsHeight;

    int newWidth = nWidth;
    int newHeight = nHeight;

    int WindowPosX = (int)(((float)DesktopX / 2.0f) - ((float)newWidth / 2.0f));
    int WindowPosY = (int)(((float)DesktopY / 2.0f) - ((float)newHeight / 2.0f));

    if (put == Top)
        return SetWindowPos(hWnd, 0, WindowPosX, 0, newWidth, newHeight, SWP_NOZORDER | SWP_FRAMECHANGED);
    else if (put == Bottom)
        return SetWindowPos(hWnd, 0, WindowPosX, DesktopY - newHeight, newWidth, newHeight, SWP_NOZORDER | SWP_FRAMECHANGED);

    return SetWindowPos(hWnd, 0, WindowPosX, WindowPosY, newWidth, newHeight, SWP_NOZORDER | SWP_FRAMECHANGED);
}

bool bFocus = false;
SafetyHookInline shWndProc{};
int __fastcall WndProc(HDC _this, void* edx, UINT Msg, int wparam, unsigned int lparam)
{
    switch (Msg)
    {
    //case WM_SETFOCUS:
    //    SetWindowPos(*(HWND*)(*((uint32_t*)_this + 253) + 4), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    //    bFocus = true;
    //    break;
    //
    //case WM_KILLFOCUS:
    //    SetWindowPos(*(HWND*)(*((uint32_t*)_this + 253) + 4), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    //    bFocus = false;
    //    break;

    case WM_ACTIVATE:
        if (LOWORD(wparam) == WA_INACTIVE)
        {
            SetWindowPos(*(HWND*)(*((uint32_t*)_this + 253) + 4), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            bFocus = false;
        }
        else
        {
            SetWindowPos(*(HWND*)(*((uint32_t*)_this + 253) + 4), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            bFocus = true;
        }
        break;
    }

    auto ret = shWndProc.fastcall<int>(_this, edx, Msg, wparam, lparam);
    return ret;
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
bool bEnableSplitscreen = false;
bool bInstance1 = true;
BOOL WINAPI SetWindowPosHook(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
    SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);

    if (bEnableSplitscreen)
    {
        if (bInstance1)
            return CenterWindowPosition(hWnd, BackBufferWidth, BackBufferHeight, Top);
        else
            return CenterWindowPosition(hWnd, BackBufferWidth, BackBufferHeight, Bottom);
    }

    return SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

SafetyHookInline shCreateSemaphoreA{};
HANDLE WINAPI CreateSemaphoreAHook(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCSTR lpName)
{
    auto ret = shCreateSemaphoreA.stdcall<HANDLE>(lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName);

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        bInstance1 = false;
        return shCreateSemaphoreA.stdcall<HANDLE>(lpSemaphoreAttributes, lInitialCount, lMaximumCount, "Global\\sc5_semaphore2");
    }

    return ret;
}

void Init()
{
    CIniReader iniReader("");
    auto bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    auto bWindowedMode = iniReader.ReadInteger("MAIN", "WindowedMode", 0) != 0;
    auto bDisableNegativeMouseAcceleration = iniReader.ReadInteger("MAIN", "DisableNegativeMouseAcceleration", 1) != 0;
    auto bSkipSystemDetection = iniReader.ReadInteger("MAIN", "SkipSystemDetection", 1) != 0;
    auto bPartialUltraWideSupport = iniReader.ReadInteger("MAIN", "PartialUltraWideSupport", 1) != 0;
    bDisableBlackAndWhiteFilter = iniReader.ReadInteger("MAIN", "DisableBlackAndWhiteFilter", 0) != 0;
    bBlacklistControlScheme = iniReader.ReadInteger("CONTROLS", "BlacklistControlScheme", 1) != 0;
    auto bUnlockDLC = iniReader.ReadInteger("UNLOCKS", "UnlockDLC", 1) != 0;

    auto sLANHelperExePath = iniReader.ReadString("LAN", "LANHelperExePath", "");
    auto bFixLAN = iniReader.ReadInteger("LAN", "FixLAN", 1) != 0;
    static auto sServerAddr = iniReader.ReadString("LAN", "ServerAddr", "127.0.0.1");

    bEnableSplitscreen = iniReader.ReadInteger("2INSTANCESPLITSCREEN", "Enable", 1) != 0;

    if (!bWindowedMode)
        bEnableSplitscreen = false;

    //accept any refresh rate
    {
        auto pattern = hook::pattern("7F 17 89 45 FC");
        injector::MakeNOP(pattern.get_first(), 2);
    }

    //disable uplay
    {
        auto pattern = hook::pattern("74 28 E8 ? ? ? ? 8B 10 8B C8 FF 92 ? ? ? ? 3B C3");
        injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true);
    }

    //allow loading unpacked files
    {
        ModuleList dlls;
        dlls.Enumerate(ModuleList::SearchLocation::LocalOnly);
        for (auto& e : dlls.m_moduleList)
        {
            auto m = std::get<HMODULE>(e);
            if (IsModuleUAL(m)) {
                GetOverloadedFilePathA = (decltype(GetOverloadedFilePathA))GetProcAddress(m, "GetOverloadedFilePathA");
                break;
            }
        }

        if (GetOverloadedFilePathA)
        {
            auto pattern = hook::pattern("83 3D ? ? ? ? ? 56 8B F1 75 5C");
            FFileManagerLinear::dword_13A334C = *pattern.get_first<int*>(2);

            pattern = hook::pattern("39 1D ? ? ? ? 0F 84 ? ? ? ? 8B 86");
            FFileManagerLinear::dword_13A3348 = *pattern.get_first<int*>(2);

            pattern = hook::pattern("55 8B EC 83 EC 18 53 56 57 FF 75 08 8D 45 E8");
            FFileManagerArc::shLookup = safetyhook::create_inline(pattern.get_first(), FFileManagerArc::Lookup);
            
            pattern = hook::pattern("55 8B EC 83 EC 18 83 3D ? ? ? ? ? 56 8B F1 75 5C");
            FFileManagerLinear::shLookup = safetyhook::create_inline(pattern.get_first(), FFileManagerLinear::Lookup);

            pattern = hook::pattern("E8 ? ? ? ? 39 1D ? ? ? ? 0F 84 ? ? ? ? 8B 86");
            FFileManagerLinear::hbsub_43A943.fun = injector::MakeCALL(pattern.get_first(), FFileManagerLinear::sub_43A943, true).get();

            pattern = hook::pattern("8D 4D E4 E8 ? ? ? ? 8B C7 5F 5E 5B C9 C2 10 00");
            static auto restore = safetyhook::create_mid(pattern.get_first(8), [](SafetyHookContext& regs)
            {
                *FFileManagerLinear::dword_13A3348 = FFileManagerLinear::curdword_13A3348;
            });
        }
    }

    if (bWindowedMode)
    {
        auto pattern = hook::pattern("55 8B EC 83 EC 40 53 56 83 C8 FF");
        shsub_46E388 = safetyhook::create_inline(pattern.get_first(), sub_46E388);
    
        pattern = hook::pattern("55 8B EC 83 E4 F0 81 EC ? ? ? ? A1 ? ? ? ? 33 C4 89 84 24 ? ? ? ? 8B 45 0C");
        shWndProc = safetyhook::create_inline(pattern.get_first(), WndProc);

        IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
            std::forward_as_tuple("SetWindowPos", SetWindowPosHook)
        );
        
        pattern = hook::pattern("A3 ? ? ? ? 83 BE");
        static auto GetPresentationParametersHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            auto PresentationParameters = (D3DPRESENT_PARAMETERS*)regs.edi;

            BackBufferWidth = PresentationParameters->BackBufferWidth;
            BackBufferHeight = PresentationParameters->BackBufferHeight;
        });
    }

    if (bDisableNegativeMouseAcceleration)
    {
        auto pattern = hook::pattern("76 05 0F 28 D9 EB 08 0F 2F DA");
        injector::MakeNOP(pattern.get_first(0), 2, true);
        injector::MakeNOP(pattern.get_first(5), 2, true);
        injector::MakeNOP(pattern.get_first(10), 2, true);

        pattern = hook::pattern("77 0D 0F 2F C2 76 05 0F 28 CA EB 03 0F 28 C8 0F 57 C0");
        injector::MakeNOP(pattern.get_first(0), 2, true);
        injector::MakeNOP(pattern.get_first(5), 2, true);
    }

    if (bSkipIntro)
    {
        auto pattern = hook::pattern("55 8D 6C 24 88 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 45 74 53 56 57 BE ? ? ? ? 68 ? ? ? ? 8B CE E8 ? ? ? ? E8 ? ? ? ? 50");
        injector::MakeRET(pattern.get_first());
    }

    if (bSkipSystemDetection)
    {
        auto pattern = hook::pattern("75 13 E8 ? ? ? ? 50 E8");
        injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true);
    }

    //if (bDisableBlackAndWhiteFilter) //light and shadow
    {
        auto pattern = hook::pattern("E8 ? ? ? ? D9 05 ? ? ? ? 83 C4 0C 33 F6 56 51");
        hb_8330DB.fun = injector::MakeCALL(pattern.get_first(), sub_8330DB, true).get();
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 0C 83 3D ? ? ? ? ? 0F 8C ? ? ? ? 7F 0D 83 3D ? ? ? ? ? 0F 86");
        hb_8330DB.fun = injector::MakeCALL(pattern.get_first(), sub_8330DB, true).get();
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 0C 83 3D ? ? ? ? ? 7C 77 7F 09 83 3D ? ? ? ? ? 76 6C");
        hb_8330DB.fun = injector::MakeCALL(pattern.get_first(), sub_8330DB, true).get();
        pattern = hook::pattern("D9 5C 24 04 D9 40 08 D9 1C 24 E8 ? ? ? ? 83 C4 0C C3");
        hb_8330DB.fun = injector::MakeCALL(pattern.get_first(10), sub_8330DB, true).get();
    }

    // controller scheme force X360
    {
        auto pattern = hook::pattern("66 81 FF ? ? 75 0E 66 81 FE ? ? 75 13");
        injector::MakeNOP(pattern.get_first(), 14, true);
    }

    if (bBlacklistControlScheme)
    {
        // aiming check
        auto pattern = hook::pattern("E8 ? ? ? ? 8B 4D F8 48");
        injector::MakeCALL(pattern.get_first(), sub_56C0AD, true);

        // add modes for aiming
        pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 10 8B 00 3B 05 ? ? ? ? 75 04");
        hb458CD3.fun = injector::MakeCALL(pattern.get_first(), fnTogglePrecisionMode, true).get();

        // check aiming mode
        pattern = hook::pattern("3B 05 ? ? ? ? 75 07 8B CE E8 ? ? ? ? 5F 5E 5B");
        static auto loc_56DC64 = (uintptr_t)pattern.get_first(15);
        struct GetPrecisionMode
        {
            void operator()(injector::reg_pack& regs)
            {
                if (regs.eax == CrouchSprint)
                    bCrouchSprintNeeded = true;
                else if (regs.eax == CrouchSprintOff)
                    bCrouchSprintNeeded = false;

                if (regs.eax == Sprint)
                    bSprintNeeded = true;
                else if (regs.eax == SprintOff)
                    bSprintNeeded = false;

                if (regs.eax == TogglePrecisionMode || regs.eax == TogglePrecisionModeOn || regs.eax == TogglePrecisionModeOff)
                    WhichPrecisionModeNeeded = regs.eax;
                else
                    *(uintptr_t*)(regs.esp - 4) = loc_56DC64;
            }
        }; injector::MakeInline<GetPrecisionMode>(pattern.get_first(0), pattern.get_first(8));

        // add crouch sprint
        pattern = hook::pattern("F3 0F 10 45 ? F3 0F 11 45 ? F3 0F 10 86");
        static auto GetSpeedReferenceAdjustment = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (bCrouchSprintNeeded)
                *(float*)(regs.ebp + 0x0C) *= 1.5f;
            else if (bSprintNeeded)
                *(float*)(regs.ebp + 0x0C) *= 1.25f;
        });

        // cover modes
        pattern = hook::pattern("E8 ? ? ? ? F6 05 ? ? ? ? ? 75 17 83 0D ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? F6 05 ? ? ? ? ? 75 17 83 0D ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? F6 05 ? ? ? ? ? 75 17 83 0D ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? F6 05 ? ? ? ? ? 75 1A 81 0D ? ? ? ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? BF");
        hb458CD3.fun = injector::MakeCALL(pattern.get_first(0), fnDetectB2W, true).get();
        pattern = hook::pattern("E8 ? ? ? ? F6 05 ? ? ? ? ? 75 17 83 0D ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? F6 05 ? ? ? ? ? 75 17 83 0D ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? F6 05 ? ? ? ? ? 75 1A 81 0D ? ? ? ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? BF");
        hb458CD3.fun = injector::MakeCALL(pattern.get_first(0), fnStopDetectB2W, true).get();

        // cover
        static bool bCoverStateStarted = false;
        static bool bReleased = false;
        static auto loc_56DBB3 = (uintptr_t)hook::get_pattern("3B 05 ? ? ? ? 75 13 8B 8E ? ? ? ? 8B 01");
        static auto lastDetectTime = std::chrono::steady_clock::now();

        pattern = hook::pattern("3B 05 ? ? ? ? 75 0B 09 BE");
        struct CoverHook
        {
            void operator()(injector::reg_pack& regs)
            {
                if (regs.eax == DetectB2W || regs.eax == StopDetectB2W)
                {
                    if (regs.eax == DetectB2W)
                    {
                        if (bCoverStateStarted)
                        {
                            *(uint32_t*)(regs.esi + 0xA44) &= ~0x100;
                        }
                        else
                        {
                            lastDetectTime = std::chrono::steady_clock::now();
                            *(uint32_t*)(regs.esi + 0xA44) |= 0x100;
                        }
                    }
                }
                else
                    *(uintptr_t*)(regs.esp - 4) = loc_56DBB3;
            }
        }; injector::MakeInline<CoverHook>(pattern.get_first(0), pattern.get_first(14));

        pattern = hook::pattern("81 88 ? ? ? ? ? ? ? ? 8B 4B 78");
        static auto UCoverNavStartState = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            bCoverStateStarted = true;
        });

        pattern = hook::pattern("0F 57 C0 53 55 56 57");
        static auto UCoverNavEndState = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            bCoverStateStarted = false;
        });

        pattern = hook::pattern("66 F7 86 ? ? ? ? ? ? 0F 84 ? ? ? ? 0F 57 C0");
        static auto UCoverNavStartStateCheck = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - lastDetectTime).count();
            
            // Wait for 1 second before applying StopDetectB2W code
            if (elapsed >= 1 && elapsed <= 2)
            {
                if (!bCoverStateStarted)
                    *(uint32_t*)(regs.esi + 0xA44) &= ~0x100;
            }
        });
    }

    if (bUnlockDLC)
    {
        auto pattern = hook::pattern("74 15 FF 74 24 08 8B CE E8 ? ? ? ? 8B 40 08");
        if (!pattern.empty())
            injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true); // UEPECInventory

        pattern = hook::pattern("74 15 FF 74 24 08 8B CE E8 ? ? ? ? 8B 40 04 F7 D0");
        if (!pattern.empty())
            injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true); // UEPECInventory

        pattern = hook::pattern("75 25 FF 75 FC");
        if (!pattern.empty())
            injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true); // MenuUniformSelect

        pattern = hook::pattern("8B 41 7C D1 E8 F7 D0");
        if (!pattern.empty())
        {
            injector::MakeNOP(pattern.get_first(), 10, true);
            static auto UEChallengeProfileExternal1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { regs.eax = 0; });
        }

        pattern = hook::pattern("8B 41 7C C1 E8 03");
        if (!pattern.empty())
        {
            injector::MakeNOP(pattern.get_first(), 11, true);
            static auto UEChallengeProfileExternal2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { regs.eax = 0; });
        }

        // MapConfigurationDLC.xml
        pattern = hook::pattern("0F 85 ? ? ? ? 39 7D EC");
        if (!pattern.empty())
            injector::MakeNOP(pattern.get_first(), 6, true);
        
        pattern = hook::pattern("0F 84 ? ? ? ? 8D 45 E8 50 8B CB");
        if (!pattern.empty())
            injector::MakeNOP(pattern.get_first(), 6, true);
        
        pattern = hook::pattern("7E 50 8B 45 E8");
        if (!pattern.empty())
            injector::MakeJMP(pattern.get_first(), hook::pattern("8B 0D ? ? ? ? 8B 01 57 FF 35 ? ? ? ? 57 68 ? ? ? ? FF 50 10 8B D8").get_first(), true);
    }

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
                workingDir.c_str(), &pi)) {
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

    if (bEnableSplitscreen)
    {
        CreateMutexA(NULL, TRUE, "XidiInstance");
        shCreateSemaphoreA = safetyhook::create_inline(CreateSemaphoreA, CreateSemaphoreAHook);
    }

    if (bPartialUltraWideSupport)
    {
        // override aspect ratio
        auto pattern = hook::pattern("74 0A F3 0F 10 05 ? ? ? ? EB 13");
        injector::MakeNOP(pattern.get_first(), 2, true);

        pattern = hook::pattern("8D 86 ? ? ? ? 8B 08 3B CD");
        static auto IniHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(uint32_t*)(regs.esi + 0x104) = 0; //ForceMultiMon
            *(float*)(regs.esi + 0x108) = 99.0f; //MinTripleDispRatio
        });

        static int nCounter = 0;
        static int nCounterPrev = 0;
        static bool bNeedsFix = false;

        pattern = hook::pattern("F3 0F 10 0D ? ? ? ? 53 8B D9 83 4B 10 FF");
        injector::MakeNOP(pattern.get_first(), 8, true);
        static auto FCanvasUtil__SetViewport = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (GetAspectRatio() > fDefaultAspectRatio)
                regs.xmm1.f32[0] = 2.0f / (GetAspectRatio() / fDefaultAspectRatio);
            else
                regs.xmm1.f32[0] = 2.0f;
        });

        pattern = hook::pattern("F3 0F 10 0D ? ? ? ? 89 4B 10");
        injector::MakeNOP(pattern.get_first(), 8, true);
        static auto FCanvasUtil__SetViewport2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (GetAspectRatio() > fDefaultAspectRatio)
                regs.xmm1.f32[0] = 2.0f / (GetAspectRatio() / fDefaultAspectRatio);
            else
                regs.xmm1.f32[0] = 2.0f;
        });

        //scaling for menus and 3d
        {
            auto pattern = hook::pattern("F3 0F 10 90 ? ? ? ? 83 EC 10 8B C4 F3 0F 11 00");
            injector::MakeNOP(pattern.get_first(0), 8, true);
            static auto UViewportHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                regs.xmm2.f32[0] = 1.0f / (GetAspectRatio() / fDefaultAspectRatio);
            });

            pattern = hook::pattern("F3 0F 10 90 ? ? ? ? 83 EC 10 8B C4 F3 0F 11 10");
            injector::MakeNOP(pattern.get_first(0), 8, true);
            static auto UViewportHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                regs.xmm2.f32[0] = 1.0f / (GetAspectRatio() / fDefaultAspectRatio);
            });

            pattern = hook::pattern("8B 3C 88 8B CF E8 ? ? ? ? 85 C0");
            static auto UViewportHook4 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                nCounterPrev = nCounter;
                nCounter = regs.ecx;
            });
        }

        //HUD (all 2D, including what's fixed above)
        {
            auto pattern = hook::pattern("F3 0F 59 6C 24 ? F3 0F 11 44 24");
            static auto FlashMenuRendererHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                //if (nCounter == 0 || nCounter == 1 || nCounter == 3 || nCounter == 4)
                if (nCounter != 5 && nCounter != 6 && nCounter != 7 && nCounter != 8 && nCounter != 9 && OpenedVideosList.empty()) // 5 and 6 mess up menu sometimes, 7, 8 and 9 is in-world UI
                {
                    bNeedsFix = true;
                    if (GetAspectRatio() > fDefaultAspectRatio)
                        regs.xmm0.f32[0] /= (GetAspectRatio() / fDefaultAspectRatio);
                }
            });

            pattern = hook::pattern("F3 0F 11 44 24 ? 0F 57 C0 F3 0F 11 5C 24");
            static auto FlashMenuRendererHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (bNeedsFix)
                {
                    if (GetAspectRatio() > fDefaultAspectRatio)
                        regs.xmm3.f32[0] /= (GetAspectRatio() / fDefaultAspectRatio);
                    bNeedsFix = false;
                }
            });
        }

        // blood overlay disable
        {
            auto pattern = hook::pattern("F3 0F 10 05 ? ? ? ? 0F 2F C1 0F 86 ? ? ? ? F3 0F 10 96");
            injector::MakeNOP(pattern.get_first(), 8, true);
            static auto FlashHudHealthFeedback = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (GetAspectRatio() > fDefaultAspectRatio)
                    regs.xmm0.f32[0] = 0.0f;
                else
                    regs.xmm0.f32[0] = 1.0f;
            });
        }

        // sticky camera overlay disable
        {
            pattern = hook::pattern("0F 84 ? ? ? ? F3 0F 10 0D ? ? ? ? 0F B6 C9");
            injector::MakeNOP(pattern.get_first(), 6, true); // make pos overwrite possible

            pattern = hook::pattern("F3 0F 11 96 ? ? ? ? F3 0F 2C C8");
            injector::MakeNOP(pattern.get_first(), 8, true);
            static auto StickyCamBorder1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (GetAspectRatio() > fDefaultAspectRatio)
                    *(float*)(regs.esi + 0x4A8) = -10000.0f;
                else
                    *(float*)(regs.esi + 0x4A8) = regs.xmm2.f32[0];
            });

            pattern = hook::pattern("F3 0F 10 86 ? ? ? ? F3 0F 59 05 ? ? ? ? 33 C9");
            injector::MakeNOP(pattern.get_first(), 8, true);
            static auto StickyCamBorder2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (GetAspectRatio() > fDefaultAspectRatio)
                    regs.xmm0.f32[0] = -10000.0f;
                else
                    regs.xmm0.f32[0] = *(float*)(regs.esi + 0x4A8);
            });
        }
    }
}

void InitLeadD3DRender()
{
    CIniReader iniReader("");
    auto bDisableDOF = iniReader.ReadInteger("MAIN", "DisableDOF", 1) != 0;
    auto bDisableBlackAndWhiteFilter = iniReader.ReadInteger("MAIN", "DisableBlackAndWhiteFilter", 0) != 0;
    auto bDisableCharacterLighting = iniReader.ReadInteger("MAIN", "DisableCharacterLighting", 0) != 0;
    auto bEnhancedSonarVision = iniReader.ReadInteger("MAIN", "EnhancedSonarVision", 0) != 0;
    gBlacklistIndicators = iniReader.ReadInteger("MAIN", "BlacklistIndicators", 0);
    auto bPartialUltraWideSupport = iniReader.ReadInteger("MAIN", "PartialUltraWideSupport", 1) != 0;

    if (bDisableDOF)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "E8 ? ? ? ? 8B 06 8B 08 83 C4 18 6A 00 68");
        injector::MakeNOP(pattern.get_first(0), 5);
    }

    if (bDisableBlackAndWhiteFilter)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "E8 ? ? ? ? 8B 06 8B 08 83 C4 34");
        hb_100177B7.fun = injector::MakeCALL(pattern.get_first(), sub_100177B7, true).get();
    }

    if (bDisableCharacterLighting)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "80 78 44 00");
        injector::WriteMemory<uint8_t>(pattern.get_first(3), 1, true);
    }

    if (bEnhancedSonarVision)
    {
        static bool bNightVision = false;
        static auto loc_1002E95F = (uintptr_t)hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "89 B8 ? ? ? ? E8 ? ? ? ? 5F 5E 5B 83 C5 78 C9 C3").get_first(13);

        auto pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "E8 ? ? ? ? 89 5D 78");
        hb_1002581C.fun = injector::MakeCALL(pattern.get_first(), sub_1002581C, true).get();

        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "8B CE 88 86");
        struct SonarVisionHook
        {
            void operator()(injector::reg_pack& regs)
            {
                bNightVision = regs.eax & 0xff;
                //if (false)
                //    *(uint8_t*)(regs.esi + 0x642C) = regs.eax & 0xff;
                //else
                *(uint8_t*)(regs.esi + 0x642C) = 0;
            }
        }; injector::MakeInline<SonarVisionHook>(pattern.get_first(2), pattern.get_first(8));

        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "80 B9 ? ? ? ? ? 0F 84 ? ? ? ? 56 57 E8 ? ? ? ? 8B 03 33 FF 47");
        struct NVCheck
        {
            void operator()(injector::reg_pack& regs)
            {
                if (!bNightVision)
                    *(uintptr_t*)(regs.esp - 4) = loc_1002E95F;
            }
        }; injector::MakeInline<NVCheck>(pattern.get_first(0), pattern.get_first(13));

        //DrawVisibleOpaque
        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "68 ? ? ? ? 57 8D 43 0C");
        injector::MakeNOP(pattern.get_first(0), 17);
        
        //to do: add bNightVision check
        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "75 15 8B 80");
        injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true);
        
        //to do: add bNightVision check
        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "80 B8 ? ? ? ? ? 74 65");
        injector::MakeNOP(pattern.get_first(0), 7);
        
        //to do: add bNightVision check
        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "8B 03 80 B8");
        injector::MakeNOP(pattern.get_first(2), 7);
    }

    // Viewport
    if (bPartialUltraWideSupport)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "F3 0F 11 49 ? F3 0F 11 61");
        static auto ViewportHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (!OpenedVideosList.empty() && bVideoRender && GetAspectRatio() > fDefaultAspectRatio)
                regs.xmm1.f32[0] /= (GetAspectRatio() / fDefaultAspectRatio);
        });
        
        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "F3 0F 11 51 ? F3 0F 11 49 ? F3 0F 59 F5");
        static auto ViewportHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (!OpenedVideosList.empty() && bVideoRender && GetAspectRatio() > fDefaultAspectRatio)
                regs.xmm2.f32[0] /= (GetAspectRatio() / fDefaultAspectRatio);
            bVideoRender = false;
        });

        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "89 4D 28 E8 ? ? ? ? 8B 75 7C 33 FF 8B D8");
        static auto VideoRenderHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            bVideoRender = true;
        });

        pattern = hook::module_pattern(GetModuleHandle(L"LeadD3DRender"), "8B 07 8B 08 6A 00 50 FF 91 ? ? ? ? 8B 07 8B 08 6A 00");
        static auto d3ddevice = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            pDevice = *(IDirect3DDevice9**)(regs.edi);
        });
    }
}

SafetyHookInline shBinkOpen{};
int __stdcall BinkOpen(const char* path, int flags)
{
    auto handle = shBinkOpen.stdcall<int>(path, flags);

    auto s = std::string_view(path);
    if (s.contains("Logo_") || s.contains("_Load") || s.contains("C00_") || s.contains("C05_"))
        OpenedVideosList.push_back(handle);

    return handle;
}

SafetyHookInline shBinkClose{};
int __stdcall BinkClose(int handle)
{
    OpenedVideosList.remove(handle);
    return shBinkClose.stdcall<int>(handle);
}

void InitLead3DEngine()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"Lead3DEngine"), "FF 15 ? ? ? ? 8B CE 89 86");
    shBinkOpen = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), BinkOpen);

    pattern = hook::module_pattern(GetModuleHandle(L"Lead3DEngine"), "FF 15 ? ? ? ? 38 1D");
    shBinkClose = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), BinkClose);
}

void InitLED()
{
    CIniReader iniReader("");
    auto bLightSyncRGB = iniReader.ReadInteger("LOGITECH", "LightSyncRGB", 1);

    if (bLightSyncRGB)
    {
        LEDEffects::Inject([]()
        {
            static auto fPlayerVisibility = gVisibility;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            auto gVisCmp = static_cast<float>(static_cast<int>(gVisibility * 10.0f)) / 10.0f;
            auto fPlVisCmp = static_cast<float>(static_cast<int>(fPlayerVisibility * 10.0f)) / 10.0f;
            
            if (fPlVisCmp > gVisCmp)
                fPlayerVisibility -= 0.05f;
            else if (fPlVisCmp < gVisCmp)
                fPlayerVisibility += 0.05f;
            
            constexpr auto minBrightness = 0.3f;
            constexpr auto maxBrightness = 1.0f;

            fPlayerVisibility = std::clamp(fPlayerVisibility, minBrightness, maxBrightness);
            
            auto [R, G, B] = LEDEffects::RGBtoPercent(255, 39, 26, gBlacklistIndicators ? fPlayerVisibility : ((maxBrightness + minBrightness) - fPlayerVisibility));
            LEDEffects::SetLighting(R, G, B, false, false, false);
        });
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        {
            std::error_code ec;
            auto injPath = std::filesystem::path("InjectorCLIx86.exe");
            auto processPath = GetExeModulePath() / injPath;
            auto workingDir = std::filesystem::path(processPath).remove_filename();
            if (std::filesystem::exists(processPath, ec))
            {
                auto cmd = processPath.wstring() + L" pid:" + std::to_wstring(GetCurrentProcessId()) + L" HookLibraryx86.dll";
                HANDLE hJob = CreateJobObject(nullptr, nullptr);
                JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = { };
                info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
                SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &info, sizeof(info));
                STARTUPINFO si = { sizeof(si) };
                PROCESS_INFORMATION pi;
                if (CreateProcessInJob(hJob,
                    NULL,
                    const_cast<wchar_t*>(cmd.c_str()),
                    nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                    nullptr, workingDir.c_str(), &si, &pi)) {
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
            }
        }

        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("D9 1C 24 E8 ? ? ? ? D9 5E 0C"));
        CallbackHandler::RegisterCallback(L"LeadD3DRender.dll", InitLeadD3DRender);
        CallbackHandler::RegisterCallback(L"Lead3DEngine.dll", InitLead3DEngine);
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitLED, hook::pattern("D9 1C 24 E8 ? ? ? ? D9 5E 0C"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}
