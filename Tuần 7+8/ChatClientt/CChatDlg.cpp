#include "pch.h"
#include "ChatClientt.h"
#include "CChatDlg.h"
#include "afxdialogex.h"
#include <algorithm>
#include <ctime>
#include <memory>

#define TIMER_REFRESH  1
#define REFRESH_MS     3000

IMPLEMENT_DYNAMIC(CChatDlg, CDialogEx)

CChatDlg::CChatDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CHAT_DIALOG, pParent)
{
}

CChatDlg::~CChatDlg()
{
}

void CChatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CChatDlg, CDialogEx)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTN_LOGOUT, &CChatDlg::OnBnClickedBtnLogout)
	ON_BN_CLICKED(IDC_BTN_REFRESH, &CChatDlg::OnBnClickedBtnRefresh)
	ON_BN_CLICKED(IDC_BTN_SEND, &CChatDlg::OnBnClickedBtnSend)
	ON_LBN_SELCHANGE(IDC_LIST_USERS, &CChatDlg::OnLbnSelchangeListUsers)
	ON_MESSAGE(WM_NET_EVENT, &CChatDlg::OnNetEvent)
	ON_MESSAGE(WM_NET_CLOSED, &CChatDlg::OnNetClosed)
	ON_STN_CLICKED(IDC_STATIC_CHAT, &CChatDlg::OnStnClickedStaticChat)
END_MESSAGE_MAP()

BOOL CChatDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	CenterWindow();

	SetDlgItemText(IDC_STATIC_ME, L"Đăng nhập với tên: " + m_me);
	
	UpdateUi();

	ClientSendGetUsers();
	SetTimer(TIMER_REFRESH, REFRESH_MS, nullptr);
	return TRUE;
}

static void EnableCtl(CWnd* dlg, int id, bool on)
{
	CWnd* c = dlg->GetDlgItem(id);
	if (c) c->EnableWindow(on ? TRUE : FALSE);
}

void CChatDlg::UpdateUi()
{
	bool canChat = !m_partner.IsEmpty();
	EnableCtl(this, IDC_EDIT_MSG, canChat);
	EnableCtl(this, IDC_BTN_SEND, canChat);
	if (!canChat)
		SetDlgItemText(IDC_STATIC_CHAT, L"Chọn một người trong danh sách để chat");
}

void CChatDlg::SetNotice(const CString& s)
{
	SetDlgItemText(IDC_STATIC_NOTICE, s);
}

void CChatDlg::AddChat(const CString& line, bool isMine)
{
	CRichEditCtrl* edit = (CRichEditCtrl*)GetDlgItem(IDC_EDIT_CHAT);

	if (!edit)
		return;

	int len = edit->GetWindowTextLength();
	edit->SetSel(len, len);

	PARAFORMAT pf = {};
	pf.cbSize = sizeof(PARAFORMAT);
	pf.dwMask = PFM_ALIGNMENT;
	pf.wAlignment = isMine ? PFA_RIGHT : PFA_LEFT;

	edit->SetParaFormat(pf);

	edit->ReplaceSel(line + L"\r\n");

	len = edit->GetWindowTextLength();
	edit->SetSel(len, len);
}

void CChatDlg::ClearChat()
{
	SetDlgItemText(IDC_EDIT_CHAT, L"");
}

CString CChatDlg::FormatMsg(const ChatMsg& m)
{
	CTime t((__time64_t)m.timestamp);
	CString who;
	if (m_me == m.from.c_str())
		who = L"Tôi";
	else
		who = m.from.c_str();
	return t.Format(L"[%H:%M] ") + who + L": " + m.content.c_str();
}

void CChatDlg::RebuildUserList()
{
	CListBox* lb = (CListBox*)GetDlgItem(IDC_LIST_USERS);
	int top = lb->GetTopIndex();
	lb->ResetContent();
	int sel = -1;
	for (size_t i = 0; i < m_users.size(); ++i){
		const UserInfo& u = m_users[i];
		CString line = u.online ? L"[Online]   " : L"[Offline]  ";
		line += u.name.c_str();
		auto it = m_unread.find(u.name);
		if (it != m_unread.end() && it->second > 0){
			CString n;
			n.Format(L"   (%d tin mới)", it->second);
			line += n;
		}
		lb->AddString(line);
		if (m_partner == u.name.c_str()) sel = (int)i;
	}
	if (sel >= 0) lb->SetCurSel(sel);
	if (top > 0 && top < lb->GetCount()) lb->SetTopIndex(top);
}

void CChatDlg::Finish(int code)
{
	KillTimer(TIMER_REFRESH);
	EndDialog(code);
}

void CChatDlg::OnBnClickedBtnLogout()
{
	if (!ClientSendLogout()) Finish(IDABORT);
}

void CChatDlg::OnBnClickedBtnRefresh()
{
	ClientSendGetUsers();
}

void CChatDlg::OnBnClickedBtnSend()
{
	CString text;
	GetDlgItemText(IDC_EDIT_MSG, text);
	text.Trim();
	if (m_partner.IsEmpty() || text.IsEmpty()) return;

	if (!ClientSendMessage((LPCWSTR)m_partner, (LPCWSTR)text)){
		SetNotice(L"Gửi tin thất bại, hãy kiểm tra kết nối.");
		return;
	}

	ChatMsg m;
	m.from = (LPCWSTR)m_me;
	m.to = (LPCWSTR)m_partner;
	m.content = (LPCWSTR)text;
	m.timestamp = (long long)time(nullptr);
	AddChat(FormatMsg(m), true);

	SetDlgItemText(IDC_EDIT_MSG, L"");
	if (CWnd* w = GetDlgItem(IDC_EDIT_MSG))
		w->SetFocus();
}

void CChatDlg::OnLbnSelchangeListUsers()
{
	CListBox* lb = (CListBox*)GetDlgItem(IDC_LIST_USERS);
	int i = lb->GetCurSel();
	if (i < 0 || i >= (int)m_users.size())
		return;

	m_partner = m_users[i].name.c_str();
	m_unread.erase(m_users[i].name);
	
	ClientSendGetMessages((LPCWSTR)m_partner);
	ClearChat();
	UpdateUi();
	SetDlgItemText(IDC_STATIC_CHAT, L"Đang chat với: " + m_partner);
	RebuildUserList();
	if (CWnd* w = GetDlgItem(IDC_EDIT_MSG)) w->SetFocus();
}

void CChatDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_REFRESH && ClientIsConnected())
		ClientSendGetUsers();
	CDialogEx::OnTimer(nIDEvent);
}

void CChatDlg::OnOK()
{
	CWnd* f = GetFocus();
	CWnd* msg = GetDlgItem(IDC_EDIT_MSG);
	if (f && msg && f->GetSafeHwnd() == msg->GetSafeHwnd())
		OnBnClickedBtnSend();
}

void CChatDlg::OnCancel()
{
	Finish(IDCANCEL);
}

LRESULT CChatDlg::OnNetEvent(WPARAM, LPARAM lp)
{
	std::unique_ptr<NetEvent> ev((NetEvent*)lp);
	const std::string& a = ev->action;
	CString msg(ev->msg.c_str());

	if (a == "logout")
	{
		Finish(IDOK);
	}
	else if (a == "get_friends")
	{
		if (ev->code == 0){
			m_users = ev->users;
			std::stable_sort(m_users.begin(), m_users.end(),
				[](const UserInfo& x, const UserInfo& y) { return x.online > y.online; });
			RebuildUserList();
		}
	}
	else if (a == "get_messages")
	{
		if (ev->code == 0){
			ClearChat();
			for (const ChatMsg& m : ev->messages){
				if (m_partner == m.from.c_str() || m_partner == m.to.c_str()) {
					bool isMine = (m.from == (LPCWSTR)m_me);
					AddChat(FormatMsg(m), isMine);
				}
			}
		}
		else{
			SetNotice(msg);
		}
	}
	else if (a == "send_message")
	{
		if (ev->code != 0)
			AddChat(L"[Lỗi] " + msg, false);
	}
	else if (a == "new_message")
	{
		if (!ev->messages.empty()){
			const ChatMsg& m = ev->messages[0];
			if (m_partner == m.from.c_str()){
				AddChat(FormatMsg(m), false);
			}
			else{
				m_unread[m.from]++;
				RebuildUserList();
				SetNotice(L"Có tin nhắn mới từ " + CString(m.from.c_str()));
			}
		}
	}
	else if (ev->code != 0)
	{
		SetNotice(msg);
	}
	return 0;
}

LRESULT CChatDlg::OnNetClosed(WPARAM, LPARAM)
{
	Finish(IDABORT);
	return 0;
}

void CChatDlg::OnStnClickedStaticChat()
{
	// TODO: Add your control notification handler code here
}
