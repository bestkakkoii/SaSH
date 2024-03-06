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
#include "lexer.h"
#include <QStack>
#include <functional>

#include <model/indexer.h>
#include "util.h"
#include "script_lua/clua.h"

static const QStringList g_sysConstVarName = {
	"char", "pet", "item", "map", "magic", "skill", "petskill", "petequip", "dialog", "chat", "battle", "point", "team", "card", "unit", "mails",
	"INDEX", "_VERSION", "MAXCARD", "MAXCHAR", "MAXCHAT", "MAXDIR", "MAXDLG", "MAXENEMY", "MAXEQUIP", "MAXITEM", "MAXMAGIC",
	"INFINITE", "MAXPET", "MAXPETSKILL", "MAXSKILL", "MAXTHREAD", "TID", "PID", "LINE", "HWND", "GAMEPID", "GAMEHWND", "GAMEHANDLE", "FILE",
	"CURRENTSCRIPTDIR", "CURRENTDIR", "SCRIPTDIR", "SETTINGDIR"
};

using CommandRegistry = std::function<long long(long long currentIndex, long long currentLine, const TokenMap& token)>;

//callbak
using ParserCallBack = std::function<long long(long long currentIndex, long long currentLine, const TokenMap& token)>;

using VariantSafeHash = safe::hash<QString, QVariant>;

struct Counter
{
	long long error = 0;									//錯誤計數器
	long long space = 0;									//當前行開頭空格數
	long long comment = 0;								//命令計數器
	long long validCommand = 0;							//有效命令計數器
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
class Parser : public QObject, public Indexer
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
	explicit Parser(long long index);
	virtual ~Parser();

	void initialize(Parser* pparent);

	//解析腳本
	void parse(long long line = 0);

	inline [[nodiscard]] long long __fastcall getBeginLine() const { return lineNumber_; }
	inline [[nodiscard]] QString __fastcall getScriptFileName() const { return scriptFileName_; }
	inline [[nodiscard]] bool __fastcall isPrivate() const { return isPrivate_; }
	inline [[nodiscard]] QHash<long long, TokenMap> __fastcall getTokens() const { return tokens_; }
	inline [[nodiscard]] QHash<QString, long long> __fastcall getLabels() const { return labels_; }
	inline [[nodiscard]] QList<FunctionNode> __fastcall getFunctionNodeList() const { return functionNodeList_; }
	inline [[nodiscard]] QList<ForNode> __fastcall getForNodeList() const { return forNodeList_; }
	inline [[nodiscard]] QList<LuaNode> __fastcall getLuaNodeList() const { return luaNodeList_; }
	inline [[nodiscard]] long long __fastcall getCurrentLine() const { return lineNumber_; }
	inline [[nodiscard]] bool __fastcall isSubScript() const { return isSubScript_; }
	inline [[nodiscard]] QHash<QString, long long> __fastcall getLabels() { return labels_; }
	inline [[nodiscard]] Parser::Mode __fastcall getMode() const { return mode_; }
	inline [[nodiscard]] std::shared_ptr<QStringList>& __fastcall getGlobalNameListPointer() { return globalNames_; }
	inline [[nodiscard]] std::shared_ptr<Counter>& __fastcall getCounterPointer() { return counter_; }
	inline [[nodiscard]] std::shared_ptr<QStack<QVariantHash>>& __fastcall getLocalVarStackPointer() { return localVarStack_; }
	inline [[nodiscard]] std::shared_ptr<QStringList>& __fastcall getLuaLocalVarStringListPointer() { return luaLocalVarStringList_; }
	inline [[nodiscard]] Parser* __fastcall getParent() const { return pparent_; }
	inline [[nodiscard]] Interpreter* __fastcall getInterpreter() const { return pinterpreter_; }

	inline void __fastcall setScriptFileName(const QString& scriptFileName) { scriptFileName_ = scriptFileName; }
	inline void __fastcall setCurrentLine(const long long line) { lineNumber_ = line; }
	inline void __fastcall setPrivate(bool isPrivate) { isPrivate_ = isPrivate; }
	inline void __fastcall setMode(Mode mode) { mode_ = mode; }
	inline void __fastcall setTokens(const QHash<long long, TokenMap>& tokens) { tokens_ = tokens; }
	inline void __fastcall setLabels(const QHash<QString, long long>& labels) { labels_ = labels; }
	inline void __fastcall setFunctionNodeList(const QList<FunctionNode>& functionNodeList) { functionNodeList_ = functionNodeList; }
	inline void __fastcall setForNodeList(const QList<ForNode>& forNodeList) { forNodeList_ = forNodeList; }
	inline void __fastcall setLuaNodeList(const QList<LuaNode>& luaNodeList) { luaNodeList_ = luaNodeList; }
	inline void __fastcall setCallBack(ParserCallBack callBack) { callBack_ = callBack; }
	inline void __fastcall setSubScript(bool isSubScript) { isSubScript_ = isSubScript; }

	inline void __fastcall setInterpreter(Interpreter* interpreter) { pinterpreter_ = interpreter; }
	inline void __fastcall setParent(Parser* pparent) { pparent_ = pparent; }
	inline void __fastcall setGlobalNameListPointer(const std::shared_ptr<QStringList>& globalNames)
	{
		globalNames_.reset();
		globalNames_ = globalNames;
	}
	inline void __fastcall setLuaMachinePointer(const std::shared_ptr<CLua>& pLua)
	{
		pLua_.reset();
		pLua_ = pLua;
	}
	inline void __fastcall setCounterPointer(const std::shared_ptr<Counter>& counter)
	{
		counter_.reset();
		counter_ = counter;
	}
	inline void __fastcall setLocalVarStackPointer(const std::shared_ptr<QStack<QVariantHash>>& localVarStack)
	{
		localVarStack_.reset();
		localVarStack_ = localVarStack;
	}
	inline void __fastcall setLuaLocalVarStringListPointer(const std::shared_ptr<QStringList>& luaLocalVarStringList)
	{
		luaLocalVarStringList_.reset();
		luaLocalVarStringList_ = luaLocalVarStringList;
	}

	bool __fastcall loadString(const QString& content);

public:
	inline [[nodiscard]] bool __fastcall hasToken() const { return !tokens_.isEmpty(); }

	inline [[nodiscard]] const QHash<long long, TokenMap> __fastcall getToken() const { return tokens_; }

	void __fastcall insertUserCallBack(const QString& name, const QString& type);

	inline void __fastcall registerFunction(const QString& commandName, const CommandRegistry& function) { commandRegistry_.insert(commandName, static_cast<CommandRegistry>(function)); }

	bool __fastcall jump(long long line, bool noStack);
	void __fastcall jumpto(long long line, bool noStack);
	bool __fastcall jump(const QString& name, bool noStack);

	bool __fastcall checkString(const TokenMap& TK, long long idx, QString* ret);
	bool __fastcall checkInteger(const TokenMap& TK, long long idx, long long* ret);
	bool __fastcall checkNumber(const TokenMap& TK, long long idx, double* ret);
	bool __fastcall checkBoolean(const TokenMap& TK, long long idx, bool* ret);

	QVariant __fastcall checkValue(const TokenMap TK, long long idx, QVariant::Type = QVariant::Invalid);
	long long __fastcall checkJump(const TokenMap& TK, long long idx, bool expr, JumpBehavior behavior);

	QVariant __fastcall luaDoString(QString expr);

public:

	[[nodiscard]] bool __fastcall isGlobalVarContains(const QString& name);

	QVariant __fastcall getGlobalVarValue(const QString& name);

	void __fastcall insertGlobalVar(const QString& name, const QVariant& value);

	void __fastcall insertLocalVar(const QString& name, const QVariant& value);

	void __fastcall insertVar(const QString& name, const QVariant& value);

	QString __fastcall getLuaTableString(const sol::table& t, long long& depth);

private:
	void __fastcall processTokens();
	long long __fastcall processCommand();
	void __fastcall processVariableIncDec();
	void __fastcall processVariableCAOs();
	void __fastcall processVariable();
	bool __fastcall processLuaString();
	void __fastcall processFormation();
	bool __fastcall processCall(RESERVE reserve);
	bool __fastcall processGoto();
	bool __fastcall processJump();
	bool __fastcall processReturn(long long takeReturnFrom = 1);
	void __fastcall processBack();
	void __fastcall processFunction();
	void __fastcall processLabel();
	void __fastcall processClean();
	void __fastcall processDelay() const;
	bool __fastcall processFor();
	bool __fastcall processEnd();
	bool __fastcall processEndFor();
	bool __fastcall processBreak();
	bool __fastcall processContinue();
	bool __fastcall processLuaCode();
	bool __fastcall processIfCompare();
	void __fastcall processMultiVariable();
#if 0
	void __fastcall processLocalVariable();
	void __fastcall processVariableExpr();
	void __fastcall processTable();
#endif

	void __fastcall updateSysConstKeyword(const QString& expr);
	void __fastcall importLocalVariablesToPreLuaList();
	bool __fastcall checkCallStack();
	void __fastcall exportVarInfo();

	void __fastcall checkConditionalOperator(QString& expr);



	template <typename T>
	typename std::enable_if<
		std::is_same<T, QString>::value ||
		std::is_same<T, QVariant>::value ||
		std::is_same<T, bool>::value ||
		std::is_same<T, long long>::value ||
		std::is_same<T, double>::value
		, bool>::type
		__fastcall exprTo(QString expr, T* ret);

	void __fastcall handleError(long long err, const QString& addition = "");

	void __fastcall checkCallArgs(long long line);

	[[nodiscard]] bool __fastcall isLocalVarContains(const QString& name);

	[[nodiscard]] QVariant __fastcall getLocalVarValue(const QString& name);

	void __fastcall removeLocalVar(const QString& name);

	void __fastcall removeGlobalVar(const QString& name);

	[[nodiscard]] QVariantHash __fastcall getLocalVars() const;

	[[nodiscard]] QVariantHash& __fastcall getLocalVarsRef();

	inline void __fastcall next() { ++lineNumber_; }

	inline [[nodiscard]] bool __fastcall empty() const { return !tokens_.contains(lineNumber_); }

	inline [[nodiscard]] RESERVE __fastcall getCurrentFirstTokenType() const
	{
		Token token = currentLineTokens_.value(0, Token{});
		return token.type;
	}

	template <typename T>
	inline [[nodiscard]] T __fastcall getToken(long long index) const
	{
		if (currentLineTokens_.contains(index))
		{
			QVariant data = currentLineTokens_.value(index).data;
			if (data.isValid())
				return currentLineTokens_.value(index).data.value<T>();
		}
		//如果是整數返回 -1
		if (std::is_same<T, long long>::value)
			return 0;
		return T();
	}

	inline [[nodiscard]] RESERVE __fastcall getTokenType(long long index) const { return currentLineTokens_.value(index).type; }

	inline [[nodiscard]] long long __fastcall size() const { return tokens_.size(); }

	inline [[nodiscard]] TokenMap __fastcall getCurrentTokens() const { return currentLineTokens_; }

	long long __fastcall matchLineFromLabel(const QString& label) const;

	long long __fastcall matchLineFromFunction(const QString& funcName) const;

	FunctionNode __fastcall getFunctionNodeByName(const QString& funcName) const;

	ForNode __fastcall getForNodeByLineIndex(long long line) const;

	[[nodiscard]] QVariantList& __fastcall getArgsRef();

public:
	std::shared_ptr<CLua> pLua_ = nullptr;

private:
	Parser* pparent_ = nullptr;
	Interpreter* pinterpreter_ = nullptr;
	Lexer lexer_;

	QString scriptFileName_;
	bool isPrivate_ = false;

	QHash<long long, TokenMap> tokens_;						//當前運行腳本的每一行token
	QHash<QString, long long> labels_;							//所有標記/函數所在行記錄
	QList<FunctionNode> functionNodeList_;
	QList<ForNode> forNodeList_;
	QList<LuaNode> luaNodeList_;

	std::shared_ptr<QStringList> globalNames_ = nullptr;				//全局變量名稱
	std::shared_ptr<QStack<QVariantHash>> localVarStack_ = nullptr;	//局變量棧
	std::shared_ptr<QStringList> luaLocalVarStringList_ = nullptr;		//lua局變量
	std::shared_ptr<Counter> counter_ = nullptr;

	QHash<QString, QString> userRegCallBack_;				//用戶註冊的回調函數
	QHash<QString, CommandRegistry> commandRegistry_;		//所有已註冊的腳本命令函數指針

	QStack<RESERVE> currentField; 						    //當前域
	QStack<FunctionNode> callStack_;						//"調用"命令所在行棧
	QStack<long long> jmpStack_;								//"跳轉"命令所在行棧
	QStack<ForNode> forStack_;			                 	//"遍歷"命令所在行棧
	QStack<QVariantList> callArgsStack_;					//"調用"命令參數棧
	QVariantList emptyArgs_;								//空參數(參數棧為空得情況下壓入一個空容器)
	QVariantHash emptyLocalVars_;							//空局變量(局變量棧為空得情況下壓入一個空容器)

	TokenMap currentLineTokens_;							//當前行token
	RESERVE currentType_ = TK_UNK;							//當前行第一個token類型
	long long lineNumber_ = 0;									//當前行號

	ParserCallBack callBack_ = nullptr;						//腳本回調函數

	Mode mode_ = kSync;										//解析模式(同步|異步)

	bool skipFunctionChunkDisable_ = false;					//是否跳過函數代碼塊檢查

	bool isSubScript_ = false;								//是否是子腳本		

	bool luaBegin_ = false;									//lua代碼塊開始標記

	bool stopByExit_ = false;								//是否因為exit命令而停止

	bool battleProcOn_ = false;								//是否處於戰鬥中
	bool offlineLabelOn_ = false;							//是否處於離線標記中
	bool closeLabelOn_ = false;								//是否處於關閉標記中

	//不輸出到列表也不做更動的全局變量名稱
	QStringList g_lua_exception_var_list = {
		"__THIS_CLUA", "__THIS_PARENT", "__HOOKFORSTOP", "__INDEX", "__HOOKFORSTOP", "__HOOKFORBATTLE",
		"__THIS_PARSER", "__THIS", "_G",
		"__print", "__require",
		"TARGET",
		"BattleClass", "CharClass", "InfoClass", "ItemClass", "MapClass", "PetClass", "SystemClass", "DialogClass", "Timer", "TARGET", "_G", "ROWCOUNT",
		"assert", "base",  "collectgarbage",
		"contains", "copy", "coroutine", "dbclick", "debug", "dofile", "dragto", "error", "find", "format", "full", "getmetatable",
		"half", "input", "io", "ipairs", "lclick", "load", "loadfile", "loadset", "lower", "math", "mkpath", "mktable", "dlg", "ocr", "findfiles",
		"msg", "next", "os", "package", "pairs", "pcall",  "print", "printf", "rawequal",
		"rawget", "rawlen", "rawset", "rclick", "regex", "replace", "require", "rex", "rexg", "rnd", "saveset", "select", "set", "setmetatable",
		"skill", "sleep", "split", "string", "table", "tadd", "tback", "tfront", "tjoin", "tmerge", "todb", "toint", "tonumber", "tostr",
		"tostring", "tpadd", "tpopback", "tpopfront", "trim", "trotate", "trsort", "tshuffle", "tsleft", "tsort", "tsright", "tswap", "tunique", "type",
		"unit", "upper", "utf8", "warn", "xpcall",
		"UnitStruct", "TeamStruct", "BattleStruct", "CardStruct", "CharacterStruct", "DialogStruct", "PetSkillStruct", "PetStruct",
		"SkillStruct", "MagicStruct", "ItemStruct"


	};
};