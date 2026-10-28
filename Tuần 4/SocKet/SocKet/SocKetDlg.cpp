
// SocKetDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "SocKet.h"
#include "SocKetDlg.h"
#include "afxdialogex.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static const UINT MAX_MSG_SIZE = 10 * 1024 * 1024;

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CSocKetDlg dialog



CSocKetDlg::CSocKetDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SOCKET_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSocKetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSocKetDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BTN_LISTEN, &CSocKetDlg::OnBnClickedBtnListen)
	ON_BN_CLICKED(IDC_BTN_CONNECT, &CSocKetDlg::OnBnClickedBtnConnect)
	ON_BN_CLICKED(IDC_BTN_DISCONNECT, &CSocKetDlg::OnBnClickedBtnDisconnect)
	ON_BN_CLICKED(IDC_BTN_SEND, &CSocKetDlg::OnBnClickedBtnSend)
	ON_MESSAGE(WM_SOCK_LOG, &CSocKetDlg::OnSockLog)
	ON_MESSAGE(WM_SOCK_CONNECTED, &CSocKetDlg::OnSockConnected)
	ON_MESSAGE(WM_SOCK_CLOSED, &CSocKetDlg::OnSockClosed)
END_MESSAGE_MAP()


// CSocKetDlg message handlers

BOOL CSocKetDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	WSADATA wsa;
	int r = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (r != 0)
	{
		CString s;
		s.Format(_T("WSAStartup thất bại, lỗi %d"), r);
		AfxMessageBox(s);
		EndDialog(IDCANCEL);
		return TRUE;
	}

	((CEdit*)GetDlgItem(IDC_LIST_CHAT))->SetLimitText(0);
	SetDlgItemText(IDC_EDIT_IP, _T("127.0.0.1"));
	SetDlgItemText(IDC_EDIT_PORT, _T("8888"));
	SetUiState(0);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CSocKetDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSocKetDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CSocKetDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
void CSocKetDlg::OnDestroy()
{
	CloseAll();
	WSACleanup();
	CDialogEx::OnDestroy();
}

void CSocKetDlg::AddChat(const CString& s) {
	CListBox* pList = (CListBox*)GetDlgItem(IDC_LIST_CHAT);
	int idx = pList->AddString(s);
	pList->SetTopIndex(idx);
}

void CSocKetDlg::SetUiState(int state) {
	GetDlgItem(IDC_EDIT_IP)->EnableWindow(state == 0);
	GetDlgItem(IDC_EDIT_PORT)->EnableWindow(state == 0);
	GetDlgItem(IDC_BTN_LISTEN)->EnableWindow(state == 0);
	GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(state == 0);
	GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(state != 0);
	GetDlgItem(IDC_EDIT_MSG)->EnableWindow(state == 2);
	GetDlgItem(IDC_BTN_SEND)->EnableWindow(state == 2);
}

void CSocKetDlg::PostLog(const CString& s)
{
	CString* p = new CString(s);
	if (!::PostMessage(m_hWnd, WM_SOCK_LOG, 0, (LPARAM)p))
		delete p;
}

LRESULT CSocKetDlg::OnSockLog(WPARAM, LPARAM lParam)
{
	CString* p = (CString*)lParam;
	AddChat(*p);
	delete p;
	return 0;
}

LRESULT CSocKetDlg::OnSockConnected(WPARAM, LPARAM)
{
	SetUiState(2);
	GetDlgItem(IDC_EDIT_MSG)->SetFocus();
	return 0;
}

LRESULT CSocKetDlg::OnSockClosed(WPARAM, LPARAM)
{
	CloseAll();
	return 0;
}

void CSocKetDlg::CloseAll() {
	m_stopping = true;

	SOCKET s = m_clientSock.exchange(INVALID_SOCKET);
	if (s != INVALID_SOCKET)
	{
		shutdown(s, SD_BOTH);
		closesocket(s);
	}
	s = m_listenSock.exchange(INVALID_SOCKET);
	if (s != INVALID_SOCKET)
		closesocket(s);

	if (m_hThread)
	{
		WaitForSingleObject(m_hThread, 3000);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	SetUiState(0);
}


void CSocKetDlg::OnBnClickedBtnListen()
{
	// TODO: Add your control notification handler code here
	UINT port = GetDlgItemInt(IDC_EDIT_PORT);
	if (port == 0 || port > 65535) {
		AfxMessageBox(_T("Port không hợp lệ"));
		return;
	}

	m_stopping = false;

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		CString e;
		e.Format(_T("socket() thất bại, lỗi %d"), WSAGetLastError());
		AfxMessageBox(e);
		return;
	}

	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons((u_short)port);
	if (bind(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR){
		CString e;
		e.Format(_T("bind() thất bại, lỗi %d (port có thể đang bị dùng)"), WSAGetLastError());
		AfxMessageBox(e);
		closesocket(s);
		return;
	}

	if (listen(s, SOMAXCONN) == SOCKET_ERROR){
		CString e;
		e.Format(_T("listen() thất bại, lỗi %d"), WSAGetLastError());
		AfxMessageBox(e);
		closesocket(s);
		return;
	}

	m_listenSock = s;
	m_isServer = true;
	m_hThread = CreateThread(NULL, 0, ThreadProc, this, 0, NULL);

	AddChat(_T("Đang chờ kết nối..."));
	SetUiState(1);
}


void CSocKetDlg::OnBnClickedBtnConnect()
{
	// TODO: Add your control notification handler code here
	CString ip;
	GetDlgItemText(IDC_EDIT_IP, ip);
	UINT port = GetDlgItemInt(IDC_EDIT_PORT);
	if (ip.IsEmpty() || port == 0 || port > 65535) {
		AfxMessageBox(_T("IP hoặc port không hợp lệ"));
		return;
	}

	m_addr = {};
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons((u_short)port);
	if (InetPton(AF_INET, ip, &m_addr.sin_addr) != 1)
	{
		AfxMessageBox(_T("Địa chỉ IP không đúng định dạng (ví dụ 192.168.1.5)"));
		return;
	}
	m_stopping = false;

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		CString e;
		e.Format(_T("socket() thất bại, lỗi %d"), WSAGetLastError());
		AfxMessageBox(e);
		return;
	}

	m_clientSock = s;
	m_isServer = false;
	m_hThread = CreateThread(NULL, 0, ThreadProc, this, 0, NULL);

	AddChat(_T("Đang kết nối tới ") + ip + _T("..."));
	SetUiState(1);
}


void CSocKetDlg::OnBnClickedBtnDisconnect()
{
	// TODO: Add your control notification handler code here
	CloseAll();
	AddChat(_T("Đã ngắt kết nối"));
}

DWORD WINAPI CSocKetDlg::ThreadProc(LPVOID lpParam)
{
	((CSocKetDlg*)lpParam)->ThreadMain();
	return 0;
}

void CSocKetDlg::ThreadMain()
{
	if (m_isServer){
		SOCKET ls = m_listenSock.load();
		sockaddr_in cli = {};
		int cliLen = sizeof(cli);
		SOCKET sock = accept(ls, (sockaddr*)&cli, &cliLen);

		if (sock == INVALID_SOCKET){
			if (!m_stopping){
				CString e;
				e.Format(_T("accept() thất bại, lỗi %d"), WSAGetLastError());
				PostLog(e);
				::PostMessage(m_hWnd, WM_SOCK_CLOSED, 0, 0);
			}
			return;
		}

		SOCKET old = m_listenSock.exchange(INVALID_SOCKET);
		if (old != INVALID_SOCKET)
			closesocket(old);

		m_clientSock = sock;
		if (m_stopping){
			SOCKET t = m_clientSock.exchange(INVALID_SOCKET);
			if (t != INVALID_SOCKET) closesocket(t);
			return;
		}

		TCHAR ipStr[INET_ADDRSTRLEN] = {};
		InetNtop(AF_INET, &cli.sin_addr, ipStr, _countof(ipStr));
		PostLog(CString(_T("Đã có bên kia kết nối: ")) + ipStr);
		::PostMessage(m_hWnd, WM_SOCK_CONNECTED, 0, 0);

		ReceiveLoop(sock);
	}
	else{
		SOCKET sock = m_clientSock.load();
		if (connect(sock, (sockaddr*)&m_addr, sizeof(m_addr)) == SOCKET_ERROR){
			if (!m_stopping){
				CString e;
				e.Format(_T("Kết nối thất bại, lỗi %d. Kiểm tra IP, port, firewall."), WSAGetLastError());
				PostLog(e);
				::PostMessage(m_hWnd, WM_SOCK_CLOSED, 0, 0);
			}
			return;
		}

		PostLog(_T("Kết nối thành công"));
		::PostMessage(m_hWnd, WM_SOCK_CONNECTED, 0, 0);

		ReceiveLoop(sock);
	}
}

void CSocKetDlg::ReceiveLoop(SOCKET sock)
{
	std::string buf;
	char tmp[4096];
	bool bad = false;
	int  err = 0;

	for (;;){
		int n = recv(sock, tmp, sizeof(tmp), 0);
		if (n > 0){
			buf.append(tmp, n);
		}
		else{
			if (n == SOCKET_ERROR) err = WSAGetLastError();
			break;
		}

		while (buf.size() >= 4){
			UINT len;
			memcpy(&len, buf.data(), 4);
			len = ntohl(len);

			if (len > MAX_MSG_SIZE) { bad = true; break; }
			if (buf.size() < 4 + (size_t)len) break;

			std::string msg = buf.substr(4, len);
			buf.erase(0, 4 + len);

			CString text(CA2W(msg.c_str(), CP_UTF8));
			PostLog(_T("Bên kia: ") + text);
		}
		if (bad) break;
	}

	if (!m_stopping){
		if (bad)
			PostLog(_T("Dữ liệu nhận không hợp lệ, ngắt kết nối"));
		else if (err != 0){
			CString e;
			e.Format(_T("Mất kết nối (lỗi %d)"), err);
			PostLog(e);
		}
		else
			PostLog(_T("Bên kia đã ngắt kết nối"));

		::PostMessage(m_hWnd, WM_SOCK_CLOSED, 0, 0);
	}
}

BOOL CSocKetDlg::SendAll(SOCKET s, const char* data, int len)
{
	int sent = 0;
	while (sent < len){
		int n = send(s, data + sent, len - sent, 0);
		if (n == SOCKET_ERROR)
			return FALSE;
		sent += n;
	}
	return TRUE;
}

void CSocKetDlg::OnBnClickedBtnSend()
{
	// TODO: Add your control notification handler code here
	SOCKET s = m_clientSock.load();
	if (s == INVALID_SOCKET)
		return;

	CString text;
	GetDlgItemText(IDC_EDIT_MSG, text);
	if (text.IsEmpty())
		return;

	CStringA utf8 = CW2A(text, CP_UTF8);
	UINT len = (UINT)utf8.GetLength();

	std::string packet;
	UINT netLen = htonl(len);
	packet.append((const char*)&netLen, 4);
	packet.append(utf8.GetString(), len);

	if (!SendAll(s, packet.data(), (int)packet.size()))
	{
		AddChat(_T("Gửi thất bại"));
		CloseAll();
		return;
	}

	AddChat(_T("Tôi: ") + text);
	SetDlgItemText(IDC_EDIT_MSG, _T(""));
	GetDlgItem(IDC_EDIT_MSG)->SetFocus();
}
