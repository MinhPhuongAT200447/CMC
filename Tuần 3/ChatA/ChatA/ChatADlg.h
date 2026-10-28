
// ChatADlg.h : header file
//

#pragma once
#define WM_PIPE_MSG (WM_APP + 1)


// CChatADlg dialog
class CChatADlg : public CDialogEx
{
// Construction
public:
	CChatADlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHATA_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support



// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	CWinThread* m_pThread = NULL;
	volatile bool m_bStop = false;

	void AppendLog(const CString& line);
	bool SendByWM(const CString& text);
	bool SendByPipe(const CString& text);
	static UINT PipeServerThread(LPVOID pParam);

	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCDS);
	afx_msg LRESULT OnPipeMsg(WPARAM wParam, LPARAM lParam);
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_edtHistory;
	CEdit m_edtInput;
	CButton m_radWM;
	CButton m_radPipe;
	afx_msg void OnBnClickedBtnSend();
};
