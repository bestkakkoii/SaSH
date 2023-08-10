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

constexpr size_t LINEBUFSIZ = 8192;

#ifdef _MORECHARACTERS_
constexpr int MAXCHARACTER = 4;
#else
constexpr int MAXCHARACTER = 2;
#endif
constexpr int CHAR_NAME_LEN = 16;
constexpr int CHAR_FREENAME_LEN = 32;
constexpr int MAGIC_NAME_LEN = 28;
constexpr int MAGIC_MEMO_LEN = 72;
constexpr int ITEM_NAME_LEN = 28;
constexpr int ITEM_NAME2_LEN = 16;
constexpr int ITEM_MEMO_LEN = 84;
constexpr int PET_NAME_LEN = 16;
constexpr int PET_FREENAME_LEN = 32;
constexpr int CHAR_FMNAME_LEN = 33;     // 家族名稱

#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
constexpr int PROFESSION_MEMO_LEN = 84;
#endif

#ifdef _GM_IDENTIFY		// Rog ADD GM識別
constexpr int GM_NAME_LEN = 32;
#endif

constexpr int CHARNAMELEN = 256;

constexpr int MAX_PET = 5;

constexpr int MAX_MAGIC = 9;

constexpr int MAX_PARTY = 5;

constexpr int MAX_ADR_BOOK_COUNT = 4;
#ifdef _EXTEND_AB
constexpr int MAX_ADR_BOOK_PAGE = 20;//20  //10   20050214 cyg 10 add to 20
#else
constexpr int MAX_ADR_BOOK_PAGE = 10;
#endif
constexpr int MAX_ADR_BOOK = (MAX_ADR_BOOK_COUNT * MAX_ADR_BOOK_PAGE);

#ifdef _PRO3_ADDSKILL
constexpr int MAX_PROFESSION_SKILL = 30;
#else
constexpr int MAX_PROFESSION_SKILL = 26;
#endif

constexpr int BATTLE_BUF_SIZE = 4;
constexpr int BATTLE_COMMAND_SIZE = 4096;

constexpr int FLOOR_NAME_LEN = 24;

constexpr int RESULT_ITEM_COUNT = 3;
constexpr int RESULT_ITEM_NAME_LEN = 24;
//constexpr int RESULT_CHR_EXP = 4;
constexpr int RESULT_CHR_EXP = 5;

constexpr int SKILL_NAME_LEN = 24;
constexpr int SKILL_MEMO_LEN = 72;
constexpr int MAX_SKILL = 7;

constexpr int MIN_HP_PERCENT = 10;

constexpr int MAX_GOLD = 1000000;
constexpr int MAX_BANKGOLD = 10000000;
constexpr int MAX_FMBANKGOLD = 100000000;


constexpr int MAX_PERSONAL_BANKGOLD = 50000000;

#ifdef _FMVER21
constexpr int FAMILY_MAXMEMBER = 100;    // 家族人數
#else
constexpr int FAMILY_MAXMEMBER = 50;    // 家族人數
#endif

constexpr int MAP_TILE_GRID_X1 = -20;
constexpr int MAP_TILE_GRID_X2 = +17;
constexpr int MAP_TILE_GRID_Y1 = -16;
constexpr int MAP_TILE_GRID_Y2 = +21;
constexpr int MAP_X_SIZE = (MAP_TILE_GRID_X2 - MAP_TILE_GRID_X1);
constexpr int MAP_Y_SIZE = (MAP_TILE_GRID_Y2 - MAP_TILE_GRID_Y1);

constexpr int MAP_READ_FLAG = 0x8000;
constexpr int  MAP_SEE_FLAG = 0x4000;

constexpr int BC_FLG_NEW = (1 << 0);
constexpr int BC_FLG_DEAD = (1 << 1);	  //死亡
constexpr int BC_FLG_PLAYER = (1 << 2);	  //玩家,玩家有异常状态时要有此值
constexpr int BC_FLG_POISON = (1 << 3);	  //中毒
constexpr int BC_FLG_PARALYSIS = (1 << 4); //麻痹
constexpr int BC_FLG_SLEEP = (1 << 5);  //昏睡
constexpr int BC_FLG_STONE = (1 << 6);	  //石化
constexpr int BC_FLG_DRUNK = (1 << 7);  //酒醉
constexpr int BC_FLG_CONFUSION = (1 << 8); //混乱
constexpr int BC_FLG_HIDE = (1 << 9);	  //是否隐藏，地球一周
constexpr int BC_FLG_REVERSE = (1 << 10);  //反转

inline constexpr bool hasBadStatus(int status)
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

	return false;
}

inline constexpr bool hasUnMoveableStatue(int status)
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

	return false;
}


constexpr int ITEM_FLAG_PET_MAIL = (1 << 0);
constexpr int ITEM_FLAG_MIX = (1 << 1);
constexpr int ITEM_FLAG_COOKING_MIX = (1 << 2);
constexpr int ITEM_FLAG_METAL_MIX = (1 << 3);	//金屬
constexpr int ITEM_FLAG_JEWEL_MIX = (1 << 4);	//寶石
constexpr int ITEM_FLAG_FIX_MIX = (1 << 5);	//修理
#ifdef _ITEM_INTENSIFY
constexpr int ITEM_FLAG_INTENSIFY_MIX = (1 << 6);	//強化
#endif
#ifdef _ITEM_UPINSLAY
constexpr int ITEM_FLAG_UPINSLAY_MIX = (1 << 7);	//鑿孔
#endif

constexpr int JOY_RIGHT = (1 << 15);/* Right Key				*/
constexpr int JOY_LEFT = (1 << 14);/*  Left Key				*/
constexpr int JOY_DOWN = (1 << 13);/*  Down Key				*/
constexpr int JOY_UP = (1 << 12);	/*    Up Key				*/
constexpr int JOY_START = (1 << 11);	/* Start					*/
constexpr int JOY_A = (1 << 10);	/* A Trigger				*/
constexpr int JOY_C = (1 << 9);/* C Trigger				*/
constexpr int JOY_B = (1 << 8);	/* B Trigger				*/
constexpr int JOY_R = (1 << 7);	/* R Trigger				*/
constexpr int JOY_X = (1 << 6);	/* X Trigger				*/
constexpr int JOY_DEL = (1 << 5);	/* DELETE					*/
constexpr int JOY_INS = (1 << 4);	/* INSERT					*/
constexpr int JOY_END = (1 << 3);/* END						*/
constexpr int JOY_HOME = (1 << 2);	/* HOME						*/
constexpr int JOY_P_DOWN = (1 << 1);	/* PAGE_UP					*/
constexpr int JOY_P_UP = (1 << 0);	/* PAGE_DOWN				*/

constexpr int JOY_ESC = (1 << 31);/* ESC Key					*/
constexpr int JOY_CTRL_M = (1 << 30);	/* Ctrl + M					*/
constexpr int JOY_CTRL_S = (1 << 29);	/* Ctrl + S					*/
constexpr int JOY_CTRL_P = (1 << 28);	/* Ctrl + P					*/
constexpr int JOY_CTRL_I = (1 << 27);	/* Ctrl + I					*/
constexpr int JOY_CTRL_E = (1 << 26);	/* Ctrl + E					*/
constexpr int JOY_CTRL_A = (1 << 25);	/* Ctrl + A					*/

constexpr int JOY_CTRL_C = (1 << 24);	/* Ctrl + C					*/
constexpr int JOY_CTRL_V = (1 << 23);	/* Ctrl + V					*/
constexpr int JOY_CTRL_T = (1 << 22);	/* Ctrl + T					*/

//#define MAIL_STR_LEN 140
//#define MAIL_DATE_STR_LEN 20
constexpr int MAIL_MAX_HISTORY = 20;
constexpr int MAX_CHAT_REGISTY_STR = 8;

constexpr int MAX_ENEMY = 20;
constexpr int MAX_CHAT_HISTORY = 20;
constexpr int MAX_DIALOG_LINE = 10;

constexpr int MAX_DIR = 8;

constexpr const char* SUCCESSFULSTR = "successful";
constexpr const char* FAILEDSTR = "failed";
constexpr const char* OKSTR = "ok";
constexpr const char* CANCLE = "cancle";

constexpr int BATTLE_MAP_FILES = 220;
constexpr int GRID_SIZE = 64;

constexpr unsigned long long LSTIME_SECONDS_PER_DAY = 5400ULL;
constexpr unsigned long long LSTIME_HOURS_PER_DAY = 1024ULL;
constexpr unsigned long long LSTIME_DAYS_PER_YEAR = 100ULL;

#if 0

constexpr int NIGHT_TO_MORNING = 906;
constexpr int MORNING_TO_NOON = 1006;
constexpr int  NOON_TO_EVENING = 356;
constexpr int EVENING_TO_NIGHT = 456;

#else

constexpr int NIGHT_TO_MORNING = 700;
constexpr int MORNING_TO_NOON = 930;
constexpr int NOON_TO_EVENING = 200;
constexpr int EVENING_TO_NIGHT = 300;

#endif

static const char* palFileName[] = {
	"data\\pal\\Palet_1.sap",	//
	"data\\pal\\Palet_2.sap",	//
	"data\\pal\\Palet_3.sap",	//
	"data\\pal\\Palet_4.sap",	//

	"data\\pal\\Palet_5.sap",	//
	"data\\pal\\Palet_6.sap",	//
	"data\\pal\\Palet_7.sap",	//
	"data\\pal\\Palet_8.sap",	//
	"data\\pal\\Palet_9.sap",	//
	"data\\pal\\Palet_10.sap",	//
	"data\\pal\\Palet_11.sap",	//
	"data\\pal\\Palet_12.sap",	//
	"data\\pal\\Palet_13.sap",	//
	"data\\pal\\Palet_14.sap",	//
	"data\\pal\\Palet_15.sap",	//
	"data\\pal\\Palet_0.sap",	//Waei logo
	"white.sap",				//
	"black.sap",				//
};

constexpr int MAX_PAL = sizeof(palFileName) / sizeof(palFileName[0]);


#pragma endregion

#pragma region Enums

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
	kOffestPersonalKey = 0x4AC0898,
	kOffestAccount = 0x414F278,
	kOffestPassword = 0x415AA58,
	kOffestServerIndex = 0x415EF28,
	kOffestSubServerIndex = 0xC4288,
	kOffestPositionIndex = 0x4ABE270,
	kOffestMousePointedIndex = 0x41F1B90,
	kOffestWorldStatus = 0x4230DD8,
	kOffestGameStatus = 0x4230DF0,
	kOffestBattleStatus = 0x41829AC,
	kOffestPlayerStatus = 0x422BF2C,
	kOffestNowX = 0x4181D3C,
	kOffestNowY = 0x4181D40,
	kOffestDir = 0x422BE94,
	kOffestNowFloor = 0x4181190,
	kOffestNowFloorName = 0x4160228,
	kOffestMailPetIndex = 0x422BF3E,
	kOffestRidePetIndex = 0x422E3D8,
	kOffestSelectPetArray = 0x422BF34,
	kOffestTeamState = 0x4230B24,
	kOffestEV = 0x41602BC,
	kOffestChatBuffer = 0x144D88,
	kOffestChatBufferMaxCount = 0x14A4F8,
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
constexpr int MAX_PET_ITEM = 7;
#endif


constexpr int MAX_ITEMSTART = CHAR_EQUIPPLACENUM;
constexpr int MAX_MAXHAVEITEM = 15;
#ifdef _NEW_ITEM_
constexpr int MAX_ITEM(MAX_ITEMSTART + MAX_MAXHAVEITEM * 3);
int 判斷玩家道具數量();
#else
constexpr int MAX_ITEM = (MAX_ITEMSTART + MAX_MAXHAVEITEM);
#endif
#else
constexpr int MAX_ITEMSTART = 5;
constexpr int MAX_ITEM = 20;
#endif


enum
{
	PC_ETCFLAG_GROUP = (1 << 0),	//組隊開關
	PC_ETCFLAG_UNK = (1 << 1),	//未知開關
	PC_ETCFLAG_PK = (1 << 2),	//決鬥開關
	PC_ETCFLAG_PARTY_CHAT = (1 << 3),//隊伍聊天開關
	PC_ETCFLAG_CARD = (1 << 4),	//名片開關
	PC_ETCFLAG_TRADE = (1 << 5),	//交易開關
	PC_ETCFLAG_WORLD = (1 << 6),	//世界頻道開關
	PC_ETCFLAG_FM = (1 << 7),	//家族頻道開關
	PC_ETCFLAG_JOB = (1 << 8),	//職業頻道開關
};

//enum
//{
//	PC_ETCFLAG_PARTY = (1 << 0),
//	PC_ETCFLAG_DUEL = (1 << 1),
//	PC_ETCFLAG_CHAT_MODE = (1 << 2),//隊伍頻道開關
//	PC_ETCFLAG_MAIL = (1 << 3),//名片頻道
//	PC_ETCFLAG_TRADE = (1 << 4)
//#ifdef _CHANNEL_MODIFY
//	, PC_ETCFLAG_CHAT_TELL = (1 << 5)//密語頻道開關
//	, PC_ETCFLAG_CHAT_FM = (1 << 6)//家族頻道開關
//#ifdef _CHAR_PROFESSION
//	, PC_ETCFLAG_CHAT_OCC = (1 << 7)//職業頻道開關
//#endif
//	, PC_ETCFLAG_CHAT_SAVE = (1 << 8)//對話儲存開關
//#ifdef _CHATROOMPROTOCOL
//	, PC_ETCFLAG_CHAT_CHAT = (1 << 9)//聊天室開關
//#endif
//#endif
//#ifdef _CHANNEL_WORLD
//	, PC_ETCFLAG_CHAT_WORLD = (1 << 10)//世界頻道開關
//#endif
//#ifdef _CHANNEL_ALL_SERV
//	, PC_ETCFLAG_ALL_SERV = (1 << 11)//星球頻道開關
//#endif
//	, PC_AI_MOD = (1 << 12)
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
	kRide
};

enum BUTTON_TYPE
{
	BUTTON_NOTUSED = 0,
	BUTTON_OK = 1 << 0,	   //確定
	BUTTON_CANCEL = 1 << 1, //取消
	BUTTON_YES = 1 << 2,	   //
	BUTTON_NO = 1 << 3,
	BUTTON_PREVIOUS = 1 << 4,  //上一頁
	BUTTON_NEXT = 1 << 5,
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

//typedef struct action
//{
//	struct 	action* pPrev, * pNext;			//上一個及下一個action指標
//	void 	(*func)(struct action*);	//action所執行的function的指標
//	void* pYobi;							//備用的struct指標
//	void* pOther;						//其它用途struct指標
//	UCHAR 	prio;							//action處理時的優先順序
//	UCHAR 	dispPrio;						//秀圖時的優先順序
//	int 	x, y;							//圖的座標
//	int		hitDispNo;						//是否命中目標編號
//	BOOL	deathFlag;						//此action是否死亡旗標
//	int 	dx, dy;							//秀圖座標位移量
//	int 	dir;							//方向
//	int 	delta;  						//合成向量
//
//	char 	name[29];						//名字
//	char 	freeName[33];					//free name
//	int 	hp;
//#ifdef _PET_ITEM
//	int		iOldHp;
//#endif
//	int 	maxHp;
//	int 	mp;
//	int 	maxMp;
//	int 	level;
//	int 	status;
//	int 	itemNameColor;
//	int		charNameColor;
//
//	int		bmpNo;							//圖號
//	int		bmpNo_bak;							//備份圖號
//	int		atr;							//屬性
//	int		state;							//狀態
//	int		actNo;							//行動編號
//	int		damage;
//
//	int		gx, gy;							//在目前的地圖上的座標
//	int		nextGx, nextGy;					//下一個座標
//	int		bufGx[10], bufGy[10];			//從目前座標到下一個座標之間座標的buffer
//	short	bufCount;						//設定目前要走到那一個座標
//	short	walkFlag;
//	float	mx, my;							//地圖座標
//	float	vx, vy;
//
//	//屬性
//	short 	earth;
//	short 	water;
//	short 	fire;
//	short 	wind;
//	//rader使用
//	int		dirCnt;
//	//gemini使用
//	int		spd;							//移動的速度(0~63)
//	int		crs;							//方向(0~31)(正上方為0,順時鐘方向)
//	int		h_mini;
//	int		v_mini;
//	//pattern使用
//	int		anim_chr_no;					//人物的編號(anim_tbl.h的編號)
//	int		anim_chr_no_bak;				//上一次的人物編號
//	int		anim_no;						//人物的動作編號
//	int		anim_no_bak;					//上一次的人物編號
//	int		anim_ang;						//動作的方向(0~7)(下0)
//	int		anim_ang_bak;					//上一次的方向
//	int		anim_cnt;						//第幾張frame
//	int		anim_frame_cnt;					//這張frame停留時間
//	int		anim_x;							//X座標(Sprbin+Adrnbin)
//	int		anim_y;							//Y座標(Sprbin+Adrnbin)
//	int		anim_hit;
//	// shan add +1
//	char    fmname[33];			            // 家族名稱
//	// Robin 0728 ride Pet
//	int		onRide;
//	char	petName[16 + 1];
//	int		petLevel;
//	int		petHp;
//	int		petMaxHp;
//	int		petDamage;
//	int		petFall;
//#ifdef _MIND_ICON
//	unsigned int sMindIcon;
//#endif
//#ifdef _SHOWFAMILYBADGE_
//	unsigned int sFamilyIcon;
//#endif
//#ifdef FAMILY_MANOR_
//	unsigned int mFamilyIcon;
//#endif
//#ifdef _CHAR_MANOR_
//	unsigned int mManorIcon;
//#endif
//#ifdef _CHARTITLE_STR_
//	TITLE_STR TitleText;
//#endif
//#ifdef _CHARTITLE_
//	unsigned int TitleIcon;
//#endif
//#ifdef _NPC_EVENT_NOTICE
//	int noticeNo;
//#endif
//
//#ifdef _SKILL_ROAR
//	int		petRoar;		//大吼(克年獸)
//#endif
//#ifdef _SKILL_SELFEXPLODE //自爆
//	int		petSelfExplode;
//#endif
//#ifdef _MAGIC_DEEPPOISION   //劇毒
//	int		petDeepPoision;
//#endif
//
//#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
//	int		profession_class;
//#endif
//	//#ifdef _BATTLESKILL				// (不可開) Syu ADD 戰鬥技能介面
//	int		warrioreffect;
//	//#endif
//#ifdef _GM_IDENTIFY		// Rog ADD GM識別
//	char gm_name[33];
//#endif
//#ifdef _STREET_VENDOR
//	char szStreetVendorTitle[64];
//#endif
//#ifdef _NPC_PICTURE
//	int picture;
//	int picturetemp;
//#endif
//#ifdef _PETSKILL_RIDE
//	int saveride;
//#endif
//#ifdef _MOUSE_DBL_CLICK
//	int index;	// 禁斷!! Server中的charaindex
//#endif
//
//#ifdef _SFUMATO
//	int sfumato;		// 二次渲染圖層色彩
//#endif
//}ACTION;

typedef struct customdialog_s
{
	int x = 0;
	int y = 0;
	BUTTON_TYPE button = BUTTON_NOTUSED;
	int row = 0;
} customdialog_t;

typedef struct tagLSTIME
{
	int year = 0;
	int day = 0;
	int hour = 0;
}LSTIME;

typedef struct tagITEM_BUFFER
{
	int 	x = 0, y = 0;
	int 	defX = 0, defY = 0;
	int 	bmpNo = 0;
	int 	dispPrio = 0;
	BOOL	dragFlag = FALSE;
	BOOL	mixFlag = FALSE;
}ITEM_BUFFER;

typedef struct tagMAIL_HISTORY
{
	QString 	str[MAIL_MAX_HISTORY];
	QString 	dateStr[MAIL_MAX_HISTORY];
	int 	noReadFlag[MAIL_MAX_HISTORY] = { 0 };
	int 	petLevel[MAIL_MAX_HISTORY] = { 0 };
	QString 	petName[MAIL_MAX_HISTORY];
	int 	itemGraNo[MAIL_MAX_HISTORY] = { 0 };
	int 	newHistoryNo = 0;

	void clear()
	{
		for (int i = 0; i < MAIL_MAX_HISTORY; ++i)
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
	int color = 0;
	int graNo = 0;
	int level = 0;
#ifdef _ITEM_PILENUMS
	int pile = 0;
#endif
#ifdef _ALCHEMIST //#ifdef _ITEMSET7_TXT
	QString alch;
#endif
	short useFlag = 0i16;
	short field = 0i16;
	short target = 0i16;
	short deadTargetFlag = 0i16;
	short sendFlag = 0i16;
	QString name = "";
	QString name2 = "";
	QString memo = "";
	QString damage = "";
#ifdef _PET_ITEM
	char type = '\0';
#endif
#ifdef _ITEM_JIGSAW
	QString jigsaw = "";
#endif
#ifdef _NPC_ITEMUP
	int itemup = 0;
#endif
#ifdef _ITEM_COUNTDOWN
	int counttime = 0;
#endif
#ifdef _MAGIC_ITEM_
	int 道具類型 = 0;
#endif

	//custom
	int maxPile = -1;
} ITEM;

typedef struct tagPC
{
	int graNo = 0;
	int faceGraNo = 0;
	int id = 0;
	int dir = 0;
	int hp = 0, maxHp = 0, hpPercent = 0;
	int mp = 0, maxMp = 0, mpPercent = 0;
	int vital = 0;
	int str = 0, tgh = 0, dex = 0;
	int exp = 0, maxExp = 0;
	int level = 0;
	int atk = 0, def = 0;
	int quick = 0, charm = 0, luck = 0;
	int earth = 0, water = 0, fire = 0, wind = 0;
	int gold = 0;
#ifdef _NEW_MANOR_LAW
	int fame = 0;
#endif
	int titleNo = 0;
	int dp = 0;
	QString name = "";
	QString freeName = "";
	short nameColor = 0i16;
#ifdef _ANGEL_SUMMON
	unsigned status = 0u;
#else
	unsigned short status = 0ui16;
#endif
	unsigned short etcFlag = 0ui16;
	short battlePetNo = -1i16;
	short selectPetNo[MAX_PET] = { 0i16, 0i16 ,0i16 ,0i16 ,0i16 };
	short mailPetNo = -1i16;
#ifdef _STANDBYPET
	short standbyPet = -1i16;
#endif
	int battleNo = 0;
	short sideNo = 0i16;
	short helpMode = 0i16;
	ITEM item[MAX_ITEM] = { 0 };
	//ACTION* ptAct;
	int pcNameColor = 0;
	short transmigration = 0i16;
	QString chusheng = "";
	QString familyName = "";
	int familyleader = 0;
	int channel = 0;
	int quickChannel = 0;
	int personal_bankgold = 0;
	int ridePetNo = -1;//寵物形像
	int learnride = 0;//學習騎乘
	unsigned int lowsride = 0u;
	QString ridePetName = "";
	int ridePetLevel = 0;
	int familySprite = 0;
	int baseGraNo = 0;
	ITEM itempool[MAX_ITEM] = {};
	int big4fm = 0;
	int trade_confirm = 0;         // 1 -> 初始值
	// 2 -> 慬我方按下確定鍵
	// 3 -> 僅對方按下確定鍵
	// 4 -> 雙方皆按下確定鍵

#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
	int profession_class = 0;
	int profession_level = 0;
	//	int profession_exp;
	int profession_skill_point = 0;
	QString profession_class_name = "";
#endif
#ifdef _ALLDOMAN // (不可開) Syu ADD 排行榜NPC
	int herofloor = 0;
#endif

#ifdef _GM_IDENTIFY		// Rog ADD GM識別
	QString gm_name = "";
#endif

#ifdef _FRIENDCHANNEL  // ROG ADD 好友頻道
	QString chatRoomNum = "";
#endif
#ifdef _STREET_VENDOR
	int iOnStreetVendor = 0;		// 擺攤模式
#endif
	int skywalker = 0; // GM天行者
#ifdef _MOVE_SCREEN
	BOOL	bMoveScreenMode = FALSE;	// 移動熒幕模式
	BOOL	bCanUseMouse = FALSE;		// 是否可以使用滑鼠移動
	int		iDestX = 0;				// 目標點 X 座標
	int		iDestY = 0;				// 目標點 Y 座標
#endif
#ifdef _THEATER
	int		iTheaterMode = 0;		// 劇場模式
	int		iSceneryNumber = 0;		// 記錄劇院背景圖號
	//ACTION* pActNPC[5];		// 記錄劇場中臨時產生出來的NPC
#endif
#ifdef _NPC_DANCE
	int     iDanceMode = 0;			// 動一動模式
#endif
#ifdef _EVIL_KILL
	int     newfame = 0; // 討伐魔軍積分
	short   ftype = 0i16;
#endif

	int debugmode = 0;
#ifdef _SFUMATO
	int sfumato = 0;		// 二次渲染圖層色彩
#endif
#ifdef _NEW_ITEM_
	int 道具欄狀態 = 0;
#endif
#ifdef _CHARSIGNADY_NO_
	int 簽到標記 = 0;
#endif
#ifdef _MAGIC_ITEM_
	int 法寶道具狀態 = 0;
	int 道具光環效果 = 0;
#endif
	//custom
	int maxload = -1;
} PC;

constexpr int DEF_PAL = 0;
typedef struct tagPALETTE_STATE
{
	int palNo = 0;
	int time = 0;
	int flag = 0;
}PALETTE_STATE;

typedef struct tagPET
{
	int index = 0;						//位置
	int graNo = 0;						//圖號
	int hp = 0, maxHp = 0, hpPercent = 0;					//血量
	int mp = 0, maxMp = 0, mpPercent = 0;					//魔力
	int exp = 0, maxExp = 0;				//經驗值
	int level = 0;						//等級
	int atk = 0;						//攻擊力
	int def = 0;						//防禦力
	int quick = 0;						//速度
	int ai = 0;							//AI
	int earth = 0, water = 0, fire = 0, wind = 0;
	int maxSkill = 0;
	int trn = 0;						// 寵物轉生數
#ifdef _SHOW_FUSION
	int fusion = 0;						// low word: 寵蛋旗標, hi word: 物種編碼
#endif
#ifdef _ANGEL_SUMMON
	unsigned status = 0u;
#else
	unsigned short status = 0ui16;
#endif
	QString name = "";
	QString freeName = "";
	short useFlag = 0i16;
	short changeNameFlag = 0i16;
#ifdef _PET_ITEM
	ITEM item[MAX_PET_ITEM] = {};		// 寵物道具
#endif
#ifdef _PETCOM_
	int oldlevel = 0, oldhp = 0, oldatk = 0, oldquick = 0, olddef = 0;
#endif
#ifdef _RIDEPET_
	int rideflg;
#endif
#ifdef _PETBLESS_
	int blessflg = 0;
	int blesshp = 0;
	int blessatk = 0;
	int blessquick = 0;
	int blessdef = 0;
#endif

	//custom
	PetState state = PetState::kNoneState;
	qreal power = 0.0;
} PET;


typedef struct tagMAGIC
{
	short useFlag = 0i16;
	int mp = 0;
	short field = 0i16;
	short target = 0i16;
	short deadTargetFlag = 0i16;
	QString name = "";
	QString memo = "";
} MAGIC;


typedef struct tagPARTY
{
	short useFlag = 0i16;
	int id = 0;
	int level = 0;
	int maxHp = 0;
	int hp = 0;
	int hpPercent = 0;
	int mp = 0;
	QString name = "";
	//ACTION* ptAct;
} PARTY;


typedef struct tagADDRESS_BOOK
{
	short useFlag = 0i16;
	short onlineFlag = 0i16;
	int level = 0;
	short transmigration = 0i16;
	int dp = 0;
	int graNo = 0;
	QString name = "";
#ifdef _MAILSHOWPLANET				// (可開放) Syu ADD 顯示名片星球
	QString planetname = "";
#endif
} ADDRESS_BOOK;


typedef struct tagBATTLE_RESULT_CHR
{
	short petNo = 0i16;
	short levelUp = 0i16;
	int exp = 0;
} BATTLE_RESULT_CHR;

typedef struct tagBATTLE_RESULT_MSG
{
	short useFlag = 0i16;
	BATTLE_RESULT_CHR resChr[RESULT_CHR_EXP] = {};
	QString item[RESULT_ITEM_COUNT] = {};
} BATTLE_RESULT_MSG;


typedef struct tagPET_SKILL
{
	short useFlag = 0i16;
	short skillId = 0i16;
	short field = 0i16;
	short target = 0i16;
	QString name = "";
	QString memo = "";
} PET_SKILL;

#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
typedef struct tagPROFESSION_SKILL
{
	short useFlag = 0i16;
	short skillId = 0i16;
	short target = 0i16;
	short kind = 0i16;
	QString name = "";
	QString memo = "";
	int icon = 0;
	int costmp = 0;
	int skill_level = 0;
#ifdef _PRO3_ADDSKILL
	int cooltime = 0;
#endif
} PROFESSION_SKILL;
#endif


typedef struct tagCHARLISTTABLE
{
	QString name = "";
	short level = 0i16;
	int login = 0;

	int faceGraNo = 0;
	int hp = 0;
	int str = 0;
	int def = 0;
	int agi = 0;
	int app = 0;
	int attr[4] = { 0,0,0,0 };
	int dp = 0;
} CHARLISTTABLE;
#ifdef _AIDENGLU_
typedef struct
{
	int 大區 = 0;
	int 隊模 = 0;
	int 小區 = 0;
	int 人物 = 0;
	int 是否自動喊話 = 0;
	int 是否自動遇敵 = 0;
	int 人物方向 = 0;
	char 登陸人物名稱[4][32] = { 0 };
	int 登陸延時時間 = 0;
}Landed;
#endif

constexpr int MAXMISSION = 300;
typedef struct tagJOBDAILY
{
	int JobId = 0;								// 任務編號
	QString explain = "";						// 任務說明
	QString state = "";							// 狀態
}JOBDAILY;

struct showitem
{
	QString name;
	QString freename;
	QString graph;
	QString effect;
	QString color;
	QString itemindex;
	QString damage;
};

typedef struct SPetItemInfo
{
	int bmpNo = 0;										// 图号
	int color = 0;										// 文字颜色
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
	int windowtype = 0;
	int buttontype = 0;
	int seqno = 0;
	int objindex = 0;
	QString data = "";
	QStringList linedatas;
	QStringList linebuttontext;
}dialog_t;


typedef struct battleobject_s
{
	int pos = -1;
	QString name = "";
	QString freename = "";
	int faceid = 0;
	int level = 0;
	int hp = 0;
	int maxHp = 0;
	int hpPercent = 0;
	int status = 0;
	int rideFlag = 0;
	QString rideName = 0;
	int rideLevel = 0;
	int rideHp = 0;
	int rideMaxHp = 0;
	int rideHpPercent = 0;
	bool ready = false;
} battleobject_t;

typedef struct battledata_s
{
	int fieldAttr = 0;
	int alliemin = 0, alliemax = 0, enemymax = 0, enemymin = 0;
	battleobject_t player = {};
	battleobject_t pet = {};
	QVector<battleobject_t> objects;
	QVector<battleobject_t> allies;
	QVector<battleobject_t> enemies;
	bool charAlreadyAction = false;
	bool petAlreadyAction = false;
} battledata_t;


typedef struct mapunit_s
{
	CHAR_TYPE type = CHAR_TYPENONE;
	int id = 0;
	int graNo = 0;
	int x = 0;
	int y = 0;
	QPoint p;
	int dir = 0;
	int level = 0;
	int nameColor = 0;
	QString name = "";
	QString freeName = "";
	bool walkable = false;
	int height = 0;
	int charNameColor = 0;
	QString fmname = "";
	QString petname = "";
	int petlevel = 0;
	int classNo = 0;
	QString item_name = "";
	int gold = 0;
	int profession_class = 0;
	int profession_level = 0;
	int profession_skill_point = 0;
	util::ObjectType objType = util::ObjectType::OBJ_UNKNOWN;
	bool isvisible = false;
	CHR_STATUS status = CHR_STATUS::CHR_STATUS_NONE;
} mapunit_t;

typedef struct bankpet_s
{
	int level = 0;
	int maxHp = 0;
	QString name;
}bankpet_t;

typedef struct currencydata_s
{
	int expbufftime = 0;
	int prestige = 0;      // 聲望
	int energy = 0;        // 氣勢
	int shell = 0;         // 貝殼
	int vitality = 0;      // 活力
	int points = 0;        // 積分
	int VIPPoints = 0;  // 會員點
} currencydata_t;

#pragma endregion
