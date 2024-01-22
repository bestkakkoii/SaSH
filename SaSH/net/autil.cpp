// Arminius' protocol utilities ver 0.1
//
/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2023 All Rights Reserved.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/

// -------------------------------------------------------------------
// The following definitions is to define game-dependent codes.
// Before compiling, remove the "//".

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include "autil.h"

#include <gamedevice.h>

Autil::Autil(long long index)
	: Indexer(index)
{
}

// -------------------------------------------------------------------
// Initialize utilities
//
void Autil::util_Init(void)
{
	QMutexLocker locker(&msgMutex_);
	msgSlice_.clear();
}

void Autil::util_Release(void)
{
	QMutexLocker locker(&msgMutex_);
	msgSlice_.clear();
}

void Autil::util_Clear(void)
{
	QMutexLocker locker(&msgMutex_);
	msgSlice_.clear();
}

// -------------------------------------------------------------------
// Discard a message from msgSlice_.
//
void Autil::util_DiscardMessage(void)
{
}

// -------------------------------------------------------------------
// Split up a message into slices by spearator.  Store those slices
// into a global buffer "char **msgSlice_"
//
// arg: source=message string;  separator=message separator (1 byte)
// ret: (none)
bool Autil::util_SplitMessage(const QByteArray& source, char separator)
{
	QMutexLocker locker(&msgMutex_);

	QByteArrayList list = source.split(separator);

	long long count = 0;
	for (QByteArray& slice : list)
	{
		msgSlice_.insert(count, std::move(slice));
		++count;
	}

	return count > 0;
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
	int rn = distribution(generator);// encode seed
	int t1 = 0, t2 = 0;
	char t3[LBUFSIZE] = {};
	char tz[LBUFSIZE] = {};

	if (!isBackVersion)
		util_swapint(&t1, &rn, const_cast<char*>("2413"));
	else
		util_swapint(&t1, &rn, const_cast<char*>("3421"));

	// t2 = t1 ^ 0x0f0f0f0f;
	t2 = t1 ^ 0xffffffff;
	util_256to64(tz, reinterpret_cast<char*>(&t2), sizeof(int), const_cast<char*>(DEFAULTTABLE));

	util_shlstring(t3, dstlen, src, rn);

	strcat_s(tz, LBUFSIZE, t3);
	util_xorstring(dst, tz);
}

// -------------------------------------------------------------------
// Decode the message
//
// arg: dst=output  src=input
// ret: (none)
void Autil::util_DecodeMessage(QByteArray& dst, QByteArray src)
{
	//  strcpy(dst, src);
	//  util_xorstring(dst, src);

	constexpr unsigned int INTCODESIZE = (sizeof(int) * 8 + 5) / 6;

	int rn = 0;
	int* t1 = nullptr;
	int t2 = 0;
	char t3[SBUFSIZE] = {};
	char t4[SBUFSIZE] = {}; // This buffer is enough for an integer.
	char tz[LBUFSIZE] = {};

	src.replace('\n', '\0');
	util_xorstring(tz, src.data());

	rn = INTCODESIZE;

	strncpy_s(t4, SBUFSIZE, tz, INTCODESIZE);
	t4[INTCODESIZE] = '\0';
	util_64to256(t3, t4, const_cast<char*>(DEFAULTTABLE));
	t1 = reinterpret_cast<int*>(t3);

	// t2 = *t1 ^ 0x0f0f0f0f;
	t2 = *t1 ^ 0xffffffff;

	if (!isBackVersion)
		util_swapint(&rn, &t2, const_cast<char*>("3142"));
	else
		util_swapint(&rn, &t2, const_cast<char*>("4312"));

	util_shrstring(dst, tz + INTCODESIZE, rn);
}

// -------------------------------------------------------------------
// Get a function information from msgSlice_.  A function is a complete
// and identifiable message received, beginned at DEFAULTFUNCBEGIN and
// ended at DEFAULTFUNCEND.  This routine will return the function ID
// (Action ID) and how many fields this function have.
//
// arg: func=return function ID    fieldcount=return fields of the function
// ret: 1=success  0=failed (function not complete)
long long Autil::util_GetFunctionFromSlice(long long* func, long long* fieldcount, long long offest)
{
	QMutexLocker locker(&msgMutex_);
	QByteArray t1(NETDATASIZE, '\0');

	// 從QHash中獲取第一个元素来替代msgSlice_[0]
	QByteArray firstItem = msgSlice_.value(0);

	if (firstItem != DEFAULTFUNCBEGIN)
		util_DiscardMessage();

	// 將第二个元素複製到t1中
	QByteArray secondItem = msgSlice_.value(1);

	*func = secondItem.toLongLong() - offest;

	// 在QHash中查找DEFAULTFUNCEND
	for (auto it = msgSlice_.constBegin(); it != msgSlice_.constEnd(); ++it)
	{
		if (it.value() == DEFAULTFUNCEND)
		{
			*fieldcount = it.key() - 2;	// - "&" - "#" - "func" 3 fields
			return 1;
		}
	}

	return 0;	// failed: message not complete
}

// -------------------------------------------------------------------
// Send a message
//
// arg: fd=socket fd   func=function ID   buffer=data to send
bool Autil::util_SendMesg(long long func, char* buffer)
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (!gamedevice.isValid())
		return false;

	char t1[NETDATASIZE] = {};
	char t2[NETDATASIZE] = {};

	constexpr int FUNCTION_OFFSET = 13;
	constexpr const char* FORMAT = "&;%d%s;#;";
	_snprintf_s(t1, NETDATASIZE, _TRUNCATE, FORMAT, static_cast<int>(func) + FUNCTION_OFFSET, buffer);

	util_EncodeMessage(t2, NETDATASIZE, t1);

	int size = static_cast<int>(strnlen_s(t2, sizeof(t2)));
	t2[size] = '\n';
	size += 1;

	HANDLE hProcess = gamedevice.getProcess();
	mem::VirtualMemory ptr(hProcess, size, true);

	mem::write(hProcess, ptr, t2, size);
	return gamedevice.sendMessage(kSendPacket, ptr, size) > 0;
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
	unsigned int dw = 0u, dwcounter = 0u;
	int i = 0;

	if (!dst || !src || !table)
		return 0;

	dw = 0;
	dwcounter = 0;

	for (i = 0; i < len; ++i)
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
int Autil::util_64to256(char* dst, char* src, char* table)
{
	unsigned int i = 0u, j = 0u;
	char* ptr = nullptr;

	unsigned int dw = 0u;
	unsigned int dwcounter = 0u;

	if (!dst || !src || !table)
		return 0;

	char c = '\0';
	for (i = 0; i < strlen(src); ++i)
	{
		c = src[i];
		for (j = 0; j < strlen(table); ++j)
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
int Autil::util_256to64_shr(char* dst, char* src, int len, char* table, char* key)
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
	for (i = 0; i < len; ++i)
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
int Autil::util_shl_64to256(char* dst, char* src, char* table, char* key)
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
	for (i = 0; i < strlen(src); ++i)
	{
		c = src[i];

		for (k = 0; k < strlen(table); ++k)
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
int Autil::util_256to64_shl(char* dst, char* src, int len, char* table, char* key)
{
	int i = 0, j = 0;

	if (!dst || !src || !table || !key)
		return 0;

	if (strlen(key) < 1)
		return 0;	// key can't be empty.

	unsigned int dw = 0u;
	unsigned int dwcounter = 0u;
	j = 0;

	for (i = 0; i < len; ++i)
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
int Autil::util_shr_64to256(char* dst, char* src, char* table, char* key)
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
	for (i = 0; i < strlen(src); ++i)
	{
		c = src[i];
		for (k = 0; k < strlen(table); ++k)
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
void Autil::util_swapint(int* dst, int* src, char* rule)
{
	int i = 0;
	char* ptr = reinterpret_cast<char*>(src);
	char* qtr = reinterpret_cast<char*>(dst);
	for (i = 0; i < 4; ++i)
		qtr[rule[i] - '1'] = ptr[i];
}

// -------------------------------------------------------------------
// Xor a string.  Be careful that your string contains '0xff'.  Your
// data may lose.
//
void Autil::util_xorstring(char* dst, char* src)
{
	unsigned int i = 0;

	for (i = 0; i < strlen(src); ++i)
		dst[i] = src[i] ^ 255;

	dst[i] = '\0';
}

// -------------------------------------------------------------------
// Shift the string right.
//
void Autil::util_shrstring(QByteArray& dst, char* src, int offs)
{
	char* ptr = nullptr;
	//int len = strlen(src);

	if (!src || (strlen(src) < 1))
		return;

	offs = static_cast<int>(strlen(src)) - (offs % strlen(src));
	ptr = src + offs;
	dst = ptr;
	dst.append(src);
}

// -------------------------------------------------------------------
// Shift the string left.
//
void Autil::util_shlstring(char* dst, size_t dstlen, char* src, int offs)
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
// arg: sliceno=slice index in msgSlice_    value=result
// ret: checksum, this value must match the one generated by util_mkint
long long Autil::util_deint(long long sliceno, long long* value)
{
	QMutexLocker locker(&msgMutex_);
	if (!msgSlice_.contains(sliceno))
		return 0;

	QByteArray slice = msgSlice_.value(sliceno);
	int* t1 = nullptr;
	int t2 = 0;
	char t3[SBUFSIZE] = {}; // This buffer is enough for an integer.

	std::string key(getKey());
	util_shl_64to256(t3, slice.data(), const_cast<char*>(DEFAULTTABLE), const_cast<char*>(key.c_str()));
	t1 = reinterpret_cast<int*>(t3);
	t2 = *t1 ^ 0xffffffff;
	int nvalue;
	if (!isBackVersion)
		util_swapint(&nvalue, &t2, const_cast<char*>("2413"));
	else
		util_swapint(&nvalue, &t2, const_cast<char*>("3421"));

	if (value != nullptr)
	{
		*value = nvalue;
		return nvalue;
	}

	return 0;
}

// -------------------------------------------------------------------
// Pack a integer into buffer (a string).  Return a checksum.
//
// arg: buffer=output   value=data to pack
// ret: checksum, this value must match the one generated by util_deint
long long Autil::util_mkint(char* buffer, long long value)
{
	int t1 = 0, t2 = 0;
	char t3[SBUFSIZE] = {}; // This buffer is enough for an integer.

	int nvalue = static_cast<int>(value);
	if (!isBackVersion)
		util_swapint(&t1, &nvalue, const_cast<char*>("3142"));
	else
		util_swapint(&t1, &nvalue, const_cast<char*>("4312"));

	t2 = t1 ^ 0xffffffff;
	std::string key(getKey());
	util_256to64_shr(t3, (char*)&t2, sizeof(int), const_cast<char*>(DEFAULTTABLE), const_cast<char*>(key.c_str()));
	strcat_s(buffer, NETDATASIZE, ";");	// It's important to append a SEPARATOR between fields
	strcat_s(buffer, NETDATASIZE, t3);

	return nvalue;
}

// -------------------------------------------------------------------
// Convert a message slice into string.  Return a checksum.
//
// arg: sliceno=slice index in msgSlice_    value=result
// ret: checksum, this value must match the one generated by util_mkstring
long long Autil::util_destring(long long sliceno, char* value)
{
	QMutexLocker locker(&msgMutex_);
	if (!msgSlice_.contains(sliceno))
		return 0;

	QByteArray slice = msgSlice_.value(sliceno);
	std::string key(getKey());
	util_shr_64to256(value, slice.data(), const_cast<char*>(DEFAULTTABLE), const_cast<char*>(key.c_str()));

	return static_cast<long long>(strlen(value));
}

// -------------------------------------------------------------------
// Convert a string into buffer (a string).  Return a checksum.
//
// arg: buffer=output   value=data to pack
// ret: checksum, this value must match the one generated by util_destring
long long Autil::util_mkstring(char* buffer, char* value)
{
	char t1[SLICE_SIZE] = {};

	std::string key(getKey());
	util_256to64_shl(t1, value, static_cast<int>(strlen(value)), const_cast<char*>(DEFAULTTABLE), const_cast<char*>(key.c_str()));
	strcat_s(buffer, NETDATASIZE, ";");	// It's important to append a SEPARATOR between fields
	strcat_s(buffer, NETDATASIZE, t1);

	return static_cast<long long>(strlen(value));
}