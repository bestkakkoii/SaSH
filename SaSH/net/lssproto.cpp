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

#include "stdafx.h"
#include "lssproto.h"
#include "autil.h"
#include <gamedevice.h>
#include "signaldispatcher.h"

Lssproto::Lssproto(Autil* autil)
	: autil_(autil)
{

}

// 0斷線1回點
bool Lssproto::lssproto_CharLogout_send(long long Flg)
{
	return autil_->util_Send(sa::LSSPROTO_CHARLOGOUT_SEND, Flg);
}

//往指定方向查看觸發對話的效果
bool Lssproto::lssproto_L_send(long long dir)
{
	return autil_->util_Send(sa::LSSPROTO_L_SEND, dir);
}

//遇敵
bool Lssproto::lssproto_EN_send(const QPoint& pos)
{
	return autil_->util_Send(sa::LSSPROTO_EN_SEND, pos.x(), pos.y());
}

//退出觀戰
bool Lssproto::lssproto_BU_send(long long dummy)
{
	return autil_->util_Send(sa::LSSPROTO_BU_SEND, dummy);
}

//開關封包
bool Lssproto::lssproto_FS_send(long long flg)
{
	return autil_->util_Send(sa::LSSPROTO_FS_SEND, flg);
}

//對話框封包 關於dialogid: 送買242 賣243
bool Lssproto::lssproto_WN_send(const QPoint& pos, long long dialogid, long long unitid, long long select, char* data)
{
	return autil_->util_Send(sa::LSSPROTO_WN_SEND, pos.x(), pos.y(), dialogid, unitid, select, data);
}

//設置寵物狀態封包  0:休息 1:戰鬥或等待 4:郵件
bool Lssproto::lssproto_PETST_send(long long nPet, long long sPet)
{
	return autil_->util_Send(sa::LSSPROTO_PETST_SEND, nPet, sPet);
}

//設置戰寵封包
bool Lssproto::lssproto_KS_send(long long petarray)
{
	return autil_->util_Send(sa::LSSPROTO_KS_SEND, petarray);
}

//設置寵物等待狀態
//1:寵物1處於等待狀態
//2:寵物2處於等待狀態
//4：寵物3處於等待狀態
//8：寵物4處於等待狀態
//16：寵物5處於等待狀態，這些值可相互組合，如5代表寵物1和寵物3處於等待狀態
bool Lssproto::lssproto_SPET_send(long long standbypet)
{
	return autil_->util_Send(sa::LSSPROTO_SPET_SEND, standbypet);
}

//移動轉向封包 (a-h方向移動 A-H轉向)
bool Lssproto::lssproto_W2_send(const QPoint& pos, char* direction)
{
	return autil_->util_Send(sa::LSSPROTO_W2_SEND, pos.x(), pos.y(), direction);
}

//發送喊話封包
bool Lssproto::lssproto_TK_send(const QPoint& pos, char* message, long long color, long long area)
{
	return autil_->util_Send(sa::LSSPROTO_TK_SEND, pos.x(), pos.y(), message, color, area);
}

//EO封包
bool Lssproto::lssproto_EO_send(long long dummy)
{
	return autil_->util_Send(sa::LSSPROTO_EO_SEND, dummy);
}

//ECHO封包
bool Lssproto::lssproto_Echo_send(char* test)
{
	return autil_->util_Send(sa::LSSPROTO_ECHO_SEND, test);
}

//丟棄道具封包
bool Lssproto::lssproto_DI_send(const QPoint& pos, long long itemIndex)
{
	return autil_->util_Send(sa::LSSPROTO_DI_SEND, pos.x(), pos.y(), itemIndex);
}

//使用道具封包
bool Lssproto::lssproto_ID_send(const QPoint& pos, long long haveitemindex, long long toindex)
{
	return autil_->util_Send(sa::LSSPROTO_ID_SEND, pos.x(), pos.y(), haveitemindex, toindex);
}

//交換道具封包
bool Lssproto::lssproto_MI_send(long long fromindex, long long toindex)
{
	return autil_->util_Send(sa::LSSPROTO_MI_SEND, fromindex, toindex);
}

//撿道具封包
bool Lssproto::lssproto_PI_send(const QPoint& pos, long long dir)
{
	return autil_->util_Send(sa::LSSPROTO_PI_SEND, pos.x(), pos.y(), dir);
}

//料理封包
bool Lssproto::lssproto_PS_send(long long havepetindex, long long havepetskill, long long toindex, char* data)
{
	return autil_->util_Send(sa::LSSPROTO_PS_SEND, havepetindex, havepetskill, toindex, data);
}

//存取家族個人銀行封包 家族個人"B|G|%d" 正數存 負數取  家族共同 "B|T|%d"
//設置騎乘 "R|P|寵物編號0-4"，"R|P|-1"取消騎乘, S|P取聲望資訊
bool Lssproto::lssproto_FM_send(char* data)
{
	return autil_->util_Send(sa::LSSPROTO_FM_SEND, data);
}

//丟棄寵物封包
bool Lssproto::lssproto_DP_send(const QPoint& pos, long long petindex)
{
	return autil_->util_Send(sa::LSSPROTO_DP_SEND, pos.x(), pos.y(), petindex);
}

//平時使用精靈
bool Lssproto::lssproto_MU_send(const QPoint& pos, long long array, long long toindex)
{
	return autil_->util_Send(sa::LSSPROTO_MU_SEND, pos.x(), pos.y(), array, toindex);
}

//人物動作
bool Lssproto::lssproto_AC_send(const QPoint& pos, long long actionno)
{
	return autil_->util_Send(sa::LSSPROTO_AC_SEND, pos.x(), pos.y(), actionno);
}

//下載地圖
bool Lssproto::lssproto_M_send(long long fl, long long x1, long long y1, long long x2, long long y2)
{
	return autil_->util_Send(sa::LSSPROTO_M_SEND, fl, x1, y1, x2, y2);
}

//地圖轉移封包
bool Lssproto::lssproto_EV_send(long long e, long long dialogid, const QPoint& pos, long long dir)
{
	return autil_->util_Send(sa::LSSPROTO_EV_SEND, e, dialogid, pos.x(), pos.y(), dir);
}

//組隊封包
bool Lssproto::lssproto_PR_send(const QPoint& pos, long long request)
{
	return autil_->util_Send(sa::LSSPROTO_PR_SEND, pos.x(), pos.y(), request);
}

//踢走隊員封包
bool Lssproto::lssproto_KTEAM_send(long long si)
{
	return autil_->util_Send(sa::LSSPROTO_KTEAM_SEND, si);
}

//退隊以後發的
bool Lssproto::lssproto_SP_send(const QPoint& pos, long long dir)
{
	return autil_->util_Send(sa::LSSPROTO_SP_SEND, pos.x(), pos.y(), dir);
}

bool Lssproto::lssproto_MSG_send(long long index, char* message, long long color)
{
	return autil_->util_Send(sa::LSSPROTO_MSG_SEND, index, message, color);
}

//人物升級加點封包
bool Lssproto::lssproto_SKUP_send(long long skillid)
{
	return autil_->util_Send(sa::LSSPROTO_SKUP_SEND, skillid);
}

//丟棄石幣封包
bool Lssproto::lssproto_DG_send(const QPoint& pos, long long amount)
{
	return autil_->util_Send(sa::LSSPROTO_DG_SEND, pos.x(), pos.y(), amount);
}

//寵物郵件封包
bool Lssproto::lssproto_PMSG_send(long long index, long long petindex, long long itemIndex, char* message, long long color)
{
	return autil_->util_Send(sa::LSSPROTO_PMSG_SEND, index, petindex, itemIndex, message, color);
}

//人物改名封包 (freeName)
bool Lssproto::lssproto_FT_send(char* data)
{
	return autil_->util_Send(sa::LSSPROTO_FT_SEND, data);
}

//寵物改名封包 (freeName)
bool Lssproto::lssproto_KN_send(long long havepetindex, char* data)
{
	return autil_->util_Send(sa::LSSPROTO_KN_SEND, havepetindex, data);
}

//寵物穿脫封包
bool Lssproto::lssproto_PetItemEquip_send(const QPoint& pos, long long iPetNo, long long iItemNo, long long iDestNO)
{
	return autil_->util_Send(sa::LSSPROTO_PET_ITEM_EQUIP_SEND, pos.x(), pos.y(), iPetNo, iItemNo, iDestNO);
}

//發起交易請求封包 發起交易D|D   放置物品:T|87|02020202|I|1|23
bool Lssproto::lssproto_TD_send(char* data)
{
	return autil_->util_Send(sa::LSSPROTO_TD_SEND, data);
}

//請求任務日誌封包  data = "dyedye"
bool Lssproto::lssproto_missionInfo_send(char* data)
{
	return autil_->util_Send(sa::LSSPROTO_missionInfo_SEND, data);
}

//老菜單封包
bool Lssproto::lssproto_ShopOk_send(long long n)
{
	return autil_->util_Send(sa::LSSPROTO_SHOPOK_SEND, n);
}

//新菜單封包
bool Lssproto::lssproto_SaMenu_send(long long index)
{
	return autil_->util_Send(sa::LSSPROTO_SAMENU_SEND, index);
}

//求救
bool Lssproto::lssproto_HL_send(long long flg)
{
	return autil_->util_Send(sa::LSSPROTO_HL_SEND, flg);
}

//戰鬥指令封包
bool Lssproto::lssproto_B_send(const QString& command)
{
	std::string cmd = util::fromUnicode(command.toUpper());
	return autil_->util_Send(sa::LSSPROTO_B_SEND, const_cast<char*>(cmd.c_str()));
}

bool Lssproto::lssproto_MA_send(const QPoint& pos, long long nMind)
{
	return autil_->util_Send(sa::LSSPROTO_MA_SEND, pos.x(), pos.y(), nMind);
}

bool Lssproto::lssproto_S2_send(char* data)
{
	return autil_->util_Send(sa::LSSPROTO_S2_SEND, data);
}

//創建人物
bool Lssproto::lssproto_CreateNewChar_send(
	long long dataplacenum,
	char* charname,
	long long imgno,
	long long faceimgno,
	long long vital,
	long long str,
	long long tgh,
	long long dex,
	long long earth,
	long long water,
	long long fire,
	long long wind,
	long long hometown)
{
	return autil_->util_Send(sa::LSSPROTO_CREATENEWCHAR_SEND, dataplacenum, charname, imgno, faceimgno, vital, str, tgh, dex, earth, water, fire, wind, hometown);
}

//刪除人物
bool Lssproto::lssproto_CharDelete_send(char* charname, char* securityCode)
{
	return autil_->util_Send(sa::LSSPROTO_CHARDELETE_SEND, charname, securityCode);
}

bool Lssproto::lssproto_ClientLogin_send(char* cdkey, char* passwd, char* mac, long long selectServerIndex, char* ip, DWORD flags)
{
	if ((flags & sa::WITH_ALL) & sa::WITH_ALL)
		return autil_->util_Send(sa::LSSPROTO_CLIENTLOGIN_SEND, cdkey, passwd, mac, selectServerIndex, ip);
	else if ((flags & (sa::WITH_CDKEY | sa::WITH_PASSWORD | sa::WITH_MACADDRESS | sa::WITH_SELECTSERVERINDEX))
		== (sa::WITH_CDKEY | sa::WITH_PASSWORD | sa::WITH_MACADDRESS | sa::WITH_SELECTSERVERINDEX))
		return autil_->util_Send(sa::LSSPROTO_CLIENTLOGIN_SEND, cdkey, passwd, mac, selectServerIndex);
	else if ((flags & (sa::WITH_CDKEY | sa::WITH_PASSWORD | sa::WITH_MACADDRESS))
		== (sa::WITH_CDKEY | sa::WITH_PASSWORD | sa::WITH_MACADDRESS))
		return autil_->util_Send(sa::LSSPROTO_CLIENTLOGIN_SEND, cdkey, passwd, mac);
	else if ((flags & (sa::WITH_CDKEY | sa::WITH_PASSWORD)) == (sa::WITH_CDKEY | sa::WITH_PASSWORD))
		return autil_->util_Send(sa::LSSPROTO_CLIENTLOGIN_SEND, cdkey, passwd);

	return false;
}

bool Lssproto::lssproto_CharLogin_send(char* charname)
{
	return autil_->util_Send(sa::LSSPROTO_CHARLOGIN_SEND, charname);
}