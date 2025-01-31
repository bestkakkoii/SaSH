// Arminius' protocol utilities ver 0.1

/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2024 All Rights Reserved.
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

constexpr size_t PERSONALKEYSIZE = 32;

constexpr char SEPARATOR = ';';

constexpr const char* DEFAULTTABLE = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz{}";
constexpr const char* DEFAULTFUNCBEGIN = "&";
constexpr const char* DEFAULTFUNCEND = "#";

class Autil : public Indexer
{
public:
	explicit Autil(long long index);

	void __fastcall util_Init(void);
	void __fastcall util_Release(void);
	void __fastcall util_Clear(void);
	bool __fastcall util_SplitMessage(const QByteArray& source, char separator);
	bool __fastcall util_SplitMessage(const QByteArray& source, char separator, QHash<long long, QByteArray>& dst) const;
	void __fastcall util_EncodeMessage(char* dst, size_t dstlen, char* src);
	void __fastcall util_DecodeMessage(QByteArray& dst, QByteArray src);
	long long __fastcall util_GetFunctionFromSlice(long long* func, long long* fieldcount, long long offest = 23);
	long long __fastcall util_GetFunctionFromSlice(const QHash<long long, QByteArray>& slices, long long* func, long long* fieldcount, long long offest = 23) const;
	void __fastcall util_DiscardMessage(void);
	bool __fastcall util_SendMesg(long long func, char* buffer);

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
	bool __fastcall util_deint(long long sliceno, long long* value, long long* sum);
	bool __fastcall util_deint(const QHash< long long, QByteArray >& slices, long long sliceno, long long* value, long long* sum);
	long long __fastcall util_mkint(char* buffer, long long value);
	bool __fastcall util_destring(long long sliceno, char* value, long long* sum);
	bool __fastcall util_destring(const QHash< long long, QByteArray >& slices, long long sliceno, char* value, long long* sum);
	long long __fastcall util_mkstring(char* buffer, char* value);

	// 輔助函數，處理整數參數
	template<typename Arg>
	inline void __fastcall util_SendProcessArg(long long& sum, char* buffer, Arg arg)
	{
		sum += util_mkint(buffer, static_cast<int>(arg));
	}

	// 輔助函數，處理字符串參數（重載版本）
	inline void __fastcall util_SendProcessArg(long long& sum, char* buffer, char* arg)
	{
		sum += util_mkstring(buffer, arg);
	}

	// 輔助函數，遞歸處理參數
	template<typename Arg, typename... Args>
	inline void __fastcall util_SendProcessArgs(long long& sum, char* buffer, Arg arg, Args... args)
	{
		util_SendProcessArg(sum, buffer, arg);
		util_SendProcessArgs(sum, buffer, args...);
	}

	// 輔助函數，處理最後一個參數
	template<typename Arg>
	inline void __fastcall util_SendProcessArgs(long long& sum, char* buffer, Arg arg)
	{
		util_SendProcessArg(sum, buffer, arg);
	}

	// 主發送函數
	template<typename... Args>
	inline bool __fastcall util_Send(long long func, Args... args)
	{
		long long iChecksum = 0;
		char buffer[NETDATASIZE] = {};

		util_SendProcessArgs(iChecksum, buffer, args...);
		util_mkint(buffer, iChecksum);
		return util_SendMesg(func, buffer);
	}

	inline bool __fastcall util_SendArgs(long long func, std::vector<std::variant<long long, std::string>>& args)
	{
		long long iChecksum = 0;
		char buffer[NETDATASIZE] = {};

		for (const std::variant<long long, std::string>& arg : args)
		{
			if (std::holds_alternative<long long>(arg))
			{
				iChecksum += util_mkint(buffer, std::get<long long>(arg));
			}
			else if (std::holds_alternative<std::string>(arg))
			{
				iChecksum += util_mkstring(buffer, const_cast<char*>(std::get<std::string>(arg).c_str()));
			}
		}

		util_mkint(buffer, iChecksum);
		return util_SendMesg(func, buffer);
	}

	template<typename... Args>
	inline bool __fastcall util_Receive(Args*... args)
	{
		long long iChecksum = 0;  // 局部變量
		long long iChecksumrecv = 0;
		long long nextSlice = 2;

		// 解碼參數並累加到 iChecksum
		auto decode_and_accumulate = [this, &iChecksum, &nextSlice](auto* val)
			{
				if constexpr (std::is_same_v<std::remove_pointer_t<decltype(val)>, long long>)
				{
					long long checksum = 0;
					util_deint(nextSlice++, val, &checksum);
					iChecksum += checksum;
				}
				else if constexpr (std::is_same_v<std::remove_pointer_t<decltype(val)>, char>)
				{
					long long checksum = 0;
					util_destring(nextSlice++, val, &checksum);
					iChecksum += checksum;
				}
			};
		(decode_and_accumulate(args), ...);

		// 獲取並校驗 iChecksum
		long long checksum = 0;
		return util_deint(nextSlice, &iChecksumrecv, &checksum) && (iChecksum == iChecksumrecv);
	}

	template<typename... Args>
	inline bool __fastcall util_Receive(const QHash< long long, QByteArray >& slices, Args*... args)
	{
		long long iChecksum = 0;  // 局部變量
		long long iChecksumrecv = 0;
		long long nextSlice = 2;

		// 解碼參數並累加到 iChecksum
		auto decode_and_accumulate = [this, &slices, &iChecksum, &nextSlice](auto* val)
			{
				if constexpr (std::is_same_v<std::remove_pointer_t<decltype(val)>, long long>)
				{
					long long checksum = 0;
					util_deint(slices, nextSlice++, val, &checksum);
					iChecksum += checksum;
				}
				else if constexpr (std::is_same_v<std::remove_pointer_t<decltype(val)>, char>)
				{
					long long checksum = 0;
					util_destring(slices, nextSlice++, val, &checksum);
					iChecksum += checksum;
				}
			};
		(decode_and_accumulate(args), ...);

		// 獲取並校驗 iChecksum
		long long checksum = 0;
		return util_deint(slices, nextSlice, &iChecksumrecv, &checksum) && (iChecksum == iChecksumrecv);
	}

	inline bool __fastcall setKey(const std::string& key)
	{
		std::unique_lock<std::shared_mutex> lock(keyMutex_);
		if (strncmp(personalKey_, key.c_str(), key.size()) == 0)
			return false;

		memset(personalKey_, 0, sizeof(personalKey_));
		_snprintf_s(personalKey_, sizeof(personalKey_), _TRUNCATE, "%s", key.c_str());
		return true;
	}

	[[nodiscard]] inline std::string __fastcall getKey() const
	{
		std::shared_lock<std::shared_mutex> lock(keyMutex_);
		return personalKey_;
	}

public:
	enum class ParamType
	{
		Integer,
		String
	};

	struct PacketParameter
	{
		QString value;
		ParamType type;
		long long checksum;

		PacketParameter()
			: type(ParamType::Integer)
			, checksum(0)
		{
		}

		PacketParameter(const QString& v, ParamType t, long long cs)
			: value(v)
			, type(t)
			, checksum(cs)
		{
		}
	};

	bool tryParamCombination(
		const QHash<long long, QByteArray>& slices,
		QVector<ParamType>& types,
		QVector<PacketParameter>& outParams,
		long long startIndex,
		long long count);

	QVector<Autil::PacketParameter> util_AutoDetectParameters(
		const QHash<long long, QByteArray>& slices,
		long long fieldcount);

	static QString util_FormatParam(const PacketParameter& value);


private:
	mutable std::shared_mutex keyMutex_;
	char personalKey_[1024] = {};



private:
	QHash<long long, QByteArray> msgSlice_ = {};
	QMutex msgMutex_;
	bool isBackVersion = false;
};