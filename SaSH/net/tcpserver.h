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

import Safe;
import Global;

#pragma once
#include <atomic>
#include <QTcpServer>
#include <QSharedPointer>
#include <QTcpSocket>
#include <QFuture>
#include <QElapsedTimer>
#include <QPair>
#include <QMutex>
#include <QVariant>
#include <threadplugin.h>
#include <indexer.h>
#include "lssproto.h"

static const QHash<QString, BUTTON_TYPE> buttonMap = {
	{"OK", BUTTON_OK},
	{"CANCEL", BUTTON_CANCEL},
	{"YES", BUTTON_YES},
	{"NO", BUTTON_NO},
	{"PREVIOUS", BUTTON_PREVIOUS},
	{"NEXT", BUTTON_NEXT},
	{"AUTO", BUTTON_AUTO},
	{"CLOSE", BUTTON_CLOSE },

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
	{ "關閉", BUTTON_CLOSE },

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
	{ "关闭", BUTTON_CLOSE },
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
	QHash<__int64, QString> g_dirStrHash = {
		{ 0, QObject::tr("North") },
		{ 1, QObject::tr("ENorth") },
		{ 2, QObject::tr("East") },
		{ 3, QObject::tr("ESouth") },
		{ 4, QObject::tr("South") },
		{ 5, QObject::tr("WSouth") },
		{ 6, QObject::tr("West") },
		{ 7, QObject::tr("WNorth") },
	};

	explicit Server(__int64 index, QObject* parent);

	virtual ~Server();

	[[nodiscard]] inline bool isListening() const { return  !server_.isNull() && server_->isListening(); }

	[[nodiscard]] inline bool hasClientExist() const { return  !server_.isNull() && !clientSockets_.isEmpty(); }

	bool start(QObject* parent);

	[[nodiscard]] inline unsigned short getPort() const { return port_.load(std::memory_order_acquire); }

signals:
	void write(QTcpSocket* clientSocket, QByteArray ba, __int64 size);

private slots:
	void onWrite(QTcpSocket* clientSocket, QByteArray ba, __int64 size);
	void onNewConnection();
	void onClientReadyRead();

private:
	__int64 dispatchMessage(const QByteArray& encoded);

	bool handleCustomMessage(const QByteArray& data);

	Q_INVOKABLE void handleData(QByteArray data);

public://actions
	[[nodiscard]] __int64 getWorldStatus();

	[[nodiscard]] __int64 getGameStatus();

	[[nodiscard]] bool checkWG(__int64  w, __int64 g);

	[[nodiscard]] __int64 getUnloginStatus();
	void setWorldStatus(__int64 w);
	void setGameStatus(__int64 g);

	bool login(__int64 s);

	void clientLogin(const QString& userName, const QString& password);
	void playerLogin(__int64 index);

	QString getBadStatusString(__int64 status);

	QString getFieldString(__int64 field);

	void unlockSecurityCode(const QString& code);

	void clearNetBuffer();

	void logOut();

	void logBack();

	void move(const QPoint& p, const QString& dir);

	void move(const QPoint& p);

	void announce(const QString& msg, __int64 color = 4);

	void createCharacter(__int64 dataplacenum
		, const QString& charname
		, __int64 imgno
		, __int64 faceimgno
		, __int64 vit
		, __int64 str
		, __int64 tgh
		, __int64 dex
		, __int64 earth
		, __int64 water
		, __int64 fire
		, __int64 wind
		, __int64 hometown
		, bool forcecover);

	void deleteCharacter(__int64 index, const QString securityCode, bool backtofirst = false);

	void talk(const QString& text, __int64 color = 0, TalkMode mode = kTalkNormal);
	void inputtext(const QString& text, __int64 dialogid = -1, __int64 npcid = -1);

	void windowPacket(const QString& command, __int64 dialogid, __int64 npcid);

	void EO();

	void dropItem(__int64 index);
	void dropItem(QVector<__int64> index);

	void useItem(__int64 itemIndex, __int64 target);


	void swapItem(__int64 from, __int64 to);

	void petitemswap(__int64 petIndex, __int64 from, __int64 to);

	void useMagic(__int64 magicIndex, __int64 target);

	void dropPet(__int64 petIndex);

	void setSwitcher(__int64 flg, bool enable);

	void setSwitcher(__int64 flg);

	void press(BUTTON_TYPE select, __int64 dialogid = -1, __int64 unitid = -1);
	void press(__int64 row, __int64 dialogid = -1, __int64 unitid = -1);

	void buy(__int64 index, __int64 amt, __int64 dialogid = -1, __int64 unitid = -1);
	void sell(const QVector<__int64>& indexs, __int64 dialogid = -1, __int64 unitid = -1);
	void sell(__int64 index, __int64 dialogid = -1, __int64 unitid = -1);
	void sell(const QString& name, const QString& memo = "", __int64 dialogid = -1, __int64 unitid = -1);
	void learn(__int64 skillIndex, __int64 petIndex, __int64 spot, __int64 dialogid = -1, __int64 unitid = -1);

	void craft(CraftType type, const QStringList& ingres);

	void createRemoteDialog(quint64 type, quint64 button, const QString& text);

	void mail(const QVariant& card, const QString& text, __int64 petIndex, const QString& itemName, const QString& itemMemo);

	void warp();

	void shopOk(__int64 n);
	void saMenu(__int64 n);

	bool addPoint(__int64 skillid, __int64 amt);

	void pickItem(__int64 dir);

	void dropGold(__int64 gold);

	void depositGold(__int64 gold, bool isPublic);
	void withdrawGold(__int64 gold, bool isPublic);

	void depositPet(__int64 petIndex, __int64 dialogid = -1, __int64 unitid = -1);
	void withdrawPet(__int64 petIndex, __int64 dialogid = -1, __int64 unitid = -1);

	void depositItem(__int64 index, __int64 dialogid = -1, __int64 unitid = -1);
	void withdrawItem(__int64 itemIndex, __int64 dialogid = -1, __int64 unitid = -1);

	bool captchaOCR(QString* pmsg);

	void setAllPetState();
	void setPetState(__int64 petIndex, PetState state);
	void setFightPet(__int64 petIndex);
	void setRidePet(__int64 petIndex);
	void setPetStateSub(__int64 petIndex, __int64 state);
	void setPetStandby(__int64 petIndex, __int64 state);

	void updateItemByMemory();
	void updateDatasFromMemory();

	void doBattleWork(bool waitforBA);
	void asyncBattleAction(bool waitforBA);

	void downloadMap(__int64 floor = -1);
	void downloadMap(__int64 x, __int64 y, __int64 floor = -1);

	bool tradeStart(const QString& name, __int64 timeout);
	void tradeComfirm(const QString& name);
	void tradeCancel();
	void tradeAppendItems(const QString& name, const QVector<__int64>& itemIndexs);
	void tradeAppendGold(const QString& name, __int64 gold);
	void tradeAppendPets(const QString& name, const QVector<__int64>& petIndex);
	void tradeComplete(const QString& name);

	void cleanChatHistory();
	[[nodiscard]] QString getChatHistory(__int64 index);

	bool findUnit(const QString& name, __int64 type, mapunit_t* unit, const QString& freeName = "", __int64 modelid = -1);

	[[nodiscard]] QString getGround();

	void setTeamState(bool join);
	void kickteam(__int64 n);

	__int64 setCharFaceToPoint(const QPoint& pos);
	void setCharFaceDirection(__int64 dir);
	void setCharFaceDirection(const QString& dirStr);

	[[nodiscard]] __int64 getPartySize() const;
	[[nodiscard]] QStringList getJoinableUnitList() const;
	[[nodiscard]] bool getItemIndexsByName(const QString& name, const QString& memo, QVector<__int64>* pv, __int64 from = 0, __int64 to = MAX_ITEM);
	[[nodiscard]] __int64 getItemIndexByName(const QString& name, bool isExact = true, const QString& memo = "", __int64 from = 0, __int64 to = MAX_ITEM);
	[[nodiscard]] __int64 getPetSkillIndexByName(__int64& petIndex, const QString& name) const;
	[[nodiscard]] __int64 getSkillIndexByName(const QString& name) const;
	[[nodiscard]] bool getPetIndexsByName(const QString& name, QVector<__int64>* pv) const;
	[[nodiscard]] __int64 getMagicIndexByName(const QString& name, bool isExact = true) const;
	[[nodiscard]] __int64 getItemEmptySpotIndex();
	bool getItemEmptySpotIndexs(QVector<__int64>* pv);
	void clear();

	[[nodiscard]] bool checkCharMp(__int64 cmpvalue, __int64* target = nullptr, bool useequal = false);
	[[nodiscard]] bool checkCharHp(__int64 cmpvalue, __int64* target = nullptr, bool useequal = false);
	[[nodiscard]] bool checkRideHp(__int64 cmpvalue, __int64* target = nullptr, bool useequal = false);
	[[nodiscard]] bool checkPetHp(__int64 cmpvalue, __int64* target = nullptr, bool useequal = false);
	[[nodiscard]] bool checkPartyHp(__int64 cmpvalue, __int64* target);

	[[nodiscard]] bool isPetSpotEmpty() const;
	[[nodiscard]] __int64 checkJobDailyState(const QString& missionName);

	[[nodiscard]] bool isDialogVisible();

	void setCharFreeName(const QString& name);
	void setPetFreeName(__int64 petIndex, const QString& name);

	[[nodiscard]] bool getBattleFlag();
	[[nodiscard]] bool getOnlineFlag() const;

	void sortItem(bool deepSort = false);

	[[nodiscard]] __int64 getDir();
	[[nodiscard]] QPoint getPoint();
	void setPoint(const QPoint& pos);

	[[nodiscard]] __int64 getFloor();
	void setFloor(__int64 floor);

	[[nodiscard]] QString getFloorName();
	//battle
	void sendBattleCharAttackAct(__int64 target);
	void sendBattleCharMagicAct(__int64 magicIndex, __int64 target);
	void sendBattleCharJobSkillAct(__int64 skillIndex, __int64 target);
	void sendBattleCharItemAct(__int64 itemIndex, __int64 target);
	void sendBattleCharDefenseAct();
	void sendBattleCharEscapeAct();
	void sendBattleCharCatchPetAct(__int64 petIndex);
	void sendBattleCharSwitchPetAct(__int64 petIndex);
	void sendBattleCharDoNothing();
	void sendBattlePetSkillAct(__int64 skillIndex, __int64 target);
	void sendBattlePetDoNothing();
	void setBattleEnd();

	void updateBattleTimeInfo();

	inline [[nodiscard]] PC getPC() const { QReadLocker locker(&charInfoLock_); return pc_; }
	inline void setPC(PC pc) { pc_ = pc; }

	inline [[nodiscard]] MAGIC getMagic(__int64 magicIndex) const { return magic_.value(magicIndex); }

	inline [[nodiscard]] PROFESSION_SKILL getSkill(__int64 skillIndex) const { QReadLocker locker(&charSkillInfoLock_); return profession_skill_.value(skillIndex); }
	inline [[nodiscard]] QHash<__int64, PROFESSION_SKILL> getSkills() const { QReadLocker locker(&charSkillInfoLock_); return profession_skill_.toHash(); }

	inline [[nodiscard]] PET getPet(__int64 petIndex) const { QReadLocker locker(&petInfoLock_); return pet_.value(petIndex); }
	inline [[nodiscard]] QHash<__int64, PET> getPets() const { QReadLocker locker(&petInfoLock_); return pet_.toHash(); }
	inline [[nodiscard]] __int64 getPetSize() const
	{
		QReadLocker locker(&petInfoLock_);
		__int64 n = 0;
		QHash<__int64, PET> pets = pet_.toHash();
		for (const PET& it : pets)
		{
			if (it.level > 0 && it.valid && it.maxHp > 0)
				++n;
		}
		return n;
	}

	inline [[nodiscard]] ITEM getItem(__int64 index) const { QReadLocker locker(&itemInfoLock_); return item_.value(index); }
	inline [[nodiscard]] QHash<__int64, ITEM> getItems() const { QReadLocker locker(&itemInfoLock_); return item_.toHash(); }

	inline [[nodiscard]] PET_SKILL getPetSkill(__int64 petIndex, __int64 skillIndex) const { QReadLocker locker(&petSkillInfoLock_); return petSkill_.value(petIndex).value(skillIndex); }
	inline [[nodiscard]] QHash<__int64, PET_SKILL> getPetSkills(__int64 petIndex) const { QReadLocker locker(&petSkillInfoLock_); return petSkill_.value(petIndex); }
	inline void setPetSkills(__int64 petIndex, const QHash<__int64, PET_SKILL>& skills) { petSkill_.insert(petIndex, skills); }
	inline void setPetSkill(__int64 petIndex, __int64 skillIndex, const PET_SKILL& skill)
	{
		QHash<__int64, PET_SKILL> skills = petSkill_.value(petIndex);
		skills.insert(skillIndex, skill);
		petSkill_.insert(petIndex, skills);
	}

	inline [[nodiscard]] PARTY getParty(__int64 partyIndex) const { QReadLocker locker(&teamInfoLock_); return party_.value(partyIndex); }
	inline [[nodiscard]] QHash<__int64, PARTY> getParties() const { QReadLocker locker(&teamInfoLock_);  return party_.toHash(); }

	inline [[nodiscard]] ITEM getPetEquip(__int64 petIndex, __int64 equipIndex) const { QReadLocker locker(&petEquipInfoLock_); return petItem_.value(petIndex).value(equipIndex); }
	inline [[nodiscard]] QHash<__int64, ITEM> getPetEquips(__int64 petIndex) const { QReadLocker locker(&petEquipInfoLock_); return petItem_.value(petIndex); }

	inline [[nodiscard]] ADDRESS_BOOK getAddressBook(__int64 index) const { return addressBook_.value(index); }
	inline [[nodiscard]] QHash<__int64, ADDRESS_BOOK> getAddressBooks() const { return addressBook_.toHash(); }

	inline [[nodiscard]] battledata_t getBattleData() const { return battleData.get(); }
	inline [[nodiscard]] JOBDAILY getJobDaily(__int64 index) const { return jobdaily_.value(index); }
	inline [[nodiscard]] QHash<__int64, JOBDAILY> getJobDailys() const { return jobdaily_.toHash(); }
	inline [[nodiscard]] CHARLISTTABLE getCharListTable(__int64 index) const { return chartable_.value(index); }
	inline [[nodiscard]] MAIL_HISTORY getMailHistory(__int64 index) const { return mailHistory_.value(index); }

	[[nodiscard]] __int64 findInjuriedAllie();

	void refreshItemInfo();

	void updateComboBoxList();

private:
	void setWindowTitle();
	void refreshItemInfo(__int64 index);

	void setBattleFlag(bool enable);
	void setOnlineFlag(bool enable);

	void getCharMaxCarryingCapacity();
	inline [[nodiscard]] constexpr bool isItemStackable(__int64 flg) { return ((flg >> 2) & 1); }
	QString getAreaString(__int64 target);
	[[nodiscard]] bool matchPetNameByIndex(__int64 index, const QString& name);
	[[nodiscard]] __int64 getProfessionSkillIndexByName(const QString& names) const;

#pragma region BattleFunctions
	__int64 playerDoBattleWork(const battledata_t& bt);
	void handleCharBattleLogics(const battledata_t& bt);
	__int64 petDoBattleWork(const battledata_t& bt);
	void handlePetBattleLogics(const battledata_t& bt);

	bool isCharMpEnoughForMagic(__int64 magicIndex) const;
	bool isCharMpEnoughForSkill(__int64 magicIndex) const;
	bool isCharHpEnoughForSkill(__int64 magicIndex) const;

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
	bool matchBattleTarget(const QVector<battleobject_t>& btobjs, BattleMatchType matchtype, __int64 firstMatchPos, QString op, QVariant cmpvar, __int64* ppos);
	bool conditionMatchTarget(QVector<battleobject_t> btobjs, const QString& conditionStr, __int64* ppos);

	[[nodiscard]] __int64 getBattleSelectableEnemyTarget(const battledata_t& bt) const;

	[[nodiscard]] __int64 getBattleSelectableEnemyOneRowTarget(const battledata_t& bt, bool front) const;

	[[nodiscard]] __int64 getBattleSelectableAllieTarget(const battledata_t& bt) const;

	[[nodiscard]] bool matchBattleEnemyByName(const QString& name, bool isExact, const QVector<battleobject_t>& src, QVector<battleobject_t>* v) const;
	[[nodiscard]] bool matchBattleEnemyByLevel(__int64 level, const QVector<battleobject_t>& src, QVector<battleobject_t>* v) const;
	[[nodiscard]] bool matchBattleEnemyByMaxHp(__int64  maxHp, const QVector<battleobject_t>& src, QVector<battleobject_t>* v) const;

	[[nodiscard]] __int64 getGetPetSkillIndexByName(__int64 etIndex, const QString& name) const;

	bool fixCharTargetByMagicIndex(__int64 magicIndex, __int64 oldtarget, __int64* target) const;
	bool fixCharTargetBySkillIndex(__int64 magicIndex, __int64 oldtarget, __int64* target) const;
	bool fixCharTargetByItemIndex(__int64 itemIndex, __int64 oldtarget, __int64* target) const;
	bool fixPetTargetBySkillIndex(__int64 skillIndex, __int64 oldtarget, __int64* target) const;
	void updateCurrentSideRange(battledata_t& bt);
	bool checkFlagState(__int64 pos);

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

	void swapItemLocal(__int64 from, __int64 to);

	void realTimeToSATime(LSTIME* lstime);

#pragma endregion

private: //lockers
	mutable QReadWriteLock petInfoLock_;
	mutable QReadWriteLock petSkillInfoLock_;
	mutable QReadWriteLock charInfoLock_;
	mutable QReadWriteLock charSkillInfoLock_;
	mutable QReadWriteLock charMagicInfoLock_;
	mutable QReadWriteLock itemInfoLock_;
	mutable QReadWriteLock petEquipInfoLock_;
	mutable QReadWriteLock teamInfoLock_;

private:
	std::atomic_bool IS_BATTLE_FLAG = false;
	std::atomic_bool IS_ONLINE_FLAG = false;

	QElapsedTimer eottlTimer;//伺服器響應時間(MS)
	QElapsedTimer connectingTimer;
	std::atomic_bool petEscapeEnableTempFlag = false;
	__int64 tempCatchPetTargetIndex = -1;

	SafeData<battledata_t> battleData;

	std::atomic_bool IS_WAITFOT_SKUP_RECV = false;
	QFuture<void> skupFuture;

	std::atomic_bool IS_LOCKATTACK_ESCAPE_DISABLE = false;//鎖定攻擊不逃跑 (轉指定攻擊)

	PC pc_ = {};

	SafeHash<__int64, PARTY> party_ = {};
	SafeHash<__int64, ITEM> item_ = {};
	SafeHash<__int64, QHash<__int64, ITEM>> petItem_ = {};
	SafeHash<__int64, PET> pet_ = {};
	SafeHash<__int64, MAGIC> magic_ = {};
	SafeHash<__int64, ADDRESS_BOOK> addressBook_ = {};
	SafeHash<__int64, JOBDAILY> jobdaily_ = {};
	SafeHash<__int64, CHARLISTTABLE> chartable_ = {};
	SafeHash<__int64, MAIL_HISTORY> mailHistory_ = {};
	SafeHash<__int64, PROFESSION_SKILL> profession_skill_ = {};
	SafeHash<__int64, QHash<__int64, PET_SKILL>> petSkill_ = {};

	__int64 swapitemModeFlag = 0; //當前自動整理功能的階段
	QHash<QString, bool>itemStackFlagHash = {};

	SafeVector<bool> battlePetDisableList_ = {};

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
	__int64 serverTime = 0LL;
	__int64 FirstTime = 0LL;

	//交易相關
	__int64 opp_showindex = 0;
	QString opp_sockfd;
	QString opp_name;
	QString opp_goldmount;
	__int64 showindex[7] = { 0 };
	__int64 tradeWndDropGoldGet = 0;
	QString opp_itemgraph;
	QString opp_itemname;
	QString opp_itemeffect;
	QString opp_itemindex;
	QString opp_itemdamage;
	QString trade_kind;
	QString trade_command;
	__int64 tradeStatus = 0;
	__int64 tradePetIndex = -1;
	PET tradePet[2] = {};
	showitem opp_item[MAX_MAXHAVEITEM];	//交易道具阵列增为15个
	showpet opp_pet[MAX_PET];
	QStringList myitem_tradeList;
	QStringList mypet_tradeList = { "P|-1", "P|-1", "P|-1" , "P|-1", "P|-1" };
	__int64 mygoldtrade = 0;

	//郵件相關
	__int64 mailHistoryWndSelectNo = 0;
	__int64 mailHistoryWndPageNo = 0;
#pragma endregion

public:
	std::atomic_llong nowFloor_;
	SafeData<QString> nowFloorName_;
	SafeData<QPoint> nowPoint_;

	//custom
	std::atomic_bool IS_TRADING = false;
	std::atomic_bool IS_DISCONNECTED = false;
	std::atomic_bool IS_TCP_CONNECTION_OK_TO_USE = false;

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

	QSharedPointer<MapAnalyzer> mapAnalyzer;

	SafeData<currencydata_t> currencyData = {};
	SafeData<customdialog_t> customDialog = {};

	SafeHash<__int64, mapunit_t> mapUnitHash;
	SafeHash<QPoint, mapunit_t> npcUnitPointHash;
	SafeQueue<QPair<__int64, QString>> chatQueue;

	QPair<__int64, QVector<bankpet_t>> currentBankPetList;
	SafeVector<ITEM> currentBankItemList;

	QElapsedTimer repTimer;
	AfkRecorder recorder[1 + MAX_PET] = {};

	SafeData<dialog_t> currentDialog = {};

	//用於緩存要發送到UI的數據(開啟子窗口初始化並加載當前最新數據時使用)
	SafeHash<__int64, QVariant> playerInfoColContents;
	SafeHash<__int64, QVariant> itemInfoRowContents;
	SafeHash<__int64, QVariant> equipInfoRowContents;
	SafeData<QStringList> enemyNameListCache;
	SafeData<QVariant> topInfoContents;
	SafeData<QVariant> bottomInfoContents;
	SafeData<QString> timeLabelContents;
	SafeData<QString> labelCharAction;
	SafeData<QString> labelPetAction;

private:
	std::atomic_uint16_t port_ = 0;

	QSharedPointer<QTcpServer> server_;

	QList<QTcpSocket*> clientSockets_;

	QByteArray net_readbuf_;
	QByteArray net_raw_;

	QMutex net_mutex;

private://lssproto
	__int64 appendReadBuf(const QByteArray& data);
	QByteArrayList splitLinesFromReadBuf();
	__int64 a62toi(const QString& a) const;
	__int64 getStringToken(const QString& src, const QString& delim, __int64 count, QString& out) const;
	__int64 getIntegerToken(const QString& src, const QString& delim, __int64 count) const;
	__int64 getInteger62Token(const QString& src, const QString& delim, __int64 count) const;
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
