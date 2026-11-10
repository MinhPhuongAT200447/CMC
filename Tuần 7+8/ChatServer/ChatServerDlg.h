
// ChatServerDlg.h : header file
//
#include "ServerCore.h"
#pragma once


// CChatServerDlg dialog
class CChatServerDlg : public CDialogEx
{
// Construction
public:
	CChatServerDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHATSERVER_DIALOG };
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
	DECLARE_MESSAGE_MAP()
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg LRESULT OnAppLog(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnAppUsers(WPARAM wp, LPARAM lp);
	void UpdateUi(bool running);
public:
	afx_msg void OnBnClickedBtnStart();
	afx_msg void OnBnClickedBtnStop();
	afx_msg void OnBnClickedBtnClear();
	afx_msg void OnBnClickedBtnInstall();
	afx_msg void OnBnClickedBtnUninstall();
	afx_msg void OnBnClickedBtnExit();
};
