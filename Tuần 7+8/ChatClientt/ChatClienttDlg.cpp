#include "pch.h"
#include "framework.h"
#include "ChatClientt.h"
#include "ChatClienttDlg.h"
#include "CChatDlg.h"
#include "afxdialogex.h"
#include <memory>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CChatClienttDlg::CChatClienttDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CHATCLIENTT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CChatClienttDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CChatClienttDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_CONNECT, &CChatClienttDlg::OnBnClickedBtnConnect)
	ON_BN_CLICKED(IDC_BTN_DISCONNECT, &CChatClienttDlg::OnBnClickedBtnDisconnect)
	ON_BN_CLICKED(IDC_BTN_REGISTER, &CChatClienttDlg::OnBnClickedBtnRegister)
	ON_BN_CLICKED(IDC_BTN_LOGIN, &CChatClienttDlg::OnBnClickedBtnLogin)
	ON_MESSAGE(WM_NET_EVENT, &CChatClienttDlg::OnNetEvent)
	ON_MESSAGE(WM_NET_CLOSED, &CChatClienttDlg::OnNetClosed)
END_MESSAGE_MAP()

BOOL CChatClienttDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	SetDlgItemText(IDC_EDIT_HOST, L"127.0.0.1");
	SetDlgItemInt(IDC_EDIT_PORT, 5000, FALSE);
	UpdateUi();

	return TRUE;
}

void CChatClienttDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CChatClienttDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

static void EnableCtl(CWnd* dlg, int id, bool on)
{
	CWnd* c = dlg->GetDlgItem(id);
	if (c) c->EnableWindow(on ? TRUE : FALSE);
}

void CChatClienttDlg::UpdateUi()
{
	bool conn = ClientIsConnected();

	EnableCtl(this, IDC_EDIT_HOST, !conn);
	EnableCtl(this, IDC_EDIT_PORT, !conn);
	EnableCtl(this, IDC_BTN_CONNECT, !conn);
	EnableCtl(this, IDC_BTN_DISCONNECT, conn);

	EnableCtl(this, IDC_EDIT_USER, conn);
	EnableCtl(this, IDC_EDIT_PASS, conn);
	EnableCtl(this, IDC_BTN_REGISTER, conn);
	EnableCtl(this, IDC_BTN_LOGIN, conn);

	SetDlgItemText(IDC_STATIC_STATE, conn ? L"Đã kết nối (chưa đăng nhập)" : L"Chưa kết nối");
}

void CChatClienttDlg::SetResult(const CString& s)
{
	SetDlgItemText(IDC_STATIC_RESULT, s);
}

void CChatClienttDlg::DoAuth(bool isRegister)
{
	CString user, pass;
	GetDlgItemText(IDC_EDIT_USER, user);
	GetDlgItemText(IDC_EDIT_PASS, pass);

	if (user.GetLength() < 3 || user.GetLength() > 20) {
		SetResult(L"Tên đăng nhập phải từ 3 đến 20 ký tự (chữ, số, _).");
		return;
	}
	if (pass.GetLength() < 4 || pass.GetLength() > 64) {
		SetResult(L"Mật khẩu phải từ 4 đến 64 ký tự.");
		return;
	}

	m_pendingUser = user;
	if (!ClientSendAuth(isRegister, (LPCWSTR)user, (LPCWSTR)pass))
		SetResult(L"Gửi yêu cầu thất bại, hãy kiểm tra kết nối.");
	else
		SetResult(isRegister ? L"Đang đăng ký..." : L"Đang đăng nhập...");
}

void CChatClienttDlg::OpenChat()
{
	CChatDlg dlg(this);
	dlg.m_me = m_me;
	m_pChat = &dlg;
	ShowWindow(SW_HIDE);
	INT_PTR r = dlg.DoModal();
	m_pChat = nullptr;

	if (r == IDCANCEL) {
		ClientDisconnect();
		CDialogEx::OnCancel();
		return;
	}

	ShowWindow(SW_SHOW);
	SetForegroundWindow();
	m_me.Empty();
	if (r == IDABORT) {
		ClientDisconnect();
		SetResult(L"Mất kết nối với máy chủ");
	}
	else {
		SetResult(L"Đã đăng xuất");
	}
	UpdateUi();
}

void CChatClienttDlg::OnBnClickedBtnConnect()
{
	CString host;
	GetDlgItemText(IDC_EDIT_HOST, host);
	BOOL ok = FALSE;
	UINT port = GetDlgItemInt(IDC_EDIT_PORT, &ok, FALSE);
	if (host.IsEmpty() || !ok || port < 1 || port > 65535){
		AfxMessageBox(L"Hãy nhập địa chỉ máy chủ và cổng hợp lệ (1-65535).", MB_ICONWARNING);
		return;
	}

	CWaitCursor wait;
	std::wstring err;
	if (!ClientConnect((LPCWSTR)host, (int)port, m_hWnd, err)){
		SetResult(err.c_str());
		return;
	}
	SetResult(L"Đã kết nối tới " + host);
	UpdateUi();
}

void CChatClienttDlg::OnBnClickedBtnDisconnect()
{
	ClientDisconnect();
	SetResult(L"Đã ngắt kết nối");
	UpdateUi();
}

void CChatClienttDlg::OnBnClickedBtnRegister()
{
	DoAuth(true);
}
void CChatClienttDlg::OnBnClickedBtnLogin() 
{
	DoAuth(false);
}

void CChatClienttDlg::OnOK()
{
	if (ClientIsConnected())
		DoAuth(false);
}

void CChatClienttDlg::OnCancel()
{
	ClientDisconnect();
	CDialogEx::OnCancel();
}

LRESULT CChatClienttDlg::OnNetEvent(WPARAM wp, LPARAM lp)
{

	if (m_pChat && m_pChat->GetSafeHwnd())
		return m_pChat->SendMessage(WM_NET_EVENT, wp, lp);

	std::unique_ptr<NetEvent> ev((NetEvent*)lp);
	const std::string& a = ev->action;
	CString msg(ev->msg.c_str());
	bool openChat = false;

	if (a == "register")
	{
		SetResult(msg);
		if (ev->code == 0) SetDlgItemText(IDC_EDIT_PASS, L"");
	}
	else if (a == "login")
	{
		SetResult(msg);
		if (ev->code == 0){
			m_me = m_pendingUser;
			SetDlgItemText(IDC_EDIT_PASS, L"");
			openChat = true;
		}
	}
	else if (ev->code != 0)
	{
		SetResult(msg);
	}

	ev.reset();
	if (openChat)
		OpenChat();
	return 0;
}

LRESULT CChatClienttDlg::OnNetClosed(WPARAM wp, LPARAM lp)
{
	if (m_pChat && m_pChat->GetSafeHwnd())
		return m_pChat->SendMessage(WM_NET_CLOSED, wp, lp);

	ClientDisconnect();
	m_me.Empty();
	SetResult(L"Mất kết nối với máy chủ");
	UpdateUi();
	return 0;
}