#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include "ClientCore.h"
#include "json.hpp"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "bcrypt.lib")

using json = nlohmann::json;

#define XOR_KEY "ChatLab2026"
#define TIMEOUT_MS 5000
#define MAX_PACKET 65536

static SOCKET g_s = INVALID_SOCKET;
static bool g_wsa = false;
static HANDLE g_hThread = NULL;
static HWND g_notify = NULL;
static volatile LONG g_closing = 0;

static std::wstring Utf8ToWide(const std::string& s){
    if (s.empty())
        return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), NULL, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], n);
    return w;
}
static std::string WideToUtf8(const std::wstring& w){
    if (w.empty())
        return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), NULL, 0, NULL, NULL);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &s[0], n, NULL, NULL);
    return s;
}

static std::string Sha256Hex(const std::string& s) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    unsigned char out[32] = {};
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, NULL, 0) != 0)
        return "";
    NTSTATUS st = BCryptHash(alg, NULL, 0, (PUCHAR)s.data(), (ULONG)s.size(), out, sizeof(out));
    BCryptCloseAlgorithmProvider(alg, 0);
    if (st != 0) return "";

    static const char* digits = "0123456789abcdef";
    std::string hex;
    for (int i = 0; i < 32; ++i){
        hex += digits[out[i] >> 4];
        hex += digits[out[i] & 15];
    }
    return hex;
}

static void XorBuf(std::string& data) {
    const char* key = XOR_KEY;
    size_t keyLen = strlen(key);
    for (size_t i = 0; i < data.size(); ++i)
        data[i] ^= key[i % keyLen];
}
static bool RecvAll(SOCKET s, char* buf, int len) {
    int got = 0;
    while (got < len){
        int r = recv(s, buf + got, len - got, 0);
        if (r <= 0)
            return false;
        got += r;
    }
    return true;
}
static bool SendAll(SOCKET s, const char* buf, int len) {
    int sent = 0;
    while (sent < len) {
        int r = send(s, buf + sent, len - sent, 0);
        if (r == SOCKET_ERROR) return false;
        sent += r;
    }
    return true;
}

static bool SendJson(const json& j) {
    SOCKET s = g_s;
    if (s == INVALID_SOCKET) return false;
    std::string data = j.dump(-1, ' ', false, json::error_handler_t::replace);
    XorBuf(data);
    unsigned long n = htonl((unsigned long)data.size());
    return SendAll(s, (const char*)&n, 4) && SendAll(s, data.data(), (int)data.size());
}

static ChatMsg ToChatMsg(const json& m) {
    ChatMsg c;
    c.id = m.value("id", 0LL);
    c.from = Utf8ToWide(m.value("from", std::string()));
    c.to = Utf8ToWide(m.value("to", std::string()));
    c.content = Utf8ToWide(m.value("content", std::string()));
    c.timestamp = m.value("timestamp", 0LL);
    return c;
}

static NetEvent* ParsePacket(const std::string& data) {
    json r = json::parse(data, nullptr, false);
    if (!r.is_object()) return nullptr;

    NetEvent* ev = new NetEvent();
    try {
        ev->action = r.value("action", std::string());
        ev->code = r.value("code", 0);
        ev->msg = Utf8ToWide(r.value("msg", std::string()));

        if (ev->action == "get_friends" && r.contains("users") && r["users"].is_array()){
            for (auto& u : r["users"]){
                UserInfo ui;
                ui.name   = Utf8ToWide(u.value("username", std::string()));
                ui.online = u.value("online", false);
                ev->users.push_back(ui);
            }
        }
        if (ev->action == "get_messages" && r.contains("messages") && r["messages"].is_array()) {
            for (auto& m : r["messages"])
                ev->messages.push_back(ToChatMsg(m));
        }
        if (ev->action == "new_message")
            ev->messages.push_back(ToChatMsg(r));
    } catch (...) {
        delete ev;
        return nullptr;
    }
    return ev;
}

static DWORD WINAPI RecvThread(LPVOID param) {
    SOCKET s = (SOCKET)param;
    for (;;) {
        unsigned long n = 0;
        if (!RecvAll(s, (char*)&n, 4)) break;
        n = ntohl(n);
        if (n == 0 || n > MAX_PACKET) break;
        std::string data(n, '\0');
        if (!RecvAll(s, &data[0], (int)n)) break;
        XorBuf(data);

        NetEvent* ev = ParsePacket(data);
        if (ev && !PostMessageW(g_notify, WM_NET_EVENT, 0, (LPARAM)ev))
            delete ev;
    }
    if (!g_closing)
        PostMessageW(g_notify, WM_NET_CLOSED, 0, 0);
    return 0;
}

static bool ConnectWithTimeout(SOCKET s, const sockaddr* addr, int len, int ms) {
    u_long nonBlocking = 1;
    ioctlsocket(s, FIONBIO, &nonBlocking);

    int rc = connect(s, addr, len);
    if (rc == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
        return false;

    if (rc != 0){
        fd_set writeSet, errSet;
        FD_ZERO(&writeSet); FD_ZERO(&errSet);
        FD_SET(s, &writeSet); FD_SET(s, &errSet);
        timeval tv; tv.tv_sec = ms / 1000; tv.tv_usec = (ms % 1000) * 1000;
        int sel = select(0, NULL, &writeSet, &errSet, &tv);
        if (sel <= 0 || FD_ISSET(s, &errSet))
            return false;
    }

    nonBlocking = 0;
    ioctlsocket(s, FIONBIO, &nonBlocking);
    return true;
}

bool ClientIsConnected() {
    return g_s != INVALID_SOCKET;
}

void ClientDisconnect() {
    if (g_s != INVALID_SOCKET) {
        InterlockedExchange(&g_closing, 1);
        shutdown(g_s, SD_BOTH);
    }
    if (g_hThread) {
        WaitForSingleObject(g_hThread, 3000);
        CloseHandle(g_hThread);
        g_hThread = NULL;
    }
    if (g_s != INVALID_SOCKET){
        closesocket(g_s);
        g_s = INVALID_SOCKET;
    }
    if (g_wsa){
        WSACleanup();
        g_wsa = false;
    }
}

bool ClientConnect(const std::wstring& host, int port, HWND notifyWnd, std::wstring& err) {
    ClientDisconnect();

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0){
        err = L"WSAStartup loi";
        return false;
    }
    g_wsa = true;

    ADDRINFOW hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    ADDRINFOW* res = nullptr;
    if (GetAddrInfoW(host.c_str(), std::to_wstring(port).c_str(), &hints, &res) != 0 || !res){
        err = L"Khong tim thay may chu (kiem tra lai dia chi)";
        ClientDisconnect();
        return false;
    }

    g_s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    bool ok = (g_s != INVALID_SOCKET) && ConnectWithTimeout(g_s, res->ai_addr, (int)res->ai_addrlen, TIMEOUT_MS);
    FreeAddrInfoW(res);
    if (!ok) {
        err = L"Khong ket noi duoc (server chua chay? sai dia chi hoac cong? bi firewall chan?)";
        ClientDisconnect();
        return false;
    }

    DWORD to = TIMEOUT_MS;
    setsockopt(g_s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&to, sizeof(to));

    g_notify = notifyWnd;
    InterlockedExchange(&g_closing, 0);
    g_hThread = CreateThread(NULL, 0, RecvThread, (LPVOID)g_s, 0, NULL);
    if (!g_hThread){
        err = L"Khong tao duoc thread nhan";
        ClientDisconnect();
        return false;
    }
    return true;
}

bool ClientSendAuth(bool isRegister, const std::wstring& user, const std::wstring& pass){
    std::string hash = Sha256Hex(WideToUtf8(pass));
    if (hash.empty())
        return false;
    json j;
    j["action"] = isRegister ? "register" : "login";
    j["username"] = WideToUtf8(user);
    j["password"] = hash;
    return SendJson(j);
}

bool ClientSendLogout(){
    json j;
    j["action"] = "logout";
    return SendJson(j);
}

bool ClientSendGetUsers(){
    json j;
    j["action"] = "get_friends";
    return SendJson(j);
}

bool ClientSendMessage(const std::wstring& to, const std::wstring& text){
    json j;
    j["action"] = "send_message";
    j["to"] = WideToUtf8(to);
    j["content"] = WideToUtf8(text);
    return SendJson(j);
}

bool ClientSendGetMessages(const std::wstring& with) {
    json j;
    j["action"] = "get_messages";
    j["with"] = WideToUtf8(with);
    j["since_id"] = 0;
    return SendJson(j);
}
