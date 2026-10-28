
// ChatBDlg.h : header file
//

#pragma once
#define WM_PIPE_MSG (WM_APP + 1)

// CChatBDlg dialog
class CChatBDlg : public CDialogEx
{
// Construction
public:
	CChatBDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHATB_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	CWinThread* m_pThread = NULL;
	volatile bool m_bStop = false;

	void AppendLog(const CString& line);
	bool SendByWM(const CString& text);
	bool SendByPipe(const CString& text);
	static UINT PipeServerThread(LPVOID pParam);

	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCDS);
	afx_msg LRESULT OnPipeMsg(WPARAM wParam, LPARAM lParam);
	afx_msg void OnDestroy();

	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CEdit m_edtInput;
	CEdit m_edtHistory;
	CButton m_radWM;
	CButton m_radPipe;
	afx_msg void OnBnClickedBtnSend();
};
