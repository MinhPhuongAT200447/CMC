// CChatDlg.cpp : implementation file
//

#include "pch.h"
#include "ChatApp.h"
#include "CChatDlg.h"
#include "afxdialogex.h"


// CChatDlg dialog

static CString Txt(HWND h, int id)
{
	TCHAR b[256] = {};
	::GetDlgItemText(h, id, b, 256);
	return b;
}

IMPLEMENT_DYNAMIC(CChatDlg, CDialogEx)

CChatDlg::CChatDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CHAT, pParent)
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
	ON_BN_CLICKED(IDC_SEND, &CChatDlg::OnBnClickedSend)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE, &CChatDlg::OnTvnSelchangedTree)

END_MESSAGE_MAP()


// CChatDlg message handlers
BOOL CChatDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_hist[_T("Chung")] = {
		{ _T("08:01"), _T("An"),   _T("Chào mọi người!") },
		{ _T("08:02"), m_user,     _T("Chào An") },
		{ _T("08:05"), _T("Bình"), _T("Hello") },
		{ _T("08:06"), m_user,     _T("Đúng rồi!") } };
	m_hist[_T("Lớp MFC")] = {
		{ _T("09:00"), _T("Dũng"), _T("Ai làm BT chưa?") },
		{ _T("09:03"), m_user,     _T("Mình.") } };
	m_hist[_T("An")] = { { _T("10:00"), _T("An"), _T("Gửi tài liệu với.") },
						   { _T("10:02"), m_user,   _T("Ok, mình gửi liền.") } };
	m_hist[_T("Bình")] = { { _T("11:30"), _T("Bình"), _T("Ok bạn nhé!") } };


	HWND c = ::GetDlgItem(m_hWnd, IDC_CHAT);
	ListView_SetExtendedListViewStyle(c, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	RECT rc;
	::GetClientRect(c, &rc);
	int totalWidth = rc.right - rc.left;;
	LVCOLUMN k = { LVCF_TEXT | LVCF_WIDTH };
	k.cx = 75; k.pszText = (LPTSTR)_T("TG");
	ListView_InsertColumn(c, 0, &k);
	k.cx = (totalWidth - 75) / 2;
	k.pszText = (LPTSTR)_T("Người A");
	ListView_InsertColumn(c, 1, &k);
	k.pszText = (LPTSTR)_T("Người B (tôi)");
	ListView_InsertColumn(c, 2, &k);

	HWND l = ::GetDlgItem(m_hWnd, IDC_LIST);
	ListView_SetExtendedListViewStyle(l, LVS_EX_FULLROWSELECT);
	k.cx = 100;
	k.pszText = (LPTSTR)_T("Tên");
	ListView_InsertColumn(l, 0, &k);
	k.pszText = (LPTSTR)_T("Trạng thái");
	ListView_InsertColumn(l, 1, &k);
	LPCTSTR name[] = { _T("An"), _T("Bình"), _T("Chi") };
	LPCTSTR st[] = { _T("Online"), _T("Online"), _T("Bận") };
	for (int i = 0; i < 3; i++)
	{
		LVITEM it = { LVIF_TEXT };
		it.iItem = i;
		it.pszText = (LPTSTR)name[i];
		ListView_InsertItem(l, &it);
		ListView_SetItemText(l, i, 1, (LPTSTR)st[i]);
	}

	HWND t = ::GetDlgItem(m_hWnd, IDC_TREE);
	TVINSERTSTRUCT tv = {};
	tv.hInsertAfter = TVI_LAST;
	tv.item.mask = TVIF_TEXT;
	auto add = [&](HTREEITEM p, LPCTSTR s) {
		tv.hParent = p;
		tv.item.pszText = (LPTSTR)s;
		return TreeView_InsertItem(t, &tv);
	};
	HTREEITEM r = add(TVI_ROOT, _T("Phòng chat"));
	HTREEITEM g = add(r, _T("Chung"));
	add(r, _T("Lớp MFC"));
	HTREEITEM f = add(TVI_ROOT, _T("Bạn bè"));
	add(f, _T("An"));
	add(f, _T("Bình"));
	TreeView_Expand(t, r, TVE_EXPAND);
	TreeView_Expand(t, f, TVE_EXPAND);
	TreeView_SelectItem(t, g);

	return TRUE;
}

void CChatDlg::Show()
{
	HWND l = ::GetDlgItem(m_hWnd, IDC_CHAT);
	ListView_DeleteAllItems(l);
	int i = 0;
	for (const Msg& m : m_hist[m_cur])
	{
		bool mine = (m.who == m_user);
		CString text = mine ? m.text : m.who + _T(": ") + m.text;

		LVITEM it = { LVIF_TEXT };
		it.iItem = i;
		it.pszText = (LPTSTR)(LPCTSTR)m.time;
		ListView_InsertItem(l, &it);
		ListView_SetItemText(l, i, mine ? 2 : 1, (LPTSTR)(LPCTSTR)text);
		++i;
	}
	if (i) ListView_EnsureVisible(l, i - 1, FALSE);
}

void CChatDlg::OnOK()
{
	OnBnClickedSend();
}

void CChatDlg::OnBnClickedSend()
{
	CString t = Txt(m_hWnd, IDC_MSG);
	if (t.IsEmpty() || m_cur.IsEmpty()) return;

	m_hist[m_cur].push_back({ CTime::GetCurrentTime().Format(_T("%H:%M")), m_user, t });
	Show();
	::SetDlgItemText(m_hWnd, IDC_MSG, _T(""));
}

void CChatDlg::OnTvnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	*pResult = 0;

	HWND t = ::GetDlgItem(m_hWnd, IDC_TREE);
	HTREEITEM it = pNMTreeView->itemNew.hItem;
	if (!it || !TreeView_GetParent(t, it)) return;

	TCHAR b[128] = {};
	TVITEM v = { TVIF_TEXT | TVIF_HANDLE, it };
	v.pszText = b; v.cchTextMax = 128;
	TreeView_GetItem(t, &v);

	m_cur = b;
	::SetWindowText(m_hWnd, _T("Chat - ") + m_user + _T("  |  ") + m_cur);
	Show();
}