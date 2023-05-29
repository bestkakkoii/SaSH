// Arminius' protocol utilities ver 0.1
//
// Any questions and bugs, mailto: arminius@mail.hwaei.com.tw

// -------------------------------------------------------------------
// The following definitions is to define game-dependent codes.
// Before compiling, remove the "//".

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include "autil.h"
#include "util.h"
#include "injector.h"


char Autil::MesgSlice[sizeof(char*) * Autil::SLICE_MAX][Autil::SLICE_SIZE];
int Autil::SliceCount;

char Autil::PersonalKey[32];

#ifdef _STONDEBUG_
extern int g_iMallocCount;
#endif

// -------------------------------------------------------------------
// Initialize utilities
//
void Autil::util_Init(void)
{

	ZeroMemory(PersonalKey, sizeof(PersonalKey));
	ZeroMemory(MesgSlice, sizeof(MesgSlice));

	SliceCount = 0;
}


void Autil::util_Release(void)
{
	ZeroMemory(MesgSlice, sizeof(MesgSlice));
	SliceCount = 0;
}

// -------------------------------------------------------------------
// Split up a message into slices by spearator.  Store those slices
// into a global buffer "char **MesgSlice"
//
// arg: source=message string;  separator=message separator (1 byte)
// ret: (none)
bool Autil::util_SplitMessage(char* source, size_t dstlen, char* separator)
{
	if (source && separator)
	{	// NULL input is invalid.
		char* ptr;
		char* head = source;

		while ((ptr = (char*)strstr(head, separator)) && (SliceCount <= Autil::SLICE_MAX))
		{
			ptr[0] = '\0';
			if (strlen(head) < Autil::SLICE_SIZE)
			{	// discard slices too large
				//strcpy_s(MesgSlice[SliceCount], Autil::SLICE_SIZE - 1, head);
				_snprintf_s(MesgSlice[SliceCount], Autil::SLICE_SIZE, _TRUNCATE, "%s", head);
				SliceCount++;
			}
			head = ptr + 1;
		}
		strcpy_s(source, dstlen, head);	// remove splited slices
	}
	return true;
}

// -------------------------------------------------------------------
// Encode the message
//
// arg: dst=output  src=input
// ret: (none)
void Autil::util_EncodeMessage(char* dst, size_t dstlen, char* src)
{
	std::mt19937 generator(std::random_device{}());
	std::uniform_int_distribution<int> distribution(0, 99);
	int rn = distribution(generator);
	int t1, t2;
	char t3[65500], tz[65500];

#ifdef _BACK_VERSION
	util_swapint(&t1, &rn, "3421");	// encode seed
#else
	Autil::util_swapint(&t1, &rn, const_cast<char*>("2413"));	// encode seed
#endif
	//  t2 = t1 ^ 0x0f0f0f0f;
	t2 = t1 ^ 0xffffffff;
	Autil::util_256to64(tz, (char*)&t2, sizeof(int), const_cast<char*>(Autil::DEFAULTTABLE));

	Autil::util_shlstring(t3, dstlen, src, rn);
	//  printf("random number=%d\n", rn);
	strcat_s(tz, t3);
	Autil::util_xorstring(dst, tz);

}

// -------------------------------------------------------------------
// Decode the message
//
// arg: dst=output  src=input
// ret: (none)
void Autil::util_DecodeMessage(char* dst, size_t dstlen, char* src)
{
	//  strcpy(dst, src);
	//  util_xorstring(dst, src);

#define INTCODESIZE	(sizeof(int)*8+5)/6

	int rn;
	int* t1, t2;
	char t3[4096], t4[4096];	// This buffer is enough for an integer.
	char tz[65500];

	if (src[strlen(src) - 1] == '\n')
		src[strlen(src) - 1] = 0;
	Autil::util_xorstring(tz, src);

	rn = INTCODESIZE;
	//  printf("INTCODESIZE=%d\n", rn);

	strncpy_s(t4, tz, INTCODESIZE);
	t4[INTCODESIZE] = '\0';
	Autil::util_64to256(t3, t4, const_cast<char*>(Autil::DEFAULTTABLE));
	t1 = (int*)t3;
	//  t2 = *t1 ^ 0x0f0f0f0f;
	t2 = *t1 ^ 0xffffffff;
#ifdef _BACK_VERSION
	util_swapint(&rn, &t2, "4312");
#else
	Autil::util_swapint(&rn, &t2, const_cast<char*>("3142"));
#endif
	//  printf("random number=%d\n", rn);
	Autil::util_shrstring(dst, dstlen, tz + INTCODESIZE, rn);

}

// -------------------------------------------------------------------
// Get a function information from MesgSlice.  A function is a complete
// and identifiable message received, beginned at DEFAULTFUNCBEGIN and
// ended at DEFAULTFUNCEND.  This routine will return the function ID
// (Action ID) and how many fields this function have.
//
// arg: func=return function ID    fieldcount=return fields of the function
// ret: 1=success  0=failed (function not complete)
int Autil::util_GetFunctionFromSlice(int* func, int* fieldcount)
{
	char t1[16384];
	int i;

	//  if (strcmp(MesgSlice[0], DEFAULTFUNCBEGIN)!=0) util_DiscardMessage();

	strcpy_s(t1, 16384, MesgSlice[1]);
	// Robin adjust
	//*func=atoi(t1);
	*func = atoi(t1) - 23;
	for (i = 0; i < Autil::SLICE_MAX; i++)
		if (strcmp(MesgSlice[i], Autil::DEFAULTFUNCEND) == 0)
		{
			*fieldcount = i - 2;	// - "&" - "#" - "func" 3 fields
			return 1;
		}

	return 0;	// failed: message not complete
}

// -------------------------------------------------------------------
// Discard a message from MesgSlice.
//
void Autil::util_DiscardMessage(void)
{

	SliceCount = 0;
	/*
	  int i,j;
	  void *ptr;

	  i=0;
	  while ((i<SliceCount)&&(strcmp(MesgSlice[i], DEFAULTFUNCBEGIN)!=0)) i++;

	  if (i>=SliceCount) {
		// discard all message
		for (j=0; j<SliceCount; j++) strcpy(MesgSlice[j],"");
	  } else {
		for (j=0; j<SliceCount-i; j++) {
		  ptr=MesgSlice[j];
		  MesgSlice[j]=MesgSlice[j+i];
		  MesgSlice[j+i]=ptr;
		}
	  }
	*/
}
#ifdef _NEWNET_
extern long TEA樓躇(long* v, long n, long* k);
void stringtohexstr(char* dst, char* src, int len)
{
	int i;
	for (i = 0; i < len; i++)
	{
		sprintf(dst + 2 * i, "%02X ", (unsigned char)src[i]);
	}
	dst[len * 2] = '\0';
}

void util_EncodeMessageTea(char* t2, char* t1)
{
#ifdef _VMP_
	VMProtectBegin("util_EncodeMessageTea");
#endif
	int len = strlen(t1);
	len = len % 4 ? len / 4 + 1 : len / 4;
#ifdef _VMP_
	TEA樓躇((long*)t1, len, (long*)VMProtectDecryptStringA(DENGLUKEY1));
#else
	TEA樓躇((long*)t1, len, (long*)DENGLUKEY1);
#endif
	stringtohexstr(t2, t1, len * 4);
#ifdef _VMP_
	VMProtectEnd();
#endif
}
#endif

// -------------------------------------------------------------------
// Send a message
//
// arg: fd=socket fd   func=function ID   buffer=data to send
void Autil::util_SendMesg(int fd, int func, char* buffer)
{
#ifdef _VMP_
	VMProtectBegin("util_SendMesg");
#endif
	char t1[16384], t2[16384];

	sprintf_s(t1, sizeof(t1), "&;%d%s;#;", func + 13, buffer);
#ifdef _NEWNET_
	util_EncodeMessageTea(t2, t1);
#else
	Autil::util_EncodeMessage(t2, 16384, t1);
#endif


	Injector& injector = Injector::getInstance();

	int size = static_cast<int>(strlen(t2));
	t2[size] = '\n';
	size += 1;

	HANDLE hProcess = injector.getProcess();
	util::VMemory ptr(hProcess, size, true);
	mem::write(hProcess, ptr, t2, size);
	injector.sendMessage(Injector::kSendPacket, ptr, size);
#ifdef _VMP_
	VMProtectEnd();
#endif
}

// -------------------------------------------------------------------
// Convert 8-bit strings into 6-bit strings, buffers that store these strings
// must have enough space.
//
// arg: dst=8-bit string;  src=6-bit string;  len=src strlen;
//      table=mapping table
// ret: 0=failed  >0=bytes converted
int Autil::util_256to64(char* dst, char* src, int len, char* table)
{
	unsigned int dw, dwcounter;
	int i;

	if (!dst || !src || !table) return 0;
	dw = 0;
	dwcounter = 0;
	for (i = 0; i < len; i++)
	{
		dw = (((unsigned int)src[i] & 0xff) << ((i % 3) << 1)) | dw;
		dst[dwcounter++] = table[dw & 0x3f];
		dw = (dw >> 6);
		if (i % 3 == 2)
		{
			dst[dwcounter++] = table[dw & 0x3f];
			dw = 0;
		}
	}
	if (dw) dst[dwcounter++] = table[dw];
	dst[dwcounter] = '\0';
	return dwcounter;
}

// -------------------------------------------------------------------
// Convert 6-bit strings into 8-bit strings, buffers that store these strings
// must have enough space.
//
// arg: dst=6-bit string;  src=8-bit string;  table=mapping table
// ret: 0=failed  >0=bytes converted
int Autil::util_64to256(char* dst, char* src, char* table)
{
	unsigned int dw, dwcounter;
	unsigned int i, j;
	char* ptr = NULL;

	dw = 0;
	dwcounter = 0;
	if (!dst || !src || !table) return 0;
	char c;
	for (i = 0; i < strlen(src); i++)
	{
		c = src[i];
		for (j = 0; j < strlen(table); j++)
		{
			if (table[j] == c)
			{
				ptr = table + j;
				break;
			}
		}
		if (!ptr) return 0;
		if (i % 4)
		{
			dw = ((unsigned int)(ptr - table) & 0x3f) << ((4 - (i % 4)) << 1) | dw;
			dst[dwcounter++] = dw & 0xff;
			dw = dw >> 8;
		}
		else
		{
			dw = (unsigned int)(ptr - table) & 0x3f;
		}
	}
	if (dw) dst[dwcounter++] = dw & 0xff;
	dst[dwcounter] = '\0';
	return dwcounter;
}

// -------------------------------------------------------------------
// This basically is a 256to64 encoder.  But it shifts the result by key.
//
// arg: dst=6-bit string;  src=8-bit string;  len=src strlen;
//      table=mapping table;  key=rotate key;
// ret: 0=failed  >0=bytes converted
int Autil::util_256to64_shr(char* dst, char* src, int len, char* table, char* key)
{
	unsigned int dw, dwcounter, j;
	int i;

	if (!dst || !src || !table || !key) return 0;
	if (strlen(key) < 1) return 0;	// key can't be empty.
	dw = 0;
	dwcounter = 0;
	j = 0;
	for (i = 0; i < len; i++)
	{
		dw = (((unsigned int)src[i] & 0xff) << ((i % 3) << 1)) | dw;
		dst[dwcounter++] = table[((dw & 0x3f) + key[j]) % 64];	// check!
		j++;  if (!key[j]) j = 0;
		dw = (dw >> 6);
		if (i % 3 == 2)
		{
			dst[dwcounter++] = table[((dw & 0x3f) + key[j]) % 64];// check!
			j++;  if (!key[j]) j = 0;
			dw = 0;
		}
	}
	if (dw) dst[dwcounter++] = table[(dw + key[j]) % 64];	// check!
	dst[dwcounter] = '\0';
	return dwcounter;
}

// -------------------------------------------------------------------
// Decoding function of util_256to64_shr.
//
// arg: dst=8-bit string;  src=6-bit string;  table=mapping table;
//      key=rotate key;
// ret: 0=failed  >0=bytes converted
int Autil::util_shl_64to256(char* dst, char* src, char* table, char* key)
{
	unsigned int dw, dwcounter, i, j, k;
	char* ptr = NULL;

	if (!key || (strlen(key) < 1)) return 0;	// must have key

	dw = 0;
	dwcounter = 0;
	j = 0;
	if (!dst || !src || !table) return 0;
	char c;
	for (i = 0; i < strlen(src); i++)
	{
		c = src[i];
		for (k = 0; k < strlen(table); k++)
		{
			if (table[k] == c)
			{
				ptr = table + k;
				break;
			}
		}
		if (!ptr) return 0;
		if (i % 4)
		{
			// check!
			dw = ((((unsigned int)(ptr - table) & 0x3f) + 64 - key[j]) % 64)
				<< ((4 - (i % 4)) << 1) | dw;
			j++;  if (!key[j]) j = 0;
			dst[dwcounter++] = dw & 0xff;
			dw = dw >> 8;
		}
		else
		{
			// check!
			dw = (((unsigned int)(ptr - table) & 0x3f) + 64 - key[j]) % 64;
			j++;  if (!key[j]) j = 0;
		}
	}
	if (dw) dst[dwcounter++] = dw & 0xff;
	dst[dwcounter] = '\0';
	return dwcounter;
}

// -------------------------------------------------------------------
// This basically is a 256to64 encoder.  But it shifts the result by key.
//
// arg: dst=6-bit string;  src=8-bit string;  len=src strlen;
//      table=mapping table;  key=rotate key;
// ret: 0=failed  >0=bytes converted
int Autil::util_256to64_shl(char* dst, char* src, int len, char* table, char* key)
{
	unsigned int dw, dwcounter;
	int i, j;

	if (!dst || !src || !table || !key) return 0;
	if (strlen(key) < 1) return 0;	// key can't be empty.
	dw = 0;
	dwcounter = 0;
	j = 0;
	for (i = 0; i < len; i++)
	{
		dw = (((unsigned int)src[i] & 0xff) << ((i % 3) << 1)) | dw;
		dst[dwcounter++] = table[((dw & 0x3f) + 64 - key[j]) % 64];	// check!
		j++;  if (!key[j]) j = 0;
		dw = (dw >> 6);
		if (i % 3 == 2)
		{
			dst[dwcounter++] = table[((dw & 0x3f) + 64 - key[j]) % 64];	// check!
			j++;  if (!key[j]) j = 0;
			dw = 0;
		}
	}
	if (dw) dst[dwcounter++] = table[(dw + 64 - key[j]) % 64];	// check!
	dst[dwcounter] = '\0';
	return dwcounter;
}

// -------------------------------------------------------------------
// Decoding function of util_256to64_shl.
// 
// arg: dst=8-bit string;  src=6-bit string;  table=mapping table;
//      key=rotate key;
// ret: 0=failed  >0=bytes converted
int Autil::util_shr_64to256(char* dst, char* src, char* table, char* key)
{
	unsigned int dw, dwcounter, i, j, k;
	char* ptr = NULL;
	if (!key || (strlen(key) < 1)) return 0;	// must have key
	dw = 0;
	dwcounter = 0;
	j = 0;
	if (!dst || !src || !table) return 0;
	char c;
	for (i = 0; i < strlen(src); i++)
	{
		c = src[i];
		for (k = 0; k < strlen(table); k++)
		{
			if (table[k] == c)
			{
				ptr = table + k;
				break;
			}
		}
		if (!ptr) return 0;
		if (i % 4)
		{
			// check!
			dw = ((((unsigned int)(ptr - table) & 0x3f) + key[j]) % 64)
				<< ((4 - (i % 4)) << 1) | dw;
			j++;  if (!key[j]) j = 0;
			dst[dwcounter++] = dw & 0xff;
			dw = dw >> 8;
		}
		else
		{
			// check!
			dw = (((unsigned int)(ptr - table) & 0x3f) + key[j]) % 64;
			j++;  if (!key[j]) j = 0;
		}
	}
	if (dw) dst[dwcounter++] = dw & 0xff;
	dst[dwcounter] = '\0';
	return dwcounter;
}

// -------------------------------------------------------------------
// Swap a integer (4 byte).
// The value "rule" indicates the swaping rule.  It's a 4 byte string
// such as "1324" or "2431".
//
void Autil::util_swapint(int* dst, int* src, char* rule)
{
	char* ptr, * qtr;
	int i;

	ptr = (char*)src;
	qtr = (char*)dst;
	for (i = 0; i < 4; i++) qtr[rule[i] - '1'] = ptr[i];
}

// -------------------------------------------------------------------
// Xor a string.  Be careful that your string contains '0xff'.  Your
// data may lose.
//
void Autil::util_xorstring(char* dst, char* src)
{
	unsigned int i;

	for (i = 0; i < strlen(src); i++) dst[i] = src[i] ^ 255;
	dst[i] = '\0';
}

// -------------------------------------------------------------------
// Shift the string right.
//
void Autil::util_shrstring(char* dst, size_t dstlen, char* src, int offs)
{
	char* ptr;
	int len = strlen(src);
	if (!dst || !src || (strlen(src) < 1)) return;

	offs = strlen(src) - (offs % strlen(src));
	ptr = src + offs;
	strcpy_s(dst, dstlen, ptr);
	strncat_s(dst, dstlen, src, offs);
	dst[strlen(src)] = '\0';
}

// -------------------------------------------------------------------
// Shift the string left.
//
void Autil::util_shlstring(char* dst, size_t dstlen, char* src, int offs)
{
	char* ptr;
	if (!dst || !src || (strlen(src) < 1)) return;

	offs = offs % strlen(src);
	ptr = src + offs;
	strcpy_s(dst, dstlen, ptr);
	strncat_s(dst, dstlen, src, offs);
	dst[strlen(src)] = '\0';
}

// -------------------------------------------------------------------
// Convert a message slice into integer.  Return a checksum.
//
// arg: sliceno=slice index in MesgSlice    value=result
// ret: checksum, this value must match the one generated by util_mkint
int Autil::util_deint(int sliceno, int* value)
{
	int* t1, t2;
	char t3[4096];	// This buffer is enough for an integer.

	Autil::util_shl_64to256(t3, MesgSlice[sliceno], const_cast<char*>(Autil::DEFAULTTABLE), PersonalKey);
	t1 = (int*)t3;
	t2 = *t1 ^ 0xffffffff;
#ifdef _BACK_VERSION
	util_swapint(value, &t2, "3421");
#else
	Autil::util_swapint(value, &t2, const_cast<char*>("2413"));
#endif
	return *value;
}

// -------------------------------------------------------------------
// Pack a integer into buffer (a string).  Return a checksum.
//
// arg: buffer=output   value=data to pack
// ret: checksum, this value must match the one generated by util_deint
int Autil::util_mkint(char* buffer, int value)
{
	int t1, t2;
	char t3[4096];	// This buffer is enough for an integer.

#ifdef _BACK_VERSION
	util_swapint(&t1, &value, "4312");
#else
	Autil::util_swapint(&t1, &value, const_cast<char*>("3142"));
#endif
	t2 = t1 ^ 0xffffffff;
	Autil::util_256to64_shr(t3, (char*)&t2, sizeof(int), const_cast<char*>(Autil::DEFAULTTABLE), PersonalKey);
	strcat_s(buffer, 16384, ";");	// It's important to append a SEPARATOR between fields
	strcat_s(buffer, 16384, t3);

	return value;
}

// -------------------------------------------------------------------
// Convert a message slice into string.  Return a checksum.
//
// arg: sliceno=slice index in MesgSlice    value=result
// ret: checksum, this value must match the one generated by util_mkstring
int Autil::util_destring(int sliceno, char* value)
{
	Autil::util_shr_64to256(value, MesgSlice[sliceno], const_cast<char*>(Autil::DEFAULTTABLE), PersonalKey);

	return strlen(value);
}

// -------------------------------------------------------------------
// Convert a string into buffer (a string).  Return a checksum.
//
// arg: buffer=output   value=data to pack
// ret: checksum, this value must match the one generated by util_destring
int Autil::util_mkstring(char* buffer, char* value)
{
	char t1[Autil::SLICE_SIZE];

	Autil::util_256to64_shl(t1, value, strlen(value), const_cast<char*>(Autil::DEFAULTTABLE), PersonalKey);
	strcat_s(buffer, 16384, ";");	// It's important to append a SEPARATOR between fields
	strcat_s(buffer, 16384, t1);

	return strlen(value);
}

int Autil::strcmptail(char* s1, char* s2)
{
	int i;
	int len1 = strlen(s1);
	int len2 = strlen(s2);

	for (i = 0;; i++)
	{
		int ind1 = len1 - 1 - i;
		int ind2 = len2 - 1 - i;
		if (ind1 < 0 || ind2 < 0) return 0;
		if (s1[ind1] != s2[ind2]) return 1;
	}
}

#define IS_2BYTEWORD( _a_ ) ( (char)(0x80) <= (_a_) && (_a_) <= (char)(0xFF) )
char* ScanOneByte(char* src, char delim)
{
	if (!src) return NULL;
	for (; src[0] != '\0'; src++)
	{
		if (IS_2BYTEWORD(src[0]))
		{
			if (src[1] != 0)
			{
				src++;
			}
			continue;
		}
		if (src[0] == delim)
		{
			return src;
		}
	}
	return NULL;
}

char* strncpy2(char* dest, const char* src, size_t n)
{
	if (n > 0)
	{
		char* d = dest;
		const char* s = src;
		unsigned int i;
		for (i = 0; i < n; i++)
		{
			if (*(s + i) == 0)
			{
				*(d + i) = '\0';
				return dest;
			}
			if (*(s + i) & 0x80)
			{
				*(d + i) = *(s + i);
				i++;
				if (i >= n)
				{
					*(d + i - 1) = '\0';
					break;
				}
				*(d + i) = *(s + i);
			}
			else
				*(d + i) = *(s + i);
		}
	}
	return dest;
}


void strcpysafe(char* dest, size_t n, const char* src)
{
	if (!src)
	{
		*dest = '\0';
		return;
	}
	if (n <= 0)
		return;
	else if (n < strlen(src) + 1)
	{
		strncpy2(dest, src, n - 1);
		dest[n - 1] = '\0';
	}
	else
		strcpy_s(dest, 16384, src);
}


void strncpysafe(char* dest, const size_t n,
	const char* src, const int length)
{
	unsigned int Short;
	Short = std::min(strlen(src), (unsigned int)length);
	if (n < Short + 1)
	{
		strncpy2(dest, src, n - 1);
		dest[n - 1] = '\0';

	}
	else if (n <= 0)
	{
		return;
	}
	else
	{
		strncpy2(dest, src, Short);
		dest[Short] = '\0';

	}
}


BOOL Autil::getStringFromIndexWithDelim_body(char* src, char* delim, int index, char* buf, int buflen)
{
	int i;
	int length = 0;
	int addlen = 0;
	int oneByteMode = 0;

	if (strlen(delim) == 1)
	{
		oneByteMode = 1;
	}
	for (i = 0; i < index; i++)
	{
		char* last;
		src += addlen;

		if (oneByteMode)
		{

			last = ScanOneByte(src, delim[0]);
		}
		else
		{
			last = strstr(src, delim);
		}
		if (last == NULL)
		{
			strcpysafe(buf, buflen, src);
			if (i == index - 1)
			{
				if (buf[0] == 0) return FALSE;
				return TRUE;
			}
			buf[0] = 0;
			return FALSE;
		}
		length = last - src;
		addlen = length + strlen(delim);
	}
	strncpysafe(buf, buflen, src, length);
	if (buf[0] == 0) return FALSE;
	return TRUE;
}

void ltrim(char* str)
{
	char* ptr;
	for (ptr = str; *ptr == 32; ptr++);

}

void rtrim(char* str)
{
	int i;
	for (i = (int)strlen(str) - 1; str[i] == 32 && i >= 0; str[i--] = 0);
}
#ifdef _FONT_STYLE_

WM_STR wmstr[25];
extern int getTextLength(char* str);
void getstrstyle(char* str, int index, int pos, int flg, WM_STR wm[])
{
	char* stemp, * etemp;
	if (flg)
		stemp = str;
	else
		stemp = sunday(str, "[style ");
	if (stemp)
	{
		if (stemp != str)
		{
			wm[index].flg = TRUE;
			wm[index].style[pos].size = FONT_SIZE1;
			if (pos == 0)
				wm[index].style[pos].x = 0;
			else
				wm[index].style[pos].x = getTextLength(wm[index].style[pos - 1].str) + wm[index].style[pos - 1].x;
			wm[index].style[pos].color = 0;
			int len = stemp - str;
			memcpy(wm[index].style[pos].str, str, len);
			wm[index].style[pos].str[len] = NULL;
			pos++;
			getstrstyle(stemp, index, pos, TRUE, wm);
		}
		else
		{
			wm[index].flg = TRUE;
			stemp = stemp + 7;
			etemp = sunday(stemp, "]") + 1;
			char* scolor = sunday(stemp, "c=");
			if (scolor)
			{
				scolor = scolor + 2;
				char strnum[3];
				int i = 0;
				for (i; i < 2 && scolor[i] != ' ' && scolor[i] != ']'; i++)
				{
					strnum[i] = scolor[i];
				}
				strnum[i] = 0;
				wm[index].style[pos].color = atoi(strnum);
			}
			else wm[index].style[pos].color = 0;

			char* ssize = sunday(stemp, "s=");
			if (ssize)
			{
				ssize = ssize + 2;
				char strsize[3];
				int i = 0;
				for (i; i < 2 && ssize[i] != ' ' && ssize[i] != ']'; i++)
				{
					strsize[i] = ssize[i];
				}
				strsize[i] = 0;
				wm[index].style[pos].size = atoi(ssize);
			}
			else wm[index].style[pos].size = FONT_SIZE1;

			stemp = sunday(str, "[/style]");

			int len = stemp - etemp;
			memcpy(wm[index].style[pos].str, etemp, len);
			wm[index].style[pos].str[len] = NULL;
			if (pos == 0)
				wm[index].style[pos].x = 0;
			else
				wm[index].style[pos].x = getTextLength(wm[index].style[pos - 1].str) + wm[index].style[pos - 1].x;
			pos++;
			stemp = stemp + 8;
			getstrstyle(stemp, index, pos, FALSE, wm);
		}
	}
	else
	{
		sprintf(wm[index].style[pos].str, "%s", str);
		if (!wm[index].flg)
		{
			wm[index].flg = TRUE;
			wm[index].style[pos].x = 0;
			wm[index].style[pos].color = 0;
			wm[index].style[pos].size = FONT_SIZE1;
		}
		else
		{
			wm[index].style[pos].size = FONT_SIZE1;
			wm[index].style[pos].x = getTextLength(wm[index].style[pos - 1].str) + wm[index].style[pos - 1].x;
			wm[index].style[pos].color = 0;
		}
	}
}

void PutWinText(int x, int y, char fontPrio, int color, char* str, BOOL hitFlag, int index)
{
	int i = 0;
	for (; i < 30; i++)
	{
		if (*wmstr[index].style[i].str)
			StockFontBufferExt(x + wmstr[index].style[i].x, y, fontPrio, wmstr[index].style[i].color,
				wmstr[index].style[i].str, hitFlag, wmstr[index].style[i].size);
	}
}
#ifdef _CHARTITLE_STR_
void PutTitleText(int x, int y, char fontPrio, TITLE_STR str, BOOL hitFlag)
{
	int i = 0;
	for (; i < 10; i++)
	{
		if (*str.style[i].str)
			StockFontBufferExt(x + str.style[i].x, y, fontPrio, str.style[i].color,
				str.style[i].str, hitFlag, str.style[i].size);
	}
}

extern int getTextLength(char* str);
void getTitlestyle(char* str, int pos, int flg, TITLE_STR* wm)
{
	char* stemp, * etemp;
	if (flg)
		stemp = str;
	else
		stemp = sunday(str, "[style ");
	if (stemp)
	{
		if (stemp != str)
		{
			wm->flg = TRUE;
			wm->style[pos].size = FONT_SIZE1;
			if (pos == 0)
				wm->style[pos].x = 0;
			else
				wm->style[pos].x = getTextLength(wm->style[pos - 1].str) + wm->style[pos - 1].x;
			wm->style[pos].color = 0;
			int len = stemp - str;
			memcpy(wm->style[pos].str, str, len);
			wm->style[pos].str[len] = NULL;
			pos++;
			getTitlestyle(stemp, pos, TRUE, wm);
		}
		else
		{
			wm->flg = TRUE;
			stemp = stemp + 7;
			etemp = sunday(stemp, "]") + 1;
			char* scolor = sunday(stemp, "c=");
			if (scolor)
			{
				scolor = scolor + 2;
				char strnum[3];
				int i = 0;
				for (i; i < 2 && scolor[i] != ' ' && scolor[i] != ']'; i++)
				{
					strnum[i] = scolor[i];
				}
				strnum[i] = 0;
				wm->style[pos].color = atoi(strnum);
			}
			else wm->style[pos].color = 0;

			char* ssize = sunday(stemp, "s=");
			if (ssize)
			{
				ssize = ssize + 2;
				char strsize[3];
				int i = 0;
				for (i; i < 2 && ssize[i] != ' ' && ssize[i] != ']'; i++)
				{
					strsize[i] = ssize[i];
				}
				strsize[i] = 0;
				wm->style[pos].size = atoi(ssize);
			}
			else wm->style[pos].size = FONT_SIZE1;

			stemp = sunday(str, "[/style]");

			int len = stemp - etemp;
			memcpy(wm->style[pos].str, etemp, len);
			wm->style[pos].str[len] = NULL;
			if (pos == 0)
				wm->style[pos].x = 0;
			else
				wm->style[pos].x = getTextLength(wm->style[pos - 1].str) + wm->style[pos - 1].x;
			pos++;
			stemp = stemp + 8;
			getTitlestyle(stemp, pos, FALSE, wm);
		}
	}
	else
	{
		sprintf(wm->style[pos].str, "%s", str);
		if (!wm->flg)
		{
			wm->flg = TRUE;
			wm->style[pos].x = 0;
			wm->style[pos].color = 0;
			wm->style[pos].size = FONT_SIZE1;
			wm->len = getTextLength(wm->style[pos].str);
		}
		else
		{
			wm->style[pos].size = FONT_SIZE1;
			wm->style[pos].x = getTextLength(wm->style[pos - 1].str) + wm->style[pos - 1].x;
			wm->style[pos].color = 0;
			wm->len = getTextLength(wm->style[pos].str) + wm->style[pos].x;
		}
	}
}

void getCharTitleSplit(char* str, TITLE_STR* title)
{
	memset(title, 0, sizeof(TITLE_STR));
	if (str == NULL) return;
	int i = 0;
	for (i; i < 25; i++)
	{
		if (*str)
		{
			getTitlestyle(str, 0, FALSE, title);
		}
	}
}
#endif


void getStrSplitNew(char str[][256])
{
	memset(wmstr, 0, sizeof(WM_STR) * 25);
	int i = 0;

	for (i; i < 25; i++)
	{
		if (str[i][0])
		{
			getstrstyle(str[i], i, 0, FALSE, wmstr);
		}
	}
}


#endif





