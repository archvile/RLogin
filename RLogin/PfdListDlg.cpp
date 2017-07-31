// PfdListDlg.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "RLogin.h"
#include "Data.h"
#include "PfdListDlg.h"
#include "PfdParaDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPfdListDlg �_�C�A���O

CPfdListDlg::CPfdListDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPfdListDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPfdListDlg)
		// ���� - ClassWizard �͂��̈ʒu�Ƀ}�b�s���O�p�̃}�N����ǉ��܂��͍폜���܂��B
	//}}AFX_DATA_INIT
	m_pData = NULL;
	m_pEntry = NULL;
	m_ModifiedFlag = FALSE;
	m_X11PortFlag = FALSE;
	m_XDisplay = _T("");
}

void CPfdListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPfdListDlg)
	DDX_Control(pDX, IDC_PFDLIST, m_List);
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_EDIT1, m_XDisplay);
	DDX_Check(pDX, IDC_CHECK1, m_X11PortFlag);
}

BEGIN_MESSAGE_MAP(CPfdListDlg, CDialog)
	//{{AFX_MSG_MAP(CPfdListDlg)
	ON_BN_CLICKED(IDC_PFDNEW, OnPfdNew)
	ON_BN_CLICKED(IDC_PFDEDIT, OnPfdEdit)
	ON_BN_CLICKED(IDC_PFDDEL, OnPfdDel)
	ON_NOTIFY(NM_DBLCLK, IDC_PFDLIST, OnDblclkPfdlist)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_EDIT_NEW, OnPfdNew)
	ON_COMMAND(ID_EDIT_UPDATE, OnPfdEdit)
	ON_COMMAND(ID_EDIT_DELETE, OnPfdDel)
	ON_COMMAND(ID_EDIT_DUPS, OnEditDups)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UPDATE, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditEntry)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DUPS, OnUpdateEditEntry)
	ON_EN_CHANGE(IDC_EDIT1, &CPfdListDlg::OnUpdateItem)
	ON_BN_CLICKED(IDC_CHECK1, &CPfdListDlg::OnUpdateItem)
END_MESSAGE_MAP()

void CPfdListDlg::InitList()
{
	int n;
	CStringArrayExt array;

	m_List.DeleteAllItems();

	for ( n = 0 ; n < m_pData->m_PortFwd.GetSize() ; n++ ) {
		array.GetString(m_pData->m_PortFwd[n]);
		if ( array.GetSize() < 4 ) {
			m_pData->m_PortFwd.RemoveAt(n);
			n--;
			continue;
		}
		m_List.InsertItem(LVIF_TEXT | LVIF_PARAM, n, array[0], 0, 0, 0, n);
		m_List.SetItemText(n, 1, array[1]);
		m_List.SetItemText(n, 2, array[2]);
		m_List.SetItemText(n, 3, array[3]);
	}
	m_List.DoSortItem();
}

/////////////////////////////////////////////////////////////////////////////
// CPfdListDlg ���b�Z�[�W �n���h��

static const LV_COLUMN InitListTab[4] = {
		{ LVCF_TEXT | LVCF_WIDTH, 0, 120, "Listened Host",  0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  60, "Port", 0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0, 120, "Connect Host",   0, 0 }, 
		{ LVCF_TEXT | LVCF_WIDTH, 0,  60, "Port",   0, 0 }, 
	};

BOOL CPfdListDlg::OnInitDialog() 
{
	ASSERT(m_pData);
	CDialog::OnInitDialog();

	m_List.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
	m_List.InitColumn("PfdList", InitListTab, 4);
	m_List.SetPopUpMenu(IDR_POPUPMENU, 1);
	InitList();

	return TRUE;
}

void CPfdListDlg::OnOK() 
{
	ASSERT(m_pData);
	m_List.SaveColumn("PfdList");
	CDialog::OnOK();
}

void CPfdListDlg::OnPfdNew() 
{
	CPfdParaDlg dlg;
	dlg.m_pData = m_pData;
	dlg.m_pEntry = m_pEntry;
	if ( dlg.DoModal() == IDOK )
		m_ModifiedFlag = TRUE;
	InitList();
}

void CPfdListDlg::OnPfdEdit() 
{
	CPfdParaDlg dlg;
	dlg.m_pData = m_pData;
	dlg.m_pEntry = m_pEntry;
	if ( (dlg.m_EntryNum = m_List.GetSelectMarkData()) < 0 )
		return;
	if ( dlg.DoModal() == IDOK )
		m_ModifiedFlag = TRUE;
	InitList();
}

void CPfdListDlg::OnPfdDel() 
{
	int n;
	if ( (n = m_List.GetSelectMarkData()) < 0 )
		return;
	m_pData->m_PortFwd.RemoveAt(n);
	m_ModifiedFlag = TRUE;
	InitList();
}

void CPfdListDlg::OnDblclkPfdlist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnPfdEdit();
	*pResult = 0;
}

void CPfdListDlg::OnUpdateEditEntry(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_List.GetSelectMarkData() >= 0);
}

void CPfdListDlg::OnEditDups() 
{
	int n;
	CStringArrayExt array;
	CPfdParaDlg dlg;
	if ( (n = m_List.GetSelectMarkData()) < 0 )
		return;
	dlg.m_pData = m_pData;
	dlg.m_pEntry = m_pEntry;
	array.GetString(m_pData->m_PortFwd[n]);
	if ( array.GetSize() >= 4 ) {
		dlg.m_ListenHost  = array[0];
		dlg.m_ListenPort  = array[1];
		dlg.m_ConnectHost = array[2];
		dlg.m_ConnectPort = array[3];
	}
	if ( dlg.DoModal() == IDOK )
		m_ModifiedFlag = TRUE;
	InitList();
}
void CPfdListDlg::OnUpdateItem()
{
	m_ModifiedFlag = TRUE;
}