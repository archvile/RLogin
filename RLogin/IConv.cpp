// IConv.cpp: CIConv �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include <errno.h>
#include "stdafx.h"
#include "RLogin.h"
#include "IConv.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// �\�z/����
//////////////////////////////////////////////////////////////////////

CIConv::CIConv()
{
	m_Mode = 0;
	m_Cd = NULL;
	m_Left = NULL;
	m_Right = NULL;
	memset(m_Table, 0, sizeof(int) * 256);
}

CIConv::~CIConv()
{
	if ( m_Left != NULL )
		delete m_Left;

	if ( m_Right != NULL )
		delete m_Right;

	if ( m_Cd != NULL && m_Cd != (iconv_t)(-1) )
	    iconv_close(m_Cd);
}

/******************
SJIS	JISX213-2
0xF040	0x2121		0xF09F	0x2821
0xF140	0x2321		0xF19F	0x2421
0xF240	0x2521		0xF29F	0x2C21
0xF340	0x2D21		0xF39F	0x2E21
0xF440	0x2F21		0xF49F	0x6E21
0xF540	0x6F21		0xF59F	0x7021
0xF640	0x7121		0xF69F	0x7221
0xF740	0x7321		0xF79F	0x7421
0xF840	0x7521		0xF89F	0x7621
0xF940	0x7721		0xF99F	0x7821
0xFA40	0x7921		0xFA9F	0x7A21
0xFB40	0x7B21		0xFB9F	0x7C21
0xFC40	0x7D21		0xFC9F	0x7E21
*******************/

int CIConv::JisToSJis(int cd)
{
	int    hi,lo;

	hi = (cd >> 8) & 0xFF;
	lo = cd & 0xFF;

	if ( (hi & 1) != 0 )
		lo += (lo <= 0x5F ? 0x1F : 0x20);
	else
		lo += 0x7E;

	if ( (cd & 0xFF0000) == 0 ) {
		if ( hi <= 0x5E )
			hi = (hi + 0xE1) / 2;
		else
			hi = (hi + 0x161) / 2;
	} else {
		if ( hi >= 0x6E )
			hi = (hi + 0x17B) / 2;
		else if ( hi == 0x21 || (hi >= 0x23 && hi <= 0x25) )
			hi = (hi + 0x1BF) / 2;
		else if ( hi == 0x28 || (hi >= 0x2C && hi <= 0x2F) )
			hi = (hi + 0x1B9) / 2;
		else
			return 0x8140;
	}

	return (hi << 8 | lo);
}

int CIConv::SJisToJis(int cd)
{
	int    hi,lo;

	hi = (cd >> 8) & 0xFF;
	lo = cd & 0xFF;

	if ( hi <= 0x9F )
		hi = (hi - 0x71) * 2 + 1;
	else if ( hi <= 0xEF )
		hi = (hi - 0xB1) * 2 + 1;
	else
		hi = (hi - 0x60) * 2 + 1;

	if ( lo >= 0x9F ) {
		lo -= 0x7E;
		hi++;
	} else
		lo -= (lo >= 0x80 ? 0x20 : 0x1F);

	if ( hi >= 0x12A )
		hi += 0x44;
	else if ( hi >= 0x126 || hi == 0x122 )
		hi += 0x06;

	return (hi << 8 | lo);
}

class CIConv *CIConv::GetIConv(LPCSTR from, LPCSTR to)
{
	int n;

	if ( m_Cd == NULL && m_Mode == 0 ) {
		m_From = from;
		m_To   = to;
		m_Mode = 0;

		if ( strcmp(from, "EUC-JISX0213") == 0 )
			m_Mode |= 001;
		else if ( strcmp(from, "SHIFT_JISX0213") == 0 )
			m_Mode |= 002;
		else if ( strcmp(from, "JIS_X0213-2000.1") == 0 )
			m_Mode |= 003;
		else if ( strcmp(from, "JIS_X0213-2000.2") == 0 )
			m_Mode |= 004;

		if ( strcmp(to, "EUC-JISX0213") == 0 )
			m_Mode |= 010;
		else if ( strcmp(to, "SHIFT_JISX0213") == 0 )
			m_Mode |= 020;
		else if ( strcmp(to, "JIS_X0213-2000.1") == 0 )
			m_Mode |= 030;
		else if ( strcmp(to, "JIS_X0213-2000.2") == 0 )
			m_Mode |= 040;

		if ( (m_Mode & 007) >= 003 )
			from = "EUC-JISX0213";
		if ( (m_Mode & 070) >= 030 )
			to   = "EUC-JISX0213";

	    m_Cd = iconv_open(to, from);
		return this;
	}

	if ( (n = m_From.Compare(from)) == 0 && (n = m_To.Compare(to)) == 0 )
		return this;
	else if ( n < 0 ) {
		if ( m_Left == NULL )
			m_Left = new CIConv;
		return m_Left->GetIConv(from, to);
	} else {
		if ( m_Right == NULL )
			m_Right = new CIConv;
		return m_Right->GetIConv(from, to);
	}
}

int CIConv::IConvBuf(LPCSTR from, LPCSTR to, CBuffer *in, CBuffer *out)
{
	int n = 0;
	CIConv *cp;
    char *inp, *otp;
    size_t ins, tns, xns, ots;
	char tmp[4096];

	cp = GetIConv(from, to);
	if ( cp->m_Cd == (iconv_t)(-1) )
		return 0;

	inp = (char *)(in->GetPtr());
	ins = in->GetSize();

    while ( ins > 0 && n != (-1) ) {
		otp = tmp;
		ots = 4096;
		xns = tns = (ins < 2048 ? ins : 2048);
        n = (int)iconv(cp->m_Cd, (const char **)&inp, &tns, &otp, &ots);
		if ( ots < 4096 )
			out->Apend((LPBYTE)tmp, (int)(4096 - ots));
		ins -= (xns - tns);
	}

	otp = tmp;
	ots = 4096;
    iconv(cp->m_Cd, NULL, NULL, &otp, &ots);
	if ( ots < 4096 )
		out->Apend((LPBYTE)tmp, (int)(4096 - ots));

	return out->GetSize();
}
int CIConv::IConvStr(LPCSTR from, LPCSTR to, LPCSTR in, CString &out)
{
	int n = 0;
	CIConv *cp;
    char *inp, *otp;
    size_t ins, ots;
	char tmp[4096];

	cp = GetIConv(from, to);
	if ( cp->m_Cd == (iconv_t)(-1) )
		return 0;

	inp = (char *)in;
	ins = strlen(in);
	out = "";

    while ( ins > 0 && n != (-1) ) {
		otp = tmp;
		ots = 4096;
        n = (int)iconv(cp->m_Cd, (const char **)&inp, &ins, &otp, &ots);
		for ( otp = tmp ; ots < 4096 ; ots++ )
			out += *(otp++);
    }

	otp = tmp;
	ots = 4096;
	iconv(cp->m_Cd, NULL, NULL, &otp, &ots);
	for ( otp = tmp ; ots < 4096 ; ots++ )
		out += *(otp++);

	return out.GetLength();
}
int CIConv::IConvChar(LPCSTR from, LPCSTR to, int ch)
{
	int n = 0;
	int od = ch;
	CIConv *cp;
    char *inp, *otp;
    size_t ins, ots;
    unsigned char itmp[32], otmp[32];

	cp = GetIConv(from, to);
	if ( cp->m_Cd == (iconv_t)(-1) )
		return 0x0000;

	if ( (ch & 0xFFFFFF00) == 0 && m_Table[ch] != 0 )
		return m_Table[ch];

	switch(cp->m_Mode) {
	case 011:		// EUC-JISX0213 > EUC-JISX0213
		goto ENDOF;
	case 021:		// EUC-JISX0213 > SHIFT_JISX0213
		ch = JisToSJis(ch & 0x17F7F);
		goto ENDOF;
	case 031:		// EUC-JISX0213 > JISX0213.1
		if ( ch >= 0x8F0000 )
			ch = 0x0000;
		ch &= 0x7F7F;
		goto ENDOF;
	case 041:		// EUC-JISX0213 > JISX0213.2
		if ( ch < 0x8F0000 )
			ch = 0x0000;
		ch &= 0x7F7F;
		goto ENDOF;

	case 012:		// SHIFT_JISX0213 > EUC-JISX0213
		ch = SJisToJis(ch);
		if ( ch >= 0x10000 )
			ch |= 0x8F0000;
		ch |= 0x8080;
		goto ENDOF;
	case 022:		// SHIFT_JISX0213 > SHIFT_JISX0213
		goto ENDOF;
	case 032:		// SHIFT_JISX0213 > JISX0213.1
		ch = SJisToJis(ch);
		if ( ch >= 0x10000 )
			ch = 0x0000;
		goto ENDOF;
	case 042:		// SHIFT_JISX0213 > JISX0213.2
		ch = SJisToJis(ch);
		if ( ch < 0x10000 )
			ch = 0x0000;
		ch &= 0x7F7F;
		goto ENDOF;

	case 003:		// JISX0213.1 > ???
		ch |= 0x8080;
		break;
	case 013:		// JISX0213.1 > EUC-JISX0213
		ch |= 0x8080;
		goto ENDOF;
	case 023:		// JISX0213.1 > SHIFT_JISX0213
		ch = JisToSJis(ch);
		goto ENDOF;
	case 033:		// JISX0213.1 > JISX0213.1
		goto ENDOF;
	case 043:		// JISX0213.1 > JISX0213.2
		ch = 0x0000;
		goto ENDOF;

	case 004:		// JISX0213.2 > ???
		ch |= 0x8F8080;
		break;
	case 014:		// JISX0213.2 > EUC-JISX0213
		ch |= 0x8F8080;
		goto ENDOF;
	case 024:		// JISX0213.2 > SHIFT_JISX0213
		ch = JisToSJis(ch | 0x10000);
		goto ENDOF;
	case 034:		// JISX0213.2 > JISX0213.1
		ch = 0x0000;
		goto ENDOF;
	case 044:		// JISX0213.2 > JISX0213.2
		goto ENDOF;
	}

	if ( strcmp(from, "UCS-4BE") == 0 ) {
		itmp[n++] = (unsigned char)(ch >> 24);
		itmp[n++] = (unsigned char)(ch >> 16);
		itmp[n++] = (unsigned char)(ch >> 8);
		itmp[n++] = (unsigned char)(ch);
	} else if ( strcmp(from, "UTF-16BE") == 0 ) {
		if ( (ch & 0xFFFF0000) != 0 ) {
			itmp[n++] = (unsigned char)(ch >> 24);
			itmp[n++] = (unsigned char)(ch >> 16);
		}
		itmp[n++] = (unsigned char)(ch >> 8);
		itmp[n++] = (unsigned char)(ch);
	} else {
		if ( (ch & 0xFF000000) != 0 )
			itmp[n++] = (unsigned char)(ch >> 24);
		if ( (ch & 0xFFFF0000) != 0 )
			itmp[n++] = (unsigned char)(ch >> 16);
		if ( (ch & 0xFFFFFF00) != 0 )
			itmp[n++] = (unsigned char)(ch >> 8);
		itmp[n++] = (unsigned char)(ch);
	}

    inp = (char *)itmp;
    ins = n;
    otp = (char *)otmp;
    ots = 32;

    while ( ins > 0 ) {
        if ( iconv(cp->m_Cd, (const char **)&inp, &ins, &otp, &ots) == (size_t)(-1) )
	        return 0x0000;
    }
	iconv(cp->m_Cd, NULL, NULL, &otp, &ots);

	ch = 0;
	otp = (char *)otmp;
	for ( n = (int)(32 - ots) ; n > 0 ;  n-- )
		ch = (ch << 8) | (unsigned char)(*(otp++));

	switch(cp->m_Mode & 070) {
	case 030:		// JISX0213.1
		if ( ch >= 0x8F0000 )
			ch = 0x0000;
		ch &= 0x7F7F;
		break;
	case 040:	// JISX0213.2
		if ( ch < 0x8F0000 )
			ch = 0x0000;
		ch &= 0x7F7F;
		break;
	}

ENDOF:
	if ( (od & 0xFFFFFF00) == 0 )
		m_Table[od] = ch;
	return ch;
}

static int GetCharSet(unsigned int namescount, const char * const * names, void* data)
{
	unsigned int i;
	CStringArray *pArray = (CStringArray *)data;

	for ( i = 0 ; i < namescount ; i++ )
		pArray->Add(names[i]);
	return 0;
}
void CIConv::SetListArray(CStringArray &array)
{
	array.RemoveAll();
	iconvlist(GetCharSet, &array);
	array.Add("JIS_X0213-2000.1");
	array.Add("JIS_X0213-2000.2");
}