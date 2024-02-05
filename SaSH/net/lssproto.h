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



class Autil;
class Lssproto
{
private:
	Autil* autil_ = nullptr;
public:
	explicit Lssproto(Autil* autil);
	bool __fastcall lssproto_W_send(const QPoint& pos, char* direction);
	bool __fastcall lssproto_W2_send(const QPoint& pos, char* direction);
	bool __fastcall lssproto_EV_send(long long event, long long dialogid, const QPoint& pos, long long dir);
	bool __fastcall lssproto_EN_send(const QPoint& pos);
	bool __fastcall lssproto_DU_send(const QPoint& pos);
	bool __fastcall lssproto_EO_send(long long dummy);
	bool __fastcall lssproto_BU_send(long long dummy);
	bool __fastcall lssproto_JB_send(const QPoint& pos);
	bool __fastcall lssproto_LB_send(const QPoint& pos);
	bool __fastcall lssproto_B_send(const QString& command);
	bool __fastcall lssproto_SKD_send(long long dir, long long index);
	bool __fastcall lssproto_ID_send(const QPoint& pos, long long haveitemindex, long long toindex);
	bool __fastcall lssproto_PI_send(const QPoint& pos, long long dir);
	bool __fastcall lssproto_DI_send(const QPoint& pos, long long itemIndex);
	bool __fastcall lssproto_DG_send(const QPoint& pos, long long amount);
	bool __fastcall lssproto_DP_send(const QPoint& pos, long long petindex);
	bool __fastcall lssproto_MI_send(long long fromindex, long long toindex);
	bool __fastcall lssproto_MSG_send(long long index, char* message, long long color);
	bool __fastcall lssproto_PMSG_send(long long index, long long petindex, long long itemIndex, char* message, long long color);
	bool __fastcall lssproto_AB_send(long long fd);
	bool __fastcall lssproto_DAB_send(long long index);
	bool __fastcall lssproto_AAB_send(const QPoint& pos);
	bool __fastcall lssproto_L_send(long long dir);
	bool __fastcall lssproto_TK_send(const QPoint& pos, char* message, long long color, long long area);
	bool __fastcall lssproto_M_send(long long fl, long long x1, long long y1, long long x2, long long y2);
	bool __fastcall lssproto_C_send(long long index);
	bool __fastcall lssproto_S_send(char* category);
	bool __fastcall lssproto_FS_send(long long flg);
	bool __fastcall lssproto_HL_send(long long flg);
	bool __fastcall lssproto_PR_send(const QPoint& pos, long long request);
	bool __fastcall lssproto_KS_send(long long petarray);

	bool __fastcall lssproto_SPET_send(long long standbypet);

	bool __fastcall lssproto_AC_send(const QPoint& pos, long long actionno);
	bool __fastcall lssproto_MU_send(const QPoint& pos, long long array, long long toindex);
	bool __fastcall lssproto_PS_send(long long havepetindex, long long havepetskill, long long toindex, char* data);
	bool __fastcall lssproto_ST_send(long long titleindex);
	bool __fastcall lssproto_DT_send(long long titleindex);
	bool __fastcall lssproto_FT_send(char* data);
	bool __fastcall lssproto_SKUP_send(long long skillid);
	bool __fastcall lssproto_KN_send(long long havepetindex, char* data);
	bool __fastcall lssproto_WN_send(const QPoint& pos, long long dialogid, long long unitid, long long select, char* data);
	bool __fastcall lssproto_SP_send(const QPoint& pos, long long dir);
	bool __fastcall lssproto_ClientLogin_send(char* cdkey, char* passwd, char* mac, long long selectServerIndex, char* ip, DWORD flags);
	bool __fastcall lssproto_CreateNewChar_send(long long dataplacenum, char* charname, long long imgno, long long faceimgno, long long vital, long long str, long long tgh, long long dex, long long earth, long long water, long long fire, long long wind, long long hometown);
	bool __fastcall lssproto_CharDelete_send(char* charname, char* securityCode);
	bool __fastcall lssproto_CharLogin_send(char* charname);

	bool __fastcall lssproto_CharList_send(long long fd);
	bool __fastcall lssproto_CharLogout_send(long long Flg);
	bool __fastcall lssproto_ProcGet_send(long long fd);
	bool __fastcall lssproto_CharNumGet_send(long long fd);
	bool __fastcall lssproto_Echo_send(char* test);
	bool __fastcall lssproto_Shutdown_send(char* passwd, long long min);
	bool __fastcall lssproto_TD_send(char* data);
	bool __fastcall lssproto_FM_send(char* data);
	bool __fastcall lssproto_PETST_send(long long nPet, long long sPet);// sPet  0:休息 1:等待 4:郵件
	bool __fastcall lssproto_BM_send(long long iindex);             // _BLACK_MARKET

	bool __fastcall lssproto_DM_send(long long fd);                         // WON ADD 玩家抽地圖送監獄


	bool __fastcall lssproto_MA_send(const QPoint& pos, long long nMind);

	bool __fastcall lssproto_CS_send(long long fd);
	long long lssproto_getdelaytimes();
	bool __fastcall lssproto_setdelaytimes(long long delays);

	bool __fastcall lssproto_KTEAM_send(long long si);

	//聊天室頻道
	bool __fastcall lssproto_CHATROOM_send(char* data);
	// 新增Protocol要求細項
	bool __fastcall lssproto_RESIST_send(char* data);

	bool __fastcall lssproto_ALCHEPLUS_send(char* data);
	//非戰鬥時技能Protocol
	bool __fastcall lssproto_BATTLESKILL_send(long long SkillNum);

	bool __fastcall lssproto_STREET_VENDOR_send(char* data);	// 擺攤功能

	bool __fastcall lssproto_missionInfo_send(char* data);

	bool __fastcall lssproto_FamilyBadge_send(long long fd);

	bool __fastcall lssproto_TEACHER_SYSTEM_send(char* data);	// 導師功能

	bool __fastcall lssproto_S2_send(char* data);

	bool __fastcall lssproto_PetItemEquip_send(const QPoint& pos, long long nPetNo, long long nItemNo, long long nDestNO);	// 寵物裝備功能

	bool __fastcall lssproto_HostName_send(long long fd);

	bool __fastcall lssproto_pkList_send(long long fd);

	bool __fastcall lssproto_SaMenu_send(long long index);

	bool __fastcall lssproto_petbless_send(long long petpos, long long type);

	bool __fastcall lssproto_RideQuery_send(long long fd);

	bool __fastcall lssproto_SignDay_send(long long fd);

	bool __fastcall lssproto_ShopOk_send(long long n);

	/////////////////////////////////////////////// recv

#pragma region Lssproto_Recv
	virtual void __fastcall lssproto_XYD_recv(const QPoint& pos, long long dir) = 0;//戰鬥結束後的大地圖座標
	virtual void __fastcall lssproto_EV_recv(long long dialogid, long long result) = 0;
	virtual void __fastcall lssproto_EN_recv(long long result, long long field) = 0;
	virtual void __fastcall lssproto_RS_recv(char* data) = 0;
	virtual void __fastcall lssproto_RD_recv(char* data) = 0;
	virtual void __fastcall lssproto_B_recv(char* command) = 0;
	virtual void __fastcall lssproto_I_recv(char* data) = 0;
	virtual void __fastcall lssproto_SI_recv(long long fromindex, long long toindex) = 0;
	virtual void __fastcall lssproto_MSG_recv(long long aindex, char* text, long long color) = 0;	//收到普通郵件或寵物郵件
	virtual void __fastcall lssproto_PME_recv(long long unitid, long long graphicsno, const QPoint& pos, long long dir, long long flg, long long no, char* cdata) = 0;
	virtual void __fastcall lssproto_AB_recv(char* data) = 0;
	virtual void __fastcall lssproto_ABI_recv(long long num, char* data) = 0;
	virtual void __fastcall lssproto_TK_recv(long long index, char* message, long long color) = 0;
	virtual void __fastcall lssproto_MC_recv(long long fl, long long x1, long long y1, long long x2, long long y2, long long tilesum, long long objsum, long long eventsum, char* data) = 0;
	virtual void __fastcall lssproto_M_recv(long long fl, long long x1, long long y1, long long x2, long long y2, char* data) = 0;
	virtual void __fastcall lssproto_C_recv(char* data) = 0;
	virtual void __fastcall lssproto_CA_recv(char* data) = 0;
	virtual void __fastcall lssproto_CD_recv(char* data) = 0;
	virtual void __fastcall lssproto_R_recv(char* data) = 0;
	virtual void __fastcall lssproto_S_recv(char* data) = 0;
	virtual void __fastcall lssproto_D_recv(long long category, long long dx, long long dy, char* data) = 0;
	virtual void __fastcall lssproto_FS_recv(long long flg) = 0;
	virtual void __fastcall lssproto_HL_recv(long long flg) = 0;//戰鬥中是否要Help
	virtual void __fastcall lssproto_PR_recv(long long request, long long result) = 0;
	virtual void __fastcall lssproto_KS_recv(long long petarray, long long result) = 0;//指定那一只寵物出場戰鬥

	virtual void __fastcall lssproto_SPET_recv(long long standbypet, long long result) = 0;

	virtual void __fastcall lssproto_PS_recv(long long result, long long havepetindex, long long havepetskill, long long toindex) = 0;	//寵物合成
	virtual void __fastcall lssproto_SKUP_recv(long long point) = 0;//取得可加的屬性點數
	virtual void __fastcall lssproto_WN_recv(long long windowtype, long long buttontype, long long dialogid, long long unitid, char* data) = 0;
	virtual void __fastcall lssproto_EF_recv(long long effect, long long level, char* option) = 0;
	virtual void __fastcall lssproto_SE_recv(const QPoint& pos, long long senumber, long long sw) = 0;
	virtual void __fastcall lssproto_ClientLogin_recv(char* result) = 0;
	virtual void __fastcall lssproto_CreateNewChar_recv(char* result, char* data) = 0;
	virtual void __fastcall lssproto_CharDelete_recv(char* result, char* data) = 0;
	virtual void __fastcall lssproto_CharLogin_recv(char* result, char* data) = 0;
	virtual void __fastcall lssproto_CharList_recv(char* result, char* data) = 0;
	virtual void __fastcall lssproto_CharLogout_recv(char* result, char* data) = 0;
	virtual void __fastcall lssproto_ProcGet_recv(char* data) = 0;
	virtual void __fastcall lssproto_CharNumGet_recv(long long logincount, long long player) = 0;
	virtual void __fastcall lssproto_Echo_recv(char* test) = 0;
	virtual void __fastcall lssproto_NU_recv(long long AddCount) = 0;
	virtual void __fastcall lssproto_WO_recv(long long effect) = 0;//取得轉生的特效
	virtual void __fastcall lssproto_TD_recv(char* data) = 0;
	virtual void __fastcall lssproto_FM_recv(char* data) = 0;

	virtual void __fastcall lssproto_IC_recv(const QPoint& pos) = 0;

	virtual void __fastcall lssproto_NC_recv(long long flg) = 0;

	virtual void __fastcall lssproto_CS_recv(long long deltimes) = 0;

	virtual void __fastcall lssproto_PETST_recv(long long petarray, long long result) = 0;
	//聊天室頻道
	virtual void __fastcall lssproto_CHATROOM_recv(char* data) = 0;
	// 新增Protocol要求細項
	virtual void __fastcall lssproto_RESIST_recv(char* data) = 0;

	virtual void __fastcall lssproto_ALCHEPLUS_recv(char* data) = 0;
	//非戰鬥時技能Protocol
	virtual void __fastcall lssproto_BATTLESKILL_recv(char* data) = 0;

	virtual void __fastcall lssproto_CHAREFFECT_recv(char* data) = 0;

	virtual void __fastcall lssproto_STREET_VENDOR_recv(char* data) = 0;	// 擺攤功能

	virtual void __fastcall lssproto_missionInfo_recv(char* data) = 0;

	virtual void __fastcall lssproto_FamilyBadge_recv(char* data) = 0;

	virtual void __fastcall lssproto_TEACHER_SYSTEM_recv(char* data) = 0;

	virtual void __fastcall lssproto_S2_recv(char* data) = 0;

	virtual void __fastcall lssproto_Firework_recv(long long nCharaindex, long long nType, long long nActionNum) = 0;	// 煙火功能

	virtual void __fastcall lssproto_TheaterData_recv(char* pData) = 0;

	virtual void __fastcall lssproto_MoveScreen_recv(BOOL bMoveScreenMode, long long iXY) = 0;	// client 移動熒幕

	virtual void __fastcall lssproto_MagiccardAction_recv(char* data) = 0;	//魔法牌功能
	virtual void __fastcall lssproto_MagiccardDamage_recv(long long position, long long damage, long long offsetx, long long offsety) = 0;

	virtual void __fastcall lssproto_DancemanOption_recv(long long option) = 0;	//動一動狀態

	virtual void __fastcall lssproto_DENGON_recv(char* data, long long colors, long long nums) = 0;

	virtual void __fastcall lssproto_hundredkill_recv(long long flag) = 0;

	virtual void __fastcall lssproto_pkList_recv(long long count, char* data) = 0;

	virtual void __fastcall lssproto_PetSkins_recv(char* data) = 0;

	//////////////////////////////////

	virtual bool __fastcall lssproto_CustomWN_recv(const QString& data) = 0;
	virtual bool __fastcall lssproto_CustomTK_recv(const QString& data) = 0;
#pragma endregion
};