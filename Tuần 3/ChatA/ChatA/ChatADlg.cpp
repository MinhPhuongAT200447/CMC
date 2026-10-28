
// ChatADlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "ChatA.h"
#include "ChatADlg.h"
#include "afxdialogex.h"

#define MY_TITLE _T("Chat MFC - App A")
#define PEER_TITLE _T("Chat MFC - App B")
#define MY_PIPE _T("\\\\.\\pipe\\ChatMFC_A")
#define PEER_PIPE _T("\\\\.\\pipe\\ChatMFC_B")
#define PEER_NAME _T("App B")
#define COPYDATA_ID 0x4348
#define PIPE_BUF_BYTES 4096


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


// CChatADlg dialog



CChatADlg::CChatADlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CHATA_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CChatADlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDT_HISTORY, m_edtHistory);
	DDX_Control(pDX, IDC_EDT_INPUT, m_edtInput);
	DDX_Control(pDX, IDC_RAD_WM, m_radWM);
	DDX_Control(pDX, IDC_RAD_PIPE, m_radPipe);
}

BEGIN_MESSAGE_MAP(CChatADlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_SEND, &CChatADlg::OnBnClickedBtnSend)

	ON_WM_COPYDATA()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_PIPE_MSG, &CChatADlg::OnPipeMsg)
END_MESSAGE_MAP()


// CChatADlg message handlers

BOOL CChatADlg::OnInitDialog()
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
	SetWindowText(MY_TITLE);
	m_radWM.SetCheck(BST_CHECKED);
	m_edtInput.SetLimitText(1000);

	m_pThread = AfxBeginThread(PipeServerThread, this, 0, 0, CREATE_SUSPENDED);
	m_pThread->m_bAutoDelete = FALSE;
	m_pThread->ResumeThread();

	AppendLog(_T("Mở app còn lại để chạy"));

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CChatADlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CChatADlg::OnPaint()
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
HCURSOR CChatADlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CChatADlg::AppendLog(const CString& line) {
	int len = m_edtHistory.GetWindowTextLengthW();
	m_edtHistory.SetSel(len, len);
	m_edtHistory.ReplaceSel(line + _T("\r\n"));
}

void CChatADlg::OnBnClickedBtnSend()
{
	// TODO: Add your control notification handler code here
	CString text;
	m_edtInput.GetWindowText(text);
	text.Trim();
	if (text.IsEmpty()) {
		return;
	}

	bool viaPipe = (m_radPipe.GetCheck() == BST_CHECKED);
	bool ok = viaPipe ? SendByPipe(text) : SendByWM(text);

	CString t = CTime::GetCurrentTime().Format(_T("%H:%M:%S"));
	if (ok) {
		AppendLog(t + _T("Tôi [") + CString(viaPipe ? _T("Name pipe") : _T("Windows message")) + _T("]: ") + text);
	}
	else {
		AppendLog(t + _T("Lỗi, không gửi được"));
	}
	m_edtInput.SetWindowTextW(_T(""));
	m_edtInput.SetFocus();
}

bool CChatADlg::SendByWM(const CString& text) {
	HWND hPeer = ::FindWindow(NULL, PEER_TITLE);
	if (!hPeer)
		return false;
	COPYDATASTRUCT cds;
	cds.dwData = COPYDATA_ID;
	cds.cbData = (text.GetLength() + 1) * sizeof(TCHAR);
	cds.lpData = (LPVOID)(LPCTSTR)text;
	::SendMessage(hPeer, WM_COPYDATA, (WPARAM)m_hWnd, (LPARAM)&cds);
	return true;
}

BOOL CChatADlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCDS) {
	if (pCDS->dwData != COPYDATA_ID || pCDS->cbData == 0)
		return FALSE;
	int len = pCDS->cbData / sizeof(TCHAR);
	LPCTSTR p = (LPCTSTR)pCDS->lpData;
	if (len > 0 && p[len - 1] == 0)
		len--;
	CString s(p, len);
	AppendLog(CTime::GetCurrentTime().Format(_T("%H:%M:%S")) + _T(" ") PEER_NAME _T(" [Windows message]: ") + s);
	return TRUE;
}

bool CChatADlg::SendByPipe(const CString& text) {
	if (!::WaitNamedPipe(PEER_PIPE, 1000))
		return false;
	HANDLE h = ::CreateFile(PEER_PIPE, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE)
		return false;
	DWORD written = 0;
	bool ok = ::WriteFile(h, (LPCTSTR)text, (text.GetLength() + 1) * sizeof(TCHAR), &written, NULL);
	::CloseHandle(h);
	return ok != false;
}

UINT CChatADlg::PipeServerThread(LPVOID pParam) {
	CChatADlg* self = (CChatADlg*)pParam;
	while (!self->m_bStop) {
		HANDLE hPipe = ::CreateNamedPipe(MY_PIPE, PIPE_ACCESS_INBOUND, PIPE_TYPE_MESSAGE
			| PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES,
			0, PIPE_BUF_BYTES, 0, NULL);
		if (hPipe == INVALID_HANDLE_VALUE) {
			::Sleep(300);
			continue;
		}
		bool connected = ::ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
		if (connect && !self->m_bStop) {
			TCHAR buf[PIPE_BUF_BYTES / sizeof(TCHAR)];
			DWORD n = 0;
			if (::ReadFile(hPipe, buf, PIPE_BUF_BYTES - sizeof(TCHAR), &n, NULL) && n > 0) {
				buf[n / sizeof(TCHAR)] = 0;
				CString* ps = new CString(buf);
				if (!::PostMessage(self->m_hWnd, WM_PIPE_MSG, 0, (LPARAM)ps))
					delete ps;
			}
		}
		::DisconnectNamedPipe(hPipe);
		::CloseHandle(hPipe);
	}
	return 0;
}

LRESULT CChatADlg::OnPipeMsg(WPARAM, LPARAM lParam) {
	CString* ps = (CString*)lParam;
	AppendLog(CTime::GetCurrentTime().Format(_T("%H:%M:%S")) + _T(" ") PEER_NAME _T("[Named pipe]: ") + *ps);
	delete ps;
	return 0;
}

void CChatADlg::OnDestroy() {
	m_bStop = true;
	HANDLE h = ::CreateFile(MY_PIPE, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (h != INVALID_HANDLE_VALUE)
		::CloseHandle(h);
	if (m_pThread) {
		::WaitForSingleObject(m_pThread->m_hThread, 2000);
		delete m_pThread;
		m_pThread = NULL;
	}
	CDialogEx::OnDestroy();
}