// AutoRenDlg.cpp : �����t�@�C��
//

#include "stdafx.h"
#include "RLogin.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "Ssh.h"
#include "SFtp.h"
#include "AutoRenDlg.h"

// CAutoRenDlg �_�C�A���O

IMPLEMENT_DYNAMIC(CAutoRenDlg, CDialog)

CAutoRenDlg::CAutoRenDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAutoRenDlg::IDD, pParent)
{
	m_Exec = 0;
}

CAutoRenDlg::~CAutoRenDlg()
{
}

void CAutoRenDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_Name[0]);
	DDX_Text(pDX, IDC_EDIT2, m_Name[1]);
	DDX_Text(pDX, IDC_EDIT3, m_Name[2]);
	DDX_Text(pDX, IDC_NAMEOK, m_NameOK);
	DDX_Radio(pDX, IDC_RADIO1, m_Exec);
}


BEGIN_MESSAGE_MAP(CAutoRenDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON1, &CAutoRenDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CAutoRenDlg::OnBnClickedButton2)
	ON_EN_UPDATE(IDC_EDIT3, &CAutoRenDlg::OnEnUpdateEdit3)
END_MESSAGE_MAP()


// CAutoRenDlg ���b�Z�[�W �n���h��

void CAutoRenDlg::OnBnClickedButton1()
{
	UpdateData(TRUE);
	OnOK();
}

void CAutoRenDlg::OnBnClickedButton2()
{
	UpdateData(TRUE);
	OnOK();
	m_Exec |= 0x80;
}

void CAutoRenDlg::OnEnUpdateEdit3()
{
	CString tmp;
	CFileNode node;

	UpdateData(TRUE);
	node.AutoRename(m_Name[2], tmp, 1);
	m_NameOK = (tmp.Compare(m_Name[2]) == 0 ? "��" : "�~");
	UpdateData(FALSE);
}

BOOL CAutoRenDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	return TRUE;
}