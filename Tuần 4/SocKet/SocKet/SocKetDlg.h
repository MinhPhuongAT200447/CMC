
// SocKetDlg.h : header file
//

#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")   // lien ket thu vien Winsock2

#define WM_SOCK_LOG (WM_APP + 1)
#define WM_SOCK_CONNECTED (WM_APP + 2)
#define WM_SOCK_CLOSED (WM_APP + 3)
// CSocKetDlg dialog
class CSocKetDlg : public CDialogEx
{
// Construction
public:
	CSocKetDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SOCKET_DIALOG };
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
	afx_msg void OnDestroy();
	DECLARE_MESSAGE_MAP()
	
public:
	afx_msg void OnBnClickedBtnListen();
	afx_msg void OnBnClickedBtnConnect();
	afx_msg void OnBnClickedBtnDisconnect();
	afx_msg void OnBnClickedBtnSend();

protected:
	afx_msg LRESULT OnSockLog(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSockConnected(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSockClosed(WPARAM wParam, LPARAM lParam);

	std::atomic<SOCKET> m_listenSock{ INVALID_SOCKET };
	std::atomic<SOCKET> m_clientSock{ INVALID_SOCKET };
	std::atomic<bool> m_stopping{ false };
	HANDLE m_hThread = NULL;
	bool m_isServer = false;
	sockaddr_in m_addr = {};

	static DWORD WINAPI ThreadProc(LPVOID lpParam);
	void ThreadMain();
	void ReceiveLoop(SOCKET sock);
	BOOL SendAll(SOCKET s, const char* data, int len);

	void PostLog(const CString& s);
	void AddChat(const CString& s);
	void SetUiState(int state);
	void CloseAll();
};
