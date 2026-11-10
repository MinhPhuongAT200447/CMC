#pragma once
#include <windows.h>
#include <string>
#include <vector>

#define WM_NET_EVENT (WM_APP + 10)
#define WM_NET_CLOSED (WM_APP + 11)

struct UserInfo{
    std::wstring name;
    bool         online = false;
};

struct ChatMsg {
    long long id = 0;
    std::wstring from, to, content;
    long long timestamp = 0;
};

struct NetEvent {
    std::string action;
    int code = 0;
    std::wstring  msg;
    std::vector<UserInfo> users;
    std::vector<ChatMsg> messages;
};

bool ClientConnect(const std::wstring& host, int port, HWND notifyWnd, std::wstring& err);
void ClientDisconnect();
bool ClientIsConnected();


bool ClientSendAuth(bool isRegister, const std::wstring& user, const std::wstring& pass);
bool ClientSendLogout();
bool ClientSendGetUsers();
bool ClientSendMessage(const std::wstring& to, const std::wstring& text);
bool ClientSendGetMessages(const std::wstring& with);
