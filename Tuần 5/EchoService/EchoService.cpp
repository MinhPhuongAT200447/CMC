
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")

#define SVC_NAME   L"EchoService"
#define SVC_DISP   L"Echo TCP Service (port 9999)"
#define ECHO_PORT  9999


static SERVICE_STATUS        g_status;
static SERVICE_STATUS_HANDLE g_hStatus = NULL;
static WSAEVENT              g_stopEvent = NULL;
static volatile LONG         g_activeClients = 0;

static void Print(const WCHAR* s)
{
    DWORD written;
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), s, lstrlenW(s), &written, NULL);
}

static void PrintError(const WCHAR* what, DWORD code)
{
    WCHAR buf[256];
    wsprintfW(buf, L"%s loi %lu\r\n", what, code);
    Print(buf);
}

static void ReportStatus(DWORD state, DWORD waitHintMs)
{
    static DWORD checkPoint = 1;

    g_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_status.dwCurrentState = state;
    g_status.dwWin32ExitCode = NO_ERROR;
    g_status.dwWaitHint = waitHintMs;

    if (state == SERVICE_START_PENDING || state == SERVICE_STOP_PENDING)
        g_status.dwControlsAccepted = 0;
    else
        g_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

    g_status.dwCheckPoint = (state == SERVICE_RUNNING || state == SERVICE_STOPPED) ? 0 : checkPoint++;
    SetServiceStatus(g_hStatus, &g_status);
}

static DWORD WINAPI CtrlHandlerEx(DWORD ctrl, DWORD eventType, LPVOID eventData, LPVOID context)
{
    switch (ctrl) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        ReportStatus(SERVICE_STOP_PENDING, 5000);
        WSASetEvent(g_stopEvent);
        return NO_ERROR;
    case SERVICE_CONTROL_INTERROGATE:
        return NO_ERROR;
    }
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static BOOL SendAll(SOCKET s, WSAEVENT sockEvent, const char* data, int n)
{
    int sent = 0;
    while (sent < n) {
        WSABUF buf;
        buf.buf = (CHAR*)data + sent;
        buf.len = n - sent;
        DWORD written = 0;

        if (WSASend(s, &buf, 1, &written, 0, NULL, NULL) == SOCKET_ERROR) {
            if (WSAGetLastError() != WSAEWOULDBLOCK) return FALSE;

            WSAEVENT waits[2] = { g_stopEvent, sockEvent };
            DWORD w = WSAWaitForMultipleEvents(2, waits, FALSE, WSA_INFINITE, FALSE);
            if (w != WSA_WAIT_EVENT_0 + 1) return FALSE;

            WSANETWORKEVENTS ne;
            WSAEnumNetworkEvents(s, sockEvent, &ne);
            if (ne.lNetworkEvents & FD_CLOSE) return FALSE;
            continue;
        }
        sent += written;
    }
    return TRUE;
}

static DWORD WINAPI ClientThread(LPVOID param)
{
    SOCKET   s = (SOCKET)param;
    char     buf[4096];
    WSAEVENT sockEvent = WSACreateEvent();

    WSAEventSelect(s, sockEvent, FD_READ | FD_WRITE | FD_CLOSE);

    BOOL alive = TRUE;
    while (alive) {
        for (;;) {
            WSABUF rb;
            rb.buf = buf;
            rb.len = sizeof(buf);
            DWORD got = 0, flags = 0;

            if (WSARecv(s, &rb, 1, &got, &flags, NULL, NULL) == SOCKET_ERROR) {
                if (WSAGetLastError() != WSAEWOULDBLOCK) alive = FALSE;
                break;
            }
            if (got == 0) { alive = FALSE; break; }

            if (!SendAll(s, sockEvent, buf, (int)got)) { alive = FALSE; break; }
        }
        if (!alive) break;

        WSAEVENT waits[2] = { g_stopEvent, sockEvent };
        DWORD w = WSAWaitForMultipleEvents(2, waits, FALSE, WSA_INFINITE, FALSE);
        if (w != WSA_WAIT_EVENT_0 + 1) break;

        WSANETWORKEVENTS ne;
        WSAEnumNetworkEvents(s, sockEvent, &ne);
    }

    WSAEventSelect(s, NULL, 0);
    WSACloseEvent(sockEvent);
    WSASendDisconnect(s, NULL);
    closesocket(s);
    InterlockedDecrement(&g_activeClients);
    return 0;
}

static void RunServer()
{
    SOCKET ls = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (ls != INVALID_SOCKET) {
        SOCKADDR_IN addr;
        ZeroMemory(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        WSAHtonl(ls, INADDR_ANY, &addr.sin_addr.s_addr);
        WSAHtons(ls, ECHO_PORT, &addr.sin_port);

        if (bind(ls, (SOCKADDR*)&addr, sizeof(addr)) == 0 && listen(ls, SOMAXCONN) == 0) {

            WSAEVENT hAccept = WSACreateEvent();
            WSAEventSelect(ls, hAccept, FD_ACCEPT);
            WSAEVENT waits[2] = { g_stopEvent, hAccept };

            ReportStatus(SERVICE_RUNNING, 0);

            for (;;) {
                DWORD w = WSAWaitForMultipleEvents(2, waits, FALSE, WSA_INFINITE, FALSE);
                if (w != WSA_WAIT_EVENT_0 + 1) break;

                WSANETWORKEVENTS ne;
                WSAEnumNetworkEvents(ls, hAccept, &ne);

                for (;;) {
                    SOCKADDR_IN ca;
                    int caLen = sizeof(ca);

                    SOCKET c = WSAAccept(ls, (SOCKADDR*)&ca, &caLen, NULL, 0);
                    if (c == INVALID_SOCKET) break;

                    InterlockedIncrement(&g_activeClients);
                    HANDLE t = CreateThread(NULL, 0, ClientThread, (LPVOID)c, 0, NULL);
                    if (t) CloseHandle(t);
                    else { InterlockedDecrement(&g_activeClients); closesocket(c); }
                }
            }
            WSAEventSelect(ls, NULL, 0);
            WSACloseEvent(hAccept);
        }
        closesocket(ls);
    }

    for (int i = 0; i < 40 && g_activeClients > 0; ++i) {
        ReportStatus(SERVICE_STOP_PENDING, 2000);
        Sleep(100);
    }
}

static void WINAPI ServiceMain(DWORD argc, LPWSTR* argv)
{
    g_hStatus = RegisterServiceCtrlHandlerExW(SVC_NAME, CtrlHandlerEx, NULL);
    if (!g_hStatus) return;

    ReportStatus(SERVICE_START_PENDING, 3000);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) == 0) {
        g_stopEvent = WSACreateEvent();
        RunServer();
        WSACloseEvent(g_stopEvent);
        WSACleanup();
    }
    ReportStatus(SERVICE_STOPPED, 0);
}

static BOOL Install()
{
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);

    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!scm) { PrintError(L"OpenSCManager (chay bang Administrator)", GetLastError()); return FALSE; }

    SC_HANDLE svc = CreateServiceW(scm, SVC_NAME, SVC_DISP, SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL, path, NULL, NULL, NULL, NULL, NULL);
    BOOL ok = (svc != NULL);
    if (ok) { Print(L"Da cai dat service. Chay: sc start EchoService\r\n"); CloseServiceHandle(svc); }
    else      PrintError(L"CreateService", GetLastError());

    CloseServiceHandle(scm);
    return ok;
}

static BOOL Uninstall()
{
    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm) { PrintError(L"OpenSCManager", GetLastError()); return FALSE; }

    BOOL ok = FALSE;
    SC_HANDLE svc = OpenServiceW(scm, SVC_NAME, SERVICE_STOP | DELETE);
    if (svc) {
        SERVICE_STATUS st;
        ControlService(svc, SERVICE_CONTROL_STOP, &st);
        ok = DeleteService(svc);
        if (ok) Print(L"Da go service.\r\n"); else PrintError(L"DeleteService", GetLastError());
        CloseServiceHandle(svc);
    }
    else PrintError(L"OpenService", GetLastError());

    CloseServiceHandle(scm);
    return ok;
}

int wmain(int argc, wchar_t** argv)
{
    if (argc > 1) {
        if (lstrcmpiW(argv[1], L"-install") == 0) return Install() ? 0 : 1;
        if (lstrcmpiW(argv[1], L"-uninstall") == 0) return Uninstall() ? 0 : 1;
    }

    SERVICE_TABLE_ENTRYW table[2];
    table[0].lpServiceName = (LPWSTR)SVC_NAME;
    table[0].lpServiceProc = ServiceMain;
    table[1].lpServiceName = NULL;
    table[1].lpServiceProc = NULL;

    if (!StartServiceCtrlDispatcherW(table))
        Print(L"Day la service. Dung -install hoac -uninstall.\r\n");
    return 0;
}
