// CLoginDlg.cpp : implementation file
//

#include "pch.h"
#include "ChatApp.h"
#include "CLoginDlg.h"
#include "afxdialogex.h"
#include <map>

static std::map<CString, CString> g_users = { {_T("admin"), _T("123456")} };

// CLoginDlg dialog

static CString Txt(HWND h, int id) {
	TCHAR b[256] = {};
	GetDlgItemText(h, id, b, 256);
	return b;
}

IMPLEMENT_DYNAMIC(CLoginDlg, CDialogEx)

CLoginDlg::CLoginDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_LOGIN, pParent)
{

}

CLoginDlg::~CLoginDlg()
{
}

void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CLoginDlg, CDialogEx)
	ON_BN_CLICKED(IDC_LOGIN, &CLoginDlg::OnBnClickedLogin)
	ON_BN_CLICKED(IDC_REG, &CLoginDlg::OnBnClickedReg)
END_MESSAGE_MAP()


// CLoginDlg message handlers
BOOL CLoginDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	::SendDlgItemMessage(m_hWnd, IDC_USER, EM_SETCUEBANNER, TRUE, (LPARAM)L"Tên đăng nhập");
	::SendDlgItemMessage(m_hWnd, IDC_PASS, EM_SETCUEBANNER, TRUE, (LPARAM)L"Mật khẩu");
	return TRUE;
}

void CLoginDlg::OnOK()
{
	OnBnClickedLogin();
}

void CLoginDlg::OnBnClickedLogin() {
	CString u = Txt(m_hWnd, IDC_USER), p = Txt(m_hWnd, IDC_PASS);
	if (g_users.count(u) && g_users[u] == p) {
		m_user = u;
		EndDialog(IDOK);
	}else
		::MessageBox(m_hWnd, _T("Sai tên đăng nhập hoặc mật khẩu"), _T("Lỗi"), MB_ICONWARNING);
}

void CLoginDlg::OnBnClickedReg()
{
	CString u = Txt(m_hWnd, IDC_USER), p = Txt(m_hWnd, IDC_PASS);

	if (u.IsEmpty() || p.IsEmpty() || g_users.count(u))
		::MessageBox(m_hWnd, _T("Thiếu thông tin hoặc tên đã tồn tại"), _T("Lỗi"), MB_ICONWARNING);
	else
	{
		g_users[u] = p;
		::MessageBox(m_hWnd, _T("Đăng ký thành công, hãy đăng nhập"), _T("OK"), MB_OK);
	}
}