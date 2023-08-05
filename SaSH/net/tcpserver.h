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
#include <QCoreApplication>
#include <QTimer>
#include <iostream>
#include <string>
#include <vector>
#include <WS2tcpip.h>

#include <threadplugin.h>
#include <util.h>
#include "lssproto.h"

static const QHash<QString, BUTTON_TYPE> buttonMap = {
	{"OK", BUTTON_OK},
	{"CANCEL", BUTTON_CANCEL},
	{"YES", BUTTON_YES},
	{"NO", BUTTON_NO},
	{"PREVIOUS", BUTTON_PREVIOUS},
	{"NEXT", BUTTON_NEXT},
	{"AUTO", BUTTON_AUTO},

	//big5
	{u8"確認", BUTTON_YES},
	{u8"確定", BUTTON_YES},
	{u8"取消", BUTTON_NO},
	{u8"好", BUTTON_YES},
	{u8"不好", BUTTON_NO},
	{u8"可以", BUTTON_YES},
	{u8"不可以", BUTTON_NO},
	{u8"上一頁", BUTTON_PREVIOUS},
	{u8"上一步", BUTTON_PREVIOUS},
	{u8"下一頁", BUTTON_NEXT},
	{u8"下一步", BUTTON_NEXT},
	{u8"買", BUTTON_BUY},
	{u8"賣", BUTTON_SELL},
	{u8"出去", BUTTON_OUT},
	{u8"回上一頁", BUTTON_BACK},

	//gb2312
	{u8"确认", BUTTON_YES},
	{u8"确定", BUTTON_YES},
	{u8"取消", BUTTON_NO},
	{u8"好", BUTTON_YES},
	{u8"不好", BUTTON_NO},
	{u8"可以", BUTTON_YES},
	{u8"不可以", BUTTON_NO},
	{u8"上一页", BUTTON_PREVIOUS},
	{u8"上一步", BUTTON_PREVIOUS},
	{u8"下一页", BUTTON_NEXT},
	{u8"下一步", BUTTON_NEXT},
	{u8"买", BUTTON_BUY},
	{u8"卖", BUTTON_SELL},
	{u8"出去", BUTTON_OUT},
	{u8"回上一页", BUTTON_BACK},
};

static const QHash<QString, PetState> petStateMap = {
	{ u8"戰鬥", kBattle },
	{ u8"等待", kStandby },
	{ u8"郵件", kMail },
	{ u8"休息", kRest },
	{ u8"騎乘", kRide },

	{ u8"战斗", kBattle },
	{ u8"等待", kStandby },
	{ u8"邮件", kMail },
	{ u8"休息", kRest },
	{ u8"骑乘", kRide },

	{ u8"battle", kBattle },
	{ u8"standby", kStandby },
	{ u8"mail", kMail },
	{ u8"rest", kRest },
	{ u8"ride", kRide },
};

static const QHash<QString, DirType> dirMap = {
	{ u8"北", kDirNorth },
	{ u8"東北", kDirNorthEast },
	{ u8"東", kDirEast },
	{ u8"東南", kDirSouthEast },
	{ u8"南", kDirSouth },
	{ u8"西南", kDirSouthWest },
	{ u8"西", kDirWest },
	{ u8"西北", kDirNorthWest },

	{ u8"北", kDirNorth },
	{ u8"东北", kDirNorthEast },
	{ u8"东", kDirEast },
	{ u8"东南", kDirSouthEast },
	{ u8"南", kDirSouth },
	{ u8"西南", kDirSouthWest },
	{ u8"西", kDirWest },
	{ u8"西北", kDirNorthWest },

	{ u8"A", kDirNorth },
	{ u8"B", kDirNorthEast },
	{ u8"C", kDirEast },
	{ u8"D", kDirSouthEast },
	{ u8"E", kDirSouth },
	{ u8"F", kDirSouthWest },
	{ u8"G", kDirWest },
	{ u8"H", kDirNorthWest },
};

static const QHash<QString, CHAR_EquipPlace> equipMap = {
	{ u8"頭", CHAR_HEAD },
	{ u8"身體", CHAR_BODY },
	{ u8"右手", CHAR_ARM },
	{ u8"左飾", CHAR_DECORATION1 },
	{ u8"右飾", CHAR_DECORATION2 },
	{ u8"腰帶", CHAR_EQBELT },
	{ u8"左手", CHAR_EQSHIELD },
	{ u8"鞋子", CHAR_EQSHOES },
	{ u8"手套", CHAR_EQGLOVE },

	{ u8"头", CHAR_HEAD },
	{ u8"身体", CHAR_BODY },
	{ u8"右手", CHAR_ARM },
	{ u8"左饰", CHAR_DECORATION1 },
	{ u8"右饰", CHAR_DECORATION2 },
	{ u8"腰带", CHAR_EQBELT },
	{ u8"左手", CHAR_EQSHIELD },
	{ u8"鞋子", CHAR_EQSHOES },
	{ u8"手套", CHAR_EQGLOVE },

	{ u8"head", CHAR_HEAD },
	{ u8"body", CHAR_BODY },
	{ u8"right", CHAR_ARM },
	{ u8"las", CHAR_DECORATION1 },
	{ u8"ras", CHAR_DECORATION2 },
	{ u8"belt", CHAR_EQBELT },
	{ u8"left", CHAR_EQSHIELD },
	{ u8"shoe", CHAR_EQSHOES },
	{ u8"glove", CHAR_EQGLOVE },
};

static const QHash<QString, CHAR_EquipPlace> petEquipMap = {
	{ u8"頭", PET_HEAD },
	{ u8"翼", PET_WING },
	{ u8"牙", PET_TOOTH },
	{ u8"身體", PET_PLATE },
	{ u8"背", PET_BACK },
	{ u8"爪", PET_CLAW },
	{ u8"腳", PET_FOOT },

	{ u8"头", PET_HEAD },
	{ u8"翼", PET_WING },
	{ u8"牙", PET_TOOTH },
	{ u8"身体", PET_PLATE },
	{ u8"背", PET_BACK },
	{ u8"爪", PET_CLAW },
	{ u8"脚", PET_FOOT },

	{ u8"head", PET_HEAD },
	{ u8"wing", PET_WING },
	{ u8"tooth", PET_TOOTH },
	{ u8"body", PET_PLATE },
	{ u8"back", PET_BACK },
	{ u8"claw", PET_CLAW },
	{ u8"foot", PET_FOOT },
};

enum BufferControl
{
	BC_NONE,
	BC_NEED_TO_CLEAN,
	BC_HAS_NEXT,
	BC_ABOUT_TO_END,
};

class MapAnalyzer;
class Server : public ThreadPlugin, public Lssproto
{
	Q_OBJECT
public:
	explicit Server(QObject* parent);

	virtual ~Server();

	Q_REQUIRED_RESULT inline bool isListening() const { return  !server_.isNull() && server_->isListening(); }

	Q_REQUIRED_RESULT inline bool hasClientExist() const { return  !server_.isNull() && !clientSockets_.isEmpty(); }

	bool start(QObject* parent);

	Q_REQUIRED_RESULT inline unsigned short getPort() const { return port_; }

signals:
	void write(QTcpSocket* clientSocket, QByteArray ba, int size);

private slots:
	void onWrite(QTcpSocket* clientSocket, QByteArray ba, int size);
	void onNewConnection();
	void onClientReadyRead();


private:
	int saDispatchMessage(char* encoded);

	void handleData(QTcpSocket* clientSocket, QByteArray data);

public://actions
	Q_REQUIRED_RESULT int getWorldStatus();

	Q_REQUIRED_RESULT int getGameStatus();

	Q_REQUIRED_RESULT bool checkGW(int w, int g);

	Q_REQUIRED_RESULT int getUnloginStatus();
	void setWorldStatus(int w);
	void setGameStatus(int g);

	bool login(int s);

	QString getBadStatusString(unsigned int status);

	QString getFieldString(unsigned int field);

	void unlockSecurityCode(const QString& code);

	void clearNetBuffer();

	void logOut();

	void logBack();

	void move(const QPoint& p, const QString& dir);

	void move(const QPoint& p);

	void announce(const QString& msg, int color = 4);


	void talk(const QString& text, int color = 0, TalkMode mode = kTalkNormal);
	void inputtext(const QString& text, int dialogid = -1, int npcid = -1);

	void windowPacket(const QString& command, int dialogid, int npcid);

	void EO();

	void dropItem(int index);
	void dropItem(QVector<int> index);

	void useItem(int itemIndex, int target);


	void swapItem(int from, int to);

	void petitemswap(int petIndex, int from, int to);

	void useMagic(int magicIndex, int target);

	void dropPet(int petIndex);

	void setSwitcher(int flg, bool enable);

	void setSwitcher(int flg);

	void press(BUTTON_TYPE select, int seqno = -1, int objindex = -1);
	void press(int row, int seqno = -1, int objindex = -1);

	void buy(int index, int amt, int seqno = -1, int objindex = -1);
	void sell(const QVector<int>& indexs, int seqno = -1, int objindex = -1);
	void sell(int index, int seqno = -1, int objindex = -1);
	void sell(const QString& name, const QString& memo = "", int seqno = -1, int objindex = -1);
	void learn(int skillIndex, int petIndex, int spot, int seqno = -1, int objindex = -1);

	void craft(util::CraftType type, const QStringList& ingres);

	void createRemoteDialog(int button, const QString& text);

	void mail(int index, const QString& text);

	void warp();

	void shopOk(int n);
	void saMenu(int n);

	bool addPoint(int skillid, int amt);

	void pickItem(int dir);

	void depositGold(int gold, bool isPublic);
	void withdrawGold(int gold, bool isPublic);

	void depositPet(int petIndex, int seqno = -1, int objindex = -1);
	void withdrawPet(int petIndex, int seqno = -1, int objindex = -1);

	void depositItem(int index, int seqno = -1, int objindex = -1);
	void withdrawItem(int itemIndex, int seqno = -1, int objindex = -1);

	bool captchaOCR(QString* pmsg);

	void setAllPetState();
	void setPetState(int petIndex, PetState state);
	void setFightPet(int petIndex);
	void setRidePet(int petIndex);
	void setStandbyPet(int standbypets);
	void setPetState(int petIndex, int state);

	void updateDatasFromMemory();

	void asyncBattleWork(bool wait);
	void asyncBattleAction();

	void downloadMap();
	void downloadMap(int x, int y);

	bool tradeStart(const QString& name, int timeout);
	void tradeComfirm(const QString name);
	void tradeCancel();
	void tradeAppendItems(const QString& name, const QVector<int>& itemIndexs);
	void tradeAppendGold(const QString& name, int gold);
	void tradeAppendPets(const QString& name, const QVector<int>& petIndex);
	void tradeComplete(const QString& name);

	void cleanChatHistory();
	QString getChatHistory(int index);

	bool findUnit(const QString& name, int type, mapunit_t* unit, const QString freename = "", int modelid = -1);

	void setTeamState(bool join);
	void kickteam(int n);

	void setPlayerFaceToPoint(const QPoint& pos);
	void setPlayerFaceDirection(int dir);
	void setPlayerFaceDirection(const QString& dirStr);

	int getPartySize() const;
	QStringList getJoinableUnitList() const;
	bool getItemIndexsByName(const QString& name, const QString& memo, QVector<int>* pv);
	int getItemIndexByName(const QString& name, bool isExact = true, const QString& memo = "") const;
	int getPetSkillIndexByName(int& petIndex, const QString& name) const;
	bool getPetIndexsByName(const QString& name, QVector<int>* pv) const;
	int getMagicIndexByName(const QString& name, bool isExact = true) const;
	int getItemEmptySpotIndex() const;
	bool getItemEmptySpotIndexs(QVector<int>* pv) const;
	void clear();

	bool checkPlayerMp(int cmpvalue, int* target = nullptr, bool useequal = false);
	bool checkPlayerHp(int cmpvalue, int* target = nullptr, bool useequal = false);
	bool checkPetHp(int cmpvalue, int* target = nullptr, bool useequal = false);
	bool checkPartyHp(int cmpvalue, int* target);

	bool isPetSpotEmpty() const;
	int checkJobDailyState(const QString& missionName);

	void setPlayerFreeName(const QString& name);
	void setPetFreeName(int petIndex, const QString& name);

	Q_REQUIRED_RESULT inline bool getBattleFlag() const
	{
		QReadLocker lock(&battleStateLocker);
		if (isInterruptionRequested())
			return false;
		return IS_BATTLE_FLAG.load(std::memory_order_acquire);
	}
	Q_REQUIRED_RESULT inline bool getOnlineFlag() const
	{
		QReadLocker lock(&onlineStateLocker);
		if (isInterruptionRequested())
			return false;
		return IS_ONLINE_FLAG.load(std::memory_order_acquire);
	}

	PC getPC() const { QMutexLocker lock(&pcMutex_); return pc_; }
	void setPC(PC pc) { QMutexLocker lock(&pcMutex_); pc_ = pc; }

	void sortItem();

	QPoint getPoint();
	void setPoint(const QPoint& pos);

	//battle
	void sendBattlePlayerAttackAct(int target);
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
	void setBattleEnd();

	void reloadHashVar(const QString& typeStr);

	MAGIC getMagic(int magicIndex) const { return magic[magicIndex]; }
	PROFESSION_SKILL getSkill(int skillIndex) const { return profession_skill[skillIndex]; }
	PET getPet(int petIndex) const { return pet[petIndex]; }
	int getPetSize() const
	{
		int n = 0;
		for (const PET& it : pet)
		{
			if (it.level > 0 && it.useFlag == 1 && it.maxHp > 0)
				++n;
		}
		return n;
	}
	PET_SKILL getPetSkill(int petIndex, int skillIndex) const { return petSkill[petIndex][skillIndex]; }
	PARTY getParty(int partyIndex) const { return party[partyIndex]; }
	ITEM getPetEquip(int petIndex, int equipIndex) const { return pet[petIndex].item[equipIndex]; }

	Q_REQUIRED_RESULT int findInjuriedAllie();
	void refreshItemInfo();
private:
	void setWindowTitle();
	void refreshItemInfo(int index);


	void setBattleFlag(bool enable);
	inline void setOnlineFlag(bool enable)
	{
		QWriteLocker lock(&onlineStateLocker);
		IS_ONLINE_FLAG.store(enable, std::memory_order_release);
		if (!enable)
		{
			setBattleEnd();
		}
	}

	void getPlayerMaxCarryingCapacity();
	inline Q_REQUIRED_RESULT constexpr bool isItemStackable(int flg) { return ((flg >> 2) & 1); }
	QString getAreaString(int target);
	Q_REQUIRED_RESULT bool matchPetNameByIndex(int index, const QString& name);


#pragma region BattleFunctions
	int playerDoBattleWork();
	void handlePlayerBattleLogics();
	int petDoBattleWork();
	void handlePetBattleLogics();


	bool isPlayerMpEnoughForMagic(int magicIndex) const;
	bool isPlayerMpEnoughForSkill(int magicIndex) const;
	bool isPlayerHpEnoughForSkill(int magicIndex) const;

	void sortBattleUnit(QVector<battleobject_t>& v) const;

	Q_REQUIRED_RESULT int getBattleSelectableEnemyTarget(const battledata_t& bt) const;

	Q_REQUIRED_RESULT int getBattleSelectableEnemyOneRowTarget(const battledata_t& bt, bool front) const;

	Q_REQUIRED_RESULT int getBattleSelectableAllieTarget(const battledata_t& bt) const;

	Q_REQUIRED_RESULT bool matchBattleEnemyByName(const QString& name, bool isExact, QVector<battleobject_t> src, QVector<battleobject_t>* v) const;
	Q_REQUIRED_RESULT bool matchBattleEnemyByLevel(int level, QVector<battleobject_t> src, QVector<battleobject_t>* v) const;
	Q_REQUIRED_RESULT bool matchBattleEnemyByMaxHp(int maxHp, QVector<battleobject_t> src, QVector<battleobject_t>* v) const;

	Q_REQUIRED_RESULT int getGetPetSkillIndexByName(int petIndex, const QString& name) const;

	bool fixPlayerTargetByMagicIndex(int magicIndex, int oldtarget, int* target) const;
	bool fixPlayerTargetBySkillIndex(int magicIndex, int oldtarget, int* target) const;
	bool fixPlayerTargetByItemIndex(int itemIndex, int oldtarget, int* target) const;
	bool fixPetTargetBySkillIndex(int skillIndex, int oldtarget, int* target) const;
	void updateCurrentSideRange(battledata_t& bt);
	inline bool checkFlagState(int pos);

	battledata_t getBattleData() const
	{
		QReadLocker locker(&battleDataLocker);
		return battleData;
	}

	void setBattleData(const battledata_t& data)
	{
		QWriteLocker locker(&battleDataLocker);
		battleData = data;
	}

#pragma endregion

#pragma region SAClientOriginal
	//StoneAge Client Original Functions

	void swapItemLocal(int from, int to);

	void updateMapArea(void)
	{
		QPoint pos = getPoint();
		mapAreaX1 = pos.x() + MAP_TILE_GRID_X1;
		mapAreaY1 = pos.y() + MAP_TILE_GRID_Y1;
		mapAreaX2 = pos.x() + MAP_TILE_GRID_X2;
		mapAreaY2 = pos.y() + MAP_TILE_GRID_Y2;

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

	void resetMap(void)
	{
		//nowGx = (int)(nowX / GRID_SIZE);
		//nowGy = (int)(nowY / GRID_SIZE);
		QPoint pos(nowX / GRID_SIZE, nowY / GRID_SIZE);
		nowPoint = pos;
		nextGx = pos.x();
		nextGy = pos.y();
		nowX = (float)pos.x() * GRID_SIZE;
		nowY = (float)pos.y() * GRID_SIZE;
		oldGx = -1;
		oldGy = -1;
		oldNextGx = -1;
		oldNextGy = -1;
		mapAreaX1 = pos.x() + MAP_TILE_GRID_X1;
		mapAreaY1 = pos.y() + MAP_TILE_GRID_Y1;
		mapAreaX2 = pos.x() + MAP_TILE_GRID_X2;
		mapAreaY2 = pos.y() + MAP_TILE_GRID_Y2;

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

		mouseLeftPushTime = 0;
		beforeMouseLeftPushTime = 0;
		//	autoMapSeeFlag = FALSE;
	}

	void realTimeToSATime(LSTIME* lstime);

	inline void redrawMap(void) { oldGx = -1; oldGy = -1; }

	inline void setPcWarpPoint(const QPoint& pos) { setWarpMap(pos); }

	inline void resetPc(void) { QMutexLocker lock(&pcMutex_); pc_.status &= (~CHR_STATUS_LEADER); }

	inline void setMap(int floor, const QPoint& pos) { nowFloor = floor; setWarpMap(pos); }

	inline unsigned long long TimeGetTime(void)
	{
#ifdef _TIME_GET_TIME
		static __int64 time;
		QueryPerformanceCounter(&CurrentTick);
		return (unsigned int)(time = CurrentTick.QuadPart / tickCount.QuadPart);
		//return GetTickCount();
#else
		return QDateTime::currentMSecsSinceEpoch();
#endif
	}

	inline void setWarpMap(const QPoint& pos)
	{
		//nowGx = gx;
		//nowGy = gy;
		nowPoint = pos;
		nowX = (float)pos.x() * GRID_SIZE;
		nowY = (float)pos.y() * GRID_SIZE;
		nextGx = pos.x();
		nextGy = pos.y();
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
#ifdef _ALLDOMAN // (不可開) Syu ADD 排行榜NPC
	void setPcParam(const QString& name
		, const QString& freeName
		, int level, const QString& petname
		, int petlevel, int nameColor
		, int walk, int height
		, int profession_class
		, int profession_level
		, int profession_skill_point
		, int herofloor);
#else
	void setPcParam(const QString& name
		, const QString& freeName
		, int level, const QString& petname
		, int petlevel, int nameColor
		, int walk, int height
		, int profession_class
		, int profession_level
		, int profession_skill_point);
#endif
	// 	#endif
#else
	void setPcParam(const QString& name, const QString& freeName, int level, const QString& petname, int petlevel, int nameColor, int walk, int height);
#endif

#pragma endregion

private:
	QFutureSynchronizer<void> ayncBattleCommandSync;
	QFuture<void> ayncBattleCommandFuture;
	QMutex ayncBattleCommandMutex;
	std::atomic_bool ayncBattleCommandFlag = false;

	std::atomic_bool IS_BATTLE_FLAG = false;
	std::atomic_bool IS_ONLINE_FLAG = false;

	std::atomic_bool isEnemyAllReady = false;

	QElapsedTimer eottlTimer;
	QElapsedTimer connectingTimer;
	bool petEscapeEnableTempFlag = false;
	int tempCatchPetTargetIndex = -1;
	int JobdailyGetMax = 0;  //是否有接收到資料
	mutable QReadWriteLock battleStateLocker;
	mutable QReadWriteLock onlineStateLocker;
	mutable QReadWriteLock worldStateLocker;
	mutable QReadWriteLock gameStateLocker;

	battledata_t battleData;
	mutable QReadWriteLock battleDataLocker;

	bool IS_WAITFOT_SKUP_RECV = false;
	QFuture<void> skupFuture;

	bool IS_LOCKATTACK_ESCAPE_DISABLE = false;

	QReadWriteLock pointMutex_;//用於保護人物座標更新順序
	QMutex swapItemMutex_;//用於保護物品數據更新順序
	mutable QMutex pcMutex_;//用於保護人物數據更新順序
	PC pc_ = {};
	ITEM pcitem[MAX_ITEM] = {};

	PET pet[MAX_PET] = {};

#ifdef MAX_AIRPLANENUM
	PARTY party_[MAX_AIRPLANENUM];
#else
	PARTY party[MAX_PARTY] = {};
#endif

#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
	PROFESSION_SKILL profession_skill[MAX_PROFESSION_SKILL];
#endif

	PET_SKILL petSkill[MAX_PET][MAX_SKILL] = {};
	//client original
#pragma region ClientOriginal
	int  talkMode = 0;						//0:一般 1:密語 2: 隊伍 3:家族 4:職業

	MAGIC magic[MAX_MAGIC] = {};

	BATTLE_RESULT_MSG battleResultMsg = {};

	ADDRESS_BOOK addressBook[MAX_ADR_BOOK] = {};

	JOBDAILY jobdaily[MAXMISSION] = {};

	CHARLISTTABLE chartable[MAXCHARACTER] = {};

	short partyModeFlag = 0;
	MAIL_HISTORY MailHistory[MAX_ADR_BOOK] = {};
	unsigned int ProcNo = 0u;
	unsigned int SubProcNo = 0u;
	unsigned int MenuToggleFlag = 0u;

	int opp_showindex = 0;
	QString opp_sockfd;
	QString opp_name;
	QString opp_goldmount;
	int showindex[7] = { 0 };
	int tradeWndDropGoldGet = 0;
	QString opp_itemgraph;
	QString  opp_itemname;
	QString opp_itemeffect;
	QString opp_itemindex;
	QString opp_itemdamage;
	QString trade_kind;
	QString trade_command;
	int tradeStatus = 0;
	int tradePetIndex = -1;
	PET tradePet[2] = {};
	showitem opp_item[MAX_MAXHAVEITEM];	//交易道具阵列增为15个
	showpet opp_pet[MAX_PET];
	QStringList myitem_tradeList;
	QStringList mypet_tradeList = { "P|-1", "P|-1", "P|-1" , "P|-1", "P|-1" };
	int mygoldtrade = 0;

	short prSendFlag = 0i16;


	short mailLampDrawFlag = 0i16;

	short mapEffectRainLevel = 0i16;
	short mapEffectSnowLevel = 0i16;
	short mapEffectKamiFubukiLevel = 0i16;



	LSTIME SaTime = { 0 };
	unsigned long long serverTime = 0ULL;
	unsigned long long FirstTime = 0ULL;
	int SaTimeZoneNo = 0;

	short charDelStatus = 0;

	bool floorChangeFlag = false;
	bool warpEffectStart = false;
	bool warpEffectOk = false;
	short eventWarpSendId = 0i16;
	short eventWarpSendFlag = 0i16;
	short eventEnemySendId = 0i16;
	short eventEnemySendFlag = 0i16;

	unsigned short event_[MAP_X_SIZE * MAP_Y_SIZE];	// ????

	short helpFlag = 0i16;

	int BattleMyNo = 0;
	int BattleBpFlag = 0;
	int BattleEscFlag = 0;
	int BattleMyMp = 0;
	int BattleAnimFlag = 0;
	int BattlePetStMenCnt = 0;

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

	int nowFloorCache = 0;
	bool mapEmptyFlag = false;
	int nowFloorGxSize = 0, nowFloorGySize = 0;
	int oldGx = -1, oldGy = -1;
	int mapAreaX1 = 0, mapAreaY1 = 0, mapAreaX2 = 0, mapAreaY2 = 0;
	//int nowGx = 0, nowGy = 0;

	int mapAreaWidth = 0, mapAreaHeight = 0;
	float nowX = (float)0 * GRID_SIZE;
	float nowY = (float)0 * GRID_SIZE;
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

	bool TimeZonePalChangeFlag = false;
	short drawTimeAnimeFlag = 0;

	bool NoCastFlag = false;
	bool StandbyPetSendFlag = false;
	bool warpEffectFlag = false;

	short newCharStatus = 0i16;
	short charListStatus = 0i16;

	short sTeacherSystemBtn = 0i16;
#pragma endregion

public:
	//custom
	bool disconnectflag = false;

	bool IS_TRADING = false;

	bool IS_TCP_CONNECTION_OK_TO_USE = false;

	bool IS_WAITFOR_JOBDAILY_FLAG = false;
	bool IS_WAITFOR_BANK_FLAG = false;
	bool IS_WAITFOR_DIALOG_FLAG = false;
	bool IS_WAITFOR_EXTRA_DIALOG_INFO_FLAG = false;

	bool IS_WAITFOR_CUSTOM_DIALOG_FLAG = false;

	std::atomic_bool  isBattleDialogReady = false;
	std::atomic_bool isEOTTLSend = false;
	std::atomic_int lastEOTime = 0;

	QElapsedTimer loginTimer;
	QElapsedTimer battleDurationTimer;
	QElapsedTimer normalDurationTimer;

	int BattleCliTurnNo = 0;
	int battle_total_time = 0;
	int battle_totol = 0;

	int nowFloor = 0;
	util::SafeData<QPoint> nowPoint;
	QString nowFloorName = "";

	QString protoBattleLogName = "";

	//main datas shared with script thread
	util::SafeHash<QString, QVariant> hashmap;
	util::SafeHash<int, QHash<QString, QVariant>> hashbattle;
	util::SafeData<QString> hashbattlefield;

	QSharedPointer<MapAnalyzer> mapAnalyzer;

	util::SafeData<currencydata_t> currencyData = {};
	util::SafeData<customdialog_t> customDialog = {};

	util::SafeHash<int, mapunit_t> mapUnitHash;
	util::SafeHash<QPoint, mapunit_t> npcUnitPointHash;
	util::SafeQueue<QPair<int, QString>> chatQueue;

	QPair<int, QVector<bankpet_t>> currentBankPetList;
	util::SafeVector<ITEM> currentBankItemList;

	QElapsedTimer repTimer;
	util::AfkRecorder recorder[1 + MAX_PET] = {};


	dialog_t currentDialog = {};

	//用於緩存要發送到UI的數據(開啟子窗口初始化並加載當前最新數據時使用)
	util::SafeHash<int, QVariant> playerInfoColContents;
	util::SafeHash<int, QVariant> itemInfoRowContents;
	util::SafeHash<int, QVariant> equipInfoRowContents;
	QStringList enemyNameListCache;
	QVariant topInfoContents;
	QVariant bottomInfoContents;
	QString timeLabelContents;
	QString labelPlayerAction;
	QString labelPetAction;

private:
	QFutureSynchronizer <void> sync_;

	unsigned short port_ = 0;

	QSharedPointer<QTcpServer> server_;

	QList<QTcpSocket*> clientSockets_;

	QByteArray net_readbuf;

	QMutex net_mutex;

private://lssproto
	int appendReadBuf(const QByteArray& data);
	QByteArrayList splitLinesFromReadBuf();
	int a62toi(const QString& a) const;
	int getStringToken(const QString& src, const QString& delim, int count, QString& out) const;
	int getIntegerToken(const QString& src, const QString& delim, int count) const;
	int getInteger62Token(const QString& src, const QString& delim, int count) const;
	QString makeStringFromEscaped(const QString& src) const;

private://lssproto_recv
#pragma region Lssproto_Recv
	virtual void lssproto_XYD_recv(const QPoint& pos, int dir) override;//戰鬥結束後的大地圖座標
	virtual void lssproto_EV_recv(int seqno, int result) override;
	virtual void lssproto_EN_recv(int result, int field) override;
	virtual void lssproto_RS_recv(char* data) override;
	virtual void lssproto_RD_recv(char* data) override;
	virtual void lssproto_B_recv(char* command) override;
	virtual void lssproto_I_recv(char* data) override;
	virtual void lssproto_SI_recv(int fromindex, int toindex) override;
	virtual void lssproto_MSG_recv(int aindex, char* text, int color) override;	//收到普通郵件或寵物郵件
	virtual void lssproto_PME_recv(int objindex, int graphicsno, const QPoint& pos, int dir, int flg, int no, char* cdata) override;
	virtual void lssproto_AB_recv(char* data) override;
	virtual void lssproto_ABI_recv(int num, char* data) override;
	virtual void lssproto_TK_recv(int index, char* message, int color) override;
	virtual void lssproto_MC_recv(int fl, int x1, int y1, int x2, int y2, int tilesum, int objsum, int eventsum, char* data) override;
	virtual void lssproto_M_recv(int fl, int x1, int y1, int x2, int y2, char* data) override;
	virtual void lssproto_C_recv(char* data) override;
	virtual void lssproto_CA_recv(char* data) override;
	virtual void lssproto_CD_recv(char* data) override;
	virtual void lssproto_R_recv(char* data) override;
	virtual void lssproto_S_recv(char* data) override;
	virtual void lssproto_D_recv(int category, int dx, int dy, char* data) override;
	virtual void lssproto_FS_recv(int flg) override;
	virtual void lssproto_HL_recv(int flg) override;//戰鬥中是否要Help
	virtual void lssproto_PR_recv(int request, int result) override;
	virtual void lssproto_KS_recv(int petarray, int result) override;//指定那一只寵物出場戰鬥
#ifdef _STANDBYPET
	virtual void lssproto_SPET_recv(int standbypet, int result) override;
#endif
	virtual void lssproto_PS_recv(int result, int havepetindex, int havepetskill, int toindex) override;	//寵物合成
	virtual void lssproto_SKUP_recv(int point) override;//取得可加的屬性點數
	virtual void lssproto_WN_recv(int windowtype, int buttontype, int seqno, int objindex, char* data) override;
	virtual void lssproto_EF_recv(int effect, int level, char* option) override;
	virtual void lssproto_SE_recv(const QPoint& pos, int senumber, int sw) override;
	virtual void lssproto_ClientLogin_recv(char* result) override;
	virtual void lssproto_CreateNewChar_recv(char* result, char* data) override;
	virtual void lssproto_CharDelete_recv(char* result, char* data) override;
	virtual void lssproto_CharLogin_recv(char* result, char* data) override;
	virtual void lssproto_CharList_recv(char* result, char* data) override;
	virtual void lssproto_CharLogout_recv(char* result, char* data) override;
	virtual void lssproto_ProcGet_recv(char* data) override;
	virtual void lssproto_PlayerNumGet_recv(int logincount, int player) override;
	virtual void lssproto_Echo_recv(char* test) override;
	virtual void lssproto_NU_recv(int AddCount) override;
	virtual void lssproto_WO_recv(int effect) override;//取得轉生的特效
	virtual void lssproto_TD_recv(char* data) override;
	virtual void lssproto_FM_recv(char* data) override;
#ifdef _ITEM_CRACKER
	virtual void lssproto_IC_recv(const QPoint& pos) override;
#endif
#ifdef _MAGIC_NOCAST//沈默
	virtual void lssproto_NC_recv(int flg) override;
#endif
#ifdef _CHECK_GAMESPEED
	virtual void lssproto_CS_recv(int deltimes) override;
#endif
#ifdef _PETS_SELECTCON
	virtual void lssproto_PETST_recv(int petarray, int result) override;
#endif
#ifdef _CHATROOMPROTOCOL			// (不可開) 聊天室頻道
	virtual void lssproto_CHATROOM_recv(char* data) override;
#endif
#ifdef _NEWREQUESTPROTOCOL			// (不可開) 新增Protocol要求細項
	virtual void lssproto_RESIST_recv(char* data) override;
#endif
#ifdef _ALCHEPLUS
	virtual void lssproto_ALCHEPLUS_recv(char* data) override;
#endif

#ifdef _OUTOFBATTLESKILL			// (不可開) Syu ADD 非戰鬥時技能Protocol
	virtual void lssproto_BATTLESKILL_recv(char* data) override;
#endif
	virtual void lssproto_CHAREFFECT_recv(char* data) override;

#ifdef _STREET_VENDOR
	virtual void lssproto_STREET_VENDOR_recv(char* data) override;	// 擺攤功能
#endif

#ifdef _JOBDAILY
	virtual void lssproto_JOBDAILY_recv(char* data) override;
#endif

#ifdef _FAMILYBADGE_
	virtual void lssproto_FamilyBadge_recv(char* data) override;
#endif

#ifdef _TEACHER_SYSTEM
	virtual void lssproto_TEACHER_SYSTEM_recv(char* data) override;
#endif

#ifdef _ADD_STATUS_2
	virtual void lssproto_S2_recv(char* data) override;
#endif
#ifdef _ITEM_FIREWORK
	virtual void lssproto_Firework_recv(int nCharaindex, int nType, int nActionNum) override;	// 煙火功能
#endif
#ifdef _THEATER
	virtual void lssproto_TheaterData_recv(char* pData) override;
#endif
#ifdef _MOVE_SCREEN
	virtual void lssproto_MoveScreen_recv(BOOL bMoveScreenMode, int iXY) override;	// client 移動熒幕
#endif
#ifdef _NPC_MAGICCARD
	virtual void lssproto_MagiccardAction_recv(char* data) override;	//魔法牌功能
	virtual void lssproto_MagiccardDamage_recv(int position, int damage, int offsetx, int offsety) override;
#endif
#ifdef  _NPC_DANCE
	virtual void lssproto_DancemanOption_recv(int option) override;	//動一動狀態
#endif
#ifdef _ANNOUNCEMENT_
	virtual void lssproto_DENGON_recv(char* data, int colors, int nums) override;
#endif
#ifdef _HUNDRED_KILL
	virtual void lssproto_hundredkill_recv(int flag) override;
#endif
#ifdef _PK2007
	virtual void lssproto_pkList_recv(int count, char* data) override;
#endif
#ifdef _PETBLESS_
	virtual void lssproto_petbless_send(int petpos, int type) override;
#endif
#ifdef _PET_SKINS
	virtual void lssproto_PetSkins_recv(char* data) override;
#endif

	//////////////////////////////////

	virtual void lssproto_CustomWN_recv(const QString& data) override;
	virtual void lssproto_CustomTK_recv(const QString& data) override;
#pragma endregion
};
