
#include "pch.h"
#include "framework.h"
#include "ChatClient.h"
#include "ChatClientDlg.h"
#include "afxdialogex.h"

#pragma comment(lib, "ws2_32.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define AUTORUN_KEY    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define AUTORUN_VALUE  L"ChatApp"

static BOOL IsAutorun()
{
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, AUTORUN_KEY, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return FALSE;
    BOOL exists = (RegQueryValueExW(key, AUTORUN_VALUE, NULL, NULL, NULL, NULL) == ERROR_SUCCESS);
    RegCloseKey(key);
    return exists;
}

static void SetAutorun(BOOL on)
{
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, AUTORUN_KEY, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS)
        return;

    if (on) {
        WCHAR path[MAX_PATH];
        WCHAR cmd[MAX_PATH + 4];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        lstrcpyW(cmd, L"\"");
        lstrcatW(cmd, path);
        lstrcatW(cmd, L"\"");
        RegSetValueExW(key, AUTORUN_VALUE, 0, REG_SZ,
            (const BYTE*)cmd, (lstrlenW(cmd) + 1) * sizeof(WCHAR));
    }
    else {
        RegDeleteValueW(key, AUTORUN_VALUE);
    }
    RegCloseKey(key);
}

BEGIN_MESSAGE_MAP(CChatClientDlg, CDialogEx)
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_BTN_CONNECT, &CChatClientDlg::OnBnClickedConnect)
    ON_BN_CLICKED(IDC_BTN_SEND, &CChatClientDlg::OnBnClickedSend)
    ON_BN_CLICKED(IDC_CHK_AUTORUN, &CChatClientDlg::OnBnClickedAutorun)
    ON_MESSAGE(WM_SOCK, &CChatClientDlg::OnSocket)
END_MESSAGE_MAP()

CChatClientDlg::CChatClientDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_CHATCLIENT_DIALOG, pParent)
    , m_sock(INVALID_SOCKET)
    , m_connected(FALSE)
    , m_rxLen(0)
    , m_rx{}
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

BOOL CChatClientDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        AfxMessageBox(L"Khong khoi tao duoc Winsock");

    SetDlgItemText(IDC_EDIT_IP, L"127.0.0.1");
    SetDlgItemInt(IDC_EDIT_PORT, 9999, FALSE);
    CheckDlgButton(IDC_CHK_AUTORUN, IsAutorun() ? BST_CHECKED : BST_UNCHECKED);
    GetDlgItem(IDC_BTN_SEND)->EnableWindow(FALSE);
    return TRUE;
}

void CChatClientDlg::OnDestroy()
{
    if (m_sock != INVALID_SOCKET) { closesocket(m_sock); m_sock = INVALID_SOCKET; }
    WSACleanup();
    CDialogEx::OnDestroy();
}


void CChatClientDlg::OnOK()
{
    OnBnClickedSend();
}

void CChatClientDlg::AppendLog(const CString& prefix, const CString& text)
{
    CString line = prefix + text + L"\r\n";
    CEdit* log = (CEdit*)GetDlgItem(IDC_EDIT_LOG);
    int len = log->GetWindowTextLength();
    log->SetSel(len, len);
    log->ReplaceSel(line);
}

void CChatClientDlg::Disconnect()
{
    if (m_sock != INVALID_SOCKET) { closesocket(m_sock); m_sock = INVALID_SOCKET; }
    m_connected = FALSE;
    m_rxLen = 0;
    SetDlgItemText(IDC_BTN_CONNECT, L"Connect");
    GetDlgItem(IDC_BTN_SEND)->EnableWindow(FALSE);
}

void CChatClientDlg::OnBnClickedConnect()
{
    if (m_sock != INVALID_SOCKET) {
        Disconnect();
        AppendLog(L"[Da ngat ket noi]", L"");
        return;
    }

    CString ip;
    GetDlgItemText(IDC_EDIT_IP, ip);

    BOOL okPort = FALSE;
    UINT port = GetDlgItemInt(IDC_EDIT_PORT, &okPort, FALSE);
    if (!okPort || port == 0 || port > 65535) { AppendLog(L"[Port khong hop le]", L""); return; }

    SOCKADDR_IN addr;
    int addrLen = sizeof(addr);
    ZeroMemory(&addr, sizeof(addr));
    if (WSAStringToAddressW((LPWSTR)(LPCWSTR)ip, AF_INET, NULL, (SOCKADDR*)&addr, &addrLen) != 0) {
        AppendLog(L"[IP khong hop le]", L"");
        return;
    }

    m_sock = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (m_sock == INVALID_SOCKET) { AppendLog(L"[Khong tao duoc socket]", L""); return; }
    WSAHtons(m_sock, (u_short)port, &addr.sin_port);

    WSAAsyncSelect(m_sock, m_hWnd, WM_SOCK, FD_CONNECT | FD_READ | FD_CLOSE);
    WSAConnect(m_sock, (SOCKADDR*)&addr, sizeof(addr), NULL, NULL, NULL, NULL);
    AppendLog(L"[Dang ket noi...]", L"");
}

void CChatClientDlg::OnBnClickedSend()
{
    if (!m_connected) return;

    CString msg;
    GetDlgItemText(IDC_EDIT_MSG, msg);
    if (msg.IsEmpty()) return;

    char data[4096];
    int bytes = WideCharToMultiByte(CP_UTF8, 0, msg, -1, data, sizeof(data) - 2, NULL, NULL);
    if (bytes <= 0) return;
    int total = bytes - 1;
    data[total++] = '\n';

    int sent = 0;
    while (sent < total) {
        WSABUF wb;
        wb.buf = data + sent;
        wb.len = total - sent;
        DWORD w = 0;
        if (WSASend(m_sock, &wb, 1, &w, 0, NULL, NULL) == SOCKET_ERROR) {
            if (WSAGetLastError() == WSAEWOULDBLOCK) { Sleep(1); continue; }
            AppendLog(L"[Loi gui]", L"");
            Disconnect();
            return;
        }
        sent += w;
    }

    AppendLog(L"Ban: ", msg);
    SetDlgItemText(IDC_EDIT_MSG, L"");
    GetDlgItem(IDC_EDIT_MSG)->SetFocus();
}

void CChatClientDlg::OnBnClickedAutorun()
{
    SetAutorun(IsDlgButtonChecked(IDC_CHK_AUTORUN) == BST_CHECKED);
}

void CChatClientDlg::ExtractLines()
{
    for (;;) {
        int pos = -1;
        for (int i = 0; i < m_rxLen; ++i)
            if (m_rx[i] == '\n') { pos = i; break; }

        int lineLen = (pos >= 0) ? pos : (m_rxLen == RX_SIZE ? m_rxLen : -1);
        if (lineLen < 0) return;

        CString text;
        WCHAR* p = text.GetBuffer(RX_SIZE + 1);
        int n = MultiByteToWideChar(CP_UTF8, 0, m_rx, lineLen, p, RX_SIZE);
        text.ReleaseBuffer(n);
        AppendLog(L"Echo: ", text);

        int consumed = (pos >= 0) ? pos + 1 : m_rxLen;
        MoveMemory(m_rx, m_rx + consumed, m_rxLen - consumed);
        m_rxLen -= consumed;
    }
}

LRESULT CChatClientDlg::OnSocket(WPARAM wParam, LPARAM lParam)
{
    if ((SOCKET)wParam != m_sock) return 0;

    WORD ev = WSAGETSELECTEVENT(lParam);
    WORD err = WSAGETSELECTERROR(lParam);

    if (ev == FD_CONNECT) {
        if (err != 0) { AppendLog(L"[Ket noi that bai]", L""); Disconnect(); return 0; }
        m_connected = TRUE;
        SetDlgItemText(IDC_BTN_CONNECT, L"Disconnect");
        GetDlgItem(IDC_BTN_SEND)->EnableWindow(TRUE);
        AppendLog(L"[Da ket noi]", L"");
    }
    else if (ev == FD_READ) {
        for (;;) {
            int space = RX_SIZE - m_rxLen;
            if (space == 0) { ExtractLines(); continue; }

            WSABUF rb;
            rb.buf = m_rx + m_rxLen;
            rb.len = space;
            DWORD n = 0, flags = 0;
            if (WSARecv(m_sock, &rb, 1, &n, &flags, NULL, NULL) == SOCKET_ERROR) break;
            if (n == 0) break;
            m_rxLen += n;
            ExtractLines();
        }
    }
    else if (ev == FD_CLOSE) {
        AppendLog(L"[Server dong ket noi]", L"");
        Disconnect();
    }
    return 0;
}
