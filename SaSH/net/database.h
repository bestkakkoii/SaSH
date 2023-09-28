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

#include "util.h"

#pragma region Const

constexpr size_t LINEBUFSIZ = 8192u;

#ifdef _MORECHARACTERS_
constexpr qint64 MAX_CHARACTER = 4;
#else
constexpr qint64 MAX_CHARACTER = 2;
#endif
constexpr qint64 CHAR_NAME_LEN = 16;
constexpr qint64 CHAR_FREENAME_LEN = 32;
constexpr qint64 MAGIC_NAME_LEN = 28;
constexpr qint64 MAGIC_MEMO_LEN = 72;
constexpr qint64 ITEM_NAME_LEN = 28;
constexpr qint64 ITEM_NAME2_LEN = 16;
constexpr qint64 ITEM_MEMO_LEN = 84;
constexpr qint64 PET_NAME_LEN = 16;
constexpr qint64 PET_FREENAME_LEN = 32;
constexpr qint64 CHAR_FMNAME_LEN = 33;     // 家族名稱

#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
constexpr qint64 PROFESSION_MEMO_LEN = 84;
#endif

#ifdef _GM_IDENTIFY		// Rog ADD GM識別
constexpr qint64 GM_NAME_LEN = 32;
#endif

constexpr qint64 CHARNAMELEN = 256;

constexpr qint64 MAX_PET = 5;

constexpr qint64 MAX_MAGIC = 9;

constexpr qint64 MAX_PETSKILL = 7;

constexpr qint64 MAX_PARTY = 5;

constexpr qint64 MAX_ADR_BOOK_COUNT = 4;
#ifdef _EXTEND_AB
constexpr qint64 MAX_ADR_BOOK_PAGE = 20;//20  //10   20050214 cyg 10 add to 20
#else
constexpr qint64 MAX_ADR_BOOK_PAGE = 10;
#endif
constexpr qint64 MAX_ADDRESS_BOOK = (MAX_ADR_BOOK_COUNT * MAX_ADR_BOOK_PAGE);

#ifdef _PRO3_ADDSKILL
constexpr qint64 MAX_PROFESSION_SKILL = 30;
#else
constexpr qint64 MAX_PROFESSION_SKILL = 26;
#endif

constexpr qint64 BATTLE_BUF_SIZE = 4;
constexpr qint64 BATTLE_COMMAND_SIZE = 4096;

constexpr qint64 FLOOR_NAME_LEN = 24;

constexpr qint64 RESULT_ITEM_COUNT = 3;
constexpr qint64 RESULT_ITEM_NAME_LEN = 24;
//constexpr qint64 RESULT_CHR_EXP = 4;
constexpr qint64 RESULT_CHR_EXP = 5;

constexpr qint64 SKILL_NAME_LEN = 24;
constexpr qint64 SKILL_MEMO_LEN = 72;
constexpr qint64 MAX_SKILL = 7;

constexpr qint64 MIN_HP_PERCENT = 10;

constexpr qint64 MAX_GOLD = 1000000;
constexpr qint64 MAX_BANKGOLD = 10000000;
constexpr qint64 MAX_FMBANKGOLD = 100000000;


constexpr qint64 MAX_PERSONAL_BANKGOLD = 50000000;

#ifdef _FMVER21
constexpr qint64 FAMILY_MAXMEMBER = 100;    // 家族人數
#else
constexpr qint64 FAMILY_MAXMEMBER = 50;    // 家族人數
#endif

constexpr qint64 MAP_TILE_GRID_X1 = -20;
constexpr qint64 MAP_TILE_GRID_X2 = +17;
constexpr qint64 MAP_TILE_GRID_Y1 = -16;
constexpr qint64 MAP_TILE_GRID_Y2 = +21;
constexpr qint64 MAP_X_SIZE = (MAP_TILE_GRID_X2 - MAP_TILE_GRID_X1);
constexpr qint64 MAP_Y_SIZE = (MAP_TILE_GRID_Y2 - MAP_TILE_GRID_Y1);

constexpr qint64 MAP_READ_FLAG = 0x8000;
constexpr qint64  MAP_SEE_FLAG = 0x4000;

constexpr qint64 BC_FLG_NEW = (1LL << 0);
constexpr qint64 BC_FLG_DEAD = (1LL << 1);	  //死亡
constexpr qint64 BC_FLG_PLAYER = (1LL << 2);	  //玩家,玩家有异常状态时要有此值
constexpr qint64 BC_FLG_POISON = (1LL << 3);	  //中毒
constexpr qint64 BC_FLG_PARALYSIS = (1LL << 4); //麻痹
constexpr qint64 BC_FLG_SLEEP = (1LL << 5);  //昏睡
constexpr qint64 BC_FLG_STONE = (1LL << 6);	  //石化
constexpr qint64 BC_FLG_DRUNK = (1LL << 7);  //酒醉
constexpr qint64 BC_FLG_CONFUSION = (1LL << 8); //混乱
constexpr qint64 BC_FLG_HIDE = (1LL << 9);	  //是否隐藏，地球一周
constexpr qint64 BC_FLG_REVERSE = (1LL << 10);  //反转
constexpr qint64 BC_FLG_WEAKEN = (1LL << 11);  //虚弱
constexpr qint64 BC_FLG_DEEPPOISON = (1LL << 12);  //劇毒
constexpr qint64 BC_FLG_BARRIER = (1LL << 13);  //魔障
constexpr qint64 BC_FLG_NOCAST = (1LL << 14);  //沉默
constexpr qint64 BC_FLG_SARS = (1LL << 15);  //毒煞
constexpr qint64 BC_FLG_DIZZY = (1LL << 16);  //眩暈
constexpr qint64 BC_FLG_ENTWINE = (1LL << 17);  //树根缠绕
constexpr qint64 BC_FLG_DRAGNET = (1LL << 18);  //天罗地网
constexpr qint64 BC_FLG_ICECRACK = (1LL << 19);  //冰爆术
constexpr qint64 BC_FLG_OBLIVION = (1LL << 20);  //遗忘
constexpr qint64 BC_FLG_ICEARROW = (1LL << 21);  //冰箭
constexpr qint64 BC_FLG_BLOODWORMS = (1LL << 22);  //嗜血蛊
constexpr qint64 BC_FLG_SIGN = (1LL << 23);  //一针见血
constexpr qint64 BC_FLG_CARY = (1LL << 24);  //挑拨
constexpr qint64 BC_FLG_F_ENCLOSE = (1LL << 25);  //火附体
constexpr qint64 BC_FLG_I_ENCLOSE = (1LL << 26);  //冰附体
constexpr qint64 BC_FLG_T_ENCLOSE = (1LL << 27);  //雷附体
constexpr qint64 BC_FLG_WATER = (1LL << 28);  //水附体
constexpr qint64 BC_FLG_FEAR = (1LL << 29);  //恐惧
constexpr qint64 BC_FLG_CHANGE = (1LL << 30);  //雷尔变身


inline constexpr bool hasBadStatus(quint64 status)
{
	//BC_FLG_POISON | BC_FLG_PARALYSIS | BC_FLG_SLEEP | BC_FLG_STONE | BC_FLG_DRUNK | BC_FLG_CONFUSION;
	if (status & BC_FLG_POISON)
		return true;
	if (status & BC_FLG_PARALYSIS)
		return true;
	if (status & BC_FLG_SLEEP)
		return true;
	if (status & BC_FLG_STONE)
		return true;
	if (status & BC_FLG_DRUNK)
		return true;
	if (status & BC_FLG_CONFUSION)
		return true;

	if (status & BC_FLG_WEAKEN)
		return true;
	if (status & BC_FLG_DEEPPOISON)
		return true;
	if (status & BC_FLG_BARRIER)
		return true;
	if (status & BC_FLG_NOCAST)
		return true;
	if (status & BC_FLG_SARS)
		return true;
	if (status & BC_FLG_DIZZY)
		return true;
	if (status & BC_FLG_ENTWINE)
		return true;
	if (status & BC_FLG_DRAGNET)
		return true;
	if (status & BC_FLG_ICECRACK)
		return true;
	if (status & BC_FLG_OBLIVION)
		return true;
	if (status & BC_FLG_ICEARROW)
		return true;
	if (status & BC_FLG_BLOODWORMS)
		return true;
	if (status & BC_FLG_SIGN)
		return true;
	if (status & BC_FLG_CARY)
		return true;
	if (status & BC_FLG_F_ENCLOSE)
		return true;
	if (status & BC_FLG_I_ENCLOSE)
		return true;
	if (status & BC_FLG_T_ENCLOSE)
		return true;
	if (status & BC_FLG_WATER)
		return true;
	if (status & BC_FLG_FEAR)
		return true;
	if (status & BC_FLG_CHANGE)
		return true;

	return false;
}

inline constexpr bool hasUnMoveableStatue(quint64 status)
{
	if (status & BC_FLG_DEAD)
		return true;
	if (status & BC_FLG_STONE)
		return true;
	if (status & BC_FLG_SLEEP)
		return true;
	if (status & BC_FLG_PARALYSIS)
		return true;
	if (status & BC_FLG_HIDE)
		return true;
	if (status & BC_FLG_DIZZY)
		return true;

	return false;
}

constexpr qint64 ITEM_FLAG_PET_MAIL = (1LL << 0);
constexpr qint64 ITEM_FLAG_MIX = (1LL << 1);
constexpr qint64 ITEM_FLAG_COOKING_MIX = (1LL << 2);
constexpr qint64 ITEM_FLAG_METAL_MIX = (1LL << 3);	//金屬
constexpr qint64 ITEM_FLAG_JEWEL_MIX = (1LL << 4);	//寶石
constexpr qint64 ITEM_FLAG_FIX_MIX = (1LL << 5);	//修理
#ifdef _ITEM_INTENSIFY
constexpr qint64 ITEM_FLAG_INTENSIFY_MIX = (1LL << 6);	//強化
#endif
#ifdef _ITEM_UPINSLAY
constexpr qint64 ITEM_FLAG_UPINSLAY_MIX = (1LL << 7);	//鑿孔
#endif

constexpr qint64 JOY_RIGHT = (1LL << 15);/* Right Key				*/
constexpr qint64 JOY_LEFT = (1LL << 14);/*  Left Key				*/
constexpr qint64 JOY_DOWN = (1LL << 13);/*  Down Key				*/
constexpr qint64 JOY_UP = (1LL << 12);	/*    Up Key				*/
constexpr qint64 JOY_START = (1LL << 11);	/* Start					*/
constexpr qint64 JOY_A = (1LL << 10);	/* A Trigger				*/
constexpr qint64 JOY_C = (1LL << 9);/* C Trigger				*/
constexpr qint64 JOY_B = (1LL << 8);	/* B Trigger				*/
constexpr qint64 JOY_R = (1LL << 7);	/* R Trigger				*/
constexpr qint64 JOY_X = (1LL << 6);	/* X Trigger				*/
constexpr qint64 JOY_DEL = (1LL << 5);	/* DELETE					*/
constexpr qint64 JOY_INS = (1LL << 4);	/* INSERT					*/
constexpr qint64 JOY_END = (1LL << 3);/* END						*/
constexpr qint64 JOY_HOME = (1LL << 2);	/* HOME						*/
constexpr qint64 JOY_P_DOWN = (1LL << 1);	/* PAGE_UP					*/
constexpr qint64 JOY_P_UP = (1LL << 0);	/* PAGE_DOWN				*/

constexpr qint64 JOY_ESC = (1LL << 31);/* ESC Key					*/
constexpr qint64 JOY_CTRL_M = (1LL << 30);	/* Ctrl + M					*/
constexpr qint64 JOY_CTRL_S = (1LL << 29);	/* Ctrl + S					*/
constexpr qint64 JOY_CTRL_P = (1LL << 28);	/* Ctrl + P					*/
constexpr qint64 JOY_CTRL_I = (1LL << 27);	/* Ctrl + I					*/
constexpr qint64 JOY_CTRL_E = (1LL << 26);	/* Ctrl + E					*/
constexpr qint64 JOY_CTRL_A = (1LL << 25);	/* Ctrl + A					*/

constexpr qint64 JOY_CTRL_C = (1LL << 24);	/* Ctrl + C					*/
constexpr qint64 JOY_CTRL_V = (1LL << 23);	/* Ctrl + V					*/
constexpr qint64 JOY_CTRL_T = (1LL << 22);	/* Ctrl + T					*/

constexpr qint64 MAIL_STR_LEN = 140;
constexpr qint64 MAIL_DATE_STR_LEN = 20;
constexpr qint64 MAIL_MAX_HISTORY = 20;
constexpr qint64 MAX_CHAT_REGISTY_STR = 8;

constexpr qint64 MAX_ENEMY = 20;
constexpr qint64 MAX_CHAT_HISTORY = 20;
constexpr qint64 MAX_DIALOG_LINE = 200;

constexpr qint64 MAX_DIR = 8;

constexpr const char* SUCCESSFULSTR = "successful";
constexpr const char* FAILEDSTR = "failed";
constexpr const char* OKSTR = "ok";
constexpr const char* CANCLE = "cancle";

constexpr qint64 BATTLE_MAP_FILES = 220;
constexpr qint64 GRID_SIZE = 64;

constexpr qint64 LSTIME_SECONDS_PER_DAY = 5400LL;
constexpr qint64 LSTIME_HOURS_PER_DAY = 1024LL;
constexpr qint64 LSTIME_DAYS_PER_YEAR = 100LL;

#if 0

constexpr qint64  NIGHT_TO_MORNING = 906;
constexpr qint64  MORNING_TO_NOON = 1006;
constexpr qint64  NOON_TO_EVENING = 356;
constexpr qint64  EVENING_TO_NIGHT = 456;

#else

constexpr qint64 NIGHT_TO_MORNING = 700;
constexpr qint64 MORNING_TO_NOON = 930;
constexpr qint64 NOON_TO_EVENING = 200;
constexpr qint64 EVENING_TO_NIGHT = 300;

#endif


#pragma endregion

#pragma region Enums

enum ClientLoginFlag
{
	WITH_CDKEY = 0x1,
	WITH_PASSWORD = 0x2,
	WITH_MACADDRESS = 0x4,
	WITH_SELECTSERVERINDEX = 0x8,
	WITH_IPADDRESS = 0x10,
	WITH_ALL = WITH_CDKEY | WITH_PASSWORD | WITH_MACADDRESS | WITH_SELECTSERVERINDEX | WITH_IPADDRESS,
};

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

	LSSPROTO_IMAGE_RECV = 151,

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

enum GameDataOffest
{
	kOffsetPersonalKey = 0x4AC0898,

	kOffsetAccount = 0x414F278,
	kOffsetAccountECB = 0x415703C,

	kOffsetPassword = 0x415AA58,
	kOffsetPasswordECB = 0x4156280,

	kOffsetServerIndex = 0x415EF28,
	kOffsetSubServerIndex = 0xC4288,
	kOffsetPositionIndex = 0x4ABE270,
	kOffsetMousePointedIndex = 0x41F1B90,
	kOffsetWorldStatus = 0x4230DD8,
	kOffsetGameStatus = 0x4230DF0,
	kOffsetBattleStatus = 0x41829AC,
	kOffsetCharStatus = 0x422BF2C,
	kOffsetNowX = 0x4181D3C,
	kOffsetNowY = 0x4181D40,
	kOffsetDir = 0x422BE94,
	kOffsetNowFloor = 0x4181190,
	kOffsetNowFloorName = 0x4160228,
	kOffsetStandbyPetCount = 0xE1074,
	kOffsetBattlePetIndex = 0x422BF32,
	kOffsetSelectPetArray = 0x422BF34,
	kOffsetMailPetIndex = 0x422BF3E,
	kOffsetRidePetIndex = 0x422E3D8,
	kOffsetTeamState = 0x4230B24,
	kOffsetEV = 0x41602BC,
	kOffsetChatBuffer = 0x144D88,
	kOffsetChatBufferMaxCount = 0x14A4F8,
};

#ifdef _ITEM_EQUITSPACE
typedef enum tagCHAR_EquipPlace
{
	CHAR_EQUIPNONE = -1,
	PET_EQUIPNONE = -1,
	CHAR_HEAD,
	CHAR_BODY,
	CHAR_ARM,
	CHAR_DECORATION1,
	CHAR_DECORATION2,

#ifdef _ITEM_EQUITSPACE
	CHAR_EQBELT,
	CHAR_EQSHIELD,
	CHAR_EQSHOES,
#endif

#ifdef _EQUIT_NEWGLOVE
	CHAR_EQGLOVE,
#endif
	CHAR_EQUIPPLACENUM,
#ifdef _PET_ITEM
	PET_HEAD = 0,	// 頭
	PET_WING,		// 翼
	PET_TOOTH,		// 牙
	PET_PLATE,		// 身體
	PET_BACK,		// 背
	PET_CLAW,		// 爪
	PET_FOOT,		// 腳(鰭)
	PET_EQUIPNUM
#endif
}CHAR_EquipPlace;

#ifdef _PET_ITEM
typedef enum tagITEM_CATEGORY
{
	// 寵物道具,共九種
	ITEM_PET_HEAD = 29,		// 頭
	ITEM_PET_WING,			// 翼
	ITEM_PET_TOOTH,			// 牙
	ITEM_PET_PLATE,			// 身體護甲
	ITEM_PET_BACK,			// 背部護甲
	ITEM_PET_CLAW,			// 爪
	ITEM_PET_1_FOOT,		// 腳部,雙足
	ITEM_PET_2_FOOT,		// 腳部,四足
	ITEM_PET_FIN,			// 腳部,鰭
	ITEM_CATEGORYNUM
}ITEM_CATEGORY;
constexpr qint64 MAX_PET_ITEM = 7;
#endif


constexpr qint64 MAX_ITEMSTART = CHAR_EQUIPPLACENUM;
constexpr qint64 MAX_MAXHAVEITEM = 15;
#ifdef _NEW_ITEM_
constexpr qint64 MAX_ITEM(MAX_ITEMSTART + MAX_MAXHAVEITEM * 3);
//qint64 判斷玩家道具數量();
#else
constexpr qint64 MAX_ITEM = (MAX_ITEMSTART + MAX_MAXHAVEITEM);
#endif
#else
constexpr qint64 MAX_ITEMSTART = 5;
constexpr qint64 MAX_ITEM = 20;
#endif

enum
{
	PC_ETCFLAG_GROUP = (1LL << 0),	//組隊開關
	PC_ETCFLAG_UNK = (1LL << 1),	//未知開關
	PC_ETCFLAG_PK = (1LL << 2),	//決鬥開關
	PC_ETCFLAG_PARTY_CHAT = (1LL << 3),//隊伍聊天開關
	PC_ETCFLAG_CARD = (1LL << 4),	//名片開關
	PC_ETCFLAG_TRADE = (1LL << 5),	//交易開關
	PC_ETCFLAG_WORLD = (1LL << 6),	//世界頻道開關
	PC_ETCFLAG_FM = (1LL << 7),	//家族頻道開關
	PC_ETCFLAG_JOB = (1LL << 8),	//職業頻道開關
};

//enum
//{
//	PC_ETCFLAG_PARTY = (1LL << 0),
//	PC_ETCFLAG_DUEL = (1LL << 1),
//	PC_ETCFLAG_CHAT_MODE = (1LL << 2),//隊伍頻道開關
//	PC_ETCFLAG_MAIL = (1LL << 3),//名片頻道
//	PC_ETCFLAG_TRADE = (1LL << 4)
//#ifdef _CHANNEL_MODIFY
//	, PC_ETCFLAG_CHAT_TELL = (1LL << 5)//密語頻道開關
//	, PC_ETCFLAG_CHAT_FM = (1LL << 6)//家族頻道開關
//#ifdef _CHAR_PROFESSION
//	, PC_ETCFLAG_CHAT_OCC = (1LL << 7)//職業頻道開關
//#endif
//	, PC_ETCFLAG_CHAT_SAVE = (1LL << 8)//對話儲存開關
//#ifdef _CHATROOMPROTOCOL
//	, PC_ETCFLAG_CHAT_CHAT = (1LL << 9)//聊天室開關
//#endif
//#endif
//#ifdef _CHANNEL_WORLD
//	, PC_ETCFLAG_CHAT_WORLD = (1LL << 10)//世界頻道開關
//#endif
//#ifdef _CHANNEL_ALL_SERV
//	, PC_ETCFLAG_ALL_SERV = (1LL << 11)//星球頻道開關
//#endif
//	, PC_AI_MOD = (1LL << 12)
//};

enum
{
	PC_ETCFLAG_CHAT_MODE_ID = 0

#ifdef _CHANNEL_MODIFY
	, PC_ETCFLAG_CHAT_TELL_ID		//密語頻道
	, PC_ETCFLAG_CHAT_PARTY_ID		//隊伍頻道
	, PC_ETCFLAG_CHAT_FM_ID			//家族頻道
#ifdef _CHAR_PROFESSION
	, PC_ETCFLAG_CHAT_OCC_ID			//職業頻道
#endif
#ifdef _CHATROOMPROTOCOL
	, PC_ETCFLAG_CHAT_CHAT_ID		//聊天室
#endif
#else
	, PC_ETCFLAG_CHAT_PARTY_ID		//隊伍頻道
#endif
#ifdef _CHANNEL_WORLD
	, PC_ETCFLAG_CHAT_WORLD_ID			//世界頻道
#endif
#ifdef _CHANNEL_ALL_SERV
	, PC_ETCFLAG_ALL_SERV_ID			//星球頻道開關
#endif
	, PC_ETCFLAG_CHAT_WORLD_NUM
};

enum CHR_STATUS
{
	CHR_STATUS_NONE = 0,
	CHR_STATUS_P = 0x0001,
	CHR_STATUS_N = 0x0002,
	CHR_STATUS_Q = 0x0004,
	CHR_STATUS_S = 0x0008,
	CHR_STATUS_D = 0x0010,
	CHR_STATUS_C = 0x0020,
	CHR_STATUS_W = 0x0040,
	CHR_STATUS_H = 0x0080,
	CHR_STATUS_LEADER = 0x0100,
	CHR_STATUS_PARTY = 0x0200,
	CHR_STATUS_BATTLE = 0x0400,
	CHR_STATUS_USE_MAGIC = 0x0800,
	CHR_STATUS_HELP = 0x1000,
	CHR_STATUS_FUKIDASHI = 0x2000,
	CHR_STATUS_WATCH = 0x4000,
	CHR_STATUS_TRADE = 0x8000,			// 交易中
#ifdef _ANGEL_SUMMON
	CHR_STATUS_ANGEL = 0x10000			// 使者任務中
#endif
};

enum CHAROBJ_TYPE
{
	CHAROBJ_TYPE_NONE = 0x0000,
	CHAROBJ_TYPE_NPC = 0x0001,		// NPC
	CHAROBJ_TYPE_ITEM = 0x0002,
	CHAROBJ_TYPE_MONEY = 0x0004,
	CHAROBJ_TYPE_USER_NPC = 0x0008,
	CHAROBJ_TYPE_LOOKAT = 0x0010,
	CHAROBJ_TYPE_PARTY_OK = 0x0020,
	CHAROBJ_TYPE_ALL = 0x00FF
};

enum CHAR_TYPE
{
	CHAR_TYPENONE,
	CHAR_TYPEPLAYER,
	CHAR_TYPEENEMY,
	CHAR_TYPEPET,
	CHAR_TYPEDOOR,
	CHAR_TYPEBOX,
	CHAR_TYPEMSG,
	CHAR_TYPEWARP,
	CHAR_TYPESHOP,
	CHAR_TYPEHEALER,
	CHAR_TYPEOLDMAN,
	CHAR_TYPEROOMADMIN,
	CHAR_TYPETOWNPEOPLE,
	CHAR_TYPEDENGON,
	CHAR_TYPEADM,
	CHAR_TYPETEMPLE,        // Temple master
	CHAR_TYPESTORYTELLER,
	CHAR_TYPERANKING,
	CHAR_TYPEOTHERNPC,
	CHAR_TYPEPRINTPASSMAN,
	CHAR_TYPENPCENEMY,
	CHAR_TYPEACTION,
	CHAR_TYPEWINDOWMAN,
	CHAR_TYPESAVEPOINT,
	CHAR_TYPEWINDOWHEALER,
	CHAR_TYPEITEMSHOP,
	CHAR_TYPESTONESHOP,
	CHAR_TYPEDUELRANKING,
	CHAR_TYPEWARPMAN,
	CHAR_TYPEEVENT,
	CHAR_TYPEMIC,
	CHAR_TYPELUCKYMAN,
	CHAR_TYPEBUS,
	CHAR_TYPECHARM,
	CHAR_TYPENUM
};

typedef enum
{
	LS_NOON,
	LS_EVENING,
	LS_NIGHT,
	LS_MORNING,
}LSTIME_SECTION;

#ifdef _FMVER21
enum
{
	FMMEMBER_NONE = -1,  // 未加入任何家族
	FMMEMBER_MEMBER = 1,   // 一般成員
	FMMEMBER_APPLY,        // 申請加入家族
	FMMEMBER_LEADER,       // 家族族長
	FMMEMBER_ELDER,        // 長老
	//FMMEMBER_INVITE,     // 祭司
	//FMMEMBER_BAILEE,     // 財務長
	//FMMEMBER_VICELEADER, // 副族長
	FMMEMBER_NUMBER,
};
#endif

enum
{
	MAGIC_FIELD_ALL,
	MAGIC_FIELD_BATTLE,
	MAGIC_FIELD_MAP
};

enum
{
	MAGIC_TARGET_MYSELF,//自己
	MAGIC_TARGET_OTHER,//雙方任意
	MAGIC_TARGET_ALLMYSIDE,//我方全體
	MAGIC_TARGET_ALLOTHERSIDE,//敵方全體
	MAGIC_TARGET_ALL,//雙方全體同時
	MAGIC_TARGET_NONE,//無
	MAGIC_TARGET_OTHERWITHOUTMYSELF,//我方任意除了自己
	MAGIC_TARGET_WITHOUTMYSELFANDPET,//我方任意除了自己和寵物
	MAGIC_TARGET_WHOLEOTHERSIDE,//敵方全體 或 我方全體
#ifdef __ATTACK_MAGIC
	MAGIC_TARGET_SINGLE,				// 針對敵方某一方
	MAGIC_TARGET_ONE_ROW,				// 針對敵方某一列
	MAGIC_TARGET_ALL_ROWS,				// 針對敵方所有人
#endif
};

enum
{
	PETSKILL_FIELD_ALL,
	PETSKILL_FIELD_BATTLE,
	PETSKILL_FIELD_MAP
};

enum
{
	PETSKILL_TARGET_MYSELF,
	PETSKILL_TARGET_OTHER,
	PETSKILL_TARGET_ALLMYSIDE,
	PETSKILL_TARGET_ALLOTHERSIDE,
	PETSKILL_TARGET_ALL,
	PETSKILL_TARGET_NONE,
	PETSKILL_TARGET_OTHERWITHOUTMYSELF,
	PETSKILL_TARGET_WITHOUTMYSELFANDPET
#ifdef _BATTLESKILL				// (不可開) Syu ADD 戰鬥技能介面
	, PETSKILL_TARGET_ONE_ROW
	, PETSKILL_TARGET_ONE_LINE
	, PETSKILL_TARGER_DEATH
#endif
#ifdef _SKILL_ADDBARRIER
	, PETSKILL_TARGET_ONE_ROW_ALL //選我方的單排
#endif
};

enum
{
	ITEM_FIELD_ALL,
	ITEM_FIELD_BATTLE,
	ITEM_FIELD_MAP,
};

enum
{
	ITEM_TARGET_MYSELF,
	ITEM_TARGET_OTHER,
	ITEM_TARGET_ALLMYSIDE,
	ITEM_TARGET_ALLOTHERSIDE,
	ITEM_TARGET_ALL,
	ITEM_TARGET_NONE,
	ITEM_TARGET_OTHERWITHOUTMYSELF,
	ITEM_TARGET_WITHOUTMYSELFANDPET,
#ifdef _PET_ITEM
	ITEM_TARGET_PET
#endif
};

enum
{
	MOUSE_CURSOR_MODE_NORMAL,
	MOUSE_CURSOR_MODE_MOVE
};

enum
{
	PROC_INIT,
	PROC_ID_PASSWORD,
	PROC_TITLE_MENU,
	PROC_CHAR_SELECT,
	PROC_CHAR_MAKE,
	PROC_CHAR_LOGIN_START,
	PROC_CHAR_LOGIN,
	PROC_CHAR_LOGOUT,
	PROC_OPENNING,
	PROC_GAME,
	PROC_BATTLE,
	PROC_DISCONNECT_SERVER,

	PROC_TAKE_TEST,
	PROC_OHTA_TEST,
	PROC_DWAF_TEST,
	PROC_SPR_VIEW,
	PROC_ANIM_VIEW,
	PROC_SE_TEST,

#ifdef _80_LOGIN_PLAY
	PROC_80_LOGIN,
#endif
#ifdef _PK2007
	PROC_PKSERVER_SELECT,
#endif
	PROC_ENDING
};

enum PetState
{
	kNoneState,
	kBattle,
	kStandby,
	kMail,
	kRest,
	kRide,
	kDouble,
};

enum BUTTON_TYPE
{
	BUTTON_NOTUSED = 0,
	BUTTON_OK = 1LL << 0,	   //確定
	BUTTON_CANCEL = 1LL << 1, //取消
	BUTTON_YES = 1LL << 2,	   //
	BUTTON_NO = 1LL << 3,
	BUTTON_PREVIOUS = 1LL << 4,  //上一頁
	BUTTON_NEXT = 1LL << 5,
	BUTTON_BUY,
	BUTTON_SELL,
	BUTTON_OUT,
	BUTTON_BACK,
	BUTTON_1,//是
	BUTTON_2,//不是
	BUTTON_AUTO,
};


enum DirType
{
	kDirNone = -1,
	kDirNorth = 0,
	kDirNorthEast,
	kDirEast,
	kDirSouthEast,
	kDirSouth,
	kDirSouthWest,
	kDirWest,
	kDirNorthWest,
};

enum DialogType
{
	kDiaogNone = -1,

	kDialogBuy = 242,
	kDialogSell = 243,

	kDialogDepositPet = 265,
	kDialogWithdrawPet = 267,

	kDialogDepositPetEx = 513,//寵倉
	kDialogWithdrawPetEx = 514,//寵倉

	kDialogDepositItem = 291,
	kDialogWithdrawItem = 292,

	kDialogDepositItemEx = 312,
	kDialogWithdrawItemEx = 313,
	kDialogWithdrawItemEx2 = 314,
	kDialogWithdrawItemEx3 = 315,
	kDialogWithdrawItemEx4 = 316,


	kDialogSecurityCode = 522,
};

enum TalkMode
{
	kTalkNormal,
	kTalkTeam,
	kTalkFamily,
	kTalkWorld,
	kTalkGlobal,
	kTalkModeMax,
};
#pragma endregion

#pragma region Structs
#ifdef _WIN64
#pragma pack(8)
#else
#pragma pack(8) 
#endif

#if 0
typedef struct action
{
	struct 	action* pPrev, * pNext;			//上一個及下一個action指標
	void 	(*func)(struct action*);	//action所執行的function的指標
	void* pYobi;							//備用的struct指標
	void* pOther;						//其它用途struct指標
	UCHAR 	prio;							//action處理時的優先順序
	UCHAR 	dispPrio;						//秀圖時的優先順序
	int 	x, y;							//圖的座標
	int		hitDispNo;						//是否命中目標編號
	BOOL	deathFlag;						//此action是否死亡旗標
	int 	dx, dy;							//秀圖座標位移量
	int 	dir;							//方向
	int 	delta;  						//合成向量

	char 	name[29];						//名字
	char 	freeName[33];					//free name
	int 	hp;
#ifdef _PET_ITEM
	int		iOldHp;
#endif
	int 	maxHp;
	int 	mp;
	int 	maxMp;
	int 	level;
	int 	status;
	int 	itemNameColor;
	int		charNameColor;

	int		bmpNo;							//圖號
	int		bmpNo_bak;							//備份圖號
	int		atr;							//屬性
	int		state;							//狀態
	int		actNo;							//行動編號
	int		damage;

	int		gx, gy;							//在目前的地圖上的座標
	int		nextGx, nextGy;					//下一個座標
	int		bufGx[10], bufGy[10];			//從目前座標到下一個座標之間座標的buffer
	int	bufCount;						//設定目前要走到那一個座標
	int	walkFlag;
	float	mx, my;							//地圖座標
	float	vx, vy;

	//屬性
	int 	earth;
	int 	water;
	int 	fire;
	int 	wind;
	//rader使用
	int		dirCnt;
	//gemini使用
	int		spd;							//移動的速度(0~63)
	int		crs;							//方向(0~31)(正上方為0,順時鐘方向)
	int		h_mini;
	int		v_mini;
	//pattern使用
	int		anim_chr_no;					//人物的編號(anim_tbl.h的編號)
	int		anim_chr_no_bak;				//上一次的人物編號
	int		anim_no;						//人物的動作編號
	int		anim_no_bak;					//上一次的人物編號
	int		anim_ang;						//動作的方向(0~7)(下0)
	int		anim_ang_bak;					//上一次的方向
	int		anim_cnt;						//第幾張frame
	int		anim_frame_cnt;					//這張frame停留時間
	int		anim_x;							//X座標(Sprbin+Adrnbin)
	int		anim_y;							//Y座標(Sprbin+Adrnbin)
	int		anim_hit;
	// shan add +1
	char    fmname[33];			            // 家族名稱
	// Robin 0728 ride Pet
	int		onRide;
	char	petName[16 + 1];
	int		petLevel;
	int		petHp;
	int		petMaxHp;
	int		petDamage;
	int		petFall;
#ifdef _MIND_ICON
	unsigned int sMindIcon;
#endif
#ifdef _SHOWFAMILYBADGE_
	unsigned int sFamilyIcon;
#endif
#ifdef FAMILY_MANOR_
	unsigned int mFamilyIcon;
#endif
#ifdef _CHAR_MANOR_
	unsigned int mManorIcon;
#endif
#ifdef _CHARTITLE_STR_
	TITLE_STR TitleText;
#endif
#ifdef _CHARTITLE_
	unsigned int TitleIcon;
#endif
#ifdef _NPC_EVENT_NOTICE
	int noticeNo;
#endif

#ifdef _SKILL_ROAR
	int		petRoar;		//大吼(克年獸)
#endif
#ifdef _SKILL_SELFEXPLODE //自爆
	int		petSelfExplode;
#endif
#ifdef _MAGIC_DEEPPOISION   //劇毒
	int		petDeepPoision;
#endif

#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
	int		profession_class;
#endif
	//#ifdef _BATTLESKILL				// (不可開) Syu ADD 戰鬥技能介面
	int		warrioreffect;
	//#endif
#ifdef _GM_IDENTIFY		// Rog ADD GM識別
	char gm_name[33];
#endif
#ifdef _STREET_VENDOR
	char szStreetVendorTitle[64];
#endif
#ifdef _NPC_PICTURE
	int picture;
	int picturetemp;
#endif
#ifdef _PETSKILL_RIDE
	int saveride;
#endif
#ifdef _MOUSE_DBL_CLICK
	int index;	// 禁斷!! Server中的charaindex
#endif

#ifdef _SFUMATO
	int sfumato;		// 二次渲染圖層色彩
#endif
}ACTION;
#endif

typedef struct customdialog_s
{
	qint64 x = 0;
	qint64 y = 0;
	BUTTON_TYPE button = BUTTON_NOTUSED;
	qint64 row = 0;
	qint64 rawbutton = 0;
} customdialog_t;

typedef struct tagLSTIME
{
	qint64 year = 0;
	qint64 day = 0;
	qint64 hour = 0;
}LSTIME;

typedef struct tagMAIL_HISTORY
{
	qint64 	noReadFlag[MAIL_MAX_HISTORY] = { 0 };
	qint64 	petLevel[MAIL_MAX_HISTORY] = { 0 };
	qint64 	itemGraNo[MAIL_MAX_HISTORY] = { 0 };
	qint64 	newHistoryNo = 0;
	QString str[MAIL_MAX_HISTORY];
	QString dateStr[MAIL_MAX_HISTORY];
	QString petName[MAIL_MAX_HISTORY];

	void clear()
	{
		for (qint64 i = 0; i < MAIL_MAX_HISTORY; ++i)
		{
			str[i] = "";
			dateStr[i] = "";
			noReadFlag[i] = 0;
			petLevel[i] = 0;
			petName[i] = "";
			itemGraNo[i] = 0;
		}
		newHistoryNo = 0;
	}
}MAIL_HISTORY;

typedef struct tagITEM
{
	bool valid = false;
	qint64 color = 0;
	qint64 modelid = 0;
	qint64 level = 0;
	qint64 stack = 0;
	qint64 type = 0ui16;
	qint64 field = 0;
	qint64 target = 0;
	qint64 deadTargetFlag = 0;
	qint64 sendFlag = 0;
	qint64 itemup = 0;
	qint64 counttime = 0;

	//custom
	qint64 maxStack = -1;

	QString name = "";
	QString name2 = "";
	QString memo = "";
	QString damage = "";
	QString alch = 0; // #ifdef _ITEMSET7_TXT_ALCHEMIST
	QString jigsaw = "";
#ifdef _MAGIC_ITEM_
	qint64 道具類型 = 0;
#endif
} ITEM;

typedef struct tagPC
{
	bool petFight[MAX_PET] = { 0, 0, 0, 0, 0 };;
	bool petWait[MAX_PET] = { 0, 0, 0, 0, 0 };
	bool petRide[MAX_PET] = { 0, 0, 0, 0, 0 };
	qint64 selectPetNo[MAX_PET] = { 0, 0, 0, 0, 0 };
	qint64 battlePetNo = -1;
	qint64 mailPetNo = -1;
	qint64 standbyPet = -1;
	qint64 ridePetNo = -1;

	qint64 modelid = 0;
	qint64 faceid = 0;
	qint64 id = 0;
	qint64 dir = 0;
	qint64 hp = 0, maxHp = 0, hpPercent = 0;
	qint64 mp = 0, maxMp = 0, mpPercent = 0;
	qint64 vit = 0;
	qint64 str = 0, tgh = 0, dex = 0;
	qint64 exp = 0, maxExp = 0;
	qint64 level = 0;
	qint64 atk = 0, def = 0;
	qint64 agi = 0, chasma = 0, luck = 0;
	qint64 earth = 0, water = 0, fire = 0, wind = 0;
	qint64 gold = 0;
	qint64 fame = 0;
	qint64 titleNo = 0;
	qint64 dp = 0;
	qint64 nameColor = 0;
	qint64 status = 0;
	qint64 etcFlag = 0;
	qint64 battleNo = 0;
	qint64 sideNo = 0i16;
	qint64 helpMode = 0i16;
	qint64 pcNameColor = 0;
	qint64 transmigration = 0;
	QString chusheng = "";
	QString family = "";
	qint64 familyleader = 0;
	qint64 channel = 0;
	qint64 quickChannel = 0;
	qint64 personal_bankgold = 0;
	qint64 learnride = 0;//學習騎乘
	qint64 lowsride = 0u;
	qint64 ridePetLevel = 0;
	qint64 familySprite = 0;
	qint64 baseGraNo = 0;
	qint64 big4fm = 0;
	qint64 trade_confirm = 0;         // 1 -> 初始值
	qint64 profession_class = 0;
	qint64 profession_level = 0;
	qint64 profession_exp = 0;
	qint64 profession_skill_point = 0;

	qint64 herofloor = 0;// (不可開)排行榜NPC
	qint64 iOnStreetVendor = 0;		// 擺攤模式
	qint64 skywalker = 0; // GM天行者
	qint64 iTheaterMode = 0;		// 劇場模式
	qint64 iSceneryNumber = 0;		// 記錄劇院背景圖號

	qint64 iDanceMode = 0;			// 動一動模式
	qint64 newfame = 0; // 討伐魔軍積分
	qint64 ftype = 0;

	//custom
	qint64 maxload = -1;
	qint64 point = 0;

	QString name = "";
	QString freeName = "";
	QString ridePetName = "";
	QString profession_class_name = "";
	QString gm_name = "";	// Rog ADD GM識別
	QString chatRoomNum = "";// ROG ADD 好友頻道
	ITEM item[MAX_ITEM] = { 0 };
	ITEM itempool[MAX_ITEM] = {};
	// 2 -> 慬我方按下確定鍵
	// 3 -> 僅對方按下確定鍵
	// 4 -> 雙方皆按下確定鍵

	//ACTION* pActNPC[5];		// 記錄劇場中臨時產生出來的NPC

#ifdef _NEW_ITEM_
	qint64 道具欄狀態 = 0;
#endif
#ifdef _CHARSIGNADY_NO_
	qint64 簽到標記 = 0;
#endif
#ifdef _MAGIC_ITEM_
	qint64 法寶道具狀態 = 0;
	qint64 道具光環效果 = 0;
#endif
} PC;

typedef struct tagPET
{
	bool valid = false;
	qint64 index = 0;						//位置
	qint64 modelid = 0;						//圖號
	qint64 hp = 0, maxHp = 0, hpPercent = 0;					//血量
	qint64 mp = 0, maxMp = 0, mpPercent = 0;					//魔力
	qint64 exp = 0, maxExp = 0;				//經驗值
	qint64 level = 0;						//等級
	qint64 atk = 0;						//攻擊力
	qint64 def = 0;						//防禦力
	qint64 agi = 0;						//速度
	qint64 loyal = 0;							//AI
	qint64 earth = 0, water = 0, fire = 0, wind = 0;
	qint64 maxSkill = 0;
	qint64 transmigration = 0;						// 寵物轉生數
	qint64 fusion = 0;						// low word: 寵蛋旗標, hi word: 物種編碼
	qint64 status = 0;
	qint64 oldlevel = 0, oldhp = 0, oldatk = 0, oldagi = 0, olddef = 0;
	qint64 rideflg;
	qint64 blessflg = 0;
	qint64 blesshp = 0;
	qint64 blessatk = 0;
	qint64 blessquick = 0;
	qint64 blessdef = 0;
	qint64 changeNameFlag = 0;
	QString name = "";
	QString freeName = "";
	PetState state = PetState::kNoneState;

	//custom
	qreal power = 0.0;
	qreal growth = 0.0;

	ITEM item[MAX_PET_ITEM] = {};		// 寵物道具
} PET;


typedef struct tagMAGIC
{
	bool valid = false;
	qint64 costmp = 0;
	qint64 field = 0;
	qint64 target = 0;
	qint64 deadTargetFlag = 0;
	QString name = "";
	QString memo = "";
} MAGIC;


typedef struct tagPARTY
{
	bool valid = false;
	qint64 id = 0;
	qint64 level = 0;
	qint64 maxHp = 0;
	qint64 hp = 0;
	qint64 hpPercent = 0;
	qint64 mp = 0;
	QString name = "";
	//ACTION* ptAct;
} PARTY;


typedef struct tagADDRESS_BOOK
{
	bool valid = false;
	bool onlineFlag = false;
	qint64 level = 0;
	qint64 transmigration = 0i16;
	qint64 dp = 0;
	qint64 modelid = 0;
	QString name = "";
#ifdef _MAILSHOWPLANET				// (可開放) Syu ADD 顯示名片星球
	QString planetname = "";
#endif
} ADDRESS_BOOK;

typedef struct tagPET_SKILL
{
	bool valid = false;
	qint64 skillId = 0;
	qint64 field = 0;
	qint64 target = 0;
	QString name = "";
	QString memo = "";
} PET_SKILL;

#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
typedef struct tagPROFESSION_SKILL
{
	bool valid = false;
	qint64 skillId = 0;
	qint64 target = 0;
	qint64 kind = 0;
	qint64 icon = 0;
	qint64 costmp = 0;
	qint64 skill_level = 0;
	qint64 cooltime = 0;
	QString name = "";
	QString memo = "";
} PROFESSION_SKILL;
#endif


typedef struct tagCHARLISTTABLE
{
	bool valid = false;
	int attr[4] = { 0,0,0,0 };
	qint64 level = 0;
	qint64 login = 0;
	qint64 faceid = 0;
	qint64 hp = 0;
	qint64 str = 0;
	qint64 def = 0;
	qint64 agi = 0;
	qint64 chasma = 0;
	qint64 dp = 0;
	qint64 pos = -1;
	QString name = "";
} CHARLISTTABLE;
#ifdef _AIDENGLU_
typedef struct
{
	qint64 大區 = 0;
	qint64 隊模 = 0;
	qint64 小區 = 0;
	qint64 人物 = 0;
	qint64 是否自動喊話 = 0;
	qint64 是否自動遇敵 = 0;
	qint64 人物方向 = 0;
	qint64 登陸延時時間 = 0;
	char 登陸人物名稱[4][32] = { 0 };
}Landed;
#endif

constexpr int MAX_MISSION = 300;
typedef struct tagJOBDAILY
{
	qint64 JobId = 0;								// 任務編號
	QString explain = "";						// 任務說明
	QString state = "";							// 狀態
}JOBDAILY;

struct showitem
{
	QString name;
	QString freeName;
	QString graph;
	QString effect;
	QString color;
	QString itemIndex;
	QString damage;
};

typedef struct SPetItemInfo
{
	qint64 bmpNo = 0;										// 图号
	qint64 color = 0;										// 文字颜色
	QString memo;						// 说明
	QString name;						// 名字
	QString damage;								// 耐久度
}PetItemInfo;

struct showpet
{
	QString opp_petname;
	QString opp_petfreename;
	QString opp_petgrano;
	QString opp_petlevel;
	QString opp_petatk;
	QString opp_petdef;
	QString opp_petquick;
	QString opp_petindex;
	QString opp_pettrans;
	QString opp_petshowhp;
	QString opp_petslot;
	QString opp_petskill1;
	QString opp_petskill2;
	QString opp_petskill3;
	QString opp_petskill4;
	QString opp_petskill5;
	QString opp_petskill6;
	QString opp_petskill7;
#ifdef _SHOW_FUSION
	QString opp_fusion;
#endif
#ifdef _PET_ITEM
	PetItemInfo oPetItemInfo[MAX_PET_ITEM];			// 宠物身上的道具
#endif
};

typedef struct dialog_s
{
	qint64 windowtype = 0;
	qint64 buttontype = 0;
	qint64 dialogid = 0;
	qint64 unitid = 0;
	QString data = "";
	QStringList linedatas;
	QStringList linebuttontext;
}dialog_t;


typedef struct battleobject_s
{
	bool ready = false;
	qint64 pos = -1;
	qint64 modelid = 0;
	qint64 level = 0;
	qint64 hp = 0;
	qint64 maxHp = 0;
	qint64 hpPercent = 0;
	qint64 status = 0;
	qint64 rideFlag = 0;
	qint64 rideLevel = 0;
	qint64 rideHp = 0;
	qint64 rideMaxHp = 0;
	qint64 rideHpPercent = 0;
	QString rideName = 0;
	QString name = "";
	QString freeName = "";
} battleobject_t;

typedef struct battledata_s
{
	bool charAlreadyAction = false;
	bool petAlreadyAction = false;
	qint64 fieldAttr = 0;
	qint64 alliemin = 0, alliemax = 0, enemymax = 0, enemymin = 0;
	battleobject_t player = {};
	battleobject_t pet = {};
	QVector<battleobject_t> objects;
	QVector<battleobject_t> allies;
	QVector<battleobject_t> enemies;
} battledata_t;


typedef struct mapunit_s
{
	bool isVisible = false;
	bool walkable = false;
	qint64 id = 0;
	qint64 modelid = 0;
	qint64 x = 0;
	qint64 y = 0;
	qint64 dir = 0;
	qint64 level = 0;
	qint64 nameColor = 0;
	qint64 height = 0;
	qint64 charNameColor = 0;
	qint64 petlevel = 0;
	qint64 classNo = 0;
	qint64 gold = 0;
	qint64 profession_class = 0;
	qint64 profession_level = 0;
	qint64 profession_skill_point = 0;
	QPoint p;
	CHAR_TYPE type = CHAR_TYPENONE;
	util::ObjectType objType = util::ObjectType::OBJ_UNKNOWN;
	CHR_STATUS status = CHR_STATUS::CHR_STATUS_NONE;
	QString name = "";
	QString freeName = "";
	QString family = "";
	QString petname = "";
	QString item_name = "";
} mapunit_t;

typedef struct bankpet_s
{
	qint64 level = 0;
	qint64 maxHp = 0;
	QString name;
}bankpet_t;

typedef struct currencydata_s
{
	qint64 expbufftime = 0;
	qint64 prestige = 0;      // 聲望
	qint64 energy = 0;        // 氣勢
	qint64 shell = 0;         // 貝殼
	qint64 vitality = 0;      // 活力
	qint64 points = 0;        // 積分
	qint64 VIPPoints = 0;  // 會員點
} currencydata_t;

#pragma pack()

#pragma endregion