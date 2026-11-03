
// ChatServerDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "ChatServer.h"
#include "ChatServerDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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


// CChatServerDlg dialog



CChatServerDlg::CChatServerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CHATSERVER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CChatServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CChatServerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_START, &CChatServerDlg::OnBnClickedBtnStart)
	ON_BN_CLICKED(IDC_BTN_STOP, &CChatServerDlg::OnBnClickedBtnStop)
	ON_BN_CLICKED(IDC_BTN_CLEAR, &CChatServerDlg::OnBnClickedBtnClear)
	ON_BN_CLICKED(IDC_BTN_INSTALL, &CChatServerDlg::OnBnClickedBtnInstall)
	ON_BN_CLICKED(IDC_BTN_UNINSTALL, &CChatServerDlg::OnBnClickedBtnUninstall)
	ON_BN_CLICKED(IDC_BTN_EXIT, &CChatServerDlg::OnBnClickedBtnExit)
	ON_MESSAGE(WM_APP_LOG, &CChatServerDlg::OnAppLog)
	ON_MESSAGE(WM_APP_USERS, &CChatServerDlg::OnAppUsers)
END_MESSAGE_MAP()


// CChatServerDlg message handlers

BOOL CChatServerDlg::OnInitDialog()
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
	SetLogWindow(m_hWnd);
	SetDlgItemInt(IDC_EDIT_PORT, ReadPortFromIni(), FALSE);
	SendDlgItemMessage(IDC_LIST_LOG, LB_SETHORIZONTALEXTENT, 2000, 0);
	UpdateUi(false);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CChatServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CChatServerDlg::OnPaint()
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
HCURSOR CChatServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CChatServerDlg::OnOK() {} 

void CChatServerDlg::OnCancel()
{
	SetLogWindow(NULL);
	ServerStop();
	CDialogEx::OnCancel();
}

void CChatServerDlg::UpdateUi(bool running)
{
	GetDlgItem(IDC_BTN_START)->EnableWindow(!running);
	GetDlgItem(IDC_BTN_STOP)->EnableWindow(running);
	GetDlgItem(IDC_EDIT_PORT)->EnableWindow(!running);
	SetDlgItemText(IDC_STATIC_STATUS, running ? L"Trạng thái: ĐANG CHẠY" : L"Trạng thái: Đã dừng");
}

void CChatServerDlg::OnBnClickedBtnStart()
{
	BOOL ok = FALSE;
	UINT port = GetDlgItemInt(IDC_EDIT_PORT, &ok, FALSE);
	if (!ok || port < 1 || port > 65535) {
		AfxMessageBox(L"Cổng phải là số từ 1 đến 65535.", MB_ICONWARNING);
		return;
	}
	std::wstring err;
	if (!ServerStart((int)port, err)) {
		AfxMessageBox(err.c_str(), MB_ICONERROR);
		return;
	}
	SavePortToIni((int)port);
	UpdateUi(true);
}

void CChatServerDlg::OnBnClickedBtnStop()
{
	ServerStop();
	UpdateUi(false);
}

void CChatServerDlg::OnBnClickedBtnClear()
{
	SendDlgItemMessage(IDC_LIST_LOG, LB_RESETCONTENT, 0, 0);
}

void CChatServerDlg::OnBnClickedBtnInstall()
{
	AfxMessageBox(InstallService().c_str(), MB_ICONINFORMATION);
}

void CChatServerDlg::OnBnClickedBtnUninstall()
{
	AfxMessageBox(UninstallService().c_str(), MB_ICONINFORMATION);
}

void CChatServerDlg::OnBnClickedBtnExit()
{
	OnCancel();
}

LRESULT CChatServerDlg::OnAppLog(WPARAM, LPARAM lp)
{
	wchar_t* s = (wchar_t*)lp;
	LRESULT idx = SendDlgItemMessage(IDC_LIST_LOG, LB_ADDSTRING, 0, (LPARAM)s);
	free(s);
	if (SendDlgItemMessage(IDC_LIST_LOG, LB_GETCOUNT, 0, 0) > 2000) {
		SendDlgItemMessage(IDC_LIST_LOG, LB_DELETESTRING, 0, 0);
		idx--;
	}
	SendDlgItemMessage(IDC_LIST_LOG, LB_SETTOPINDEX, (WPARAM)idx, 0);
	return 0;
}

LRESULT CChatServerDlg::OnAppUsers(WPARAM, LPARAM)
{
	std::vector<std::string> names = OnlineNames();
	SendDlgItemMessage(IDC_LIST_ONLINE, LB_RESETCONTENT, 0, 0);
	for (auto& n : names)
		SendDlgItemMessage(IDC_LIST_ONLINE, LB_ADDSTRING, 0, (LPARAM)Utf8ToWide(n).c_str());
	CString t; t.Format(L"Đang online: %d", (int)names.size());
	SetDlgItemText(IDC_STATIC_ONLINE, t);
	return 0;
}