// ProgDlg.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "rlogin.h"
#include "ProgDlg.h"
#include "ExtSocket.h"
#include "SyncSock.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProgDlg �_�C�A���O


CProgDlg::CProgDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CProgDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CProgDlg)
	m_EndTime = _T("");
	m_TotalSize = _T("");
	m_TransRate = _T("");
	m_FileName = _T("");
	m_Message = _T("");
	//}}AFX_DATA_INIT
	m_Div = 1;
	m_AbortFlag = FALSE;
	m_pSock = NULL;
}


void CProgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProgDlg)
	DDX_Control(pDX, IDC_SIZEPROG, m_FileSize);
	DDX_Text(pDX, IDC_ENDTIME, m_EndTime);
	DDX_Text(pDX, IDC_TOTALSIZE, m_TotalSize);
	DDX_Text(pDX, IDC_TRANSRATE, m_TransRate);
	DDX_Text(pDX, IDC_FILENAME, m_FileName);
	DDX_Text(pDX, IDC_MESSAGE, m_Message);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProgDlg, CDialog)
	//{{AFX_MSG_MAP(CProgDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProgDlg ���b�Z�[�W �n���h��

void CProgDlg::OnCancel() 
{
	m_AbortFlag = TRUE;
	if ( m_pSock != NULL )
		m_pSock->SyncAbort();
//	CDialog::OnCancel();
}

BOOL CProgDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_FileSize.SetRange(0, 0);
	m_FileSize.SetPos(0);
	m_EndTime = _T("");
	m_TotalSize = _T("");
	m_TransRate = _T("");
	m_Message   = _T("");
	m_AbortFlag = FALSE;
	UpdateData(FALSE);

	m_LastSize = 0;
	m_ResumeSize = 0;
	m_StartClock = clock();

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

void CProgDlg::SetRange(LONGLONG max, LONGLONG rem)
{
	for ( m_Div = 1 ; (max / m_Div) >= 0x8000 ; m_Div *= 2 )
		;
	
	m_FileSize.SetRange(0, (int)(max / m_Div));
	m_FileSize.SetPos(0);

	UpdateData(TRUE);
	m_LastSize = max;
	m_ResumeSize = rem;
	m_StartClock = clock();
	UpdateData(FALSE);
}

void CProgDlg::SetPos(LONGLONG pos)
{
	int n;
	double d;

	UpdateData(TRUE);
	m_FileSize.SetPos((int)(pos / m_Div));

	if ( pos > 1000000 )
		m_TotalSize.Format("%d.%03dM", (int)(pos / 1000000), (int)((pos / 1000) % 1000));
	else if ( pos > 1000 )
		m_TotalSize.Format("%dK", (int)(pos / 1000));
	else
		m_TotalSize.Format("%d", (int)pos);

	if ( clock() > m_StartClock ) {
		d = (double)(pos - m_ResumeSize) * CLOCKS_PER_SEC / (double)(clock() - m_StartClock);

		if ( d > 0.0 && m_LastSize > 0 ) {
			n = (int)((double)(m_LastSize - pos) / d);
			if ( n >= 3600 )
				m_EndTime.Format("%d:%02d:%02d", n / 3600, (n % 3600) / 60, n % 60);
			else if ( n >= 60 )
				m_EndTime.Format("%d:%02d", n / 60, n % 60);
			else
				m_EndTime.Format("%d", n);
		} else
			m_EndTime = "";

		if ( d > 10048576.0 )
			m_TransRate.Format("%dM", (int)(d / 1048576.0));
		else if ( d > 10024.0 )
			m_TransRate.Format("%dK", (int)(d / 1024.0));
		else
			m_TransRate.Format("%d", (int)(d));
	}

	UpdateData(FALSE);
}
void CProgDlg::SetFileName(LPCSTR file)
{
	UpdateData(TRUE);
	m_FileName = file;
	UpdateData(FALSE);
}
void CProgDlg::SetMessage(LPCSTR msg)
{
	UpdateData(TRUE);
	m_Message = msg;
	UpdateData(FALSE);
}