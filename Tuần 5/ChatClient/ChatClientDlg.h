
#pragma once

#define WM_SOCK   (WM_APP + 1)
#define RX_SIZE   8192

class CChatClientDlg : public CDialogEx
{
public:
    CChatClientDlg(CWnd* pParent = nullptr);

    enum { IDD = IDD_CHATCLIENT_DIALOG };

protected:
    HICON   m_hIcon;
    SOCKET  m_sock;
    BOOL    m_connected;
    char    m_rx[RX_SIZE];
    int     m_rxLen;

    void AppendLog(const CString& prefix, const CString& text);
    void Disconnect();
    void ExtractLines();

    virtual BOOL OnInitDialog();
    virtual void OnOK();

    afx_msg void    OnDestroy();
    afx_msg void    OnBnClickedConnect();
    afx_msg void    OnBnClickedSend();
    afx_msg void    OnBnClickedAutorun();
    afx_msg LRESULT OnSocket(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};
