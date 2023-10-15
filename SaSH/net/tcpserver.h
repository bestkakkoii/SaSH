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
#include <indexer.h>
#include <util.h>
#include "lssproto.h"
#include "map/mapanalyzer.h"

class Socket : public QTcpSocket
{
	Q_OBJECT

public:
	Socket(qintptr socketDescriptor, QObject* parent = nullptr);

	virtual ~Socket();

	QThread thread;

signals:
	void read(const QByteArray& badata);

private slots:
	void onReadyRead();
	void onWrite(QByteArray ba, long long size);

private:
	long long index_ = -1;
	bool init = false;
};

class Worker : public ThreadPlugin, public Lssproto
{
	Q_OBJECT

public:
	Worker(long long index, Socket* socket, QObject* parent);

	virtual ~Worker();

private:
	QHash<long long, QString> g_dirStrHash = {
		{ 0, QObject::tr("North") },
		{ 1, QObject::tr("ENorth") },
		{ 2, QObject::tr("East") },
		{ 3, QObject::tr("ESouth") },
		{ 4, QObject::tr("South") },
		{ 5, QObject::tr("WSouth") },
		{ 6, QObject::tr("West") },
		{ 7, QObject::tr("WNorth") },
	};


signals:
	void write(QByteArray ba, long long size = 0);

public slots:
	void onClientReadyRead(const QByteArray& badata);

private:
	long long dispatchMessage(const QByteArray& encoded);

	bool handleCustomMessage(const QByteArray& data);

	Q_INVOKABLE void handleData(QByteArray data);

public://actions
	QString battleStringFormat(const battleobject_t& obj, QString formatStr);

	Q_REQUIRED_RESULT long long getWorldStatus();

	Q_REQUIRED_RESULT long long getGameStatus();

	Q_REQUIRED_RESULT bool checkWG(long long  w, long long g);

	Q_REQUIRED_RESULT long long getUnloginStatus();
	void setWorldStatus(long long w);
	void setGameStatus(long long g);

	bool login(long long s);

	void clientLogin(const QString& userName, const QString& password);
	void playerLogin(long long index);

	QString getBadStatusString(long long status);

	QString getFieldString(long long field);

	void unlockSecurityCode(const QString& code);

	void clearNetBuffer();

	void logOut();

	void logBack();

	void move(const QPoint& p, const QString& dir);

	void move(const QPoint& p);

	void announce(const QString& msg, long long color = 4);

	void createCharacter(long long dataplacenum
		, const QString& charname
		, long long imgno
		, long long faceimgno
		, long long vit
		, long long str
		, long long tgh
		, long long dex
		, long long earth
		, long long water
		, long long fire
		, long long wind
		, long long hometown
		, bool forcecover);

	void deleteCharacter(long long index, const QString securityCode, bool backtofirst = false);

	void talk(const QString& text, long long color = 0, TalkMode mode = kTalkNormal);
	void inputtext(const QString& text, long long dialogid = -1, long long npcid = -1);

	void windowPacket(const QString& command, long long dialogid, long long npcid);

	void EO();

	void dropItem(long long index);
	void dropItem(QVector<long long> index);

	void useItem(long long itemIndex, long long target);


	void swapItem(long long from, long long to);

	void petitemswap(long long petIndex, long long from, long long to);

	void useMagic(long long magicIndex, long long target);

	void dropPet(long long petIndex);

	void setSwitcher(long long flg, bool enable);

	void setSwitcher(long long flg);

	void press(BUTTON_TYPE select, long long dialogid = -1, long long unitid = -1);
	void press(long long row, long long dialogid = -1, long long unitid = -1);

	void buy(long long index, long long amt, long long dialogid = -1, long long unitid = -1);
	void sell(const QVector<long long>& indexs, long long dialogid = -1, long long unitid = -1);
	void sell(long long index, long long dialogid = -1, long long unitid = -1);
	void sell(const QString& name, const QString& memo = "", long long dialogid = -1, long long unitid = -1);
	void learn(long long skillIndex, long long petIndex, long long spot, long long dialogid = -1, long long unitid = -1);

	void craft(util::CraftType type, const QStringList& ingres);

	void createRemoteDialog(unsigned long long type, unsigned long long button, const QString& text);

	void mail(const QVariant& card, const QString& text, long long petIndex, const QString& itemName, const QString& itemMemo);

	void warp();

	void shopOk(long long n);
	void saMenu(long long n);

	bool addPoint(long long skillid, long long amt);

	void pickItem(long long dir);

	void dropGold(long long gold);

	void depositGold(long long gold, bool isPublic);
	void withdrawGold(long long gold, bool isPublic);

	void depositPet(long long petIndex, long long dialogid = -1, long long unitid = -1);
	void withdrawPet(long long petIndex, long long dialogid = -1, long long unitid = -1);

	void depositItem(long long index, long long dialogid = -1, long long unitid = -1);
	void withdrawItem(long long itemIndex, long long dialogid = -1, long long unitid = -1);

	bool captchaOCR(QString* pmsg);

	void setAllPetState();
	void setPetState(long long petIndex, PetState state);
	void setFightPet(long long petIndex);
	void setRidePet(long long petIndex);
	void setPetStateSub(long long petIndex, long long state);
	void setPetStandby(long long petIndex, long long state);

	void updateItemByMemory();
	void updateDatasFromMemory();

	void doBattleWork(bool waitforBA);
	void asyncBattleAction(bool waitforBA);

	void downloadMap(long long floor = -1);
	void downloadMap(long long x, long long y, long long floor = -1);

	bool tradeStart(const QString& name, long long timeout);
	void tradeComfirm(const QString& name);
	void tradeCancel();
	void tradeAppendItems(const QString& name, const QVector<long long>& itemIndexs);
	void tradeAppendGold(const QString& name, long long gold);
	void tradeAppendPets(const QString& name, const QVector<long long>& petIndex);
	void tradeComplete(const QString& name);

	void cleanChatHistory();
	Q_REQUIRED_RESULT QString getChatHistory(long long index);

	bool findUnit(const QString& name, long long type, mapunit_t* unit, const QString& freeName = "", long long modelid = -1);

	Q_REQUIRED_RESULT QString getGround();

	void setTeamState(bool join);
	void kickteam(long long n);

	long long setCharFaceToPoint(const QPoint& pos);
	void setCharFaceDirection(long long dir);
	void setCharFaceDirection(const QString& dirStr);

	Q_REQUIRED_RESULT long long getPartySize() const;
	Q_REQUIRED_RESULT QStringList getJoinableUnitList() const;
	Q_REQUIRED_RESULT bool getItemIndexsByName(const QString& name, const QString& memo, QVector<long long>* pv, long long from = 0, long long to = MAX_ITEM);
	Q_REQUIRED_RESULT long long getItemIndexByName(const QString& name, bool isExact = true, const QString& memo = "", long long from = 0, long long to = MAX_ITEM);
	Q_REQUIRED_RESULT long long getPetSkillIndexByName(long long& petIndex, const QString& name) const;
	Q_REQUIRED_RESULT long long getSkillIndexByName(const QString& name) const;
	Q_REQUIRED_RESULT bool getPetIndexsByName(const QString& name, QVector<long long>* pv) const;
	Q_REQUIRED_RESULT long long getMagicIndexByName(const QString& name, bool isExact = true) const;
	Q_REQUIRED_RESULT long long getItemEmptySpotIndex();
	bool getItemEmptySpotIndexs(QVector<long long>* pv);
	void clear();

	Q_REQUIRED_RESULT bool checkCharMp(long long cmpvalue, long long* target = nullptr, bool useequal = false);
	Q_REQUIRED_RESULT bool checkCharHp(long long cmpvalue, long long* target = nullptr, bool useequal = false);
	Q_REQUIRED_RESULT bool checkRideHp(long long cmpvalue, long long* target = nullptr, bool useequal = false);
	Q_REQUIRED_RESULT bool checkPetHp(long long cmpvalue, long long* target = nullptr, bool useequal = false);
	Q_REQUIRED_RESULT bool checkPartyHp(long long cmpvalue, long long* target);

	Q_REQUIRED_RESULT bool isPetSpotEmpty() const;
	Q_REQUIRED_RESULT long long checkJobDailyState(const QString& missionName);

	Q_REQUIRED_RESULT bool isDialogVisible();

	void setCharFreeName(const QString& name);
	void setPetFreeName(long long petIndex, const QString& name);

	Q_REQUIRED_RESULT bool getBattleFlag();
	Q_REQUIRED_RESULT bool getOnlineFlag() const;

	void sortItem(bool deepSort = false);

	Q_REQUIRED_RESULT long long getDir();
	Q_REQUIRED_RESULT QPoint getPoint();
	void setPoint(const QPoint& pos);

	Q_REQUIRED_RESULT long long getFloor();
	void setFloor(long long floor);

	Q_REQUIRED_RESULT QString getFloorName();
	//battle
	void sendBattleCharAttackAct(long long target);
	void sendBattleCharMagicAct(long long magicIndex, long long target);
	void sendBattleCharJobSkillAct(long long skillIndex, long long target);
	void sendBattleCharItemAct(long long itemIndex, long long target);
	void sendBattleCharDefenseAct();
	void sendBattleCharEscapeAct();
	void sendBattleCharCatchPetAct(long long petIndex);
	void sendBattleCharSwitchPetAct(long long petIndex);
	void sendBattleCharDoNothing();
	void sendBattlePetSkillAct(long long skillIndex, long long target);
	void sendBattlePetDoNothing();
	void setBattleEnd();

	void updateBattleTimeInfo();

	inline Q_REQUIRED_RESULT PC getPC() const { /*QReadLocker locker(&charInfoLock_); */ return pc_; }
	inline void setPC(PC pc) { pc_ = pc; }

	inline Q_REQUIRED_RESULT MAGIC getMagic(long long magicIndex) const { return magic_.value(magicIndex); }

	inline Q_REQUIRED_RESULT PROFESSION_SKILL getSkill(long long skillIndex) const { /*QReadLocker locker(&charSkillInfoLock_); */ return profession_skill_.value(skillIndex); }
	inline Q_REQUIRED_RESULT QHash<long long, PROFESSION_SKILL> getSkills() const { /*QReadLocker locker(&charSkillInfoLock_); */ return profession_skill_.toHash(); }

	inline Q_REQUIRED_RESULT PET getPet(long long petIndex) const { QReadLocker locker(&petInfoLock_);  return pet_.value(petIndex); }
	inline Q_REQUIRED_RESULT QHash<long long, PET> getPets() const { QReadLocker locker(&petInfoLock_);  return pet_.toHash(); }
	inline Q_REQUIRED_RESULT long long getPetSize() const
	{
		QReadLocker locker(&petInfoLock_);
		long long n = 0;
		QHash<long long, PET> pets = pet_.toHash();
		for (const PET& it : pets)
		{
			if (it.level > 0 && it.valid && it.maxHp > 0)
				++n;
		}
		return n;
	}

	inline Q_REQUIRED_RESULT ITEM getItem(long long index) const { QReadLocker locker(&itemInfoLock_);  return item_.value(index); }
	inline Q_REQUIRED_RESULT QHash<long long, ITEM> getItems() const { QReadLocker locker(&itemInfoLock_);  return item_.toHash(); }

	inline Q_REQUIRED_RESULT PET_SKILL getPetSkill(long long petIndex, long long skillIndex) const { /*QReadLocker locker(&petSkillInfoLock_); */ return petSkill_.value(petIndex).value(skillIndex); }
	inline Q_REQUIRED_RESULT QHash<long long, PET_SKILL> getPetSkills(long long petIndex) const { /*QReadLocker locker(&petSkillInfoLock_); */ return petSkill_.value(petIndex); }
	inline void setPetSkills(long long petIndex, const QHash<long long, PET_SKILL>& skills) { petSkill_.insert(petIndex, skills); }
	inline void setPetSkill(long long petIndex, long long skillIndex, const PET_SKILL& skill)
	{
		QHash<long long, PET_SKILL> skills = petSkill_.value(petIndex);
		skills.insert(skillIndex, skill);
		petSkill_.insert(petIndex, skills);
	}

	inline Q_REQUIRED_RESULT PARTY getParty(long long partyIndex) const { /*QReadLocker locker(&teamInfoLock_); */ return party_.value(partyIndex); }
	inline Q_REQUIRED_RESULT QHash<long long, PARTY> getParties() const { /*QReadLocker locker(&teamInfoLock_); */ return party_.toHash(); }

	inline Q_REQUIRED_RESULT ITEM getPetEquip(long long petIndex, long long equipIndex) const {/* QReadLocker locker(&petEquipInfoLock_); */ return petItem_.value(petIndex).value(equipIndex); }
	inline Q_REQUIRED_RESULT QHash<long long, ITEM> getPetEquips(long long petIndex) const { /*QReadLocker locker(&petEquipInfoLock_); */ return petItem_.value(petIndex); }

	inline Q_REQUIRED_RESULT ADDRESS_BOOK getAddressBook(long long index) const { return addressBook_.value(index); }
	inline Q_REQUIRED_RESULT QHash<long long, ADDRESS_BOOK> getAddressBooks() const { return addressBook_.toHash(); }

	inline Q_REQUIRED_RESULT battledata_t getBattleData() const { return battleData.get(); }
	inline Q_REQUIRED_RESULT JOBDAILY getJobDaily(long long index) const { return jobdaily_.value(index); }
	inline Q_REQUIRED_RESULT QHash<long long, JOBDAILY> getJobDailys() const { return jobdaily_.toHash(); }
	inline Q_REQUIRED_RESULT CHARLISTTABLE getCharListTable(long long index) const { return chartable_.value(index); }
	inline Q_REQUIRED_RESULT MAIL_HISTORY getMailHistory(long long index) const { return mailHistory_.value(index); }

	Q_REQUIRED_RESULT long long findInjuriedAllie();

	void refreshItemInfo();

	void updateComboBoxList();

	void setWindowTitle(QString formatStr);
private:

	void refreshItemInfo(long long index);

	void setBattleFlag(bool enable);
	void setOnlineFlag(bool enable);

	void getCharMaxCarryingCapacity();
	inline Q_REQUIRED_RESULT constexpr bool isItemStackable(long long flg) { return ((flg >> 2) & 1); }
	QString getAreaString(long long target);
	Q_REQUIRED_RESULT bool matchPetNameByIndex(long long index, const QString& name);
	Q_REQUIRED_RESULT long long getProfessionSkillIndexByName(const QString& names) const;

#pragma region BattleFunctions
	long long playerDoBattleWork(const battledata_t& bt);
	void handleCharBattleLogics(const battledata_t& bt);
	long long petDoBattleWork(const battledata_t& bt);
	void handlePetBattleLogics(const battledata_t& bt);

	bool isCharMpEnoughForMagic(long long magicIndex) const;
	bool isCharMpEnoughForSkill(long long magicIndex) const;
	bool isCharHpEnoughForSkill(long long magicIndex) const;

	void sortBattleUnit(QVector<battleobject_t>& v) const;

	enum BattleMatchType
	{
		MatchNotUsed,
		MatchPos,
		MatchLevel,
		MatchHp,
		MatchMaxHp,
		MatchHPP,
		MatchModel,
		MatchName,
		MatchStatus
	};
	bool matchBattleTarget(const QVector<battleobject_t>& btobjs, BattleMatchType matchtype, long long firstMatchPos, QString op, QVariant cmpvar, long long* ppos);
	bool conditionMatchTarget(QVector<battleobject_t> btobjs, const QString& conditionStr, long long* ppos);

	Q_REQUIRED_RESULT long long getBattleSelectableEnemyTarget(const battledata_t& bt) const;

	Q_REQUIRED_RESULT long long getBattleSelectableEnemyOneRowTarget(const battledata_t& bt, bool front) const;

	Q_REQUIRED_RESULT long long getBattleSelectableAllieTarget(const battledata_t& bt) const;

	Q_REQUIRED_RESULT bool matchBattleEnemyByName(const QString& name, bool isExact, const QVector<battleobject_t>& src, QVector<battleobject_t>* v) const;
	Q_REQUIRED_RESULT bool matchBattleEnemyByLevel(long long level, const QVector<battleobject_t>& src, QVector<battleobject_t>* v) const;
	Q_REQUIRED_RESULT bool matchBattleEnemyByMaxHp(long long  maxHp, const QVector<battleobject_t>& src, QVector<battleobject_t>* v) const;

	Q_REQUIRED_RESULT long long getGetPetSkillIndexByName(long long etIndex, const QString& name) const;

	bool fixCharTargetByMagicIndex(long long magicIndex, long long oldtarget, long long* target) const;
	bool fixCharTargetBySkillIndex(long long magicIndex, long long oldtarget, long long* target) const;
	bool fixCharTargetByItemIndex(long long itemIndex, long long oldtarget, long long* target) const;
	bool fixPetTargetBySkillIndex(long long skillIndex, long long oldtarget, long long* target) const;
	void updateCurrentSideRange(battledata_t& bt);
	bool checkFlagState(long long pos);

	inline void setBattleData(const battledata_t& data) { battleData = data; }

	//自動鎖寵
	void checkAutoLockPet();

	//自動加點
	void checkAutoAbility();

	//檢查並自動吃肉、或丟肉
	void checkAutoDropMeat();
#pragma endregion

#pragma region SAClientOriginal
	//StoneAge Client Original Functions

	void swapItemLocal(long long from, long long to);

	void realTimeToSATime(LSTIME* lstime);

#pragma endregion

private: //lockers
	mutable QReadWriteLock petInfoLock_;
	//mutable QReadWriteLock petSkillInfoLock_;
	//mutable QReadWriteLock charInfoLock_;
	//mutable QReadWriteLock charSkillInfoLock_;
	//mutable QReadWriteLock charMagicInfoLock_;
	mutable QReadWriteLock itemInfoLock_;
	//mutable QReadWriteLock petEquipInfoLock_;
	//mutable QReadWriteLock teamInfoLock_;

private:
	std::atomic_bool IS_BATTLE_FLAG = false;
	std::atomic_bool IS_ONLINE_FLAG = false;

	QElapsedTimer eottlTimer;//伺服器響應時間(MS)
	QElapsedTimer connectingTimer;
	std::atomic_bool petEscapeEnableTempFlag = false;
	long long tempCatchPetTargetIndex = -1;

	util::SafeData<battledata_t> battleData;

	std::atomic_bool IS_WAITFOT_SKUP_RECV = false;
	QFuture<void> skupFuture;

	std::atomic_bool IS_LOCKATTACK_ESCAPE_DISABLE = false;//鎖定攻擊不逃跑 (轉指定攻擊)

	PC pc_ = {};

	util::SafeHash<long long, PARTY> party_ = {};
	util::SafeHash<long long, ITEM> item_ = {};
	util::SafeHash<long long, QHash<long long, ITEM>> petItem_ = {};
	util::SafeHash<long long, PET> pet_ = {};
	util::SafeHash<long long, MAGIC> magic_ = {};
	util::SafeHash<long long, ADDRESS_BOOK> addressBook_ = {};
	util::SafeHash<long long, JOBDAILY> jobdaily_ = {};
	util::SafeHash<long long, CHARLISTTABLE> chartable_ = {};
	util::SafeHash<long long, MAIL_HISTORY> mailHistory_ = {};
	util::SafeHash<long long, PROFESSION_SKILL> profession_skill_ = {};
	util::SafeHash<long long, QHash<long long, PET_SKILL>> petSkill_ = {};

	long long swapitemModeFlag = 0; //當前自動整理功能的階段
	QHash<QString, bool>itemStackFlagHash = {};

	util::SafeVector<bool> battlePetDisableList_ = {};

	//戰鬥相關
	std::atomic_llong battleCharCurrentPos = 0;
	std::atomic_llong battleBpFlag = 0;
	std::atomic_bool battleCharEscapeFlag = 0;
	std::atomic_llong battleCharCurrentMp = 0;
	std::atomic_llong battleCurrentAnimeFlag = 0;

	//client original 目前很多都是沒用處的
#pragma region ClientOriginal
	QString lastSecretChatName = "";//最後一次收到密語的發送方名稱

	//遊戲內當前時間相關
	LSTIME saTimeStruct = { 0 };
	long long serverTime = 0LL;
	long long FirstTime = 0LL;

	//交易相關
	long long opp_showindex = 0;
	QString opp_sockfd;
	QString opp_name;
	QString opp_goldmount;
	long long showindex[7] = { 0 };
	long long tradeWndDropGoldGet = 0;
	QString opp_itemgraph;
	QString opp_itemname;
	QString opp_itemeffect;
	QString opp_itemindex;
	QString opp_itemdamage;
	QString trade_kind;
	QString trade_command;
	long long tradeStatus = 0;
	long long tradePetIndex = -1;
	PET tradePet[2] = {};
	showitem opp_item[MAX_MAXHAVEITEM];	//交易道具阵列增为15个
	showpet opp_pet[MAX_PET];
	QStringList myitem_tradeList;
	QStringList mypet_tradeList = { "P|-1", "P|-1", "P|-1" , "P|-1", "P|-1" };
	long long mygoldtrade = 0;

	//郵件相關
	long long mailHistoryWndSelectNo = 0;
	long long mailHistoryWndPageNo = 0;
#pragma endregion

public:
	std::atomic_llong nowFloor_;
	util::SafeData<QString> nowFloorName_;
	util::SafeData<QPoint> nowPoint_;

	//custom
	std::atomic_bool IS_TRADING = false;
	std::atomic_bool IS_DISCONNECTED = false;

	std::atomic_bool IS_WAITFOR_JOBDAILY_FLAG = false;
	std::atomic_bool IS_WAITFOR_BANK_FLAG = false;
	std::atomic_bool IS_WAITFOR_DIALOG_FLAG = false;
	std::atomic_bool IS_WAITFOR_CUSTOM_DIALOG_FLAG = false;
	std::atomic_llong IS_WAITOFR_ITEM_CHANGE_PACKET = false;

	std::atomic_bool isBattleDialogReady = false;
	std::atomic_bool isEOTTLSend = false;
	std::atomic_llong lastEOTime = 0;

	QElapsedTimer loginTimer;
	QElapsedTimer battleDurationTimer;
	QElapsedTimer normalDurationTimer;
	QElapsedTimer oneRoundDurationTimer;

	std::atomic_llong battleCurrentRound = 0;
	std::atomic_llong battle_total_time = 0;
	std::atomic_llong battle_total = 0;
	std::atomic_llong battle_one_round_time = 0;

	std::atomic_llong saCurrentGameTime = 0;//遊戲時間 LSTIME_SECTION

	MapAnalyzer mapAnalyzer;

	util::SafeData<currencydata_t> currencyData = {};
	util::SafeData<customdialog_t> customDialog = {};

	util::SafeHash<long long, mapunit_t> mapUnitHash;
	util::SafeHash<QPoint, mapunit_t> npcUnitPointHash;
	util::SafeQueue<QPair<long long, QString>> chatQueue;

	QPair<long long, QVector<bankpet_t>> currentBankPetList;
	util::SafeVector<ITEM> currentBankItemList;

	QElapsedTimer repTimer;
	util::AfkRecorder recorder[1 + MAX_PET] = {};

	util::SafeData<dialog_t> currentDialog = {};

	//用於緩存要發送到UI的數據(開啟子窗口初始化並加載當前最新數據時使用)
	util::SafeHash<long long, QVariant> playerInfoColContents;
	util::SafeHash<long long, QVariant> itemInfoRowContents;
	util::SafeHash<long long, QVariant> equipInfoRowContents;
	util::SafeData<QStringList> enemyNameListCache;
	util::SafeData<QVariant> topInfoContents;
	util::SafeData<QVariant> bottomInfoContents;
	util::SafeData<QString> timeLabelContents;
	util::SafeData<QString> labelCharAction;
	util::SafeData<QString> labelPetAction;

private:

	QByteArray net_readbuf_;
	QByteArray net_raw_;

	QMutex net_mutex;

private://lssproto
	long long appendReadBuf(const QByteArray& data);
	QByteArrayList splitLinesFromReadBuf();
	long long a62toi(const QString& a) const;
	long long getStringToken(const QString& src, const QString& delim, long long count, QString& out) const;
	long long getIntegerToken(const QString& src, const QString& delim, long long count) const;
	long long getInteger62Token(const QString& src, const QString& delim, long long count) const;
	void makeStringFromEscaped(QString& src) const;

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
	virtual void lssproto_CharNumGet_recv(int logincount, int player) override;
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

	virtual void lssproto_S2_recv(char* data) override;

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

class Server : public QTcpServer
{
	Q_OBJECT
public:
	explicit Server(QObject* parent = nullptr);

	virtual ~Server();

	bool start(QObject* parent);

	void clear();

protected:
	virtual void incomingConnection(qintptr socketDescriptor) override;

private:
	QList<Socket*> clientSockets_;
};
