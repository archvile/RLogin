// ExtSocket.cpp: CExtSocket �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RLogin.h"
#include "MainFrm.h"
#include "RLoginDoc.h"
#include "RLoginView.h"
#include "ExtSocket.h"
#include "ssh.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// CSockBuffer

CSockBuffer::CSockBuffer()
{
	m_Type = 0;
	m_Ofs = m_Len = 0;
	m_Max = RECVBUFSIZ;
	m_Data = new BYTE[m_Max];
	m_Left = m_Right = NULL;
}
CSockBuffer::~CSockBuffer()
{
	delete m_Data;
}
void CSockBuffer::Alloc(int len)
{
	if ( (len += m_Len) <= m_Max )
		return;
	else if ( (len -= m_Ofs) <= m_Max ) {
		if ( (m_Len -= m_Ofs) > 0 )
			memcpy(m_Data, m_Data + m_Ofs, m_Len);
		m_Ofs = 0;
		return;
	}

	m_Max = (len * 2 + RECVMINSIZ) & ~(RECVMINSIZ - 1);
	LPBYTE tmp = new BYTE[m_Max];

	if ( (m_Len -= m_Ofs) > 0 )
		memcpy(tmp, m_Data + m_Ofs, m_Len);
	m_Ofs = 0;

	delete m_Data;
	m_Data = tmp;
}

//////////////////////////////////////////////////////////////////////
// CExtSocket

static const int EventMask[2] = { FD_READ, FD_OOB };

CExtSocket::CExtSocket(class CRLoginDoc *pDoc)
{
	m_pDocument = pDoc;
	m_Type = 0;
	m_Fd = m_Fdv6 = (-1);
	m_SocketEvent = FD_READ | FD_OOB | FD_CONNECT | FD_CLOSE;
	m_RecvOverSize = RECVBUFSIZ;
	m_RecvSyncMode = 0;
	m_OnRecvFlag = m_OnSendFlag = 0;
	m_DestroyFlag = FALSE;
	m_pRecvEvent = NULL;
	m_FreeHead = m_ProcHead = m_RecvHead = m_SendHead = NULL;
	m_ProcSize[0] = m_ProcSize[1] = 0;
	m_RecvSize[0] = m_RecvSize[1] = 0;
	m_SendSize[0] = m_SendSize[1] = 0;
	m_ProxyStatus = 0;
	m_SSL_mode  = 0;
	m_SSL_pCtx  = NULL;
	m_SSL_pSock = NULL;
}

CExtSocket::~CExtSocket()
{
	if ( m_pRecvEvent != NULL )
		delete m_pRecvEvent;

	while ( m_ProcHead != NULL )
		m_ProcHead = RemoveHead(m_ProcHead);

	while ( m_RecvHead != NULL )
		m_RecvHead = RemoveHead(m_RecvHead);

	while ( m_SendHead != NULL )
		m_SendHead = RemoveHead(m_SendHead);

	CSockBuffer *sp;
	while ( (sp = m_FreeHead) != NULL ) {
		m_FreeHead = sp->m_Left;
		delete sp;
	}
}
void CExtSocket::Destroy()
{
	Close();
	if ( (m_OnRecvFlag & 001) != 0 ) {
		if ( !m_DestroyFlag )
			((CRLoginApp *)AfxGetApp())->SetSocketIdle(this);
		m_DestroyFlag = TRUE;
	} else {
		if ( m_DestroyFlag )
			((CRLoginApp *)AfxGetApp())->DelSocketIdle(this);
		delete this;
	}
}
CSockBuffer *CExtSocket::AddTail(CSockBuffer *sp, CSockBuffer *head)
{
	if ( head == NULL ) {
		sp->m_Left = sp;
		sp->m_Right = sp;
		return sp;
	} else {
		sp->m_Left = head->m_Left;
		sp->m_Right = head;
		head->m_Left = sp;
		sp->m_Left->m_Right = sp;
	}
	return head;
}
CSockBuffer *CExtSocket::AddHead(CSockBuffer *sp, CSockBuffer *head)
{
	if ( head == NULL ) {
		sp->m_Left = sp;
		sp->m_Right = sp;
	} else {
		sp->m_Left = head->m_Left;
		sp->m_Right = head;
		head->m_Left = sp;
		sp->m_Left->m_Right = sp;
	}
	return sp;
}
CSockBuffer *CExtSocket::RemoveHead(CSockBuffer *head)
{
	CSockBuffer *sp;
	if ( (sp = head) == NULL )
		return NULL;
	else if ( sp == sp->m_Right )
		head = NULL;
	else {
		sp->m_Right->m_Left = sp->m_Left;
		sp->m_Left->m_Right = sp->m_Right;
		head = sp->m_Right;
	}
	FreeBuffer(sp);
	return head;
}
CSockBuffer *CExtSocket::AllocBuffer()
{
	CSockBuffer *sp;

	m_FreeSema.Lock();
	if ( m_FreeHead == NULL )
		m_FreeHead = new CSockBuffer;
	sp = m_FreeHead;
	m_FreeHead = sp->m_Left;
	m_FreeSema.Unlock();

	sp->Clear();
	return sp;
}
void CExtSocket::FreeBuffer(CSockBuffer *sp)
{
	m_FreeSema.Lock();
	sp->m_Left = m_FreeHead;
	m_FreeHead = sp;
	m_FreeSema.Unlock();
}

//////////////////////////////////////////////////////////////////////

BOOL CExtSocket::AsyncGetHostByName(LPCSTR pHostName)
{
	m_AsyncMode = 0;
	return GetMainWnd()->SetAsyncHostAddr(pHostName, this);
}
BOOL CExtSocket::AsyncCreate(LPCTSTR lpszHostAddress, UINT nHostPort, LPCSTR lpszRemoteAddress, int nSocketType)
{
	if ( lpszRemoteAddress == NULL )
		return Create(lpszHostAddress, nHostPort, lpszRemoteAddress, nSocketType);

	m_AsyncMode  = 1;
	m_RealHostAddr   = (lpszHostAddress == NULL ? "" : lpszHostAddress);
	m_RealHostPort   = nHostPort;
	m_RealRemoteAddr = lpszRemoteAddress;
	m_RealSocketType = nSocketType;

	return GetMainWnd()->SetAsyncHostAddr(lpszRemoteAddress, this);
}
BOOL CExtSocket::AsyncOpen(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType)
{
	if ( lpszHostAddress == NULL || nHostPort == (-1) )
		return Open(lpszHostAddress, nHostPort, nSocketPort, nSocketType);

	m_AsyncMode  = 2;
	m_RealHostAddr   = lpszHostAddress;
	m_RealHostPort   = nHostPort;
	m_RealRemotePort = nSocketPort;
	m_RealSocketType = nSocketType;

	return GetMainWnd()->SetAsyncHostAddr(lpszHostAddress, this);
}
BOOL CExtSocket::ProxyOpen(int mode, LPCSTR ProxyHost, UINT ProxyPort, LPCSTR ProxyUser, LPCSTR ProxyPass, LPCSTR RealHost, UINT RealPort)
{
	m_AsyncMode      = 2;
	m_RealHostAddr   = ProxyHost;
	m_RealHostPort   = ProxyPort;
	m_RealRemotePort = 0;
	m_RealSocketType = SOCK_STREAM;

	switch(mode & 7) {
	case 0: m_ProxyStatus =  0; break;	// Non
	case 1: m_ProxyStatus =  1; break;	// HTTP
	case 2: m_ProxyStatus = 10; break;	// SOCKS4
	case 3: m_ProxyStatus = 20; break;	// SOCKS5
	}

	switch(mode >> 3) {
	case 0: m_SSL_mode = 0; break;	// Non
	case 1: m_SSL_mode = 1; break;	// SSLv2
	case 2: m_SSL_mode = 2; break;	// SSLv3
	case 3: m_SSL_mode = 3; break;	// TLSv1
	}

	m_ProxyHost = RealHost;
	m_ProxyPort = RealPort;
	m_ProxyUser = ProxyUser;
	m_ProxyPass = ProxyPass;

	if ( m_ProxyStatus == 0 ) {
		m_RealHostAddr   = RealHost;
		m_RealHostPort   = RealPort;
	}

	return GetMainWnd()->SetAsyncHostAddr(m_RealHostAddr, this);
}

//////////////////////////////////////////////////////////////////////

BOOL CExtSocket::Create(LPCTSTR lpszHostAddress, UINT nHostPort, LPCSTR lpszRemoteAddress, int nSocketType)
{
	m_SocketEvent = FD_ACCEPT | FD_CLOSE;

#ifdef	NOIPV6
	int opval;
    struct hostent *hp;
    struct sockaddr_in in;

	if ( (m_Fd = ::socket(AF_INET, nSocketType, 0)) == (-1) )
		return FALSE;

	if ( !GetMainWnd()->SetAsyncSelect(m_Fd, this, m_SocketEvent) )
		goto ERRRET2;

	opval = 1;
	if ( ::setsockopt(m_Fd, SOL_SOCKET, SO_REUSEADDR, (const char *)(&opval), sizeof(opval)) != 0 )
		goto ERRRET1;

    memset(&in, 0, sizeof(in));
    in.sin_family = AF_INET;
    in.sin_addr.s_addr = INADDR_ANY;
    in.sin_port = htons(nHostPort);

	if ( lpszRemoteAddress != NULL && (hp = ::gethostbyname(lpszRemoteAddress)) != NULL )
		in.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;

    if ( bind(m_Fd, (struct sockaddr *)&in, sizeof(in)) < 0 || listen(m_Fd, 128) == SOCKET_ERROR )
		goto ERRRET1;

	return TRUE;

ERRRET1:
	GetMainWnd()->DelAsyncSelect(m_Fd, this);
ERRRET2:
	::closesocket(m_Fd);
	m_Fd = (-1);
	return FALSE;
#else
	int opval;
	struct addrinfo hints, *ai, *aitop;
	char *addr = NULL;
	CString str;
	SOCKET fd;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = nSocketType;
	hints.ai_flags = AI_PASSIVE;
	str.Format("%d", nHostPort);

	if ( getaddrinfo(lpszHostAddress, str, &hints, &aitop) != 0)
		return FALSE;

	m_Fd = m_Fdv6 = (-1);

	for ( ai = aitop ; ai != NULL ; ai = ai->ai_next ) {
		if ( ai->ai_family != AF_INET && ai->ai_family != AF_INET6 )
			continue;

		if ( (fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == (-1) )
			continue;

		if ( !GetMainWnd()->SetAsyncSelect(fd, this, m_SocketEvent) ) {
			::closesocket(fd);
			continue;
		}

		opval = 1;
		if ( ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)(&opval), sizeof(opval)) != 0 ) {
			GetMainWnd()->DelAsyncSelect(fd, this);
			::closesocket(fd);
			continue;
		}

		if ( ::bind(fd, ai->ai_addr, (int)ai->ai_addrlen) < 0 || ::listen(fd, 128) == SOCKET_ERROR ) {
			GetMainWnd()->DelAsyncSelect(fd, this);
			::closesocket(fd);
			continue;
		}

		if ( ai->ai_family == AF_INET && m_Fd == (-1) )
			m_Fd = fd;
		else if ( ai->ai_family == AF_INET6 && m_Fdv6 == (-1) )
			m_Fdv6 = fd;
		else {
			GetMainWnd()->DelAsyncSelect(fd, this);
			::closesocket(fd);
		}
	}
	freeaddrinfo(aitop);
	return (m_Fd == (-1) && m_Fdv6 == (-1) ? FALSE : TRUE);
#endif
}
BOOL CExtSocket::Open(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType)
{
	while ( m_ProcHead != NULL )
		m_ProcHead = RemoveHead(m_ProcHead);

	while ( m_RecvHead != NULL )
		m_RecvHead = RemoveHead(m_RecvHead);

	while ( m_SendHead != NULL )
		m_SendHead = RemoveHead(m_SendHead);

	m_ProcSize[0] = m_ProcSize[1] = 0;
	m_RecvSize[0] = m_RecvSize[1] = 0;
	m_SendSize[0] = m_SendSize[1] = 0;

	m_SocketEvent = FD_READ | FD_OOB | FD_CONNECT | FD_CLOSE;
	m_RecvOverSize = RECVBUFSIZ;

	if ( m_SSL_mode != 0 )
		m_SocketEvent &= ~FD_READ;

#ifdef	NOIPV6
	DWORD val;
    struct hostent *hp;
    struct sockaddr_in in;

	if ( (m_Fd = ::socket(AF_INET, nSocketType, 0)) == (-1) )
		return FALSE;

	if ( !GetMainWnd()->SetAsyncSelect(m_Fd, this, m_SocketEvent) )
		goto ERRRET2;

	if ( nSocketPort != 0 ) {
	    memset(&in, 0, sizeof(in));
	    in.sin_family = AF_INET;
	    in.sin_addr.s_addr = INADDR_ANY;
	    in.sin_port = htons(nSocketPort);
		if ( ::bind(m_Fd, (struct sockaddr *)&in, sizeof(in)) == SOCKET_ERROR )
			goto ERRRET1;
	}

	if ( (hp = ::gethostbyname(lpszHostAddress)) == NULL )
		goto ERRRET1;

	in.sin_family = AF_INET;
	in.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
	in.sin_port = htons(nHostPort);

	val = 1;
	if ( ::ioctlsocket(m_Fd, FIONBIO, &val) != 0 )
		goto ERRRET1;

    if ( ::connect(m_Fd, (struct sockaddr *)&in, sizeof(in)) == SOCKET_ERROR ) {
		if ( WSAGetLastError() != WSAEWOULDBLOCK )
			goto ERRRET1;
	} else
		OnPreConnect();

	return TRUE;

ERRRET1:
	GetMainWnd()->DelAsyncSelect(m_Fd, this);
ERRRET2:
	::closesocket(m_Fd);
	m_Fd = (-1);
	return FALSE;

#else
	DWORD val;
	struct addrinfo hints, *ai, *aitop;
    struct sockaddr_in in;
    struct sockaddr_in6 in6;
	CString str;
	struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = nSocketType;
	str.Format("%d", nHostPort);

	if ( getaddrinfo(lpszHostAddress, str, &hints, &aitop) != 0)
		return FALSE;

	for ( ai = aitop ; ai != NULL ; ai = ai->ai_next ) {
		if ( ai->ai_family != AF_INET && ai->ai_family != AF_INET6 )
			continue;

		if ( (m_Fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == (-1) )
			continue;

		if ( !GetMainWnd()->SetAsyncSelect(m_Fd, this, m_SocketEvent) ) {
			::closesocket(m_Fd);
			m_Fd = (-1);
			continue;
		}

		if ( nSocketPort != 0 ) {
			if ( ai->ai_family == AF_INET ) {
			    memset(&in, 0, sizeof(in));
			    in.sin_family = AF_INET;
			    in.sin_addr.s_addr = INADDR_ANY;
			    in.sin_port = htons(nSocketPort);
				if ( ::bind(m_Fd, (struct sockaddr *)&in, sizeof(in)) == SOCKET_ERROR ) {
					GetMainWnd()->DelAsyncSelect(m_Fd, this);
					::closesocket(m_Fd);
					m_Fd = (-1);
					continue;
				}
			} else {	// AF_INET6
			    memset(&in6, 0, sizeof(in6));
			    in6.sin6_family = AF_INET6;
			    in6.sin6_addr = in6addr_any;
			    in6.sin6_port = htons(nSocketPort);
				if ( ::bind(m_Fd, (struct sockaddr *)&in6, sizeof(in6)) == SOCKET_ERROR ) {
					GetMainWnd()->DelAsyncSelect(m_Fd, this);
					::closesocket(m_Fd);
					m_Fd = (-1);
					continue;
				}
			}
		}

		val = 1;
		if ( ::ioctlsocket(m_Fd, FIONBIO, &val) != 0 ) {
			GetMainWnd()->DelAsyncSelect(m_Fd, this);
			::closesocket(m_Fd);
			m_Fd = (-1);
			continue;
		}

	    if ( ::connect(m_Fd, ai->ai_addr, (int)ai->ai_addrlen) == SOCKET_ERROR ) {
			if ( WSAGetLastError() != WSAEWOULDBLOCK ) {
				GetMainWnd()->DelAsyncSelect(m_Fd, this);
				::closesocket(m_Fd);
				m_Fd = (-1);
				continue;
			}
		} else
			OnPreConnect();

		break;
	}
	freeaddrinfo(aitop);
	if ( ai == NULL ) {
		CString errmsg;
		errmsg.Format("ExtSocket Open Error '%s:%d'", lpszHostAddress, nHostPort);
		AfxMessageBox(errmsg, MB_ICONSTOP);
		return FALSE;
	}
	return TRUE;
#endif
}
void CExtSocket::Close()
{
	SSLClose();

	if ( m_Fd != (-1) ) {
		GetMainWnd()->DelAsyncSelect(m_Fd, this);
		::shutdown(m_Fd, 2);
		::closesocket(m_Fd);
		m_Fd = (-1);
	}
	if ( m_Fdv6 != (-1) ) {
		GetMainWnd()->DelAsyncSelect(m_Fdv6, this);
		::shutdown(m_Fdv6, 2);
		::closesocket(m_Fdv6);
		m_Fdv6 = (-1);
	}
}
int CExtSocket::Accept(class CExtSocket *pSock, SOCKET hand)
{
	unsigned long val = 1;
	SOCKET sock;
	struct sockaddr_storage sa;
	int len;

    memset(&sa, 0, sizeof(sa));
    len = sizeof(sa);
    if ( (sock = accept(hand, (struct sockaddr *)&sa, &len)) < 0 )
		return FALSE;

	if ( ::ioctlsocket(sock, FIONBIO, &val) != 0 ) {
		::closesocket(sock);
		return FALSE;
	}

	if ( !pSock->Attach(m_pDocument, sock) ) {
		::closesocket(sock);
		return FALSE;
	}

	return TRUE;
}
int CExtSocket::Attach(class CRLoginDoc *pDoc, SOCKET fd)
{
	Close();

	m_pDocument = pDoc;
	m_Fd = fd;

	while ( m_ProcHead != NULL )
		m_ProcHead = RemoveHead(m_ProcHead);

	while ( m_RecvHead != NULL )
		m_RecvHead = RemoveHead(m_RecvHead);

	while ( m_SendHead != NULL )
		m_SendHead = RemoveHead(m_SendHead);

	m_ProcSize[0] = m_ProcSize[1] = 0;
	m_RecvSize[0] = m_RecvSize[1] = 0;
	m_SendSize[0] = m_SendSize[1] = 0;

	m_SocketEvent = FD_READ | FD_OOB | FD_CONNECT | FD_CLOSE;
	m_RecvOverSize = RECVBUFSIZ;

	if ( !GetMainWnd()->SetAsyncSelect(m_Fd, this, m_SocketEvent) )
		return FALSE;

	return TRUE;
}
int CExtSocket::Recive(void* lpBuf, int nBufLen, int nFlags)
{
	int len = 0;
	m_RecvSema.Lock();
	if ( m_ProcHead != NULL && m_ProcHead->m_Type == nFlags ) {
		if ( (len = m_ProcHead->GetSize()) > nBufLen )
			len = nBufLen;
		memcpy(lpBuf, m_ProcHead->GetPtr(), len);
		if ( m_ProcHead->Consume(len) <= 0 )
			m_ProcHead = RemoveHead(m_ProcHead);
		if ( (m_ProcSize[nFlags] -= len) <= 0 )
			m_RecvSyncMode |= 010;
	}
	m_RecvSema.Unlock();
	return len;
}
int CExtSocket::SyncRecive(void* lpBuf, int nBufLen, int nSec, BOOL *pAbort)
{
	int n;
	int len = nBufLen;
	BOOL f = FALSE;
	time_t st, nt;

	time(&st);
	while ( !*pAbort ) {
		m_RecvSema.Lock();
		while ( m_ProcHead != NULL ) {
			if ( m_ProcHead->m_Type != 0 ) {
				m_ProcSize[1] -= m_ProcHead->GetSize();
				m_ProcHead = RemoveHead(m_ProcHead);
			} else {
				if ( (n = m_ProcHead->GetSize()) > nBufLen )
					n = nBufLen;
				memcpy(lpBuf, m_ProcHead->GetPtr(), n);
				if ( m_ProcHead->Consume(n) <= 0 )
					m_ProcHead = RemoveHead(m_ProcHead);
				if ( (m_ProcSize[0] -= n) < RECVMINSIZ ) {
					m_RecvSyncMode |= 010;
					if ( m_Fd != (-1) && (m_SocketEvent & FD_READ) == 0 )
						f = TRUE;
				}
				m_RecvSema.Unlock();
				if ( f )
					GetMainWnd()->PostMessage(WM_SOCKSEL, m_Fd, FD_READ);
				return n;
			}
		}
		if ( m_pRecvEvent != NULL ) {
			m_pRecvEvent->ResetEvent();
			m_RecvSyncMode |= 020;
			m_RecvSema.Unlock();
			time(&nt);
			if ( (n = (int)(nt - st)) >= nSec )
				break;
//			TRACE("SyncRecive wait %d\n", nSec - n);
			WaitForSingleObject(m_pRecvEvent->m_hObject, (nSec - n) * 1000);
		} else {
			m_RecvSema.Unlock();
			time(&nt);
			if ( (n = (int)(nt - st)) >= nSec )
				break;
			Sleep(100);
		}
	}
	return 0;
}
void CExtSocket::SyncReciveBack(void *lpBuf, int nBufLen)
{
	CSockBuffer *sp = AllocBuffer();
	m_RecvSema.Lock();
	sp->m_Type = 0;
	sp->Alloc(nBufLen);
	memcpy(sp->GetLast(), lpBuf, nBufLen);
	sp->SetSize(nBufLen);
	m_ProcHead = AddHead(sp, m_ProcHead);
	m_ProcSize[0] += nBufLen;
	m_RecvSyncMode &= ~010;
	m_RecvSema.Unlock();
}
int CExtSocket::SyncRecvSize()
{
	return m_ProcSize[0];
}
void CExtSocket::SyncAbort()
{
	if ( m_pRecvEvent == NULL )
		return;
	m_RecvSema.Lock();
	m_pRecvEvent->SetEvent();
	m_RecvSema.Unlock();
}
void CExtSocket::SetRecvSyncMode(BOOL mode)
{
	if ( m_pRecvEvent == NULL )
		m_pRecvEvent = new CEvent(FALSE, TRUE);
	m_RecvSyncMode = (mode ? 001 : 000); 
}
int CExtSocket::Send(const void* lpBuf, int nBufLen, int nFlags)
{
	int n;
	int len = nBufLen;
	CSockBuffer *sp;

	if ( nBufLen <= 0 )
		return 0;

	if ( m_SendHead == NULL ) {
		n = (m_SSL_mode ? SSL_write(m_SSL_pSock, lpBuf, nBufLen):
						  ::send(m_Fd, (char *)lpBuf, nBufLen, nFlags));
		if ( n > 0 ) {
			m_OnSendFlag |= 001;
			lpBuf = (LPBYTE)lpBuf + n;
			if ( (nBufLen -= n) <= 0 )
				return len;
		}
	}

	if ( m_SendHead == NULL || (sp = m_SendHead->m_Left)->m_Type != nFlags || sp->GetSize() > RECVBUFSIZ )
		sp = AllocBuffer();
	sp->m_Type = nFlags;
	sp->Alloc(nBufLen);
	memcpy(sp->GetLast(), lpBuf, nBufLen);
	sp->SetSize(nBufLen);
	if ( m_SendHead == NULL || sp != m_SendHead->m_Left )
		m_SendHead = AddTail(sp, m_SendHead);
	m_SendSize[nFlags] += nBufLen;

//	TRACE("Send Save %d\n", nBufLen);
	
	if ( (m_SocketEvent & FD_WRITE) == 0 ) {
		m_SocketEvent |= FD_WRITE;
		WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);
	}
	return len;
}
void CExtSocket::SendWindSize(int x, int y)
{
}
void CExtSocket::SendBreak(int opt)
{
}
void CExtSocket::SetXonXoff(int sw)
{
}

//////////////////////////////////////////////////////////////////////

int CExtSocket::GetRecvSize()
{
	return (m_ProcSize[0] + m_ProcSize[1]);
}
int CExtSocket::GetSendSize()
{
	return (m_SendSize[0] + m_SendSize[1]);
}
void CExtSocket::OnRecvEmpty()
{
}
void CExtSocket::OnSendEmpty()
{
}

//////////////////////////////////////////////////////////////////////

BOOL CExtSocket::ProxyReadLine()
{
	int c = 0;

	while ( m_RecvHead != NULL ) {
		if ( m_RecvHead->m_Type != 0 ) {
			m_RecvSize[m_RecvHead->m_Type] -= m_RecvHead->GetSize();
			m_RecvHead = RemoveHead(m_RecvHead);
		} else {
			while ( (c = m_RecvHead->GetByte()) != EOF ) {
				m_RecvSize[m_RecvHead->m_Type] -= 1;
				if ( c == '\n' )
					break;
				else if ( c != '\r' )
					m_ProxyStr += (char)c;
			}
			if ( m_RecvHead->GetSize() <= 0 )
				m_RecvHead = RemoveHead(m_RecvHead);
			if ( c == '\n' )
				break;
		}
	}
	return (c == '\n' ? TRUE : FALSE);
}
BOOL CExtSocket::ProxyReadBuff(int len)
{
	int n;
	if ( m_ProxyBuff.GetSize() >= len )
		return TRUE;
	while ( m_RecvHead != NULL ) {
		if ( m_RecvHead->m_Type != 0 ) {
			m_RecvSize[m_RecvHead->m_Type] -= m_RecvHead->GetSize();
			m_RecvHead = RemoveHead(m_RecvHead);
		} else {
			if ( (n = len - m_ProxyBuff.GetSize()) <= 0 )
				return TRUE;
			if ( m_RecvHead->GetSize() < n )
				n = m_RecvHead->GetSize();
			m_ProxyBuff.Apend(m_RecvHead->GetPtr(), n);
			m_RecvHead->Consume(n);
			m_RecvSize[m_RecvHead->m_Type] -= n;
			if ( m_RecvHead->GetSize() <= 0 )
				m_RecvHead = RemoveHead(m_RecvHead);
			if ( m_ProxyBuff.GetSize() >= len )
				return TRUE;
		}
	}
	return FALSE;
}
int CExtSocket::ProxyFunc()
{
	int n, i;
	CBuffer buf;
	CString tmp;
	CBuffer A1, A2;

	while ( m_ProxyStatus ) {
		switch(m_ProxyStatus) {
		case 1:		// HTTP
			tmp.Format("CONNECT %s:%d HTTP/1.0\r\n"\
					   "Host: %s\r\n"\
					   "Connection: keep-alive\r\n"\
					   "\r\n",
					   m_ProxyHost, m_ProxyPort, m_RealHostAddr);
			CExtSocket::Send((void *)(LPCSTR)tmp, tmp.GetLength(), 0);
			m_ProxyStatus = 2;
			m_ProxyStr.Empty();
			m_ProxyResult.RemoveAll();
			break;
		case 2:
			if ( !ProxyReadLine() )
				return TRUE;
			if ( m_ProxyStr.IsEmpty() ) {
				m_ProxyStatus = 3;
				break;
			} else if ( (m_ProxyStr[0] == '\t' || m_ProxyStr[0] == ' ') && (n = m_ProxyResult.GetSize()) > 0 )
				m_ProxyResult[n - 1] += m_ProxyStr;
			else
				m_ProxyResult.Add(m_ProxyStr);
			m_ProxyStr.Empty();
			break;
		case 3:
			m_ProxyCode = 0;
			m_ProxyLength = 0;
			m_ProxyConnect = FALSE;
			m_ProxyAuth.RemoveAll();
			for ( n = 0 ; n < m_ProxyResult.GetSize() ; n++ ) {
				if ( _strnicmp(m_ProxyResult[n], "HTTP", 4) == 0 ) {
					if ( (i = m_ProxyResult[n].Find(' ')) >= 0 )
						m_ProxyCode = atoi(m_ProxyResult[n].Mid(i + 1));
				} else if ( _strnicmp(m_ProxyResult[n], "Content-Length:", 15) == 0 ) {
					if ( (i = m_ProxyResult[n].Find(' ')) >= 0 )
						m_ProxyLength = atoi(m_ProxyResult[n].Mid(i + 1));
				} else if ( _strnicmp(m_ProxyResult[n], "WWW-Authenticate:", 17) == 0 ) {
					m_ProxyAuth.SetArray(m_ProxyResult[n]);
				} else if ( _strnicmp(m_ProxyResult[n], "Proxy-Authenticate:", 19) == 0 ) {
					m_ProxyAuth.SetArray(m_ProxyResult[n]);
				} else if ( _strnicmp(m_ProxyResult[n], "Connection:", 11) == 0 ) {
					tmp = m_ProxyResult[n].Mid(12);
					tmp.Trim(" \t\r\n");
					if ( tmp.CompareNoCase("Keep-Alive") == 0 )
						m_ProxyConnect = TRUE;
				}
			}
			m_ProxyStatus = (m_ProxyLength > 0 ? 4 : 5);
			m_ProxyBuff.Clear();
			break;
		case 4:
			if ( !ProxyReadBuff(m_ProxyLength) )
				return TRUE;
			m_ProxyStatus = 5;
			break;
		case 5:
			switch(m_ProxyCode) {
			case 200:
				m_ProxyStatus = 0;
				OnPreConnect();
				break;
			case 401:	// Authorization Required
			case 407:	// Proxy-Authorization Required
				if ( m_ProxyConnect ) {
					if ( m_ProxyAuth.Find("basic") >= 0 ) {
						m_ProxyStatus = 6;
						break;
					} else if ( m_ProxyAuth.Find("digest") >= 0 ) {
						m_ProxyStatus = 7;
						break;
					}
				}
			default:
				m_pDocument->m_ErrorPrompt = (m_ProxyResult.GetSize() > 0 ? m_ProxyResult[0]: "");
				GetMainWnd()->PostMessage(WM_SOCKSEL, m_Fd, WSAMAKESELECTREPLY(0, WSAECONNREFUSED));
				m_ProxyStatus = 0;
				return TRUE;
			}
			break;
		case 6:
			tmp.Format("%s:%s", m_ProxyUser, m_ProxyPass);
			buf.Base64Encode((LPBYTE)(LPCSTR)tmp, tmp.GetLength());
			tmp.Format("CONNECT %s:%d HTTP/1.0\r\n"\
					   "Host: %s\r\n"\
					   "%sAuthorization: Basic %s\r\n"\
					   "\r\n",
					   m_ProxyHost, m_ProxyPort, m_RealHostAddr,
					   (m_ProxyCode == 407 ? "Proxy-" : ""),
					   buf.GetPtr());
			CExtSocket::Send((void *)(LPCSTR)tmp, tmp.GetLength(), 0);
			m_ProxyStatus = 2;
			m_ProxyStr.Empty();
			m_ProxyResult.RemoveAll();
			SSLClose();					// XXXXXXXX BUG???
			break;
		case 7:
			if (m_ProxyAuth.Find("qop") < 0 || m_ProxyAuth["algorithm"].m_String.CompareNoCase("md5") != 0 || m_ProxyAuth.Find("realm") < 0 || m_ProxyAuth.Find("nonce") < 0 ) {
				m_pDocument->m_ErrorPrompt = "Authorization Paramter Error";
				GetMainWnd()->PostMessage(WM_SOCKSEL, m_Fd, WSAMAKESELECTREPLY(0, WSAECONNREFUSED));
				m_ProxyStatus = 0;
				return TRUE;
			}

			tmp.Format("%s:%d", m_ProxyHost, m_ProxyPort);
			m_ProxyAuth["uri"] = tmp;
			m_ProxyAuth["qop"] = "auth";
			tmp.Format("%08d", 1);
			m_ProxyAuth["nc"] = tmp;
			srand((int)time(NULL));
			tmp.Format("%04x%04x%04x%04x", rand(), rand(), rand(), rand());
			m_ProxyAuth["cnonce"] = tmp;

			tmp.Format("%s:%s:%s", m_ProxyUser, (LPCSTR)m_ProxyAuth["realm"], m_ProxyPass);
			A1.md5(tmp);

			tmp.Format("%s:%s", "CONNECT", (LPCSTR)m_ProxyAuth["uri"]);
			A2.md5(tmp);

			tmp.Format("%s:%s:%s:%s:%s:%s",
				A1.GetPtr(), (LPCSTR)m_ProxyAuth["nonce"], (LPCSTR)m_ProxyAuth["nc"],
				(LPCSTR)m_ProxyAuth["cnonce"], (LPCSTR)m_ProxyAuth["qop"], A2.GetPtr());
			buf.md5(tmp);

			tmp.Format("CONNECT %s:%d HTTP/1.0\r\n"\
					   "Host: %s\r\n"\
					   "%sAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\","\
					   " algorithm=%s, response=\"%s\","\
					   " qop=%s, uri=\"%s\","\
					   " nc=%s, cnonce=\"%s\""\
					   "\r\n"\
					   "\r\n",
					   m_ProxyHost, m_ProxyPort, m_RealHostAddr,
					   (m_ProxyCode == 407 ? "Proxy-" : ""),
					   m_ProxyUser, (LPCSTR)m_ProxyAuth["realm"], (LPCSTR)m_ProxyAuth["nonce"],
					   (LPCSTR)m_ProxyAuth["algorithm"], buf.GetPtr(),
					   (LPCSTR)m_ProxyAuth["qop"], (LPCSTR)m_ProxyAuth["uri"],
					   (LPCSTR)m_ProxyAuth["nc"], (LPCSTR)m_ProxyAuth["cnonce"]);
			CExtSocket::Send((void *)(LPCSTR)tmp, tmp.GetLength(), 0);
			m_ProxyStatus = 2;
			m_ProxyStr.Empty();
			m_ProxyResult.RemoveAll();
			SSLClose();					// XXXXXXXX BUG???
			break;

		case 10:	// SOCKS4
			ULONG dw;
			struct hostent *hp;
			if ( (hp = ::gethostbyname(m_ProxyHost)) == NULL ) {
				m_ProxyStatus = 19;
				break;
			}
			dw = ((struct in_addr *)(hp->h_addr))->s_addr;
			buf.Clear();
			buf.Put8Bit(4);
			buf.Put8Bit(1);
			buf.Put8Bit(m_ProxyPort >> 8);
			buf.Put8Bit(m_ProxyPort);
			buf.Put8Bit(dw);
			buf.Put8Bit(dw >> 8);
			buf.Put8Bit(dw >> 16);
			buf.Put8Bit(dw >> 24);
			buf.Apend((LPBYTE)(LPCSTR)m_ProxyUser, m_ProxyUser.GetLength() + 1);
			CExtSocket::Send(buf.GetPtr(), buf.GetSize(), 0);
			m_ProxyStatus = 11;
			m_ProxyBuff.Clear();
			break;
		case 11:
			if ( !ProxyReadBuff(8) )
				return TRUE;
			m_ProxyStatus = 12;
			break;
		case 12:
			if ( *(m_ProxyBuff.GetPos(1)) == 90 ) {
				m_ProxyStatus = 0;
				OnPreConnect();
			} else
				m_ProxyStatus = 19;
			break;
		case 19:
			m_pDocument->m_ErrorPrompt = "SOCKS4 Proxy Conection Error";
			GetMainWnd()->PostMessage(WM_SOCKSEL, m_Fd, WSAMAKESELECTREPLY(0, WSAECONNREFUSED));
			m_ProxyStatus = 0;
			return TRUE;

		case 20:	// SOCKS5
			buf.Clear();
			buf.Put8Bit(5);
			buf.Put8Bit(3);
			buf.Put8Bit(0x00);		// SOCKS5_AUTH_NOAUTH
			buf.Put8Bit(0x02);		// SOCKS5_AUTH_USERPASS
			buf.Put8Bit(0x03);		// SOCKS5_AUTH_CHAP
			CExtSocket::Send(buf.GetPtr(), buf.GetSize(), 0);
			m_ProxyStatus = 21;
			m_ProxyBuff.Clear();
			break;
		case 21:
			if ( !ProxyReadBuff(2) )
				return TRUE;
			if ( *(m_ProxyBuff.GetPos(0)) != 5 || (m_ProxyCode = *(m_ProxyBuff.GetPos(1))) == 0xFF )
				m_ProxyStatus = 29;
			else if ( m_ProxyCode == 0x03 )	// SOCKS5_AUTH_CHAP
				m_ProxyStatus = 30;
			else if ( m_ProxyCode == 0x02 )	// SOCKS5_AUTH_USERPASS
				m_ProxyStatus = 22;
			else if ( m_ProxyCode == 0x00 )	// SOCKS5_AUTH_NOAUTH
				m_ProxyStatus = 24;
			else
				m_ProxyStatus = 29;
			break;
		case 22:	// SOCKS5_AUTH_USERPASS
			buf.Clear();
			buf.Put8Bit(1);
			buf.Put8Bit(m_ProxyUser.GetLength());
			buf.Apend((LPBYTE)(LPCSTR)m_ProxyUser, m_ProxyUser.GetLength());
			buf.Put8Bit(m_ProxyPass.GetLength());
			buf.Apend((LPBYTE)(LPCSTR)m_ProxyPass, m_ProxyPass.GetLength());
			CExtSocket::Send(buf.GetPtr(), buf.GetSize(), 0);
			m_ProxyStatus = 23;
			m_ProxyBuff.Clear();
			break;
		case 23:
			if ( !ProxyReadBuff(2) )
				return TRUE;
			if ( *(m_ProxyBuff.GetPos(1)) != 0 )
				m_ProxyStatus = 29;
			else
				m_ProxyStatus = 24;
			break;
		case 24:	// SOCKS5_CONNECT
			buf.Clear();
			buf.Put8Bit(5);		// SOCKS version 5
			buf.Put8Bit(1);		// CONNECT
			buf.Put8Bit(0);		// FLAG
			buf.Put8Bit(3);		// DOMAIN
			buf.Put8Bit(m_ProxyHost.GetLength());
			buf.Apend((LPBYTE)(LPCSTR)m_ProxyHost, m_ProxyHost.GetLength());
			buf.Put8Bit(m_ProxyPort >> 8);
			buf.Put8Bit(m_ProxyPort);
			CExtSocket::Send(buf.GetPtr(), buf.GetSize(), 0);
			m_ProxyStatus = 25;
			m_ProxyBuff.Clear();
			break;
		case 25:
			if ( !ProxyReadBuff(4) )
				return TRUE;
			if ( *(m_ProxyBuff.GetPos(0)) != 5 || *(m_ProxyBuff.GetPos(1)) != 0 )
				m_ProxyStatus = 29;
			switch(*(m_ProxyBuff.GetPos(3))) {
			case 1:
				m_ProxyLength = 4 + 4 + 2;
				m_ProxyStatus = 27;
				break;
			case 3:
				m_ProxyLength = 4 + 1;
				m_ProxyStatus = 26;
				break;
			case 4:
				m_ProxyLength = 4 + 16 + 2;
				m_ProxyStatus = 27;
				break;
			}
			break;
		case 26:
			if ( !ProxyReadBuff(m_ProxyLength) )
				return TRUE;
			m_ProxyLength = 4 + 1 + *(m_ProxyBuff.GetPos(4)) + 2;
			m_ProxyStatus = 27;
			break;
		case 27:
			if ( !ProxyReadBuff(m_ProxyLength) )
				return TRUE;
			m_ProxyStatus = 0;
			OnPreConnect();
			break;
		case 29:	// SOCKS5_ERROR
			m_pDocument->m_ErrorPrompt = "SOCKS5 Proxy Conection Error";
			GetMainWnd()->PostMessage(WM_SOCKSEL, m_Fd, WSAMAKESELECTREPLY(0, WSAECONNREFUSED));
			m_ProxyStatus = 0;
			return TRUE;

		case 30:	// SOCKS5 CHAP
			buf.Clear();
			buf.Put8Bit(0x01);
			buf.Put8Bit(0x02);
			buf.Put8Bit(0x11);
			buf.Put8Bit(0x85);		// HMAC-MD5
			buf.Put8Bit(0x02);
			buf.Put8Bit(m_ProxyUser.GetLength());
			buf.Apend((LPBYTE)(LPCSTR)m_ProxyUser, m_ProxyUser.GetLength());
			CExtSocket::Send(buf.GetPtr(), buf.GetSize(), 0);
			m_ProxyConnect = FALSE;
			m_ProxyStatus = 31;
			m_ProxyBuff.Clear();
			break;
		case 31:
			if ( !ProxyReadBuff(2) )
				return TRUE;
			if ( *(m_ProxyBuff.GetPos(0)) != 0x01 || (m_ProxyNum = *(m_ProxyBuff.GetPos(1))) == 0x00 )
				m_ProxyStatus = 29;
			else 
				m_ProxyStatus = 32;
			m_ProxyBuff.Clear();
			break;
		case 32:
			if ( !ProxyReadBuff(2) )
				return TRUE;
			m_ProxyCode   = *(m_ProxyBuff.GetPos(0));
			m_ProxyLength = *(m_ProxyBuff.GetPos(1));
			m_ProxyBuff.Clear();
			m_ProxyStatus = 33;
			break;
		case 33:
			if ( !ProxyReadBuff(m_ProxyLength) )
				return TRUE;
			m_ProxyStatus = 34;
			switch(m_ProxyCode) {
			case 0x00:
				if ( *(m_ProxyBuff.GetPos(0)) == 0x00 )
					m_ProxyConnect = TRUE;
				else
					m_ProxyStatus = 29;
				break;
			case 0x03:
				{
					HMAC_CTX c;
					u_char hmac[EVP_MAX_MD_SIZE];

					HMAC_Init(&c, (LPCSTR)m_ProxyPass, m_ProxyPass.GetLength(), EVP_md5());
					HMAC_Update(&c, m_ProxyBuff.GetPtr(), m_ProxyBuff.GetSize());
					HMAC_Final(&c, hmac, NULL);
					HMAC_cleanup(&c);

					buf.Clear();
					buf.Put8Bit(0x01);
					buf.Put8Bit(0x01);
					buf.Put8Bit(0x04);
					buf.Put8Bit(16);
					buf.Apend(hmac, 16);
					CExtSocket::Send(buf.GetPtr(), buf.GetSize(), 0);
				}
				break;
			case 0x11:
				if ( *(m_ProxyBuff.GetPos(0)) != 0x85 )
					m_ProxyStatus = 29;
				break;
			}
			break;
		case 34:
			if ( --m_ProxyNum > 0 )
				m_ProxyStatus = 32;
			else if ( m_ProxyConnect )
				m_ProxyStatus = 24;
			else
				m_ProxyStatus = 31;
			m_ProxyBuff.Clear();
			break;
		}
	}
	return FALSE;
}
void CExtSocket::OnPreConnect()
{
	if ( m_SSL_mode != 0 ) {
		if ( !SSLConnect() ) {
			OnError(WSAECONNREFUSED);
			return;
		}
	}

	if ( m_ProxyStatus ) {
		ProxyFunc();
		return;
	}

	OnConnect();
}
void CExtSocket::OnPreClose()
{
	if ( CExtSocket::GetRecvSize() == 0 )
		OnClose();
	else
		m_OnRecvFlag |= 002;
}

//////////////////////////////////////////////////////////////////////

void CExtSocket::OnGetHostByName(LPCSTR pHostName)
{
}
void CExtSocket::OnAsyncHostByName(LPCSTR pHostName)
{
	switch(m_AsyncMode) {
	case 0:
		OnGetHostByName(pHostName);
		break;
	case 1:
		if ( !Create((m_RealHostAddr.IsEmpty() ? NULL:(LPCSTR)m_RealHostAddr), m_RealHostPort, pHostName, m_RealSocketType) )
			OnError(WSAENOTCONN);
		break;
	case 2:
		if ( !Open(pHostName, m_RealHostPort, m_RealRemotePort, m_RealSocketType) )
			OnError(WSAENOTCONN);
		break;
	}
}
void CExtSocket::OnError(int err)
{
	Close();
	if ( m_pDocument == NULL )
		return;
	m_pDocument->OnSocketError(err);
}
void CExtSocket::OnClose()
{
	Close();
	if ( m_pDocument == NULL )
		return;
	m_pDocument->OnSocketClose();
}
void CExtSocket::OnConnect()
{
	if ( m_pDocument == NULL )
		return;
	if ( m_pDocument->m_TextRam.IsOptEnable(TO_RLKEEPAL) ) {
		BOOL opval = TRUE;
		::setsockopt(m_Fd, SOL_SOCKET, SO_KEEPALIVE, (const char *)(&opval), sizeof(opval));
	}
	m_pDocument->OnSocketConnect();
}
void CExtSocket::OnAccept(SOCKET hand)
{
}
void CExtSocket::OnSend()
{
	int n;
	while ( m_SendHead != NULL ) {
		n = (m_SSL_mode ? SSL_write(m_SSL_pSock, m_SendHead->GetPtr(), m_SendHead->GetSize()):
						  ::send(m_Fd, (char *)m_SendHead->GetPtr(), m_SendHead->GetSize(), m_SendHead->m_Type));
		if ( n <= 0 ) {
			m_SocketEvent |= FD_WRITE;
			WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);
			return;
		}
		m_SendSize[m_SendHead->m_Type] -= n;
		if ( m_SendHead->Consume(n) <= 0 )
			m_SendHead = RemoveHead(m_SendHead);
//		TRACE("OnSend %d\n", n);
	}
	m_SocketEvent &= ~FD_WRITE;
	WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);
	m_OnSendFlag |= 001;
//	TRACE("OnSend Empty\n");
}
void CExtSocket::OnRecive(int nFlags)
{
	int n, i;
	CSockBuffer *sp;

	if ( (m_SocketEvent & EventMask[nFlags]) == 0 ) {
		if ( (m_RecvSize[nFlags] + m_ProcSize[nFlags]) < RECVMINSIZ ) {
			m_SocketEvent |= EventMask[nFlags];
			WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);
			TRACE("OnRecive SocketEvent %04x (%d)\n", m_SocketEvent, m_RecvSize[nFlags] + m_ProcSize[nFlags]);
		} else
			return;
	}

	for ( i = 0 ; i < 2 ; i++ ) {
		sp = AllocBuffer();
		sp->m_Type = nFlags;
		sp->Alloc(m_RecvOverSize);

		n = (m_SSL_mode ? SSL_read(m_SSL_pSock, (void *)sp->GetLast(), m_RecvOverSize):
						  ::recv(m_Fd, (char *)sp->GetLast(), m_RecvOverSize, nFlags));

		if ( n > 0 )
			sp->SetSize(n);
		else if ( n < 0 && m_SSL_mode && SSL_get_error(m_SSL_pSock, n) != SSL_ERROR_WANT_READ )
			OnError(WSAENOTCONN);

		if ( sp->GetSize() <= 0 ) {
			FreeBuffer(sp);
			break;
		}

		m_RecvHead = AddTail(sp, m_RecvHead);
		m_RecvSize[nFlags] += sp->GetSize();

		if ( sp->GetSize() < m_RecvOverSize )
			break;
	}

//	TRACE("OnRecive %dByte\n", m_RecvSize[nFlags]);

	if ( (m_SocketEvent & EventMask[nFlags]) != 0 && m_RecvSize[nFlags] > RECVMAXSIZ ) {
		m_SocketEvent &= ~EventMask[nFlags];
		WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);
		TRACE("OnRecive SocketEvent %04x (%d)\n", m_SocketEvent, m_RecvSize[nFlags] + m_ProcSize[nFlags]);
	}

	ReciveCall();
}
int CExtSocket::ReciveCall()
{
	if ( m_RecvHead != NULL && m_ProxyStatus && ProxyFunc() )
		return m_RecvSize[0] + m_RecvSize[1];

	if ( m_RecvHead == NULL )
		return 0;

	if ( (m_OnRecvFlag & 001) != 0 )
		return m_RecvSize[0] + m_RecvSize[1];

	m_OnRecvFlag |= 001;

	OnReciveCallBack(m_RecvHead->GetPtr(), m_RecvHead->GetSize(), m_RecvHead->m_Type);

	int n = m_RecvHead->m_Type;

	m_RecvSize[n] -= m_RecvHead->GetSize();
	m_RecvHead = RemoveHead(m_RecvHead);

	if ( (m_SocketEvent & EventMask[n]) == 0 && (m_RecvSize[n] + m_ProcSize[n]) < RECVMINSIZ )
		OnRecive(n);

	m_OnRecvFlag &= ~001;

	return m_RecvSize[0] + m_RecvSize[1];
}
void CExtSocket::OnReciveCallBack(void *lpBuf, int nBufLen, int nFlags)
{
	int n;
	CSockBuffer *sp;

	if ( nBufLen <= 0 )
		return;

	if ( (m_RecvSyncMode & 001) == 0 && m_ProcHead == NULL ) {
		if ( (n = OnReciveProcBack((LPBYTE)lpBuf, nBufLen, nFlags)) >= nBufLen )
			return;
		lpBuf = (LPBYTE)lpBuf + n;
		nBufLen -= n;
	}

	m_RecvSema.Lock();
	if ( m_ProcHead == NULL || (sp = m_ProcHead->m_Left)->m_Type != nFlags || sp->GetSize() > RECVBUFSIZ )
		sp = AllocBuffer();
	sp->m_Type = nFlags;
	sp->Alloc(nBufLen);
	memcpy(sp->GetLast(), lpBuf, nBufLen);
	sp->SetSize(nBufLen);
	if ( m_ProcHead == NULL || sp != m_ProcHead->m_Left )
		m_ProcHead = AddTail(sp, m_ProcHead);
	m_ProcSize[nFlags] += nBufLen;
	if ( m_pRecvEvent != NULL && (m_RecvSyncMode & 020) != 0 )
		m_pRecvEvent->SetEvent();
	m_RecvSyncMode &= ~030;
	m_RecvSema.Unlock();

//	TRACE("OnReciveCallBack %d\n", m_ProcSize[nFlags]);

	if ( m_Fd != (-1) && (m_SocketEvent & EventMask[nFlags]) != 0 && m_ProcSize[nFlags] > m_RecvOverSize ) {
		m_SocketEvent &= ~EventMask[nFlags];
		WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);
		TRACE("OnReciveCallBack SocketEvent %04x (%d)\n", m_SocketEvent, m_RecvSize[nFlags] + m_ProcSize[nFlags]);
	}
}
int CExtSocket::ReciveProc()
{
	int n;
	if ( (m_RecvSyncMode & 001) == 0 ) {
		if ( m_ProcHead != NULL && (n = OnReciveProcBack(m_ProcHead->GetPtr(), m_ProcHead->GetSize(), m_ProcHead->m_Type)) > 0 ) {
			int i = m_ProcHead->m_Type;
			m_ProcSize[i] -= n;
			if ( m_ProcHead->Consume(n) <= 0 )
				m_ProcHead = RemoveHead(m_ProcHead);
			if ( m_Fd != (-1) && (m_SocketEvent & EventMask[i]) == 0 && (m_RecvSize[i] + m_ProcSize[i]) < RECVMINSIZ )
				OnRecive(i);
			OnRecvEmpty();
			return TRUE;
		}
	} else if ( (m_RecvSyncMode & 010) != 0 ) {
		m_RecvSyncMode &= ~010;
		OnRecvEmpty();
	}
	return FALSE;
}
int CExtSocket::OnReciveProcBack(void *lpBuf, int nBufLen, int nFlags)
{
	if ( m_pDocument != NULL )
		return m_pDocument->OnSocketRecive((LPBYTE)lpBuf, nBufLen, nFlags);

	return nBufLen;
}
int CExtSocket::OnIdle()
{
	if ( m_DestroyFlag ) {
		Destroy();
		return TRUE;
	}

	if ( (m_OnSendFlag & 001) != 0 ) {
		m_OnSendFlag &= ~001;
		OnSendEmpty();
	}

	if ( ReciveCall() )
		return TRUE;

	if ( ReciveProc() )
		return TRUE;

	if ( (m_OnRecvFlag & 002) != 0 && CExtSocket::GetRecvSize() == 0 ) {
		m_OnRecvFlag &= ~002;
		OnClose();
	}

	return FALSE;
}
void CExtSocket::OnTimer(UINT_PTR nIDEvent)
{
}

//////////////////////////////////////////////////////////////////////

BOOL CExtSocket::IOCtl(long lCommand, DWORD* lpArgument)
{
	if ( m_Fd == (-1) || ::ioctlsocket(m_Fd, lCommand, lpArgument) != 0 )
		return FALSE;
	return TRUE;
}
BOOL CExtSocket::SetSockOpt(int nOptionName, const void* lpOptionValue, int nOptionLen, int nLevel)
{
	if ( m_Fd == (-1) || ::setsockopt(m_Fd, nLevel, nOptionName, (const char *)lpOptionValue, nOptionLen) != 0 )
		return FALSE;
	return TRUE;
}
BOOL CExtSocket::GetSockOpt(int nOptionName, void* lpOptionValue, int* lpOptionLen, int nLevel)
{
	if ( m_Fd == (-1) || ::getsockopt(m_Fd, nLevel, nOptionName, (char *)lpOptionValue, lpOptionLen) != 0 )
		return FALSE;
	return TRUE;
}
void CExtSocket::GetPeerName(int fd, CString &host, int *port)
{
#ifdef	NOIPV6
	int inlen;
	struct sockaddr_in in;

	memset(&in, 0, sizeof(in));
	inlen = sizeof(in);
	getpeername(fd, (struct sockaddr *)&in, &inlen);
	host = inet_ntoa(in.sin_addr);
	*port = ntohs(in.sin_port);
#else
	struct sockaddr_storage in;
	socklen_t inlen;
	char name[NI_MAXHOST], serv[NI_MAXSERV ];

	memset(&in, 0, sizeof(in));
	memset(name, 0, sizeof(name));
	memset(serv, 0, sizeof(serv));
	inlen = sizeof(in);
	getpeername(fd, (struct sockaddr *)&in, &inlen);
	getnameinfo((struct sockaddr *)&in, inlen, name, sizeof(name), serv, sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV);
	host = name;
	*port = atoi(serv);
#endif
}
void CExtSocket::GetHostName(int af, void *addr, CString &host)
{
	int n;
	unsigned char *p = (unsigned char *)addr;
	unsigned short *s = (unsigned short *)addr;
	CString tmp;

	host = "";
	switch(af) {
#ifndef	NOIPV6
	case AF_INET6:
		for ( n = 0 ; n < 6 ; n++ ) {
			tmp.Format("%04x:", *(s++));
			host += tmp;
		}
		p = (unsigned char *)s;
#endif
	case AF_INET:
		tmp.Format("%u.%u.%u.%u", p[0], p[1], p[2], p[3]);
		host += tmp;
		break;
	}
}
int CExtSocket::GetPortNum(LPCSTR str)
{
	int n;
    struct servent *sp;

	if ( *str == 0 )
		return (-1);
	else if ( strncmp(str, "COM", 3) == 0 )
		return (0 - atoi(str + 3));
	else if ( (n = atoi(str)) > 0 )
		return n;
	else if ( (sp = getservbyname(str, "tcp")) != NULL )
		return ntohs(sp->s_port);
	else if ( strcmp(str, "telnet") == 0 )
		return 23;
	else if ( strcmp(str, "login") == 0 )
		return 513;
	else if ( strcmp(str, "ssh") == 0 )
		return 22;
	else if ( strcmp(str, "socks") == 0 )
		return 1080;
	else if ( strcmp(str, "http") == 0 )
		return 80;
	else if ( strcmp(str, "https") == 0 )
		return 443;
	return 0;
}
BOOL CExtSocket::SokcetCheck(LPCTSTR lpszHostAddress, UINT nHostPort, UINT nSocketPort, int nSocketType)
{
#ifdef	NOIPV6
	SOCKET Fd;
    struct hostent *hp;
    struct sockaddr_in in;

	if ( (Fd = ::socket(AF_INET, nSocketType, 0)) == (-1) )
		return FALSE;

	if ( nSocketPort != 0 ) {
	    memset(&in, 0, sizeof(in));
	    in.sin_family = AF_INET;
	    in.sin_addr.s_addr = INADDR_ANY;
	    in.sin_port = htons(nSocketPort);
		if ( ::bind(Fd, (struct sockaddr *)&in, sizeof(in)) == SOCKET_ERROR )
			goto ERRRET;
	}

	if ( (hp = ::gethostbyname(lpszHostAddress)) == NULL )
		goto ERRRET;

	in.sin_family = AF_INET;
	in.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
	in.sin_port = htons(nHostPort);

    if ( ::connect(Fd, (struct sockaddr *)&in, sizeof(in)) == SOCKET_ERROR )
		goto ERRRET;

	::closesocket(Fd);
	return TRUE;

ERRRET:
	::closesocket(Fd);
	return FALSE;

#else
	SOCKET Fd;
	struct addrinfo hints, *ai, *aitop;
    struct sockaddr_in in;
    struct sockaddr_in6 in6;
	CString str;
	struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = nSocketType;
	str.Format("%d", nHostPort);

	if ( getaddrinfo(lpszHostAddress, str, &hints, &aitop) != 0)
		return FALSE;

	for ( ai = aitop ; ai != NULL ; ai = ai->ai_next ) {
		if ( ai->ai_family != AF_INET && ai->ai_family != AF_INET6 )
			continue;

		if ( (Fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == (-1) )
			continue;

		if ( nSocketPort != 0 ) {
			if ( ai->ai_family == AF_INET ) {
			    memset(&in, 0, sizeof(in));
			    in.sin_family = AF_INET;
			    in.sin_addr.s_addr = INADDR_ANY;
			    in.sin_port = htons(nSocketPort);
				if ( ::bind(Fd, (struct sockaddr *)&in, sizeof(in)) == SOCKET_ERROR ) {
					::closesocket(Fd);
					continue;
				}
			} else {	// AF_INET6
			    memset(&in6, 0, sizeof(in6));
			    in6.sin6_family = AF_INET6;
			    in6.sin6_addr = in6addr_any;
			    in6.sin6_port = htons(nSocketPort);
				if ( ::bind(Fd, (struct sockaddr *)&in6, sizeof(in6)) == SOCKET_ERROR ) {
					::closesocket(Fd);
					continue;
				}
			}
		}

	    if ( ::connect(Fd, ai->ai_addr, (int)ai->ai_addrlen) == SOCKET_ERROR ) {
			::closesocket(Fd);
			continue;
		}

		break;
	}
	freeaddrinfo(aitop);
	::closesocket(Fd);
	return (ai == NULL ? FALSE : TRUE);
#endif
}

//////////////////////////////////////////////////////////////////////

int CExtSocket::SSLConnect()
{
	DWORD val;
	SSL_METHOD *method;

	WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), 0, 0);

	val = 0;
	::ioctlsocket(m_Fd, FIONBIO, &val);

	GetApp()->SSL_Init();

	switch(m_SSL_mode) {
	case 1:
		method = SSLv2_client_method();
		break;
	case 2:
		method = SSLv3_client_method();
		break;
	case 3:
		method = TLSv1_client_method();
		break;
	}

	if ( (m_SSL_pCtx = SSL_CTX_new(method)) == NULL )
		return FALSE;

	if ( (m_SSL_pSock = SSL_new(m_SSL_pCtx)) == NULL ) {
		SSL_CTX_free(m_SSL_pCtx);
		return FALSE;
	}

	SSL_CTX_set_mode(m_SSL_pCtx, SSL_MODE_AUTO_RETRY);
	SSL_set_fd(m_SSL_pSock, m_Fd);

	if ( SSL_connect(m_SSL_pSock) < 0 ) {
		SSL_free(m_SSL_pSock);
		SSL_CTX_free(m_SSL_pCtx);
		return FALSE;
	}

	val = 1;
	::ioctlsocket(m_Fd, FIONBIO, &val);

	m_SocketEvent |= FD_READ;
	WSAAsyncSelect(m_Fd, GetMainWnd()->GetSafeHwnd(), WM_SOCKSEL, m_SocketEvent);

	return TRUE;
}
void CExtSocket::SSLClose()
{
	if ( m_SSL_pSock != NULL )
		SSL_free(m_SSL_pSock);

	if ( m_SSL_pCtx != NULL )
		SSL_CTX_free(m_SSL_pCtx);

	m_SSL_mode  = 0;
	m_SSL_pCtx  = NULL;
	m_SSL_pSock = NULL;
}