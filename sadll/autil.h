#pragma once
#if 0
// Arminius' protocol utilities ver 0.1
//
// Any questions and bugs, mailto: arminius@mail.hwaei.com.tw

#ifndef AUTIL_H
#define AUTIL_H
namespace Autil
{

	constexpr size_t NETBUFSIZ = 1024 * 64;
	constexpr size_t SLICE_MAX = 20;
	constexpr size_t SLICE_SIZE = 65500;
	extern char MesgSlice[][Autil::SLICE_SIZE];	//autil.cpp// store message slices
	extern int SliceCount;		//autil.cpp// count slices in MesgSlice

	extern char* PersonalKey;//autil.cpp

	constexpr const char* SEPARATOR = ";";

	constexpr const char* DEFAULTTABLE = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz{}";
	constexpr const char* DEFAULTFUNCBEGIN = "&";
	constexpr const char* DEFAULTFUNCEND = "#";

	void __fastcall util_Init();
	void __fastcall util_Release();
	bool __fastcall util_SplitMessage(char* source, size_t dstlen, char* separator);
	void __fastcall util_EncodeMessage(char* dst, size_t dstlen, char* src);
	void __fastcall util_DecodeMessage(char* dst, size_t dstlen, char* src);
	int __fastcall util_GetFunctionFromSlice(int* func, int* fieldcount);
	void __fastcall util_DiscardMessage(void);
	void __fastcall util_SendMesg(int fd, int func, char* buffer);

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
	void __fastcall util_shrstring(char* dst, size_t dstlen, char* src, int offs);
	void __fastcall util_shlstring(char* dst, size_t dstlen, char* src, int offs);
	// -------------------------------------------------------------------
	// Encrypting functions
	int __fastcall util_deint(int sliceno, int* value);
	int __fastcall util_mkint(char* buffer, int value);
	int __fastcall util_destring(int sliceno, char* value);
	int __fastcall util_mkstring(char* buffer, char* value);

	enum FUNCTIONTYPE
	{
		LSSPROTO_ERROR_RECV = -23,
		LSSPROTO_W_SEND = 0,
		LSSPROTO_W2_SEND = 1,
		LSSPROTO_XYD_RECV = 2,
		LSSPROTO_EV_SEND = 3,
		LSSPROTO_EV_RECV = 4,
		LSSPROTO_EN_SEND = 5,
		LSSPROTO_DU_SEND = 6,
		LSSPROTO_EN_RECV = 7,
		LSSPROTO_EO_SEND = 8,
		LSSPROTO_BU_SEND = 9,
		LSSPROTO_JB_SEND = 10,
		LSSPROTO_LB_SEND = 11,
		LSSPROTO_RS_RECV = 12,
		LSSPROTO_RD_RECV = 13,
		LSSPROTO_B_SEND = 14,
		LSSPROTO_B_RECV = 15,
		LSSPROTO_SKD_SEND = 16,
		LSSPROTO_ID_SEND = 17,
		LSSPROTO_PI_SEND = 18,
		LSSPROTO_DI_SEND = 19,
		LSSPROTO_DG_SEND = 20,
		LSSPROTO_DP_SEND = 21,
		LSSPROTO_I_RECV = 22,
		LSSPROTO_MI_SEND = 23,
		LSSPROTO_SI_RECV = 24,
		LSSPROTO_MSG_SEND = 25,
		LSSPROTO_MSG_RECV = 26,
		LSSPROTO_PMSG_SEND = 27,
		LSSPROTO_PME_RECV = 28,
		LSSPROTO_AB_SEND = 29,
		LSSPROTO_AB_RECV = 30,
		LSSPROTO_ABI_RECV = 31,
		LSSPROTO_DAB_SEND = 32,
		LSSPROTO_AAB_SEND = 33,
		LSSPROTO_L_SEND = 34,
		LSSPROTO_TK_SEND = 35,
		LSSPROTO_TK_RECV = 36,
		LSSPROTO_MC_RECV = 37,
		LSSPROTO_M_SEND = 38,
		LSSPROTO_M_RECV = 39,
		LSSPROTO_C_SEND = 40,
		LSSPROTO_C_RECV = 41,
		LSSPROTO_CA_RECV = 42,
		LSSPROTO_CD_RECV = 43,
		LSSPROTO_R_RECV = 44,
		LSSPROTO_S_SEND = 45,
		LSSPROTO_S_RECV = 46,
		LSSPROTO_D_RECV = 47,
		LSSPROTO_FS_SEND = 48,
		LSSPROTO_FS_RECV = 49,
		LSSPROTO_HL_SEND = 50,
		LSSPROTO_HL_RECV = 51,
		LSSPROTO_PR_SEND = 52,
		LSSPROTO_PR_RECV = 53,
		LSSPROTO_KS_SEND = 54,
		LSSPROTO_KS_RECV = 55,
		LSSPROTO_AC_SEND = 56,
		LSSPROTO_MU_SEND = 57,
		LSSPROTO_PS_SEND = 58,
		LSSPROTO_PS_RECV = 59,
		LSSPROTO_ST_SEND = 60,
		LSSPROTO_DT_SEND = 61,
		LSSPROTO_FT_SEND = 62,
		LSSPROTO_SKUP_RECV = 63,
		LSSPROTO_SKUP_SEND = 64,
		LSSPROTO_KN_SEND = 65,
		LSSPROTO_WN_RECV = 66,
		LSSPROTO_WN_SEND = 67,
		LSSPROTO_EF_RECV = 68,
		LSSPROTO_SE_RECV = 69,
		LSSPROTO_SP_SEND = 70,
		LSSPROTO_CLIENTLOGIN_SEND = 71,
		LSSPROTO_CLIENTLOGIN_RECV = 72,
		LSSPROTO_CREATENEWCHAR_SEND = 73,
		LSSPROTO_CREATENEWCHAR_RECV = 74,
		LSSPROTO_CHARDELETE_SEND = 75,
		LSSPROTO_CHARDELETE_RECV = 76,
		LSSPROTO_CHARLOGIN_SEND = 77,
		LSSPROTO_CHARLOGIN_RECV = 78,
		LSSPROTO_CHARLIST_SEND = 79,
		LSSPROTO_CHARLIST_RECV = 80,
		LSSPROTO_CHARLOGOUT_SEND = 81,
		LSSPROTO_CHARLOGOUT_RECV = 82,
		LSSPROTO_PROCGET_SEND = 83,
		LSSPROTO_PROCGET_RECV = 84,
		LSSPROTO_PLAYERNUMGET_SEND = 85,
		LSSPROTO_PLAYERNUMGET_RECV = 86,
		LSSPROTO_ECHO_SEND = 87,
		LSSPROTO_ECHO_RECV = 88,
		LSSPROTO_SHUTDOWN_SEND = 89,
		LSSPROTO_NU_RECV = 90,
		LSSPROTO_TD_SEND = 91,
		LSSPROTO_TD_RECV = 92,
		LSSPROTO_FM_RECV = 93,
		LSSPROTO_FM_SEND = 94,
		LSSPROTO_WO_RECV = 95,
		LSSPROTO_PETST_SEND = 96,
		LSSPROTO_BM_SEND = 97,   // _BLACK_MARKET,
#ifdef _MIND_ICON
		LSSPROTO_MA_SEND = 98,
#endif
#ifdef _FIX_DEL_MAP
		LSSPROTO_DM_SEND = 99   // 玩家抽地圖送監獄,
#endif
#ifdef _ITEM_CRACKER
		LSSPROTO_IC_RECV = 100,
#endif
#ifdef _MAGIC_NOCAST//沈默,
		LSSPROTO_NC_RECV = 101,
#endif
#ifdef _CHECK_GAMESPEED
		LSSPROTO_CS_SEND = 103,
		LSSPROTO_CS_RECV = 104,
#endif
#ifdef _TEAM_KICKPARTY
		LSSPROTO_KTEAM_SEND = 106,
#endif

#ifdef _PETS_SELECTCON
		LSSPROTO_PETST_RECV = 107,
#endif
#ifdef _NEWREQUESTPROTOCOL			// (不可開) Syu ADD 新增Protocol要求細項,
		LSSPROTO_RESIST_SEND = 108,
		LSSPROTO_RESIST_RECV = 109,
#endif
#ifdef _OUTOFBATTLESKILL			// (不可開) Syu ADD 非戰鬥時技能Protocol,
		LSSPROTO_BATTLESKILL_SEND = 110,
		LSSPROTO_BATTLESKILL_RECV = 111,
#endif
#ifdef _CHATROOMPROTOCOL			// (不可開) Syu ADD 聊天室頻道,
		LSSPROTO_CHATROOM_SEND = 112,
		LSSPROTO_CHATROOM_RECV = 113,
#endif
		LSSPROTO_SPET_SEND = 114,		// Robin 待機寵,
		LSSPROTO_SPET_RECV = 115,
#ifdef _STREET_VENDOR
		LSSPROTO_STREET_VENDOR_SEND = 116		// 擺攤功能,
		LSSPROTO_STREET_VENDOR_RECV = 117,
#endif
#ifdef _JOBDAILY
		LSSPROTO_JOBDAILY_SEND = 121,		// CYG　任務日志功能,
		LSSPROTO_JOBDAILY_RECV = 120,
#endif

#ifdef _TEACHER_SYSTEM
		LSSPROTO_TEACHER_SYSTEM_SEND = 122,		// 導師功能,
		LSSPROTO_TEACHER_SYSTEM_RECV = 123,
#endif

#ifdef _ADD_STATUS_2
		LSSPROTO_S2_SEND = 124,
		LSSPROTO_S2_RECV = 125,
#endif

#ifdef _ITEM_FIREWORK
		LSSPROTO_FIREWORK_RECV = 126,
#endif
#ifdef _PET_ITEM
		LSSPROTO_PET_ITEM_EQUIP_SEND = 127,
#endif
#ifdef _MOVE_SCREEN
		LSSPROTO_MOVE_SCREEN_RECV = 128,
#endif
#ifdef _GET_HOSTNAME
		LSSPROTO_HOSTNAME_SEND = 129,
		LSSPROTO_HOSTNAME_RECV = 130,
#endif
#ifdef _THEATER
		LSSPROTO_THEATER_DATA_RECV = 131,
		LSSPROTO_THEATER_DATA_SEND = 132,
#endif

#ifdef _NPC_MAGICCARD
		LSSPROTO_MAGICCARD_ACTION_RECV = 133,
		LSSPROTO_MAGICCARD_DAMAGE_RECV = 134,
#endif

#ifdef _SECONDARY_WINDOW_DATA_
		LSSPROTO_SECONDARY_WINDOW_RECV = 137,
#endif

#ifdef _ICONBUTTONS_
		LSSPROTO_TRUNTABLE_RECV = 138,
#endif

#ifdef _ALCHEPLUS
		LSSPROTO_ALCHEPLUS_SEND = 135,
		LSSPROTO_ALCHEPLUS_RECV = 136,
#endif

#ifdef _NPC_DANCE
		LSSPROTO_DANCEMAN_OPTION_RECV = 137,
#endif

#ifdef _HUNDRED_KILL
		LSSPROTO_HUNDREDKILL_RECV = 138,
#endif

#ifdef _PK2007
		LSSPROTO_PKLIST_SEND = 139,
		LSSPROTO_PKLIST_RECV = 140,
#endif

#ifdef _CHARSIGNDAY_
		LSSPROTO_SIGNDAY_SEND = 141,
#endif

		LSSPROTO_CHAREFFECT_RECV = 146,
#ifdef _RED_MEMOY_
		LSSPROTO_REDMEMOY_SEND = 147,
		LSSPROTO_REDMEMOY_RECV = 148,
#endif

#ifdef _ANNOUNCEMENT_
		LSSPROTO_DENGON_RECV = 200,
#endif
#ifdef _NEW_SYSTEM_MENU
		LSSPROTO_SAMENU_RECV = 201,
		LSSPROTO_SAMENU_SEND = 202,
#endif
#ifdef _NEWSHOP_
		LSSPROTO_SHOPOK_SEND = 208,
		LSSPROTO_SHOPOK_RECV = 209,
#endif
#ifdef _FAMILYBADGE_
		LSSPROTO_FAMILYBADGE_SEND = 210,
		LSSPROTO_FAMILYBADGE_RECV = 211,
#endif

#ifdef _CHARTITLE_
		LSSPROTO_CHARTITLE_SEND = 212,
		LSSPROTO_CHARTITLE_RECV = 213,
#endif

#ifdef _CHARTITLE_STR_
		LSSPROTO_CHARTITLE_SEND = 212,
		LSSPROTO_CHARTITLE_RECV = 213,
#endif

#ifdef _PETBLESS_
		LSSPROTO_VB_SEND = 218,
		LSSPROTO_VB_RECV = 219,
#endif

#ifdef _RIDEQUERY_//騎寵查詢,
		LSSPROTO_RIDEQUERY_SEND = 220,
#endif

#ifdef _PET_SKINS
		LSSPROTO_PETSKINS_SEND = 221,
		LSSPROTO_PETSKINS_RECV = 222,
#endif
		LSSPROTO_JOINTEAM_SEND = 239,
		LSSPROTO_ARRAGEITEM_SEND = 260,
	};

}

#endif


#endif