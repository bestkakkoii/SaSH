#pragma once
#include "lexer.h"
#include <QStack>
#include <functional>

using CommandRegistry = std::function<int(int currentLine, const TokenMap& token)>;

//callbak
using ParserCallBack = std::function<int(int currentLine, const TokenMap& token)>;

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

class Parser
{
public:
	enum
	{
		kStop = 0,
		kContinue = 1,
		kPause,
		kResume,
	};

	enum ParserError
	{
		kNoError = 0,
		kException,
		kUndefinedVariable,
	};

	enum ParserCommandStatus
	{
		kHasJump = 0,
		kNoChange,
		kError,
		kArgError,
	};

	enum Mode
	{
		kSync,
		kAsync,
	};

public:
	inline ParserError lastError() const { return lastError_; }

	//設置所有標籤所在行號數據
	inline void setLabels(const QHash<QString, int>& labels) { labels_ = labels; }

	//設置腳本所有Token數據
	inline void setTokens(const QHash<int, TokenMap>& tokens) { tokens_ = tokens; }

	Q_REQUIRED_RESULT inline bool hasToken() const { return !tokens_.isEmpty(); }

	Q_REQUIRED_RESULT inline const QHash<int, TokenMap> getToken() const { return tokens_; }

	inline void setCallBack(const ParserCallBack& callBack) { callBack_ = callBack; }

	inline void setCurrentLine(int line) { lineNumber_ = line; }

	inline void setMode(Mode mode) { mode_ = mode; }

	inline void registerFunction(const QString& commandName, const CommandRegistry& function)
	{
		commandRegistry_.insert(commandName, static_cast<CommandRegistry>(function));
	}

	template <typename T>
	Q_REQUIRED_RESULT inline T getVar(const QString& name)
	{
		QString newName = name;
		if (newName.startsWith(kVariablePrefix))
			newName.remove(0, 1);

		if (variables_.contains(newName))
		{
			return variables_.value(newName).value<T>();
		}
		else
		{
			lastError_ = kUndefinedVariable;
			isStop_.store(true, std::memory_order_release);
			return T();
		}
	}

	void jump(int line, bool noStack = false);
	void jumpto(int line, bool noStack = false);
	bool jump(const QString& name, bool noStack = false);
public:
	//解析腳本
	void parse(int line = 0);

	Q_REQUIRED_RESULT inline QVariant& getVarRef(const QString& name) { return variables_[name]; }
	Q_REQUIRED_RESULT inline QVariantHash& getVarsRef() { return variables_; }
	Q_REQUIRED_RESULT inline QHash<QString, QVariant>& getLabelVars()
	{
		if (!labalVarStack_.isEmpty())
			return labalVarStack_.top();
		else
			return emptyLabelVars_;
	}
private:
	void processTokens();
	int processCommand();
	void processVariable(RESERVE type);
	void processMultiVariable();
	void processFormation();
	void processRandom();
	bool processCall();
	bool processGoto();
	bool processJump();
	void processReturn();
	void processBack();
	void processLabel();
	void processEnd();
	bool processGetSystemVarValue(const QString& varName, QString& valueStr, QVariant& varValue);

	void handleError(int err);
	void checkArgs();

	inline void next() { ++lineNumber_; }

	Q_REQUIRED_RESULT inline bool isEmpty() const { return !tokens_.contains(lineNumber_); }
	Q_REQUIRED_RESULT inline RESERVE getType() const { return currentLineTokens_.value(0).type; }
	template <typename T>
	Q_REQUIRED_RESULT inline T getToken(int index) const
	{
		if (currentLineTokens_.contains(index))
		{
			QVariant data = currentLineTokens_.value(index).data;
			if (data.isValid())
				return currentLineTokens_.value(index).data.value<T>();
		}
		//如果是整數返回 -1
		if (std::is_same<T, int>::value)
			return 0;
		return T();
	}
	Q_REQUIRED_RESULT inline RESERVE getTokenType(int index) const { return currentLineTokens_.value(index).type; }
	Q_REQUIRED_RESULT TokenMap getCurrentTokens() const { return currentLineTokens_; }
	void variableCalculate(const QString& varName, RESERVE op, QVariant* var, const QVariant& varValue);
	int matchLineFromLabel(const QString& label) const { return labels_.value(label, -1); }

	Q_REQUIRED_RESULT inline QVariantList& getArgsRef()
	{
		if (!callArgsStack_.isEmpty())
			return callArgsStack_.top();
		else
			return emptyArgs_;
	}

private:
	bool usestate = false;
	std::atomic_bool isStop_ = false; //是否停止
	QHash<int, TokenMap> tokens_;//當前運行腳本的每一行token
	QVariantHash variables_;//所有用腳本變量
	QHash<QString, int> labels_;//所有標記所在行記錄
	QHash<QString, CommandRegistry> commandRegistry_;//所有命令的函數指針
	QStack<int> callStack_;//"調用"命令所在行棧
	QStack<int> jmpStack_; //"跳轉"命令所在行棧
	QStack<QVariantList> callArgsStack_;//"調用"命令參數棧
	QVariantList emptyArgs_;//空參數
	QStack<QHash<QString, QVariant>> labalVarStack_;//標籤變量名
	QHash<QString, QVariant> emptyLabelVars_;//空標籤變量名

	TokenMap currentLineTokens_; //當前行token
	RESERVE currentType_ = TK_UNK; //當前行第一個token類型
	int lineNumber_ = 0; //當前行號

	ParserError lastError_ = kNoError; //最後一次錯誤

	ParserCallBack callBack_ = nullptr; //解析腳本回調函數

	Mode mode_ = kSync; //解析模式

};