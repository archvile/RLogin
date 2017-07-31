#pragma once


// CScrnPage �_�C�A���O

class CScrnPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CScrnPage)

public:
	class COptDlg *m_pSheet;

	CScrnPage();   // �W���R���X�g���N�^
	virtual ~CScrnPage();
	virtual BOOL OnApply();
	virtual void OnReset();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_SCRNPAGE };
	int		m_ScrnFont;
	CString	m_HisMax;
	CString	m_FontSize;
	CString	m_ColsMax[2];
	CString	m_WheelSize;
	int m_VisualBell;
	int m_RecvCrLf;
	int m_SendCrLf;
	BOOL m_Check[4];
	CString m_HisFile;
	int m_MouseMode[4];

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g

	virtual BOOL OnInitDialog();
	afx_msg void OnHisfileSel();
	//}}AFX_MSG
	afx_msg void OnUpdateCheck(UINT nId);
	afx_msg void OnUpdateEdit();
	DECLARE_MESSAGE_MAP()
};