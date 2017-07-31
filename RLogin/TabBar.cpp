// TabBar.cpp: CTabBar �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "TabBar.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// �\�z/����
//////////////////////////////////////////////////////////////////////

CTabBar::CTabBar()
{
}

CTabBar::~CTabBar()
{
}

IMPLEMENT_DYNAMIC(CTabBar, CControlBar)

BEGIN_MESSAGE_MAP(CTabBar, CControlBar)
	//{{AFX_MSG_MAP(CTabBar)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
	ON_NOTIFY(TCN_SELCHANGE, IDC_MDI_TAB_CTRL, OnSelchange)
END_MESSAGE_MAP()

BOOL CTabBar::Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID)
{
	m_dwStyle = (dwStyle & CBRS_ALL);

	dwStyle &= ~CBRS_ALL;
	dwStyle |= CCS_NOPARENTALIGN|CCS_NOMOVEY|CCS_NODIVIDER|CCS_NORESIZE;
	if (pParentWnd->GetStyle() & WS_THICKFRAME) dwStyle |= SBARS_SIZEGRIP;

	CRect rect; rect.SetRectEmpty();
	return CWnd::Create(STATUSCLASSNAME, NULL, dwStyle, rect, pParentWnd, nID);
}

//////////////////////////////////////////////////////////////////////
// CTabBar ���b�Z�[�W �n���h��

int CTabBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if ( CControlBar::OnCreate(lpCreateStruct) == (-1) )
		return (-1);
	
	CRect rect; rect.SetRectEmpty();
	if ( !m_TabCtrl.Create(WS_VISIBLE | WS_CHILD | TCS_FOCUSNEVER | TCS_FIXEDWIDTH | TCS_FORCELABELLEFT, rect, this, IDC_MDI_TAB_CTRL) ) {
		TRACE0("Unable to create tab control bar\n");
		return (-1);
	}
 
	CFont *font = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));
	m_TabCtrl.SetFont(font);

	return 0;
}

void CTabBar::OnSize(UINT nType, int cx, int cy) 
{
	CControlBar::OnSize(nType, cx, cy);
	if ( m_TabCtrl.m_hWnd == NULL )
		return;
	CRect rect;
	GetWindowRect(rect);
	m_TabCtrl.AdjustRect(TRUE, &rect);
	m_TabCtrl.SetWindowPos(&wndTop ,0, 0, rect.Width(), rect.Height(), SWP_SHOWWINDOW);
}

void CTabBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	int n;
	TC_ITEM tci;
	CString title;
	char tmp[MAX_PATH + 2];
	CWnd *pWnd;
	CMDIFrameWnd *pMainframe = ((CMDIFrameWnd *)AfxGetMainWnd());
	CMDIChildWnd* pActive = (pMainframe == NULL ? NULL : pMainframe->MDIGetActive(NULL));

	for ( n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
		tci.mask = TCIF_PARAM | TCIF_TEXT;
		tci.pszText = tmp;
		tci.cchTextMax = MAX_PATH;
		if ( !m_TabCtrl.GetItem(n, &tci) )
			continue;
		pWnd = FromHandle((HWND)tci.lParam);
		pWnd->GetWindowText(title);
		if ( title.Compare(tmp) != 0 ) {
			tci.mask = TCIF_TEXT;
			tci.pszText = title.LockBuffer();
			m_TabCtrl.SetItem(n, &tci);
			title.UnlockBuffer();
		}
		if ( pActive != NULL && pActive->m_hWnd == pWnd->m_hWnd )
			m_TabCtrl.SetCurSel(n);
	}
	ReSize();
}

void CTabBar::Add(CWnd *pWnd)
{
	TC_ITEM tci;
	CString title;

	pWnd->GetWindowText(title);
	tci.mask = TCIF_PARAM | TCIF_TEXT;
	tci.pszText = title.LockBuffer();
	tci.lParam = (LPARAM)(pWnd->m_hWnd);
	m_TabCtrl.InsertItem(m_TabCtrl.GetItemCount(), &tci);
	title.UnlockBuffer();
	ReSize();
}

void CTabBar::Remove(CWnd *pWnd)
{
	int n;
	TC_ITEM tci;

	for ( n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
		tci.mask = TCIF_PARAM;
		if ( !m_TabCtrl.GetItem(n, &tci) )
			continue;
		if ( (HWND)(tci.lParam) == pWnd->m_hWnd ) {
			m_TabCtrl.DeleteItem(n);
			break;
		}
	}
	ReSize();
}

CWnd *CTabBar::GetAt(int nIndex)
{
	TC_ITEM tci;
	tci.mask = TCIF_PARAM;
	if ( !m_TabCtrl.GetItem(nIndex, &tci) )
		return NULL;
	return FromHandle((HWND)tci.lParam);
}

void CTabBar::ReSize()
{
	int width;
	CRect rect;
	CSize sz;
	int n = m_TabCtrl.GetItemCount();
	CWnd *pWnd = GetParent();

	if ( pWnd == NULL || n <= 0 )
		return;

	if ( n < 4 ) n = 4;
	pWnd->GetClientRect(rect);
	width = (rect.Width() - 2) / n;

	m_TabCtrl.GetItemRect(0, rect);
	if ( width == rect.Width() )
		return;

	sz.cx = width;
	sz.cy = rect.Height();
	sz = m_TabCtrl.SetItemSize(sz);
}

void CTabBar::OnSelchange(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TC_ITEM tci;
	int idx = m_TabCtrl.GetCurSel();

	tci.mask = TCIF_PARAM;
	if ( m_TabCtrl.GetItem(idx, &tci) ) {
		CWnd *pFrame = FromHandle((HWND) tci.lParam);
		ASSERT(pFrame != NULL);
		ASSERT_KINDOF(CMDIChildWnd, pFrame);
		((CMDIFrameWnd *)AfxGetMainWnd())->MDIActivate(pFrame);
	}

	*pResult = 0;
}
void CTabBar::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CRect rect;
	TC_ITEM tci;
	int idx = m_TabCtrl.GetCurSel();

	CControlBar::OnLButtonDown(nFlags, point);

	if ( !m_TabCtrl.GetItemRect(idx, rect) || !rect.PtInRect(point) )
		return;

	CRectTracker tracker(rect, CRectTracker::hatchedBorder);

	if ( !tracker.Track(this, point, FALSE, CWnd::GetDesktopWindow()) )
		return;

	m_TabCtrl.Invalidate(FALSE);

	point.x = point.x - rect.left + tracker.m_rect.left;
	point.y = point.y - rect.top + tracker.m_rect.top;

	for ( int n = 0 ; n < m_TabCtrl.GetItemCount() ; n++ ) {
		if ( n == idx || !m_TabCtrl.GetItemRect(n, rect) || !rect.PtInRect(point) )
			continue;

		char Text[MAX_PATH + 2];

		tci.mask = TCIF_PARAM | TCIF_TEXT;
		tci.pszText = Text;
		tci.cchTextMax = MAX_PATH;
		if ( !m_TabCtrl.GetItem(idx, &tci) )
			return;

		m_TabCtrl.DeleteItem(idx);
		tci.mask = TCIF_PARAM | TCIF_TEXT;
		m_TabCtrl.InsertItem(n, &tci);
		return;
	}

	tci.mask = TCIF_PARAM;
	if ( !m_TabCtrl.GetItem(idx, &tci) )
		return;

	CWnd *pWnd = FromHandle((HWND)tci.lParam);
	CMainFrame *pMain = (CMainFrame *)AfxGetMainWnd();

	ClientToScreen(&point);
	pMain->GetWindowRect(rect);
	if ( rect.PtInRect(point) ) {
		pMain->ScreenToClient(&point);
		pMain->MoveChild(pWnd, point);
		return;
	}

	pWnd->PostMessage(WM_CLOSE);
}

CSize CTabBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
	ASSERT_VALID(this);
	ASSERT(::IsWindow(m_hWnd));

	// determinme size of font being used by the status bar
	TEXTMETRIC tm;
	{
		CClientDC dc(NULL);
		HFONT hFont = (HFONT)SendMessage(WM_GETFONT);
		HGDIOBJ hOldFont = NULL;
		if (hFont != NULL)
			hOldFont = dc.SelectObject(hFont);
		VERIFY(dc.GetTextMetrics(&tm));
		if (hOldFont != NULL)
			dc.SelectObject(hOldFont);
	}

	// get border information
	CRect rect; rect.SetRectEmpty();
	CalcInsideRect(rect, bHorz);
	int rgBorders[3];
	DefWindowProc(SB_GETBORDERS, 0, (LPARAM)&rgBorders);

	// determine size, including borders
	CSize size;
	size.cx = 32767;
	size.cy = tm.tmHeight - tm.tmInternalLeading - 1
		+ rgBorders[1] * 2 + ::GetSystemMetrics(SM_CYBORDER) * (2 + 3)
		- rect.Height();

	return size;
}

BOOL CTabBar::PreTranslateMessage(MSG* pMsg) 
{
	if ( pMsg->hwnd == 	m_TabCtrl.m_hWnd && pMsg->message == WM_LBUTTONDOWN ) {
		if ( !CControlBar::PreTranslateMessage(pMsg) )
			return 0;
		CPoint point(LOWORD(pMsg->lParam), HIWORD(pMsg->lParam));
		::ClientToScreen(pMsg->hwnd, &point);
		ScreenToClient(&point);
		pMsg->hwnd = m_hWnd;
		pMsg->lParam = MAKELPARAM(point.x, point.y);
	}
	return CControlBar::PreTranslateMessage(pMsg);
}