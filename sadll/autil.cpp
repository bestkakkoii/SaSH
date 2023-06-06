// Arminius' protocol utilities ver 0.1
//
// Any questions and bugs, mailto: arminius@mail.hwaei.com.tw

// -------------------------------------------------------------------
// The following definitions is to define game-dependent codes.
// Before compiling, remove the "//".

#include "pch.h"
#include <stdio.h>
#include <stdlib.h>
#include "autil.h"

char Autil::MesgSlice[sizeof(char*) * Autil::SLICE_MAX][Autil::SLICE_SIZE] = { 0 };
int Autil::SliceCount = 0;

char* Autil::PersonalKey = nullptr;

// -------------------------------------------------------------------
// Initialize utilities
//
void __fastcall Autil::util_Init()
{
	memset(MesgSlice, 0, sizeof(MesgSlice));
	SliceCount = 0;
}


void __fastcall Autil::util_Release()
{
	memset(MesgSlice, 0, sizeof(MesgSlice));
	SliceCount = 0;
}

// -------------------------------------------------------------------
// Split up a message into slices by spearator.  Store those slices
// into a global buffer "char **MesgSlice"
//
// arg: source=message string;  separator=message separator (1 byte)
// ret: (none)
bool __fastcall Autil::util_SplitMessage(char* source, size_t dstlen, char* separator)
{
	if (source && separator)
	{	// NULL input is invalid.
		char* ptr = nullptr;
		char* head = source;

		while ((ptr = reinterpret_cast<char*>(strstr(head, separator))) && (SliceCount <= Autil::SLICE_MAX))
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
void __fastcall Autil::util_EncodeMessage(char* dst, size_t dstlen, char* src)
{
	std::mt19937 generator(std::random_device{}());
	std::uniform_int_distribution<int> distribution(0, 99);
	int rn = distribution(generator);
	int t1 = 0, t2 = 0;
	char t3[65500], tz[65500];
	memset(tz, 0, sizeof(tz));
	memset(t3, 0, sizeof(t3));

#ifdef _BACK_VERSION
	util_swapint(&t1, &rn, "3421");	// encode seed
#else
	Autil::util_swapint(&t1, &rn, const_cast<char*>("2413"));	// encode seed
#endif
	//  t2 = t1 ^ 0x0f0f0f0f;
	t2 = t1 ^ 0xffffffff;
	Autil::util_256to64(tz, reinterpret_cast<char*>(&t2), sizeof(int), const_cast<char*>(Autil::DEFAULTTABLE));

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
void __fastcall Autil::util_DecodeMessage(char* dst, size_t dstlen, char* src)
{
	//  strcpy(dst, src);
	//  util_xorstring(dst, src);

#define INTCODESIZE	(sizeof(int)*8+5)/6

	int rn = 0;
	int* t1 = nullptr;
	int t2 = 0;
	char t3[4096], t4[4096];	// This buffer is enough for an integer.
	char tz[65500];

	memset(tz, 0, sizeof(tz));
	memset(t3, 0, sizeof(t3));
	memset(t4, 0, sizeof(t4));

	if (src[strlen(src) - 1] == '\n')
		src[strlen(src) - 1] = 0;
	Autil::util_xorstring(tz, src);

	rn = INTCODESIZE;
	//  printf("INTCODESIZE=%d\n", rn);

	strncpy_s(t4, tz, INTCODESIZE);
	t4[INTCODESIZE] = '\0';
	Autil::util_64to256(t3, t4, const_cast<char*>(Autil::DEFAULTTABLE));
	t1 = reinterpret_cast<int*>(t3);

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
int __fastcall Autil::util_GetFunctionFromSlice(int* func, int* fieldcount)
{
	char t1[16384];
	memset(t1, 0, sizeof(t1));
	int i = 0;

	//  if (strcmp(MesgSlice[0], DEFAULTFUNCBEGIN)!=0) util_DiscardMessage();

	strcpy_s(t1, 16384, MesgSlice[1]);

	// Robin adjust
	//*func=atoi(t1);
	*func = std::atoi(t1) - 23;
	for (i = 0; i < Autil::SLICE_MAX; i++)
	{
		if (strcmp(MesgSlice[i], Autil::DEFAULTFUNCEND) == 0)
		{
			*fieldcount = i - 2;	// - "&" - "#" - "func" 3 fields
			return 1;
		}
	}

	return 0;	// failed: message not complete
}

// -------------------------------------------------------------------
// Discard a message from MesgSlice.
//
void __fastcall Autil::util_DiscardMessage(void)
{
	SliceCount = 0;
}

// -------------------------------------------------------------------
// Send a message
//
// arg: fd=socket fd   func=function ID   buffer=data to send
void __fastcall Autil::util_SendMesg(int fd, int func, char* buffer)
{
	//#ifdef _VMP_
	//	VMProtectBegin("util_SendMesg");
	//#endif
	//	char t1[16384], t2[16384];
	//
	//	sprintf_s(t1, sizeof(t1), "&;%d%s;#;", func + 13, buffer);
	//#ifdef _NEWNET_
	//	util_EncodeMessageTea(t2, t1);
	//#else
	//	Autil::util_EncodeMessage(t2, 16384, t1);
	//#endif
	//
	//
	//	Injector& injector = Injector::getInstance();
	//
	//	int size = static_cast<int>(strlen(t2));
	//	t2[size] = '\n';
	//	size += 1;
	//
	//	HANDLE hProcess = injector.getProcess();
	//	util::VirtualMemory ptr(hProcess, size, true);
	//	mem::write(hProcess, ptr, t2, size);
	//	injector.sendMessage(Injector::kSendPacket, ptr, size);
	//#ifdef _VMP_
	//	VMProtectEnd();
	//#endif
}

// -------------------------------------------------------------------
// Convert 8-bit strings into 6-bit strings, buffers that store these strings
// must have enough space.
//
// arg: dst=8-bit string;  src=6-bit string;  len=src strlen;
//      table=mapping table
// ret: 0=failed  >0=bytes converted
int __fastcall Autil::util_256to64(char* dst, char* src, int len, char* table)
{
	unsigned int dw = 0u, dwcounter = 0u;
	int i = 0;

	if (!dst || !src || !table)
		return 0;

	dw = 0;
	dwcounter = 0;

	for (i = 0; i < len; i++)
	{
		dw = ((static_cast<unsigned int>(src[i]) & 0xff) << ((i % 3) << 1)) | dw;

		dst[dwcounter++] = table[dw & 0x3f];

		dw = (dw >> 6);

		if (i % 3 == 2)
		{
			dst[dwcounter++] = table[dw & 0x3f];
			dw = 0;
		}
	}

	if (dw)
		dst[dwcounter++] = table[dw];

	dst[dwcounter] = '\0';
	return dwcounter;
}

// -------------------------------------------------------------------
// Convert 6-bit strings into 8-bit strings, buffers that store these strings
// must have enough space.
//
// arg: dst=6-bit string;  src=8-bit string;  table=mapping table
// ret: 0=failed  >0=bytes converted
int __fastcall Autil::util_64to256(char* dst, char* src, char* table)
{
	unsigned int i = 0u, j = 0u;
	char* ptr = nullptr;

	unsigned int dw = 0u;
	unsigned int dwcounter = 0u;

	if (!dst || !src || !table)
		return 0;

	char c = '\0';
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

		if (!ptr)
			return 0;

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

	if (dw)
		dst[dwcounter++] = dw & 0xff;

	dst[dwcounter] = '\0';
	return dwcounter;
}

// -------------------------------------------------------------------
// This basically is a 256to64 encoder.  But it shifts the result by key.
//
// arg: dst=6-bit string;  src=8-bit string;  len=src strlen;
//      table=mapping table;  key=rotate key;
// ret: 0=failed  >0=bytes converted
int __fastcall Autil::util_256to64_shr(char* dst, char* src, int len, char* table, char* key)
{
	unsigned int j = 0u;
	int i = 0u;

	if (!dst || !src || !table || !key)
		return 0;

	if (strlen(key) < 1)
		return 0;	// key can't be empty.

	unsigned int dw = 0u;
	unsigned int dwcounter = 0u;

	j = 0;
	for (i = 0; i < len; i++)
	{
		dw = ((static_cast<unsigned int>(src[i]) & 0xff) << ((i % 3) << 1)) | dw;

		dst[dwcounter++] = table[((dw & 0x3f) + key[j]) % 64];	// check!

		j++;

		if (!key[j])
			j = 0;

		dw = (dw >> 6);

		if (i % 3 == 2)
		{
			dst[dwcounter++] = table[((dw & 0x3f) + key[j]) % 64];// check!
			j++;

			if (!key[j])
				j = 0;

			dw = 0;
		}
	}

	if (dw)
		dst[dwcounter++] = table[(dw + key[j]) % 64];	// check!

	dst[dwcounter] = '\0';
	return dwcounter;
}

// -------------------------------------------------------------------
// Decoding function of util_256to64_shr.
//
// arg: dst=8-bit string;  src=6-bit string;  table=mapping table;
//      key=rotate key;
// ret: 0=failed  >0=bytes converted
int __fastcall Autil::util_shl_64to256(char* dst, char* src, char* table, char* key)
{
	unsigned int i = 0u, j = 0u, k = 0u;
	char* ptr = nullptr;

	if (!key || (strlen(key) < 1))
		return 0;	// must have key

	unsigned int dw = 0u;
	unsigned int dwcounter = 0u;
	j = 0;

	if (!dst || !src || !table)
		return 0;

	char c = '\0';
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

		if (!ptr)
			return 0;

		if (i % 4)
		{
			// check!
			dw = (((static_cast<unsigned int>(ptr - table) & 0x3f) + 64 - key[j]) % 64)
				<< ((4 - (i % 4)) << 1) | dw;

			j++;

			if (!key[j])
				j = 0;

			dst[dwcounter++] = dw & 0xff;
			dw = dw >> 8;
		}
		else
		{
			// check!
			dw = ((static_cast<unsigned int>(ptr - table) & 0x3f) + 64 - key[j]) % 64;
			j++;

			if (!key[j])
				j = 0;
		}
	}

	if (dw)
		dst[dwcounter++] = dw & 0xff;

	dst[dwcounter] = '\0';

	return dwcounter;
}

// -------------------------------------------------------------------
// This basically is a 256to64 encoder.  But it shifts the result by key.
//
// arg: dst=6-bit string;  src=8-bit string;  len=src strlen;
//      table=mapping table;  key=rotate key;
// ret: 0=failed  >0=bytes converted
int __fastcall Autil::util_256to64_shl(char* dst, char* src, int len, char* table, char* key)
{
	int i = 0, j = 0;

	if (!dst || !src || !table || !key)
		return 0;

	if (strlen(key) < 1)
		return 0;	// key can't be empty.

	unsigned int dw = 0u;
	unsigned int dwcounter = 0u;
	j = 0;

	for (i = 0; i < len; i++)
	{
		dw = ((static_cast<unsigned int>(src[i]) & 0xff) << ((i % 3) << 1)) | dw;

		dst[dwcounter++] = table[((dw & 0x3f) + 64 - key[j]) % 64];	// check!

		j++;
		if (!key[j])
			j = 0;

		dw = (dw >> 6);

		if (i % 3 == 2)
		{
			dst[dwcounter++] = table[((dw & 0x3f) + 64 - key[j]) % 64];	// check!
			j++;

			if (!key[j])
				j = 0;

			dw = 0;
		}
	}

	if (dw)
		dst[dwcounter++] = table[(dw + 64 - key[j]) % 64];	// check!

	dst[dwcounter] = '\0';

	return dwcounter;
}

// -------------------------------------------------------------------
// Decoding function of util_256to64_shl.
// 
// arg: dst=8-bit string;  src=6-bit string;  table=mapping table;
//      key=rotate key;
// ret: 0=failed  >0=bytes converted
int __fastcall Autil::util_shr_64to256(char* dst, char* src, char* table, char* key)
{
	unsigned int i, k;
	char* ptr = nullptr;

	if (!key || (strlen(key) < 1))
		return 0;	// must have key

	unsigned int dw = 0u;
	unsigned int dwcounter = 0u;
	unsigned int j = 0u;

	if (!dst || !src || !table)
		return 0;

	char c = '\0';
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

		if (!ptr)
			return 0;

		if (i % 4)
		{
			// check!
			dw = (((static_cast<unsigned int>(ptr - table) & 0x3f) + key[j]) % 64)
				<< ((4 - (i % 4)) << 1) | dw;
			j++;

			if (!key[j])
				j = 0;

			dst[dwcounter++] = dw & 0xff;
			dw = dw >> 8;
		}
		else
		{
			// check!
			dw = ((static_cast<unsigned int>(ptr - table) & 0x3f) + key[j]) % 64;
			j++;

			if (!key[j]) j = 0;
		}
	}

	if (dw)
		dst[dwcounter++] = dw & 0xff;

	dst[dwcounter] = '\0';

	return dwcounter;
}

// -------------------------------------------------------------------
// Swap a integer (4 byte).
// The value "rule" indicates the swaping rule.  It's a 4 byte string
// such as "1324" or "2431".
//
void __fastcall Autil::util_swapint(int* dst, int* src, char* rule)
{
	int i = 0;
	char* ptr = reinterpret_cast<char*>(src);
	char* qtr = reinterpret_cast<char*>(dst);
	for (i = 0; i < 4; i++)
		qtr[rule[i] - '1'] = ptr[i];
}

// -------------------------------------------------------------------
// Xor a string.  Be careful that your string contains '0xff'.  Your
// data may lose.
//
void __fastcall Autil::util_xorstring(char* dst, char* src)
{
	unsigned int i = 0;

	for (i = 0; i < strlen(src); i++)
		dst[i] = src[i] ^ 255;

	dst[i] = '\0';
}

// -------------------------------------------------------------------
// Shift the string right.
//
void __fastcall Autil::util_shrstring(char* dst, size_t dstlen, char* src, int offs)
{
	char* ptr = nullptr;
	int len = strlen(src);

	if (!dst || !src || (strlen(src) < 1))
		return;

	offs = strlen(src) - (offs % strlen(src));
	ptr = src + offs;
	strcpy_s(dst, dstlen, ptr);
	strncat_s(dst, dstlen, src, offs);
	dst[strlen(src)] = '\0';
}

// -------------------------------------------------------------------
// Shift the string left.
//
void __fastcall Autil::util_shlstring(char* dst, size_t dstlen, char* src, int offs)
{
	char* ptr = nullptr;
	if (!dst || !src || (strlen(src) < 1))
		return;

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
int __fastcall Autil::util_deint(int sliceno, int* value)
{
	int* t1 = nullptr;
	int t2 = 0;
	char t3[4096];	// This buffer is enough for an integer.
	memset(t3, 0, sizeof(t3));

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
int __fastcall Autil::util_mkint(char* buffer, int value)
{
	int t1 = 0, t2 = 0;
	char t3[4096];	// This buffer is enough for an integer.
	memset(t3, 0, sizeof(t3));

#ifdef _BACK_VERSION
	util_swapint(&t1, &value, "4312");
#else
	Autil::util_swapint(&t1, &value, const_cast<char*>("3142"));
#endif
	t2 = t1 ^ 0xffffffff;
	Autil::util_256to64_shr(t3, (char*)&t2, sizeof(int), const_cast<char*>(Autil::DEFAULTTABLE), PersonalKey);
	strcat_s(buffer, 16384, SEPARATOR);	// It's important to append a SEPARATOR between fields
	strcat_s(buffer, 16384, t3);

	return value;
}

// -------------------------------------------------------------------
// Convert a message slice into string.  Return a checksum.
//
// arg: sliceno=slice index in MesgSlice    value=result
// ret: checksum, this value must match the one generated by util_mkstring
int __fastcall Autil::util_destring(int sliceno, char* value)
{
	Autil::util_shr_64to256(value, MesgSlice[sliceno], const_cast<char*>(Autil::DEFAULTTABLE), PersonalKey);

	return strlen(value);
}

// -------------------------------------------------------------------
// Convert a string into buffer (a string).  Return a checksum.
//
// arg: buffer=output   value=data to pack
// ret: checksum, this value must match the one generated by util_destring
int __fastcall Autil::util_mkstring(char* buffer, char* value)
{
	char t1[Autil::SLICE_SIZE];
	memset(t1, 0, sizeof(t1));

	Autil::util_256to64_shl(t1, value, strlen(value), const_cast<char*>(Autil::DEFAULTTABLE), PersonalKey);
	strcat_s(buffer, 16384, SEPARATOR);	// It's important to append a SEPARATOR between fields
	strcat_s(buffer, 16384, t1);

	return strlen(value);
}