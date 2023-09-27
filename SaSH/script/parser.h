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
#include "lexer.h"
#include <QStack>
#include <functional>

#include "threadplugin.h"
#include "util.h"
#include "script_lua/clua.h"

using CommandRegistry = std::function<qint64(qint64 currentIndex, qint64 currentLine, const TokenMap& token)>;

//callbak
using ParserCallBack = std::function<qint64(qint64 currentIndex, qint64 currentLine, const TokenMap& token)>;

using VariantSafeHash = util::SafeHash<QString, QVariant>;

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
	{ u8"pos", kBattleUnitPos },
	{ u8"name", kBattleUnitName },
	{ u8"fname", kBattleUnitFreeName },
	{ u8"modelid", kBattleUnitModelId },
	{ u8"lv", kBattleUnitLevel },
	{ u8"hp", kBattleUnitHp },
	{ u8"maxhp", kBattleUnitMaxHp },
	{ u8"hpp", kBattleUnitHpPercent },
	{ u8"status", kBattleUnitStatus },
	{ u8"ride", kBattleUnitRideFlag },
	{ u8"ridename", kBattleUnitRideName },
	{ u8"ridelevel", kBattleUnitRideLevel },
	{ u8"ridehp", kBattleUnitRideHp },
	{ u8"ridemaxhp", kBattleUnitRideMaxHp },
	{ u8"ridehpp", kBattleUnitRideHpPercent },
};

inline static const QHash<QString, CompareType> compareBattleTypeMap = {
	{ u8"round", kBattleRound },
	{ u8"field", kBattleField },
	{ u8"duration", kBattleDuration },
	{ u8"totalduration", kBattleTotalDuration },
	{ u8"totalcombat", kBattleTotalCombat },

};

inline static const QHash<QString, CompareType> compareUnitTypeMap = {
	{ u8"name", kUnitName },
	{ u8"fname", kUnitFreeName },
	{ u8"family", kUnitFamilyName },
	{ u8"lv", kUnitLevel },
	{ u8"dir", kUnitDir },
	{ u8"x", kUnitX },
	{ u8"y", kUnitY },
	{ u8"gold", kUnitGold },
	{ u8"modelid", kUnitModelId },
};

inline static const QHash<QString, CompareType> compareCardTypeMap = {
	{ u8"name", kCardName },
	{ u8"online", kCardOnlineState },
	{ u8"turn", kCardTurn },
	{ u8"dp", kCardDp },
	{ u8"lv", kCardLevel },
};

inline static const QHash<QString, CompareType> compareItemTypeMap = {
	{ u8"count", kitemCount },
	{ u8"name", kItemName },
	{ u8"memo", kItemMemo },
	{ u8"dura", kItemDura },
	{ u8"lv", kItemLevel },
	{ u8"stack", kItemStack },
};

inline static const QHash<QString, CompareType> compareTeamTypeMap = {
	{ u8"name", kTeamName },
	{ u8"lv", kTeamLevel },
	{ u8"hp", kTeamHp },
	{ u8"maxhp", kTeamMaxHp },
	{ u8"hpp", kTeamHpPercent },
	{ u8"mp", kTeamMp },
};

inline static const QHash<QString, CompareType> compareMapTypeMap = {
	{ u8"name", kMapName },
	{ u8"floor", kMapFloor },
	{ u8"x", kMapX },
	{ u8"y", kMapY },
};

inline static const QHash<QString, CompareType> comparePcTypeMap = {
	{ u8"name", kCharName },
	{ u8"fname", kCharFreeName },
	{ u8"lv", kCharLevel },
	{ u8"hp", kCharHp },
	{ u8"maxhp", kCharMaxHp },
	{ u8"hpp", kCharHpPercent },
	{ u8"mp", kCharMp },
	{ u8"maxmp", kCharMaxMp },
	{ u8"mpp", kCharMpPercent },
	{ u8"exp", kCharExp },
	{ u8"maxexp", kCharMaxExp },
	{ u8"stone", kCharStone },
	{ u8"vit", kCharVit },
	{ u8"str", kCharStr },
	{ u8"tgh", kCharTgh },
	{ u8"def", kCharDex },
	{ u8"atk", kCharAtk },
	{ u8"def", kCharDef },
	{ u8"agi", kCharAgi },
	{ u8"chasma", kCharChasma },
	{ u8"turn", kCharTurn },
	{ u8"earth", kCharEarth },
	{ u8"water", kCharWater },
	{ u8"fire", kCharFire },
	{ u8"wind", kCharWind },
};

inline static const QHash<QString, CompareType> comparePetTypeMap = {
	{ u8"name", kPetName },
	{ u8"fname", kPetFreeName },
	{ u8"lv", kPetLevel },
	{ u8"hp", kPetHp },
	{ u8"maxhp", kPetMaxHp },
	{ u8"hpp", kPetHpPercent },
	{ u8"exp", kPetExp },
	{ u8"maxexp", kPetMaxExp },
	{ u8"atk", kPetAtk },
	{ u8"def", kPetDef },
	{ u8"agi", kPetAgi },
	{ u8"loyal", kPetLoyal },
	{ u8"turn", kPetTurn },
	{ u8"state", kPetState },
	{ u8"earth", kPetEarth },
	{ u8"water", kPetWater },
	{ u8"fire", kPetFire },
	{ u8"wind", kPetWind },
	{ u8"power", kPetPower },
};

inline static const QHash<QString, CompareType> compareAmountTypeMap = {
	{ u8"道具數量", kitemCount },
	{ u8"組隊人數", kTeamCount },
	{ u8"寵物數量", kPetCount },

	{ u8"道具数量", kitemCount },
	{ u8"组队人数", kTeamCount },
	{ u8"宠物数量", kPetCount },

	{ u8"ifitem", kitemCount },
	{ u8"ifteam", kTeamCount },
	{ u8"ifpet", kPetCount },
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
	explicit Parser(qint64 index);
	virtual ~Parser();

	//解析腳本
	void parse(qint64 line = 0);

	inline Q_REQUIRED_RESULT qint64 getBeginLine() const { return lineNumber_; }
	inline Q_REQUIRED_RESULT QString getScriptFileName() const { return scriptFileName_; }
	inline Q_REQUIRED_RESULT bool isPrivate() const { return isPrivate_; }
	inline Q_REQUIRED_RESULT QHash<qint64, TokenMap> getTokens() const { return tokens_; }
	inline Q_REQUIRED_RESULT QHash<QString, qint64> getLabels() const { return labels_; }
	inline Q_REQUIRED_RESULT QList<FunctionNode> getFunctionNodeList() const { return functionNodeList_; }
	inline Q_REQUIRED_RESULT QList<ForNode> getForNodeList() const { return forNodeList_; }
	inline Q_REQUIRED_RESULT QList<LuaNode> getLuaNodeList() const { return luaNodeList_; }
	inline Q_REQUIRED_RESULT qint64 getCurrentLine() const { return lineNumber_; }

	inline void setScriptFileName(const QString& scriptFileName) { scriptFileName_ = scriptFileName; }
	inline void setCurrentLine(const qint64 line) { lineNumber_ = line; }
	inline void setPrivate(bool isPrivate) { isPrivate_ = isPrivate; }
	inline void setMode(Mode mode) { mode_ = mode; }
	inline void setTokens(const QHash<qint64, TokenMap>& tokens) { tokens_ = tokens; }
	inline void setLabels(const QHash<QString, qint64>& labels) { labels_ = labels; }
	inline void setFunctionNodeList(const QList<FunctionNode>& functionNodeList) { functionNodeList_ = functionNodeList; }
	inline void setForNodeList(const QList<ForNode>& forNodeList) { forNodeList_ = forNodeList; }
	inline void setLuaNodeList(const QList<LuaNode>& luaNodeList) { luaNodeList_ = luaNodeList; }
	inline void setCallBack(ParserCallBack callBack) { callBack_ = callBack; }

	bool loadFile(const QString& fileName, QString* pcontent);
	bool loadString(const QString& content);

public:
	inline Q_REQUIRED_RESULT bool hasToken() const { return !tokens_.isEmpty(); }

	inline Q_REQUIRED_RESULT const QHash<qint64, TokenMap> getToken() const { return tokens_; }

	void insertUserCallBack(const QString& name, const QString& type);

	inline void registerFunction(const QString& commandName, const CommandRegistry& function) { commandRegistry_.insert(commandName, static_cast<CommandRegistry>(function)); }

	bool jump(qint64 line, bool noStack);
	void jumpto(qint64 line, bool noStack);
	bool jump(const QString& name, bool noStack);

	void removeEscapeChar(QString* str) const;
	bool checkString(const TokenMap& TK, qint64 idx, QString* ret);
	bool checkInteger(const TokenMap& TK, qint64 idx, qint64* ret);
	bool checkNumber(const TokenMap& TK, qint64 idx, double* ret);
	bool checkBoolean(const TokenMap& TK, qint64 idx, bool* ret);

	bool toVariant(const TokenMap& TK, qint64 idx, QVariant* ret);

	QVariant checkValue(const TokenMap TK, qint64 idx, QVariant::Type = QVariant::Invalid);
	qint64 checkJump(const TokenMap& TK, qint64 idx, bool expr, JumpBehavior behavior);

	bool isSubScript() const { return isSubScript_; }
	void setSubScript(bool isSubScript) { isSubScript_ = isSubScript; }

	QVariant luaDoString(QString expr);

public:

	Q_REQUIRED_RESULT bool isGlobalVarContains(const QString& name);

	QVariant getGlobalVarValue(const QString& name);

	void insertGlobalVar(const QString& name, const QVariant& value);

	void insertLocalVar(const QString& name, const QVariant& value);

	void insertVar(const QString& name, const QVariant& value);

	void setLuaMachinePointer(sol::state* pLua);

	QHash<QString, qint64> getLabels() { return labels_; }

	QString getLuaTableString(const sol::table& t, qint64& depth);

private:
	void processTokens();
	qint64 processCommand();
	void processLocalVariable();
	void processVariableIncDec();
	void processVariableCAOs();
	void processVariableExpr();
	void processVariable();
	void processTable();
	void processLuaString();
	void processFormation();
	bool processCall(RESERVE reserve);
	bool processGoto();
	bool processJump();
	bool processReturn(qint64 takeReturnFrom = 1);
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

	void updateSysConstKeyword(const QString& expr);
	void importVariablesToLua(const QString& expr);
	bool checkCallStack();
	void exportVarInfo();

	void checkConditionalOp(QString& expr);



	template <typename T>
	typename std::enable_if<
		std::is_same<T, QString>::value ||
		std::is_same<T, QVariant>::value ||
		std::is_same<T, bool>::value ||
		std::is_same<T, qint64>::value ||
		std::is_same<T, double>::value
		, bool>::type
		exprTo(QString expr, T* ret);

	void handleError(qint64 err, const QString& addition = "");

	void checkCallArgs(qint64 line);

	Q_REQUIRED_RESULT bool isLocalVarContains(const QString& name);

	Q_REQUIRED_RESULT QVariant getLocalVarValue(const QString& name);

	void removeLocalVar(const QString& name);

	void removeGlobalVar(const QString& name);

	Q_REQUIRED_RESULT QVariantHash getLocalVars() const;

	Q_REQUIRED_RESULT QVariantHash& getLocalVarsRef();

	inline void next() { ++lineNumber_; }

	inline Q_REQUIRED_RESULT bool empty() const { return !tokens_.contains(lineNumber_); }

	inline Q_REQUIRED_RESULT RESERVE getCurrentFirstTokenType() const
	{
		Token token = currentLineTokens_.value(0, Token{});
		return token.type;
	}

	template <typename T>
	inline Q_REQUIRED_RESULT T getToken(qint64 index) const
	{
		if (currentLineTokens_.contains(index))
		{
			QVariant data = currentLineTokens_.value(index).data;
			if (data.isValid())
				return currentLineTokens_.value(index).data.value<T>();
		}
		//如果是整數返回 -1
		if (std::is_same<T, qint64>::value)
			return 0;
		return T();
	}

	inline Q_REQUIRED_RESULT RESERVE getTokenType(qint64 index) const { return currentLineTokens_.value(index).type; }

	inline Q_REQUIRED_RESULT qint64 size() const { return tokens_.size(); }

	inline Q_REQUIRED_RESULT TokenMap getCurrentTokens() const { return currentLineTokens_; }

	qint64 matchLineFromLabel(const QString& label) const;

	qint64 matchLineFromFunction(const QString& funcName) const;

	FunctionNode getFunctionNodeByName(const QString& funcName) const;

	ForNode getForNodeByLineIndex(qint64 line) const;

	Q_REQUIRED_RESULT QVariantList& getArgsRef();

	void generateStackInfo(qint64 type);

public:
	QSharedPointer<CLua> pLua_;

private:

	Lexer lexer_;

	QHash<quint64, QSharedPointer<QElapsedTimer>> timerMap_;

	QString scriptFileName_;
	bool isPrivate_ = false;

	QHash<qint64, TokenMap> tokens_;						//當前運行腳本的每一行token
	QHash<QString, qint64> labels_;							//所有標記/函數所在行記錄
	QList<FunctionNode> functionNodeList_;
	QList<ForNode> forNodeList_;
	QList<LuaNode> luaNodeList_;

	QStringList globalNames_;								//全局變量名稱

	QHash<QString, QString> userRegCallBack_;				//用戶註冊的回調函數
	QHash<QString, CommandRegistry> commandRegistry_;		//所有已註冊的腳本命令函數指針

	QStack<RESERVE> currentField; 						    //當前域
	QStack<FunctionNode> callStack_;						//"調用"命令所在行棧
	QStack<qint64> jmpStack_;								//"跳轉"命令所在行棧
	QStack<ForNode> forStack_;			                 	//"遍歷"命令所在行棧
	QStack<QVariantList> callArgsStack_;					//"調用"命令參數棧
	QVariantList emptyArgs_;								//空參數(參數棧為空得情況下壓入一個空容器)
	QStack<QVariantHash> localVarStack_;					//局變量棧
	QStringList luaLocalVarStringList;						//lua局變量
	QVariantHash emptyLocalVars_;							//空局變量(局變量棧為空得情況下壓入一個空容器)
	QVariantList lastReturnValue_;							//函數返回值

	TokenMap currentLineTokens_;							//當前行token
	RESERVE currentType_ = TK_UNK;							//當前行第一個token類型
	qint64 lineNumber_ = 0;									//當前行號

	ParserCallBack callBack_ = nullptr;						//腳本回調函數

	Mode mode_ = kSync;										//解析模式(同步|異步)

	bool skipFunctionChunkDisable_ = false;					//是否跳過函數代碼塊檢查

	bool isSubScript_ = false;								//是否是子腳本		

};