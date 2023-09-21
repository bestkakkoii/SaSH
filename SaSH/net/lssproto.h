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

#pragma region MACROS
//custom
//#define OCR_ENABLE

//sa original
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
#define _OBJSEND_C
#define _NEWSHOP_
#define _NEW_CLIENT_LOGIN
#define _PETCOM_
#pragma endregion

#include "database.h"

class Autil;
class Lssproto
{
private:
	Autil* autil_ = nullptr;
public:
	explicit Lssproto(Autil* autil);
	void lssproto_W_send(const QPoint& pos, char* direction);
	void lssproto_W2_send(const QPoint& pos, char* direction);
	void lssproto_EV_send(int event, int dialogid, const QPoint& pos, int dir);
	void lssproto_EN_send(const QPoint& pos);
	void lssproto_DU_send(const QPoint& pos);
	void lssproto_EO_send(int dummy);
	void lssproto_BU_send(int dummy);
	void lssproto_JB_send(const QPoint& pos);
	void lssproto_LB_send(const QPoint& pos);
	void lssproto_B_send(const QString& command);
	void lssproto_SKD_send(int dir, int index);
	void lssproto_ID_send(const QPoint& pos, int haveitemindex, int toindex);
	void lssproto_PI_send(const QPoint& pos, int dir);
	void lssproto_DI_send(const QPoint& pos, int itemIndex);
	void lssproto_DG_send(const QPoint& pos, int amount);
	void lssproto_DP_send(const QPoint& pos, int petindex);
	void lssproto_MI_send(int fromindex, int toindex);
	void lssproto_MSG_send(int index, char* message, int color);
	void lssproto_PMSG_send(int index, int petindex, int itemIndex, char* message, int color);
	void lssproto_AB_send(int fd);
	void lssproto_DAB_send(int index);
	void lssproto_AAB_send(const QPoint& pos);
	void lssproto_L_send(int dir);
	void lssproto_TK_send(const QPoint& pos, char* message, int color, int area);
	void lssproto_M_send(int fl, int x1, int y1, int x2, int y2);
	void lssproto_C_send(int index);
	void lssproto_S_send(char* category);
	void lssproto_FS_send(int flg);
	void lssproto_HL_send(int flg);
	void lssproto_PR_send(const QPoint& pos, int request);
	void lssproto_KS_send(int petarray);
#ifdef _STANDBYPET
	void lssproto_SPET_send(int standbypet);
#endif
	void lssproto_AC_send(const QPoint& pos, int actionno);
	void lssproto_MU_send(const QPoint& pos, int array, int toindex);
	void lssproto_PS_send(int havepetindex, int havepetskill, int toindex, char* data);
	void lssproto_ST_send(int titleindex);
	void lssproto_DT_send(int titleindex);
	void lssproto_FT_send(char* data);
	void lssproto_SKUP_send(int skillid);
	void lssproto_KN_send(int havepetindex, char* data);
	void lssproto_WN_send(const QPoint& pos, int dialogid, int unitid, int select, char* data);
	void lssproto_SP_send(const QPoint& pos, int dir);
	void lssproto_ClientLogin_send(char* cdkey, char* passwd, char* mac, int selectServerIndex, char* ip, DWORD flags);
	void lssproto_CreateNewChar_send(int dataplacenum, char* charname, int imgno, int faceimgno, int vital, int str, int tgh, int dex, int earth, int water, int fire, int wind, int hometown);
	void lssproto_CharDelete_send(char* charname, char* securityCode);
	void lssproto_CharLogin_send(char* charname);

	void lssproto_CharList_send(int fd);
	void lssproto_CharLogout_send(int Flg);
	void lssproto_ProcGet_send(int fd);
	void lssproto_PlayerNumGet_send(int fd);
	void lssproto_Echo_send(char* test);
	void lssproto_Shutdown_send(char* passwd, int min);
	void lssproto_TD_send(char* data);
	void lssproto_FM_send(char* data);
	void lssproto_PETST_send(int nPet, int sPet);// sPet  0:休息 1:等待 4:郵件
	void lssproto_BM_send(int iindex);             // _BLACK_MARKET
#ifdef _FIX_DEL_MAP
	void lssproto_DM_send(int fd);                         // WON ADD 玩家抽地圖送監獄
#endif
#ifdef _MIND_ICON
	void lssproto_MA_send(const QPoint& pos, int nMind);
#endif
#ifdef _CHECK_GAMESPEED
	void lssproto_CS_send(int fd);
	int lssproto_getdelaytimes();
	void lssproto_setdelaytimes(int delays);
#endif
#ifdef _TEAM_KICKPARTY
	void lssproto_KTEAM_send(int si);
#endif
#ifdef _CHATROOMPROTOCOL			// (不可開) Syu ADD 聊天室頻道
	void lssproto_CHATROOM_send(char* data);
#endif
#ifdef _NEWREQUESTPROTOCOL			// (不可開) Syu ADD 新增Protocol要求細項
	void lssproto_RESIST_send(char* data);
#endif
#ifdef _ALCHEPLUS
	void lssproto_ALCHEPLUS_send(char* data);
#endif
#ifdef _OUTOFBATTLESKILL			// (不可開) Syu ADD 非戰鬥時技能Protocol
	void lssproto_BATTLESKILL_send(int SkillNum);
#endif
#ifdef _STREET_VENDOR
	void lssproto_STREET_VENDOR_send(char* data);	// 擺攤功能
#endif
#ifdef _JOBDAILY
	void lssproto_JOBDAILY_send(char* data);
#endif
#ifdef _FAMILYBADGE_
	void lssproto_FamilyBadge_send(int fd);
#endif
#ifdef _TEACHER_SYSTEM
	void lssproto_TEACHER_SYSTEM_send(char* data);	// 導師功能
#endif
#ifdef _ADD_STATUS_2
	void lssproto_S2_send(char* data);
#endif
#ifdef _PET_ITEM
	void lssproto_PetItemEquip_send(const QPoint& pos, int nPetNo, int nItemNo, int nDestNO);	// 寵物裝備功能
#endif
#ifdef _GET_HOSTNAME
	void lssproto_HostName_send(int fd);
#endif
#ifdef _PK2007
	void lssproto_pkList_send(int fd);
#endif
#ifdef _NEW_SYSTEM_MENU
	void lssproto_SaMenu_send(int index);
#endif
#ifdef _PETBLESS_
	void lssproto_petbless_send(int petpos, int type);
#endif
#ifdef _RIDEQUERY_
	void lssproto_RideQuery_send(int fd);
#endif
#ifdef _CHARSIGNDAY_
	void lssproto_SignDay_send(int fd);
#endif

	void lssproto_ShopOk_send(int n);

	/////////////////////////////////////////////// recv

#pragma region Lssproto_Recv
	virtual void lssproto_XYD_recv(const QPoint& pos, int dir) = 0;//戰鬥結束後的大地圖座標
	virtual void lssproto_EV_recv(int dialogid, int result) = 0;
	virtual void lssproto_EN_recv(int result, int field) = 0;
	virtual void lssproto_RS_recv(char* data) = 0;
	virtual void lssproto_RD_recv(char* data) = 0;
	virtual void lssproto_B_recv(char* command) = 0;
	virtual void lssproto_I_recv(char* data) = 0;
	virtual void lssproto_SI_recv(int fromindex, int toindex) = 0;
	virtual void lssproto_MSG_recv(int aindex, char* text, int color) = 0;	//收到普通郵件或寵物郵件
	virtual void lssproto_PME_recv(int unitid, int graphicsno, const QPoint& pos, int dir, int flg, int no, char* cdata) = 0;
	virtual void lssproto_AB_recv(char* data) = 0;
	virtual void lssproto_ABI_recv(int num, char* data) = 0;
	virtual void lssproto_TK_recv(int index, char* message, int color) = 0;
	virtual void lssproto_MC_recv(int fl, int x1, int y1, int x2, int y2, int tilesum, int objsum, int eventsum, char* data) = 0;
	virtual void lssproto_M_recv(int fl, int x1, int y1, int x2, int y2, char* data) = 0;
	virtual void lssproto_C_recv(char* data) = 0;
	virtual void lssproto_CA_recv(char* data) = 0;
	virtual void lssproto_CD_recv(char* data) = 0;
	virtual void lssproto_R_recv(char* data) = 0;
	virtual void lssproto_S_recv(char* data) = 0;
	virtual void lssproto_D_recv(int category, int dx, int dy, char* data) = 0;
	virtual void lssproto_FS_recv(int flg) = 0;
	virtual void lssproto_HL_recv(int flg) = 0;//戰鬥中是否要Help
	virtual void lssproto_PR_recv(int request, int result) = 0;
	virtual void lssproto_KS_recv(int petarray, int result) = 0;//指定那一只寵物出場戰鬥
#ifdef _STANDBYPET
	virtual void lssproto_SPET_recv(int standbypet, int result) = 0;
#endif
	virtual void lssproto_PS_recv(int result, int havepetindex, int havepetskill, int toindex) = 0;	//寵物合成
	virtual void lssproto_SKUP_recv(int point) = 0;//取得可加的屬性點數
	virtual void lssproto_WN_recv(int windowtype, int buttontype, int dialogid, int unitid, char* data) = 0;
	virtual void lssproto_EF_recv(int effect, int level, char* option) = 0;
	virtual void lssproto_SE_recv(const QPoint& pos, int senumber, int sw) = 0;
	virtual void lssproto_ClientLogin_recv(char* result) = 0;
	virtual void lssproto_CreateNewChar_recv(char* result, char* data) = 0;
	virtual void lssproto_CharDelete_recv(char* result, char* data) = 0;
	virtual void lssproto_CharLogin_recv(char* result, char* data) = 0;
	virtual void lssproto_CharList_recv(char* result, char* data) = 0;
	virtual void lssproto_CharLogout_recv(char* result, char* data) = 0;
	virtual void lssproto_ProcGet_recv(char* data) = 0;
	virtual void lssproto_PlayerNumGet_recv(int logincount, int player) = 0;
	virtual void lssproto_Echo_recv(char* test) = 0;
	virtual void lssproto_NU_recv(int AddCount) = 0;
	virtual void lssproto_WO_recv(int effect) = 0;//取得轉生的特效
	virtual void lssproto_TD_recv(char* data) = 0;
	virtual void lssproto_FM_recv(char* data) = 0;
#ifdef _ITEM_CRACKER
	virtual void lssproto_IC_recv(const QPoint& pos) = 0;
#endif
#ifdef _MAGIC_NOCAST//沈默
	virtual void lssproto_NC_recv(int flg) = 0;
#endif
#ifdef _CHECK_GAMESPEED
	virtual void lssproto_CS_recv(int deltimes) = 0;
#endif
#ifdef _PETS_SELECTCON
	virtual void lssproto_PETST_recv(int petarray, int result) = 0;
#endif
#ifdef _CHATROOMPROTOCOL			// (不可開) 聊天室頻道
	virtual void lssproto_CHATROOM_recv(char* data) = 0;
#endif
#ifdef _NEWREQUESTPROTOCOL			// (不可開) 新增Protocol要求細項
	virtual void lssproto_RESIST_recv(char* data) = 0;
#endif
#ifdef _ALCHEPLUS
	virtual void lssproto_ALCHEPLUS_recv(char* data) = 0;
#endif

#ifdef _OUTOFBATTLESKILL			// (不可開) Syu ADD 非戰鬥時技能Protocol
	virtual void lssproto_BATTLESKILL_recv(char* data) = 0;
#endif
	virtual void lssproto_CHAREFFECT_recv(char* data) = 0;

#ifdef _STREET_VENDOR
	virtual void lssproto_STREET_VENDOR_recv(char* data) = 0;	// 擺攤功能
#endif

#ifdef _JOBDAILY
	virtual void lssproto_JOBDAILY_recv(char* data) = 0;
#endif

#ifdef _FAMILYBADGE_
	virtual void lssproto_FamilyBadge_recv(char* data) = 0;
#endif

#ifdef _TEACHER_SYSTEM
	virtual void lssproto_TEACHER_SYSTEM_recv(char* data) = 0;
#endif

#ifdef _ADD_STATUS_2
	virtual void lssproto_S2_recv(char* data) = 0;
#endif
#ifdef _ITEM_FIREWORK
	virtual void lssproto_Firework_recv(int nCharaindex, int nType, int nActionNum) = 0;	// 煙火功能
#endif
#ifdef _THEATER
	virtual void lssproto_TheaterData_recv(char* pData) = 0;
#endif
#ifdef _MOVE_SCREEN
	virtual void lssproto_MoveScreen_recv(BOOL bMoveScreenMode, int iXY) = 0;	// client 移動熒幕
#endif
#ifdef _NPC_MAGICCARD
	virtual void lssproto_MagiccardAction_recv(char* data) = 0;	//魔法牌功能
	virtual void lssproto_MagiccardDamage_recv(int position, int damage, int offsetx, int offsety) = 0;
#endif
#ifdef  _NPC_DANCE
	virtual void lssproto_DancemanOption_recv(int option) = 0;	//動一動狀態
#endif
#ifdef _ANNOUNCEMENT_
	virtual void lssproto_DENGON_recv(char* data, int colors, int nums) = 0;
#endif
#ifdef _HUNDRED_KILL
	virtual void lssproto_hundredkill_recv(int flag) = 0;
#endif
#ifdef _PK2007
	virtual void lssproto_pkList_recv(int count, char* data) = 0;
#endif
#ifdef _PETBLESS_
	virtual void lssproto_petbless_send(int petpos, int type) = 0;
#endif
#ifdef _PET_SKINS
	virtual void lssproto_PetSkins_recv(char* data) = 0;
#endif

	//////////////////////////////////

	virtual void lssproto_CustomWN_recv(const QString& data) = 0;
	virtual void lssproto_CustomTK_recv(const QString& data) = 0;
#pragma endregion
};