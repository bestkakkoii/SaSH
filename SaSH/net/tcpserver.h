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
#include <QCoreApplication>
#include <QTimer>
#include <iostream>
#include <string>
#include <vector>
#include <WS2tcpip.h>

#include <indexer.h>
#include <util.h>
#include "lssproto.h"
#include "autil.h"
#include "map/mapdevice.h"

class Socket : public QTcpSocket
{
	Q_OBJECT

public:
	Socket(qintptr socketDescriptor, QObject* parent = nullptr);

	virtual ~Socket();

	//QThread thread;

private slots:
	void onReadyRead();
	void onWrite(QByteArray ba, long long size);

private:
	long long index_ = -1;
	bool init = false;
	mutable QMutex socketLock_;
	QFuture<void> future_;
};

class CLua;
class Worker : public QObject, public Indexer, public Lssproto
{
	Q_OBJECT
public:
	Worker(long long index, QObject* parent);

	virtual ~Worker();

private:
	QHash<long long, QString> g_dirStrHash = {
		{ -1, QObject::tr("Unknown") },
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
	void findPathFinished();

public slots:

public:
	void handleData(QByteArray badata);

private:
	void __fastcall dispatchSendMessage(const QByteArray& encoded) const;

	long long __fastcall dispatchMessage(const QByteArray& encoded);

	bool __fastcall handleCustomMessage(const QByteArray& data);



public://actions
	void __fastcall clearNetBuffer();

	QString __fastcall battleStringFormat(const sa::battle_object_t& obj, QString formatStr) const;

	[[nodiscard]] long long __fastcall getWorldStatus() const;

	[[nodiscard]] long long __fastcall getGameStatus() const;

	[[nodiscard]] bool __fastcall checkWG(long long  w, long long g) const;

	[[nodiscard]] long long __fastcall getUnloginStatus();
	void __fastcall setWorldStatus(long long w) const;
	void __fastcall setGameStatus(long long g) const;

	bool __fastcall login(long long s);

	bool __fastcall clientLogin(const QString& userName, const QString& password);
	bool __fastcall playerLogin(long long index);

	static QString __fastcall getBadStatusString(long long status);

	QString __fastcall getFieldString(long long field);

	bool __fastcall unlockSecurityCode(const QString& code);

	bool __fastcall logOut();

	bool __fastcall logBack();

	bool __fastcall move(const QPoint& p, const QString& dir);

	bool __fastcall move(const QPoint& p) const;

	bool __fastcall stepMove(const QPoint& p);

	bool __fastcall announce(const QString& msg, long long color = 4);

	bool __fastcall createCharacter(long long dataplacenum
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

	bool __fastcall deleteCharacter(long long index, const QString securityCode, bool backtofirst = false);

	bool __fastcall talk(const QString& text, long long color = 0, sa::TalkMode mode = sa::kTalkNormal);
	bool __fastcall inputtext(const QString& text, long long dialogid = -1, long long npcid = -1);

	bool __fastcall windowPacket(const QString& command, long long dialogid, long long npcid);

	bool __fastcall EO();

	bool __fastcall echo();

	bool __fastcall dropItem(long long index);
	bool __fastcall dropItem(QVector<long long> index);

	bool __fastcall useItem(long long itemIndex, long long target);

	bool __fastcall swapItem(long long from, long long to);

	bool __fastcall petitemswap(long long petIndex, long long from, long long to);

	bool __fastcall useMagic(long long magicIndex, long long target);

	bool __fastcall dropPet(long long petIndex);

	bool __fastcall setSwitcher(long long flg, bool enable);

	bool __fastcall setSwitchers(long long flg);

	bool __fastcall press(sa::ButtonType select, long long dialogid = -1, long long unitid = -1);
	bool __fastcall press(long long row, long long dialogid = -1, long long unitid = -1);

	bool __fastcall buy(long long index, long long amt, long long dialogid = -1, long long unitid = -1);
	bool __fastcall sell(const QVector<long long>& indexs, long long dialogid = -1, long long unitid = -1);
	bool __fastcall sell(long long index, long long dialogid = -1, long long unitid = -1);
	bool __fastcall sell(const QString& name, const QString& memo = "", long long dialogid = -1, long long unitid = -1);
	bool __fastcall learn(long long petIndex, long long shopSkillIndex, long long petSkillSpot, long long dialogid = -1, long long unitid = -1);

	bool __fastcall craft(sa::CraftType type, const QStringList& ingres);

	bool __fastcall createRemoteDialog(unsigned long long type, unsigned long long button, const QString& text) const;

	bool __fastcall mail(const QVariant& card, const QString& text, long long petIndex, const QString& itemName, const QString& itemMemo);

	bool __fastcall warp();

	bool __fastcall shopOk(long long n);
	bool __fastcall saMenu(long long n);

	bool __fastcall addPoint(long long skillid, long long amt);

	bool __fastcall pickItem(long long dir);

	bool __fastcall dropGold(long long gold);

	bool __fastcall depositGold(long long gold, bool isPublic);
	bool __fastcall withdrawGold(long long gold, bool isPublic);

	bool __fastcall depositPet(long long petIndex, long long dialogid = -1, long long unitid = -1);
	bool __fastcall withdrawPet(long long petIndex, long long dialogid = -1, long long unitid = -1);

	bool __fastcall depositItem(long long index, long long dialogid = -1, long long unitid = -1);
	bool __fastcall withdrawItem(long long itemIndex, long long dialogid = -1, long long unitid = -1);

#ifdef OCR_ENABLE
	bool __fastcall captchaOCR(QString* pmsg);
#endif

	void __fastcall findPathAsync(const QPoint& dst, const std::function<bool()>& func);

	bool __fastcall setAllPetState();
	bool __fastcall setPetState(long long petIndex, sa::PetState state);
	bool __fastcall setFightPet(long long petIndex);
	bool __fastcall setRidePet(long long petIndex);
	bool __fastcall setPetStateSub(long long petIndex, long long state);
	bool __fastcall setPetStandby(long long petIndex, long long state);

	void __fastcall updateItemByMemory();
	void __fastcall updateDatasFromMemory();

	//void __fastcall doBattleWork(bool canDelay);
	bool asyncBattleAction(bool canDelay);

	bool __fastcall downloadMap(long long floor = -1);
	bool __fastcall downloadMap(long long x, long long y, long long floor = -1);

	bool __fastcall tradeStart(const QString& name, long long timeout);
	bool __fastcall tradeComfirm(const QString& name);
	bool __fastcall tradeCancel();
	long long __fastcall tradeAppendItems(const QString& name, const QVector<long long>& itemIndexs);
	long long __fastcall tradeAppendGold(const QString& name, long long gold);
	long long __fastcall tradeAppendPets(const QString& name, const QVector<long long>& petIndex);
	bool __fastcall tradeComplete(const QString& name);

	bool __fastcall cleanChatHistory();
	[[nodiscard]] QString __fastcall getChatHistory(long long index) const;

	bool __fastcall findUnit(const QString& name, long long type, sa::map_unit_t* unit, const QString& freeName = "", long long modelid = -1);

	[[nodiscard]] QString __fastcall getGround();

	bool __fastcall setTeamState(bool join);
	bool __fastcall kickteam(long long n);

	long long __fastcall setCharFaceToPoint(const QPoint& pos);
	bool __fastcall setCharFaceDirection(long long dir, bool noWindow = false);
	bool __fastcall setCharFaceDirection(const QString& dirStr, bool noWindow = false);

	[[nodiscard]] long long __fastcall getTeamSize();
	[[nodiscard]] QStringList __fastcall getJoinableUnitList() const;
	[[nodiscard]] bool __fastcall getItemIndexsByName(const QString& name, const QString& memo, QVector<long long>* pv,
		long long from = 0, long long to = sa::MAX_ITEM, QVector<long long>* pindexs = nullptr);
	[[nodiscard]] long long __fastcall getItemIndexByName(const QString& name, bool isExact = true, const QString& memo = "", long long from = 0, long long to = sa::MAX_ITEM);
	[[nodiscard]] long long __fastcall getPetSkillIndexByName(long long& petIndex, const QString& name) const;
	[[nodiscard]] long long __fastcall getSkillIndexByName(const QString& name) const;
	[[nodiscard]] bool __fastcall getPetIndexsByName(const QString& name, QVector<long long>* pv) const;
	[[nodiscard]] long long __fastcall getMagicIndexByName(const QString& name, bool isExact = true) const;
	[[nodiscard]] long long __fastcall getItemEmptySpotIndex();
	bool __fastcall getItemEmptySpotIndexs(QVector<long long>* pv);
	void __fastcall clear();

	[[nodiscard]] bool __fastcall checkCharMp(long long cmpvalue, long long* target = nullptr, bool useequal = false) const;
	[[nodiscard]] bool __fastcall checkCharHp(long long cmpvalue, long long* target = nullptr, bool useequal = false) const;
	[[nodiscard]] bool __fastcall checkRideHp(long long cmpvalue, long long* target = nullptr, bool useequal = false) const;
	[[nodiscard]] bool __fastcall checkPetHp(long long cmpvalue, long long* target = nullptr, bool useequal = false) const;
	[[nodiscard]] bool __fastcall checkTeammateHp(long long cmpvalue, long long* target) const;

	[[nodiscard]] bool __fastcall isPetSpotEmpty() const;
	[[nodiscard]] QVector<long long> __fastcall checkJobDailyState(const QString& missionName, long long timeout);

	[[nodiscard]] bool __fastcall isDialogVisible() const;

	bool __fastcall setCharFreeName(const QString& name);
	bool __fastcall setPetFreeName(long long petIndex, const QString& name);

	[[nodiscard]] bool __fastcall getBattleFlag();
	[[nodiscard]] bool __fastcall getOnlineFlag();

	void __fastcall sortItem();

	[[nodiscard]] long long __fastcall getDir();
	[[nodiscard]] QPoint __fastcall getPoint();
	void __fastcall setPoint(const QPoint& pos);

	[[nodiscard]] long long __fastcall getFloor();

	[[nodiscard]] QString __fastcall getFloorName();
	//battle
	bool __fastcall sendBattleCharAttackAct(long long target);
	bool __fastcall sendBattleCharMagicAct(long long magicIndex, long long target);
	bool __fastcall sendBattleCharJobSkillAct(long long skillIndex, long long target);
	bool __fastcall sendBattleCharItemAct(long long itemIndex, long long target);
	bool __fastcall sendBattleCharDefenseAct();
	bool __fastcall sendBattleCharEscapeAct();
	bool __fastcall sendBattleCharCatchPetAct(long long petIndex);
	bool __fastcall sendBattleCharSwitchPetAct(long long petIndex);
	bool __fastcall sendBattleCharDoNothing();
	bool __fastcall sendBattlePetSkillAct(long long skillIndex, long long target);
	bool __fastcall sendBattlePetDoNothing();
	void __fastcall setBattleEnd();

	void updateBattleTimeInfo();

	inline [[nodiscard]] sa::character_t __fastcall getCharacter() const { return character_.get(); }
	inline void __fastcall setCharacter(sa::character_t pc) { character_.set(pc); }

	inline [[nodiscard]] sa::magic_t __fastcall getMagic(long long magicIndex) const { return magic_.value(magicIndex); }

	inline [[nodiscard]] sa::profession_skill_t __fastcall getSkill(long long skillIndex) const { return professionSkill_.value(skillIndex); }
	inline [[nodiscard]] QHash<long long, sa::profession_skill_t> __fastcall getSkills() const { return professionSkill_.toHash(); }

	inline [[nodiscard]] sa::pet_t __fastcall getPet(long long petIndex) const { QReadLocker locker(&petInfoLock_);  return pet_.value(petIndex); }
	inline [[nodiscard]] QHash<long long, sa::pet_t> __fastcall getPets() const { QReadLocker locker(&petInfoLock_);  return pet_.toHash(); }
	inline [[nodiscard]] long long __fastcall getPetSize() const
	{
		QReadLocker locker(&petInfoLock_);
		long long n = 0;
		QHash<long long, sa::pet_t> pets = pet_.toHash();
		for (const sa::pet_t& it : pets)
		{
			if (it.level > 0 && it.valid && it.maxHp > 0)
				++n;
		}
		return n;
	}

	inline [[nodiscard]] sa::item_t __fastcall getItem(long long index) const { QReadLocker locker(&itemInfoLock_);  return item_.value(index); }
	inline [[nodiscard]] QHash<long long, sa::item_t> __fastcall getItems() const { QReadLocker locker(&itemInfoLock_);  return item_.toHash(); }
	inline void setItems(const QHash<long long, sa::item_t>& items) { QWriteLocker locker(&itemInfoLock_);  item_ = items; }

	inline [[nodiscard]] sa::pet_skill_t __fastcall getPetSkill(long long petIndex, long long skillIndex) const { return petSkill_.value(petIndex).value(skillIndex); }
	inline [[nodiscard]] QHash<long long, sa::pet_skill_t> __fastcall getPetSkills(long long petIndex) const { return petSkill_.value(petIndex); }
	inline void __fastcall setPetSkills(long long petIndex, const QHash<long long, sa::pet_skill_t>& skills) { petSkill_.insert(petIndex, skills); }
	inline void __fastcall setPetSkill(long long petIndex, long long skillIndex, const sa::pet_skill_t& skill)
	{
		QHash<long long, sa::pet_skill_t> skills = petSkill_.value(petIndex);
		skills.insert(skillIndex, skill);
		petSkill_.insert(petIndex, skills);
	}

	inline [[nodiscard]] sa::team_t __fastcall getTeam(long long teamIndex) const { return team_.value(teamIndex); }
	inline [[nodiscard]] QHash<long long, sa::team_t> __fastcall getTeams() const { return team_.toHash(); }


	inline [[nodiscard]] sa::item_t __fastcall getPetEquip(long long petIndex, long long equipIndex) const { return petItem_.value(petIndex).value(equipIndex); }
	inline [[nodiscard]] QHash<long long, sa::item_t> __fastcall getPetEquips(long long petIndex) const { return petItem_.value(petIndex); }

	inline [[nodiscard]] sa::address_bool_t __fastcall getAddressBook(long long index) const { return addressBook_.value(index); }
	inline [[nodiscard]] QHash<long long, sa::address_bool_t> __fastcall getAddressBooks() const { return addressBook_.toHash(); }

	inline [[nodiscard]] sa::battle_data_t __fastcall getBattleData() const { return battleData_.get(); }
	inline [[nodiscard]] sa::mission_data_t __fastcall getMissionInfo(long long index) const { return missionInfo_.value(index); }
	inline [[nodiscard]] QHash<long long, sa::mission_data_t> __fastcall getMissionInfos() const { return missionInfo_.toHash(); }
	inline [[nodiscard]] sa::char_list_data_t __fastcall getCharListTable(long long index) const { return charListData_.value(index); }
	inline [[nodiscard]] sa::mail_history_t __fastcall getMailHistory(long long index) const { return mailHistory_.value(index); }

	sa::SATime getSaTime() const;

	[[nodiscard]] long long __fastcall findInjuriedAllie() const;

	void __fastcall refreshItemInfo();

	void __fastcall updateComboBoxList() const;

	bool __fastcall setWindowTitle(QString formatStr);

	void addNetQueue(const QByteArray& data) { readQueue_.enqueue(data); }
private:
	bool __fastcall setCharModelDirection(long long dir) const;

	void __fastcall refreshItemInfo(long long index);

	void __fastcall setBattleFlag(bool enable);
	void __fastcall setOnlineFlag(bool enable);

	void __fastcall getCharMaxCarryingCapacity();
	inline [[nodiscard]] constexpr bool __fastcall isItemStackable(long long flg) { return ((flg >> 2) & 1); }
	QString __fastcall getAreaString(long long target);
	[[nodiscard]] bool __fastcall matchPetNameByIndex(long long index, const QString& name);
	[[nodiscard]] long long __fastcall getProfessionSkillIndexByName(const QString& names) const;

#pragma region BattleFunctions
	enum BattleScriptType
	{
		kCharScript,
		kPetScript,
	};
	bool __fastcall runBattleLua(BattleScriptType script);

	long long __fastcall playerDoBattleWork(const sa::battle_data_t& bt);
	bool __fastcall handleCharBattleLogics(const sa::battle_data_t& bt);
	long long __fastcall petDoBattleWork(const sa::battle_data_t& bt);
	bool __fastcall handlePetBattleLogics(const sa::battle_data_t& bt);

	bool __fastcall isCharMpEnoughForMagic(long long magicIndex) const;
	bool __fastcall isCharMpEnoughForSkill(long long magicIndex) const;
	bool __fastcall isCharHpEnoughForSkill(long long magicIndex) const;

	void __fastcall sortBattleUnit(QVector<sa::battle_object_t>& v) const;

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
	bool __fastcall matchBattleTarget(const QVector<sa::battle_object_t>& btobjs, BattleMatchType matchtype, long long firstMatchPos, QString op, QVariant cmpvar, long long* ppos);
	bool __fastcall conditionMatchTarget(QVector<sa::battle_object_t> btobjs, const QString& conditionStr, long long* ppos);

	[[nodiscard]] long long __fastcall getBattleSelectableEnemyTarget(const sa::battle_data_t& bt) const;

	[[nodiscard]] long long __fastcall getBattleSelectableEnemyOneRowTarget(const sa::battle_data_t& bt, bool front) const;

	[[nodiscard]] long long __fastcall getBattleSelectableAllieTarget(const sa::battle_data_t& bt) const;

	[[nodiscard]] bool __fastcall matchBattleEnemyByName(const QString& name, bool isExact, const QVector<sa::battle_object_t>& src, QVector<sa::battle_object_t>* v) const;
	[[nodiscard]] bool __fastcall matchBattleEnemyByLevel(long long level, const QVector<sa::battle_object_t>& src, QVector<sa::battle_object_t>* v) const;
	[[nodiscard]] bool __fastcall matchBattleEnemyByMaxHp(long long  maxHp, const QVector<sa::battle_object_t>& src, QVector<sa::battle_object_t>* v) const;

	[[nodiscard]] long long __fastcall getGetPetSkillIndexByName(long long etIndex, const QString& name) const;

	bool __fastcall fixCharTargetByMagicIndex(long long magicIndex, long long oldtarget, long long* target) const;
	bool __fastcall fixCharTargetBySkillIndex(long long magicIndex, long long oldtarget, long long* target) const;
	bool __fastcall fixCharTargetByItemIndex(long long itemIndex, long long oldtarget, long long* target) const;
	bool __fastcall fixPetTargetBySkillIndex(long long skillIndex, long long oldtarget, long long* target) const;
	void __fastcall updateCurrentSideRange(sa::battle_data_t* bt) const;
	bool __fastcall checkFlagState(long long pos) const;

	inline void __fastcall setBattleData(const sa::battle_data_t& data) { battleData_.set(data); }

	//自動鎖寵
	void __fastcall checkAutoLockPet(); //Async concurrent, DO NOT change calling convention

	//自動加點
	void checkAutoAbility(); //Async concurrent, DO NOT change calling convention

	//檢查並自動吃肉、或丟肉
	void __fastcall checkAutoDropMeat(); //Async concurrent, DO NOT change calling convention

	//自動吃經驗加乘道具
	void __fastcall checkAutoEatBoostExpItem();

	//自動丟棄道具
	void __fastcall checkAutoDropItems();

	//自動丟寵
	void __fastcall checkAutoDropPet(); //Async concurrent, DO NOT change calling convention
#pragma endregion

#pragma region SAClientOriginal
	//StoneAge Client Original Functions

	void __fastcall swapItemLocal(long long from, long long to);

	void __fastcall realTimeToSATime(sa::ls_time_t* lstime);

#pragma endregion

private: //lockers
	mutable QReadWriteLock petInfoLock_;
	mutable QReadWriteLock itemInfoLock_;
	mutable QMutex moveLock_;

private:
	safe::flag isBattle_ = false;//是否在戰鬥中
	safe::flag isOnline_ = false;//是否在線上
	safe::flag waitfor_C_recv_ = false;//等待接收C封包
	safe::flag doNotChangeTitle_ = false; //不要更改窗口標題

	util::timer eoTTLTimer_;//伺服器響應時間(MS)
	util::timer connectingTimer_;//登入連接時間(MS)
	safe::flag petEnableEscapeForTemp_ = false;//寵物臨時設置逃跑模式(觸發調用DoNothing)
	safe::integer tempCatchPetTargetIndex_ = -1;//臨時捕捉寵物目標索引

	safe::data<sa::battle_data_t> battleData_; //戰鬥數據

	safe::flag IS_WAITFOT_SKUP_RECV_ = false; //等待接收SKUP封包
	QFuture<void> skupFuture_; //自動加點線程管理器

	safe::flag IS_LOCKATTACK_ESCAPE_DISABLE_ = false;//鎖定攻擊不逃跑 (轉指定攻擊)

	safe::flag battleBackupThreadFlag_ = false;//戰鬥動作備用線程標誌

	safe::data<sa::character_t> character_ = {};

	safe::hash<long long, sa::team_t> team_ = {};
	safe::hash<long long, sa::item_t> item_ = {};
	safe::hash<long long, QHash<long long, sa::item_t>> petItem_ = {};
	safe::hash<long long, sa::pet_t> pet_ = {};
	safe::hash<long long, sa::magic_t> magic_ = {};
	safe::hash<long long, sa::address_bool_t> addressBook_ = {};
	safe::hash<long long, sa::mission_data_t> missionInfo_ = {};
	safe::hash<long long, sa::char_list_data_t> charListData_ = {};
	safe::hash<long long, sa::mail_history_t> mailHistory_ = {};
	safe::hash<long long, sa::profession_skill_t> professionSkill_ = {};
	safe::hash<long long, QHash<long long, sa::pet_skill_t>> petSkill_ = {};

	enum
	{
		kItemUnableSort = -1,
		kItemFirstSort = 0,
		kItemEnableSort = 1,
	};
	QHash<QString, long long> itemStackFlagHash_ = {};
	QHash<QString, long long> itemMaxStackHash_ = {};
	QHash<QString, long long> itemTryStackHash_ = {};

	safe::vector<bool> battlePetDisableList_ = {};

	safe::data<QString> nowFloorName_; //當前地圖名稱
	safe::integer nowFloor_; //當前地圖編號
	safe::data<QPoint> nowPoint_; //當前人物座標

	QFuture<void> battleBackupFuture_; //戰鬥動作備用線程管理器

	QString battleCharLuaScript_;
	QString battlePetLuaScript_;
	QString battleCharLuaScriptPath_;
	QString battlePetLuaScriptPath_;

	//client original 目前很多都是沒用處的
#pragma region ClientOriginal
	safe::data<QString> lastSecretChatName_;//最後一次收到密語的發送方名稱

	//遊戲內當前時間相關
	sa::ls_time_t saTimeStruct_ = {};
	safe::integer serverTime_ = 0LL;
	safe::integer firstServerTime_ = 0LL;

	//交易相關
	long long opp_showindex = 0;
	QString opp_sockfd;
	QString opp_name;
	QString opp_goldmount;
	long long showindex[7] = {};
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
	sa::pet_t tradePet[2] = {};
	sa::show_item_t opp_item[sa::MAX_MAXHAVEITEM];	//交易道具阵列增为15个
	sa::show_pet_t opp_pet[sa::MAX_PET];
	QStringList myitem_tradeList;
	QStringList mypet_tradeList = { "P|-1", "P|-1", "P|-1" , "P|-1", "P|-1" };
	long long mygoldtrade = 0;

	void __fastcall clearTradeData()
	{
		opp_showindex = 0;
		opp_sockfd.clear();
		opp_name.clear();
		opp_goldmount.clear();
		tradeWndDropGoldGet = 0;
		opp_itemgraph.clear();
		opp_itemname.clear();
		opp_itemeffect.clear();
		opp_itemindex.clear();
		opp_itemdamage.clear();
		trade_kind.clear();
		trade_command.clear();
		tradeStatus = 0;
		tradePetIndex = -1;
		for (long long i = 0; i < 2; ++i)
			tradePet[i] = {};

		for (long long i = 0; i < sa::MAX_MAXHAVEITEM; ++i)
			opp_item[i] = {};

		for (long long i = 0; i < sa::MAX_PET; ++i)
			opp_pet[i] = {};

		myitem_tradeList.clear();
		mypet_tradeList = QStringList{ "P|-1", "P|-1", "P|-1" , "P|-1", "P|-1" };
		mygoldtrade = 0;
	}
#pragma endregion

public:
	//戰鬥相關
	safe::flag battleCharAlreadyActed = true; //戰鬥人物已經動作
	safe::flag battlePetAlreadyActed = true; //戰鬥寵物已經動作
	safe::integer battleCharCurrentPos = 0; //戰鬥人物當前位置
	safe::integer battleBpFlag = 0; //戰鬥BP標誌
	safe::integer battleField = 0; //戰鬥場地
	safe::flag battleCharEscapeFlag = 0; //戰鬥人物退戰標誌
	safe::integer battleCharCurrentMp = 0; //戰鬥人物當前MP
	safe::integer battleCurrentAnimeFlag = 0; //戰鬥當前動畫標誌
	std::unique_ptr<CLua> battleLua = nullptr; //戰鬥Lua腳本

	//custom
	safe::flag IS_TRADING = false;

	safe::flag IS_WAITFOR_missionInfo_FLAG = false;
	safe::flag IS_WAITFOR_BANK_FLAG = false;
	safe::flag IS_WAITFOR_DIALOG_FLAG = false;
	safe::flag IS_WAITFOR_CUSTOM_DIALOG_FLAG = false;
	safe::integer IS_WAITOFR_ITEM_CHANGE_PACKET = false;

	safe::flag isBattleDialogReady = false;
	safe::flag isEOTTLSend = false;
	safe::integer lastEOTime = 0;

	util::timer loginTimer;
	util::timer battleDurationTimer;
	util::timer normalDurationTimer;
	util::timer oneRoundDurationTimer;

	safe::integer battleCurrentRound = 0;
	safe::integer battle_total_time = 0;
	safe::integer battle_total = 0;
	safe::integer battle_one_round_time = 0;

	safe::integer saCurrentGameTime = 0;//遊戲時間 LSTimeSection

	safe::data<sa::currency_data_t> currencyData = {};
	safe::data<sa::custom_dialog_t> customDialog = {};

	safe::hash<long long, sa::map_unit_t> mapUnitHash;
	safe::hash<QPoint, sa::map_unit_t> npcUnitPointHash;
	safe::queue<QPair<long long, QString>> chatQueue;

	QPair<long long, QVector<sa::bank_pet_t>> currentBankPetList;
	safe::vector<sa::item_t> currentBankItemList;

	util::timer repTimer;
	sa::afk_record_data_t afkRecords[1 + sa::MAX_PET] = {};

	safe::data<sa::dialog_t> currentDialog = {};

	//用於緩存要發送到UI的數據(開啟子窗口初始化並加載當前最新數據時使用)
	safe::hash<long long, QVariant> playerInfoColContents;
	safe::hash<long long, QVariant> itemInfoRowContents;
	safe::hash<long long, QVariant> equipInfoRowContents;
	safe::data<QStringList> enemyNameListCache;
	safe::data<QString> timeLabelContents;
	safe::data<QString> labelCharAction;
	safe::data<QString> labelPetAction;

	MapDevice mapDevice;

private:
	enum NextBufferAction
	{
		kBufferNone,
		kBufferNeedToClean,
		kContinueAppendBuffer,
		kBufferAboutToEnd,
		kInvalidBuffer,
	};

	char netDataBuffer_[NETDATASIZE] = {};
	char netResultDataBuffer_[SBUFSIZE] = {};
	QByteArrayList netDataArrayList_ = {};
	QByteArray netReadBufferArray_;
	QByteArray netRawBufferArray_;
	safe::queue<QByteArray> readQueue_; //接收來自客戶端的數據隊列
private://lssproto
	long long __fastcall appendReadBuf(const QByteArray& data);
	bool __fastcall splitLinesFromReadBuf(QByteArrayList& list);
	long long __fastcall a62toi(const QString& a) const;
	long long __fastcall getStringToken(const QString& src, const QString& delim, long long count, QString& out) const;
	long long __fastcall getIntegerToken(const QString& src, const QString& delim, long long count) const;
	long long __fastcall getInteger62Token(const QString& src, const QString& delim, long long count) const;
	void __fastcall makeStringFromEscaped(QString& src) const;

private://lssproto_recv
#pragma region Lssproto_Recv
	virtual void __fastcall lssproto_XYD_recv(const QPoint& pos, long long dir) override;//戰鬥結束後的大地圖座標
	virtual void __fastcall lssproto_EV_recv(long long dialogid, long long result) override;
	virtual void __fastcall lssproto_EN_recv(long long result, long long field) override;
	virtual void __fastcall lssproto_RS_recv(char* data) override;
	virtual void __fastcall lssproto_RD_recv(char* data) override;
	virtual void __fastcall lssproto_B_recv(char* command) override;
	virtual void __fastcall lssproto_I_recv(char* data) override;
	virtual void __fastcall lssproto_SI_recv(long long fromindex, long long toindex) override;
	virtual void __fastcall lssproto_MSG_recv(long long aindex, char* text, long long color) override;	//收到普通郵件或寵物郵件
	virtual void __fastcall lssproto_PME_recv(long long unitid, long long graphicsno, const QPoint& pos, long long dir, long long flg, long long no, char* cdata) override;
	virtual void __fastcall lssproto_AB_recv(char* data) override;
	virtual void __fastcall lssproto_ABI_recv(long long num, char* data) override;
	virtual void __fastcall lssproto_TK_recv(long long index, char* message, long long color) override;
	virtual void __fastcall lssproto_MC_recv(long long fl, long long x1, long long y1, long long x2, long long y2, long long tilesum, long long objsum, long long eventsum, char* data) override;
	virtual void __fastcall lssproto_M_recv(long long fl, long long x1, long long y1, long long x2, long long y2, char* data) override;
	virtual void __fastcall lssproto_C_recv(char* data) override;
	virtual void __fastcall lssproto_CA_recv(char* data) override;
	virtual void __fastcall lssproto_CD_recv(char* data) override;
	virtual void __fastcall lssproto_R_recv(char* data) override;
	virtual void __fastcall lssproto_S_recv(char* data) override;
	virtual void __fastcall lssproto_D_recv(long long category, long long dx, long long dy, char* data) override;
	virtual void __fastcall lssproto_FS_recv(long long flg) override;
	virtual void __fastcall lssproto_HL_recv(long long flg) override;//戰鬥中是否要Help
	virtual void __fastcall lssproto_PR_recv(long long request, long long result) override;
	virtual void __fastcall lssproto_KS_recv(long long petarray, long long result) override;//指定那一只寵物出場戰鬥

	virtual void __fastcall lssproto_SPET_recv(long long standbypet, long long result) override;

	virtual void __fastcall lssproto_PS_recv(long long result, long long havepetindex, long long havepetskill, long long toindex) override;	//寵物合成
	virtual void __fastcall lssproto_SKUP_recv(long long point) override;//取得可加的屬性點數
	virtual void __fastcall lssproto_WN_recv(long long windowtype, long long buttontype, long long dialogid, long long unitid, char* data) override;
	virtual void __fastcall lssproto_EF_recv(long long effect, long long level, char* option) override;
	virtual void __fastcall lssproto_SE_recv(const QPoint& pos, long long senumber, long long sw) override;
	virtual void __fastcall lssproto_ClientLogin_recv(char* result) override;
	virtual void __fastcall lssproto_CreateNewChar_recv(char* result, char* data) override;
	virtual void __fastcall lssproto_CharDelete_recv(char* result, char* data) override;
	virtual void __fastcall lssproto_CharLogin_recv(char* result, char* data) override;
	virtual void __fastcall lssproto_CharList_recv(char* result, char* data) override;
	virtual void __fastcall lssproto_CharLogout_recv(char* result, char* data) override;
	virtual void __fastcall lssproto_ProcGet_recv(char* data) override;
	virtual void __fastcall lssproto_CharNumGet_recv(long long logincount, long long player) override;
	virtual void __fastcall lssproto_Echo_recv(char* test) override;
	virtual void __fastcall lssproto_NU_recv(long long AddCount) override;
	virtual void __fastcall lssproto_WO_recv(long long effect) override;//取得轉生的特效
	virtual void __fastcall lssproto_TD_recv(char* data) override;
	virtual void __fastcall lssproto_FM_recv(char* data) override;

	virtual void __fastcall lssproto_IC_recv(const QPoint& pos) override {};

	virtual void __fastcall lssproto_NC_recv(long long flg) override;

	virtual void __fastcall lssproto_CS_recv(long long deltimes) override;

	virtual void __fastcall lssproto_PETST_recv(long long petarray, long long result) override;
	//  聊天室頻道
	virtual void __fastcall lssproto_CHATROOM_recv(char* data) override {};

	//新增Protocol要求細項
	virtual void __fastcall lssproto_RESIST_recv(char* data) override {};

	virtual void __fastcall lssproto_ALCHEPLUS_recv(char* data) override {};
	//非戰鬥時技能Protocol
	virtual void __fastcall lssproto_BATTLESKILL_recv(char* data) override {};

	virtual void __fastcall lssproto_CHAREFFECT_recv(char* data) override;

	virtual void __fastcall lssproto_STREET_VENDOR_recv(char* data) override {};	// 擺攤功能

	virtual void __fastcall lssproto_missionInfo_recv(char* data) override;

	virtual void __fastcall lssproto_FamilyBadge_recv(char* data) override {};

	virtual void __fastcall lssproto_TEACHER_SYSTEM_recv(char* data) override;

	virtual void __fastcall lssproto_S2_recv(char* data) override;

	virtual void __fastcall lssproto_Firework_recv(long long nCharaindex, long long nType, long long nActionNum) override;	// 煙火功能

	virtual void __fastcall lssproto_TheaterData_recv(char* pData) override {};

	virtual void __fastcall lssproto_MoveScreen_recv(BOOL bMoveScreenMode, long long iXY) override {};	// client 移動熒幕

	virtual void __fastcall lssproto_MagiccardAction_recv(char* data) override {};	//魔法牌功能
	virtual void __fastcall lssproto_MagiccardDamage_recv(long long position, long long damage, long long offsetx, long long offsety) override {};

	virtual void __fastcall lssproto_DancemanOption_recv(long long option) override {};	//動一動狀態

	virtual void __fastcall lssproto_DENGON_recv(char* data, long long colors, long long nums) override;

	virtual void __fastcall lssproto_hundredkill_recv(long long flag) override {};

	virtual void __fastcall lssproto_pkList_recv(long long count, char* data) override {};

	virtual void __fastcall lssproto_PetSkins_recv(char* data) override {};

	//////////////////////////////////

	virtual bool __fastcall lssproto_CustomWN_recv(const QString& data) override;
	virtual bool __fastcall lssproto_CustomTK_recv(const QString& data) override;
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
