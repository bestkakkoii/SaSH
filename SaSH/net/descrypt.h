#pragma once
/*
 * DES encryption/decryption library interface
 *
 *	Written by Koichiro Mori (kmori@lsi-j.co.jp)
 */

namespace sacrypt
{
	constexpr auto YES = 1;
	constexpr auto NO = 0;
	constexpr auto DES_DIRMASK = 001;
	constexpr auto DES_ENCRYPT = (0 * DES_DIRMASK);
	constexpr auto DES_DECRYPT = (1 * DES_DIRMASK);
	constexpr auto DES_DEVMASK = 002;
	constexpr auto DES_HW = (0 * DES_DEVMASK);
	constexpr auto DES_SW = (1 * DES_DEVMASK);

	constexpr auto DESERR_NONE = 0;
	constexpr auto DESERR_NOHWDEVICE = 1;
	constexpr auto DESERR_HWERROR = 2;
	constexpr auto DESERR_BADPARAM = 3;

	inline constexpr auto DES_FAILED(long err) { return ((err) > DESERR_NOHWDEVICE); }

	int ecb_crypt(const char* key, char* buf, unsigned len, unsigned mode);
	int cbc_crypt(const char* key, char* buf, unsigned len, unsigned mode, char* ivec);
	void des_setparity(char* key);

	/* Not defined by Sun Microsystems - internally used by desbench program */
	void des_setkey(const char* key);
	void des_crypt(char* buf, int dflag);
}
/*
#else

 int ecb_crypt();
 int cbc_crypt();
 void des_setparity();

// Not defined by Sun Microsystems - internally used by desbench program
 void des_setkey();
 void des_crypt();

#endif // __STDC__ || LSI_C
*/