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



class Autil;
class Lssproto
{
private:
	Autil* autil_ = nullptr;
public:
	explicit Lssproto(Autil* autil);
	void __fastcall lssproto_W_send(const QPoint& pos, char* direction);
	void __fastcall lssproto_W2_send(const QPoint& pos, char* direction);
	void __fastcall lssproto_EV_send(int event, int dialogid, const QPoint& pos, int dir);
	void __fastcall lssproto_EN_send(const QPoint& pos);
	void __fastcall lssproto_DU_send(const QPoint& pos);
	void __fastcall lssproto_EO_send(int dummy);
	void __fastcall lssproto_BU_send(int dummy);
	void __fastcall lssproto_JB_send(const QPoint& pos);
	void __fastcall lssproto_LB_send(const QPoint& pos);
	void __fastcall lssproto_B_send(const QString& command);
	void __fastcall lssproto_SKD_send(int dir, int index);
	void __fastcall lssproto_ID_send(const QPoint& pos, int haveitemindex, int toindex);
	void __fastcall lssproto_PI_send(const QPoint& pos, int dir);
	void __fastcall lssproto_DI_send(const QPoint& pos, int itemIndex);
	void __fastcall lssproto_DG_send(const QPoint& pos, int amount);
	void __fastcall lssproto_DP_send(const QPoint& pos, int petindex);
	void __fastcall lssproto_MI_send(int fromindex, int toindex);
	void __fastcall lssproto_MSG_send(int index, char* message, int color);
	void __fastcall lssproto_PMSG_send(int index, int petindex, int itemIndex, char* message, int color);
	void __fastcall lssproto_AB_send(int fd);
	void __fastcall lssproto_DAB_send(int index);
	void __fastcall lssproto_AAB_send(const QPoint& pos);
	void __fastcall lssproto_L_send(int dir);
	void __fastcall lssproto_TK_send(const QPoint& pos, char* message, int color, int area);
	void __fastcall lssproto_M_send(int fl, int x1, int y1, int x2, int y2);
	void __fastcall lssproto_C_send(int index);
	void __fastcall lssproto_S_send(char* category);
	void __fastcall lssproto_FS_send(int flg);
	void __fastcall lssproto_HL_send(int flg);
	void __fastcall lssproto_PR_send(const QPoint& pos, int request);
	void __fastcall lssproto_KS_send(int petarray);
#ifdef _STANDBYPET
	void __fastcall lssproto_SPET_send(int standbypet);
#endif
	void __fastcall lssproto_AC_send(const QPoint& pos, int actionno);
	void __fastcall lssproto_MU_send(const QPoint& pos, int array, int toindex);
	void __fastcall lssproto_PS_send(int havepetindex, int havepetskill, int toindex, char* data);
	void __fastcall lssproto_ST_send(int titleindex);
	void __fastcall lssproto_DT_send(int titleindex);
	void __fastcall lssproto_FT_send(char* data);
	void __fastcall lssproto_SKUP_send(int skillid);
	void __fastcall lssproto_KN_send(int havepetindex, char* data);
	void __fastcall lssproto_WN_send(const QPoint& pos, int dialogid, int unitid, int select, char* data);
	void __fastcall lssproto_SP_send(const QPoint& pos, int dir);
	void __fastcall lssproto_ClientLogin_send(char* cdkey, char* passwd, char* mac, int selectServerIndex, char* ip, DWORD flags);
	void __fastcall lssproto_CreateNewChar_send(int dataplacenum, char* charname, int imgno, int faceimgno, int vital, int str, int tgh, int dex, int earth, int water, int fire, int wind, int hometown);
	void __fastcall lssproto_CharDelete_send(char* charname, char* securityCode);
	void __fastcall lssproto_CharLogin_send(char* charname);

	void __fastcall lssproto_CharList_send(int fd);
	void __fastcall lssproto_CharLogout_send(int Flg);
	void __fastcall lssproto_ProcGet_send(int fd);
	void __fastcall lssproto_CharNumGet_send(int fd);
	void __fastcall lssproto_Echo_send(char* test);
	void __fastcall lssproto_Shutdown_send(char* passwd, int min);
	void __fastcall lssproto_TD_send(char* data);
	void __fastcall lssproto_FM_send(char* data);
	void __fastcall lssproto_PETST_send(int nPet, int sPet);// sPet  0:休息 1:等待 4:郵件
	void __fastcall lssproto_BM_send(int iindex);             // _BLACK_MARKET
#ifdef _FIX_DEL_MAP
	void __fastcall lssproto_DM_send(int fd);                         // WON ADD 玩家抽地圖送監獄
#endif

	void __fastcall lssproto_MA_send(const QPoint& pos, int nMind);

#ifdef _CHECK_GAMESPEED
	void __fastcall lssproto_CS_send(int fd);
	int lssproto_getdelaytimes();
	void __fastcall lssproto_setdelaytimes(int delays);
#endif
#ifdef _TEAM_KICKPARTY
	void __fastcall lssproto_KTEAM_send(int si);
#endif
#ifdef _CHATROOMPROTOCOL			// (不可開) Syu ADD 聊天室頻道
	void __fastcall lssproto_CHATROOM_send(char* data);
#endif
#ifdef _NEWREQUESTPROTOCOL			// (不可開) Syu ADD 新增Protocol要求細項
	void __fastcall lssproto_RESIST_send(char* data);
#endif
#ifdef _ALCHEPLUS
	void __fastcall lssproto_ALCHEPLUS_send(char* data);
#endif
#ifdef _OUTOFBATTLESKILL			// (不可開) Syu ADD 非戰鬥時技能Protocol
	void __fastcall lssproto_BATTLESKILL_send(int SkillNum);
#endif
#ifdef _STREET_VENDOR
	void __fastcall lssproto_STREET_VENDOR_send(char* data);	// 擺攤功能
#endif
#ifdef _JOBDAILY
	void __fastcall lssproto_JOBDAILY_send(char* data);
#endif
#ifdef _FAMILYBADGE_
	void __fastcall lssproto_FamilyBadge_send(int fd);
#endif
#ifdef _TEACHER_SYSTEM
	void __fastcall lssproto_TEACHER_SYSTEM_send(char* data);	// 導師功能
#endif

	void __fastcall lssproto_S2_send(char* data);

#ifdef _PET_ITEM
	void __fastcall lssproto_PetItemEquip_send(const QPoint& pos, int nPetNo, int nItemNo, int nDestNO);	// 寵物裝備功能
#endif
#ifdef _GET_HOSTNAME
	void __fastcall lssproto_HostName_send(int fd);
#endif
#ifdef _PK2007
	void __fastcall lssproto_pkList_send(int fd);
#endif
#ifdef _NEW_SYSTEM_MENU
	void __fastcall lssproto_SaMenu_send(int index);
#endif
#ifdef _PETBLESS_
	void __fastcall lssproto_petbless_send(int petpos, int type);
#endif
#ifdef _RIDEQUERY_
	void __fastcall lssproto_RideQuery_send(int fd);
#endif
#ifdef _CHARSIGNDAY_
	void __fastcall lssproto_SignDay_send(int fd);
#endif

	void __fastcall lssproto_ShopOk_send(int n);

	/////////////////////////////////////////////// recv

#pragma region Lssproto_Recv
	virtual void __fastcall lssproto_XYD_recv(const QPoint& pos, int dir) = 0;//戰鬥結束後的大地圖座標
	virtual void __fastcall lssproto_EV_recv(int dialogid, int result) = 0;
	virtual void __fastcall lssproto_EN_recv(int result, int field) = 0;
	virtual void __fastcall lssproto_RS_recv(char* data) = 0;
	virtual void __fastcall lssproto_RD_recv(char* data) = 0;
	virtual void __fastcall lssproto_B_recv(char* command) = 0;
	virtual void __fastcall lssproto_I_recv(char* data) = 0;
	virtual void __fastcall lssproto_SI_recv(int fromindex, int toindex) = 0;
	virtual void __fastcall lssproto_MSG_recv(int aindex, char* text, int color) = 0;	//收到普通郵件或寵物郵件
	virtual void __fastcall lssproto_PME_recv(int unitid, int graphicsno, const QPoint& pos, int dir, int flg, int no, char* cdata) = 0;
	virtual void __fastcall lssproto_AB_recv(char* data) = 0;
	virtual void __fastcall lssproto_ABI_recv(int num, char* data) = 0;
	virtual void __fastcall lssproto_TK_recv(int index, char* message, int color) = 0;
	virtual void __fastcall lssproto_MC_recv(int fl, int x1, int y1, int x2, int y2, int tilesum, int objsum, int eventsum, char* data) = 0;
	virtual void __fastcall lssproto_M_recv(int fl, int x1, int y1, int x2, int y2, char* data) = 0;
	virtual void __fastcall lssproto_C_recv(char* data) = 0;
	virtual void __fastcall lssproto_CA_recv(char* data) = 0;
	virtual void __fastcall lssproto_CD_recv(char* data) = 0;
	virtual void __fastcall lssproto_R_recv(char* data) = 0;
	virtual void __fastcall lssproto_S_recv(char* data) = 0;
	virtual void __fastcall lssproto_D_recv(int category, int dx, int dy, char* data) = 0;
	virtual void __fastcall lssproto_FS_recv(int flg) = 0;
	virtual void __fastcall lssproto_HL_recv(int flg) = 0;//戰鬥中是否要Help
	virtual void __fastcall lssproto_PR_recv(int request, int result) = 0;
	virtual void __fastcall lssproto_KS_recv(int petarray, int result) = 0;//指定那一只寵物出場戰鬥
#ifdef _STANDBYPET
	virtual void __fastcall lssproto_SPET_recv(int standbypet, int result) = 0;
#endif
	virtual void __fastcall lssproto_PS_recv(int result, int havepetindex, int havepetskill, int toindex) = 0;	//寵物合成
	virtual void __fastcall lssproto_SKUP_recv(int point) = 0;//取得可加的屬性點數
	virtual void __fastcall lssproto_WN_recv(int windowtype, int buttontype, int dialogid, int unitid, char* data) = 0;
	virtual void __fastcall lssproto_EF_recv(int effect, int level, char* option) = 0;
	virtual void __fastcall lssproto_SE_recv(const QPoint& pos, int senumber, int sw) = 0;
	virtual void __fastcall lssproto_ClientLogin_recv(char* result) = 0;
	virtual void __fastcall lssproto_CreateNewChar_recv(char* result, char* data) = 0;
	virtual void __fastcall lssproto_CharDelete_recv(char* result, char* data) = 0;
	virtual void __fastcall lssproto_CharLogin_recv(char* result, char* data) = 0;
	virtual void __fastcall lssproto_CharList_recv(char* result, char* data) = 0;
	virtual void __fastcall lssproto_CharLogout_recv(char* result, char* data) = 0;
	virtual void __fastcall lssproto_ProcGet_recv(char* data) = 0;
	virtual void __fastcall lssproto_CharNumGet_recv(int logincount, int player) = 0;
	virtual void __fastcall lssproto_Echo_recv(char* test) = 0;
	virtual void __fastcall lssproto_NU_recv(int AddCount) = 0;
	virtual void __fastcall lssproto_WO_recv(int effect) = 0;//取得轉生的特效
	virtual void __fastcall lssproto_TD_recv(char* data) = 0;
	virtual void __fastcall lssproto_FM_recv(char* data) = 0;
#ifdef _ITEM_CRACKER
	virtual void __fastcall lssproto_IC_recv(const QPoint& pos) = 0;
#endif
#ifdef _MAGIC_NOCAST//沈默
	virtual void __fastcall lssproto_NC_recv(int flg) = 0;
#endif
#ifdef _CHECK_GAMESPEED
	virtual void __fastcall lssproto_CS_recv(int deltimes) = 0;
#endif
#ifdef _PETS_SELECTCON
	virtual void __fastcall lssproto_PETST_recv(int petarray, int result) = 0;
#endif
#ifdef _CHATROOMPROTOCOL			// (不可開) 聊天室頻道
	virtual void __fastcall lssproto_CHATROOM_recv(char* data) = 0;
#endif
#ifdef _NEWREQUESTPROTOCOL			// (不可開) 新增Protocol要求細項
	virtual void __fastcall lssproto_RESIST_recv(char* data) = 0;
#endif
#ifdef _ALCHEPLUS
	virtual void __fastcall lssproto_ALCHEPLUS_recv(char* data) = 0;
#endif

#ifdef _OUTOFBATTLESKILL			// (不可開) Syu ADD 非戰鬥時技能Protocol
	virtual void __fastcall lssproto_BATTLESKILL_recv(char* data) = 0;
#endif
	virtual void __fastcall lssproto_CHAREFFECT_recv(char* data) = 0;

#ifdef _STREET_VENDOR
	virtual void __fastcall lssproto_STREET_VENDOR_recv(char* data) = 0;	// 擺攤功能
#endif

#ifdef _JOBDAILY
	virtual void __fastcall lssproto_JOBDAILY_recv(char* data) = 0;
#endif

#ifdef _FAMILYBADGE_
	virtual void __fastcall lssproto_FamilyBadge_recv(char* data) = 0;
#endif

#ifdef _TEACHER_SYSTEM
	virtual void __fastcall lssproto_TEACHER_SYSTEM_recv(char* data) = 0;
#endif

	virtual void __fastcall lssproto_S2_recv(char* data) = 0;

#ifdef _ITEM_FIREWORK
	virtual void __fastcall lssproto_Firework_recv(int nCharaindex, int nType, int nActionNum) = 0;	// 煙火功能
#endif
#ifdef _THEATER
	virtual void __fastcall lssproto_TheaterData_recv(char* pData) = 0;
#endif
#ifdef _MOVE_SCREEN
	virtual void __fastcall lssproto_MoveScreen_recv(BOOL bMoveScreenMode, int iXY) = 0;	// client 移動熒幕
#endif
#ifdef _NPC_MAGICCARD
	virtual void __fastcall lssproto_MagiccardAction_recv(char* data) = 0;	//魔法牌功能
	virtual void __fastcall lssproto_MagiccardDamage_recv(int position, int damage, int offsetx, int offsety) = 0;
#endif
#ifdef  _NPC_DANCE
	virtual void __fastcall lssproto_DancemanOption_recv(int option) = 0;	//動一動狀態
#endif
#ifdef _ANNOUNCEMENT_
	virtual void __fastcall lssproto_DENGON_recv(char* data, int colors, int nums) = 0;
#endif
#ifdef _HUNDRED_KILL
	virtual void __fastcall lssproto_hundredkill_recv(int flag) = 0;
#endif
#ifdef _PK2007
	virtual void __fastcall lssproto_pkList_recv(int count, char* data) = 0;
#endif
#ifdef _PETBLESS_
	virtual void __fastcall lssproto_petbless_send(int petpos, int type) = 0;
#endif
#ifdef _PET_SKINS
	virtual void __fastcall lssproto_PetSkins_recv(char* data) = 0;
#endif

	//////////////////////////////////

	virtual void __fastcall lssproto_CustomWN_recv(const QString& data) = 0;
	virtual void __fastcall lssproto_CustomTK_recv(const QString& data) = 0;
#pragma endregion
};