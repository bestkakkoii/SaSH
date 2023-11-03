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
#include <QString>
#include <QByteArray>
#include <QHash>
#include <QPoint>

namespace sa
{
#pragma region Const
	constexpr long long MAX_TIMEOUT = 10000;

	constexpr size_t LINEBUFSIZ = 8192u;

	//constexpr long long MAX_CHARACTER = 4;
	constexpr long long MAX_CHARACTER = 2;

	constexpr long long CHAR_NAME_LEN = 16;
	constexpr long long CHAR_FREENAME_LEN = 12;
	constexpr long long MAGIC_NAME_LEN = 28;
	constexpr long long MAGIC_MEMO_LEN = 72;
	constexpr long long ITEM_NAME_LEN = 28;
	constexpr long long ITEM_NAME2_LEN = 16;
	constexpr long long ITEM_MEMO_LEN = 84;
	constexpr long long PET_NAME_LEN = 16;
	constexpr long long PET_FREENAME_LEN = 12;
	constexpr long long CHAR_FMNAME_LEN = 33;     // 家族名稱

	//人物職業
	constexpr long long PROFESSION_MEMO_LEN = 84;
	//GM識別
	constexpr long long GM_NAME_LEN = 32;

	constexpr long long CHARNAMELEN = 256;

	constexpr long long MAX_PET = 5;

	constexpr long long MAX_MAGIC = 9;

	constexpr long long MAX_PETSKILL = 7;

	constexpr long long MAX_PARTY = 5;

	constexpr long long MAX_ADR_BOOK_COUNT = 4;

	//constexpr long long MAX_ADR_BOOK_PAGE = 20;//20  //10   20050214 cyg 10 add to 20
	constexpr long long MAX_ADR_BOOK_PAGE = 10;

	constexpr long long MAX_ADDRESS_BOOK = (MAX_ADR_BOOK_COUNT * MAX_ADR_BOOK_PAGE);

	//constexpr long long MAX_PROFESSION_SKILL = 30;
	constexpr long long MAX_PROFESSION_SKILL = 26;

	constexpr long long BATTLE_BUF_SIZE = 4;
	constexpr long long BATTLE_COMMAND_SIZE = 4096;

	constexpr long long FLOOR_NAME_LEN = 24;

	constexpr long long RESULT_ITEM_COUNT = 3;
	constexpr long long RESULT_ITEM_NAME_LEN = 24;
	//constexpr long long RESULT_CHR_EXP = 4;
	constexpr long long RESULT_CHR_EXP = 5;

	constexpr long long SKILL_NAME_LEN = 24;
	constexpr long long SKILL_MEMO_LEN = 72;
	constexpr long long MAX_SKILL = 7;

	constexpr long long MIN_HP_PERCENT = 10;

	constexpr long long MAX_GOLD = 1000000;
	constexpr long long MAX_BANKGOLD = 10000000;
	constexpr long long MAX_FMBANKGOLD = 100000000;


	constexpr long long MAX_PERSONAL_BANKGOLD = 50000000;

	constexpr long long FAMILY_MAXMEMBER = 100;    // 家族人數
	//constexpr long long FAMILY_MAXMEMBER = 50;    // 家族人數

	constexpr long long MAP_TILE_GRID_X1 = -20;
	constexpr long long MAP_TILE_GRID_X2 = +17;
	constexpr long long MAP_TILE_GRID_Y1 = -16;
	constexpr long long MAP_TILE_GRID_Y2 = +21;
	constexpr long long MAP_X_SIZE = (MAP_TILE_GRID_X2 - MAP_TILE_GRID_X1);
	constexpr long long MAP_Y_SIZE = (MAP_TILE_GRID_Y2 - MAP_TILE_GRID_Y1);

	constexpr long long MAP_READ_FLAG = 0x8000;
	constexpr long long MAP_SEE_FLAG = 0x4000;

	constexpr long long TARGET_SIDE_0 = 20;	// 右下
	constexpr long long TARGET_SIDE_1 = 21;	// 左上
	constexpr long long TARGET_ALL = 22;// 全體
	constexpr long long TARGET_SIDE_0_B_ROW = 26;  // 右下後一列
	constexpr long long TARGET_SIDE_0_F_ROW = 25;  // 右下前一列
	constexpr long long TARGET_SIDE_1_F_ROW = 24;  // 左上前一列
	constexpr long long TARGET_SIDE_1_B_ROW = 23;  // 左上後一列
	constexpr long long TARGER_THROUGH = 27;

	constexpr long long BATTLE_BP_JOIN = (1 << 0);//等待回合结束
	constexpr long long BATTLE_BP_PLAYER_MENU_NON = (1 << 1);//觀戰
	constexpr long long BATTLE_BP_BOOMERANG = (1 << 2);//迴旋鏢
	constexpr long long BATTLE_BP_PET_MENU_NON = (1 << 3);//觀戰
	constexpr long long BATTLE_BP_ENEMY_SURPRISAL = (1 << 4);//遭遇偷襲
	constexpr long long BATTLE_BP_PLAYER_SURPRISAL = (1 << 5);//出奇不意

	constexpr long long BC_FLG_NEW = (1LL << 0);
	constexpr long long BC_FLG_DEAD = (1LL << 1);	  //死亡
	constexpr long long BC_FLG_PLAYER = (1LL << 2);	  //玩家,玩家有異常狀態時要有此值
	constexpr long long BC_FLG_POISON = (1LL << 3);	  //中毒
	constexpr long long BC_FLG_PARALYSIS = (1LL << 4); //麻痹
	constexpr long long BC_FLG_SLEEP = (1LL << 5);  //昏睡
	constexpr long long BC_FLG_STONE = (1LL << 6);	  //石化
	constexpr long long BC_FLG_DRUNK = (1LL << 7);  //酒醉
	constexpr long long BC_FLG_CONFUSION = (1LL << 8); //混亂
	constexpr long long BC_FLG_HIDE = (1LL << 9);	  //是否隱藏，地球一周
	constexpr long long BC_FLG_REVERSE = (1LL << 10);  //反轉
	constexpr long long BC_FLG_WEAKEN = (1 << 11); // 虛弱
	constexpr long long BC_FLG_DEEPPOISON = (1LL << 12); // 劇毒
	constexpr long long BC_FLG_BARRIER = (1LL << 13); // 魔障
	constexpr long long BC_FLG_NOCAST = (1LL << 14); // 沈默
	constexpr long long BC_FLG_SARS = (1LL << 15); // 毒煞蔓延
	constexpr long long BC_FLG_DIZZY = (1LL << 16);	// 暈眩
	constexpr long long BC_FLG_ENTWINE = (1LL << 17);	// 樹根纏繞
	constexpr long long BC_FLG_DRAGNET = (1LL << 18);	// 天羅地網
	constexpr long long BC_FLG_ICECRACK = (1LL << 19);	// 冰爆術
	constexpr long long BC_FLG_OBLIVION = (1LL << 20);	// 遺忘
	constexpr long long BC_FLG_ICEARROW = (1LL << 21);	// 冰箭
	constexpr long long BC_FLG_BLOODWORMS = (1LL << 22);	// 嗜血蠱
	constexpr long long BC_FLG_SIGN = (1LL << 23);	// 一針見血
	constexpr long long BC_FLG_CARY = (1LL << 24); // 挑撥
	constexpr long long BC_FLG_F_ENCLOSE = (1LL << 25); // 火附體
	constexpr long long BC_FLG_I_ENCLOSE = (1LL << 26); // 冰附體
	constexpr long long BC_FLG_T_ENCLOSE = (1LL << 27); // 雷附體
	constexpr long long BC_FLG_WATER = (1LL << 28); // 水附體
	constexpr long long BC_FLG_FEAR = (1LL << 29); // 恐懼
	constexpr long long BC_FLG_CHANGE = (1LL << 30); // 雷爾變身


	inline constexpr bool hasBadStatus(unsigned long long status)
	{
		if (status & BC_FLG_DEAD) //死亡
			return true;
		if (status & BC_FLG_HIDE) // 是否隱藏，地球一周
			return true;
		if (status & BC_FLG_POISON) // 中毒
			return true;
		if (status & BC_FLG_PARALYSIS) // 麻痹
			return true;
		if (status & BC_FLG_SLEEP) // 昏睡
			return true;
		if (status & BC_FLG_STONE) // 石化
			return true;
		if (status & BC_FLG_DRUNK) // 酒醉
			return true;
		if (status & BC_FLG_CONFUSION) // 混亂
			return true;
		if (status & BC_FLG_WEAKEN) // 虛弱
			return true;
		if (status & BC_FLG_DEEPPOISON) // 劇毒
			return true;
		if (status & BC_FLG_BARRIER) // 魔障
			return true;
		if (status & BC_FLG_NOCAST) // 沈默
			return true;
		if (status & BC_FLG_SARS) // 毒煞蔓延
			return true;
		if (status & BC_FLG_DIZZY)	// 暈眩
			return true;
		if (status & BC_FLG_ENTWINE)	// 樹根纏繞
			return true;
		if (status & BC_FLG_DRAGNET)	// 天羅地網
			return true;
		if (status & BC_FLG_ICECRACK)	// 冰爆術
			return true;
		if (status & BC_FLG_OBLIVION)	// 遺忘
			return true;
		if (status & BC_FLG_ICEARROW)	// 冰箭
			return true;
		if (status & BC_FLG_BLOODWORMS)	// 嗜血蠱
			return true;
		if (status & BC_FLG_SIGN)	// 一針見血
			return true;
		if (status & BC_FLG_CARY) // 挑撥
			return true;
		//if (status & BC_FLG_F_ENCLOSE) // 火附體
		//	return true;
		//if (status & BC_FLG_I_ENCLOSE) // 冰附體
		//	return true;
		//if (status & BC_FLG_T_ENCLOSE) // 雷附體
		//	return true;
		//if (status & BC_FLG_WATER) // 水附體
		//	return true;
		if (status & BC_FLG_FEAR) // 恐懼
			return true;
		//if (status & BC_FLG_CHANGE) // 雷爾變身
		//	return true;
		return false;
	}

	inline constexpr bool hasUnMoveableStatus(unsigned long long status)
	{
		if (status & BC_FLG_DEAD) //死亡
			return true;
		if (status & BC_FLG_STONE) // 石化
			return true;
		if (status & BC_FLG_SLEEP) // 昏睡
			return true;
		if (status & BC_FLG_PARALYSIS) // 麻痹=
			return true;
		if (status & BC_FLG_HIDE) // 是否隱藏，地球一周
			return true;
		if (status & BC_FLG_DIZZY) // 暈眩
			return true;
		if (status & BC_FLG_BARRIER) // 魔障
			return true;
		if (status & BC_FLG_DRAGNET)	// 天羅地網
			return true;
		return false;
	}

	constexpr long long ITEM_FLAG_PET_MAIL = (1LL << 0);
	constexpr long long ITEM_FLAG_MIX = (1LL << 1);
	constexpr long long ITEM_FLAG_COOKING_MIX = (1LL << 2);
	constexpr long long ITEM_FLAG_METAL_MIX = (1LL << 3);	//金屬
	constexpr long long ITEM_FLAG_JEWEL_MIX = (1LL << 4);	//寶石
	constexpr long long ITEM_FLAG_FIX_MIX = (1LL << 5);	//修理
	constexpr long long ITEM_FLAG_INTENSIFY_MIX = (1LL << 6);	//強化
	constexpr long long ITEM_FLAG_UPINSLAY_MIX = (1LL << 7);	//鑿孔

	constexpr long long JOY_RIGHT = (1LL << 15);/* Right Key				*/
	constexpr long long JOY_LEFT = (1LL << 14);/*  Left Key				*/
	constexpr long long JOY_DOWN = (1LL << 13);/*  Down Key				*/
	constexpr long long JOY_UP = (1LL << 12);	/*    Up Key				*/
	constexpr long long JOY_START = (1LL << 11);	/* Start					*/
	constexpr long long JOY_A = (1LL << 10);	/* A Trigger				*/
	constexpr long long JOY_C = (1LL << 9);/* C Trigger				*/
	constexpr long long JOY_B = (1LL << 8);	/* B Trigger				*/
	constexpr long long JOY_R = (1LL << 7);	/* R Trigger				*/
	constexpr long long JOY_X = (1LL << 6);	/* X Trigger				*/
	constexpr long long JOY_DEL = (1LL << 5);	/* DELETE					*/
	constexpr long long JOY_INS = (1LL << 4);	/* INSERT					*/
	constexpr long long JOY_END = (1LL << 3);/* END						*/
	constexpr long long JOY_HOME = (1LL << 2);	/* HOME						*/
	constexpr long long JOY_P_DOWN = (1LL << 1);	/* PAGE_UP					*/
	constexpr long long JOY_P_UP = (1LL << 0);	/* PAGE_DOWN				*/

	constexpr long long JOY_ESC = (1LL << 31);/* ESC Key					*/
	constexpr long long JOY_CTRL_M = (1LL << 30);	/* Ctrl + M					*/
	constexpr long long JOY_CTRL_S = (1LL << 29);	/* Ctrl + S					*/
	constexpr long long JOY_CTRL_P = (1LL << 28);	/* Ctrl + P					*/
	constexpr long long JOY_CTRL_I = (1LL << 27);	/* Ctrl + I					*/
	constexpr long long JOY_CTRL_E = (1LL << 26);	/* Ctrl + E					*/
	constexpr long long JOY_CTRL_A = (1LL << 25);	/* Ctrl + A					*/

	constexpr long long JOY_CTRL_C = (1LL << 24);	/* Ctrl + C					*/
	constexpr long long JOY_CTRL_V = (1LL << 23);	/* Ctrl + V					*/
	constexpr long long JOY_CTRL_T = (1LL << 22);	/* Ctrl + T					*/

	constexpr long long MAIL_STR_LEN = 140;
	constexpr long long MAIL_DATE_STR_LEN = 20;
	constexpr long long MAIL_MAX_HISTORY = 20;
	constexpr long long MAX_CHAT_REGISTY_STR = 8;

	constexpr long long MAX_ENEMY = 20;
	constexpr long long MAX_CHAT_HISTORY = 20;
	constexpr long long MAX_DIALOG_LINE = 200;

	constexpr long long MAX_DIR = 8;

	constexpr const char* SUCCESSFULSTR = "successful";
	constexpr const char* FAILEDSTR = "failed";
	constexpr const char* OKSTR = "ok";
	constexpr const char* CANCLE = "cancle";

	constexpr long long BATTLE_MAP_FILES = 220;
	constexpr long long GRID_SIZE = 64;

	constexpr long long LSTIME_SECONDS_PER_DAY = 5400LL;
	constexpr long long LSTIME_HOURS_PER_DAY = 1024LL;
	constexpr long long LSTIME_DAYS_PER_YEAR = 100LL;

#if 0
	constexpr long long  NIGHT_TO_MORNING = 906;
	constexpr long long  MORNING_TO_NOON = 1006;
	constexpr long long  NOON_TO_EVENING = 356;
	constexpr long long  EVENING_TO_NIGHT = 456;
#else
	constexpr long long NIGHT_TO_MORNING = 700;
	constexpr long long MORNING_TO_NOON = 930;
	constexpr long long NOON_TO_EVENING = 200;
	constexpr long long EVENING_TO_NIGHT = 300;
#endif
#pragma endregion

#pragma region Enums
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
		kOffsetChatBufferMaxPointer = 0x146278,
		kOffsetChatBufferMaxCount = 0x14A4F8,
		kOffestMouseClick = 0x41F1BC4,
		kOffestMouseX = 0x41F1B98,
		kOffestMouseY = 0x41F1B9C,

		//item
		kItemStructSize = 388,
		kOffestItemValid = 0x422C028,
		kOffsetItemName = 0x422C032,
		kOffsetItemMemo = 0x422C060,
		kOffsetItemDurability = 0x422C0B5,
		kOffsetItemStack = 0x422BF58,

		//dialog
		kOffsetDialogType = 0xB83EC,
		kOffsetGameInjectState = 0x4200000,//Custom
		kOffsetDialogValid = 0x4200004,//Custom
	};

	enum CraftType
	{
		kCraftFood,
		kCraftItem
	};

	enum ObjectType
	{
		OBJ_UNKNOWN,
		OBJ_ROAD,
		OBJ_UP,
		OBJ_DOWN,
		OBJ_JUMP,
		OBJ_WARP,
		OBJ_WALL,
		OBJ_ROCK,
		OBJ_ROCKEX,
		OBJ_BOUNDARY,
		OBJ_WATER,
		OBJ_EMPTY,
		OBJ_HUMAN,
		OBJ_NPC,
		OBJ_BUILDING,
		OBJ_ITEM,
		OBJ_PET,
		OBJ_GOLD,
		OBJ_GM,
		OBJ_MAX,
	};

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

		LSSPROTO_MA_SEND = 98,

		LSSPROTO_DM_SEND = 99,   // 玩家抽地圖送監獄,
		LSSPROTO_IC_RECV = 100,
		LSSPROTO_NC_RECV = 101,

		LSSPROTO_CS_SEND = 103,
		LSSPROTO_CS_RECV = 104,

		LSSPROTO_KTEAM_SEND = 106,

		LSSPROTO_PETST_RECV = 107,

		//新增Protocol要求細項,
		LSSPROTO_RESIST_SEND = 108,
		LSSPROTO_RESIST_RECV = 109,
		//非戰鬥時技能Protocol,
		LSSPROTO_BATTLESKILL_SEND = 110,
		LSSPROTO_BATTLESKILL_RECV = 111,

		LSSPROTO_CHATROOM_SEND = 112,
		LSSPROTO_CHATROOM_RECV = 113,

		LSSPROTO_SPET_SEND = 114,		// Robin 待機寵,
		LSSPROTO_SPET_RECV = 115,

		LSSPROTO_STREET_VENDOR_SEND = 116,		// 擺攤功能,
		LSSPROTO_STREET_VENDOR_RECV = 117,

		LSSPROTO_JOBDAILY_SEND = 121,		// CYG　任務日志功能,
		LSSPROTO_JOBDAILY_RECV = 120,

		LSSPROTO_TEACHER_SYSTEM_SEND = 122,		// 導師功能,
		LSSPROTO_TEACHER_SYSTEM_RECV = 123,

		LSSPROTO_S2_SEND = 124,
		LSSPROTO_S2_RECV = 125,

		LSSPROTO_FIREWORK_RECV = 126,

		LSSPROTO_PET_ITEM_EQUIP_SEND = 127,

		LSSPROTO_MOVE_SCREEN_RECV = 128,

		LSSPROTO_HOSTNAME_SEND = 129,
		LSSPROTO_HOSTNAME_RECV = 130,

		LSSPROTO_THEATER_DATA_RECV = 131,
		LSSPROTO_THEATER_DATA_SEND = 132,

		LSSPROTO_MAGICCARD_ACTION_RECV = 133,
		LSSPROTO_MAGICCARD_DAMAGE_RECV = 134,

		LSSPROTO_SECONDARY_WINDOW_RECV = 137,

		LSSPROTO_TRUNTABLE_RECV = 138,

		LSSPROTO_ALCHEPLUS_SEND = 135,
		LSSPROTO_ALCHEPLUS_RECV = 136,

		LSSPROTO_DANCEMAN_OPTION_RECV = 137,

		LSSPROTO_HUNDREDKILL_RECV = 138,

		LSSPROTO_PKLIST_SEND = 139,
		LSSPROTO_PKLIST_RECV = 140,

		LSSPROTO_SIGNDAY_SEND = 141,

		LSSPROTO_CHAREFFECT_RECV = 146,

		LSSPROTO_REDMEMOY_SEND = 147,
		LSSPROTO_REDMEMOY_RECV = 148,

		LSSPROTO_IMAGE_RECV = 151,

		LSSPROTO_DENGON_RECV = 200,

		LSSPROTO_SAMENU_RECV = 201,
		LSSPROTO_SAMENU_SEND = 202,

		LSSPROTO_SHOPOK_SEND = 208,
		LSSPROTO_SHOPOK_RECV = 209,

		LSSPROTO_FAMILYBADGE_SEND = 210,
		LSSPROTO_FAMILYBADGE_RECV = 211,

		LSSPROTO_CHARTITLE_SEND = 212,
		LSSPROTO_CHARTITLE_RECV = 213,

		LSSPROTO_VB_SEND = 218,
		LSSPROTO_VB_RECV = 219,

		//騎寵查詢,
		LSSPROTO_RIDEQUERY_SEND = 220,

		LSSPROTO_PETSKINS_SEND = 221,
		LSSPROTO_PETSKINS_RECV = 222,

		LSSPROTO_JOINTEAM_SEND = 239,
		LSSPROTO_ARRAGEITEM_SEND = 260,
	};

	typedef enum tagCHAR_EquipPlace
	{
		CHAR_EQUIPNONE = -1,
		PET_EQUIPNONE = -1,
		CHAR_HEAD,
		CHAR_BODY,
		CHAR_ARM,
		CHAR_DECORATION1,
		CHAR_DECORATION2,

		CHAR_EQBELT,
		CHAR_EQSHIELD,
		CHAR_EQSHOES,


		CHAR_EQGLOVE,

		CHAR_EQUIPPLACENUM,

		PET_HEAD = 0,	// 頭
		PET_WING,		// 翼
		PET_TOOTH,		// 牙
		PET_PLATE,		// 身體
		PET_BACK,		// 背
		PET_CLAW,		// 爪
		PET_FOOT,		// 腳(鰭)
		PET_EQUIPNUM

	}CHAR_EquipPlace;

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
	constexpr long long MAX_PET_ITEM = ITEM_CATEGORYNUM - ITEM_PET_HEAD;

	constexpr long long MAX_ITEMSTART = CHAR_EQUIPPLACENUM;
	constexpr long long MAX_MAXHAVEITEM = 15;

	//constexpr long long MAX_ITEM(MAX_ITEMSTART + MAX_MAXHAVEITEM * 3);
	//long long 判斷玩家道具數量();
	constexpr long long MAX_ITEM = (MAX_ITEMSTART + MAX_MAXHAVEITEM);

	//constexpr long long MAX_ITEMSTART = 5;
	//constexpr long long MAX_ITEM = 20;

	enum ETCFLAG
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

		CHR_STATUS_ANGEL = 0x10000			// 使者任務中
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

		MAGIC_TARGET_SINGLE,				// 針對敵方某一方
		MAGIC_TARGET_ONE_ROW,				// 針對敵方某一列
		MAGIC_TARGET_ALL_ROWS,				// 針對敵方所有人
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
		PETSKILL_TARGET_WITHOUTMYSELFANDPET,
		// 戰鬥技能介面
		PETSKILL_TARGET_ONE_ROW,
		PETSKILL_TARGET_ONE_LINE,
		PETSKILL_TARGER_DEATH,

		PETSKILL_TARGET_ONE_ROW_ALL, //選我方的單排
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

		ITEM_TARGET_PET
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
		BUTTON_CLOSE = -1,
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
#pragma pack(8)
	//用於掛機訊息紀錄
	typedef struct tagAfkRecorder
	{
		long long levelrecord = 0;
		long long leveldifference = 0;
		long long exprecord = 0;
		long long expdifference = 0;
		long long goldearn = 0;
		long long deadthcount = 0;
		long long reprecord = 0;
		long long repearn = 0;
		bool deadthcountflag = false;
	}AfkRecorder;

	typedef struct customdialog_s
	{
		long long x = 0;
		long long y = 0;
		BUTTON_TYPE button = BUTTON_NOTUSED;
		long long row = 0;
		long long rawbutton = 0;
		QString rawdata;
	} customdialog_t;

	typedef struct tagLSTIME
	{
		long long year = 0;
		long long day = 0;
		long long hour = 0;
	}LSTIME;

	typedef struct tagMAIL_HISTORY
	{
		long long 	noReadFlag[MAIL_MAX_HISTORY] = {};
		long long 	petLevel[MAIL_MAX_HISTORY] = {};
		long long 	itemGraNo[MAIL_MAX_HISTORY] = {};
		long long 	newHistoryNo = 0;
		QString str[MAIL_MAX_HISTORY];
		QString dateStr[MAIL_MAX_HISTORY];
		QString petName[MAIL_MAX_HISTORY];

		void clear()
		{
			for (long long i = 0; i < MAIL_MAX_HISTORY; ++i)
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
		long long color = 0;
		long long modelid = 0;
		long long level = 0;
		long long stack = 0;
		long long type = 0ui16;
		long long field = 0;
		long long target = 0;
		long long deadTargetFlag = 0;
		long long sendFlag = 0;
		long long itemup = 0;
		long long counttime = 0;
		long long damage = 0;
		//custom
		long long maxStack = -1;
		long long count = 0;

		QString name = "";
		QString name2 = "";
		QString memo = "";

		QString alch = 0; // #ifdef _ITEMSET7_TXT_ALCHEMIST
		QString jigsaw = "";

		//long long 道具類型 = 0;
		std::string getName() const { return util::toConstData(name); }
		std::string getName2() const { return util::toConstData(name2); }
		std::string getMemo() const { return util::toConstData(memo); }

	} ITEM;

	typedef struct tagPC
	{
		long long selectPetNo[MAX_PET] = { 0, 0, 0, 0, 0 };
		long long battlePetNo = -1;
		long long mailPetNo = -1;
		long long standbyPet = -1;
		long long ridePetNo = -1;

		long long modelid = 0;
		long long faceid = 0;
		long long id = 0;
		long long dir = 0;
		long long hp = 0, maxHp = 0, hpPercent = 0;
		long long mp = 0, maxMp = 0, mpPercent = 0;
		long long vit = 0;
		long long str = 0, tgh = 0, dex = 0;
		long long exp = 0, maxExp = 0;
		long long level = 0;
		long long atk = 0, def = 0;
		long long agi = 0, chasma = 0, luck = 0;
		long long earth = 0, water = 0, fire = 0, wind = 0;
		long long gold = 0;
		long long fame = 0;
		long long titleNo = 0;
		long long dp = 0;
		long long nameColor = 0;
		long long status = 0;
		long long etcFlag = 0;
		long long battleNo = 0;
		long long sideNo = 0i16;
		long long helpMode = 0i16;
		long long pcNameColor = 0;
		long long transmigration = 0;

		long long familyleader = 0;
		long long channel = 0;
		long long quickChannel = 0;
		long long personal_bankgold = 0;
		long long learnride = 0;//學習騎乘
		long long lowsride = 0u;
		long long ridePetLevel = 0;
		long long familySprite = 0;
		long long baseGraNo = 0;
		long long big4fm = 0;
		long long trade_confirm = 0;         // 1 -> 初始值
		long long profession_class = 0;
		long long profession_level = 0;
		long long profession_exp = 0;
		long long profession_skill_point = 0;

		long long herofloor = 0;// (不可開)排行榜NPC
		long long iOnStreetVendor = 0;		// 擺攤模式
		long long skywalker = 0; // GM天行者
		long long iTheaterMode = 0;		// 劇場模式
		long long iSceneryNumber = 0;		// 記錄劇院背景圖號

		long long iDanceMode = 0;			// 動一動模式
		long long newfame = 0; // 討伐魔軍積分
		long long ftype = 0;

		//custom
		long long maxload = -1;
		long long point = 0;

		QString name = "";
		QString freeName = "";
		QString ridePetName = "";
		QString profession_class_name = "";
		QString gm_name = "";	// Rog ADD GM識別
		QString chatRoomNum = "";// ROG ADD 好友頻道
		QString chusheng = "";
		QString family = "";

		std::string getName() const { return util::toConstData(name); }
		std::string getFreeName() const { return util::toConstData(freeName); }
		std::string getRidePetName() const { return util::toConstData(ridePetName); }
		std::string getProfessionClassName() const { return util::toConstData(profession_class_name); }
		std::string getGmName() const { return util::toConstData(gm_name); }
		std::string getChatRoomNum() const { return util::toConstData(chatRoomNum); }
		std::string getChusheng() const { return util::toConstData(chusheng); }
		std::string getFamily() const { return util::toConstData(family); }

		//以下棄用
		//ITEM item[MAX_ITEM] = {};
		//ITEM itempool[MAX_ITEM] = {};
		// 2 -> 慬我方按下確定鍵
		// 3 -> 僅對方按下確定鍵
		// 4 -> 雙方皆按下確定鍵
		//ACTION* pActNPC[5];		// 記錄劇場中臨時產生出來的NPC
		//long long 道具欄狀態 = 0;
		//long long 簽到標記 = 0;
		//long long 法寶道具狀態 = 0;
		//long long 道具光環效果 = 0;
	} PC;

	typedef struct tagPET
	{
		bool valid = false;
		PetState state = PetState::kNoneState;
		long long index = 0;						//位置
		long long modelid = 0;						//圖號
		long long hp = 0, maxHp = 0, hpPercent = 0;					//血量
		long long mp = 0, maxMp = 0, mpPercent = 0;					//魔力
		long long exp = 0, maxExp = 0;				//經驗值
		long long level = 0;						//等級
		long long atk = 0;						//攻擊力
		long long def = 0;						//防禦力
		long long agi = 0;						//速度
		long long loyal = 0;							//AI
		long long earth = 0, water = 0, fire = 0, wind = 0;
		long long maxSkill = 0;
		long long transmigration = 0;						// 寵物轉生數
		long long fusion = 0;						// low word: 寵蛋旗標, hi word: 物種編碼
		long long status = 0;
		long long oldlevel = 0, oldhp = 0, oldatk = 0, oldagi = 0, olddef = 0;
		long long rideflg = 0;
		long long blessflg = 0;
		long long blesshp = 0;
		long long blessatk = 0;
		long long blessquick = 0;
		long long blessdef = 0;
		long long changeNameFlag = 0;
		QString name = "";
		QString freeName = "";

		//custom
		qreal power = 0.0;
		qreal growth = 0.0;

		//ITEM item[MAX_PET_ITEM] = {};		// 寵物道具
	} PET;


	typedef struct tagMAGIC
	{
		bool valid = false;
		long long costmp = 0;
		long long field = 0;
		long long target = 0;
		long long deadTargetFlag = 0;
		QString name = "";
		QString memo = "";
	} MAGIC;


	typedef struct tagPARTY
	{
		bool valid = false;
		long long id = 0;
		long long level = 0;
		long long maxHp = 0;
		long long hp = 0;
		long long hpPercent = 0;
		long long mp = 0;
		QString name = "";
		//ACTION* ptAct;
	} PARTY;


	typedef struct tagADDRESS_BOOK
	{
		bool valid = false;
		bool onlineFlag = false;
		long long level = 0;
		long long transmigration = 0i16;
		long long dp = 0;
		long long modelid = 0;
		QString name = "";
		//顯示名片星球
		QString planetname = "";

	} ADDRESS_BOOK;

	typedef struct tagPET_SKILL
	{
		bool valid = false;
		long long skillId = 0;
		long long field = 0;
		long long target = 0;
		QString name = "";
		QString memo = "";
	} PET_SKILL;

	//人物職業
	typedef struct tagPROFESSION_SKILL
	{
		bool valid = false;
		long long skillId = 0;
		long long target = 0;
		long long kind = 0;
		long long icon = 0;
		long long costmp = 0;
		long long skill_level = 0;
		long long cooltime = 0;
		QString name = "";
		QString memo = "";
	} PROFESSION_SKILL;

	typedef struct tagCHARLISTTABLE
	{
		bool valid = false;
		long long attr[4] = { 0, 0, 0, 0 };
		long long level = 0;
		long long login = 0;
		long long faceid = 0;
		long long hp = 0;
		long long str = 0;
		long long def = 0;
		long long agi = 0;
		long long chasma = 0;
		long long dp = 0;
		long long pos = -1;
		QString name = "";
	} CHARLISTTABLE;

	//typedef struct
	//{
	//	long long 大區 = 0;
	//	long long 隊模 = 0;
	//	long long 小區 = 0;
	//	long long 人物 = 0;
	//	long long 是否自動喊話 = 0;
	//	long long 是否自動遇敵 = 0;
	//	long long 人物方向 = 0;
	//	long long 登陸延時時間 = 0;
	//	char 登陸人物名稱[4][32] = {};
	//}Landed;

	constexpr long long MAX_MISSION = 300;
	typedef struct tagJOBDAILY
	{
		long long id = 0;								// 任務編號
		long long state = 0;							//0無 1進行中 2已完成
		QString name = "";						// 任務說明
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
		long long bmpNo = 0;										// 图号
		long long color = 0;										// 文字颜色
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

		QString opp_fusion;

		PetItemInfo oPetItemInfo[MAX_PET_ITEM];			// 宠物身上的道具
	};

	typedef struct dialog_s
	{
		long long windowtype = 0;
		long long buttontype = 0;
		long long dialogid = 0;
		long long unitid = 0;
		QString data = "";
		QStringList linedatas;
		QStringList linebuttontext;
	}dialog_t;


	typedef struct battleobject_s
	{
		bool ready = false;
		long long pos = -1;
		long long modelid = 0;
		long long level = 0;
		long long hp = 0;
		long long maxHp = 0;
		long long hpPercent = 0;
		long long status = 0;
		long long rideFlag = 0;
		long long rideLevel = 0;
		long long rideHp = 0;
		long long rideMaxHp = 0;
		long long rideHpPercent = 0;
		QString rideName = 0;
		QString name = "";
		QString freeName = "";
	} battleobject_t;

	typedef struct battledata_s
	{
		long long alliemin = 0, alliemax = 0, enemymax = 0, enemymin = 0;
		QVector<battleobject_t> objects;
		QVector<battleobject_t> allies;
		QVector<battleobject_t> enemies;
	} battledata_t;


	typedef struct mapunit_s
	{
		bool isVisible = false;
		bool walkable = false;
		long long id = 0;
		long long modelid = 0;
		long long x = 0;
		long long y = 0;
		long long dir = 0;
		long long level = 0;
		long long nameColor = 0;
		long long height = 0;
		long long charNameColor = 0;
		long long petlevel = 0;
		long long classNo = 0;
		long long gold = 0;
		long long profession_class = 0;
		long long profession_level = 0;
		long long profession_skill_point = 0;
		QPoint p;
		CHAR_TYPE type = CHAR_TYPENONE;
		ObjectType objType = ObjectType::OBJ_UNKNOWN;
		CHR_STATUS status = CHR_STATUS::CHR_STATUS_NONE;
		QString name = "";
		QString freeName = "";
		QString family = "";
		QString petname = "";
		QString item_name = "";
	} mapunit_t;

	typedef struct bankpet_s
	{
		long long level = 0;
		long long maxHp = 0;
		QString name;
	}bankpet_t;

	typedef struct currencydata_s
	{
		long long expbufftime = 0;
		long long prestige = 0;      // 聲望
		long long energy = 0;        // 氣勢
		long long shell = 0;         // 貝殼
		long long vitality = 0;      // 活力
		long long points = 0;        // 積分
		long long VIPPoints = 0;  // 會員點
	} currencydata_t;

#pragma pack()

#pragma endregion

	static const QHash<QString, BUTTON_TYPE> buttonMap = {
		{"OK", BUTTON_OK},
		{"CANCEL", BUTTON_CANCEL},
		{"YES", BUTTON_YES},
		{"NO", BUTTON_NO},
		{"PREVIOUS", BUTTON_PREVIOUS},
		{"NEXT", BUTTON_NEXT},
		{"AUTO", BUTTON_AUTO},
		{"CLOSE", BUTTON_CLOSE },

		//big5
		{"確認", BUTTON_YES},
		{"確定", BUTTON_YES},
		{"取消", BUTTON_NO},
		{"好", BUTTON_YES},
		{"不好", BUTTON_NO},
		{"可以", BUTTON_YES},
		{"不可以", BUTTON_NO},
		{"上一頁", BUTTON_PREVIOUS},
		{"上一步", BUTTON_PREVIOUS},
		{"下一頁", BUTTON_NEXT},
		{"下一步", BUTTON_NEXT},
		{"買", BUTTON_BUY},
		{"賣", BUTTON_SELL},
		{"出去", BUTTON_OUT},
		{"回上一頁", BUTTON_BACK},
		{ "關閉", BUTTON_CLOSE },

		//gb2312
		{"确认", BUTTON_YES},
		{"确定", BUTTON_YES},
		{"取消", BUTTON_NO},
		{"好", BUTTON_YES},
		{"不好", BUTTON_NO},
		{"可以", BUTTON_YES},
		{"不可以", BUTTON_NO},
		{"上一页", BUTTON_PREVIOUS},
		{"上一步", BUTTON_PREVIOUS},
		{"下一页", BUTTON_NEXT},
		{"下一步", BUTTON_NEXT},
		{"买", BUTTON_BUY},
		{"卖", BUTTON_SELL},
		{"出去", BUTTON_OUT},
		{"回上一页", BUTTON_BACK},
		{ "关闭", BUTTON_CLOSE },
	};

	static const QHash<QString, PetState> petStateMap = {
		{ "戰鬥", kBattle },
		{ "等待", kStandby },
		{ "郵件", kMail },
		{ "休息", kRest },
		{ "騎乘", kRide },

		{ "战斗", kBattle },
		{ "等待", kStandby },
		{ "邮件", kMail },
		{ "休息", kRest },
		{ "骑乘", kRide },

		{ "battle", kBattle },
		{ "standby", kStandby },
		{ "mail", kMail },
		{ "rest", kRest },
		{ "ride", kRide },
	};

	static const QHash<QString, DirType> dirMap = {
		{ "北", kDirNorth },
		{ "東北", kDirNorthEast },
		{ "東", kDirEast },
		{ "東南", kDirSouthEast },
		{ "南", kDirSouth },
		{ "西南", kDirSouthWest },
		{ "西", kDirWest },
		{ "西北", kDirNorthWest },

		{ "北", kDirNorth },
		{ "东北", kDirNorthEast },
		{ "东", kDirEast },
		{ "东南", kDirSouthEast },
		{ "南", kDirSouth },
		{ "西南", kDirSouthWest },
		{ "西", kDirWest },
		{ "西北", kDirNorthWest },

		{ "A", kDirNorth },
		{ "B", kDirNorthEast },
		{ "C", kDirEast },
		{ "D", kDirSouthEast },
		{ "E", kDirSouth },
		{ "F", kDirSouthWest },
		{ "G", kDirWest },
		{ "H", kDirNorthWest },
	};

	static const QHash<QString, CHAR_EquipPlace> equipMap = {
		{ "頭", CHAR_HEAD },
		{ "身體", CHAR_BODY },
		{ "右手", CHAR_ARM },
		{ "左飾", CHAR_DECORATION1 },
		{ "右飾", CHAR_DECORATION2 },
		{ "腰帶", CHAR_EQBELT },
		{ "左手", CHAR_EQSHIELD },
		{ "鞋子", CHAR_EQSHOES },
		{ "手套", CHAR_EQGLOVE },

		{ "头", CHAR_HEAD },
		{ "身体", CHAR_BODY },
		{ "右手", CHAR_ARM },
		{ "左饰", CHAR_DECORATION1 },
		{ "右饰", CHAR_DECORATION2 },
		{ "腰带", CHAR_EQBELT },
		{ "左手", CHAR_EQSHIELD },
		{ "鞋子", CHAR_EQSHOES },
		{ "手套", CHAR_EQGLOVE },

		{ "head", CHAR_HEAD },
		{ "body", CHAR_BODY },
		{ "right", CHAR_ARM },
		{ "las", CHAR_DECORATION1 },
		{ "ras", CHAR_DECORATION2 },
		{ "belt", CHAR_EQBELT },
		{ "left", CHAR_EQSHIELD },
		{ "shoe", CHAR_EQSHOES },
		{ "glove", CHAR_EQGLOVE },
	};

	static const QHash<QString, CHAR_EquipPlace> petEquipMap = {
		{ "頭", PET_HEAD },
		{ "翅", PET_WING },
		{ "牙", PET_TOOTH },
		{ "身", PET_PLATE },
		{ "背", PET_BACK },
		{ "爪", PET_CLAW },
		{ "腳", PET_FOOT },

		{ "头", PET_HEAD },
		{ "翅", PET_WING },
		{ "牙", PET_TOOTH },
		{ "身", PET_PLATE },
		{ "背", PET_BACK },
		{ "爪", PET_CLAW },
		{ "脚", PET_FOOT },

		{ "head", PET_HEAD },
		{ "wing", PET_WING },
		{ "tooth", PET_TOOTH },
		{ "body", PET_PLATE },
		{ "back", PET_BACK },
		{ "claw", PET_CLAW },
		{ "foot", PET_FOOT },
	};
}