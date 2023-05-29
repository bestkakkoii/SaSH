#pragma once
#include <QCoreApplication>
#include <QThread>
#include <QTimer>
#include <iostream>
#include <string>
#include <vector>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define _PETS_SELECTCON
#define _NEW_SYSTEM_MENU
#define _ANNOUNCEMENT_
#define _JOBDAILY
#define _TEACHER_SYSTEM
#define _MAGIC_NOCAST
#define _TEAM_KICKPARTY
#define _CHECK_GAMESPEED
#define _PET_ITEM
#define _ITEM_FIREWORK
#define _ITEM_EQUITSPACE
#define _CHAR_PROFESSION
#define _STANDBYPET
#define _ITEM_PILENUMS
#define _ALCHEMIST
#define _ITEM_JIGSAW
#define _SHOW_FUSION
#define _CHAR_NEWLOGOUT
#define _CHANNEL_MODIFY
#define _FMVER21
#define _CHANNEL_WORLD
#define _EQUIT_NEWGLOVE
#define __ATTACK_MAGIC
#define _SKILL_ADDBARRIER

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

#ifdef _MORECHARACTERS_
#define MAXCHARACTER			4
#else
#define MAXCHARACTER			2
#endif
//#define CHAR_NAME_LEN			16
//#define CHAR_FREENAME_LEN		32
//#define MAGIC_NAME_LEN			28
//#define MAGIC_MEMO_LEN			72
//#define ITEM_NAME_LEN			28
//#define ITEM_NAME2_LEN			16
//#define ITEM_MEMO_LEN			84
//#define PET_NAME_LEN			16
//#define PET_FREENAME_LEN		32
//#define CHAR_FMNAME_LEN			33      // 家族名稱

#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
//#define PROFESSION_MEMO_LEN		84
#endif

#ifdef _GM_IDENTIFY		// Rog ADD GM識別
#define GM_NAME_LEN		        32
#endif

//#define CHARNAMELEN				256		

#define MAX_PET					5

#define MAX_MAGIC				9

#define MAX_PARTY				5

#define	MAX_ADR_BOOK_COUNT		4
#ifdef _EXTEND_AB
#define	MAX_ADR_BOOK_PAGE		20//20  //10   20050214 cyg 10 add to 20
#else
#define	MAX_ADR_BOOK_PAGE		10
#endif
#define MAX_ADR_BOOK			(MAX_ADR_BOOK_COUNT*MAX_ADR_BOOK_PAGE)

#ifdef _PRO3_ADDSKILL
#define MAX_PROFESSION_SKILL	30
#else
#define MAX_PROFESSION_SKILL	26
#endif

#define BATTLE_BUF_SIZE	4
#define BATTLE_COMMAND_SIZE			4096

//#define FLOOR_NAME_LEN	24

#ifdef _ITEM_EQUITSPACE
typedef enum tagCHAR_EquipPlace
{
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
#define MAX_PET_ITEM	7
#endif

#define MAX_ITEMSTART CHAR_EQUIPPLACENUM
#define MAX_MAXHAVEITEM	15
#ifdef _NEW_ITEM_
#define MAX_ITEM (MAX_ITEMSTART+MAX_MAXHAVEITEM*3)
int 判斷玩家道具數量();
#else
#define MAX_ITEM (MAX_ITEMSTART+MAX_MAXHAVEITEM)
#endif
#else
#define MAX_ITEMSTART 5
#define MAX_ITEM				20
#endif

#define RESULT_ITEM_COUNT		3
//#define RESULT_ITEM_NAME_LEN	24
//#define RESULT_CHR_EXP			4
#define RESULT_CHR_EXP			5

//#define SKILL_NAME_LEN			24
//#define SKILL_MEMO_LEN			72
#define MAX_SKILL				7



#define MAX_GOLD				1000000
#define MAX_BANKGOLD			10000000
#define MAX_FMBANKGOLD			100000000


#define MAX_PERSONAL_BANKGOLD 50000000

#ifdef _FMVER21
#define FAMILY_MAXMEMBER                100     // 家族人數
#else
#define FAMILY_MAXMEMBER                50     // 家族人數
#endif

#define MAP_TILE_GRID_X1	-20
#define MAP_TILE_GRID_X2	+17	
#define MAP_TILE_GRID_Y1	-16
#define MAP_TILE_GRID_Y2	+21		
#define MAP_X_SIZE	(MAP_TILE_GRID_X2 - MAP_TILE_GRID_X1)
#define MAP_Y_SIZE	(MAP_TILE_GRID_Y2 - MAP_TILE_GRID_Y1)

#define MAP_READ_FLAG	0x8000		
#define MAP_SEE_FLAG	0x4000

#define BC_FLG_NEW (1 << 0)
#define BC_FLG_DEAD (1 << 1)	  //死亡
#define BC_FLG_PLAYER (1 << 2)	  //玩家,玩家有异常状态时要有此值
#define BC_FLG_POISON (1 << 3)	  //中毒
#define BC_FLG_PARALYSIS (1 << 4) //麻痹
#define BC_FLG_SLEEP (1 << 5)	  //昏睡
#define BC_FLG_STONE (1 << 6)	  //石化
#define BC_FLG_DRUNK (1 << 7)	  //眩晕
#define BC_FLG_CONFUSION (1 << 8) //混乱
#define BC_FLG_HIDE (1 << 9)	  //是否隐藏，地球一周
#define BC_FLG_REVERSE (1 << 10)  //反转

enum
{
	PC_ETCFLAG_GROUP = (1 << 0),	//組隊開關
	PC_ETCFLAG_PK = (1 << 2),	//決鬥開關
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

enum
{
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

enum
{
	CHAROBJ_TYPE_NPC = 0x0001,		// NPC
	CHAROBJ_TYPE_ITEM = 0x0002,
	CHAROBJ_TYPE_MONEY = 0x0004,
	CHAROBJ_TYPE_USER_NPC = 0x0008,
	CHAROBJ_TYPE_LOOKAT = 0x0010,
	CHAROBJ_TYPE_PARTY_OK = 0x0020,
	CHAROBJ_TYPE_ALL = 0x00FF
};

enum
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

#if 0

#define NIGHT_TO_MORNING	906
#define MORNING_TO_NOON		1006
#define NOON_TO_EVENING		356
#define EVENING_TO_NIGHT	456

#else

#define NIGHT_TO_MORNING	700
#define MORNING_TO_NOON		930
#define NOON_TO_EVENING		200
#define EVENING_TO_NIGHT	300

#endif

#define ITEM_FLAG_PET_MAIL		( 1 << 0 )
#define ITEM_FLAG_MIX			( 1 << 1 )
#define ITEM_FLAG_COOKING_MIX	( 1 << 2 )
#define ITEM_FLAG_METAL_MIX		( 1 << 3 )	//金屬
#define ITEM_FLAG_JEWEL_MIX		( 1 << 4 )	//寶石
#define ITEM_FLAG_FIX_MIX		( 1 << 5 )	//修理
#ifdef _ITEM_INTENSIFY
#define ITEM_FLAG_INTENSIFY_MIX ( 1 << 6 )	//強化
#endif
#ifdef _ITEM_UPINSLAY
#define ITEM_FLAG_UPINSLAY_MIX ( 1 << 7 )	//鑿孔
#endif

#define	JOY_RIGHT	(1 << 15)	/* Right Key				*/
#define	JOY_LEFT	(1 << 14)	/*  Left Key				*/
#define	JOY_DOWN	(1 << 13)	/*  Down Key				*/
#define	JOY_UP		(1 << 12)	/*    Up Key				*/
#define	JOY_START	(1 << 11)	/* Start					*/
#define	JOY_A		(1 << 10)	/* A Trigger				*/
#define	JOY_C		(1 <<  9)	/* C Trigger				*/
#define	JOY_B		(1 <<  8)	/* B Trigger				*/
#define	JOY_R		(1 <<  7)	/* R Trigger				*/
#define	JOY_X		(1 <<  6)	/* X Trigger				*/
#define	JOY_DEL		(1 <<  5)	/* DELETE					*/
#define	JOY_INS		(1 <<  4)	/* INSERT					*/
#define	JOY_END		(1 <<  3)	/* END						*/
#define	JOY_HOME	(1 <<  2)	/* HOME						*/
#define	JOY_P_DOWN	(1 <<  1)	/* PAGE_UP					*/
#define	JOY_P_UP	(1 <<  0)	/* PAGE_DOWN				*/

#define	JOY_ESC		(1 << 31)	/* ESC Key					*/
#define	JOY_CTRL_M	(1 << 30)	/* Ctrl + M					*/
#define	JOY_CTRL_S	(1 << 29)	/* Ctrl + S					*/
#define	JOY_CTRL_P	(1 << 28)	/* Ctrl + P					*/
#define	JOY_CTRL_I	(1 << 27)	/* Ctrl + I					*/
#define	JOY_CTRL_E	(1 << 26)	/* Ctrl + E					*/
#define	JOY_CTRL_A	(1 << 25)	/* Ctrl + A					*/

#define	JOY_CTRL_C	(1 << 24)	/* Ctrl + C					*/
#define	JOY_CTRL_V	(1 << 23)	/* Ctrl + V					*/
#define	JOY_CTRL_T	(1 << 22)	/* Ctrl + T					*/

//#define MAIL_STR_LEN 140		
//#define MAIL_DATE_STR_LEN 20
#define MAIL_MAX_HISTORY 20	
#define MAX_CHAT_REGISTY_STR		8		


#define SUCCESSFULSTR	"successful"  
#define FAILEDSTR		"failed" 
#define OKSTR           "ok"
#define CANCLE          "cancle"

#define BATTLE_MAP_FILES 220
#define GRID_SIZE		64

#define LSTIME_SECONDS_PER_DAY 5400 
#define LSTIME_HOURS_PER_DAY 1024 
#define LSTIME_DAYS_PER_YEAR 100

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
		for (int i = 0; i < MAIL_MAX_HISTORY; i++)
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
	short battlePetNo = 0i16;
	short selectPetNo[MAX_PET] = { 0i16, 0i16 ,0i16 ,0i16 ,0i16 };
	short mailPetNo = 0i16;
#ifdef _STANDBYPET
	short standbyPet = 0i16;
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
	int ridePetNo = 0;//寵物形像
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
} PC;

#define DEF_PAL	0
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
	int hp = 0, maxHp = 0, hpPercent;					//血量
	int mp = 0, maxMp = 0, mpPercent;					//魔力
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

#define MAXMISSION	300 
typedef struct tagJOBDAILY
{
	int JobId = 0;								// 任務編號
	QString explain = "";						// 任務說明
	QString state = "";							// 狀態
}JOBDAILY;

typedef struct battleobject_s
{
	int pos = -1;
	QString name = "";
	QString freename = "";
	int faceid = 0;
	int level = 0;
	int hp = 0;
	int maxHp = 0;
	int status = 0;
	int rideFlag = 0;
	QString rideName = 0;
	int rideLevel = 0;
	int rideHp = 0;
	int rideMaxHp = 0;
} battleobject_t;

typedef struct battledata_s
{
	int fieldAttr = 0;
	battleobject_t player = {};
	battleobject_t pet = {};
	QVector<battleobject_t> objects;
	QVector<battleobject_t> allies;
	QVector<battleobject_t> enemies;

} battledata_t;

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

const int MAX_PAL = sizeof(palFileName) / sizeof(palFileName[0]);

class Server : public QObject
{
	Q_OBJECT
public:
	explicit Server(QObject* parent = nullptr);

	virtual ~Server();

	bool isRunning() const
	{
		return server_->isListening();
	}
public slots:
	void run();

public:
	void requestInterruption() { isRequestInterrupted.store(true, std::memory_order_release); }

	unsigned short getPort() const { return port_; }

private:
	QFutureSynchronizer <void> sync_;
	QMutex mutex_;

	bool isInterruptionRequested() const { return isRequestInterrupted.load(std::memory_order_acquire); }

	int SaDispatchMessage(int fd, char* encoded);
	void handleData(QTcpSocket* clientSocket, QByteArray data);
private://lssproto
	void lssproto_W_send(int fd, int x, int y, char* direction);
	void lssproto_W2_send(int fd, int x, int y, char* direction);
	//Cary say 戰鬥結束後的大地圖座標
	void lssproto_XYD_recv(int fd, int x, int y, int dir);
	void lssproto_EV_send(int fd, int event, int seqno, int x, int y, int dir);
	void lssproto_EV_recv(int fd, int seqno, int result);
	void lssproto_EN_send(int fd, int x, int y);
	void lssproto_DU_send(int fd, int x, int y);
	void lssproto_EN_recv(int fd, int result, int field);
	void lssproto_EO_send(int fd, int dummy);
	void lssproto_BU_send(int fd, int dummy);
	void lssproto_JB_send(int fd, int x, int y);
	void lssproto_LB_send(int fd, int x, int y);
	void lssproto_RS_recv(int fd, char* data);
	void lssproto_RD_recv(int fd, char* data);
	void lssproto_B_send(int fd, char* command);
	void lssproto_B_recv(int fd, char* command);
	void lssproto_SKD_send(int fd, int dir, int index);
	void lssproto_ID_send(int fd, int x, int y, int haveitemindex, int toindex);
	void lssproto_PI_send(int fd, int x, int y, int dir);
	void lssproto_DI_send(int fd, int x, int y, int itemindex);
	void lssproto_DG_send(int fd, int x, int y, int amount);
	void lssproto_DP_send(int fd, int x, int y, int petindex);
	void lssproto_I_recv(int fd, char* data);
	void lssproto_MI_send(int fd, int fromindex, int toindex);
	void lssproto_SI_recv(int fd, int fromindex, int toindex);
	void lssproto_MSG_send(int fd, int index, char* message, int color);
	//Cary say 收到普通郵件或寵物郵件
	void lssproto_MSG_recv(int fd, int aindex, char* text, int color);
	void lssproto_PMSG_send(int fd, int index, int petindex, int itemindex, char* message, int color);
	void lssproto_PME_recv(int fd, int objindex, int graphicsno, int x, int y, int dir, int flg, int no, char* cdata);
	void lssproto_AB_send(int fd);
	void lssproto_AB_recv(int fd, char* data);
	void lssproto_ABI_recv(int fd, int num, char* data);
	void lssproto_DAB_send(int fd, int index);
	void lssproto_AAB_send(int fd, int x, int y);
	void lssproto_L_send(int fd, int dir);
	void lssproto_TK_send(int fd, int x, int y, char* message, int color, int area);
	void lssproto_TK_recv(int fd, int index, char* message, int color);
	void lssproto_MC_recv(int fd, int fl, int x1, int y1, int x2, int y2, int tilesum, int objsum, int eventsum, char* data);
	void lssproto_M_send(int fd, int fl, int x1, int y1, int x2, int y2);
	void lssproto_M_recv(int fd, int fl, int x1, int y1, int x2, int y2, char* data);
	void lssproto_C_send(int fd, int index);
	void lssproto_C_recv(int fd, char* data);
	void lssproto_CA_recv(int fd, char* data);
	void lssproto_CD_recv(int fd, char* data);
	void lssproto_R_recv(int fd, char* data);
	void lssproto_S_send(int fd, char* category);
	void lssproto_S_recv(int fd, char* data);
	void lssproto_D_recv(int fd, int category, int dx, int dy, char* data);
	void lssproto_FS_send(int fd, int flg);
	void lssproto_FS_recv(int fd, int flg);
	void lssproto_HL_send(int fd, int flg);
	//戰鬥中是否要Help
	void lssproto_HL_recv(int fd, int flg);
	void lssproto_PR_send(int fd, int x, int y, int request);
	void lssproto_PR_recv(int fd, int request, int result);
	void lssproto_KS_send(int fd, int petarray);
	//Cary say 指定那一只寵物出場戰鬥
	void lssproto_KS_recv(int fd, int petarray, int result);

#ifdef _STANDBYPET
	void lssproto_SPET_send(int fd, int standbypet);
	void lssproto_SPET_recv(int fd, int standbypet, int result);
#endif

	void lssproto_AC_send(int fd, int x, int y, int actionno);
	void lssproto_MU_send(int fd, int x, int y, int array, int toindex);
	void lssproto_PS_send(int fd, int havepetindex, int havepetskill, int toindex, char* data);
	//Cary say 寵物合成
	void lssproto_PS_recv(int fd, int result, int havepetindex, int havepetskill, int toindex);
	void lssproto_ST_send(int fd, int titleindex);
	void lssproto_DT_send(int fd, int titleindex);
	void lssproto_FT_send(int fd, char* data);
	//Cary say 取得可加的屬性點數
	void lssproto_SKUP_recv(int fd, int point);
	void lssproto_SKUP_send(int fd, int skillid);
	void lssproto_KN_send(int fd, int havepetindex, char* data);
	void lssproto_WN_recv(int fd, int windowtype, int buttontype, int seqno, int objindex, char* data);
	void lssproto_WN_send(int fd, int x, int y, int seqno, int objindex, int select, char* data);
	void lssproto_EF_recv(int fd, int effect, int level, char* option);
	void lssproto_SE_recv(int fd, int x, int y, int senumber, int sw);
	void lssproto_SP_send(int fd, int x, int y, int dir);
#ifdef _NEW_CLIENT_LOGIN
	void lssproto_ClientLogin_send(int fd, char* cdkey, char* passwd, char* mac, int selectServerIndex, char* ip);
#else
	void lssproto_ClientLogin_send(int fd, char* cdkey, char* passwd);
#endif
	void lssproto_ClientLogin_recv(int fd, char* result);

	void lssproto_CreateNewChar_send(int fd, int dataplacenum, char* charname, int imgno, int faceimgno, int vital, int str, int tgh, int dex, int earth, int water, int fire, int wind, int hometown);
	void lssproto_CreateNewChar_recv(int fd, char* result, char* data);
	void lssproto_CharDelete_send(int fd, char* charname);
	void lssproto_CharDelete_recv(int fd, char* result, char* data);
	void lssproto_CharLogin_send(int fd, char* charname);
	void lssproto_CharLogin_recv(int fd, char* result, char* data);
	void lssproto_CharList_send(int fd);
	void lssproto_CharList_recv(int fd, char* result, char* data);
	void lssproto_CharLogout_send(int fd, int Flg);
	void lssproto_CharLogout_recv(int fd, char* result, char* data);
	void lssproto_ProcGet_send(int fd);
	void lssproto_ProcGet_recv(int fd, char* data);
	void lssproto_PlayerNumGet_send(int fd);
	void lssproto_PlayerNumGet_recv(int fd, int logincount, int player);
	void lssproto_Echo_send(int fd, char* test);
	void lssproto_Echo_recv(int fd, char* test);
	void lssproto_Shutdown_send(int fd, char* passwd, int min);
	void lssproto_NU_recv(int fd, int AddCount);
	void lssproto_TD_send(int fd, char* data);
	void lssproto_TD_recv(int fd, char* data);
	void lssproto_FM_send(int fd, char* data);
	void lssproto_FM_recv(int fd, char* data);
	//Cary say 取得轉生的特效
	void lssproto_WO_recv(int fd, int effect);
	void lssproto_PETST_send(int fd, int nPet, int sPet);// sPet  0:休息 1:等待 4:郵件
	void lssproto_BM_send(int fd, int iindex);             // _BLACK_MARKET
#ifdef _FIX_DEL_MAP
	void lssproto_DM_send(int fd);                         // WON ADD 玩家抽地圖送監獄
#endif
#ifdef _MIND_ICON 
	void lssproto_MA_send(int fd, int x, int y, int nMind);
#endif
#ifdef _ITEM_CRACKER 
	void lssproto_IC_recv(int fd, int x, int y);
#endif
#ifdef _MAGIC_NOCAST//沈默
	void lssproto_NC_recv(int fd, int flg);
#endif

#ifdef _CHECK_GAMESPEED
	void lssproto_CS_send(int fd);
	void lssproto_CS_recv(int fd, int deltimes);
	int lssproto_getdelaytimes();
	void lssproto_setdelaytimes(int delays);
#endif

#ifdef _TEAM_KICKPARTY
	void lssproto_KTEAM_send(int fd, int si);
#endif

#ifdef _PETS_SELECTCON
	void lssproto_PETST_recv(int fd, int petarray, int result);
#endif
#ifdef _CHATROOMPROTOCOL			// (不可開) Syu ADD 聊天室頻道
	void lssproto_CHATROOM_send(int fd, char* data);
	void lssproto_CHATROOM_recv(int fd, char* data);
#endif
#ifdef _NEWREQUESTPROTOCOL			// (不可開) Syu ADD 新增Protocol要求細項
	void lssproto_RESIST_send(int fd, char* data);
	void lssproto_RESIST_recv(int fd, char* data);
#endif

#ifdef _ALCHEPLUS
	void lssproto_ALCHEPLUS_send(int fd, char* data);
	void lssproto_ALCHEPLUS_recv(int fd, char* data);
#endif

#ifdef _OUTOFBATTLESKILL			// (不可開) Syu ADD 非戰鬥時技能Protocol
	void lssproto_BATTLESKILL_send(int fd, int SkillNum);
	void lssproto_BATTLESKILL_recv(int fd, char* data);
#endif
	void lssproto_CHAREFFECT_recv(int fd, char* data);

#ifdef _STREET_VENDOR
	void lssproto_STREET_VENDOR_send(int fd, char* data);	// 擺攤功能
	void lssproto_STREET_VENDOR_recv(int fd, char* data);	// 擺攤功能
#endif

#ifdef _JOBDAILY
	void lssproto_JOBDAILY_send(int fd, char* data);
	void lssproto_JOBDAILY_recv(int fd, char* data);
#endif

#ifdef _FAMILYBADGE_
	void lssproto_FamilyBadge_send(int fd);
	void lssproto_FamilyBadge_recv(char* data);
#endif

#ifdef _TEACHER_SYSTEM
	void lssproto_TEACHER_SYSTEM_send(int fd, char* data);	// 導師功能
	void lssproto_TEACHER_SYSTEM_recv(int fd, char* data);
#endif

#ifdef _ADD_STATUS_2
	void lssproto_S2_send(int fd, char* data);
	void lssproto_S2_recv(int fd, char* data);
#endif

#ifdef _ITEM_FIREWORK
	void lssproto_Firework_recv(int fd, int nCharaindex, int nType, int nActionNum);	// 煙火功能
#endif
#ifdef _PET_ITEM
	void lssproto_PetItemEquip_send(int fd, int nGx, int nGy, int nPetNo, int nItemNo, int nDestNO);	// 寵物裝備功能
#endif
#ifdef _THEATER
	void lssproto_TheaterData_recv(int fd, char* pData);
#endif
#ifdef _MOVE_SCREEN
	void lssproto_MoveScreen_recv(int fd, BOOL bMoveScreenMode, int iXY);	// client 移動熒幕
#endif
#ifdef _GET_HOSTNAME
	void lssproto_HostName_send(int fd);
#endif

#ifdef _NPC_MAGICCARD
	void lssproto_MagiccardAction_recv(int fd, char* data);	//魔法牌功能
	void lssproto_MagiccardDamage_recv(int fd, int position, int damage, int offsetx, int offsety);
#endif

#ifdef  _NPC_DANCE
	void lssproto_DancemanOption_recv(int fd, int option);	//動一動狀態
#endif



#ifdef _ANNOUNCEMENT_

	void lssproto_DENGON_recv(char* data, int colors, int nums);
#endif
#ifdef _HUNDRED_KILL
	void lssproto_hundredkill_recv(int fd, int flag);
#endif
#ifdef _PK2007
	void lssproto_pkList_send(int fd);
	void lssproto_pkList_recv(int fd, int count, char* data);
#endif
#ifdef _NEW_SYSTEM_MENU
	void lssproto_SaMenu_send(int fd, int index);
#endif


#ifdef _PETBLESS_
	void lssproto_petbless_send(int fd, int petpos, int type);
#endif
#ifdef _RIDEQUERY_
	void lssproto_RideQuery_send(int fd);
#endif
#ifdef _PET_SKINS
	void lssproto_PetSkins_recv(char* data);
#endif



#ifdef _CHARSIGNDAY_
	void lssproto_SignDay_send(int fd);
#endif

public:
	int getWorldStatus();
	int getGameStatus();

	int getUnloginStatus();
	bool login(int s);
	void leftCLick(int x, int y);
	//int getSocket() const { return sockfd_; }
	void unlockSecurityCode(const QString& code);
	void clearNetBuffer();

	void logOut();
	void logBack();
	void move(const QPoint& p, const QString& dir);
	void move(const QPoint& p);
	void announce(const QString& msg, int color = 4);
	void EO();
	void dropItem(int index);

	void setSwitcher(int flg, bool enable);
	void setSwitcher(int flg);

	void updateDatasFromMemory();

	//battle
	void asyncBattleWork();
	int playerDoBattleWork();
	int petDoBattleWork();
	int getBattleSelectableEnemyTarget();
	int getGetPetSkillIndexByName(int petIndex, const QString& name);

	bool fixPlayerTargetByMagicIndex(int magicIndex, int oldtarget, int* target);
	bool fixPetTargetBySkillIndex(int skillIndex, int oldtarget, int* target);
	void sendBattlePlayerAttckAct(int target);
	void sendBattlePlayerMagicAct(int magicIndex, int target);
	void sendBattlePlayerJobSkillAct(int skillIndex, int target);
	void sendBattlePlayerItemAct(int itemIndex, int target);
	void sendBattlePlayerDefenseAct();
	void sendBattlePlayerEscapeAct();
	void sendBattlePlayerCatchPetAct(int petIndex);
	void sendBattlePlayerSwitchPetAct(int petIndex);
	void sendBattlePlayerDoNothing();
	void sendBattlePetSkillAct(int skillIndex, int target);
	void sendBattlePetDoNothing();
private:
	void setBattleEnd();
	void refreshItemInfo(int index);
	void refreshItemInfo();

	void setWorldStatus(int w);
	void setGameStatus(int g);

	bool CheckMailNoReadFlag();

	void clearPartyParam();


	void swapItem(int from, int to);

	void redrawMap(void)
	{
		oldGx = -1;
		oldGy = -1;
	}

	void updateMapArea(void)
	{
		mapAreaX1 = nowGx + MAP_TILE_GRID_X1;
		mapAreaY1 = nowGy + MAP_TILE_GRID_Y1;
		mapAreaX2 = nowGx + MAP_TILE_GRID_X2;
		mapAreaY2 = nowGy + MAP_TILE_GRID_Y2;

		if (mapAreaX1 < 0)
			mapAreaX1 = 0;
		if (mapAreaY1 < 0)
			mapAreaY1 = 0;
		if (mapAreaX2 > nowFloorGxSize)
			mapAreaX2 = nowFloorGxSize;
		if (mapAreaY2 > nowFloorGySize)
			mapAreaY2 = nowFloorGySize;

		mapAreaWidth = mapAreaX2 - mapAreaX1;
		mapAreaHeight = mapAreaY2 - mapAreaY1;
	}

	void setPcWarpPoint(int gx, int gy)
	{
		//	if(pc.ptAct == NULL)
		//		return;

		setWarpMap(gx, gy);
	}

	unsigned int TimeGetTime(void)
	{
#ifdef _TIME_GET_TIME
		static __int64 time;
		QueryPerformanceCounter(&CurrentTick);
		return (unsigned int)(time = CurrentTick.QuadPart / tickCount.QuadPart);
		//return GetTickCount();
#else
		return GetTickCount();
		//return timeGetTime();
#endif
	}

	void setWarpMap(int gx, int gy)
	{
		nowGx = gx;
		nowGy = gy;
		nowX = (float)nowGx * GRID_SIZE;
		nowY = (float)nowGy * GRID_SIZE;
		nextGx = nowGx;
		nextGy = nowGy;
		nowVx = 0;
		nowVy = 0;
		nowSpdRate = 1;
		oldGx = -1;
		oldGy = -1;
		oldNextGx = -1;
		oldNextGy = -1;
		viewPointX = nowX;
		viewPointY = nowY;
		wnCloseFlag = 1;
#ifdef _AniCrossFrame	   // Syu ADD 動畫層遊過畫面生物
		extern void crossAniRelease();
		crossAniRelease();
#endif
#ifdef _SURFACE_ANIM       //ROG ADD 動態場景
		extern void ReleaseSpecAnim();
		ReleaseSpecAnim();
#endif
	}

#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
	//    #ifdef _GM_IDENTIFY		// Rog ADD GM識別
	//  void setPcParam(char *name, char *freeName, int level, char *petname, int petlevel, int nameColor, int walk, int height, int profession_class, int profession_level, int profession_exp, int profession_skill_point , char *gm_name);
	//    void setPcParam(char *name, char *freeName, int level, char *petname, int petlevel, int nameColor, int walk, int height, int profession_class, int profession_level, int profession_skill_point , char *gm_name)   ; 
	//	#else
	//	void setPcParam(char *name, char *freeName, int level, char *petname, int petlevel, int nameColor, int walk, int height, int profession_class, int profession_level, int profession_exp, int profession_skill_point);
#ifdef _ALLDOMAN // (不可開) Syu ADD 排行榜NPC
	void setPcParam(const QString& name, const QString& freeName, int level, const QString& petname, int petlevel, int nameColor, int walk, int height, int profession_class, int profession_level, int profession_skill_point, int herofloor);
#else
	void setPcParam(const QString& name, const QString& freeName, int level, const QString& petname, int petlevel, int nameColor, int walk, int height, int profession_class, int profession_level, int profession_skill_point);
#endif
	// 	#endif
#else
	void setPcParam(const QString& name, const QString& freeName, int level, const QString& petname, int petlevel, int nameColor, int walk, int height);
#endif

	void resetPc(void)
	{
		pc.status &= (~CHR_STATUS_LEADER);
	}

	void setMap(int floor, int gx, int gy)
	{
		nowFloor = floor;
		setWarpMap(gx, gy);
	}

	void resetMap(void)
	{
		nowGx = (int)(nowX / GRID_SIZE);
		nowGy = (int)(nowY / GRID_SIZE);
		nextGx = nowGx;
		nextGy = nowGy;
		nowX = (float)nowGx * GRID_SIZE;
		nowY = (float)nowGy * GRID_SIZE;
		oldGx = -1;
		oldGy = -1;
		oldNextGx = -1;
		oldNextGy = -1;
		mapAreaX1 = nowGx + MAP_TILE_GRID_X1;
		mapAreaY1 = nowGy + MAP_TILE_GRID_Y1;
		mapAreaX2 = nowGx + MAP_TILE_GRID_X2;
		mapAreaY2 = nowGy + MAP_TILE_GRID_Y2;

		if (mapAreaX1 < 0)
			mapAreaX1 = 0;
		if (mapAreaY1 < 0)
			mapAreaY1 = 0;
		if (mapAreaX2 > nowFloorGxSize)
			mapAreaX2 = nowFloorGxSize;
		if (mapAreaY2 > nowFloorGySize)
			mapAreaY2 = nowFloorGySize;

		mapAreaWidth = mapAreaX2 - mapAreaX1;
		mapAreaHeight = mapAreaY2 - mapAreaY1;
		nowVx = 0;
		nowVy = 0;
		nowSpdRate = 1;
		viewPointX = nowX;
		viewPointY = nowY;
		moveRouteCnt = 0;
		moveRouteCnt2 = 0;
		moveStackFlag = false;
		mouseCursorMode = MOUSE_CURSOR_MODE_NORMAL;
		mouseLeftPushTime = 0;
		beforeMouseLeftPushTime = 0;
		//	autoMapSeeFlag = FALSE;
	}

	void PaletteChange(int palNo, int time);

	void RealTimeToSATime(LSTIME* lstime);

	bool mapCheckSum(int floor, int x1, int y1, int x2, int y2, int tileSum, int partsSum, int eventSum);
	bool writeMap(int floor, int x1, int y1, int x2, int y2, unsigned short* tile, unsigned short* parts, unsigned short* event);
	void setEventMemory(int x, int y, unsigned short ev);

public:
	bool IS_ONLINE_FLAG = false;
	bool IS_BATTLE_FLAG = false;

	std::atomic_bool isPacketAutoClear = false;
	bool disconnectflag = false;

	bool enablePlayerWork = false;
	bool enablePetWork = false;
	bool enemyAllReady = false;

	QFuture<void> ayncBattleCommand;
	std::atomic_bool ayncBattleCommandFlag = false;
	QElapsedTimer battleDurationTimer;
	QElapsedTimer normalDurationTimer;
	QElapsedTimer eottlTimer;
	bool isEOTTLSend = false;

	int battle_total_time = 0;
	int battle_totol = 0;

	int  TalkMode = 0;						//0:一般 1:密語 2: 隊伍 3:家族 4:職業 

	unsigned int ProcNo = 0u;
	unsigned int SubProcNo = 0u;
	unsigned int MenuToggleFlag = 0u;

	short prSendFlag = 0i16;

	short mailLamp = 0i16;
	short mailLampDrawFlag = 0i16;

	short mapEffectRainLevel = 0i16;
	short mapEffectSnowLevel = 0i16;
	short mapEffectKamiFubukiLevel = 0i16;

	bool DuelFlag = false;
	bool NoHelpFlag = false;
	short vsLookFlag = 0i16;
	short eventEnemyFlag = 0i16;
	int BattleMapNo = 0;

	LSTIME SaTime = { 0 };
	long serverTime = 0L;
	long FirstTime = 0L;
	int SaTimeZoneNo = 0;

	short charDelStatus = 0;

	bool floorChangeFlag = false;
	bool warpEffectStart = false;
	bool warpEffectOk = false;
	short eventWarpSendId = 0i16;
	short eventWarpSendFlag = 0i16;
	short eventEnemySendId = 0i16;
	short eventEnemySendFlag = 0i16;

	short sendEnFlag = 0i16;
	short duelSendFlag = 0i16;
	short jbSendFlag = 0i16;
	short helpFlag = 0i16;

	bool BattleTurnReceiveFlag = false;
	int BattleMyNo = 0;
	int BattleBpFlag = 0;
	int BattleEscFlag = 0;
	int BattleMyMp = 0;
	int BattleAnimFlag = 0;
	int BattleSvTurnNo = 0;
	int BattleCliTurnNo = 0;
	int BattlePetStMenCnt = 0;

	bool BattlePetReceiveFlag = false;
	int BattlePetReceivePetNo = -1;
	int battlePetNoBak = -2;

	int BattleCmdReadPointer = 0;
	int BattleStatusReadPointer = 0;
	int BattleCmdWritePointer = 0;
	int BattleStatusWritePointer = 0;
	QString BattleCmdBak[BATTLE_BUF_SIZE] = {};
	QString  BattleStatus[BATTLE_COMMAND_SIZE] = {};
	QString  BattleStatusBak[BATTLE_BUF_SIZE] = {};

	int StatusUpPoint = 0;

	int mailHistoryWndSelectNo = 0;
	int mailHistoryWndPageNo = 0;
	bool ItemMixRecvFlag = FALSE;

	int transmigrationEffectFlag = 0;
	int transEffectPaletteStatus = 0;

	short clientLoginStatus = 0i16;
	time_t serverAliveLongTime = 0;
	struct tm serverAliveTime = { 0 };

	QString secretName = "";

	int palNo = 0;
	int palTime = 0;

	QString nowFloorName = "";
	bool mapEmptyFlag = false;
	int nowFloor = 0;
	int nowFloorGxSize = 0, nowFloorGySize = 0;
	int oldGx = -1, oldGy = -1;
	int mapAreaX1 = 0, mapAreaY1 = 0, mapAreaX2 = 0, mapAreaY2 = 0;
	int nowGx = 0, nowGy = 0;
	int mapAreaWidth = 0, mapAreaHeight = 0;
	float nowX = (float)nowGx * GRID_SIZE, nowY = (float)nowGy * GRID_SIZE;
	int nextGx = 0, nextGy = 0;
	float nowVx = 0.0f, nowVy = 0.0f, nowSpdRate = 1.0f;
	int oldNextGx = -1, oldNextGy = -1;
	float viewPointX = 0.0f;
	float viewPointY = 0.0f;
	short wnCloseFlag = 0i16;
	short moveRouteCnt = 0i16;
	short moveRouteCnt2 = 0i16;

	bool moveStackFlag = false;
	unsigned int mouseLeftPushTime = 0u;
	unsigned int beforeMouseLeftPushTime = 0u;

	short nowEncountPercentage = 0i16;
	short minEncountPercentage = 0i16;
	short maxEncountPercentage = 0i16;
	short nowEncountExtra = 0i16;
	int MapWmdFlagBak = 0;
	unsigned short event_[MAP_X_SIZE * MAP_Y_SIZE] = {};

	short mouseCursorMode = MOUSE_CURSOR_MODE_NORMAL;

	bool TimeZonePalChangeFlag = false;
	short drawTimeAnimeFlag = 0;

	bool NoCastFlag = false;
	bool StandbyPetSendFlag = false;
	bool warpEffectFlag = false;

	short newCharStatus = 0i16;
	short charListStatus = 0i16;

	short sTeacherSystemBtn = 0i16;

	PC pc = {};

	battledata_t battleData;

	PALETTE_STATE 	PalState = {};

	MAIL_HISTORY MailHistory[MAX_ADR_BOOK] = {};
	PET pet[MAX_PET] = {};

#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
	PROFESSION_SKILL profession_skill[MAX_PROFESSION_SKILL];
#endif

	MAGIC magic[MAX_MAGIC] = {};

#ifdef MAX_AIRPLANENUM
	PARTY party[MAX_AIRPLANENUM];
#else
	PARTY party[MAX_PARTY] = {};
#endif
	short partyModeFlag = 0;


	CHARLISTTABLE chartable[MAXCHARACTER] = {};

	ADDRESS_BOOK addressBook[MAX_ADR_BOOK] = {};

	BATTLE_RESULT_MSG battleResultMsg = {};

	PET_SKILL petSkill[MAX_PET][MAX_SKILL] = {};

	JOBDAILY jobdaily[MAXMISSION] = {};
	int JobdailyGetMax = 0;  //是否有接收到資料

signals:
	void write(QTcpSocket* clientSocket, QByteArray ba, int size);

private slots:
	void onWrite(QTcpSocket* clientSocket, QByteArray ba, int size);
	void onNewConnection();
	void onClientReadyRead();
private:
	std::atomic_bool isRequestInterrupted = false;
	int sockfd_ = 0;
	unsigned short port_ = 0;
	QScopedPointer<QTcpServer> server_;
	QList<QTcpSocket*> clientSockets_;


};
