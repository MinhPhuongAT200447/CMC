#pragma once
#include "afxdialogex.h"
#include <map>
#include <vector>

struct Msg {
	CString time, who, text;
};
// CChatDlg dialog

class CChatDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CChatDlg)

public:
	CChatDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CChatDlg();

	CString m_user, m_cur;
	std::map<CString, std::vector<Msg>> m_hist;
	void Show();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHAT };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedSend();
	afx_msg void OnTvnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult);

};
