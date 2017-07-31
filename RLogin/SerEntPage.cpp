// SerEntPage.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "rlogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TextRam.h"
#include "Data.h"
#include "ComSock.h"
#include "OptDlg.h"
#include "SerEntPage.h"
#include "ChatDlg.h"
#include "ProxyDlg.h"
#include "EnvDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSerEntPage プロパティ ページ

IMPLEMENT_DYNCREATE(CSerEntPage, CPropertyPage)

CSerEntPage::CSerEntPage() : CTreePropertyPage(CSerEntPage::IDD)
{
	m_EntryName = _T("");
	m_HostName = _T("");
	m_PortName = _T("");
	m_UserName = _T("");
	m_PassName = _T("");
	m_TermName = _T("");
	m_KanjiCode = 0;
	m_ProtoType = 0;
	m_DefComPort = _T("");
	m_IdkeyName = _T("");
	m_Memo = _T("");
	m_Group = _T("");
	m_BeforeEntry = _T("");
}
CSerEntPage::~CSerEntPage()
{
}

void CSerEntPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_ENTRYNAME, m_EntryName);
	DDX_CBString(pDX, IDC_SERVERNAME, m_HostName);
	DDX_CBString(pDX, IDC_SOCKNO, m_PortName);
	DDX_CBString(pDX, IDC_LOGINNAME, m_UserName);
	DDX_Text(pDX, IDC_PASSWORD, m_PassName);
	DDX_CBString(pDX, IDC_TERMNAME, m_TermName);
	DDX_Radio(pDX, IDC_KANJICODE1, m_KanjiCode);
	DDX_Radio(pDX, IDC_PROTO1, m_ProtoType);
	DDX_Text(pDX, IDC_ENTRYMEMO, m_Memo);
	DDX_CBString(pDX, IDC_GROUP, m_Group);
	DDX_CBString(pDX, IDC_BEFORE, m_BeforeEntry);
}

BEGIN_MESSAGE_MAP(CSerEntPage, CPropertyPage)
	ON_BN_CLICKED(IDC_COMCONFIG, OnComconfig)
	ON_BN_CLICKED(IDC_KEYFILESELECT, OnKeyfileselect)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_PROTO1, IDC_PROTO6, OnProtoType)
	ON_EN_CHANGE(IDC_ENTRYNAME, OnUpdateEdit)
	ON_CBN_EDITCHANGE(IDC_SERVERNAME, OnUpdateEdit)
	ON_CBN_EDITCHANGE(IDC_SOCKNO, OnUpdateEdit)
	ON_CBN_EDITCHANGE(IDC_LOGINNAME, OnUpdateEdit)
	ON_EN_CHANGE(IDC_PASSWORD, OnUpdateEdit)
	ON_CBN_EDITCHANGE(IDC_TERMNAME, OnUpdateEdit)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_KANJICODE1, IDC_KANJICODE4, OnUpdateCheck)
	ON_BN_CLICKED(IDC_CHATEDIT, &CSerEntPage::OnChatEdit)
	ON_BN_CLICKED(IDC_PROXYSET, &CSerEntPage::OnProxySet)
	ON_BN_CLICKED(IDC_TERMCAP, &CSerEntPage::OnBnClickedTermcap)
	ON_EN_CHANGE(IDC_ENTRYMEMO, OnUpdateEdit)
	ON_CBN_EDITCHANGE(IDC_GROUP, OnUpdateEdit)
	ON_CBN_EDITCHANGE(IDC_BEFORE, OnUpdateEdit)
END_MESSAGE_MAP()

void CSerEntPage::SetEnableWind()
{
	int n;
	CWnd *pWnd;
	static LPCTSTR	DefPortName[] = { _T(""), _T("login"), _T("telnet"), _T("ssh"), _T("COM1"), _T("") };
	static const struct {
		int		nId;
		BOOL	mode[6];
	} ItemTab[] = {			/*	drect	login	telnet	ssh		com		pipe */
		{ IDC_COMCONFIG,	  {	FALSE,	FALSE,	FALSE,	FALSE,	TRUE,	FALSE } },
		{ IDC_KEYFILESELECT,  {	FALSE,	FALSE,	FALSE,	TRUE,	FALSE,	FALSE } },
		{ IDC_SOCKNO,		  {	TRUE,	TRUE,	TRUE,	TRUE,	TRUE,	FALSE } },
		{ IDC_LOGINNAME,	  {	FALSE,	TRUE,	TRUE,	TRUE,	TRUE,	FALSE } },
		{ IDC_PASSWORD,		  {	FALSE,	TRUE,	TRUE,	TRUE,	TRUE,	FALSE } },
		{ IDC_TERMNAME,		  {	FALSE,	TRUE,	TRUE,	TRUE,	TRUE,	FALSE } },
		{ IDC_PROXYSET,		  {	TRUE,	TRUE,	TRUE,	TRUE,	FALSE,	FALSE } },
		{ IDC_TERMCAP,		  {	FALSE,	FALSE,	TRUE,	TRUE,	FALSE,	FALSE } },
		{ 0 }
	};

	for ( n = 0 ; ItemTab[n].nId != 0 ; n++ ) {
		if ( (pWnd = GetDlgItem(ItemTab[n].nId)) != NULL )
			pWnd->EnableWindow(ItemTab[n].mode[m_ProtoType]);
	}

	switch(m_ProtoType) {
	case PROTO_COMPORT:
		if ( m_PortName.Left(3).Compare(_T("COM")) != 0 )
			m_PortName = m_DefComPort;
		break;

	case PROTO_SSH:
	case PROTO_LOGIN:
	case PROTO_TELNET:
		if ( _tstoi(m_PortName) <= 0 )
			m_PortName = DefPortName[m_ProtoType];
		break;

	case PROTO_DIRECT:
	case PROTO_PIPE:
		break;
	}

	UpdateData(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CSerEntPage メッセージ ハンドラ

void CSerEntPage::DoInit()
{
	CComSock com(NULL);

	m_EntryName   = m_pSheet->m_pEntry->m_EntryName;
	m_HostName    = m_pSheet->m_pEntry->m_HostNameProvs;
	m_PortName    = m_pSheet->m_pEntry->m_PortName;
	m_UserName    = m_pSheet->m_pEntry->m_UserNameProvs;
	m_PassName    = m_pSheet->m_pEntry->m_PassNameProvs;
	m_TermName    = m_pSheet->m_pEntry->m_TermName;
	m_IdkeyName   = m_pSheet->m_pEntry->m_IdkeyName;
	m_KanjiCode   = m_pSheet->m_pEntry->m_KanjiCode;
	m_ProtoType   = m_pSheet->m_pEntry->m_ProtoType;
	m_ProxyMode   = m_pSheet->m_pEntry->m_ProxyMode;
	m_ProxyHost   = m_pSheet->m_pEntry->m_ProxyHostProvs;
	m_ProxyPort   = m_pSheet->m_pEntry->m_ProxyPort;
	m_ProxyUser   = m_pSheet->m_pEntry->m_ProxyUserProvs;
	m_ProxyPass   = m_pSheet->m_pEntry->m_ProxyPassProvs;
	m_Memo        = m_pSheet->m_pEntry->m_Memo;
	m_Group       = m_pSheet->m_pEntry->m_Group;
	m_BeforeEntry = m_pSheet->m_pEntry->m_BeforeEntry;

	m_ExtEnvStr   = m_pSheet->m_pParamTab->m_ExtEnvStr;

	if ( m_PortName.Compare(_T("serial")) == 0 ) {
		com.GetMode(m_HostName);
		m_PortName = com.m_ComName;
	}

	UpdateData(FALSE);
}

BOOL CSerEntPage::OnInitDialog() 
{
	int n, i;
	CString str;
	CComboBox *pCombo;
	DWORD pb = CComSock::AliveComPort();

	ASSERT(m_pSheet != NULL && m_pSheet->m_pEntry != NULL);

	CPropertyPage::OnInitDialog();

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_SOCKNO)) != NULL ) {
		for ( n = 1 ; n <= 31 ; n++ ) {
			str.Format(_T("COM%d"), n);
			if ( (pb & (1 << n)) != 0 ) {
				if ( (i = pCombo->FindStringExact((-1), str)) == CB_ERR )
					pCombo->AddString(str);
				if ( m_DefComPort.IsEmpty() )
					m_DefComPort = str;
			} else if ( (i = pCombo->FindStringExact((-1), str)) != CB_ERR ) {
				pCombo->DeleteString(i);
			}
		}
	}

	CServerEntryTab *pTab = &(((CMainFrame *)AfxGetMainWnd())->m_ServerEntryTab);

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_SERVERNAME)) != NULL ) {
		for ( n = 0 ; n < pTab->m_Data.GetSize() ; n++ ) {
			str = pTab->m_Data[n].m_HostName;
			if ( !str.IsEmpty() && pCombo->FindStringExact((-1), str) == CB_ERR )
				pCombo->AddString(str);
		}
	}
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_LOGINNAME)) != NULL ) {
		for ( n = 0 ; n < pTab->m_Data.GetSize() ; n++ ) {
			str = pTab->m_Data[n].m_UserName;
			if ( !str.IsEmpty() && pCombo->FindStringExact((-1), str) == CB_ERR )
				pCombo->AddString(str);
		}
	}
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_TERMNAME)) != NULL ) {
		for ( n = 0 ; n < pTab->m_Data.GetSize() ; n++ ) {
			str = pTab->m_Data[n].m_TermName;
			if ( !str.IsEmpty() && pCombo->FindStringExact((-1), str) == CB_ERR )
				pCombo->AddString(str);
		}
	}
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_SOCKNO)) != NULL ) {
		for ( n = 0 ; n < pTab->m_Data.GetSize() ; n++ ) {
			str = pTab->m_Data[n].m_PortName;
			if ( !str.IsEmpty() && pCombo->FindStringExact((-1), str) == CB_ERR )
				pCombo->AddString(str);
		}
	}
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_GROUP)) != NULL ) {
		for ( n = 0 ; n < pTab->m_Data.GetSize() ; n++ ) {
			str = pTab->m_Data[n].m_Group;
			if ( !str.IsEmpty() && pCombo->FindStringExact((-1), str) == CB_ERR )
				pCombo->AddString(str);
		}
	}
	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_BEFORE)) != NULL ) {
		for ( n = 0 ; n < pTab->m_Data.GetSize() ; n++ ) {
			str = pTab->m_Data[n].m_EntryName;
			if ( !str.IsEmpty() && pCombo->FindStringExact((-1), str) == CB_ERR )
				pCombo->AddString(str);
		}
	}

	DoInit();

	SetEnableWind();
	
	return TRUE;
}
BOOL CSerEntPage::OnApply() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pEntry != NULL);

	UpdateData(TRUE);

	m_pSheet->m_pEntry->m_EntryName = m_EntryName;
	m_pSheet->m_pEntry->m_HostName  = m_HostName;
	m_pSheet->m_pEntry->m_PortName  = m_PortName;
	m_pSheet->m_pEntry->m_UserName  = m_UserName;
	m_pSheet->m_pEntry->m_PassName  = m_PassName;
	m_pSheet->m_pEntry->m_TermName  = m_TermName;
	m_pSheet->m_pEntry->m_IdkeyName = m_IdkeyName;
	m_pSheet->m_pEntry->m_KanjiCode = m_KanjiCode;
	m_pSheet->m_pEntry->m_ProtoType = m_ProtoType;
	m_pSheet->m_pEntry->m_ProxyMode = m_ProxyMode;
	m_pSheet->m_pEntry->m_ProxyHost = m_ProxyHost;
	m_pSheet->m_pEntry->m_ProxyPort = m_ProxyPort;
	m_pSheet->m_pEntry->m_ProxyUser = m_ProxyUser;
	m_pSheet->m_pEntry->m_ProxyPass = m_ProxyPass;
	m_pSheet->m_pEntry->m_Memo      = m_Memo;
	m_pSheet->m_pEntry->m_Group     = m_Group;
	m_pSheet->m_pEntry->m_HostNameProvs  = m_HostName;
	m_pSheet->m_pEntry->m_UserNameProvs  = m_UserName;
	m_pSheet->m_pEntry->m_PassNameProvs  = m_PassName;
	m_pSheet->m_pEntry->m_ProxyHostProvs = m_ProxyHost;
	m_pSheet->m_pEntry->m_ProxyUserProvs = m_ProxyUser;
	m_pSheet->m_pEntry->m_ProxyPassProvs = m_ProxyPass;
	m_pSheet->m_pEntry->m_BeforeEntry    = m_BeforeEntry;
	m_pSheet->m_pParamTab->m_ExtEnvStr = m_ExtEnvStr;

	return TRUE;
}

void CSerEntPage::OnReset() 
{
	ASSERT(m_pSheet != NULL && m_pSheet->m_pEntry != NULL);

	DoInit();
	SetModified(FALSE);
}

void CSerEntPage::OnComconfig() 
{
	CComSock com(NULL);

	UpdateData(TRUE);
	if ( m_PortName.Left(3).CompareNoCase(_T("COM")) == 0 ) {
		com.m_ComPort = (-1);
		com.GetMode(m_HostName);
		if ( com.m_ComPort != _tstoi(m_PortName.Mid(3)) )
			m_HostName = m_PortName;
	}
	com.ConfigDlg(this, m_HostName);

	UpdateData(FALSE);
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_ENTRY;
}
void CSerEntPage::OnKeyfileselect() 
{
	UpdateData(TRUE);

	CFileDialog dlg(TRUE, _T(""), m_IdkeyName, OFN_HIDEREADONLY, CStringLoad(IDS_FILEDLGALLFILE), this);

	if ( dlg.DoModal() != IDOK ) {
		if ( m_IdkeyName.IsEmpty() || MessageBox(CStringLoad(IDS_IDKEYFILEDELREQ), _T("Question"), MB_ICONQUESTION | MB_YESNO) != IDYES )
			return;
		m_IdkeyName.Empty();
	} else
		m_IdkeyName = dlg.GetPathName();

	UpdateData(FALSE);
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_ENTRY;
}
void CSerEntPage::OnProtoType(UINT nID) 
{
	UpdateData(TRUE);
	SetEnableWind();

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_ENTRY;
}
void CSerEntPage::OnUpdateCheck(UINT nID) 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_ENTRY;
}
void CSerEntPage::OnUpdateEdit() 
{
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_ENTRY;
}
void CSerEntPage::OnChatEdit()
{
	CChatDlg dlg;

	dlg.m_Script = m_pSheet->m_pEntry->m_ChatScript;

	if ( dlg.DoModal() != IDOK )
		return;

	m_pSheet->m_pEntry->m_ChatScript = dlg.m_Script;

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_ENTRY;
}

void CSerEntPage::OnProxySet()
{
	CProxyDlg dlg;

	dlg.m_ProxyMode  = m_ProxyMode & 7;
	dlg.m_SSLMode    = m_ProxyMode >> 3;
	dlg.m_ServerName = m_ProxyHost;
	dlg.m_PortName   = m_ProxyPort;
	dlg.m_UserName   = m_ProxyUser;
	dlg.m_PassWord   = m_ProxyPass;

	if ( dlg.DoModal() != IDOK )
		return;

	m_ProxyMode = dlg.m_ProxyMode | (dlg.m_SSLMode << 3);
	m_ProxyHost = dlg.m_ServerName;
	m_ProxyPort = dlg.m_PortName;
	m_ProxyUser = dlg.m_UserName;
	m_ProxyPass = dlg.m_PassWord;

	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_ENTRY;
}

void CSerEntPage::OnBnClickedTermcap()
{
	int n;
	CEnvDlg dlg;
	CComboBox *pCombo;

	UpdateData(TRUE);

	dlg.m_Env.SetNoSort(TRUE);
	dlg.m_Env.GetString(m_ExtEnvStr);

	if ( !m_TermName.IsEmpty() ) {
		dlg.m_Env[_T("TERM")].m_Value  = 0;
		dlg.m_Env[_T("TERM")].m_String = m_TermName;
	}

	if ( dlg.DoModal() != IDOK )
		return;

	if ( (n = dlg.m_Env.Find(_T("TERM"))) >= 0 ) {
		dlg.m_Env[_T("TERM")].m_Value = 0;
		if ( !dlg.m_Env[_T("TERM")].m_String.IsEmpty() )
			m_TermName = dlg.m_Env[_T("TERM")];
	}

	dlg.m_Env.SetString(m_ExtEnvStr);

	if ( (pCombo = (CComboBox *)GetDlgItem(IDC_TERMNAME)) != NULL ) {
		if ( pCombo->FindStringExact(0, m_TermName) < 0 )
			pCombo->AddString(m_TermName);
	}

	UpdateData(FALSE);
	SetModified(TRUE);
	m_pSheet->m_ModFlag |= UMOD_PARAMTAB;
}
