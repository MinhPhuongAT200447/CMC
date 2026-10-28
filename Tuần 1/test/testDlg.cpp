
// testDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "test.h"
#include "testDlg.h"
#include "afxdialogex.h"
#include <vector>

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


// CtestDlg dialog



CtestDlg::CtestDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TEST_DIALOG, pParent)
	, stPath(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CtestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CONTENT, editContent);
	DDX_Text(pDX, IDC_PATH, stPath);
}

BEGIN_MESSAGE_MAP(CtestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_FOLDER, &CtestDlg::OnBnClickedFolder)
	ON_BN_CLICKED(IDC_SAVE, &CtestDlg::OnBnClickedSave)
	ON_BN_CLICKED(IDC_LOAD, &CtestDlg::OnBnClickedLoad)
	
END_MESSAGE_MAP()


// CtestDlg message handlers

BOOL CtestDlg::OnInitDialog()
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

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CtestDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CtestDlg::OnPaint()
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
HCURSOR CtestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CtestDlg::OnEnChangeEdit1()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CtestDlg::OnBnClickedFolder()
{
	// TODO: Add your control notification handler code here
	CFolderPickerDialog dlg(NULL, 0, this);
	if (dlg.DoModal() == IDOK) {
		stPath = dlg.GetPathName();
		UpdateData(FALSE);
	}
}


void CtestDlg::OnBnClickedSave()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);

	if (stPath.IsEmpty()) {
		AfxMessageBox(_T("Chưa chọn thư mục"));
		return;
	}

	CString content;
	editContent.GetWindowText(content);
	CString folderPath = stPath;
	if (folderPath.Right(4).CompareNoCase(_T(".txt")) == 0) {
		int lastSlash = folderPath.ReverseFind(_T('\\'));
		if (lastSlash != -1) {
			folderPath = folderPath.Left(lastSlash);
		}
	}
	CString path = folderPath + _T("\\data.txt");
	CStdioFile file;
	if (file.Open(path, CFile::modeCreate | CFile::modeWrite)) {
		int len = WideCharToMultiByte(CP_UTF8, 0, content, -1, NULL, 0, NULL, NULL);
		std::vector<char> utf8Buf(len);
		WideCharToMultiByte(CP_UTF8, 0, content, -1, utf8Buf.data(), len, NULL, NULL);

		unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
		file.Write(bom, 3);
		file.Write(utf8Buf.data(), (UINT)(len - 1));

		UpdateData(FALSE);
		AfxMessageBox(_T("Lưu thành công"));
	}
	
	else {
		AfxMessageBox(_T("Không thể mở file để ghi"));
	}
}


void CtestDlg::OnBnClickedLoad()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);

	if (stPath.IsEmpty()) {
		AfxMessageBox(_T("Chưa chọn folder, file"));
		return;
	}

	CString path = stPath;
	if (path.Right(4).CompareNoCase(_T(".txt")) != 0) {
		if (path.Right(1) != _T("\\")) {
			path += _T("\\");
		}
		path += _T("data.txt");
	}

	CFile file;
	
	if (file.Open(path, CFile::modeRead)) {
		ULONGLONG filelen = file.GetLength();
		std::vector<char> buffer((size_t)filelen, 0);
		file.Read(buffer.data(), (UINT)filelen);
		file.Close();

		char* dataPtr = buffer.data();
		int dataLen = (int)filelen;
		if (filelen >= 3 && (unsigned char)dataPtr[0] == 0xEF && (unsigned char)dataPtr[1] == 0xBB && (unsigned char)dataPtr[2] == 0xBF) {
			dataPtr += 3;
			dataLen -= 3;
		}

		int wlen = MultiByteToWideChar(CP_UTF8, 0, dataPtr, dataLen, NULL, 0);
		std::vector<wchar_t> wbuf(wlen + 1, 0);
		MultiByteToWideChar(CP_UTF8, 0, dataPtr, dataLen, wbuf.data(), wlen);

		CStringW finalContent(wbuf.data());
		finalContent.Replace(_T("\n"), _T("\r\n"));
		finalContent.Replace(_T("\r\r\n"), _T("\r\n"));
		editContent.SetWindowTextW(finalContent);
		AfxMessageBox(_T("Load thành công"));
	}
	else {
		AfxMessageBox(_T("Không tìm thấy file thư mục"));
	}

}


