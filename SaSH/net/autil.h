// Arminius' protocol utilities ver 0.1
//
// Any questions and bugs, mailto: arminius@mail.hwaei.com.tw

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
	extern QByteArray MesgSlice[];//[][Autil::SLICE_SIZE];	// store message slices
	extern util::SafeData<size_t> SliceCount;		// count slices in MesgSlice

	constexpr size_t PERSONALKEYSIZE = 32;
	//extern QScopedArrayPointer<char> PersonalKey;
	extern util::SafeData<QString> PersonalKey;

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
	int __stdcall util_GetFunctionFromSlice(int* func, int* fieldcount);
	void __stdcall util_DiscardMessage(void);
	void __stdcall util_SendMesg(int func, char* buffer);

	// -------------------------------------------------------------------
	// Encoding function units.  Use in Encrypting functions.
	int __stdcall util_256to64(char* dst, char* src, int len, char* table);
	int __stdcall util_64to256(char* dst, char* src, char* table);
	int __stdcall util_256to64_shr(char* dst, char* src, int len, char* table, char* key);
	int __stdcall util_shl_64to256(char* dst, char* src, char* table, char* key);
	int __stdcall util_256to64_shl(char* dst, char* src, int len, char* table, char* key);
	int __stdcall util_shr_64to256(char* dst, char* src, char* table, char* key);

	void __stdcall util_swapint(int* dst, int* src, char* rule);
	void __stdcall util_xorstring(char* dst, char* src);
	void __stdcall util_shrstring(char* dst, size_t dstlen, char* src, int offs);
	void __stdcall util_shlstring(char* dst, size_t dstlen, char* src, int offs);
	// -------------------------------------------------------------------
	// Encrypting functions
	int __stdcall util_deint(int sliceno, int* value);
	int __stdcall util_mkint(char* buffer, int value);
	int __stdcall util_destring(int sliceno, char* value);
	int __stdcall util_mkstring(char* buffer, char* value);

}