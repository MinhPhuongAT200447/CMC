// ServerCore.h - giao tiep giua giao dien MFC va loi server
#pragma once
#include <windows.h>
#include <string>
#include <vector>

#define WM_APP_LOG    (WM_APP + 1)   // lParam = wchar_t* (cap phat bang _wcsdup, nhan xong phai free())
#define WM_APP_USERS  (WM_APP + 2)   // danh sach online thay doi

void InitExeDir();                                   // goi 1 lan khi chuong trinh bat dau
void SetLogWindow(HWND h);                           // cua so nhan WM_APP_LOG / WM_APP_USERS (NULL = khong gui)
bool ServerStart(int port, std::wstring& err);       // WSAStartup, socket, bind, listen, accept
void ServerStop();                                   // closesocket, doi thread, WSACleanup
bool ServerIsRunning();
std::vector<std::string> OnlineNames();
std::wstring Utf8ToWide(const std::string& s);
int  ReadPortFromIni();
void SavePortToIni(int port);
std::wstring InstallService();
std::wstring UninstallService();
void RunServiceDispatcher();                         // che do -service
