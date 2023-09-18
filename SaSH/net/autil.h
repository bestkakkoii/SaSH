// Arminius' protocol utilities ver 0.1

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

#pragma once
#include <util.h>
namespace Autil
{
	constexpr size_t NETDATASIZE = 16384;
	constexpr size_t NETBUFSIZ = 1024 * 64;
	constexpr size_t SLICE_MAX = 20;
	constexpr size_t SLICE_SIZE = 65500;
	constexpr size_t LBUFSIZE = 65500;
	constexpr size_t SBUFSIZE = 4096;
	extern QByteArray MesgSlice[];
	extern std::atomic_uint64_t SliceCount;//autil.cpp		// count slices in MesgSlice

	constexpr size_t PERSONALKEYSIZE = 32;
	extern util::SafeData<QString> PersonalKey;//autil.cpp

	constexpr const char* SEPARATOR = ";";

	constexpr const char* DEFAULTTABLE = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz{}";
	constexpr const char* DEFAULTFUNCBEGIN = "&";
	constexpr const char* DEFAULTFUNCEND = "#";

	void __stdcall util_Init(void);
	void __stdcall util_Release(void);
	void __stdcall util_Clear(void);
	bool __stdcall util_SplitMessage(char* source, size_t dstlen, char* separator);
	void __stdcall util_EncodeMessage(char* dst, size_t dstlen, char* src);
	void __stdcall util_DecodeMessage(char* dst, size_t dstlen, char* src);
	uint64_t __stdcall util_GetFunctionFromSlice(uint64_t* func, uint64_t* fieldcount);
	void __stdcall util_DiscardMessage(void);
	void __stdcall util_SendMesg(uint64_t func, char* buffer);

	// -------------------------------------------------------------------
	// Encoding function units.  Use in Encrypting functions.
	uint64_t __stdcall util_256to64(char* dst, char* src, size_t len, char* table);
	uint64_t __stdcall util_64to256(char* dst, char* src, char* table);
	uint64_t __stdcall util_256to64_shr(char* dst, char* src, size_t len, char* table, char* key);
	uint64_t __stdcall util_shl_64to256(char* dst, char* src, char* table, char* key);
	uint64_t __stdcall util_256to64_shl(char* dst, char* src, size_t len, char* table, char* key);
	uint64_t __stdcall util_shr_64to256(char* dst, char* src, char* table, char* key);

	void __stdcall util_swapint(int64_t* dst, int64_t* src, char* rule);
	void __stdcall util_xorstring(char* dst, char* src);
	void __stdcall util_shrstring(char* dst, size_t dstlen, char* src, int64_t offs);
	void __stdcall util_shlstring(char* dst, size_t dstlen, char* src, int64_t offs);
	// -------------------------------------------------------------------
	// Encrypting functions
	uint64_t __stdcall util_deint(uint64_t sliceno, int64_t* value);
	uint64_t __stdcall util_mkint(char* buffer, int64_t value);
	uint64_t __stdcall util_destring(uint64_t sliceno, char* value);
	uint64_t __stdcall util_mkstring(char* buffer, char* value);

	template<typename... Args>
	inline void util_Send(int func, Args... args)
	{
		uint64_t iChecksum = 0;
		char buffer[NETDATASIZE] = {};
		memset(buffer, 0, sizeof(buffer));

		// 编码参数并累加到 iChecksum
		auto encode_and_accumulate = [&iChecksum, &buffer](auto arg)
		{
			if constexpr (std::is_same_v<std::remove_reference_t<decltype(arg)>, int>)
			{
				iChecksum += util_mkint(buffer, arg);
			}
			else if constexpr (std::is_same_v<std::remove_reference_t<decltype(arg)>, std::string>)
			{
				iChecksum += util_mkstring(buffer, arg);
			}
			// 可以根据需要添加其他类型的处理
		};

		(encode_and_accumulate(args), ...);

		util_mkint(buffer, iChecksum);
		util_SendMesg(func, buffer);
	}

	void util_SendArgs(int func, std::vector<std::variant<int, std::string>>& args);

	template<typename... Args>
	inline bool util_Receive(Args*... args)
	{
		int64_t iChecksum = 0;  // 局部變量
		int64_t iChecksumrecv = 0;
		int64_t nextSlice = 2;

		// 解碼參數並累加到 iChecksum
		auto decode_and_accumulate = [&iChecksum, &nextSlice](auto* val)
		{
			if constexpr (std::is_same_v<std::remove_pointer_t<decltype(val)>, int>)
			{
				iChecksum += Autil::util_deint(nextSlice++, val);
			}
			else if constexpr (std::is_same_v<std::remove_pointer_t<decltype(val)>, char>)
			{
				iChecksum += Autil::util_destring(nextSlice++, val);
			}
		};
		(decode_and_accumulate(args), ...);

		Autil::util_deint(nextSlice, &iChecksumrecv);
		return (iChecksum == iChecksumrecv);
	}
}