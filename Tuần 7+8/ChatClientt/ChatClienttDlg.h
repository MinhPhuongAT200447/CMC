#pragma once
#include "pch.h"
#include "afxdialogex.h"
#include "ClientCore.h"

class CChatDlg;

class CChatClienttDlg : public CDialogEx
{
public:
	CChatClienttDlg(CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHATCLIENTT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

	HICON      m_hIcon;
	CString    m_me;
	CString    m_pendingUser;
	CChatDlg* m_pChat = nullptr;

	void UpdateUi();
	void SetResult(const CString& s);
	void DoAuth(bool isRegister);
	void OpenChat();

	afx_msg LRESULT OnNetEvent(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnNetClosed(WPARAM wp, LPARAM lp);

	afx_msg void OnBnClickedBtnConnect();
	afx_msg void OnBnClickedBtnDisconnect();
	afx_msg void OnBnClickedBtnRegister();
	afx_msg void OnBnClickedBtnLogin();

	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
};