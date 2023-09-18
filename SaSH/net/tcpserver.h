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
	{ "翼", PET_WING },
	{ "牙", PET_TOOTH },
	{ "身體", PET_PLATE },
	{ "背", PET_BACK },
	{ "爪", PET_CLAW },
	{ "腳", PET_FOOT },

	{ "头", PET_HEAD },
	{ "翼", PET_WING },
	{ "牙", PET_TOOTH },
	{ "身体", PET_PLATE },
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

enum BufferControl
{
	BC_NONE,
	BC_NEED_TO_CLEAN,
	BC_HAS_NEXT,
	BC_ABOUT_TO_END,
	BC_INVALID,
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
	int dispatchMessage(char* encoded);

	bool handleCustomMessage(QTcpSocket* clientSocket, const QByteArray& data);

	void handleData(QTcpSocket* clientSocket, QByteArray data);

public://actions
	Q_REQUIRED_RESULT int getWorldStatus();

	Q_REQUIRED_RESULT int getGameStatus();

	Q_REQUIRED_RESULT bool checkWG(int w, int g);

	Q_REQUIRED_RESULT int getUnloginStatus();
	void setWorldStatus(int w);
	void setGameStatus(int g);

	bool login(int s);

	void clientLogin(const QString& userName, const QString& password);
	void playerLogin(int index);

	QString getBadStatusString(unsigned int status);

	QString getFieldString(unsigned int field);

	void unlockSecurityCode(const QString& code);

	void clearNetBuffer();

	void logOut();

	void logBack();

	void move(const QPoint& p, const QString& dir);

	void move(const QPoint& p);

	void announce(const QString& msg, int color = 4);

	void createCharacter(int dataplacenum
		, const QString& charname
		, int imgno
		, int faceimgno
		, int vit
		, int str
		, int tgh
		, int dex
		, int earth
		, int water
		, int fire
		, int wind
		, int hometown);

	void deleteCharacter(int index, const QString securityCode, bool backtofirst = false);

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

	void press(BUTTON_TYPE select, int dialogid = -1, int unitid = -1);
	void press(int row, int dialogid = -1, int unitid = -1);

	void buy(int index, int amt, int dialogid = -1, int unitid = -1);
	void sell(const QVector<int>& indexs, int dialogid = -1, int unitid = -1);
	void sell(int index, int dialogid = -1, int unitid = -1);
	void sell(const QString& name, const QString& memo = "", int dialogid = -1, int unitid = -1);
	void learn(int skillIndex, int petIndex, int spot, int dialogid = -1, int unitid = -1);

	void craft(util::CraftType type, const QStringList& ingres);

	void createRemoteDialog(int button, const QString& text);

	void mail(const QVariant& card, const QString& text, int petIndex, const QString& itemName, const QString& itemMemo);

	void warp();

	void shopOk(int n);
	void saMenu(int n);

	bool addPoint(int skillid, int amt);

	void pickItem(int dir);

	void dropGold(int gold);

	void depositGold(int gold, bool isPublic);
	void withdrawGold(int gold, bool isPublic);

	void depositPet(int petIndex, int dialogid = -1, int unitid = -1);
	void withdrawPet(int petIndex, int dialogid = -1, int unitid = -1);

	void depositItem(int index, int dialogid = -1, int unitid = -1);
	void withdrawItem(int itemIndex, int dialogid = -1, int unitid = -1);

	bool captchaOCR(QString* pmsg);

	void setAllPetState();
	void setPetState(int petIndex, PetState state);
	void setFightPet(int petIndex);
	void setRidePet(int petIndex);
	void setStandbyPet(int standbypets);
	void setPetState(int petIndex, int state);

	void updateItemByMemory();
	void updateDatasFromMemory();

	void doBattleWork(bool async);
	void syncBattleAction();
	void asyncBattleAction();

	void downloadMap(int floor = -1);
	void downloadMap(int x, int y, int floor = -1);

	bool tradeStart(const QString& name, int timeout);
	void tradeComfirm(const QString& name);
	void tradeCancel();
	void tradeAppendItems(const QString& name, const QVector<int>& itemIndexs);
	void tradeAppendGold(const QString& name, int gold);
	void tradeAppendPets(const QString& name, const QVector<int>& petIndex);
	void tradeComplete(const QString& name);

	void cleanChatHistory();
	QString getChatHistory(int index);

	bool findUnit(const QString& name, int type, mapunit_t* unit, const QString& freeName = "", int modelid = -1);

	void setTeamState(bool join);
	void kickteam(int n);

	void setPlayerFaceToPoint(const QPoint& pos);
	void setPlayerFaceDirection(int dir);
	void setPlayerFaceDirection(const QString& dirStr);

	int getPartySize() const;
	QStringList getJoinableUnitList() const;
	bool getItemIndexsByName(const QString& name, const QString& memo, QVector<int>* pv, int from = 0, int to = MAX_ITEM);
	int getItemIndexByName(const QString& name, bool isExact = true, const QString& memo = "", int from = 0, int to = MAX_ITEM) const;
	int getPetSkillIndexByName(int& petIndex, const QString& name) const;
	bool getPetIndexsByName(const QString& name, QVector<int>* pv) const;
	int getMagicIndexByName(const QString& name, bool isExact = true) const;
	int getItemEmptySpotIndex() const;
	bool getItemEmptySpotIndexs(QVector<int>* pv) const;
	void clear();

	bool checkPlayerMp(int cmpvalue, int* target = nullptr, bool useequal = false);
	bool checkPlayerHp(int cmpvalue, int* target = nullptr, bool useequal = false);
	bool checkRideHp(int cmpvalue, int* target = nullptr, bool useequal = false);
	bool checkPetHp(int cmpvalue, int* target = nullptr, bool useequal = false);
	bool checkPartyHp(int cmpvalue, int* target);

	bool isPetSpotEmpty() const;
	int checkJobDailyState(const QString& missionName);

	bool isDialogVisible();

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

	void sortItem(bool deepSort = false);

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
	void updateBattleTimeInfo();

	MAGIC getMagic(int magicIndex) const { return magic[magicIndex]; }
	PROFESSION_SKILL getSkill(int skillIndex) const { return profession_skill[skillIndex]; }
	PET getPet(int petIndex) const { return pet[petIndex]; }
	int getPetSize() const
	{
		int n = 0;
		for (const PET& it : pet)
		{
			if (it.level > 0 && it.valid && it.maxHp > 0)
				++n;
		}
		return n;
	}
	PET_SKILL getPetSkill(int petIndex, int skillIndex) const { return petSkill[petIndex][skillIndex]; }
	PARTY getParty(int partyIndex) const { return party[partyIndex]; }
	ITEM getPetEquip(int petIndex, int equipIndex) const { return pet[petIndex].item[equipIndex]; }
	ADDRESS_BOOK getAddressBook(int index) const { return addressBook[index]; }
	battledata_t getBattleData() const
	{
		QReadLocker locker(&battleDataLocker);
		return battleData;
	}
	JOBDAILY getJobDaily(int index) const { return jobdaily[index]; }

	Q_REQUIRED_RESULT int findInjuriedAllie();
	void refreshItemInfo();

	void updateComboBoxList();


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
	Q_REQUIRED_RESULT int getProfessionSkillIndexByName(const QString& names) const
	{
		int i = 0;
		bool isExact = true;
		QStringList list = names.split(util::rexOR, Qt::SkipEmptyParts);
		for (QString name : list)
		{
			if (name.isEmpty())
				continue;

			if (name.startsWith("?"))
			{
				name = name.mid(1);
				isExact = false;
			}

			for (i = 0; i < MAX_PROFESSION_SKILL; ++i)
			{
				if (!profession_skill[i].valid)
					continue;

				if (isExact && profession_skill[i].name == name)
					return i;
				else if (!isExact && profession_skill[i].name.contains(name))
					return i;

			}
		}
		return -1;
	}

#pragma region BattleFunctions
	int playerDoBattleWork(const battledata_t& bt);
	void handlePlayerBattleLogics(const battledata_t& bt);
	int petDoBattleWork(const battledata_t& bt);
	void handlePetBattleLogics(const battledata_t& bt);


	bool isPlayerMpEnoughForMagic(int magicIndex) const;
	bool isPlayerMpEnoughForSkill(int magicIndex) const;
	bool isPlayerHpEnoughForSkill(int magicIndex) const;

	void sortBattleUnit(QVector<battleobject_t>& v) const;

	Q_REQUIRED_RESULT int getBattleSelectableEnemyTarget(const battledata_t& bt) const;

	Q_REQUIRED_RESULT int getBattleSelectableEnemyOneRowTarget(const battledata_t& bt, bool front) const;

	Q_REQUIRED_RESULT int getBattleSelectableAllieTarget(const battledata_t& bt) const;

	Q_REQUIRED_RESULT bool matchBattleEnemyByName(const QString& name, bool isExact, const QVector<battleobject_t>& src, QVector<battleobject_t>* v) const;
	Q_REQUIRED_RESULT bool matchBattleEnemyByLevel(int level, const QVector<battleobject_t>& src, QVector<battleobject_t>* v) const;
	Q_REQUIRED_RESULT bool matchBattleEnemyByMaxHp(int maxHp, const QVector<battleobject_t>& src, QVector<battleobject_t>* v) const;

	Q_REQUIRED_RESULT int getGetPetSkillIndexByName(int petIndex, const QString& name) const;

	bool fixPlayerTargetByMagicIndex(int magicIndex, int oldtarget, int* target) const;
	bool fixPlayerTargetBySkillIndex(int magicIndex, int oldtarget, int* target) const;
	bool fixPlayerTargetByItemIndex(int itemIndex, int oldtarget, int* target) const;
	bool fixPetTargetBySkillIndex(int skillIndex, int oldtarget, int* target) const;
	void updateCurrentSideRange(battledata_t& bt);
	inline bool checkFlagState(int pos);

	void setBattleData(const battledata_t& data)
	{
		QWriteLocker locker(&battleDataLocker);
		battleData = data;
	}

#pragma endregion

#pragma region SAClientOriginal
	//StoneAge Client Original Functions

	void swapItemLocal(int from, int to);

	void updateMapArea()
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

	void realTimeToSATime(LSTIME* lstime);

	inline void setFloor(int floor) { nowFloor = floor; }

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
	PARTY party[MAX_AIRPLANENUM];
#else
	PARTY party[MAX_PARTY] = {};
#endif

#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
	PROFESSION_SKILL profession_skill[MAX_PROFESSION_SKILL];
#endif

	PET_SKILL petSkill[MAX_PET][MAX_SKILL] = {};

	int swapitemModeFlag = 0;
	QHash<QString, bool>itemStackFlagHash = {};

	QVector<bool> battlePetDisableList_ = {};

	//client original 目前很多都是沒用處的
#pragma region ClientOriginal
	MAGIC magic[MAX_MAGIC] = {};

	BATTLE_RESULT_MSG battleResultMsg = {};

	ADDRESS_BOOK addressBook[MAX_ADDRESS_BOOK] = {};

	JOBDAILY jobdaily[MAX_MISSION] = {};

	CHARLISTTABLE chartable[MAX_CHARACTER] = {};

	MAIL_HISTORY mailHistory[MAX_ADDRESS_BOOK] = {};

	bool hasTeam = false;
	unsigned int ProcNo = 0u;
	unsigned int SubProcNo = 0u;
	unsigned int MenuToggleFlag = 0u;

	//trade
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
	int battleCurrentAnimeFlag = 0;
	int BattlePetStMenCnt = 0;

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
	bool IS_TRADING = false;

	bool IS_DISCONNECTED = false;

	std::atomic_bool IS_TCP_CONNECTION_OK_TO_USE = false;

	bool IS_WAITFOR_JOBDAILY_FLAG = false;
	bool IS_WAITFOR_BANK_FLAG = false;
	bool IS_WAITFOR_DIALOG_FLAG = false;
	bool IS_WAITFOR_EXTRA_DIALOG_INFO_FLAG = false;
	bool IS_WAITFOR_CUSTOM_DIALOG_FLAG = false;

	std::atomic_bool isBattleDialogReady = false;
	std::atomic_bool isEOTTLSend = false;
	std::atomic_int lastEOTime = 0;

	QElapsedTimer loginTimer;
	QElapsedTimer battleDurationTimer;
	QElapsedTimer normalDurationTimer;
	QElapsedTimer oneRoundDurationTimer;

	std::atomic_int battleCurrentRound = 0;
	std::atomic_llong battle_total_time = 0;
	std::atomic_int battle_total = 0;
	std::atomic_llong battle_one_round_time = 0;

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

	util::SafeData<dialog_t> currentDialog = {};

	//用於緩存要發送到UI的數據(開啟子窗口初始化並加載當前最新數據時使用)
	util::SafeHash<int, QVariant> playerInfoColContents;
	util::SafeHash<int, QVariant> itemInfoRowContents;
	util::SafeHash<int, QVariant> equipInfoRowContents;
	util::SafeData<QStringList> enemyNameListCache;
	util::SafeData<QVariant> topInfoContents;
	util::SafeData<QVariant> bottomInfoContents;
	util::SafeData<QString> timeLabelContents;
	util::SafeData<QString> labelPlayerAction;
	util::SafeData<QString> labelPetAction;

private:
	unsigned short port_ = 0;

	QSharedPointer<QTcpServer> server_;

	QList<QTcpSocket*> clientSockets_;

	QByteArray net_readbuf_;

	QMutex net_mutex;

	QByteArray tmpdata_;

private://lssproto
	bool appendReadBuf(const QByteArray& data);
	QByteArrayList splitLinesFromReadBuf();
	int a62toi(const QString& a) const;
	int getStringToken(const QString& src, const QString& delim, int count, QString& out) const;
	int getIntegerToken(const QString& src, const QString& delim, int count) const;
	int getInteger62Token(const QString& src, const QString& delim, int count) const;
	QString makeStringFromEscaped(QString& src) const;

private://lssproto_recv
#pragma region Lssproto_Recv
	virtual void lssproto_XYD_recv(const QPoint& pos, int dir) override;//戰鬥結束後的大地圖座標
	virtual void lssproto_EV_recv(int dialogid, int result) override;
	virtual void lssproto_EN_recv(int result, int field) override;
	virtual void lssproto_RS_recv(char* data) override;
	virtual void lssproto_RD_recv(char* data) override;
	virtual void lssproto_B_recv(char* command) override;
	virtual void lssproto_I_recv(char* data) override;
	virtual void lssproto_SI_recv(int fromindex, int toindex) override;
	virtual void lssproto_MSG_recv(int aindex, char* text, int color) override;	//收到普通郵件或寵物郵件
	virtual void lssproto_PME_recv(int unitid, int graphicsno, const QPoint& pos, int dir, int flg, int no, char* cdata) override;
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
	virtual void lssproto_WN_recv(int windowtype, int buttontype, int dialogid, int unitid, char* data) override;
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
