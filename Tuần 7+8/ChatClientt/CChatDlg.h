#pragma once
#include <map>
#include <string>
#include <vector>
#include "pch.h"
#include "afxdialogex.h"
#include "ClientCore.h"

class CChatDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CChatDlg)

public:
	CChatDlg(CWnd* pParent = nullptr);
	virtual ~CChatDlg();

	CString m_me;

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHAT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

	CString                     m_partner;
	std::vector<UserInfo>       m_users;
	std::map<std::wstring, int> m_unread;

	void    UpdateUi();
	void    SetNotice(const CString& s);
	void    AddChat(const CString& line, bool isMine);
	void    ClearChat();
	CString FormatMsg(const ChatMsg& m);
	void    RebuildUserList();
	void    Finish(int code);


	afx_msg LRESULT OnNetEvent(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnNetClosed(WPARAM wp, LPARAM lp);

	afx_msg void OnBnClickedBtnLogout();
	afx_msg void OnBnClickedBtnRefresh();
	afx_msg void OnBnClickedBtnSend();
	afx_msg void OnLbnSelchangeListUsers();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnStnClickedStaticChat();
};