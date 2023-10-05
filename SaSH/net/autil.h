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
#include <indexer.h>

constexpr size_t NETDATASIZE = 16384;
constexpr size_t NETBUFSIZ = 1024 * 64;
constexpr size_t SLICE_MAX = 20;
constexpr size_t SLICE_SIZE = 65500;
constexpr size_t LBUFSIZE = 65500;
constexpr size_t SBUFSIZE = 4096;
extern QByteArray MesgSlice[];//autil.cpp//[][Autil::SLICE_SIZE];	// store message slices
extern util::SafeData<size_t> SliceCount;//autil.cpp		// count slices in MesgSlice
constexpr size_t PERSONALKEYSIZE = 32;
//extern QScopedArrayPointer<char> PersonalKey;
extern util::SafeData<QString> PersonalKey;//autil.cpp

constexpr char SEPARATOR = ';';

constexpr const char* DEFAULTTABLE = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz{}";
constexpr const char* DEFAULTFUNCBEGIN = "&";
constexpr const char* DEFAULTFUNCEND = "#";

class Autil : public Indexer
{
public:
	explicit Autil(qint64 index);

	void __fastcall util_Init(void);
	void __fastcall util_Release(void);
	void __fastcall util_Clear(void);
	bool __fastcall util_SplitMessage(const QByteArray& source, char separator);
	void __fastcall util_EncodeMessage(char* dst, size_t dstlen, char* src);
	void __fastcall util_DecodeMessage(QByteArray& dst, QByteArray src);
	qint64 __fastcall util_GetFunctionFromSlice(qint64* func, qint64* fieldcount, qint64 offest = 23);
	void __fastcall util_DiscardMessage(void);
	void __fastcall util_SendMesg(int func, char* buffer);

	// -------------------------------------------------------------------
	// Encoding function units.  Use in Encrypting functions.
	int __fastcall util_256to64(char* dst, char* src, int len, char* table);
	int __fastcall util_64to256(char* dst, char* src, char* table);
	int __fastcall util_256to64_shr(char* dst, char* src, int len, char* table, char* key);
	int __fastcall util_shl_64to256(char* dst, char* src, char* table, char* key);
	int __fastcall util_256to64_shl(char* dst, char* src, int len, char* table, char* key);
	int __fastcall util_shr_64to256(char* dst, char* src, char* table, char* key);

	void __fastcall util_swapint(int* dst, int* src, char* rule);
	void __fastcall util_xorstring(char* dst, char* src);
	void __fastcall util_shrstring(QByteArray& dst, char* src, int offs);
	void __fastcall util_shlstring(char* dst, size_t dstlen, char* src, int offs);
	// -------------------------------------------------------------------
	// Encrypting functions
	int __fastcall util_deint(int sliceno, int* value);
	int __fastcall util_mkint(char* buffer, int value);
	int __fastcall util_destring(int sliceno, char* value);
	int __fastcall util_mkstring(char* buffer, char* value);

	// 輔助函數，處理整數參數
	template<typename Arg>
	inline void __fastcall util_SendProcessArg(int& sum, char* buffer, Arg arg)
	{
		sum += util_mkint(buffer, arg);
	}

	// 輔助函數，處理字符串參數（重載版本）
	inline void __fastcall util_SendProcessArg(int& sum, char* buffer, char* arg)
	{
		sum += util_mkstring(buffer, arg);
	}

	// 輔助函數，遞歸處理參數
	template<typename Arg, typename... Args>
	inline void __fastcall util_SendProcessArgs(int& sum, char* buffer, Arg arg, Args... args)
	{
		util_SendProcessArg(sum, buffer, arg);
		util_SendProcessArgs(sum, buffer, args...);
	}

	// 輔助函數，處理最後一個參數
	template<typename Arg>
	inline void __fastcall util_SendProcessArgs(int& sum, char* buffer, Arg arg)
	{
		util_SendProcessArg(sum, buffer, arg);
	}

	// 主發送函數
	template<typename... Args>
	inline void __fastcall util_Send(int func, Args... args)
	{
		int iChecksum = 0;
		std::unique_ptr <char[]> buffer(new char[NETDATASIZE]);
		std::fill(buffer.get(), buffer.get() + NETDATASIZE, 0);

		util_SendProcessArgs(iChecksum, buffer.get(), args...);
		util_mkint(buffer.get(), iChecksum);
		util_SendMesg(func, buffer.get());
	}

	inline void __fastcall util_SendArgs(int func, std::vector<std::variant<int, std::string>>& args)
	{
		int iChecksum = 0;
		std::unique_ptr <char[]> buffer(new char[NETDATASIZE]());
		std::fill(buffer.get(), buffer.get() + NETDATASIZE, 0);

		for (const std::variant<int, std::string>& arg : args)
		{
			if (std::holds_alternative<int>(arg))
			{
				iChecksum += util_mkint(buffer.get(), std::get<int>(arg));
			}
			else if (std::holds_alternative<std::string>(arg))
			{
				iChecksum += util_mkstring(buffer.get(), const_cast<char*>(std::get<std::string>(arg).c_str()));
			}
		}

		util_mkint(buffer.get(), iChecksum);
		util_SendMesg(func, buffer.get());
	}

	template<typename... Args>
	inline bool __fastcall util_Receive(Args*... args)
	{
		int iChecksum = 0;  // 局部變量
		int iChecksumrecv = 0;
		int nextSlice = 2;

		// 解碼參數並累加到 iChecksum
		auto decode_and_accumulate = [this, &iChecksum, &nextSlice](auto* val)
		{
			if constexpr (std::is_same_v<std::remove_pointer_t<decltype(val)>, int>)
			{
				iChecksum += util_deint(nextSlice++, val);
			}
			else if constexpr (std::is_same_v<std::remove_pointer_t<decltype(val)>, char>)
			{
				iChecksum += util_destring(nextSlice++, val);
			}
		};
		(decode_and_accumulate(args), ...);

		// 獲取並校驗 iChecksum
		util_deint(nextSlice, &iChecksumrecv);
		return (iChecksum == iChecksumrecv);
	}

public:
	util::SafeData<size_t> SliceCount;
	util::SafeData<QString> PersonalKey;


private:
	QHash<qint64, QByteArray> msgSlice_ = {};
	QMutex msgMutex_;
};