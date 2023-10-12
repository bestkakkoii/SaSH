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

import Utility;

#pragma once
#include "lexer.h"
#include <QStack>
#include <functional>

#include "threadplugin.h"
#include "script_lua/clua.h"

static const QStringList g_sysConstVarName = {
	"char", "pet", "item", "map", "magic", "skill", "petskill", "petequip", "dialog", "chat", "battle", "point", "team", "card", "unit", "mails",
	"INDEX", "_VERSION", "_LINE_", "isbattle", "isonline", "isnormal"
};

using CommandRegistry = std::function<__int64(__int64 currentIndex, __int64 currentLine, const TokenMap& token)>;

//callbak
using ParserCallBack = std::function<__int64(__int64 currentIndex, __int64 currentLine, const TokenMap& token)>;

struct Counter
{
	__int64 error = 0;									//錯誤計數器
	__int64 space = 0;									//當前行開頭空格數
	__int64 comment = 0;								//命令計數器
	__int64 validCommand = 0;							//有效命令計數器
};

enum CompareArea
{
	kAreaChar,
	kAreaPet,
	kAreaItem,
	kAreaCount,
};

enum CompareType
{
	kCompareTypeNone,
	kCharName,
	kCharFreeName,
	kCharLevel,
	kCharHp,
	kCharMaxHp,
	kCharHpPercent,
	kCharMp,
	kCharMaxMp,
	kCharMpPercent,
	kCharExp,
	kCharMaxExp,
	kCharStone,
	kCharVit,
	kCharStr,
	kCharTgh,
	kCharDex,
	kCharAtk,
	kCharDef,
	kCharAgi,
	kCharChasma,
	kCharTurn,
	kCharEarth,
	kCharWater,
	kCharFire,
	kCharWind,

	kPetName,
	kPetFreeName,
	kPetLevel,
	kPetHp,
	kPetMaxHp,
	kPetHpPercent,
	kPetExp,
	kPetMaxExp,
	kPetAtk,
	kPetDef,
	kPetAgi,
	kPetLoyal,
	kPetTurn,
	kPetState,
	kPetEarth,
	kPetWater,
	kPetFire,
	kPetWind,
	kPetPower,

	kitemCount,
	kItemName,
	kItemMemo,
	kItemDura,
	kItemLevel,
	kItemStack,

	kTeamName,
	kTeamLevel,
	kTeamHp,
	kTeamMaxHp,
	kTeamHpPercent,
	kTeamMp,

	kCardName,
	kCardOnlineState,
	kCardTurn,
	kCardDp,
	kCardLevel,

	kUnitName,
	kUnitFreeName,
	kUnitFamilyName,
	kUnitLevel,
	kUnitDir,
	kUnitX,
	kUnitY,
	kUnitGold,
	kUnitModelId,

	kBattleUnitPos,
	kBattleUnitName,
	kBattleUnitFreeName,
	kBattleUnitModelId,
	kBattleUnitLevel,
	kBattleUnitHp,
	kBattleUnitMaxHp,
	kBattleUnitHpPercent,
	kBattleUnitStatus,
	kBattleUnitRideFlag,
	kBattleUnitRideName,
	kBattleUnitRideLevel,
	kBattleUnitRideHp,
	kBattleUnitRideMaxHp,
	kBattleUnitRideHpPercent,

	kBattleRound,
	kBattleField,
	kBattleDuration,
	kBattleTotalDuration,
	kBattleTotalCombat,

	kTeamCount,
	kPetCount,

	kMapName,
	kMapFloor,
	kMapX,
	kMapY,

};

inline static const QHash<QString, CompareType> compareBattleUnitTypeMap = {
	{ "pos", kBattleUnitPos },
	{ "name", kBattleUnitName },
	{ "fname", kBattleUnitFreeName },
	{ "modelid", kBattleUnitModelId },
	{ "lv", kBattleUnitLevel },
	{ "hp", kBattleUnitHp },
	{ "maxhp", kBattleUnitMaxHp },
	{ "hpp", kBattleUnitHpPercent },
	{ "status", kBattleUnitStatus },
	{ "ride", kBattleUnitRideFlag },
	{ "ridename", kBattleUnitRideName },
	{ "ridelevel", kBattleUnitRideLevel },
	{ "ridehp", kBattleUnitRideHp },
	{ "ridemaxhp", kBattleUnitRideMaxHp },
	{ "ridehpp", kBattleUnitRideHpPercent },
};

inline static const QHash<QString, CompareType> compareBattleTypeMap = {
	{ "round", kBattleRound },
	{ "field", kBattleField },
	{ "duration", kBattleDuration },
	{ "totalduration", kBattleTotalDuration },
	{ "totalcombat", kBattleTotalCombat },

};

inline static const QHash<QString, CompareType> compareUnitTypeMap = {
	{ "name", kUnitName },
	{ "fname", kUnitFreeName },
	{ "family", kUnitFamilyName },
	{ "lv", kUnitLevel },
	{ "dir", kUnitDir },
	{ "x", kUnitX },
	{ "y", kUnitY },
	{ "gold", kUnitGold },
	{ "modelid", kUnitModelId },
};

inline static const QHash<QString, CompareType> compareCardTypeMap = {
	{ "name", kCardName },
	{ "online", kCardOnlineState },
	{ "turn", kCardTurn },
	{ "dp", kCardDp },
	{ "lv", kCardLevel },
};

inline static const QHash<QString, CompareType> compareItemTypeMap = {
	{ "count", kitemCount },
	{ "name", kItemName },
	{ "memo", kItemMemo },
	{ "dura", kItemDura },
	{ "lv", kItemLevel },
	{ "stack", kItemStack },
};

inline static const QHash<QString, CompareType> compareTeamTypeMap = {
	{ "name", kTeamName },
	{ "lv", kTeamLevel },
	{ "hp", kTeamHp },
	{ "maxhp", kTeamMaxHp },
	{ "hpp", kTeamHpPercent },
	{ "mp", kTeamMp },
};

inline static const QHash<QString, CompareType> compareMapTypeMap = {
	{ "name", kMapName },
	{ "floor", kMapFloor },
	{ "x", kMapX },
	{ "y", kMapY },
};

inline static const QHash<QString, CompareType> comparePcTypeMap = {
	{ "name", kCharName },
	{ "fname", kCharFreeName },
	{ "lv", kCharLevel },
	{ "hp", kCharHp },
	{ "maxhp", kCharMaxHp },
	{ "hpp", kCharHpPercent },
	{ "mp", kCharMp },
	{ "maxmp", kCharMaxMp },
	{ "mpp", kCharMpPercent },
	{ "exp", kCharExp },
	{ "maxexp", kCharMaxExp },
	{ "stone", kCharStone },
	{ "vit", kCharVit },
	{ "str", kCharStr },
	{ "tgh", kCharTgh },
	{ "def", kCharDex },
	{ "atk", kCharAtk },
	{ "def", kCharDef },
	{ "agi", kCharAgi },
	{ "chasma", kCharChasma },
	{ "turn", kCharTurn },
	{ "earth", kCharEarth },
	{ "water", kCharWater },
	{ "fire", kCharFire },
	{ "wind", kCharWind },
};

inline static const QHash<QString, CompareType> comparePetTypeMap = {
	{ "name", kPetName },
	{ "fname", kPetFreeName },
	{ "lv", kPetLevel },
	{ "hp", kPetHp },
	{ "maxhp", kPetMaxHp },
	{ "hpp", kPetHpPercent },
	{ "exp", kPetExp },
	{ "maxexp", kPetMaxExp },
	{ "atk", kPetAtk },
	{ "def", kPetDef },
	{ "agi", kPetAgi },
	{ "loyal", kPetLoyal },
	{ "turn", kPetTurn },
	{ "state", kPetState },
	{ "earth", kPetEarth },
	{ "water", kPetWater },
	{ "fire", kPetFire },
	{ "wind", kPetWind },
	{ "power", kPetPower },
};

inline static const QHash<QString, CompareType> compareAmountTypeMap = {
	{ "道具數量", kitemCount },
	{ "組隊人數", kTeamCount },
	{ "寵物數量", kPetCount },

	{ "道具数量", kitemCount },
	{ "组队人数", kTeamCount },
	{ "宠物数量", kPetCount },

	{ "ifitem", kitemCount },
	{ "ifteam", kTeamCount },
	{ "ifpet", kPetCount },
};

static const QSet<RESERVE> operatorTypes = {
	TK_ADD, //"+"
	TK_SUB, // '-'
	TK_MUL, // '*'
	TK_DIV, // '/'
	TK_MOD, // '%'
	TK_SHL, // "<<"
	TK_SHR, // ">>"
	TK_AND, // "&"
	TK_OR, // "|"
	TK_NOT, // "~"
	TK_XOR, // "^"
	TK_NEG, //"!" (與C++的 ! 同義)
	TK_INC, // "++" (與C++的 ++ 同義)
	TK_DEC, // "--" (與C++的 -- 同義)
};

static const QSet<RESERVE> relationalOperatorTypes = {
	TK_EQ, // "=="
	TK_NEQ, // "!="
	TK_GT, // ">"
	TK_LT, // "<"
	TK_GEQ, // ">="
	TK_LEQ, // "<="
};

enum JumpBehavior
{
	SuccessJump,
	FailedJump,
};

class Interpreter;
class Parser : public ThreadPlugin
{
	Q_OBJECT
public:
	enum
	{
		kStop = 0,
		kContinue = 1,
		kPause,
		kResume,
	};

	enum ParserCommandStatus
	{
		kHasJump = 0,
		kNoChange,
		kError,
		kServerNotReady,
		kLabelError,
		kUnknownCommand,
		kLuaError,
		kArgError = 100,
	};

	enum Mode
	{
		kSync,
		kAsync,
	};

	enum StackMode
	{
		kModeCallStack,
		kModeJumpStack,
	};

public:
	explicit Parser(__int64 index);
	virtual ~Parser();

	void initialize(Parser* pparent);

	//解析腳本
	void parse(__int64 line = 0);

	inline [[nodiscard]] __int64 getBeginLine() const { return lineNumber_; }
	inline [[nodiscard]] QString getScriptFileName() const { return scriptFileName_; }
	inline [[nodiscard]] bool isPrivate() const { return isPrivate_; }
	inline [[nodiscard]] QHash<__int64, TokenMap> getTokens() const { return tokens_; }
	inline [[nodiscard]] QHash<QString, __int64> getLabels() const { return labels_; }
	inline [[nodiscard]] QList<FunctionNode> getFunctionNodeList() const { return functionNodeList_; }
	inline [[nodiscard]] QList<ForNode> getForNodeList() const { return forNodeList_; }
	inline [[nodiscard]] QList<LuaNode> getLuaNodeList() const { return luaNodeList_; }
	inline [[nodiscard]] __int64 getCurrentLine() const { return lineNumber_; }
	inline [[nodiscard]] bool isSubScript() const { return isSubScript_; }
	inline [[nodiscard]] QHash<QString, __int64> getLabels() { return labels_; }
	inline [[nodiscard]] Parser::Mode getMode() const { return mode_; }
	inline [[nodiscard]] QSharedPointer<QStringList> getGlobalNameListPointer() const { return globalNames_; }
	inline [[nodiscard]] QSharedPointer<Counter> getCounterPointer() const { return counter_; }
	inline [[nodiscard]] QSharedPointer<QStack<QVariantHash>> getLocalVarStackPointer() const { return localVarStack_; }
	inline [[nodiscard]] QSharedPointer<QStringList> getLuaLocalVarStringListPointer() const { return luaLocalVarStringList_; }
	inline [[nodiscard]] Parser* getParent() const { return pparent_; }
	inline [[nodiscard]] Interpreter* getInterpreter() const { return pinterpreter_; }

	inline void setScriptFileName(const QString& scriptFileName) { scriptFileName_ = scriptFileName; }
	inline void setCurrentLine(const __int64 line) { lineNumber_ = line; }
	inline void setPrivate(bool isPrivate) { isPrivate_ = isPrivate; }
	inline void setMode(Mode mode) { mode_ = mode; }
	inline void setTokens(const QHash<__int64, TokenMap>& tokens) { tokens_ = tokens; }
	inline void setLabels(const QHash<QString, __int64>& labels) { labels_ = labels; }
	inline void setFunctionNodeList(const QList<FunctionNode>& functionNodeList) { functionNodeList_ = functionNodeList; }
	inline void setForNodeList(const QList<ForNode>& forNodeList) { forNodeList_ = forNodeList; }
	inline void setLuaNodeList(const QList<LuaNode>& luaNodeList) { luaNodeList_ = luaNodeList; }
	inline void setCallBack(ParserCallBack callBack) { callBack_ = callBack; }
	inline void setSubScript(bool isSubScript) { isSubScript_ = isSubScript; }

	inline void setInterpreter(Interpreter* interpreter) { pinterpreter_ = interpreter; }
	inline void setParent(Parser* pparent) { pparent_ = pparent; }
	inline void setGlobalNameListPointer(const QSharedPointer<QStringList>& globalNames)
	{
		if (!globalNames_.isNull())
			globalNames_.reset();
		globalNames_ = globalNames;
	}
	inline void setLuaMachinePointer(const QSharedPointer<CLua>& pLua)
	{
		if (!pLua_.isNull())
			pLua_.reset();
		pLua_ = pLua;
	}
	inline void setCounterPointer(const QSharedPointer<Counter>& counter)
	{
		if (!counter_.isNull())
			counter_.reset();
		counter_ = counter;
	}
	inline void setLocalVarStackPointer(const QSharedPointer<QStack<QVariantHash>>& localVarStack)
	{
		if (!localVarStack_.isNull())
			localVarStack_.reset();
		localVarStack_ = localVarStack;
	}
	inline void setLuaLocalVarStringListPointer(const QSharedPointer<QStringList>& luaLocalVarStringList)
	{
		if (!luaLocalVarStringList_.isNull())
			luaLocalVarStringList_.reset();
		luaLocalVarStringList_ = luaLocalVarStringList;
	}

	bool loadString(const QString& content);

public:
	inline [[nodiscard]] bool hasToken() const { return !tokens_.isEmpty(); }

	inline [[nodiscard]] const QHash<__int64, TokenMap> getToken() const { return tokens_; }

	void insertUserCallBack(const QString& name, const QString& type);

	inline void registerFunction(const QString& commandName, const CommandRegistry& function) { commandRegistry_.insert(commandName, static_cast<CommandRegistry>(function)); }

	bool jump(__int64 line, bool noStack);
	void jumpto(__int64 line, bool noStack);
	bool jump(const QString& name, bool noStack);

	bool checkString(const TokenMap& TK, __int64 idx, QString* ret);
	bool checkInteger(const TokenMap& TK, __int64 idx, __int64* ret);
	bool checkNumber(const TokenMap& TK, __int64 idx, double* ret);
	bool checkBoolean(const TokenMap& TK, __int64 idx, bool* ret);

	QVariant checkValue(const TokenMap TK, __int64 idx, QVariant::Type = QVariant::Invalid);
	__int64 checkJump(const TokenMap& TK, __int64 idx, bool expr, JumpBehavior behavior);

	QVariant luaDoString(QString expr);

public:

	[[nodiscard]] bool isGlobalVarContains(const QString& name);

	QVariant getGlobalVarValue(const QString& name);

	void insertGlobalVar(const QString& name, const QVariant& value);

	void insertLocalVar(const QString& name, const QVariant& value);

	void insertVar(const QString& name, const QVariant& value);

	QString getLuaTableString(const sol::table& t, __int64& depth);

private:
	void processTokens();
	__int64 processCommand();
	void processVariableIncDec();
	void processVariableCAOs();
	void processVariable();
	bool processLuaString();
	void processFormation();
	bool processCall(RESERVE reserve);
	bool processGoto();
	bool processJump();
	bool processReturn(__int64 takeReturnFrom = 1);
	void processBack();
	void processFunction();
	void processLabel();
	void processClean();
	void processDelay();
	bool processFor();
	bool processEnd();
	bool processEndFor();
	bool processBreak();
	bool processContinue();
	bool processLuaCode();
	bool processIfCompare();
	void processMultiVariable();
#if 0
	void processLocalVariable();
	void processVariableExpr();
	void processTable();
#endif

	void updateSysConstKeyword(const QString& expr);
	void importLocalVariablesToPreLuaList();
	bool checkCallStack();
	void exportVarInfo();

	void checkConditionalOperator(QString& expr);



	template <typename T>
	typename std::enable_if<
		std::is_same<T, QString>::value ||
		std::is_same<T, QVariant>::value ||
		std::is_same<T, bool>::value ||
		std::is_same<T, __int64>::value ||
		std::is_same<T, double>::value
		, bool>::type
		exprTo(QString expr, T* ret);

	void handleError(__int64 err, const QString& addition = "");

	void checkCallArgs(__int64 line);

	[[nodiscard]] bool isLocalVarContains(const QString& name);

	[[nodiscard]] QVariant getLocalVarValue(const QString& name);

	void removeLocalVar(const QString& name);

	void removeGlobalVar(const QString& name);

	[[nodiscard]] QVariantHash getLocalVars() const;

	[[nodiscard]] QVariantHash& getLocalVarsRef();

	inline void next() { ++lineNumber_; }

	inline [[nodiscard]] bool empty() const { return !tokens_.contains(lineNumber_); }

	inline [[nodiscard]] RESERVE getCurrentFirstTokenType() const
	{
		Token token = currentLineTokens_.value(0, Token{});
		return token.type;
	}

	template <typename T>
	inline [[nodiscard]] T getToken(__int64 index) const
	{
		if (currentLineTokens_.contains(index))
		{
			QVariant data = currentLineTokens_.value(index).data;
			if (data.isValid())
				return currentLineTokens_.value(index).data.value<T>();
		}
		//如果是整數返回 -1
		if (std::is_same<T, __int64>::value)
			return 0;
		return T();
	}

	inline [[nodiscard]] RESERVE getTokenType(__int64 index) const { return currentLineTokens_.value(index).type; }

	inline [[nodiscard]] __int64 size() const { return tokens_.size(); }

	inline [[nodiscard]] TokenMap getCurrentTokens() const { return currentLineTokens_; }

	__int64 matchLineFromLabel(const QString& label) const;

	__int64 matchLineFromFunction(const QString& funcName) const;

	FunctionNode getFunctionNodeByName(const QString& funcName) const;

	ForNode getForNodeByLineIndex(__int64 line) const;

	[[nodiscard]] QVariantList& getArgsRef();

public:
	QSharedPointer<CLua> pLua_ = nullptr;

private:
	Parser* pparent_ = nullptr;
	Interpreter* pinterpreter_ = nullptr;
	Lexer lexer_;

	QHash<quint64, QSharedPointer<QElapsedTimer>> timerMap_;

	QString scriptFileName_;
	bool isPrivate_ = false;

	QHash<__int64, TokenMap> tokens_;						//當前運行腳本的每一行token
	QHash<QString, __int64> labels_;							//所有標記/函數所在行記錄
	QList<FunctionNode> functionNodeList_;
	QList<ForNode> forNodeList_;
	QList<LuaNode> luaNodeList_;

	QSharedPointer<QStringList> globalNames_ = nullptr;				//全局變量名稱
	QSharedPointer<QStack<QVariantHash>> localVarStack_ = nullptr;	//局變量棧
	QSharedPointer<QStringList> luaLocalVarStringList_ = nullptr;		//lua局變量
	QSharedPointer<Counter> counter_ = nullptr;

	QHash<QString, QString> userRegCallBack_;				//用戶註冊的回調函數
	QHash<QString, CommandRegistry> commandRegistry_;		//所有已註冊的腳本命令函數指針

	QStack<RESERVE> currentField; 						    //當前域
	QStack<FunctionNode> callStack_;						//"調用"命令所在行棧
	QStack<__int64> jmpStack_;								//"跳轉"命令所在行棧
	QStack<ForNode> forStack_;			                 	//"遍歷"命令所在行棧
	QStack<QVariantList> callArgsStack_;					//"調用"命令參數棧
	QVariantList emptyArgs_;								//空參數(參數棧為空得情況下壓入一個空容器)
	QVariantHash emptyLocalVars_;							//空局變量(局變量棧為空得情況下壓入一個空容器)

	TokenMap currentLineTokens_;							//當前行token
	RESERVE currentType_ = TK_UNK;							//當前行第一個token類型
	__int64 lineNumber_ = 0;									//當前行號

	ParserCallBack callBack_ = nullptr;						//腳本回調函數

	Mode mode_ = kSync;										//解析模式(同步|異步)

	bool skipFunctionChunkDisable_ = false;					//是否跳過函數代碼塊檢查

	bool isSubScript_ = false;								//是否是子腳本		

	bool luaBegin_ = false;									//lua代碼塊開始標記
};