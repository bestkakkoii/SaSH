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

#include "stdafx.h"
#include "lssproto.h"
#include "autil.h"
#include "injector.h"
#include "signaldispatcher.h"

Lssproto::Lssproto(Autil* autil)
	: autil_(autil)
{

}

// 0斷線1回點
void Lssproto::lssproto_CharLogout_send(int Flg)
{
	autil_->util_Send(LSSPROTO_CHARLOGOUT_SEND, Flg);
}

//往指定方向查看觸發對話的效果
void Lssproto::lssproto_L_send(int dir)
{
	autil_->util_Send(LSSPROTO_L_SEND, dir);
}

//遇敵
void Lssproto::lssproto_EN_send(const QPoint& pos)
{
	autil_->util_Send(LSSPROTO_EN_SEND, pos.x(), pos.y());
}

//退出觀戰
void Lssproto::lssproto_BU_send(int dummy)
{
	autil_->util_Send(LSSPROTO_BU_SEND, dummy);
}

//開關封包
void Lssproto::lssproto_FS_send(int flg)
{
	autil_->util_Send(LSSPROTO_FS_SEND, flg);
}

//對話框封包 關於dialogid: 送買242 賣243
void Lssproto::lssproto_WN_send(const QPoint& pos, int dialogid, int unitid, int select, char* data)
{
	autil_->util_Send(LSSPROTO_WN_SEND, pos.x(), pos.y(), dialogid, unitid, select, data);
}

//設置寵物狀態封包  0:休息 1:戰鬥或等待 4:郵件
void Lssproto::lssproto_PETST_send(int nPet, int sPet)
{
	autil_->util_Send(LSSPROTO_PETST_SEND, nPet, sPet);

}

//設置戰寵封包
void Lssproto::lssproto_KS_send(int petarray)
{
	autil_->util_Send(LSSPROTO_KS_SEND, petarray);
}

//設置寵物等待狀態
//1:寵物1處於等待狀態
//2:寵物2處於等待狀態
//4：寵物3處於等待狀態
//8：寵物4處於等待狀態
//16：寵物5處於等待狀態，這些值可相互組合，如5代表寵物1和寵物3處於等待狀態
void Lssproto::lssproto_SPET_send(int standbypet)
{
	autil_->util_Send(LSSPROTO_SPET_SEND, standbypet);
}

//移動轉向封包 (a-h方向移動 A-H轉向)
void Lssproto::lssproto_W2_send(const QPoint& pos, char* direction)
{
	autil_->util_Send(LSSPROTO_W2_SEND, pos.x(), pos.y(), direction);
}

//發送喊話封包
void Lssproto::lssproto_TK_send(const QPoint& pos, char* message, int color, int area)
{
	autil_->util_Send(LSSPROTO_TK_SEND, pos.x(), pos.y(), message, color, area);
}

//EO封包
void Lssproto::lssproto_EO_send(int dummy)
{
	autil_->util_Send(LSSPROTO_EO_SEND, dummy);
}

//ECHO封包
void Lssproto::lssproto_Echo_send(char* test)
{
	autil_->util_Send(LSSPROTO_ECHO_SEND, test);
}

//丟棄道具封包
void Lssproto::lssproto_DI_send(const QPoint& pos, int itemIndex)
{
	autil_->util_Send(LSSPROTO_DI_SEND, pos.x(), pos.y(), itemIndex);
}

//使用道具封包
void Lssproto::lssproto_ID_send(const QPoint& pos, int haveitemindex, int toindex)
{
	autil_->util_Send(LSSPROTO_ID_SEND, pos.x(), pos.y(), haveitemindex, toindex);
}

//交換道具封包
void Lssproto::lssproto_MI_send(int fromindex, int toindex)
{
	autil_->util_Send(LSSPROTO_MI_SEND, fromindex, toindex);
}

//撿道具封包
void Lssproto::lssproto_PI_send(const QPoint& pos, int dir)
{
	autil_->util_Send(LSSPROTO_PI_SEND, pos.x(), pos.y(), dir);
}

//料理封包
void Lssproto::lssproto_PS_send(int havepetindex, int havepetskill, int toindex, char* data)
{
	autil_->util_Send(LSSPROTO_PS_SEND, havepetindex, havepetskill, toindex, data);
}

//存取家族個人銀行封包 家族個人"B|G|%d" 正數存 負數取  家族共同 "B|T|%d"
//設置騎乘 "R|P|寵物編號0-4"，"R|P|-1"取消騎乘, S|P取聲望資訊
void Lssproto::lssproto_FM_send(char* data)
{
	autil_->util_Send(LSSPROTO_FM_SEND, data);
}

//丟棄寵物封包
void Lssproto::lssproto_DP_send(const QPoint& pos, int petindex)
{
	autil_->util_Send(LSSPROTO_DP_SEND, pos.x(), pos.y(), petindex);
}

//平時使用精靈
void Lssproto::lssproto_MU_send(const QPoint& pos, int array, int toindex)
{
	autil_->util_Send(LSSPROTO_MU_SEND, pos.x(), pos.y(), array, toindex);
}

//人物動作
void Lssproto::lssproto_AC_send(const QPoint& pos, int actionno)
{
	autil_->util_Send(LSSPROTO_AC_SEND, pos.x(), pos.y(), actionno);
}

//下載地圖
void Lssproto::lssproto_M_send(int fl, int x1, int y1, int x2, int y2)
{
	autil_->util_Send(LSSPROTO_M_SEND, fl, x1, y1, x2, y2);
}

//地圖轉移封包
void Lssproto::lssproto_EV_send(int e, int dialogid, const QPoint& pos, int dir)
{
	autil_->util_Send(LSSPROTO_EV_SEND, e, dialogid, pos.x(), pos.y(), dir);
}

//組隊封包
void Lssproto::lssproto_PR_send(const QPoint& pos, int request)
{
	autil_->util_Send(LSSPROTO_PR_SEND, pos.x(), pos.y(), request);
}

//踢走隊員封包
void Lssproto::lssproto_KTEAM_send(int si)
{
	autil_->util_Send(LSSPROTO_KTEAM_SEND, si);
}

//退隊以後發的
void Lssproto::lssproto_SP_send(const QPoint& pos, int dir)
{
	autil_->util_Send(LSSPROTO_SP_SEND, pos.x(), pos.y(), dir);
}

void Lssproto::lssproto_MSG_send(int index, char* message, int color)
{
	autil_->util_Send(LSSPROTO_MSG_SEND, index, message, color);
}

//人物升級加點封包
void Lssproto::lssproto_SKUP_send(int skillid)
{
	autil_->util_Send(LSSPROTO_SKUP_SEND, skillid);
}

//丟棄石幣封包
void Lssproto::lssproto_DG_send(const QPoint& pos, int amount)
{
	autil_->util_Send(LSSPROTO_DG_SEND, pos.x(), pos.y(), amount);
}

//寵物郵件封包
void Lssproto::lssproto_PMSG_send(int index, int petindex, int itemIndex, char* message, int color)
{
	autil_->util_Send(LSSPROTO_PMSG_SEND, index, petindex, itemIndex, message, color);
}

//人物改名封包 (freeName)
void Lssproto::lssproto_FT_send(char* data)
{
	autil_->util_Send(LSSPROTO_FT_SEND, data);
}

//寵物改名封包 (freeName)
void Lssproto::lssproto_KN_send(int havepetindex, char* data)
{
	autil_->util_Send(LSSPROTO_KN_SEND, havepetindex, data);
}

//寵物穿脫封包
void Lssproto::lssproto_PetItemEquip_send(const QPoint& pos, int iPetNo, int iItemNo, int iDestNO)
{
	autil_->util_Send(LSSPROTO_PET_ITEM_EQUIP_SEND, pos.x(), pos.y(), iPetNo, iItemNo, iDestNO);
}

//發起交易請求封包 發起交易D|D   放置物品:T|87|02020202|I|1|23
void Lssproto::lssproto_TD_send(char* data)
{
	autil_->util_Send(LSSPROTO_TD_SEND, data);
}

//請求任務日誌封包  data = "dyedye"
void Lssproto::lssproto_JOBDAILY_send(char* data)
{
	autil_->util_Send(LSSPROTO_JOBDAILY_SEND, data);
}

//老菜單封包
void Lssproto::lssproto_ShopOk_send(int n)
{
	autil_->util_Send(LSSPROTO_SHOPOK_SEND, n);
}

//新菜單封包
#ifdef _NEW_SYSTEM_MENU
void Lssproto::lssproto_SaMenu_send(int index)
{
	autil_->util_Send(LSSPROTO_SAMENU_SEND, index);
}
#endif

//求救
void Lssproto::lssproto_HL_send(int flg)
{
	autil_->util_Send(LSSPROTO_HL_SEND, flg);
}

//戰鬥指令封包
void Lssproto::lssproto_B_send(const QString& command)
{
	std::string cmd = util::fromUnicode(command.toUpper());
	autil_->util_Send(LSSPROTO_B_SEND, const_cast<char*>(cmd.c_str()));
}

void Lssproto::lssproto_MA_send(const QPoint& pos, int nMind)
{
	autil_->util_Send(LSSPROTO_MA_SEND, pos.x(), pos.y(), nMind);
}

void Lssproto::lssproto_S2_send(char* data)
{
	autil_->util_Send(LSSPROTO_S2_SEND, data);
}

//創建人物
void Lssproto::lssproto_CreateNewChar_send(
	int dataplacenum,
	char* charname,
	int imgno,
	int faceimgno,
	int vital,
	int str,
	int tgh,
	int dex,
	int earth,
	int water,
	int fire,
	int wind,
	int hometown)
{
	autil_->util_Send(LSSPROTO_CREATENEWCHAR_SEND, dataplacenum, charname, imgno, faceimgno, vital, str, tgh, dex, earth, water, fire, wind, hometown);
}

//刪除人物
void Lssproto::lssproto_CharDelete_send(char* charname, char* securityCode)
{
	autil_->util_Send(LSSPROTO_CHARDELETE_SEND, charname, securityCode);
}

void Lssproto::lssproto_ClientLogin_send(char* cdkey, char* passwd, char* mac, int selectServerIndex, char* ip, DWORD flags)
{
	if ((flags & WITH_ALL) & WITH_ALL)
		autil_->util_Send(LSSPROTO_CLIENTLOGIN_SEND, cdkey, passwd, mac, selectServerIndex, ip);
	else if ((flags & (WITH_CDKEY | WITH_PASSWORD | WITH_MACADDRESS | WITH_SELECTSERVERINDEX)) == (WITH_CDKEY | WITH_PASSWORD | WITH_MACADDRESS | WITH_SELECTSERVERINDEX))
		autil_->util_Send(LSSPROTO_CLIENTLOGIN_SEND, cdkey, passwd, mac, selectServerIndex);
	else if ((flags & (WITH_CDKEY | WITH_PASSWORD | WITH_MACADDRESS)) == (WITH_CDKEY | WITH_PASSWORD | WITH_MACADDRESS))
		autil_->util_Send(LSSPROTO_CLIENTLOGIN_SEND, cdkey, passwd, mac);
	else if ((flags & (WITH_CDKEY | WITH_PASSWORD)) == (WITH_CDKEY | WITH_PASSWORD))
		autil_->util_Send(LSSPROTO_CLIENTLOGIN_SEND, cdkey, passwd);
}

void Lssproto::lssproto_CharLogin_send(char* charname)
{
	autil_->util_Send(LSSPROTO_CHARLOGIN_SEND, charname);
}