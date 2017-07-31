// Data.h: CData �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATA_H__6A23DC3E_3DDC_47BD_A6FC_E0127564AE6E__INCLUDED_)
#define AFX_DATA_H__6A23DC3E_3DDC_47BD_A6FC_E0127564AE6E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxtempl.h>
#include <afxmt.h>
#include "openssl/bn.h"
#include "Regex.h"
#include "ChatStatDlg.h"

#define	NIMALLOC	256

typedef	union _MEMSWAP {
	LONGLONG	ll;
	LONG		l[2];
	WORD		w[4];
	BYTE		b[8];
} MEMSWAP, *LPMEMSWAP;

#define	WORDSWAP(d, s)		{ d->b[1] = s->b[0]; d->b[0] = s->b[1]; }
#define	LONGSWAP(d, s)		{ d->b[3] = s->b[0]; d->b[2] = s->b[1]; d->b[1] = s->b[2]; d->b[0] = s->b[3]; }
#define	LONGLONGSWAP(d, s)	{ d->b[7] = s->b[0]; d->b[6] = s->b[1]; d->b[5] = s->b[2]; d->b[4] = s->b[3]; \
							  d->b[3] = s->b[4]; d->b[2] = s->b[5]; d->b[1] = s->b[6]; d->b[0] = s->b[7]; }

class CBuffer : public CObject
{
public:
	int m_Max;
	int m_Ofs;
	int m_Len;
	LPBYTE m_Data;

	void ReAlloc(int len);
	inline int GetSize() { return (m_Len - m_Ofs); }
	void Consume(int len);
	inline void ConsumeEnd(int len) { m_Len -= len; }
	void Apend(LPBYTE buff, int len);
	inline void Clear() { m_Len = m_Ofs = 0; }
	inline LPBYTE GetPtr() { return (m_Data + m_Ofs); }
	inline LPBYTE GetPos(int pos) { return (m_Data + m_Ofs + pos); }
	inline BYTE GetAt(int pos) { return m_Data[m_Ofs + pos]; }
	LPBYTE PutSpc(int len);
	void RoundUp(int len);

	void Put8Bit(int val);
	void Put16Bit(int val);
	void Put32Bit(int val);
	void Put64Bit(LONGLONG val);
	void PutBuf(LPBYTE buf, int len);
	void PutStr(LPCSTR str);
	void PutBIGNUM(BIGNUM *val);
	void PutBIGNUM2(BIGNUM *val);
	void PutWord(int val);

	int Get8Bit();
	int Get16Bit();
	int Get32Bit();
	LONGLONG Get64Bit();
	int GetStr(CString &str);
	int GetBuf(CBuffer *buf);
	int GetBIGNUM(BIGNUM *val);
	int GetBIGNUM2(BIGNUM *val);
	int GetWord();

	inline void SET8BIT(LPBYTE pos, int val) { *pos = (BYTE)(val); }
	void SET16BIT(LPBYTE pos, int val);
	void SET32BIT(LPBYTE pos, int val);
	void SET64BIT(LPBYTE pos, LONGLONG val);

	inline int PTR8BIT(LPBYTE pos) { return (int)(*pos); }
	int PTR16BIT(LPBYTE pos);
	int PTR32BIT(LPBYTE pos);
	LONGLONG PTR64BIT(LPBYTE pos);

	void Base64Decode(LPCSTR str);
	void Base64Encode(LPBYTE buf, int len);
	void Base16Decode(LPCSTR str);
	void Base16Encode(LPBYTE buf, int len);
	void QuotedDecode(LPCSTR str);
	void QuotedEncode(LPBYTE buf, int len);
	void md5(LPCSTR str);

	const CBuffer & operator = (CBuffer &data) { Clear(); Apend(data.GetPtr(), data.GetSize()); return *this; }
	operator LPCSTR() { if ( m_Max <= m_Len) ReAlloc(1); m_Data[m_Len] = 0; return (LPCSTR)GetPtr(); }
	operator LPCWSTR() { if ( m_Max <= (m_Len + 1) ) ReAlloc(2); m_Data[m_Len] = 0; m_Data[m_Len + 1] = 0; return (LPCWSTR)GetPtr(); }

#ifdef	_DEBUGXXX
	void RepSet();
	void Report();
	void Debug();
#endif

	CBuffer(int size);
	CBuffer();
	~CBuffer();
};

class CSpace : public CObject
{
public:
	int m_Len;
	LPBYTE m_Data;

	inline int GetSize() { return m_Len; }
	inline LPBYTE GetPtr() { return m_Data; }
	LPBYTE PutBuf(LPBYTE buf, int len)  { if ( m_Data != NULL ) delete m_Data; if ( len <= 0 ) m_Data = NULL; else { m_Data = new BYTE[len]; memcpy(m_Data, buf, len); } m_Len = len; return m_Data; }
	LPBYTE SetSize(int len) { if ( m_Data != NULL ) delete m_Data; if ( len <= 0 ) m_Data = NULL; else m_Data = new BYTE[len]; m_Len = len; return m_Data;  }

	const CSpace & operator = (CSpace &data) { PutBuf(data.GetPtr(), data.GetSize()); return *this; }

	CSpace() { m_Len = 0; m_Data = NULL; }
	~CSpace() { if ( m_Data != NULL ) delete m_Data; }
};

class CStringArrayExt : public CStringArray  
{
public:
	void AddVal(int value) { CString str; str.Format("%d", value); Add(str); }
	int GetVal(int index) { return atoi(GetAt(index)); }
	void AddBin(void *buf, int len);
	int GetBin(int index, void *buf, int len);
	void GetBuf(int index, CBuffer &buf);
	void AddArray(CStringArrayExt &array);
	void GetArray(int index, CStringArrayExt &array);
	void SetString(CString &str, int sep = '\t');
	void GetString(LPCSTR str, int sep = '\t');
	void SetBuffer(CBuffer &buf);
	void GetBuffer(CBuffer &buf);
	const CStringArrayExt & operator = (CStringArrayExt &data);
	void Serialize(CArchive& ar);
	int Find(LPCSTR str);
	int FindNoCase(LPCSTR str);
};

class CBmpFile : public CObject
{
public:
	IPicture *m_pPic;
	CBitmap m_Bitmap;
	int m_Width, m_Height;
	CString m_FileName;

	int LoadFile(LPCSTR filename);
	CBitmap *GetBitmap(CDC *pDC, int width, int height);

	CBmpFile();
	virtual ~CBmpFile();
};

class CFontChacheNode : public CObject
{
public:
	CFont *m_pFont;
	LOGFONT m_LogFont;
	CFontChacheNode *m_pNext;
	int m_Style;
	int m_KanWidMul;

	CFont *Open(LPCSTR pFontName, int Width, int Height, int CharSet, int Style, int Quality);
	CFontChacheNode();
	~CFontChacheNode();
};

#define	FONTCACHEMAX	32

class CFontChache : public CObject
{
public:
	CFontChacheNode *m_pTop[4];
	CFontChacheNode m_Data[FONTCACHEMAX];

	CFontChacheNode *GetFont(LPCSTR pFontName, int Width, int Height, int CharSet, int Style, int Quality, int Hash);
	CFontChache();
};

class CMutexLock : public CObject
{
public:
	CMutex *m_pMutex;
	CSingleLock *m_pLock;
	CMutexLock(LPCSTR lpszName = NULL); 
	~CMutexLock();
};

class COptObject : public CObject
{
public:
	LPCSTR m_pSection;
	int m_MinSize;

	virtual void Init();
	virtual void SetArray(CStringArrayExt &array) = 0;
	virtual void GetArray(CStringArrayExt &array) = 0;

	virtual void Serialize(int mode);					// To Profile
	virtual void Serialize(int mode, CBuffer &buf);		// To CBuffer
	virtual void Serialize(CArchive &ar);				// To Archive

	COptObject();
};

class CStrScriptNode : public CObject
{
public:
	class CStrScriptNode	*m_Left;
	class CStrScriptNode	*m_Right;

	CRegEx		m_Reg;
	CString		m_RecvStr;
	CString		m_SendStr;

	CStrScriptNode();
	~CStrScriptNode();
};

#define	SCPSTAT_NON		0
#define	SCPSTAT_EXEC	1
#define	SCPSTAT_MAKE	2

class CStrScript : public CObject
{
public:
	CStrScriptNode	*m_Node;
	CStrScriptNode	*m_Exec;

	CStringW		m_Str;
	CRegExRes		m_Res;

	CChatStatDlg	m_StatDlg;
	BOOL			m_MakeChat;
	BOOL			m_MakeFlag;
	CStringW		m_Line[3];

	CString			m_LexTmp;

	void Init();
	CStrScriptNode *CopyNode(CStrScriptNode *np);

	void SetNode(CStrScriptNode *np, CBuffer &buf);
	CStrScriptNode *GetNode(CBuffer &buf);
	void SetBuffer(CBuffer &buf);
	int GetBuffer(CBuffer &buf);

	LPCSTR QuoteStr(CString &tmp, LPCSTR str);
	void SetNodeStr(CStrScriptNode *np, CString &str, int nst);
	int GetLex(LPCSTR &str);
	CStrScriptNode *GetNodeStr(int &lex, LPCSTR &str);
	void SetString(CString &str);
	void GetString(LPCSTR str);

	void EscapeStr(LPCWSTR str, CString &buf, BOOL reg = FALSE);
	void AddNode(LPCWSTR recv, LPCWSTR send);

	int Status();
	void ExecInit();
	void ExecStop();

	void ExecNode(CStrScriptNode *np);
	LPCWSTR ExecChar(int ch);

	void SendStr(LPCWSTR str, int len, class CServerEntry *ep = NULL);

	void SetTreeNode(CStrScriptNode *np, CTreeCtrl &tree, HTREEITEM own);
	void SetTreeCtrl(CTreeCtrl &tree);

	CStrScriptNode *GetTreeNode(CTreeCtrl &tree, HTREEITEM hti);
	void GetTreeCtrl(CTreeCtrl &tree);

	const CStrScript & operator = (CStrScript &data);

	CStrScript();
	~CStrScript();
};

class CServerEntry : public CObject
{
public:
	CString m_EntryName;
	CString m_HostName;
	CString m_PortName;
	CString m_UserName;
	CString m_PassName;
	CString m_TermName;
	CString m_IdkeyName;
	int m_KanjiCode;
	int m_ProtoType;
	CBuffer m_ProBuffer;
	CString m_HostReal;
	CString m_UserReal;
	CString m_PassReal;
	BOOL m_SaveFlag;
	BOOL m_CheckFlag;
	int m_Uid;
	CStrScript m_Script;
	int m_ProxyMode;
	CString m_ProxyHost;
	CString m_ProxyPort;
	CString m_ProxyUser;
	CString m_ProxyPass;

	void Init();
	void SetArray(CStringArrayExt &array);
	void GetArray(CStringArrayExt &array);
	void SetBuffer(CBuffer &buf);
	int GetBuffer(CBuffer &buf);
	void SetProfile(LPCSTR pSection);
	int GetProfile(LPCSTR pSection, int Uid);
	void DelProfile(LPCSTR pSection);
	void Serialize(CArchive &ar);

	LPCSTR GetKanjiCode();
	void SetKanjiCode(LPCSTR str);
	int GetProtoType(LPCSTR str);

	const CServerEntry & operator = (CServerEntry &data);
	CServerEntry();
};

class CServerEntryTab : public COptObject
{
public:
	CArray<CServerEntry, CServerEntry &> m_Data;

	void Init();
	void SetArray(CStringArrayExt &array);
	void GetArray(CStringArrayExt &array);
	void Serialize(int mode);
	int AddEntry(CServerEntry &Entry);
	void UpdateAt(int nIndex);
	void RemoveAt(int nIndex);

	inline CServerEntry &GetAt(int nIndex) { return m_Data[nIndex]; }
	inline INT_PTR GetSize() { return m_Data.GetSize(); }

	CServerEntryTab();
};

#define	MASK_SHIFT	001
#define	MASK_CTRL	002
#define	MASK_ALT	004
#define	MASK_APPL	010

class CKeyNode : public CObject
{
public:
	int m_Code;
	int m_Mask;
	CBuffer m_Maps;
	CString m_Temp;

	LPCSTR GetMaps();
	void SetMaps(LPCSTR str);
	LPCSTR GetCode();
	void SetCode(LPCSTR name);
	LPCSTR GetMask();
	void CommandLine(LPCWSTR str, CStringW &cmd);
	void SetComboList(CComboBox *pCombo);

	const CKeyNode & operator = (CKeyNode &data);

	CKeyNode();
};

class CKeyNodeTab : public COptObject
{
public:
	CArray<CKeyNode, CKeyNode &> m_Node;

	void Init();
	void SetArray(CStringArrayExt &array);
	void GetArray(CStringArrayExt &array);

	BOOL FindMaps(int code, int mask, CBuffer *pBuf);
	int Add(CKeyNode &node);
	int Add(int code, int mask, LPCSTR str);
	int Add(LPCSTR code, int mask, LPCSTR maps);
	CKeyNode &GetAt(int pos);
	int GetSize();
	void SetSize(int sz);

	inline CKeyNode & operator[](int nIndex) { return m_Node[nIndex]; }
	const CKeyNodeTab & operator = (CKeyNodeTab &data);
	CKeyNodeTab();
};

class CKeyMac : public CObject
{
public:
	int m_Len;
	LPBYTE m_Data;

	LPBYTE GetPtr() { return m_Data; }
	int GetSize() { return m_Len; }

	void GetMenuStr(CString &str);
	void GetStr(CString &str);
	void SetStr(LPCSTR str);
	void SetBuf(LPBYTE buf, int len);

	BOOL operator == (CKeyMac &data);
	const CKeyMac & operator = (CKeyMac &data);
	CKeyMac();
	~CKeyMac();
};

class CKeyMacTab : public COptObject
{
public:
	CArray<CKeyMac, CKeyMac &> m_Data;

	void Init();
	void GetArray(CStringArrayExt &array);
	void SetArray(CStringArrayExt &array);

	void Top(int nIndex);
	void Add(CKeyMac &tmp);
	void GetAt(int nIndex, CBuffer &buf);
	void SetHisMenu(CWnd *pWnd);

	inline CKeyMac & operator[](int nIndex) { return m_Data[nIndex]; }
	const CKeyMacTab & operator = (CKeyMacTab &data);
	CKeyMacTab();
};

class CParamTab : public COptObject
{
public:
	CString m_IdKeyStr[9];
	CStringArrayExt m_AlgoTab[9];
	CStringArrayExt m_PortFwd;
	CStringArrayExt m_IdKeyList;
	CString m_XDisplay;
	CString m_ExtEnvStr;

	CParamTab();
	void Init();
	void GetArray(CStringArrayExt &array);
	void SetArray(CStringArrayExt &array);

	void GetProp(int num, CString &str, int shuffle = FALSE);
	int GetPropNode(int num, int node, CString &str);

	const CParamTab & operator = (CParamTab &data);
};

class CStringMaps : public CObject
{
public:
	CArray<CStringW, CStringW &> m_Data;

	int Add(LPCWSTR wstr);
	int Find(LPCWSTR wstr);
	int Next(LPCWSTR wstr, int pos);
	LPCWSTR GetAt(int n) { return m_Data[n]; }
	void RemoveAll() { m_Data.RemoveAll(); }
	int GetSize() { return (int)m_Data.GetSize(); }
	void AddWStrBuf(LPBYTE lpBuf, int nLen);

	CStringMaps();
	~CStringMaps();
};

class CFifoBuffer : public CObject
{
public:
	CSemaphore m_Sema;
	CEvent *m_pEvent;
	CBuffer m_Data;

	int GetData(LPBYTE lpBuf, int nBufLen, int sec = 0);
	void SetData(LPBYTE lpBuf, int nBufLen);

	WCHAR GetWChar(int sec = 0);
	void SetWChar(WCHAR ch);
	int WReadLine(CStringW &str, int sec = 0);

	CFifoBuffer();
	~CFifoBuffer();
};

class CStringIndex : public CObject
{
public:
	BOOL m_bNoCase;
	int m_Value;
	CString m_nIndex;
	CString m_String;
	CArray<CStringIndex, CStringIndex &> m_Array;

	const CStringIndex & operator = (CStringIndex &data);
	CStringIndex & operator [] (LPCSTR str);
	CStringIndex & operator [] (int nIndex) { return m_Array[nIndex]; }
	const LPCSTR operator = (LPCSTR str) { return (m_String = str); }
	operator LPCTSTR () const { return m_String; }

	int GetSize() { return m_Array.GetSize(); }
	void SetSize(int nIndex) { m_Array.SetSize(nIndex); }
	LPCSTR GetIndex() { return m_nIndex; }
	void RemoveAll() { m_Array.RemoveAll(); }
	void SetNoCase(BOOL b) { m_bNoCase = b; }
	int Find(LPCSTR str);
	void SetArray(LPCSTR str);

	CStringIndex();
	~CStringIndex();
};

class CStringEnv : public CObject
{
public:
	int m_Value;
	CString m_nIndex;
	CString m_String;
	CArray<CStringEnv, CStringEnv &> m_Array;

	const CStringEnv & operator = (CStringEnv &data);
	CStringEnv & operator [] (LPCSTR str);
	CStringEnv & operator [] (int nIndex) { return m_Array[nIndex]; }
	const LPCSTR operator = (LPCSTR str) { return (m_String = str); }
	operator LPCTSTR () const { return m_String; }

	int GetSize() { return m_Array.GetSize(); }
	void SetSize(int nIndex) { m_Array.SetSize(nIndex); }
	LPCSTR GetIndex() { return m_nIndex; }
	void RemoveAll() { m_Array.RemoveAll(); }
	int Find(LPCSTR str);

	void GetBuffer(CBuffer *bp);
	void SetBuffer(CBuffer *bp);
	void GetString(LPCSTR str);
	void SetString(CString &str);

	CStringEnv();
	~CStringEnv();
};

class CStringBinary : public CObject
{
public:
	CStringBinary *m_pLeft;
	CStringBinary *m_pRight;
	CString m_Index;
	CString m_String;
	int m_Value;

	CStringBinary();
	CStringBinary(LPCSTR str);
	~CStringBinary();

	void RemoveAll();
	CStringBinary * FindValue(int value);
	CStringBinary & operator [] (LPCSTR str);
	const LPCSTR operator = (LPCSTR str) { m_Value = 0; return (m_String = str); }
	operator LPCTSTR () const { return m_String; }
	const int operator = (int val) { return (m_Value = val); }
	operator int () const { return m_Value; }
};

#endif // !defined(AFX_DATA_H__6A23DC3E_3DDC_47BD_A6FC_E0127564AE6E__INCLUDED_)